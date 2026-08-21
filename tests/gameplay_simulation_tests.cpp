#include "core/runtime.hpp"
#include "game/flock_observables.hpp"
#include "game/gameplay_replay.hpp"
#include "game/gameplay_simulation.hpp"

#include <algorithm>
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

CadenceResult run_cadence(const wide_eye::game::GameplayScenarioDefinition& scenario,
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

const wide_eye::game::SheepState& sheep_with_id(const wide_eye::game::SheepStateBuffer& sheep,
                                                std::uint32_t id) {
    const auto member = std::find_if(sheep.begin(), sheep.end(),
                                     [id](const auto& candidate) { return candidate.id == id; });
    if (member == sheep.end()) {
        std::abort();
    }
    return *member;
}

const wide_eye::game::SheepSocialEvidence&
evidence_with_id(const wide_eye::game::SheepSocialEvidenceBuffer& evidence, std::uint32_t id) {
    const auto member = std::find_if(evidence.begin(), evidence.end(), [id](const auto& candidate) {
        return candidate.subject_id == id;
    });
    if (member == evidence.end()) {
        std::abort();
    }
    return *member;
}

const wide_eye::game::SheepDogPressureEvidence&
evidence_with_id(const wide_eye::game::SheepDogPressureEvidenceBuffer& evidence, std::uint32_t id) {
    const auto member = std::find_if(evidence.begin(), evidence.end(), [id](const auto& candidate) {
        return candidate.subject_id == id;
    });
    if (member == evidence.end()) {
        std::abort();
    }
    return *member;
}

double planar_distance(const wide_eye::game::SheepState& left,
                       const wide_eye::game::SheepState& right) {
    return std::hypot(left.position.x - right.position.x, left.position.z - right.position.z);
}

bool separation_acceleration_is_bounded(const wide_eye::game::GameplaySimulation& simulation,
                                        double maximum_acceleration) {
    const auto& previous = simulation.previous_snapshot().sheep;
    const auto& current = simulation.current_snapshot().sheep;
    for (const auto& member : current) {
        const auto& prior_member = sheep_with_id(previous, member.id);
        const double acceleration = std::hypot(member.velocity.x - prior_member.velocity.x,
                                               member.velocity.z - prior_member.velocity.z) /
                                    wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
        if (acceleration > maximum_acceleration + 1.0e-10) {
            return false;
        }
    }
    return true;
}

} // namespace

int main() {
    if (!check(
            wide_eye::game::is_known_sheep_behavior(wide_eye::game::SheepBehaviorState::settled) &&
                wide_eye::game::is_known_sheep_behavior(
                    wide_eye::game::SheepBehaviorState::recovering) &&
                !wide_eye::game::is_known_sheep_behavior(
                    static_cast<wide_eye::game::SheepBehaviorState>(255)),
            "sheep_behavior_domain_is_closed")) {
        return EXIT_FAILURE;
    }

    using namespace std::chrono_literals;

    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepState>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepStateBuffer>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepSocialEvidence>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepSocialEvidenceBuffer>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepDogPressureEvidence>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepDogPressureEvidenceBuffer>);

    const auto scenario = wide_eye::game::find_gameplay_scenario("paddock-start");
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

    const auto motion_scenario = wide_eye::game::find_gameplay_scenario("presentation-motion");
    if (!check(motion_scenario.has_value() &&
                   motion_scenario->sheep_fixture ==
                       wide_eye::game::SheepFixture::scripted_presentation_motion,
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

    const auto separation_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-only-separation");
    if (!check(separation_scenario.has_value() &&
                   separation_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_only_separation &&
                   separation_scenario->sheep_fixture ==
                       wide_eye::game::SheepFixture::local_social_response &&
                   separation_scenario->sheep_separation.enabled &&
                   !separation_scenario->sheep_attraction.enabled,
               "named_sheep_only_separation_fixture_available")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation separation{*separation_scenario};
    const auto separation_initial = separation.current_snapshot();
    if (!check(planar_distance(sheep_with_id(separation_initial.sheep, 1),
                               sheep_with_id(separation_initial.sheep, 2)) == 0.0,
               "separation_fixture_starts_with_exact_overlap")) {
        return EXIT_FAILURE;
    }

    separation.fixed_update({});
    const auto& separation_member_one_evidence =
        evidence_with_id(separation.current_snapshot().sheep_social_evidence, 1);
    if (!check(separation.previous_snapshot() == separation_initial,
               "separation_reads_immutable_prior_snapshot") ||
        !check(separation_acceleration_is_bounded(
                   separation, separation_scenario->sheep_separation.maximum_acceleration),
               "overlap_recovery_acceleration_is_bounded") ||
        !check(sheep_with_id(separation.current_snapshot().sheep, 3).velocity ==
                       wide_eye::game::Vec3{} &&
                   sheep_with_id(separation.current_snapshot().sheep, 4).velocity ==
                       wide_eye::game::Vec3{} &&
                   sheep_with_id(separation.current_snapshot().sheep, 5).velocity ==
                       wide_eye::game::Vec3{},
               "initially_out_of_range_sheep_receive_no_separation") ||
        !check(separation_member_one_evidence.separation_acceleration.x < 0.0 &&
                   separation_member_one_evidence.attraction_acceleration ==
                       wide_eye::game::Vec3{} &&
                   separation_member_one_evidence.attraction_neighbor_count == 0,
               "separation_influence_is_published_independently")) {
        return EXIT_FAILURE;
    }

    constexpr std::uint64_t kSeparationTicks = 120;
    for (std::uint64_t tick = 1; tick < kSeparationTicks; ++tick) {
        separation.fixed_update({});
        if (!check(separation_acceleration_is_bounded(
                       separation, separation_scenario->sheep_separation.maximum_acceleration),
                   "separation_acceleration_is_bounded")) {
            return EXIT_FAILURE;
        }
    }
    const auto separation_final = separation.current_snapshot();
    if (!check(separation.previous_snapshot().sheep != separation_final.sheep,
               "separation_publishes_prior_and_current") ||
        !check(planar_distance(sheep_with_id(separation_final.sheep, 1),
                               sheep_with_id(separation_final.sheep, 2)) >
                   separation_scenario->sheep_separation.radius,
               "exact_overlap_recovers_beyond_separation_radius") ||
        !check(sheep_with_id(separation_final.sheep, 1).position.x <
                       sheep_with_id(separation_initial.sheep, 1).position.x &&
                   sheep_with_id(separation_final.sheep, 2).position.x >
                       sheep_with_id(separation_initial.sheep, 2).position.x,
               "stable_ids_choose_opposite_overlap_directions")) {
        return EXIT_FAILURE;
    }

    auto reversed_separation_scenario = *separation_scenario;
    std::reverse(reversed_separation_scenario.initial_sheep.begin(),
                 reversed_separation_scenario.initial_sheep.end());
    wide_eye::game::GameplaySimulation reversed_separation{reversed_separation_scenario};
    for (std::uint64_t tick = 0; tick < kSeparationTicks; ++tick) {
        reversed_separation.fixed_update({});
    }
    for (const auto& member : separation_final.sheep) {
        if (!check(member == sheep_with_id(reversed_separation.current_snapshot().sheep, member.id),
                   "separation_result_is_stable_by_id_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    wide_eye::game::GameplaySimulation allocation_separation{*separation_scenario};
    const std::size_t separation_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_separation.fixed_update({});
    }
    const std::size_t separation_allocations = g_allocation_count - separation_allocations_before;
    if (!check(separation_allocations == 0, "separation_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    separation.restart();
    if (!check(separation.current_snapshot() == separation_initial &&
                   separation.previous_snapshot() == separation_initial,
               "separation_restart_restores_overlap_fixture")) {
        return EXIT_FAILURE;
    }

    const auto attraction_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-only-attraction");
    if (!check(attraction_scenario.has_value() &&
                   attraction_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_only_attraction &&
                   attraction_scenario->sheep_fixture ==
                       wide_eye::game::SheepFixture::local_social_response &&
                   !attraction_scenario->sheep_separation.enabled &&
                   attraction_scenario->sheep_attraction.enabled &&
                   attraction_scenario->sheep_attraction.neighbor_limit ==
                       wide_eye::game::kMaximumSelectedAttractionNeighbors,
               "named_bounded_attraction_fixture_available")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation attraction{*attraction_scenario};
    const auto attraction_initial = attraction.current_snapshot();
    attraction.fixed_update({});
    const auto attraction_after_one = attraction.current_snapshot();
    const auto& subject_one_evidence =
        evidence_with_id(attraction_after_one.sheep_social_evidence, 1);
    const auto& subject_one = sheep_with_id(attraction_after_one.sheep, 1);
    const double subject_one_acceleration_x =
        subject_one.velocity.x / wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    const double subject_one_acceleration_z =
        subject_one.velocity.z / wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    if (!check(attraction.previous_snapshot() == attraction_initial,
               "attraction_reads_immutable_prior_snapshot") ||
        !check(subject_one_evidence.attraction_candidate_count == 4 &&
                   subject_one_evidence.attraction_neighbor_count == 2,
               "dense_query_is_bounded_to_two_selected_neighbors") ||
        !check(subject_one_evidence.attraction_neighbor_ids[0] == 2 &&
                   subject_one_evidence.attraction_neighbor_ids[1] == 3,
               "attraction_neighbor_ids_publish_distance_and_id_tie_order") ||
        !check(std::abs(subject_one_evidence.attraction_acceleration.x - 0.09375) < 1.0e-12 &&
                   std::abs(subject_one_evidence.attraction_acceleration.z - 0.09375) < 1.0e-12 &&
                   subject_one_evidence.separation_acceleration == wide_eye::game::Vec3{},
               "attraction_influence_is_independent_and_exact") ||
        !check(std::abs(subject_one_acceleration_x -
                        subject_one_evidence.attraction_acceleration.x) < 1.0e-12 &&
                   std::abs(subject_one_acceleration_z -
                            subject_one_evidence.attraction_acceleration.z) < 1.0e-12,
               "published_attraction_matches_applied_acceleration")) {
        return EXIT_FAILURE;
    }

    for (const auto& evidence : attraction_after_one.sheep_social_evidence) {
        if (!check(evidence.attraction_neighbor_count <=
                           attraction_scenario->sheep_attraction.neighbor_limit &&
                       evidence.attraction_candidate_count >= evidence.attraction_neighbor_count &&
                       std::hypot(evidence.attraction_acceleration.x,
                                  evidence.attraction_acceleration.z) <=
                           attraction_scenario->sheep_attraction.maximum_acceleration + 1.0e-12,
                   "all_attraction_neighbor_sets_respect_bound")) {
            return EXIT_FAILURE;
        }
    }

    auto reversed_attraction_scenario = *attraction_scenario;
    std::reverse(reversed_attraction_scenario.initial_sheep.begin(),
                 reversed_attraction_scenario.initial_sheep.end());
    wide_eye::game::GameplaySimulation reversed_attraction{reversed_attraction_scenario};
    reversed_attraction.fixed_update({});
    for (const auto& member : attraction_after_one.sheep) {
        if (!check(member == sheep_with_id(reversed_attraction.current_snapshot().sheep, member.id),
                   "attraction_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(attraction_after_one.sheep_social_evidence, member.id) ==
                       evidence_with_id(
                           reversed_attraction.current_snapshot().sheep_social_evidence, member.id),
                   "chosen_neighbor_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto attraction_state = wide_eye::game::gameplay_state_dump_json(attraction);
    if (!check(attraction_state &&
                   attraction_state.text.find(
                       "\"subject_id\":1,\"attraction_neighbor_ids\":[2,3],"
                       "\"attraction_neighbor_count\":2,\"attraction_candidate_count\":4") !=
                       std::string::npos,
               "state_dump_contains_exact_chosen_neighbor_evidence")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation allocation_attraction{*attraction_scenario};
    const std::size_t attraction_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_attraction.fixed_update({});
    }
    const std::size_t attraction_allocations = g_allocation_count - attraction_allocations_before;
    if (!check(attraction_allocations == 0, "attraction_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    attraction.restart();
    if (!check(attraction.current_snapshot() == attraction_initial &&
                   attraction.previous_snapshot() == attraction_initial,
               "attraction_restart_restores_dense_fixture")) {
        return EXIT_FAILURE;
    }

    const auto alignment_off_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-alignment-off");
    const auto alignment_on_scenario = wide_eye::game::find_gameplay_scenario("sheep-alignment-on");
    auto alignment_on_as_control =
        alignment_on_scenario.value_or(wide_eye::game::GameplayScenarioDefinition{});
    if (alignment_off_scenario.has_value()) {
        alignment_on_as_control.id = alignment_off_scenario->id;
    }
    alignment_on_as_control.sheep_alignment.enabled = false;
    if (!check(alignment_off_scenario.has_value() && alignment_on_scenario.has_value() &&
                   alignment_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_alignment_off &&
                   alignment_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_alignment_on &&
                   alignment_on_as_control == *alignment_off_scenario &&
                   alignment_on_scenario->sheep_alignment.enabled &&
                   alignment_on_scenario->sheep_alignment.neighbor_limit ==
                       wide_eye::game::kMaximumSelectedAlignmentNeighbors,
               "paired_alignment_fixture_differs_only_by_alignment_switch")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation alignment_off{*alignment_off_scenario};
    wide_eye::game::GameplaySimulation alignment_on{*alignment_on_scenario};
    const auto alignment_initial = alignment_on.current_snapshot();
    alignment_off.fixed_update({});
    alignment_on.fixed_update({});
    const auto alignment_off_after_one = alignment_off.current_snapshot();
    const auto alignment_on_after_one = alignment_on.current_snapshot();
    const auto& alignment_evidence =
        evidence_with_id(alignment_on_after_one.sheep_social_evidence, 1);
    const auto& aligned_subject = sheep_with_id(alignment_on_after_one.sheep, 1);
    const auto& unaligned_subject = sheep_with_id(alignment_off_after_one.sheep, 1);
    if (!check(alignment_on.previous_snapshot() == alignment_initial,
               "alignment_reads_immutable_prior_snapshot") ||
        !check(alignment_evidence.alignment_candidate_count == 2 &&
                   alignment_evidence.alignment_neighbor_count == 1 &&
                   alignment_evidence.alignment_neighbor_ids[0] == 2,
               "alignment_selects_smaller_nearest_neighbor_subset") ||
        !check(alignment_evidence.separation_acceleration == wide_eye::game::Vec3{} &&
                   alignment_evidence.attraction_acceleration == wide_eye::game::Vec3{} &&
                   std::abs(alignment_evidence.alignment_acceleration.x + 1.0) < 1.0e-12 &&
                   std::abs(alignment_evidence.alignment_acceleration.z + 1.0) < 1.0e-12,
               "alignment_influence_is_independent_and_exact") ||
        !check(std::abs((aligned_subject.velocity.x - alignment_initial.sheep[0].velocity.x) /
                            wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                        alignment_evidence.alignment_acceleration.x) < 1.0e-12 &&
                   std::abs((aligned_subject.velocity.z - alignment_initial.sheep[0].velocity.z) /
                                wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                            alignment_evidence.alignment_acceleration.z) < 1.0e-12,
               "published_alignment_matches_applied_acceleration") ||
        !check(unaligned_subject.velocity == alignment_initial.sheep[0].velocity &&
                   evidence_with_id(alignment_off_after_one.sheep_social_evidence, 1)
                           .alignment_acceleration == wide_eye::game::Vec3{},
               "alignment_off_preserves_velocity_and_zeroes_evidence")) {
        return EXIT_FAILURE;
    }

    for (const auto& evidence : alignment_on_after_one.sheep_social_evidence) {
        if (!check(evidence.alignment_neighbor_count <=
                           alignment_on_scenario->sheep_alignment.neighbor_limit &&
                       evidence.alignment_candidate_count >= evidence.alignment_neighbor_count &&
                       std::hypot(evidence.alignment_acceleration.x,
                                  evidence.alignment_acceleration.z) <=
                           alignment_on_scenario->sheep_alignment.maximum_acceleration + 1.0e-12,
                   "all_alignment_neighbor_sets_and_accelerations_respect_bounds")) {
            return EXIT_FAILURE;
        }
    }

    constexpr std::uint64_t kAlignmentComparisonTicks = 60;
    for (std::uint64_t tick = 1; tick < kAlignmentComparisonTicks; ++tick) {
        alignment_off.fixed_update({});
        alignment_on.fixed_update({});
    }
    constexpr std::array<std::uint32_t, wide_eye::game::kGameplaySheepCount> kNoNeighbors{};
    const auto alignment_off_observables = wide_eye::game::compute_five_sheep_observables(
        alignment_off.current_snapshot().sheep, kNoNeighbors, 3.0);
    const auto alignment_on_observables = wide_eye::game::compute_five_sheep_observables(
        alignment_on.current_snapshot().sheep, kNoNeighbors, 3.0);
    if (!check(alignment_off_observables.has_value() && alignment_on_observables.has_value() &&
                   alignment_on_observables->polarization >
                       alignment_off_observables->polarization + 0.05,
               "alignment_on_improves_directional_agreement_over_paired_control")) {
        return EXIT_FAILURE;
    }

    auto reversed_alignment_scenario = *alignment_on_scenario;
    std::reverse(reversed_alignment_scenario.initial_sheep.begin(),
                 reversed_alignment_scenario.initial_sheep.end());
    wide_eye::game::GameplaySimulation reversed_alignment{reversed_alignment_scenario};
    for (std::uint64_t tick = 0; tick < kAlignmentComparisonTicks; ++tick) {
        reversed_alignment.fixed_update({});
    }
    for (const auto& member : alignment_on.current_snapshot().sheep) {
        if (!check(member == sheep_with_id(reversed_alignment.current_snapshot().sheep, member.id),
                   "alignment_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(alignment_on.current_snapshot().sheep_social_evidence,
                                    member.id) ==
                       evidence_with_id(reversed_alignment.current_snapshot().sheep_social_evidence,
                                        member.id),
                   "alignment_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto alignment_state = wide_eye::game::gameplay_state_dump_json(alignment_on);
    if (!check(alignment_state &&
                   alignment_state.text.find("\"alignment_neighbor_ids\":[") != std::string::npos &&
                   alignment_state.text.find("\"alignment_acceleration\":{") != std::string::npos,
               "state_dump_contains_alignment_selection_and_influence_evidence")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation allocation_alignment{*alignment_on_scenario};
    const std::size_t alignment_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_alignment.fixed_update({});
    }
    const std::size_t alignment_allocations = g_allocation_count - alignment_allocations_before;
    if (!check(alignment_allocations == 0, "alignment_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    alignment_on.restart();
    if (!check(alignment_on.current_snapshot() == alignment_initial &&
                   alignment_on.previous_snapshot() == alignment_initial,
               "alignment_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    const auto dog_pressure_off_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-dog-pressure-off");
    const auto dog_pressure_on_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-dog-pressure-on");
    auto dog_pressure_on_as_control =
        dog_pressure_on_scenario.value_or(wide_eye::game::GameplayScenarioDefinition{});
    if (dog_pressure_off_scenario.has_value()) {
        dog_pressure_on_as_control.id = dog_pressure_off_scenario->id;
    }
    dog_pressure_on_as_control.sheep_dog_pressure.enabled = false;
    if (!check(dog_pressure_off_scenario.has_value() && dog_pressure_on_scenario.has_value() &&
                   dog_pressure_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_pressure_off &&
                   dog_pressure_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_pressure_on &&
                   dog_pressure_on_as_control == *dog_pressure_off_scenario &&
                   dog_pressure_on_scenario->sheep_dog_pressure.enabled,
               "paired_dog_pressure_fixture_differs_only_by_pressure_switch")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation dog_pressure_off{*dog_pressure_off_scenario};
    wide_eye::game::GameplaySimulation dog_pressure_on{*dog_pressure_on_scenario};
    const auto dog_pressure_initial = dog_pressure_on.current_snapshot();
    dog_pressure_off.fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0}});
    dog_pressure_on.fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0}});
    const auto& dog_pressure_off_after_one = dog_pressure_off.current_snapshot();
    const auto& dog_pressure_on_after_one = dog_pressure_on.current_snapshot();
    const auto& near_pressure =
        evidence_with_id(dog_pressure_on_after_one.sheep_dog_pressure_evidence, 1);
    const auto& middle_pressure =
        evidence_with_id(dog_pressure_on_after_one.sheep_dog_pressure_evidence, 2);
    const auto& radius_boundary =
        evidence_with_id(dog_pressure_on_after_one.sheep_dog_pressure_evidence, 3);
    const auto& outside_pressure =
        evidence_with_id(dog_pressure_on_after_one.sheep_dog_pressure_evidence, 4);
    const auto& forward_dog =
        evidence_with_id(dog_pressure_on_after_one.sheep_dog_pressure_evidence, 5);
    if (!check(dog_pressure_on.previous_snapshot() == dog_pressure_initial,
               "dog_pressure_reads_immutable_prior_snapshot") ||
        !check(near_pressure.stimulus_evaluated && near_pressure.dog_distance == 2.0 &&
                   std::abs(near_pressure.dog_relative_bearing_radians + 1.57079632679489661923) <
                       1.0e-12 &&
                   std::abs(near_pressure.pressure_acceleration.x - 2.0) < 1.0e-12 &&
                   near_pressure.pressure_acceleration.z == 0.0,
               "near_dog_pressure_publishes_distance_bearing_and_away_vector") ||
        !check(middle_pressure.dog_distance == 3.0 &&
                   std::abs(middle_pressure.pressure_acceleration.x - 1.5) < 1.0e-12,
               "dog_pressure_has_linear_distance_falloff") ||
        !check(radius_boundary.dog_distance == 6.0 &&
                   radius_boundary.pressure_acceleration == wide_eye::game::Vec3{} &&
                   outside_pressure.dog_distance == 7.0 &&
                   outside_pressure.pressure_acceleration == wide_eye::game::Vec3{},
               "dog_pressure_is_zero_at_and_beyond_radius") ||
        !check(forward_dog.dog_distance == 4.0 && forward_dog.dog_relative_bearing_radians == 0.0 &&
                   forward_dog.pressure_acceleration.x == 0.0 &&
                   std::abs(forward_dog.pressure_acceleration.z - 1.0) < 1.0e-12,
               "dog_pressure_direction_uses_prior_planar_geometry")) {
        return EXIT_FAILURE;
    }

    for (const auto& on_evidence : dog_pressure_on_after_one.sheep_dog_pressure_evidence) {
        const auto& off_evidence = evidence_with_id(
            dog_pressure_off_after_one.sheep_dog_pressure_evidence, on_evidence.subject_id);
        const auto& current_member =
            sheep_with_id(dog_pressure_on_after_one.sheep, on_evidence.subject_id);
        const auto& prior_member =
            sheep_with_id(dog_pressure_initial.sheep, on_evidence.subject_id);
        if (!check(off_evidence.stimulus_evaluated == on_evidence.stimulus_evaluated &&
                       off_evidence.dog_distance == on_evidence.dog_distance &&
                       off_evidence.dog_relative_bearing_radians ==
                           on_evidence.dog_relative_bearing_radians &&
                       off_evidence.pressure_acceleration == wide_eye::game::Vec3{},
                   "pressure_control_publishes_same_geometry_without_influence") ||
            !check(std::abs((current_member.velocity.x - prior_member.velocity.x) /
                                wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                            on_evidence.pressure_acceleration.x) < 1.0e-12 &&
                       std::abs((current_member.velocity.z - prior_member.velocity.z) /
                                    wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                                on_evidence.pressure_acceleration.z) < 1.0e-12,
                   "published_dog_pressure_matches_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    auto overlapping_dog_pressure_scenario = *dog_pressure_on_scenario;
    overlapping_dog_pressure_scenario.initial_sheep[0].position =
        overlapping_dog_pressure_scenario.dog.initial_state.position;
    wide_eye::game::GameplaySimulation overlapping_dog_pressure{overlapping_dog_pressure_scenario};
    overlapping_dog_pressure.fixed_update({});
    const auto& overlap_evidence = evidence_with_id(
        overlapping_dog_pressure.current_snapshot().sheep_dog_pressure_evidence, 1);
    if (!check(overlap_evidence.stimulus_evaluated && overlap_evidence.dog_distance == 0.0 &&
                   overlap_evidence.dog_relative_bearing_radians == 0.0 &&
                   overlap_evidence.pressure_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(overlapping_dog_pressure.current_snapshot().sheep, 1).velocity ==
                       wide_eye::game::Vec3{},
               "exact_dog_overlap_does_not_invent_pressure_direction")) {
        return EXIT_FAILURE;
    }

    auto reversed_dog_pressure_scenario = *dog_pressure_on_scenario;
    std::reverse(reversed_dog_pressure_scenario.initial_sheep.begin(),
                 reversed_dog_pressure_scenario.initial_sheep.end());
    wide_eye::game::GameplaySimulation reversed_dog_pressure{reversed_dog_pressure_scenario};
    reversed_dog_pressure.fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0}});
    for (const auto& member : dog_pressure_on_after_one.sheep) {
        if (!check(member ==
                       sheep_with_id(reversed_dog_pressure.current_snapshot().sheep, member.id),
                   "dog_pressure_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(dog_pressure_on_after_one.sheep_dog_pressure_evidence,
                                    member.id) ==
                       evidence_with_id(
                           reversed_dog_pressure.current_snapshot().sheep_dog_pressure_evidence,
                           member.id),
                   "dog_pressure_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto dog_pressure_state = wide_eye::game::gameplay_state_dump_json(dog_pressure_on);
    if (!check(dog_pressure_state &&
                   dog_pressure_state.text.find("\"sheep_dog_pressure_evidence\":[") !=
                       std::string::npos &&
                   dog_pressure_state.text.find("\"dog_relative_bearing_radians\":") !=
                       std::string::npos &&
                   dog_pressure_state.text.find("\"pressure_acceleration\":{") != std::string::npos,
               "state_dump_contains_dog_stimulus_and_pressure_evidence")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation allocation_dog_pressure{*dog_pressure_on_scenario};
    const std::size_t dog_pressure_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_dog_pressure.fixed_update({});
    }
    const std::size_t dog_pressure_allocations =
        g_allocation_count - dog_pressure_allocations_before;
    if (!check(dog_pressure_allocations == 0, "dog_pressure_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    dog_pressure_on.restart();
    if (!check(dog_pressure_on.current_snapshot() == dog_pressure_initial &&
                   dog_pressure_on.previous_snapshot() == dog_pressure_initial,
               "dog_pressure_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    const auto approach_off_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-dog-approach-off");
    const auto approach_on_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-dog-approach-on");
    auto approach_on_as_control =
        approach_on_scenario.value_or(wide_eye::game::GameplayScenarioDefinition{});
    if (approach_off_scenario.has_value()) {
        approach_on_as_control.id = approach_off_scenario->id;
    }
    approach_on_as_control.sheep_dog_approach.enabled = false;
    if (!check(approach_off_scenario.has_value() && approach_on_scenario.has_value() &&
                   approach_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_approach_off &&
                   approach_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_approach_on &&
                   approach_on_as_control == *approach_off_scenario &&
                   approach_on_scenario->sheep_dog_approach.enabled &&
                   approach_off_scenario->sheep_dog_pressure.enabled &&
                   approach_off_scenario->sheep_dog_pressure ==
                       approach_on_scenario->sheep_dog_pressure,
               "paired_approach_fixture_differs_only_by_approach_switch")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation approach_off{*approach_off_scenario};
    wide_eye::game::GameplaySimulation approach_on{*approach_on_scenario};
    const auto approach_initial = approach_on.current_snapshot();
    approach_off.fixed_update({});
    approach_on.fixed_update({});
    const auto& approach_off_after_one = approach_off.current_snapshot();
    const auto& approach_on_after_one = approach_on.current_snapshot();
    const auto head_on_approach =
        evidence_with_id(approach_on_after_one.sheep_dog_pressure_evidence, 1);
    const auto abeam_approach =
        evidence_with_id(approach_on_after_one.sheep_dog_pressure_evidence, 2);
    const auto opening_approach =
        evidence_with_id(approach_on_after_one.sheep_dog_pressure_evidence, 3);
    const auto diagonal_approach =
        evidence_with_id(approach_on_after_one.sheep_dog_pressure_evidence, 4);
    const auto outside_approach =
        evidence_with_id(approach_on_after_one.sheep_dog_pressure_evidence, 5);
    if (!check(approach_on.previous_snapshot() == approach_initial,
               "approach_reads_immutable_prior_snapshot") ||
        !check(head_on_approach.stimulus_evaluated && head_on_approach.dog_distance == 2.0 &&
                   head_on_approach.dog_approach_speed == 4.0 &&
                   std::abs(head_on_approach.approach_acceleration.x - 4.0 / 3.0) < 1.0e-12 &&
                   head_on_approach.approach_acceleration.z == 0.0,
               "closing_dog_above_reference_speed_saturates_approach_response") ||
        !check(abeam_approach.dog_distance == 3.0 && abeam_approach.dog_approach_speed == 0.0 &&
                   abeam_approach.approach_acceleration == wide_eye::game::Vec3{},
               "abeam_dog_motion_adds_no_approach_pressure") ||
        !check(opening_approach.dog_distance == 3.0 &&
                   opening_approach.dog_approach_speed == -4.0 &&
                   opening_approach.approach_acceleration == wide_eye::game::Vec3{},
               "leaving_dog_releases_instead_of_pulling") ||
        !check(std::abs(diagonal_approach.dog_approach_speed - 2.4) < 1.0e-12 &&
                   std::abs(diagonal_approach.approach_acceleration.x - 0.16) < 1.0e-12 &&
                   std::abs(diagonal_approach.approach_acceleration.z - 0.32 / 1.5) < 1.0e-12,
               "partial_approach_uses_exact_prior_state_projection") ||
        !check(outside_approach.dog_distance == 8.0 && outside_approach.dog_approach_speed == 4.0 &&
                   outside_approach.pressure_acceleration == wide_eye::game::Vec3{} &&
                   outside_approach.approach_acceleration == wide_eye::game::Vec3{},
               "approach_outside_pressure_radius_publishes_speed_without_influence")) {
        return EXIT_FAILURE;
    }

    for (const auto& on_evidence : approach_on_after_one.sheep_dog_pressure_evidence) {
        const auto& off_evidence = evidence_with_id(
            approach_off_after_one.sheep_dog_pressure_evidence, on_evidence.subject_id);
        const auto& current_member =
            sheep_with_id(approach_on_after_one.sheep, on_evidence.subject_id);
        const auto& prior_member = sheep_with_id(approach_initial.sheep, on_evidence.subject_id);
        if (!check(off_evidence.stimulus_evaluated == on_evidence.stimulus_evaluated &&
                       off_evidence.dog_distance == on_evidence.dog_distance &&
                       off_evidence.dog_relative_bearing_radians ==
                           on_evidence.dog_relative_bearing_radians &&
                       off_evidence.dog_approach_speed == on_evidence.dog_approach_speed &&
                       off_evidence.pressure_acceleration == on_evidence.pressure_acceleration &&
                       off_evidence.approach_acceleration == wide_eye::game::Vec3{},
                   "approach_control_preserves_accepted_distance_only_pressure") ||
            !check(std::abs((current_member.velocity.x - prior_member.velocity.x) /
                                wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                            (on_evidence.pressure_acceleration.x +
                             on_evidence.approach_acceleration.x)) < 1.0e-12 &&
                       std::abs((current_member.velocity.z - prior_member.velocity.z) /
                                    wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                                (on_evidence.pressure_acceleration.z +
                                 on_evidence.approach_acceleration.z)) < 1.0e-12,
                   "published_dog_terms_match_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    // The dog motor changes velocity within the same tick. Approach evidence must
    // still describe the prior state that caused the published sheep result.
    wide_eye::game::GameplaySimulation approach_same_tick_move{*approach_on_scenario};
    approach_same_tick_move.fixed_update(
        {.dog_move = wide_eye::game::DogMoveInput{.world_x = -1.0}});
    if (!check(approach_same_tick_move.current_snapshot().dog.velocity.x != 4.0 &&
                   evidence_with_id(
                       approach_same_tick_move.current_snapshot().sheep_dog_pressure_evidence, 1) ==
                       head_on_approach,
               "same_tick_dog_motor_change_does_not_alter_prior_state_approach")) {
        return EXIT_FAILURE;
    }

    auto overlapping_approach_scenario = *approach_on_scenario;
    overlapping_approach_scenario.initial_sheep[0].position =
        overlapping_approach_scenario.dog.initial_state.position;
    wide_eye::game::GameplaySimulation overlapping_approach{overlapping_approach_scenario};
    overlapping_approach.fixed_update({});
    const auto& overlap_approach_evidence =
        evidence_with_id(overlapping_approach.current_snapshot().sheep_dog_pressure_evidence, 1);
    if (!check(overlap_approach_evidence.stimulus_evaluated &&
                   overlap_approach_evidence.dog_distance == 0.0 &&
                   overlap_approach_evidence.dog_approach_speed == 0.0 &&
                   overlap_approach_evidence.approach_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(overlapping_approach.current_snapshot().sheep, 1).velocity ==
                       wide_eye::game::Vec3{},
               "exact_dog_overlap_does_not_invent_approach_direction")) {
        return EXIT_FAILURE;
    }

    auto reversed_approach_scenario = *approach_on_scenario;
    std::reverse(reversed_approach_scenario.initial_sheep.begin(),
                 reversed_approach_scenario.initial_sheep.end());
    wide_eye::game::GameplaySimulation reversed_approach{reversed_approach_scenario};
    reversed_approach.fixed_update({});
    for (const auto& member : approach_on_after_one.sheep) {
        if (!check(member == sheep_with_id(reversed_approach.current_snapshot().sheep, member.id),
                   "approach_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(approach_on_after_one.sheep_dog_pressure_evidence, member.id) ==
                       evidence_with_id(
                           reversed_approach.current_snapshot().sheep_dog_pressure_evidence,
                           member.id),
                   "approach_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto approach_state = wide_eye::game::gameplay_state_dump_json(approach_on);
    if (!check(approach_state &&
                   approach_state.text.find("\"dog_approach_speed\":") != std::string::npos &&
                   approach_state.text.find("\"approach_acceleration\":{") != std::string::npos,
               "state_dump_contains_dog_approach_stimulus_and_influence")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation allocation_approach{*approach_on_scenario};
    const std::size_t approach_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_approach.fixed_update({});
    }
    const std::size_t approach_allocations = g_allocation_count - approach_allocations_before;
    if (!check(approach_allocations == 0, "approach_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    approach_on.restart();
    if (!check(approach_on.current_snapshot() == approach_initial &&
                   approach_on.previous_snapshot() == approach_initial,
               "approach_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    const auto facing_off_scenario = wide_eye::game::find_gameplay_scenario("sheep-dog-facing-off");
    const auto facing_on_scenario = wide_eye::game::find_gameplay_scenario("sheep-dog-facing-on");
    auto facing_on_as_control =
        facing_on_scenario.value_or(wide_eye::game::GameplayScenarioDefinition{});
    if (facing_off_scenario.has_value()) {
        facing_on_as_control.id = facing_off_scenario->id;
    }
    facing_on_as_control.sheep_dog_facing.enabled = false;
    if (!check(
            facing_off_scenario.has_value() && facing_on_scenario.has_value() &&
                facing_off_scenario->id ==
                    wide_eye::game::GameplayScenarioId::sheep_dog_facing_off &&
                facing_on_scenario->id == wide_eye::game::GameplayScenarioId::sheep_dog_facing_on &&
                facing_on_as_control == *facing_off_scenario &&
                facing_on_scenario->sheep_dog_facing.enabled &&
                facing_off_scenario->sheep_dog_pressure.enabled &&
                facing_off_scenario->sheep_dog_pressure == facing_on_scenario->sheep_dog_pressure &&
                !facing_off_scenario->sheep_dog_approach.enabled &&
                facing_off_scenario->dog.initial_state.velocity == wide_eye::game::Vec3{},
            "paired_facing_fixture_differs_only_by_facing_switch")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation facing_off{*facing_off_scenario};
    wide_eye::game::GameplaySimulation facing_on{*facing_on_scenario};
    const auto facing_initial = facing_on.current_snapshot();
    facing_off.fixed_update({});
    facing_on.fixed_update({});
    const auto& facing_off_after_one = facing_off.current_snapshot();
    const auto& facing_on_after_one = facing_on.current_snapshot();
    const auto ahead_facing = evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 1);
    const auto abeam_facing = evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 2);
    const auto behind_facing = evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 3);
    const auto diagonal_facing =
        evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 4);
    const auto outside_facing =
        evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 5);
    if (!check(facing_on.previous_snapshot() == facing_initial,
               "facing_reads_immutable_prior_snapshot") ||
        !check(ahead_facing.stimulus_evaluated && ahead_facing.dog_distance == 2.0 &&
                   ahead_facing.dog_facing_alignment == 1.0 &&
                   ahead_facing.facing_acceleration.x == 0.0 &&
                   std::abs(ahead_facing.facing_acceleration.z + 1.0) < 1.0e-12,
               "dog_looking_straight_at_sheep_applies_full_facing_response") ||
        !check(abeam_facing.dog_distance == 3.0 && abeam_facing.dog_facing_alignment == 0.0 &&
                   abeam_facing.facing_acceleration == wide_eye::game::Vec3{} &&
                   std::abs(abeam_facing.pressure_acceleration.x - 1.5) < 1.0e-12,
               "abeam_sheep_receives_distance_pressure_without_facing_pressure") ||
        !check(behind_facing.dog_distance == 3.0 && behind_facing.dog_facing_alignment == -1.0 &&
                   behind_facing.facing_acceleration == wide_eye::game::Vec3{},
               "dog_looking_away_releases_instead_of_pulling") ||
        !check(std::abs(diagonal_facing.dog_facing_alignment - 0.8) < 1.0e-12 &&
                   std::abs(diagonal_facing.facing_acceleration.x - 0.12) < 1.0e-12 &&
                   std::abs(diagonal_facing.facing_acceleration.z + 0.16) < 1.0e-12,
               "partial_facing_uses_exact_prior_state_cosine") ||
        !check(outside_facing.dog_distance == 8.0 && outside_facing.dog_facing_alignment == 1.0 &&
                   outside_facing.pressure_acceleration == wide_eye::game::Vec3{} &&
                   outside_facing.facing_acceleration == wide_eye::game::Vec3{},
               "facing_outside_pressure_radius_publishes_alignment_without_influence")) {
        return EXIT_FAILURE;
    }

    for (const auto& on_evidence : facing_on_after_one.sheep_dog_pressure_evidence) {
        const auto& off_evidence = evidence_with_id(
            facing_off_after_one.sheep_dog_pressure_evidence, on_evidence.subject_id);
        const auto& current_member =
            sheep_with_id(facing_on_after_one.sheep, on_evidence.subject_id);
        const auto& prior_member = sheep_with_id(facing_initial.sheep, on_evidence.subject_id);
        if (!check(off_evidence.stimulus_evaluated == on_evidence.stimulus_evaluated &&
                       off_evidence.dog_distance == on_evidence.dog_distance &&
                       off_evidence.dog_relative_bearing_radians ==
                           on_evidence.dog_relative_bearing_radians &&
                       off_evidence.dog_approach_speed == on_evidence.dog_approach_speed &&
                       off_evidence.dog_facing_alignment == on_evidence.dog_facing_alignment &&
                       off_evidence.pressure_acceleration == on_evidence.pressure_acceleration &&
                       off_evidence.facing_acceleration == wide_eye::game::Vec3{},
                   "facing_control_preserves_accepted_distance_only_pressure") ||
            !check(std::abs((current_member.velocity.x - prior_member.velocity.x) /
                                wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                            (on_evidence.pressure_acceleration.x +
                             on_evidence.facing_acceleration.x)) < 1.0e-12 &&
                       std::abs((current_member.velocity.z - prior_member.velocity.z) /
                                    wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                                (on_evidence.pressure_acceleration.z +
                                 on_evidence.facing_acceleration.z)) < 1.0e-12,
                   "published_facing_term_matches_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    // Facing must be read from the dog's heading rather than assumed from the
    // fixture layout. Turning the same dog through half a turn swaps which sheep
    // it looks at without moving any position.
    auto reversed_heading_scenario = *facing_on_scenario;
    reversed_heading_scenario.dog.initial_state.heading_radians = 3.14159265358979323846;
    wide_eye::game::GameplaySimulation reversed_heading{reversed_heading_scenario};
    reversed_heading.fixed_update({});
    const auto& reversed_ahead =
        evidence_with_id(reversed_heading.current_snapshot().sheep_dog_pressure_evidence, 1);
    const auto& reversed_behind =
        evidence_with_id(reversed_heading.current_snapshot().sheep_dog_pressure_evidence, 3);
    if (!check(reversed_ahead.dog_distance == ahead_facing.dog_distance &&
                   reversed_ahead.pressure_acceleration == ahead_facing.pressure_acceleration &&
                   reversed_ahead.dog_facing_alignment == -1.0 &&
                   reversed_ahead.facing_acceleration == wide_eye::game::Vec3{},
               "turned_dog_stops_facing_the_sheep_ahead_of_its_old_heading") ||
        !check(reversed_behind.dog_facing_alignment == 1.0 &&
                   reversed_behind.facing_acceleration.x == 0.0 &&
                   std::abs(reversed_behind.facing_acceleration.z - 0.75) < 1.0e-12,
               "turned_dog_now_faces_the_sheep_behind_its_old_heading")) {
        return EXIT_FAILURE;
    }

    // The dog motor turns within the same tick. Facing evidence must still
    // describe the prior state that caused the published sheep result.
    wide_eye::game::GameplaySimulation facing_same_tick_turn{*facing_on_scenario};
    facing_same_tick_turn.fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0}});
    if (!check(facing_same_tick_turn.current_snapshot().dog.heading_radians != 0.0 &&
                   evidence_with_id(
                       facing_same_tick_turn.current_snapshot().sheep_dog_pressure_evidence, 1) ==
                       ahead_facing,
               "same_tick_dog_motor_turn_does_not_alter_prior_state_facing")) {
        return EXIT_FAILURE;
    }

    auto overlapping_facing_scenario = *facing_on_scenario;
    overlapping_facing_scenario.initial_sheep[0].position =
        overlapping_facing_scenario.dog.initial_state.position;
    wide_eye::game::GameplaySimulation overlapping_facing{overlapping_facing_scenario};
    overlapping_facing.fixed_update({});
    const auto& overlap_facing_evidence =
        evidence_with_id(overlapping_facing.current_snapshot().sheep_dog_pressure_evidence, 1);
    if (!check(overlap_facing_evidence.stimulus_evaluated &&
                   overlap_facing_evidence.dog_distance == 0.0 &&
                   overlap_facing_evidence.dog_facing_alignment == 0.0 &&
                   overlap_facing_evidence.facing_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(overlapping_facing.current_snapshot().sheep, 1).velocity ==
                       wide_eye::game::Vec3{},
               "exact_dog_overlap_does_not_invent_facing_direction")) {
        return EXIT_FAILURE;
    }

    auto reversed_facing_scenario = *facing_on_scenario;
    std::reverse(reversed_facing_scenario.initial_sheep.begin(),
                 reversed_facing_scenario.initial_sheep.end());
    wide_eye::game::GameplaySimulation reversed_facing{reversed_facing_scenario};
    reversed_facing.fixed_update({});
    for (const auto& member : facing_on_after_one.sheep) {
        if (!check(member == sheep_with_id(reversed_facing.current_snapshot().sheep, member.id),
                   "facing_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, member.id) ==
                    evidence_with_id(reversed_facing.current_snapshot().sheep_dog_pressure_evidence,
                                     member.id),
                "facing_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto facing_state = wide_eye::game::gameplay_state_dump_json(facing_on);
    if (!check(facing_state &&
                   facing_state.text.find("\"dog_facing_alignment\":") != std::string::npos &&
                   facing_state.text.find("\"facing_acceleration\":{") != std::string::npos,
               "state_dump_contains_dog_facing_stimulus_and_influence")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation allocation_facing{*facing_on_scenario};
    const std::size_t facing_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_facing.fixed_update({});
    }
    const std::size_t facing_allocations = g_allocation_count - facing_allocations_before;
    if (!check(facing_allocations == 0, "facing_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    facing_on.restart();
    if (!check(facing_on.current_snapshot() == facing_initial &&
                   facing_on.previous_snapshot() == facing_initial,
               "facing_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    const auto sight_off_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-dog-line-of-sight-off");
    const auto sight_on_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-dog-line-of-sight-on");
    auto sight_on_as_control =
        sight_on_scenario.value_or(wide_eye::game::GameplayScenarioDefinition{});
    if (sight_off_scenario.has_value()) {
        sight_on_as_control.id = sight_off_scenario->id;
    }
    sight_on_as_control.sheep_dog_line_of_sight.enabled = false;
    if (!check(sight_off_scenario.has_value() && sight_on_scenario.has_value() &&
                   sight_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_line_of_sight_off &&
                   sight_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_line_of_sight_on &&
                   sight_on_as_control == *sight_off_scenario &&
                   sight_on_scenario->sheep_dog_line_of_sight.enabled &&
                   sight_off_scenario->sheep_dog_pressure.enabled &&
                   sight_off_scenario->sheep_dog_pressure ==
                       sight_on_scenario->sheep_dog_pressure &&
                   !sight_off_scenario->sheep_dog_approach.enabled &&
                   !sight_off_scenario->sheep_dog_facing.enabled && sight_off_scenario->gate_open &&
                   sight_off_scenario->dog.initial_state.velocity == wide_eye::game::Vec3{},
               "paired_line_of_sight_fixture_differs_only_by_sight_switch")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation sight_off{*sight_off_scenario};
    wide_eye::game::GameplaySimulation sight_on{*sight_on_scenario};
    const auto sight_initial = sight_on.current_snapshot();
    sight_off.fixed_update({});
    sight_on.fixed_update({});
    const auto& sight_off_after_one = sight_off.current_snapshot();
    const auto& sight_on_after_one = sight_on.current_snapshot();
    const auto clear_sight = evidence_with_id(sight_on_after_one.sheep_dog_pressure_evidence, 1);
    const auto left_wall_sight =
        evidence_with_id(sight_on_after_one.sheep_dog_pressure_evidence, 2);
    const auto gate_gap_sight = evidence_with_id(sight_on_after_one.sheep_dog_pressure_evidence, 3);
    const auto right_wall_sight =
        evidence_with_id(sight_on_after_one.sheep_dog_pressure_evidence, 4);
    const auto distant_sight = evidence_with_id(sight_on_after_one.sheep_dog_pressure_evidence, 5);
    const auto blocked_left_pressure =
        evidence_with_id(sight_off_after_one.sheep_dog_pressure_evidence, 2).pressure_acceleration;
    const auto blocked_right_pressure =
        evidence_with_id(sight_off_after_one.sheep_dog_pressure_evidence, 4).pressure_acceleration;
    if (!check(sight_on.previous_snapshot() == sight_initial,
               "line_of_sight_reads_immutable_prior_snapshot") ||
        !check(clear_sight.stimulus_evaluated && clear_sight.dog_distance == 4.0 &&
                   !clear_sight.dog_line_of_sight_blocked &&
                   clear_sight.dog_line_of_sight_occluder ==
                       wide_eye::game::PaddockObstacle::none &&
                   clear_sight.pressure_acceleration.x == 0.0 &&
                   std::abs(clear_sight.pressure_acceleration.z + 1.0) < 1.0e-12,
               "clear_line_reproduces_accepted_distance_pressure") ||
        !check(left_wall_sight.dog_distance == 5.0 && left_wall_sight.dog_line_of_sight_blocked &&
                   left_wall_sight.dog_line_of_sight_occluder ==
                       wide_eye::game::PaddockObstacle::left_wall &&
                   left_wall_sight.pressure_acceleration == wide_eye::game::Vec3{} &&
                   std::abs(blocked_left_pressure.x + 0.3) < 1.0e-12 &&
                   std::abs(blocked_left_pressure.z - 0.4) < 1.0e-12,
               "wall_between_sheep_and_dog_releases_pressure") ||
        !check(right_wall_sight.dog_distance == 5.0 && right_wall_sight.dog_line_of_sight_blocked &&
                   right_wall_sight.dog_line_of_sight_occluder ==
                       wide_eye::game::PaddockObstacle::right_wall &&
                   right_wall_sight.pressure_acceleration == wide_eye::game::Vec3{} &&
                   std::abs(blocked_right_pressure.x - 0.3) < 1.0e-12 &&
                   std::abs(blocked_right_pressure.z - 0.4) < 1.0e-12,
               "opposite_wall_is_named_as_its_own_occluder") ||
        !check(gate_gap_sight.dog_distance == 5.0 && !gate_gap_sight.dog_line_of_sight_blocked &&
                   gate_gap_sight.dog_line_of_sight_occluder ==
                       wide_eye::game::PaddockObstacle::none &&
                   gate_gap_sight.pressure_acceleration.x == 0.0 &&
                   std::abs(gate_gap_sight.pressure_acceleration.z - 0.5) < 1.0e-12,
               "sight_through_the_open_gate_keeps_full_pressure") ||
        !check(distant_sight.dog_distance == 10.0 && distant_sight.dog_line_of_sight_blocked &&
                   distant_sight.dog_line_of_sight_occluder ==
                       wide_eye::game::PaddockObstacle::left_wall &&
                   distant_sight.pressure_acceleration == wide_eye::game::Vec3{},
               "occlusion_outside_pressure_radius_publishes_without_influence")) {
        return EXIT_FAILURE;
    }

    for (const auto& on_evidence : sight_on_after_one.sheep_dog_pressure_evidence) {
        const auto& off_evidence = evidence_with_id(sight_off_after_one.sheep_dog_pressure_evidence,
                                                    on_evidence.subject_id);
        const auto& prior_member = sheep_with_id(sight_initial.sheep, on_evidence.subject_id);
        const auto applied = [&prior_member](const wide_eye::game::SheepState& member) {
            return wide_eye::game::Vec3{.x = (member.velocity.x - prior_member.velocity.x) /
                                             wide_eye::game::GameplaySimulation::kFixedDeltaSeconds,
                                        .z =
                                            (member.velocity.z - prior_member.velocity.z) /
                                            wide_eye::game::GameplaySimulation::kFixedDeltaSeconds};
        };
        const auto on_applied =
            applied(sheep_with_id(sight_on_after_one.sheep, on_evidence.subject_id));
        const auto off_applied =
            applied(sheep_with_id(sight_off_after_one.sheep, on_evidence.subject_id));
        const bool released = on_evidence.dog_line_of_sight_blocked;
        if (!check(off_evidence.stimulus_evaluated == on_evidence.stimulus_evaluated &&
                       off_evidence.dog_distance == on_evidence.dog_distance &&
                       off_evidence.dog_relative_bearing_radians ==
                           on_evidence.dog_relative_bearing_radians &&
                       off_evidence.dog_approach_speed == on_evidence.dog_approach_speed &&
                       off_evidence.dog_facing_alignment == on_evidence.dog_facing_alignment &&
                       off_evidence.dog_line_of_sight_blocked ==
                           on_evidence.dog_line_of_sight_blocked &&
                       off_evidence.dog_line_of_sight_occluder ==
                           on_evidence.dog_line_of_sight_occluder,
                   "line_of_sight_control_publishes_identical_prior_state_evidence") ||
            !check(released
                       ? on_evidence.pressure_acceleration == wide_eye::game::Vec3{}
                       : on_evidence.pressure_acceleration == off_evidence.pressure_acceleration,
                   "only_an_occluded_dog_changes_the_applied_pressure") ||
            !check(std::abs(on_applied.x - on_evidence.pressure_acceleration.x) < 1.0e-12 &&
                       std::abs(on_applied.z - on_evidence.pressure_acceleration.z) < 1.0e-12 &&
                       std::abs(off_applied.x - off_evidence.pressure_acceleration.x) < 1.0e-12 &&
                       std::abs(off_applied.z - off_evidence.pressure_acceleration.z) < 1.0e-12,
                   "published_line_of_sight_terms_match_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    // The gate is world state, not fixture layout: closing it must hide the dog
    // from the sheep that was watching through the opening and leave every other
    // sight line unchanged.
    auto closed_gate_sight_scenario = *sight_on_scenario;
    closed_gate_sight_scenario.gate_open = false;
    wide_eye::game::GameplaySimulation closed_gate_sight{closed_gate_sight_scenario};
    closed_gate_sight.fixed_update({});
    const auto& closed_gate_evidence =
        closed_gate_sight.current_snapshot().sheep_dog_pressure_evidence;
    const auto& gated = evidence_with_id(closed_gate_evidence, 3);
    if (!check(gated.dog_distance == gate_gap_sight.dog_distance &&
                   gated.dog_facing_alignment == gate_gap_sight.dog_facing_alignment &&
                   gated.dog_line_of_sight_blocked &&
                   gated.dog_line_of_sight_occluder == wide_eye::game::PaddockObstacle::gate &&
                   gated.pressure_acceleration == wide_eye::game::Vec3{},
               "closing_the_gate_hides_the_dog_that_was_visible_through_it") ||
        !check(evidence_with_id(closed_gate_evidence, 1) == clear_sight &&
                   evidence_with_id(closed_gate_evidence, 2) == left_wall_sight,
               "closing_the_gate_leaves_other_sight_lines_unchanged")) {
        return EXIT_FAILURE;
    }

    // Visibility multiplies every dog term rather than adding a fourth vector,
    // so the same occluder must release approach and facing with pressure.
    auto combined_sight_on_scenario = *sight_on_scenario;
    combined_sight_on_scenario.dog.initial_state.velocity = {.z = 3.0};
    combined_sight_on_scenario.sheep_dog_approach.enabled = true;
    combined_sight_on_scenario.sheep_dog_facing.enabled = true;
    auto combined_sight_off_scenario = combined_sight_on_scenario;
    combined_sight_off_scenario.id = sight_off_scenario->id;
    combined_sight_off_scenario.sheep_dog_line_of_sight.enabled = false;
    wide_eye::game::GameplaySimulation combined_sight_on{combined_sight_on_scenario};
    wide_eye::game::GameplaySimulation combined_sight_off{combined_sight_off_scenario};
    combined_sight_on.fixed_update({});
    combined_sight_off.fixed_update({});
    const auto& combined_on_blocked =
        evidence_with_id(combined_sight_on.current_snapshot().sheep_dog_pressure_evidence, 2);
    const auto& combined_off_blocked =
        evidence_with_id(combined_sight_off.current_snapshot().sheep_dog_pressure_evidence, 2);
    const auto& combined_on_visible =
        evidence_with_id(combined_sight_on.current_snapshot().sheep_dog_pressure_evidence, 3);
    const auto& combined_off_visible =
        evidence_with_id(combined_sight_off.current_snapshot().sheep_dog_pressure_evidence, 3);
    if (!check(std::abs(combined_off_blocked.dog_approach_speed - 2.4) < 1.0e-12 &&
                   std::abs(combined_off_blocked.dog_facing_alignment - 0.8) < 1.0e-12 &&
                   combined_off_blocked.pressure_acceleration != wide_eye::game::Vec3{} &&
                   combined_off_blocked.approach_acceleration != wide_eye::game::Vec3{} &&
                   combined_off_blocked.facing_acceleration != wide_eye::game::Vec3{},
               "combined_control_applies_all_three_dog_terms_through_the_wall") ||
        !check(combined_on_blocked.dog_approach_speed == combined_off_blocked.dog_approach_speed &&
                   combined_on_blocked.dog_facing_alignment ==
                       combined_off_blocked.dog_facing_alignment &&
                   combined_on_blocked.pressure_acceleration == wide_eye::game::Vec3{} &&
                   combined_on_blocked.approach_acceleration == wide_eye::game::Vec3{} &&
                   combined_on_blocked.facing_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(combined_sight_on.current_snapshot().sheep, 2).velocity ==
                       wide_eye::game::Vec3{},
               "occluded_dog_releases_pressure_approach_and_facing_together") ||
        !check(std::abs(combined_on_visible.dog_approach_speed - 3.0) < 1.0e-12 &&
                   combined_on_visible.pressure_acceleration ==
                       combined_off_visible.pressure_acceleration &&
                   combined_on_visible.approach_acceleration ==
                       combined_off_visible.approach_acceleration &&
                   combined_on_visible.facing_acceleration ==
                       combined_off_visible.facing_acceleration &&
                   combined_on_visible.approach_acceleration != wide_eye::game::Vec3{} &&
                   combined_on_visible.facing_acceleration != wide_eye::game::Vec3{},
               "visible_dog_keeps_every_accepted_term_unchanged")) {
        return EXIT_FAILURE;
    }

    // The dog motor moves within the same tick. Sight evidence must still
    // describe the prior state that caused the published sheep result.
    wide_eye::game::GameplaySimulation sight_same_tick_move{*sight_on_scenario};
    sight_same_tick_move.fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_z = 1.0}});
    if (!check(sight_same_tick_move.current_snapshot().dog.position.z >
                       sight_on_scenario->dog.initial_state.position.z &&
                   evidence_with_id(
                       sight_same_tick_move.current_snapshot().sheep_dog_pressure_evidence, 2) ==
                       left_wall_sight,
               "same_tick_dog_motor_move_does_not_alter_prior_state_sight")) {
        return EXIT_FAILURE;
    }

    auto overlapping_sight_scenario = *sight_on_scenario;
    overlapping_sight_scenario.initial_sheep[0].position =
        overlapping_sight_scenario.dog.initial_state.position;
    wide_eye::game::GameplaySimulation overlapping_sight{overlapping_sight_scenario};
    overlapping_sight.fixed_update({});
    const auto& overlap_sight_evidence =
        evidence_with_id(overlapping_sight.current_snapshot().sheep_dog_pressure_evidence, 1);
    if (!check(overlap_sight_evidence.stimulus_evaluated &&
                   overlap_sight_evidence.dog_distance == 0.0 &&
                   !overlap_sight_evidence.dog_line_of_sight_blocked &&
                   overlap_sight_evidence.dog_line_of_sight_occluder ==
                       wide_eye::game::PaddockObstacle::none &&
                   overlap_sight_evidence.pressure_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(overlapping_sight.current_snapshot().sheep, 1).velocity ==
                       wide_eye::game::Vec3{},
               "exact_dog_overlap_does_not_invent_an_occluder")) {
        return EXIT_FAILURE;
    }

    auto reversed_sight_scenario = *sight_on_scenario;
    std::reverse(reversed_sight_scenario.initial_sheep.begin(),
                 reversed_sight_scenario.initial_sheep.end());
    wide_eye::game::GameplaySimulation reversed_sight{reversed_sight_scenario};
    reversed_sight.fixed_update({});
    for (const auto& member : sight_on_after_one.sheep) {
        if (!check(member == sheep_with_id(reversed_sight.current_snapshot().sheep, member.id),
                   "line_of_sight_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(sight_on_after_one.sheep_dog_pressure_evidence, member.id) ==
                    evidence_with_id(reversed_sight.current_snapshot().sheep_dog_pressure_evidence,
                                     member.id),
                "line_of_sight_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto sight_state = wide_eye::game::gameplay_state_dump_json(sight_on);
    if (!check(sight_state &&
                   sight_state.text.find("\"dog_line_of_sight_blocked\":true") !=
                       std::string::npos &&
                   sight_state.text.find("\"dog_line_of_sight_occluder\":\"left_wall\"") !=
                       std::string::npos &&
                   sight_state.text.find("\"dog_line_of_sight_occluder\":\"right_wall\"") !=
                       std::string::npos &&
                   sight_state.text.find("\"dog_line_of_sight_occluder\":\"none\"") !=
                       std::string::npos,
               "state_dump_contains_dog_line_of_sight_stimulus_and_occluder")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation allocation_sight{*sight_on_scenario};
    const std::size_t sight_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_sight.fixed_update({});
    }
    const std::size_t sight_allocations = g_allocation_count - sight_allocations_before;
    if (!check(sight_allocations == 0, "line_of_sight_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    sight_on.restart();
    if (!check(sight_on.current_snapshot() == sight_initial &&
                   sight_on.previous_snapshot() == sight_initial,
               "line_of_sight_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation replay_a{*scenario};
    wide_eye::game::GameplaySimulation replay_b{*scenario};
    const wide_eye::game::GameplayReplay replay = sample_replay(replay_a);
    const auto replay_text = wide_eye::game::gameplay_replay_json(replay);
    if (!check(wide_eye::game::kGameplaySeedFormatVersion == 1 &&
                   wide_eye::game::kGameplayActionInputFormatVersion == 1 &&
                   wide_eye::game::kGameplayReplayFormatVersion == 1 &&
                   wide_eye::game::kGameplayStateDumpFormatVersion == 8,
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
        !check(state_a.text.starts_with("{\"schema\":\"wide-eye.gameplay-state\",\"version\":8,"
                                        "\"tick_rate\":60,\"scenario\":{"),
               "state_dump_schema_header") ||
        !check(state_a.text.find("\"current\":{\"tick\":3") != std::string::npos,
               "state_dump_contains_authoritative_tick") ||
        !check(state_a.text.find("\"sheep\":[{\"id\":1") != std::string::npos &&
                   state_a.text.find("\"id\":5") != std::string::npos &&
                   state_a.text.find("\"behavior\":\"settled\"") != std::string::npos &&
                   state_a.text.find("\"sheep_social_evidence\":[{\"subject_id\":1") !=
                       std::string::npos,
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
    incompatible.scenario.scenario = static_cast<wide_eye::game::GameplayScenarioId>(255);
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

    wide_eye::game::GameplayScenarioDefinition invalid_state_scenario = *scenario;
    invalid_state_scenario.dog.initial_state.position.x = std::numeric_limits<double>::quiet_NaN();
    const wide_eye::game::GameplaySimulation invalid_state_simulation{invalid_state_scenario};
    if (!check(wide_eye::game::gameplay_state_dump_json(invalid_state_simulation).error ==
                   wide_eye::game::GameplayContractError::non_finite_state,
               "state_dump_rejects_non_finite_json")) {
        return EXIT_FAILURE;
    }
    invalid_state_scenario = *scenario;
    invalid_state_scenario.id = static_cast<wide_eye::game::GameplayScenarioId>(255);
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
              << "sheep_only_separation_fixture=close_range_repulsion\n"
              << "separation_steady_state_allocations=" << separation_allocations << '\n'
              << "sheep_only_attraction_fixture=bounded_selected_neighbors\n"
              << "attraction_steady_state_allocations=" << attraction_allocations << '\n'
              << "alignment_pair_ticks=" << kAlignmentComparisonTicks << '\n'
              << "alignment_off_polarization=" << alignment_off_observables->polarization << '\n'
              << "alignment_on_polarization=" << alignment_on_observables->polarization << '\n'
              << "alignment_steady_state_allocations=" << alignment_allocations << '\n'
              << "dog_pressure_fixture=distance_only_paired_control\n"
              << "dog_pressure_steady_state_allocations=" << dog_pressure_allocations << '\n'
              << "dog_approach_fixture=closing_speed_paired_control\n"
              << "dog_approach_head_on_speed=" << head_on_approach.dog_approach_speed << '\n'
              << "dog_approach_diagonal_speed=" << diagonal_approach.dog_approach_speed << '\n'
              << "dog_approach_steady_state_allocations=" << approach_allocations << '\n'
              << "dog_facing_fixture=heading_cosine_paired_control\n"
              << "dog_facing_ahead_alignment=" << ahead_facing.dog_facing_alignment << '\n'
              << "dog_facing_diagonal_alignment=" << diagonal_facing.dog_facing_alignment << '\n'
              << "dog_facing_steady_state_allocations=" << facing_allocations << '\n'
              << "dog_line_of_sight_fixture=analytic_occluder_paired_control\n"
              << "dog_line_of_sight_clear_pressure_z=" << clear_sight.pressure_acceleration.z
              << '\n'
              << "dog_line_of_sight_control_left_wall_pressure_x=" << blocked_left_pressure.x
              << '\n'
              << "dog_line_of_sight_occluded_pressure="
              << std::hypot(left_wall_sight.pressure_acceleration.x,
                            left_wall_sight.pressure_acceleration.z)
              << '\n'
              << "dog_line_of_sight_gate_gap_pressure_z=" << gate_gap_sight.pressure_acceleration.z
              << '\n'
              << "dog_line_of_sight_closed_gate_blocks_gate_gap=yes\n"
              << "dog_line_of_sight_steady_state_allocations=" << sight_allocations << '\n'
              << "repeated_local_replay_equal=yes\n"
              << "gameplay_simulation_result=pass\n";
    return EXIT_SUCCESS;
}
