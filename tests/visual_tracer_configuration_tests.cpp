#include "game/gameplay_simulation.hpp"
#include "platform/visual_tracer_configuration.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

namespace {

bool check(bool condition, const char* name) {
    std::cout << name << '=' << (condition ? "pass" : "fail") << '\n';
    return condition;
}

} // namespace

int main() {
    using namespace wide_eye;

    const auto configuration =
        platform::find_visual_tracer_configuration("visual-feasibility-five-sheep-v1");
    if (!check(configuration.has_value(), "named_visual_tracer_available") ||
        !check(!platform::find_visual_tracer_configuration("unknown").has_value(),
               "unknown_visual_tracer_rejected")) {
        return EXIT_FAILURE;
    }

    const auto representative = platform::find_visual_tracer_camera("representative");
    const auto holdout = platform::find_visual_tracer_camera("holdout");
    if (!check(representative == platform::VisualTracerCamera::representative &&
                   holdout == platform::VisualTracerCamera::holdout &&
                   !platform::find_visual_tracer_camera("beauty").has_value(),
               "camera_names_are_bounded") ||
        !check(configuration->reference_tick == 30 &&
                   configuration->motion_ticks == std::array<std::uint64_t, 3>{1, 30, 90} &&
                   configuration->provisional_viewport_width == 2560 &&
                   configuration->provisional_viewport_height == 1440 &&
                   configuration->provisional_refresh_hz == 60,
               "phase0_review_inputs_are_explicit") ||
        !check(platform::is_valid_visual_tracer_run(*configuration, configuration->graphics_profile,
                                                    2560, 1440, 60) &&
                   !platform::is_valid_visual_tracer_run(*configuration, "low", 2560, 1440, 60) &&
                   !platform::is_valid_visual_tracer_run(
                       *configuration, configuration->graphics_profile, 0, 1440, 60),
               "profile_and_viewport_validation")) {
        return EXIT_FAILURE;
    }

    const auto scenario = game::find_gameplay_scenario(configuration->gameplay_scenario);
    if (!check(scenario.has_value(), "visual_tracer_gameplay_scenario_available")) {
        return EXIT_FAILURE;
    }

    const auto first = std::make_unique<game::GameplaySimulation>(*scenario);
    const auto repeat = std::make_unique<game::GameplaySimulation>(*scenario);
    for (std::uint64_t tick = 0; tick < configuration->reference_tick; ++tick) {
        const game::GameplayTickInput input = platform::visual_tracer_input_for_tick(tick);
        first->fixed_update(input);
        repeat->fixed_update(input);
    }
    const game::GameplaySnapshot& snapshot = first->current_snapshot();
    const render::CameraPose representative_pose = platform::visual_tracer_camera_pose(
        *configuration, platform::VisualTracerCamera::representative, snapshot);
    const render::CameraPose holdout_pose = platform::visual_tracer_camera_pose(
        *configuration, platform::VisualTracerCamera::holdout, snapshot);

    const game::GameplayTickInput route_0 = platform::visual_tracer_input_for_tick(0);
    const game::GameplayTickInput route_40 = platform::visual_tracer_input_for_tick(40);
    const game::GameplayTickInput route_90 = platform::visual_tracer_input_for_tick(90);
    const game::GameplayTickInput route_150 = platform::visual_tracer_input_for_tick(150);
    const bool route_segments_match =
        route_0.dog_move.has_value() && route_0.dog_move->world_z == -1.0 &&
        route_40.dog_move.has_value() && route_40.dog_move->world_x == 0.75 &&
        route_90.dog_move.has_value() && route_90.dog_move->sprint &&
        route_150.dog_move.has_value() && route_150.dog_move->world_z == 1.0;
    const bool stable_ids = snapshot.sheep.front().id == 1 && snapshot.sheep.back().id == 5;
    if (!check(first->current_snapshot() == repeat->current_snapshot(),
               "visual_tracer_route_repeats_exactly") ||
        !check(route_segments_match, "visual_tracer_route_boundaries") ||
        !check(snapshot.tick == configuration->reference_tick && stable_ids,
               "visual_tracer_publishes_five_stable_sheep") ||
        !check(representative_pose.target[0] == static_cast<float>(snapshot.dog.position.x) &&
                   representative_pose.target[2] == static_cast<float>(snapshot.dog.position.z),
               "representative_camera_follows_published_dog") ||
        !check(holdout_pose.eye == configuration->holdout_camera.eye &&
                   holdout_pose.target == configuration->holdout_camera.target,
               "holdout_camera_is_fixed")) {
        return EXIT_FAILURE;
    }

    std::cout << "visual_tracer_configuration_result=pass\n";
    return EXIT_SUCCESS;
}
