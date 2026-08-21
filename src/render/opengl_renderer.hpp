#pragma once

#include "render/influence_debug_view.hpp"
#include "render/sheep_proxy.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <vector>

namespace wide_eye::voxel {
struct ChunkMesh;
class PaddockPalette;
} // namespace wide_eye::voxel

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

enum class HandcraftedPaddockView : std::uint8_t {
    normal,
    chunk_bounds,
    face_normals,
    wireframe,
    mesh_statistics,
};

struct CameraPose {
    std::array<float, 3> eye{40.0F, 24.0F, 46.0F};
    std::array<float, 3> target{15.5F, 2.5F, 14.5F};
};

struct DogRenderPose {
    std::array<float, 3> ground_position{};
    float heading_radians = 0.0F;
};

struct HandcraftedPaddockFrame {
    HandcraftedPaddockView view = HandcraftedPaddockView::normal;
    CameraPose camera{};
    std::optional<DogRenderPose> dog;
    SheepProxyPoseBuffer sheep{};
    std::size_t sheep_count = 0;
};

// Owns OpenGL resources, draw submission, and framebuffer readback for the
// current context. The context and generated loader must remain available
// through destruction.
class OpenGlRenderer {
  public:
    OpenGlRenderer();
    ~OpenGlRenderer();

    OpenGlRenderer(const OpenGlRenderer&) = delete;
    OpenGlRenderer& operator=(const OpenGlRenderer&) = delete;
    OpenGlRenderer(OpenGlRenderer&&) = delete;
    OpenGlRenderer& operator=(OpenGlRenderer&&) = delete;

    [[nodiscard]] bool initialize(std::ostream& diagnostics);
    [[nodiscard]] bool upload_handcrafted_paddock(const voxel::ChunkMesh& mesh,
                                                  const voxel::PaddockPalette& palette,
                                                  std::size_t source_chunk_count,
                                                  std::size_t occupied_block_count,
                                                  std::ostream& diagnostics);
    void render_triangle(int pixel_width, int pixel_height) const;
    void render_voxel_cube(int pixel_width, int pixel_height) const;
    void render_voxel_cube_wireframe(int pixel_width, int pixel_height) const;
    void render_handcrafted_paddock(int pixel_width, int pixel_height,
                                    HandcraftedPaddockFrame frame = {}) const;
    // Draws one already-built influence debug frame over whatever is in the
    // colour and depth buffers. It is a separate call rather than another
    // `HandcraftedPaddockFrame` field because the frame is a few hundred
    // segments and that structure is passed by value; the overlay is also a
    // genuinely separate pass, drawn after the scene it explains.
    void render_influence_debug_overlay(int pixel_width, int pixel_height, const CameraPose& camera,
                                        const InfluenceDebugFrame& frame) const;
    [[nodiscard]] TriangleSample sample_triangle_center(int pixel_width, int pixel_height) const;
    [[nodiscard]] VoxelCubeSample sample_voxel_cube_center(int pixel_width, int pixel_height) const;
    [[nodiscard]] VoxelCubeSample sample_handcrafted_paddock_center(int pixel_width,
                                                                    int pixel_height) const;
    [[nodiscard]] std::optional<Rgba8Frame> capture_rgba8(int pixel_width, int pixel_height,
                                                          std::ostream& diagnostics) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] bool is_expected_triangle_sample(const TriangleSample& sample);
[[nodiscard]] bool is_expected_voxel_cube_sample(const VoxelCubeSample& sample);
[[nodiscard]] bool is_expected_handcrafted_paddock_sample(const VoxelCubeSample& sample);
[[nodiscard]] std::size_t count_handcrafted_paddock_debug_pixels(const Rgba8Frame& frame,
                                                                 HandcraftedPaddockView view);
[[nodiscard]] bool is_expected_handcrafted_paddock_debug_frame(const Rgba8Frame& frame,
                                                               HandcraftedPaddockView view);
[[nodiscard]] std::size_t count_voxel_cube_wireframe_pixels(const Rgba8Frame& frame);
[[nodiscard]] bool is_expected_voxel_cube_wireframe(const Rgba8Frame& frame);
// Per-lane framebuffer evidence: how many pixels of the readback carry each
// channel's colour. The overlay is unlit, so a fragment's colour is exactly its
// vertex colour and a narrow tolerance band around each entry separates the
// eight lanes. This is what turns "the capture has some lines on it" into "the
// dog-pressure term is visible in this capture and the avoidance term is not".
[[nodiscard]] std::array<std::size_t, kInfluenceChannelCount>
count_influence_debug_channel_pixels(const Rgba8Frame& frame);
[[nodiscard]] bool is_expected_influence_debug_frame(const Rgba8Frame& frame);

} // namespace wide_eye::render
