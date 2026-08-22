#include "platform/visual_tracer_configuration.hpp"

namespace wide_eye::platform {
namespace {

constexpr VisualTracerConfiguration kVisualFeasibilityFiveSheep{
    .id = "visual-feasibility-five-sheep-v1",
    .version = 1,
    .gameplay_scenario = "sheep-all-influences-diagnostic",
    .route_id = "influence-debug-route-v1",
    .route_version = 1,
    .reference_tick = 30,
    .motion_ticks = {1, 30, 90},
    .graphics_profile = "visual-feasibility-reference-high-v1",
    .provisional_viewport_width = 2560,
    .provisional_viewport_height = 1440,
    .provisional_refresh_hz = 60,
    // This freezes the existing third-person gameplay camera parameters. The
    // pose follows the deterministic dog route but does not read or change the
    // camera controller used by interactive play.
    .representative_camera_state =
        {
            .mode = game::CameraMode::gameplay,
            .gameplay_yaw = 0.0,
            .gameplay_pitch = -0.55,
        },
    // This is the existing fixed influence-review composition, retained as the
    // elevated holdout rather than tuned alongside the representative view.
    .holdout_camera = {.eye = {38.0F, 24.0F, 42.0F}, .target = {16.0F, 1.5F, 21.0F}},
    .performance_budget =
        {
            .id = "visual-feasibility-reference-high-v1",
            .synchronized_frame_p95_ns = 16'670'000,
            .synchronized_frame_p99_ns = 20'840'000,
            .peak_rss_bytes = 1'610'612'736,
        },
};

} // namespace

std::optional<VisualTracerConfiguration>
find_visual_tracer_configuration(std::string_view id) noexcept {
    if (id == kVisualFeasibilityFiveSheep.id) {
        return kVisualFeasibilityFiveSheep;
    }
    return std::nullopt;
}

std::optional<VisualTracerCamera> find_visual_tracer_camera(std::string_view name) noexcept {
    if (name == "representative") {
        return VisualTracerCamera::representative;
    }
    if (name == "holdout") {
        return VisualTracerCamera::holdout;
    }
    return std::nullopt;
}

std::string_view visual_tracer_camera_name(VisualTracerCamera camera) noexcept {
    switch (camera) {
    case VisualTracerCamera::representative:
        return "representative";
    case VisualTracerCamera::holdout:
        return "holdout";
    }
    return "unknown";
}

game::GameplayTickInput visual_tracer_input_for_tick(std::uint64_t tick) noexcept {
    if (tick < 40) {
        return {.dog_move = game::DogMoveInput{.world_z = -1.0}};
    }
    if (tick < 90) {
        return {.dog_move = game::DogMoveInput{.world_x = 0.75, .world_z = -0.5}};
    }
    if (tick < 150) {
        return {.dog_move = game::DogMoveInput{.world_x = -1.0, .world_z = 0.5, .sprint = true}};
    }
    return {.dog_move = game::DogMoveInput{.world_x = 0.25, .world_z = 1.0}};
}

render::CameraPose visual_tracer_camera_pose(const VisualTracerConfiguration& configuration,
                                             VisualTracerCamera camera,
                                             const game::GameplaySnapshot& snapshot) noexcept {
    if (camera == VisualTracerCamera::holdout) {
        return configuration.holdout_camera;
    }

    const game::CameraPose pose =
        game::camera_pose(snapshot.dog, configuration.representative_camera_state);
    return {
        .eye = {static_cast<float>(pose.eye.x), static_cast<float>(pose.eye.y),
                static_cast<float>(pose.eye.z)},
        .target = {static_cast<float>(pose.target.x), static_cast<float>(pose.target.y),
                   static_cast<float>(pose.target.z)},
    };
}

bool is_valid_visual_tracer_run(const VisualTracerConfiguration& configuration,
                                std::string_view graphics_profile, int viewport_width,
                                int viewport_height, int refresh_hz) noexcept {
    return graphics_profile == configuration.graphics_profile && viewport_width > 0 &&
           viewport_height > 0 && refresh_hz > 0;
}

} // namespace wide_eye::platform
