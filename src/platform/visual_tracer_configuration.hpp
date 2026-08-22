#pragma once

#include "core/performance.hpp"
#include "game/camera_controller.hpp"
#include "game/gameplay_scenario.hpp"
#include "render/opengl_renderer.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace wide_eye::platform {

enum class VisualTracerCamera : std::uint8_t {
    representative,
    holdout,
};

struct VisualTracerConfiguration {
    std::string_view id;
    std::uint32_t version;
    std::string_view gameplay_scenario;
    std::string_view route_id;
    std::uint32_t route_version;
    std::uint64_t reference_tick;
    std::array<std::uint64_t, 3> motion_ticks;
    std::string_view graphics_profile;
    int provisional_viewport_width;
    int provisional_viewport_height;
    int provisional_refresh_hz;
    game::CameraState representative_camera_state;
    render::CameraPose holdout_camera;
    core::PerformanceBudget performance_budget;
};

// The single approved Phase 0 scene/configuration seam. It describes review
// inputs only: gameplay remains owned by the named scenario and renderer state
// remains owned by OpenGlRenderer.
[[nodiscard]] std::optional<VisualTracerConfiguration>
find_visual_tracer_configuration(std::string_view id) noexcept;

[[nodiscard]] std::optional<VisualTracerCamera>
find_visual_tracer_camera(std::string_view name) noexcept;
[[nodiscard]] std::string_view visual_tracer_camera_name(VisualTracerCamera camera) noexcept;

[[nodiscard]] game::GameplayTickInput visual_tracer_input_for_tick(std::uint64_t tick) noexcept;
[[nodiscard]] render::CameraPose
visual_tracer_camera_pose(const VisualTracerConfiguration& configuration, VisualTracerCamera camera,
                          const game::GameplaySnapshot& snapshot) noexcept;

[[nodiscard]] bool is_valid_visual_tracer_run(const VisualTracerConfiguration& configuration,
                                              std::string_view graphics_profile, int viewport_width,
                                              int viewport_height, int refresh_hz) noexcept;

} // namespace wide_eye::platform
