#include "render/png_writer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <ostream>
#include <span>
#include <vector>

namespace wide_eye::render {
namespace {

constexpr std::array<std::uint8_t, 8> kPngSignature{
    0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU,
};
constexpr std::size_t kBytesPerPixel = 4;
constexpr std::size_t kMaximumStoredBlockSize = 65'535;
constexpr std::uint32_t kAdlerModulus = 65'521U;

void append_big_endian(std::vector<std::uint8_t>& output, std::uint32_t value) {
    output.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

std::uint32_t crc32(std::span<const std::uint8_t> bytes) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const std::uint8_t byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

std::uint32_t adler32(std::span<const std::uint8_t> bytes) {
    std::uint32_t first = 1U;
    std::uint32_t second = 0U;
    for (const std::uint8_t byte : bytes) {
        first = (first + byte) % kAdlerModulus;
        second = (second + first) % kAdlerModulus;
    }
    return (second << 16U) | first;
}

void append_chunk(std::vector<std::uint8_t>& output, const std::array<std::uint8_t, 4>& type,
                  std::span<const std::uint8_t> data) {
    append_big_endian(output, static_cast<std::uint32_t>(data.size()));
    const std::size_t crc_begin = output.size();
    output.insert(output.end(), type.begin(), type.end());
    output.insert(output.end(), data.begin(), data.end());
    append_big_endian(output, crc32(std::span{output}.subspan(crc_begin)));
}

bool calculate_image_sizes(std::uint32_t width, std::uint32_t height, std::size_t pixel_count,
                           std::size_t& row_bytes, std::size_t& filtered_bytes,
                           std::ostream& diagnostics) {
    if (width == 0U || height == 0U) {
        diagnostics << "png_error=invalid_dimensions width=" << width << " height=" << height
                    << '\n';
        return false;
    }

    constexpr std::size_t kMaximumSize = std::numeric_limits<std::size_t>::max();
    if (static_cast<std::uint64_t>(width) >
        static_cast<std::uint64_t>(kMaximumSize / kBytesPerPixel)) {
        diagnostics << "png_error=image_size_overflow\n";
        return false;
    }
    row_bytes = static_cast<std::size_t>(width) * kBytesPerPixel;
    if (static_cast<std::size_t>(height) > kMaximumSize / row_bytes) {
        diagnostics << "png_error=image_size_overflow\n";
        return false;
    }

    const std::size_t expected_pixels = row_bytes * static_cast<std::size_t>(height);
    if (pixel_count != expected_pixels) {
        diagnostics << "png_error=pixel_size_mismatch expected=" << expected_pixels
                    << " actual=" << pixel_count << '\n';
        return false;
    }
    if (row_bytes == kMaximumSize ||
        static_cast<std::size_t>(height) > kMaximumSize / (row_bytes + 1U)) {
        diagnostics << "png_error=filtered_size_overflow\n";
        return false;
    }
    filtered_bytes = (row_bytes + 1U) * static_cast<std::size_t>(height);

    const std::uint64_t block_count =
        (static_cast<std::uint64_t>(filtered_bytes) + kMaximumStoredBlockSize - 1U) /
        kMaximumStoredBlockSize;
    const std::uint64_t zlib_bytes =
        2U + static_cast<std::uint64_t>(filtered_bytes) + (block_count * 5U) + 4U;
    if (zlib_bytes > std::numeric_limits<std::uint32_t>::max()) {
        diagnostics << "png_error=idat_too_large\n";
        return false;
    }
    return true;
}

std::vector<std::uint8_t> make_filtered_scanlines(std::uint32_t height, std::size_t row_bytes,
                                                  std::size_t filtered_bytes,
                                                  std::span<const std::uint8_t> pixels) {
    std::vector<std::uint8_t> filtered;
    filtered.reserve(filtered_bytes);
    for (std::uint32_t row = 0; row < height; ++row) {
        filtered.push_back(0U);
        const std::size_t row_begin = static_cast<std::size_t>(row) * row_bytes;
        const auto source_row = pixels.subspan(row_begin, row_bytes);
        filtered.insert(filtered.end(), source_row.begin(), source_row.end());
    }
    return filtered;
}

std::vector<std::uint8_t> make_uncompressed_zlib(std::span<const std::uint8_t> filtered) {
    std::vector<std::uint8_t> zlib;
    const std::size_t block_count =
        (filtered.size() + kMaximumStoredBlockSize - 1U) / kMaximumStoredBlockSize;
    zlib.reserve(2U + filtered.size() + (block_count * 5U) + 4U);

    zlib.push_back(0x78U);
    zlib.push_back(0x01U);

    std::size_t offset = 0;
    while (offset < filtered.size()) {
        const std::size_t remaining = filtered.size() - offset;
        const std::size_t block_size =
            remaining < kMaximumStoredBlockSize ? remaining : kMaximumStoredBlockSize;
        const bool final_block = offset + block_size == filtered.size();
        const auto length = static_cast<std::uint16_t>(block_size);
        const auto inverse_length = static_cast<std::uint16_t>(~length);

        zlib.push_back(final_block ? 0x01U : 0x00U);
        zlib.push_back(static_cast<std::uint8_t>(length & 0xFFU));
        zlib.push_back(static_cast<std::uint8_t>((length >> 8U) & 0xFFU));
        zlib.push_back(static_cast<std::uint8_t>(inverse_length & 0xFFU));
        zlib.push_back(static_cast<std::uint8_t>((inverse_length >> 8U) & 0xFFU));
        const auto block = filtered.subspan(offset, block_size);
        zlib.insert(zlib.end(), block.begin(), block.end());
        offset += block_size;
    }

    append_big_endian(zlib, adler32(filtered));
    return zlib;
}

std::vector<std::uint8_t> encode_png_rgba8(std::uint32_t width, std::uint32_t height,
                                           std::size_t row_bytes, std::size_t filtered_bytes,
                                           std::span<const std::uint8_t> pixels) {
    const std::vector<std::uint8_t> filtered =
        make_filtered_scanlines(height, row_bytes, filtered_bytes, pixels);
    const std::vector<std::uint8_t> zlib = make_uncompressed_zlib(filtered);

    std::vector<std::uint8_t> png;
    png.reserve(kPngSignature.size() + 25U + 12U + zlib.size() + 12U);
    png.insert(png.end(), kPngSignature.begin(), kPngSignature.end());

    std::vector<std::uint8_t> header;
    header.reserve(13U);
    append_big_endian(header, width);
    append_big_endian(header, height);
    header.insert(header.end(), {8U, 6U, 0U, 0U, 0U});
    append_chunk(png, {'I', 'H', 'D', 'R'}, header);
    append_chunk(png, {'I', 'D', 'A', 'T'}, zlib);
    append_chunk(png, {'I', 'E', 'N', 'D'}, {});
    return png;
}

} // namespace

bool write_png_rgba8(const std::filesystem::path& output_path, std::uint32_t width,
                     std::uint32_t height, std::span<const std::uint8_t> pixels,
                     std::ostream& diagnostics) {
    std::size_t row_bytes = 0;
    std::size_t filtered_bytes = 0;
    if (!calculate_image_sizes(width, height, pixels.size(), row_bytes, filtered_bytes,
                               diagnostics)) {
        return false;
    }

    const std::vector<std::uint8_t> png =
        encode_png_rgba8(width, height, row_bytes, filtered_bytes, pixels);
    std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
    if (!output) {
        diagnostics << "png_error=output_open_failed path=" << output_path.string() << '\n';
        return false;
    }
    output.write(reinterpret_cast<const char*>(png.data()),
                 static_cast<std::streamsize>(png.size()));
    output.close();
    if (!output) {
        diagnostics << "png_error=output_write_failed path=" << output_path.string() << '\n';
        return false;
    }
    return true;
}

} // namespace wide_eye::render
