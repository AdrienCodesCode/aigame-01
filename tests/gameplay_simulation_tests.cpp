#include "core/runtime.hpp"
#include "game/gameplay_replay.hpp"
#include "game/gameplay_simulation.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <span>
#include <string>
#include <type_traits>

std::size_t g_allocation_count = 0;

void* operator new(std::size_t size) {
    ++g_allocation_count;
    if (void* memory = std::malloc(size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}

namespace {

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "gameplay_simulation_failure=" << name << '\n';
    }
    return condition;
}

wide_eye::game::GameplayTickInput input_for_tick(std::uint64_t tick) {
    if (tick < 20) {
        return {.dog_move = wide_eye::game::DogMoveInput{.world_z = -1.0}};
    }
    if (tick < 40) {
        return {.dog_move = wide_eye::game::DogMoveInput{.world_x = 0.5, .world_z = -1.0}};
    }
    return {.dog_move =
                wide_eye::game::DogMoveInput{.world_x = -1.0, .world_z = 0.25, .sprint = true}};
}

struct CadenceResult {
    wide_eye::game::GameplaySnapshot snapshot{};
    std::uint64_t scheduled_ticks = 0;
};

CadenceResult run_cadence(const wide_eye::game::DogScenarioDefinition& scenario,
                          std::span<const std::chrono::nanoseconds> frame_deltas) {
    wide_eye::core::FixedStepAccumulator scheduler;
    wide_eye::game::GameplaySimulation simulation{scenario};

    for (const std::chrono::nanoseconds frame_delta : frame_deltas) {
        const wide_eye::core::FixedStepUpdate update = scheduler.advance(frame_delta);
        for (std::uint32_t index = 0; index < update.ticks; ++index) {
            simulation.fixed_update(input_for_tick(simulation.current_snapshot().tick));
        }

        const auto before_observation = simulation.current_snapshot();
        static_cast<void>(simulation.interpolated_snapshot(update.interpolation_alpha));
        if (simulation.current_snapshot() != before_observation) {
            return {};
        }
    }

    return {.snapshot = simulation.current_snapshot(), .scheduled_ticks = scheduler.total_ticks()};
}

wide_eye::game::GameplayReplay sample_replay(const wide_eye::game::GameplaySimulation& simulation) {
    return {
        .scenario = wide_eye::game::gameplay_scenario_seed(simulation),
        .actions =
            {
                {.tick = 0, .input = {.dog_move = wide_eye::game::DogMoveInput{.world_z = -1.0}}},
                {.tick = 1,
                 .input = {.dog_move = wide_eye::game::DogMoveInput{.world_x = 0.25,
                                                                    .world_z = -0.5,
                                                                    .sprint = true}}},
                {.tick = 2, .input = {}},
            },
    };
}

} // namespace

int main() {
    using namespace std::chrono_literals;

    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepState>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepStateBuffer>);

    const auto scenario = wide_eye::game::find_dog_scenario("paddock-start");
    if (!check(scenario.has_value(), "scenario_available") ||
        !check(wide_eye::game::GameplaySimulation::kTicksPerSecond ==
                   wide_eye::core::FixedStepAccumulator::ticks_per_second,
               "single_fixed_rate_definition")) {
        return EXIT_FAILURE;
    }

    std::array<std::chrono::nanoseconds, 100> fine_frames{};
    fine_frames.fill(10ms);
    std::array<std::chrono::nanoseconds, 10> coarse_frames{};
    coarse_frames.fill(100ms);

    const CadenceResult fine = run_cadence(*scenario, fine_frames);
    const CadenceResult coarse = run_cadence(*scenario, coarse_frames);
    if (!check(fine.scheduled_ticks == 60 && coarse.scheduled_ticks == 60,
               "one_second_schedules_sixty_ticks") ||
        !check(fine.snapshot.tick == 60 && coarse.snapshot.tick == 60,
               "gameplay_consumes_every_scheduled_tick") ||
        !check(fine.snapshot == coarse.snapshot, "authoritative_state_ignores_render_cadence")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation simulation{*scenario};
    const wide_eye::game::GameplaySnapshot initial = simulation.current_snapshot();
    bool initial_sheep_valid = initial.sheep.size() == wide_eye::game::kGameplaySheepCount;
    for (std::size_t index = 0; index < initial.sheep.size(); ++index) {
        const wide_eye::game::SheepState& sheep = initial.sheep[index];
        initial_sheep_valid = initial_sheep_valid && sheep.id == index + 1 &&
                              sheep.behavior == wide_eye::game::SheepBehaviorState::settled &&
                              sheep.arousal == 0.0 && sheep.velocity == wide_eye::game::Vec3{} &&
                              sheep.grounded;
        if (index > 0) {
            initial_sheep_valid =
                initial_sheep_valid && &initial.sheep[index] == &initial.sheep[index - 1] + 1;
        }
    }
    if (!check(initial_sheep_valid, "five_contiguous_stable_settled_sheep")) {
        return EXIT_FAILURE;
    }

    simulation.fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_z = -1.0}});
    if (!check(simulation.previous_snapshot() == initial, "previous_snapshot_is_prior_tick") ||
        !check(simulation.current_snapshot().tick == 1 &&
                   simulation.current_snapshot().dog != initial.dog,
               "fixed_update_publishes_current_tick") ||
        !check(simulation.previous_snapshot().sheep == initial.sheep &&
                   simulation.current_snapshot().sheep == initial.sheep,
               "sheep_next_state_reads_immutable_prior") ||
        !check(simulation.interpolated_snapshot(0.0).dog == initial.dog &&
                   simulation.interpolated_snapshot(1.0).dog == simulation.current_snapshot().dog,
               "render_interpolation_is_read_only")) {
        return EXIT_FAILURE;
    }

    const wide_eye::game::SheepState interpolation_previous{
        .id = 3,
        .position = {.x = 0.0, .y = 1.0, .z = 2.0},
        .heading_radians = 3.0,
        .arousal = 0.2,
        .behavior = wide_eye::game::SheepBehaviorState::settled,
    };
    const wide_eye::game::SheepState interpolation_current{
        .id = 3,
        .position = {.x = 10.0, .y = 3.0, .z = 6.0},
        .heading_radians = -3.0,
        .arousal = 0.8,
        .behavior = wide_eye::game::SheepBehaviorState::driven,
        .grounded = true,
    };
    const wide_eye::game::SheepState interpolation_midpoint =
        wide_eye::game::interpolate_sheep_state(interpolation_previous, interpolation_current, 0.5);
    if (!check(interpolation_midpoint.id == 3 && interpolation_midpoint.position.x == 5.0 &&
                   interpolation_midpoint.position.y == 2.0 &&
                   interpolation_midpoint.position.z == 4.0 &&
                   std::abs(interpolation_midpoint.heading_radians - 3.14159265358979323846) <
                       1.0e-12 &&
                   std::abs(interpolation_midpoint.arousal - 0.5) < 1.0e-12 &&
                   interpolation_midpoint.behavior == wide_eye::game::SheepBehaviorState::driven &&
                   interpolation_midpoint.grounded,
               "sheep_interpolation_preserves_identity_and_discrete_current_state")) {
        return EXIT_FAILURE;
    }

    const auto dog_before_suspension = simulation.current_snapshot().dog;
    simulation.fixed_update({});
    if (!check(simulation.current_snapshot().tick == 2,
               "suspended_motor_still_advances_authoritative_tick") ||
        !check(simulation.current_snapshot().dog == dog_before_suspension,
               "suspended_motor_preserves_dog_state")) {
        return EXIT_FAILURE;
    }

    simulation.restart();
    if (!check(simulation.current_snapshot() == initial &&
                   simulation.previous_snapshot() == initial,
               "restart_restores_coherent_snapshots") ||
        !check(simulation.restart_count() == 1, "restart_count_preserved")) {
        return EXIT_FAILURE;
    }

    const std::size_t allocations_before_updates = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        simulation.fixed_update(input_for_tick(tick));
    }
    const std::size_t steady_state_allocations = g_allocation_count - allocations_before_updates;
    if (!check(steady_state_allocations == 0, "fixed_updates_do_not_allocate_per_agent")) {
        return EXIT_FAILURE;
    }
    simulation.restart();

    const auto motion_scenario = wide_eye::game::find_dog_scenario("presentation-motion");
    if (!check(motion_scenario.has_value() && motion_scenario->presentation_only_sheep_motion,
               "named_presentation_motion_fixture_available")) {
        return EXIT_FAILURE;
    }
    wide_eye::game::GameplaySimulation motion_a{*motion_scenario};
    wide_eye::game::GameplaySimulation motion_b{*motion_scenario};
    const auto motion_initial = motion_a.current_snapshot();
    for (std::uint64_t tick = 0; tick < 61; ++tick) {
        motion_a.fixed_update({});
        motion_b.fixed_update({});
    }
    const auto motion_mid_turn = motion_a.interpolated_snapshot(0.5);
    bool all_sheep_scripted = true;
    for (std::size_t index = 0; index < motion_mid_turn.sheep.size(); ++index) {
        const auto& sheep = motion_mid_turn.sheep[index];
        all_sheep_scripted = all_sheep_scripted && sheep.id == index + 1 &&
                             sheep.position.z < motion_initial.sheep[index].position.z &&
                             sheep.position.x > motion_initial.sheep[index].position.x &&
                             sheep.behavior == wide_eye::game::SheepBehaviorState::settled &&
                             sheep.arousal == 0.0 && sheep.grounded;
    }
    if (!check(motion_a.current_snapshot() == motion_b.current_snapshot(),
               "presentation_motion_repeats_exactly") ||
        !check(all_sheep_scripted, "presentation_motion_moves_all_without_behavior") ||
        !check(std::abs(motion_mid_turn.sheep.front().heading_radians -
                        0.25 * 3.14159265358979323846) < 1.0e-12,
               "presentation_motion_interpolates_turn") ||
        !check(motion_a.previous_snapshot().sheep.front().position.x <
                   motion_a.current_snapshot().sheep.front().position.x,
               "presentation_motion_publishes_prior_and_current")) {
        return EXIT_FAILURE;
    }
    motion_a.restart();
    if (!check(motion_a.current_snapshot() == motion_initial &&
                   motion_a.previous_snapshot() == motion_initial,
               "presentation_motion_restart_is_exact")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation replay_a{*scenario};
    wide_eye::game::GameplaySimulation replay_b{*scenario};
    const wide_eye::game::GameplayReplay replay = sample_replay(replay_a);
    const auto replay_text = wide_eye::game::gameplay_replay_json(replay);
    if (!check(wide_eye::game::kGameplaySeedFormatVersion == 1 &&
                   wide_eye::game::kGameplayActionInputFormatVersion == 1 &&
                   wide_eye::game::kGameplayReplayFormatVersion == 1 &&
                   wide_eye::game::kGameplayStateDumpFormatVersion == 2,
               "contract_versions_are_explicit") ||
        !check(replay_text &&
                   replay_text.text ==
                       "{\"schema\":\"wide-eye.gameplay-replay\",\"version\":1,"
                       "\"tick_rate\":60,\"action_input_version\":1,\"scenario\":{"
                       "\"seed_format_version\":1,\"id\":\"paddock-start\",\"version\":1,"
                       "\"seed\":0},\"actions\":[{\"tick\":0,\"dog_move\":{"
                       "\"world_x\":0,\"world_z\":-1,\"sprint\":false}},{\"tick\":1,"
                       "\"dog_move\":{\"world_x\":0.25,\"world_z\":-0.5,\"sprint\":true}},"
                       "{\"tick\":2,\"dog_move\":null}]}",
               "canonical_replay_json") ||
        !check(wide_eye::game::apply_gameplay_replay(replay_a, replay) ==
                       wide_eye::game::GameplayContractError::none &&
                   wide_eye::game::apply_gameplay_replay(replay_b, replay) ==
                       wide_eye::game::GameplayContractError::none &&
                   replay_a.current_snapshot() == replay_b.current_snapshot(),
               "repeated_local_replay_state_equal")) {
        return EXIT_FAILURE;
    }

    const auto state_a = wide_eye::game::gameplay_state_dump_json(replay_a);
    const auto state_b = wide_eye::game::gameplay_state_dump_json(replay_b);
    if (!check(state_a && state_b && state_a.text == state_b.text,
               "canonical_state_dump_repeats") ||
        !check(state_a.text.starts_with("{\"schema\":\"wide-eye.gameplay-state\",\"version\":2,"
                                        "\"tick_rate\":60,\"scenario\":{"),
               "state_dump_schema_header") ||
        !check(state_a.text.find("\"current\":{\"tick\":3") != std::string::npos,
               "state_dump_contains_authoritative_tick") ||
        !check(state_a.text.find("\"sheep\":[{\"id\":1") != std::string::npos &&
                   state_a.text.find("\"id\":5") != std::string::npos &&
                   state_a.text.find("\"behavior\":\"settled\"") != std::string::npos,
               "state_dump_contains_five_sheep_state")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplayReplay incompatible = replay;
    incompatible.format_version += 1;
    const auto before_rejection = replay_a.current_snapshot();
    if (!check(wide_eye::game::apply_gameplay_replay(replay_a, incompatible) ==
                   wide_eye::game::GameplayContractError::unsupported_replay_version,
               "unsupported_replay_version_rejected") ||
        !check(replay_a.current_snapshot() == before_rejection,
               "rejected_replay_does_not_mutate_state") ||
        !check(wide_eye::game::gameplay_contract_error_name(
                   wide_eye::game::GameplayContractError::unsupported_replay_version) ==
                   "unsupported_replay_version",
               "compatibility_error_is_named")) {
        return EXIT_FAILURE;
    }

    incompatible = replay;
    incompatible.action_input_format_version += 1;
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible) ==
                   wide_eye::game::GameplayContractError::unsupported_action_input_version,
               "unsupported_action_version_rejected")) {
        return EXIT_FAILURE;
    }
    incompatible = replay;
    incompatible.scenario.format_version += 1;
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible) ==
                   wide_eye::game::GameplayContractError::unsupported_seed_version,
               "unsupported_seed_version_rejected")) {
        return EXIT_FAILURE;
    }
    incompatible = replay;
    incompatible.scenario.scenario = static_cast<wide_eye::game::DogScenarioId>(255);
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible) ==
                   wide_eye::game::GameplayContractError::unknown_scenario,
               "unknown_scenario_rejected")) {
        return EXIT_FAILURE;
    }
    incompatible = replay;
    incompatible.scenario.scenario_version = 0;
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible) ==
                   wide_eye::game::GameplayContractError::invalid_scenario_version,
               "zero_scenario_version_rejected")) {
        return EXIT_FAILURE;
    }
    incompatible = replay;
    incompatible.ticks_per_second -= 1;
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible) ==
                   wide_eye::game::GameplayContractError::tick_rate_mismatch,
               "tick_rate_mismatch_rejected")) {
        return EXIT_FAILURE;
    }
    incompatible = replay;
    incompatible.actions[1].tick = 4;
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible) ==
                   wide_eye::game::GameplayContractError::non_contiguous_action_tick,
               "non_contiguous_tick_rejected")) {
        return EXIT_FAILURE;
    }
    incompatible = replay;
    incompatible.actions[0].input.dog_move->world_x = std::numeric_limits<double>::quiet_NaN();
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible) ==
                       wide_eye::game::GameplayContractError::invalid_action_value &&
                   !wide_eye::game::gameplay_replay_json(incompatible),
               "non_finite_action_rejected")) {
        return EXIT_FAILURE;
    }
    incompatible = replay;
    incompatible.actions[0].input.dog_move->world_z = 1.01;
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible) ==
                   wide_eye::game::GameplayContractError::invalid_action_value,
               "out_of_range_action_rejected")) {
        return EXIT_FAILURE;
    }
    incompatible = replay;
    incompatible.scenario.seed = 99;
    wide_eye::game::GameplaySimulation fresh_simulation{*scenario};
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible, fresh_simulation) ==
                   wide_eye::game::GameplayContractError::scenario_mismatch,
               "scenario_seed_mismatch_rejected")) {
        return EXIT_FAILURE;
    }
    if (!check(wide_eye::game::validate_gameplay_replay(replay, replay_a) ==
                   wide_eye::game::GameplayContractError::simulation_not_at_replay_start,
               "replay_requires_initial_tick")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::DogScenarioDefinition invalid_state_scenario = *scenario;
    invalid_state_scenario.initial_state.position.x = std::numeric_limits<double>::quiet_NaN();
    const wide_eye::game::GameplaySimulation invalid_state_simulation{invalid_state_scenario};
    if (!check(wide_eye::game::gameplay_state_dump_json(invalid_state_simulation).error ==
                   wide_eye::game::GameplayContractError::non_finite_state,
               "state_dump_rejects_non_finite_json")) {
        return EXIT_FAILURE;
    }
    invalid_state_scenario = *scenario;
    invalid_state_scenario.id = static_cast<wide_eye::game::DogScenarioId>(255);
    const wide_eye::game::GameplaySimulation unknown_state_simulation{invalid_state_scenario};
    if (!check(wide_eye::game::gameplay_state_dump_json(unknown_state_simulation).error ==
                   wide_eye::game::GameplayContractError::unknown_scenario,
               "state_dump_rejects_unknown_scenario")) {
        return EXIT_FAILURE;
    }

    std::cout << "authoritative_tick_hz=" << wide_eye::game::GameplaySimulation::kTicksPerSecond
              << '\n'
              << "fine_render_frames=" << fine_frames.size() << '\n'
              << "coarse_render_frames=" << coarse_frames.size() << '\n'
              << "authoritative_ticks=" << fine.snapshot.tick << '\n'
              << "cadence_state_equal=yes\n"
              << "replay_contract_version=" << wide_eye::game::kGameplayReplayFormatVersion << '\n'
              << "state_dump_contract_version=" << wide_eye::game::kGameplayStateDumpFormatVersion
              << '\n'
              << "steady_state_allocations=" << steady_state_allocations << '\n'
              << "presentation_motion_fixture=scripted_non_behavior\n"
              << "repeated_local_replay_equal=yes\n"
              << "gameplay_simulation_result=pass\n";
    return EXIT_SUCCESS;
}
