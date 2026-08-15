#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <span>

namespace wide_eye::render {

[[nodiscard]] bool write_png_rgba8(const std::filesystem::path& output_path, std::uint32_t width,
                                   std::uint32_t height, std::span<const std::uint8_t> pixels,
                                   std::ostream& diagnostics);

} // namespace wide_eye::render
