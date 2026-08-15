#include "render/png_writer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr std::array<std::uint8_t, 73> kExpectedRedPixelPng{
    0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U, 0x0DU, 0x49U,
    0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x01U, 0x08U, 0x06U,
    0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U, 0x00U, 0x00U, 0x00U, 0x10U, 0x49U, 0x44U,
    0x41U, 0x54U, 0x78U, 0x01U, 0x01U, 0x05U, 0x00U, 0xFAU, 0xFFU, 0x00U, 0xFFU, 0x00U, 0x00U,
    0xFFU, 0x05U, 0x00U, 0x01U, 0xFFU, 0xFAU, 0x5CU, 0x88U, 0xD1U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
};

int fail(const char* stage) {
    std::cerr << "png_writer_result=fail\n"
              << "failure_stage=" << stage << '\n';
    return EXIT_FAILURE;
}

std::vector<std::uint8_t> read_bytes(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return fail("arguments");
    }

    const std::filesystem::path output_path{argv[1]};
    constexpr std::array<std::uint8_t, 4> kRedPixel{0xFFU, 0x00U, 0x00U, 0xFFU};
    std::ostringstream diagnostics;
    if (!wide_eye::render::write_png_rgba8(output_path, 1U, 1U, kRedPixel, diagnostics)) {
        std::cerr << diagnostics.str();
        return fail("write_known_pixel");
    }

    const std::vector<std::uint8_t> encoded = read_bytes(output_path);
    if (!std::equal(encoded.begin(), encoded.end(), kExpectedRedPixelPng.begin(),
                    kExpectedRedPixelPng.end())) {
        return fail("known_png_bytes");
    }

    diagnostics.str({});
    const std::span<const std::uint8_t> incomplete_pixels{kRedPixel.data(), 3U};
    if (wide_eye::render::write_png_rgba8(output_path, 1U, 1U, incomplete_pixels, diagnostics) ||
        diagnostics.str().find("png_error=pixel_size_mismatch") == std::string::npos) {
        return fail("invalid_pixel_count");
    }

    std::cout << "png_writer_known_bytes=yes\n"
              << "png_writer_invalid_input_rejected=yes\n"
              << "png_writer_result=pass\n";
    return EXIT_SUCCESS;
}
