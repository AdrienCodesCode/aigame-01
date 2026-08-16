#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

namespace wide_eye::platform {

[[nodiscard]] int run_interactive_scenario(std::string_view dog_scenario = "paddock-start");
[[nodiscard]] int run_dog_headless_scenario(std::string_view dog_scenario);
[[nodiscard]] int
run_dog_render_scenario(std::string_view dog_scenario,
                        const std::optional<std::filesystem::path>& capture_path = std::nullopt);
[[nodiscard]] int run_sheep_motion_render_scenario(
    const std::optional<std::filesystem::path>& capture_path = std::nullopt,
    std::uint64_t capture_tick = 61, bool debug_view = false,
    const std::optional<std::filesystem::path>& state_dump_path = std::nullopt);
[[nodiscard]] int run_sheep_motion_performance_scenario();
[[nodiscard]] int run_window_smoke_scenario();
[[nodiscard]] int run_context_smoke_scenario();
[[nodiscard]] int run_triangle_smoke_scenario();
[[nodiscard]] int
run_voxel_cube_smoke_scenario(const std::optional<std::filesystem::path>& capture_path);
[[nodiscard]] int
run_voxel_cube_debug_smoke_scenario(const std::optional<std::filesystem::path>& capture_path);
[[nodiscard]] int
run_handcrafted_paddock_scenario(const std::optional<std::filesystem::path>& capture_path);
[[nodiscard]] int run_handcrafted_paddock_performance_scenario();
[[nodiscard]] int run_handcrafted_paddock_chunk_bounds_scenario(
    const std::optional<std::filesystem::path>& capture_path);
[[nodiscard]] int run_handcrafted_paddock_face_normals_scenario(
    const std::optional<std::filesystem::path>& capture_path);
[[nodiscard]] int run_handcrafted_paddock_wireframe_scenario(
    const std::optional<std::filesystem::path>& capture_path);
[[nodiscard]] int run_handcrafted_paddock_mesh_statistics_scenario(
    const std::optional<std::filesystem::path>& capture_path);
[[nodiscard]] int run_context_high_severity_scenario();

} // namespace wide_eye::platform
