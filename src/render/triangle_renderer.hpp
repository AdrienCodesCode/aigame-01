#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <vector>

namespace wide_eye::render {

struct TriangleSample {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};

struct VoxelCubeSample {
    TriangleSample color;
    float depth;
    bool depth_test_enabled;
    bool depth_function_less;
    bool depth_write_enabled;
};

struct Rgba8Frame {
    std::uint32_t width;
    std::uint32_t height;
    std::vector<std::uint8_t> pixels;
};

class TriangleRenderer {
  public:
    TriangleRenderer();
    ~TriangleRenderer();

    TriangleRenderer(const TriangleRenderer&) = delete;
    TriangleRenderer& operator=(const TriangleRenderer&) = delete;
    TriangleRenderer(TriangleRenderer&&) = delete;
    TriangleRenderer& operator=(TriangleRenderer&&) = delete;

    [[nodiscard]] bool initialize(std::ostream& diagnostics);
    void render(int pixel_width, int pixel_height) const;
    void render_voxel_cube(int pixel_width, int pixel_height) const;
    [[nodiscard]] TriangleSample sample_center(int pixel_width, int pixel_height) const;
    [[nodiscard]] VoxelCubeSample sample_voxel_cube_center(int pixel_width, int pixel_height) const;
    [[nodiscard]] std::optional<Rgba8Frame> capture_rgba8(int pixel_width, int pixel_height,
                                                          std::ostream& diagnostics) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] bool is_expected_triangle_sample(const TriangleSample& sample);
[[nodiscard]] bool is_expected_voxel_cube_sample(const VoxelCubeSample& sample);

} // namespace wide_eye::render
