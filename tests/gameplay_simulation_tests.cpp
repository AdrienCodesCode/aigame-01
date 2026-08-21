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

const wide_eye::game::SheepCollisionEvidence&
evidence_with_id(const wide_eye::game::SheepCollisionEvidenceBuffer& evidence, std::uint32_t id) {
    const auto member = std::find_if(evidence.begin(), evidence.end(), [id](const auto& candidate) {
        return candidate.subject_id == id;
    });
    if (member == evidence.end()) {
        std::abort();
    }
    return *member;
}

// One sheep's first refused displacement, kept with the immutable prior state
// that caused it so the oracle can say the sheep arrived from open ground and
// stopped rather than starting inside a wall.
struct SheepContactRecord {
    bool observed = false;
    std::uint64_t tick = 0;
    std::uint32_t contact_ticks = 0;
    double minimum_x = 0.0;
    double minimum_z = 0.0;
    wide_eye::game::SheepState prior{};
    wide_eye::game::SheepState state{};
    wide_eye::game::SheepCollisionEvidence evidence{};
};

using SheepContactRecordBuffer =
    std::array<SheepContactRecord, wide_eye::game::kGameplaySheepCount>;

struct PaddockCollisionRun {
    SheepContactRecordBuffer contacts{};
    wide_eye::game::GameplaySnapshot midpoint{};
    wide_eye::game::GameplaySnapshot final_snapshot{};
};

const SheepContactRecord& contact_with_id(const SheepContactRecordBuffer& contacts,
                                          std::uint32_t id) {
    const auto member = std::find_if(contacts.begin(), contacts.end(), [id](const auto& candidate) {
        return candidate.evidence.subject_id == id;
    });
    if (member == contacts.end()) {
        std::abort();
    }
    return *member;
}

PaddockCollisionRun
run_paddock_collision(const wide_eye::game::GameplayScenarioDefinition& scenario,
                      std::uint64_t ticks, std::uint64_t midpoint_tick) {
    PaddockCollisionRun result;
    wide_eye::game::GameplaySimulation simulation{scenario};
    for (std::size_t index = 0; index < scenario.initial_sheep.size(); ++index) {
        result.contacts[index].evidence.subject_id = scenario.initial_sheep[index].id;
        result.contacts[index].minimum_x = scenario.initial_sheep[index].position.x;
        result.contacts[index].minimum_z = scenario.initial_sheep[index].position.z;
    }

    for (std::uint64_t tick = 1; tick <= ticks; ++tick) {
        simulation.fixed_update({});
        const auto& snapshot = simulation.current_snapshot();
        for (std::size_t index = 0; index < snapshot.sheep.size(); ++index) {
            SheepContactRecord& record = result.contacts[index];
            record.minimum_x = std::min(record.minimum_x, snapshot.sheep[index].position.x);
            record.minimum_z = std::min(record.minimum_z, snapshot.sheep[index].position.z);
            const auto& evidence = snapshot.sheep_collision_evidence[index];
            if (!evidence.clipped_x && !evidence.clipped_z) {
                continue;
            }
            ++record.contact_ticks;
            if (record.observed) {
                continue;
            }
            record.observed = true;
            record.tick = tick;
            record.prior = sheep_with_id(simulation.previous_snapshot().sheep, evidence.subject_id);
            record.state = snapshot.sheep[index];
            record.evidence = evidence;
        }
        if (tick == midpoint_tick) {
            result.midpoint = snapshot;
        }
    }

    result.final_snapshot = simulation.current_snapshot();
    return result;
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
                    static_cast<wide_eye::game::SheepBehaviorState>(255)) &&
                wide_eye::game::is_known_sheep_temperament(
                    wide_eye::game::SheepTemperament::ordinary) &&
                wide_eye::game::is_known_sheep_temperament(
                    wide_eye::game::SheepTemperament::stubborn) &&
                !wide_eye::game::is_known_sheep_temperament(
                    static_cast<wide_eye::game::SheepTemperament>(255)),
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
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepCollisionEvidence>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepCollisionEvidenceBuffer>);

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
                              sheep.temperament == wide_eye::game::SheepTemperament::ordinary &&
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

    // Paddock collision authority. The paired fixture disables every steering
    // term, so each sheep travels in a straight line at a constant speed and the
    // analytic paddock is the only thing that can stop it. That keeps the
    // expected resting coordinates exact arithmetic instead of the end point of
    // an accumulated response.
    constexpr std::uint64_t kPaddockCollisionTicks = 420;
    constexpr std::uint64_t kPaddockCollisionMidpointTick = 150;
    constexpr double kWallRestZ = 16.0 + wide_eye::game::kSheepCollisionRadius;
    constexpr double kWesternBoundX =
        wide_eye::game::PaddockCollisionField::kMinimumX + wide_eye::game::kSheepCollisionRadius;
    constexpr double kSouthernBoundZ =
        wide_eye::game::PaddockCollisionField::kMinimumZ + wide_eye::game::kSheepCollisionRadius;
    const auto collision_closed_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-paddock-collision-closed-gate");
    const auto collision_open_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-paddock-collision-open-gate");
    auto collision_open_as_control =
        collision_open_scenario.value_or(wide_eye::game::GameplayScenarioDefinition{});
    if (collision_closed_scenario.has_value()) {
        collision_open_as_control.id = collision_closed_scenario->id;
    }
    collision_open_as_control.gate_open = false;
    if (!check(collision_closed_scenario.has_value() && collision_open_scenario.has_value() &&
                   collision_closed_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_paddock_collision_closed_gate &&
                   collision_open_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_paddock_collision_open_gate &&
                   collision_open_as_control == *collision_closed_scenario &&
                   collision_open_scenario->gate_open && !collision_closed_scenario->gate_open &&
                   collision_closed_scenario->version == 1 &&
                   collision_closed_scenario->seed == 0 &&
                   collision_closed_scenario->sheep_fixture ==
                       wide_eye::game::SheepFixture::local_social_response &&
                   !collision_closed_scenario->sheep_separation.enabled &&
                   !collision_closed_scenario->sheep_attraction.enabled &&
                   !collision_closed_scenario->sheep_alignment.enabled &&
                   !collision_closed_scenario->sheep_dog_pressure.enabled &&
                   !collision_closed_scenario->sheep_dog_approach.enabled &&
                   !collision_closed_scenario->sheep_dog_facing.enabled &&
                   !collision_closed_scenario->sheep_dog_line_of_sight.enabled,
               "paired_paddock_collision_fixture_differs_only_by_gate_state")) {
        return EXIT_FAILURE;
    }

    const PaddockCollisionRun closed_gate_run = run_paddock_collision(
        *collision_closed_scenario, kPaddockCollisionTicks, kPaddockCollisionMidpointTick);
    const PaddockCollisionRun open_gate_run = run_paddock_collision(
        *collision_open_scenario, kPaddockCollisionTicks, kPaddockCollisionMidpointTick);
    const SheepContactRecord& left_wall_contact = contact_with_id(closed_gate_run.contacts, 1);
    const SheepContactRecord& gate_contact = contact_with_id(closed_gate_run.contacts, 2);
    const SheepContactRecord& right_wall_contact = contact_with_id(closed_gate_run.contacts, 3);
    const SheepContactRecord& untouched_contact = contact_with_id(closed_gate_run.contacts, 4);
    const SheepContactRecord& bound_contact = contact_with_id(closed_gate_run.contacts, 5);
    const auto& closed_gate_sheep_two = sheep_with_id(closed_gate_run.final_snapshot.sheep, 2);
    if (!check(left_wall_contact.observed && left_wall_contact.evidence.clipped_z &&
                   !left_wall_contact.evidence.clipped_x &&
                   left_wall_contact.evidence.obstacle ==
                       wide_eye::game::PaddockObstacle::left_wall &&
                   left_wall_contact.prior.position.z > kWallRestZ &&
                   left_wall_contact.state.position.z == kWallRestZ &&
                   left_wall_contact.state.velocity.z == 0.0 &&
                   left_wall_contact.minimum_z == kWallRestZ &&
                   sheep_with_id(closed_gate_run.final_snapshot.sheep, 1).position.z == kWallRestZ,
               "wall_stops_a_moving_sheep_at_the_clipped_coordinate") ||
        !check(gate_contact.observed && gate_contact.evidence.clipped_z &&
                   gate_contact.evidence.obstacle == wide_eye::game::PaddockObstacle::gate &&
                   gate_contact.state.position.z == kWallRestZ &&
                   gate_contact.state.velocity.z == 0.0 && gate_contact.minimum_z == kWallRestZ &&
                   closed_gate_sheep_two.position.x == 16.0 &&
                   closed_gate_sheep_two.position.z == kWallRestZ &&
                   closed_gate_sheep_two.velocity == wide_eye::game::Vec3{},
               "closed_gate_stops_a_moving_sheep_and_names_the_gate") ||
        !check(right_wall_contact.observed && right_wall_contact.evidence.clipped_z &&
                   !right_wall_contact.evidence.clipped_x &&
                   right_wall_contact.evidence.obstacle ==
                       wide_eye::game::PaddockObstacle::right_wall &&
                   right_wall_contact.state.position.z == kWallRestZ &&
                   right_wall_contact.state.velocity.z == 0.0 &&
                   right_wall_contact.state.velocity.x == -3.0 &&
                   right_wall_contact.state.position.x < right_wall_contact.prior.position.x &&
                   sheep_with_id(closed_gate_run.final_snapshot.sheep, 3).position.z ==
                       kWallRestZ &&
                   sheep_with_id(closed_gate_run.final_snapshot.sheep, 3).position.x < 4.0,
               "contact_clears_only_the_blocked_axis_velocity") ||
        !check(bound_contact.observed && bound_contact.evidence.clipped_x &&
                   !bound_contact.evidence.clipped_z &&
                   bound_contact.evidence.obstacle == wide_eye::game::PaddockObstacle::none &&
                   bound_contact.state.position.x == kWesternBoundX &&
                   bound_contact.state.velocity.x == 0.0 &&
                   bound_contact.minimum_x == kWesternBoundX,
               "paddock_outer_bound_stops_a_sheep_without_naming_an_obstacle")) {
        return EXIT_FAILURE;
    }

    // The same fixture with the gate open must let that sheep through and then
    // stop it on the paddock's own southern bound instead.
    const SheepContactRecord& open_gate_contact = contact_with_id(open_gate_run.contacts, 2);
    const auto& open_gate_sheep_two = sheep_with_id(open_gate_run.final_snapshot.sheep, 2);
    if (!check(sheep_with_id(open_gate_run.midpoint.sheep, 2).position.z < 15.0 &&
                   open_gate_contact.observed &&
                   open_gate_contact.tick > kPaddockCollisionMidpointTick &&
                   open_gate_contact.evidence.obstacle == wide_eye::game::PaddockObstacle::none &&
                   open_gate_sheep_two.position.x == 16.0 &&
                   open_gate_sheep_two.position.z == kSouthernBoundZ &&
                   open_gate_sheep_two.velocity == wide_eye::game::Vec3{},
               "an_open_gate_is_the_only_way_through_the_wall_line")) {
        return EXIT_FAILURE;
    }

    // A sheep that never reaches a boundary must be bit-identical to plain
    // unclipped integration, and every non-contacting sheep must be identical
    // between the two gate states, including all published evidence.
    wide_eye::game::SheepState unclipped =
        sheep_with_id(collision_closed_scenario->initial_sheep, 4);
    for (std::uint64_t tick = 0; tick < kPaddockCollisionTicks; ++tick) {
        unclipped.position.x +=
            unclipped.velocity.x * wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
        unclipped.position.z +=
            unclipped.velocity.z * wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    }
    bool untouched_matches_control =
        !untouched_contact.observed && untouched_contact.contact_ticks == 0 &&
        sheep_with_id(closed_gate_run.final_snapshot.sheep, 4) ==
            sheep_with_id(open_gate_run.final_snapshot.sheep, 4) &&
        sheep_with_id(closed_gate_run.final_snapshot.sheep, 4) == unclipped;
    for (const std::uint32_t id : {1U, 3U, 4U, 5U}) {
        untouched_matches_control =
            untouched_matches_control &&
            sheep_with_id(closed_gate_run.final_snapshot.sheep, id) ==
                sheep_with_id(open_gate_run.final_snapshot.sheep, id) &&
            evidence_with_id(closed_gate_run.final_snapshot.sheep_social_evidence, id) ==
                evidence_with_id(open_gate_run.final_snapshot.sheep_social_evidence, id) &&
            evidence_with_id(closed_gate_run.final_snapshot.sheep_dog_pressure_evidence, id) ==
                evidence_with_id(open_gate_run.final_snapshot.sheep_dog_pressure_evidence, id) &&
            evidence_with_id(closed_gate_run.final_snapshot.sheep_collision_evidence, id) ==
                evidence_with_id(open_gate_run.final_snapshot.sheep_collision_evidence, id) &&
            contact_with_id(closed_gate_run.contacts, id).tick ==
                contact_with_id(open_gate_run.contacts, id).tick;
    }
    if (!check(untouched_matches_control,
               "collision_authority_leaves_a_non_contacting_sheep_untouched")) {
        return EXIT_FAILURE;
    }

    // A dog placed north of the gate line must physically drive one sheep into
    // the closed gate and be unable to push it through, while the same fixture
    // with the gate open lets that sheep out.
    auto driven_closed_scenario = *collision_closed_scenario;
    driven_closed_scenario.dog.initial_state.position = {.x = 16.0, .y = 1.0, .z = 20.0};
    driven_closed_scenario.initial_sheep[1] = {.id = 2,
                                               .position = {.x = 16.0, .y = 1.0, .z = 18.0},
                                               .heading_radians = 0.0,
                                               .grounded = true};
    driven_closed_scenario.sheep_dog_pressure.enabled = true;
    auto driven_open_scenario = driven_closed_scenario;
    driven_open_scenario.id = collision_open_scenario->id;
    driven_open_scenario.gate_open = true;
    constexpr std::uint64_t kDrivenCollisionTicks = 300;
    const PaddockCollisionRun driven_closed_run = run_paddock_collision(
        driven_closed_scenario, kDrivenCollisionTicks, kPaddockCollisionMidpointTick);
    const PaddockCollisionRun driven_open_run = run_paddock_collision(
        driven_open_scenario, kDrivenCollisionTicks, kPaddockCollisionMidpointTick);
    const SheepContactRecord& driven_contact = contact_with_id(driven_closed_run.contacts, 2);
    const auto& driven_closed_two = sheep_with_id(driven_closed_run.final_snapshot.sheep, 2);
    const auto& driven_open_two = sheep_with_id(driven_open_run.final_snapshot.sheep, 2);
    const auto& driven_pressure =
        evidence_with_id(driven_closed_run.final_snapshot.sheep_dog_pressure_evidence, 2);
    const auto& driven_collision =
        evidence_with_id(driven_closed_run.final_snapshot.sheep_collision_evidence, 2);
    if (!check(driven_contact.observed &&
                   driven_contact.evidence.obstacle == wide_eye::game::PaddockObstacle::gate &&
                   driven_closed_two.position.z == kWallRestZ &&
                   driven_closed_two.velocity.z == 0.0 &&
                   driven_contact.contact_ticks == kDrivenCollisionTicks - driven_contact.tick + 1,
               "a_closed_gate_holds_a_dog_driven_sheep_on_every_pushed_tick") ||
        !check(!contact_with_id(driven_open_run.contacts, 2).observed &&
                   sheep_with_id(driven_open_run.midpoint.sheep, 2).position.z < 15.0 &&
                   driven_open_two.position.z < driven_closed_two.position.z &&
                   driven_open_two.velocity.z < 0.0,
               "the_same_dog_drives_that_sheep_through_an_open_gate") ||
        // Collision is a later positional authority, not a steering correction:
        // the published dog term still describes the pressure that was applied
        // even on a tick where the paddock refused the resulting displacement.
        !check(driven_pressure.dog_distance == 3.5 &&
                   driven_pressure.pressure_acceleration.z == -1.25 && driven_collision.clipped_z &&
                   driven_collision.obstacle == wide_eye::game::PaddockObstacle::gate,
               "a_clipped_tick_still_publishes_the_steering_it_applied")) {
        return EXIT_FAILURE;
    }

    auto reversed_collision_scenario = *collision_closed_scenario;
    std::reverse(reversed_collision_scenario.initial_sheep.begin(),
                 reversed_collision_scenario.initial_sheep.end());
    const PaddockCollisionRun reversed_collision_run = run_paddock_collision(
        reversed_collision_scenario, kPaddockCollisionTicks, kPaddockCollisionMidpointTick);
    for (const auto& member : closed_gate_run.final_snapshot.sheep) {
        const SheepContactRecord& expected = contact_with_id(closed_gate_run.contacts, member.id);
        const SheepContactRecord& observed =
            contact_with_id(reversed_collision_run.contacts, member.id);
        if (!check(member == sheep_with_id(reversed_collision_run.final_snapshot.sheep, member.id),
                   "collision_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(closed_gate_run.final_snapshot.sheep_collision_evidence,
                                 member.id) ==
                    evidence_with_id(reversed_collision_run.final_snapshot.sheep_collision_evidence,
                                     member.id),
                "collision_evidence_is_stable_under_reversed_storage") ||
            !check(expected.observed == observed.observed && expected.tick == observed.tick &&
                       expected.contact_ticks == observed.contact_ticks &&
                       expected.state == observed.state && expected.evidence == observed.evidence,
                   "first_contact_is_stable_by_id_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    wide_eye::game::GameplaySimulation collision_dump{driven_closed_scenario};
    for (std::uint64_t tick = 0; tick < 100; ++tick) {
        collision_dump.fixed_update({});
    }
    const auto collision_state = wide_eye::game::gameplay_state_dump_json(collision_dump);
    if (!check(
            collision_state &&
                collision_state.text.find("\"sheep_collision_evidence\":[") != std::string::npos &&
                collision_state.text.find("\"clipped_z\":true") != std::string::npos &&
                collision_state.text.find("\"contact_obstacle\":\"gate\"") != std::string::npos &&
                collision_state.text.find("\"contact_obstacle\":\"none\"") != std::string::npos,
            "state_dump_contains_sheep_collision_contact_and_obstacle")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation allocation_collision{*collision_closed_scenario};
    const std::size_t collision_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_collision.fixed_update({});
    }
    const std::size_t collision_allocations = g_allocation_count - collision_allocations_before;
    if (!check(collision_allocations == 0, "collision_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation collision_restart{*collision_closed_scenario};
    const auto collision_initial = collision_restart.current_snapshot();
    for (std::uint64_t tick = 0; tick < 120; ++tick) {
        collision_restart.fixed_update({});
    }
    collision_restart.restart();
    if (!check(collision_restart.current_snapshot() == collision_initial &&
                   collision_restart.previous_snapshot() == collision_initial,
               "collision_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    // Temperament. The accepted design makes the dog's effective pressure depend
    // on distance, approach speed, facing, terrain, and the sheep's temperament,
    // so temperament is a response scale carried by the sheep rather than a
    // fourth social term. The paired fixture stands five sheep on one exact
    // 5-unit ring around a stationary dog: every sheep sees the same stimulus,
    // so the only thing that can separate two of them is the temperament each
    // one carries.
    constexpr double kNervousResponseScale = 2.0;
    constexpr double kStubbornResponseScale = 0.5;
    constexpr double kTemperamentRingDistance = 5.0;
    // The accepted 6-unit radius and 3.0 maximum give an ordinary sheep exactly
    // (1 - 5/6) * 3 = 0.5 of pressure at that ring.
    constexpr double kOrdinaryRingPressure = 0.5;
    constexpr std::uint64_t kTemperamentDriftTicks = 120;
    const auto temperament_neutral_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-temperament-neutral");
    const auto temperament_varied_scenario =
        wide_eye::game::find_gameplay_scenario("sheep-temperament-varied");
    auto temperament_varied_as_control =
        temperament_varied_scenario.value_or(wide_eye::game::GameplayScenarioDefinition{});
    if (temperament_neutral_scenario.has_value()) {
        temperament_varied_as_control.id = temperament_neutral_scenario->id;
    }
    temperament_varied_as_control.sheep_temperament.enabled = false;
    if (!check(
            temperament_neutral_scenario.has_value() && temperament_varied_scenario.has_value() &&
                temperament_neutral_scenario->id ==
                    wide_eye::game::GameplayScenarioId::sheep_temperament_neutral &&
                temperament_varied_scenario->id ==
                    wide_eye::game::GameplayScenarioId::sheep_temperament_varied &&
                temperament_varied_as_control == *temperament_neutral_scenario &&
                temperament_varied_scenario->sheep_temperament.enabled &&
                !temperament_neutral_scenario->sheep_temperament.enabled &&
                temperament_varied_scenario->sheep_temperament.nervous_response_scale ==
                    kNervousResponseScale &&
                temperament_varied_scenario->sheep_temperament.stubborn_response_scale ==
                    kStubbornResponseScale &&
                temperament_neutral_scenario->version == 1 &&
                temperament_neutral_scenario->seed == 0 &&
                temperament_neutral_scenario->sheep_dog_pressure.enabled &&
                temperament_neutral_scenario->sheep_dog_pressure ==
                    temperament_varied_scenario->sheep_dog_pressure &&
                !temperament_neutral_scenario->sheep_separation.enabled &&
                !temperament_neutral_scenario->sheep_attraction.enabled &&
                !temperament_neutral_scenario->sheep_alignment.enabled &&
                !temperament_neutral_scenario->sheep_dog_approach.enabled &&
                !temperament_neutral_scenario->sheep_dog_facing.enabled &&
                !temperament_neutral_scenario->sheep_dog_line_of_sight.enabled &&
                temperament_neutral_scenario->dog.initial_state.velocity == wide_eye::game::Vec3{},
            "paired_temperament_fixture_differs_only_by_temperament_switch")) {
        return EXIT_FAILURE;
    }

    // Temperament belongs to the sheep, so both members of the pair carry the
    // same assignment and only the switch decides whether it does anything.
    const auto& temperament_fixture = temperament_varied_scenario->initial_sheep;
    if (!check(temperament_fixture == temperament_neutral_scenario->initial_sheep &&
                   sheep_with_id(temperament_fixture, 1).temperament ==
                       wide_eye::game::SheepTemperament::ordinary &&
                   sheep_with_id(temperament_fixture, 2).temperament ==
                       wide_eye::game::SheepTemperament::nervous &&
                   sheep_with_id(temperament_fixture, 3).temperament ==
                       wide_eye::game::SheepTemperament::stubborn &&
                   sheep_with_id(temperament_fixture, 4).temperament ==
                       wide_eye::game::SheepTemperament::stubborn &&
                   sheep_with_id(temperament_fixture, 5).temperament ==
                       wide_eye::game::SheepTemperament::nervous,
               "temperament_is_part_of_the_shared_scenario_fixture")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation temperament_neutral{*temperament_neutral_scenario};
    wide_eye::game::GameplaySimulation temperament_varied{*temperament_varied_scenario};
    const auto temperament_initial = temperament_varied.current_snapshot();
    temperament_neutral.fixed_update({});
    temperament_varied.fixed_update({});
    const auto& temperament_neutral_after_one = temperament_neutral.current_snapshot();
    const auto& temperament_varied_after_one = temperament_varied.current_snapshot();
    const auto expected_temperament_scale = [](wide_eye::game::SheepTemperament temperament) {
        switch (temperament) {
        case wide_eye::game::SheepTemperament::nervous:
            return kNervousResponseScale;
        case wide_eye::game::SheepTemperament::stubborn:
            return kStubbornResponseScale;
        case wide_eye::game::SheepTemperament::ordinary:
            break;
        }
        return 1.0;
    };
    if (!check(temperament_varied.previous_snapshot() == temperament_initial,
               "temperament_reads_immutable_prior_snapshot")) {
        return EXIT_FAILURE;
    }

    for (const auto& varied_evidence : temperament_varied_after_one.sheep_dog_pressure_evidence) {
        const std::uint32_t subject_id = varied_evidence.subject_id;
        const auto& neutral_evidence =
            evidence_with_id(temperament_neutral_after_one.sheep_dog_pressure_evidence, subject_id);
        const auto& prior_member = sheep_with_id(temperament_initial.sheep, subject_id);
        const double scale = expected_temperament_scale(prior_member.temperament);
        const auto applied = [&prior_member](const wide_eye::game::SheepState& member) {
            return wide_eye::game::Vec3{.x = (member.velocity.x - prior_member.velocity.x) /
                                             wide_eye::game::GameplaySimulation::kFixedDeltaSeconds,
                                        .z =
                                            (member.velocity.z - prior_member.velocity.z) /
                                            wide_eye::game::GameplaySimulation::kFixedDeltaSeconds};
        };
        const auto varied_applied =
            applied(sheep_with_id(temperament_varied_after_one.sheep, subject_id));
        const auto neutral_applied =
            applied(sheep_with_id(temperament_neutral_after_one.sheep, subject_id));
        const double neutral_magnitude = std::hypot(neutral_evidence.pressure_acceleration.x,
                                                    neutral_evidence.pressure_acceleration.z);
        const double varied_magnitude = std::hypot(varied_evidence.pressure_acceleration.x,
                                                   varied_evidence.pressure_acceleration.z);
        if (!check(varied_evidence.stimulus_evaluated &&
                       varied_evidence.dog_distance == kTemperamentRingDistance &&
                       neutral_evidence.dog_distance == kTemperamentRingDistance &&
                       neutral_evidence.dog_relative_bearing_radians ==
                           varied_evidence.dog_relative_bearing_radians &&
                       neutral_evidence.dog_approach_speed == varied_evidence.dog_approach_speed &&
                       neutral_evidence.dog_facing_alignment ==
                           varied_evidence.dog_facing_alignment &&
                       neutral_evidence.dog_line_of_sight_blocked ==
                           varied_evidence.dog_line_of_sight_blocked &&
                       neutral_evidence.dog_line_of_sight_occluder ==
                           varied_evidence.dog_line_of_sight_occluder,
                   "temperament_leaves_the_published_dog_stimulus_identical") ||
            !check(neutral_evidence.temperament_response_scale == 1.0 &&
                       varied_evidence.temperament_response_scale == scale,
                   "published_temperament_scale_names_the_factor_that_was_applied") ||
            // Exact equality, not a tolerance: the configured factors are powers
            // of two, so a scaled response is the neutral response with one
            // exponent changed and an ordinary sheep is bit-for-bit unchanged.
            !check(varied_evidence.pressure_acceleration.x ==
                           scale * neutral_evidence.pressure_acceleration.x &&
                       varied_evidence.pressure_acceleration.z ==
                           scale * neutral_evidence.pressure_acceleration.z,
                   "temperament_scales_the_accepted_pressure_by_exactly_its_factor") ||
            !check(std::abs(neutral_magnitude - kOrdinaryRingPressure) < 1.0e-12 &&
                       std::abs(varied_magnitude - scale * kOrdinaryRingPressure) < 1.0e-12,
                   "every_ring_sheep_responds_at_its_temperament_share_of_one_pressure") ||
            !check(std::abs(varied_applied.x - varied_evidence.pressure_acceleration.x) < 1.0e-12 &&
                       std::abs(varied_applied.z - varied_evidence.pressure_acceleration.z) <
                           1.0e-12 &&
                       std::abs(neutral_applied.x - neutral_evidence.pressure_acceleration.x) <
                           1.0e-12 &&
                       std::abs(neutral_applied.z - neutral_evidence.pressure_acceleration.z) <
                           1.0e-12,
                   "published_temperament_terms_match_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    // Copies, not references: the restart oracle below rewinds this simulation,
    // and these records are still reported at the end of the run.
    const auto ordinary_pressure =
        evidence_with_id(temperament_varied_after_one.sheep_dog_pressure_evidence, 1);
    const auto near_nervous_pressure =
        evidence_with_id(temperament_varied_after_one.sheep_dog_pressure_evidence, 2);
    const auto near_stubborn_pressure =
        evidence_with_id(temperament_varied_after_one.sheep_dog_pressure_evidence, 3);
    const auto far_stubborn_pressure =
        evidence_with_id(temperament_varied_after_one.sheep_dog_pressure_evidence, 4);
    const auto far_nervous_pressure =
        evidence_with_id(temperament_varied_after_one.sheep_dog_pressure_evidence, 5);
    if (!check(ordinary_pressure.pressure_acceleration ==
                   evidence_with_id(temperament_neutral_after_one.sheep_dog_pressure_evidence, 1)
                       .pressure_acceleration,
               "an_ordinary_sheep_reproduces_the_accepted_pressure_bit_for_bit") ||
        // The nervous and stubborn sheep are mirrored across the gate line, so
        // the exact 4:1 ratio between them cannot be an artifact of one bearing.
        !check(near_nervous_pressure.pressure_acceleration.x ==
                       -(kNervousResponseScale / kStubbornResponseScale) *
                           near_stubborn_pressure.pressure_acceleration.x &&
                   near_nervous_pressure.pressure_acceleration.z ==
                       (kNervousResponseScale / kStubbornResponseScale) *
                           near_stubborn_pressure.pressure_acceleration.z &&
                   far_nervous_pressure.pressure_acceleration.x ==
                       -(kNervousResponseScale / kStubbornResponseScale) *
                           far_stubborn_pressure.pressure_acceleration.x &&
                   far_nervous_pressure.pressure_acceleration.z ==
                       (kNervousResponseScale / kStubbornResponseScale) *
                           far_stubborn_pressure.pressure_acceleration.z,
               "mirrored_bearings_swap_the_response_with_the_temperament")) {
        return EXIT_FAILURE;
    }

    // "Ordinary is exactly neutral" is a claim about arithmetic, not about
    // tuning: a fixture whose sheep are all ordinary must produce the same
    // authoritative state with the factor switched on as with it switched off,
    // for every tick, including every published evidence field.
    auto all_ordinary_scenario = *temperament_varied_scenario;
    for (auto& member : all_ordinary_scenario.initial_sheep) {
        member.temperament = wide_eye::game::SheepTemperament::ordinary;
    }
    auto all_ordinary_control_scenario = all_ordinary_scenario;
    all_ordinary_control_scenario.id = temperament_neutral_scenario->id;
    all_ordinary_control_scenario.sheep_temperament.enabled = false;
    wide_eye::game::GameplaySimulation all_ordinary{all_ordinary_scenario};
    wide_eye::game::GameplaySimulation all_ordinary_control{all_ordinary_control_scenario};
    bool all_ordinary_is_neutral = true;
    for (std::uint64_t tick = 0; tick < kTemperamentDriftTicks; ++tick) {
        all_ordinary.fixed_update({});
        all_ordinary_control.fixed_update({});
        all_ordinary_is_neutral =
            all_ordinary_is_neutral &&
            all_ordinary.current_snapshot() == all_ordinary_control.current_snapshot();
    }
    if (!check(all_ordinary_is_neutral,
               "an_all_ordinary_flock_is_identical_with_the_factor_on_or_off")) {
        return EXIT_FAILURE;
    }

    // The vector-level ratio should also be visible as motion: over the same
    // ticks, from the same bearing, the nervous sheep must end further from the
    // dog than its mirrored stubborn twin, with neither having touched anything
    // so the comparison stays pure steering.
    wide_eye::game::GameplaySimulation temperament_drift{*temperament_varied_scenario};
    bool temperament_drift_is_contact_free = true;
    for (std::uint64_t tick = 0; tick < kTemperamentDriftTicks; ++tick) {
        temperament_drift.fixed_update({});
        for (const auto& contact : temperament_drift.current_snapshot().sheep_collision_evidence) {
            temperament_drift_is_contact_free =
                temperament_drift_is_contact_free && !contact.clipped_x && !contact.clipped_z &&
                contact.obstacle == wide_eye::game::PaddockObstacle::none;
        }
    }
    const auto& drift_dog = temperament_drift.current_snapshot().dog;
    const auto dog_range = [&drift_dog](const wide_eye::game::SheepState& member) {
        return std::hypot(member.position.x - drift_dog.position.x,
                          member.position.z - drift_dog.position.z);
    };
    const double nervous_range =
        dog_range(sheep_with_id(temperament_drift.current_snapshot().sheep, 2));
    const double stubborn_range =
        dog_range(sheep_with_id(temperament_drift.current_snapshot().sheep, 3));
    if (!check(temperament_drift_is_contact_free && stubborn_range > kTemperamentRingDistance &&
                   nervous_range > stubborn_range,
               "a_nervous_sheep_outruns_its_mirrored_stubborn_twin")) {
        return EXIT_FAILURE;
    }

    // Temperament scales the whole dog response, not only the distance term, so
    // a derived fixture that also enables approach and facing must scale all
    // three vectors by the same published factor while the stimulus that
    // produced them stays identical.
    auto combined_temperament_varied_scenario = *temperament_varied_scenario;
    combined_temperament_varied_scenario.dog.initial_state.velocity = {.z = -3.0};
    combined_temperament_varied_scenario.sheep_dog_approach.enabled = true;
    combined_temperament_varied_scenario.sheep_dog_facing.enabled = true;
    auto combined_temperament_neutral_scenario = combined_temperament_varied_scenario;
    combined_temperament_neutral_scenario.id = temperament_neutral_scenario->id;
    combined_temperament_neutral_scenario.sheep_temperament.enabled = false;
    wide_eye::game::GameplaySimulation combined_temperament_varied{
        combined_temperament_varied_scenario};
    wide_eye::game::GameplaySimulation combined_temperament_neutral{
        combined_temperament_neutral_scenario};
    combined_temperament_varied.fixed_update({});
    combined_temperament_neutral.fixed_update({});
    bool combined_temperament_scales_every_term = true;
    for (const std::uint32_t subject_id : {1U, 2U, 3U, 4U, 5U}) {
        const auto& varied_evidence = evidence_with_id(
            combined_temperament_varied.current_snapshot().sheep_dog_pressure_evidence, subject_id);
        const auto& neutral_evidence = evidence_with_id(
            combined_temperament_neutral.current_snapshot().sheep_dog_pressure_evidence,
            subject_id);
        const double scale =
            expected_temperament_scale(sheep_with_id(temperament_fixture, subject_id).temperament);
        combined_temperament_scales_every_term =
            combined_temperament_scales_every_term &&
            neutral_evidence.dog_approach_speed == varied_evidence.dog_approach_speed &&
            neutral_evidence.dog_facing_alignment == varied_evidence.dog_facing_alignment &&
            neutral_evidence.pressure_acceleration != wide_eye::game::Vec3{} &&
            neutral_evidence.approach_acceleration != wide_eye::game::Vec3{} &&
            neutral_evidence.facing_acceleration != wide_eye::game::Vec3{} &&
            varied_evidence.pressure_acceleration.x ==
                scale * neutral_evidence.pressure_acceleration.x &&
            varied_evidence.pressure_acceleration.z ==
                scale * neutral_evidence.pressure_acceleration.z &&
            varied_evidence.approach_acceleration.x ==
                scale * neutral_evidence.approach_acceleration.x &&
            varied_evidence.approach_acceleration.z ==
                scale * neutral_evidence.approach_acceleration.z &&
            varied_evidence.facing_acceleration.x ==
                scale * neutral_evidence.facing_acceleration.x &&
            varied_evidence.facing_acceleration.z == scale * neutral_evidence.facing_acceleration.z;
    }
    const auto& combined_head_on = evidence_with_id(
        combined_temperament_varied.current_snapshot().sheep_dog_pressure_evidence, 1);
    const auto& combined_diagonal = evidence_with_id(
        combined_temperament_varied.current_snapshot().sheep_dog_pressure_evidence, 2);
    if (!check(combined_temperament_scales_every_term,
               "temperament_scales_pressure_approach_and_facing_by_one_factor") ||
        !check(combined_head_on.dog_approach_speed == 3.0 &&
                   combined_head_on.dog_facing_alignment == 1.0 &&
                   std::abs(combined_diagonal.dog_approach_speed - 2.4) < 1.0e-12 &&
                   std::abs(combined_diagonal.dog_facing_alignment - 0.8) < 1.0e-12,
               "temperament_does_not_alter_the_measured_approach_or_facing_stimulus")) {
        return EXIT_FAILURE;
    }

    // Deliberate scope: the design names temperament as a factor of the dog's
    // effective pressure and does not give the flock-neighbour terms one, so a
    // derived fixture with every social term enabled must publish identical
    // social evidence while the dog vectors still differ.
    auto social_temperament_varied_scenario = *temperament_varied_scenario;
    social_temperament_varied_scenario.sheep_separation.enabled = true;
    social_temperament_varied_scenario.sheep_attraction.enabled = true;
    social_temperament_varied_scenario.sheep_alignment.enabled = true;
    auto social_temperament_neutral_scenario = social_temperament_varied_scenario;
    social_temperament_neutral_scenario.id = temperament_neutral_scenario->id;
    social_temperament_neutral_scenario.sheep_temperament.enabled = false;
    wide_eye::game::GameplaySimulation social_temperament_varied{
        social_temperament_varied_scenario};
    wide_eye::game::GameplaySimulation social_temperament_neutral{
        social_temperament_neutral_scenario};
    social_temperament_varied.fixed_update({});
    social_temperament_neutral.fixed_update({});
    bool temperament_leaves_social_terms_alone = true;
    for (const auto& varied_social :
         social_temperament_varied.current_snapshot().sheep_social_evidence) {
        temperament_leaves_social_terms_alone =
            temperament_leaves_social_terms_alone &&
            varied_social ==
                evidence_with_id(
                    social_temperament_neutral.current_snapshot().sheep_social_evidence,
                    varied_social.subject_id);
    }
    const auto& social_ordinary_evidence =
        evidence_with_id(social_temperament_varied.current_snapshot().sheep_social_evidence, 1);
    const auto& social_nervous_dog = evidence_with_id(
        social_temperament_varied.current_snapshot().sheep_dog_pressure_evidence, 2);
    const auto& social_neutral_dog = evidence_with_id(
        social_temperament_neutral.current_snapshot().sheep_dog_pressure_evidence, 2);
    if (!check(temperament_leaves_social_terms_alone,
               "temperament_leaves_every_social_term_identical") ||
        !check(social_ordinary_evidence.attraction_candidate_count == 2 &&
                   social_ordinary_evidence.attraction_acceleration != wide_eye::game::Vec3{} &&
                   social_nervous_dog.pressure_acceleration.z ==
                       kNervousResponseScale * social_neutral_dog.pressure_acceleration.z,
               "the_social_comparison_is_not_vacuous")) {
        return EXIT_FAILURE;
    }

    // The dog motor moves within the same tick. The published scale and the
    // vectors it scaled must still describe the prior state that caused them.
    // The dog already faces -z here, so driving it that way moves it on the
    // first tick instead of spending the tick turning around.
    wide_eye::game::GameplaySimulation temperament_same_tick_move{*temperament_varied_scenario};
    temperament_same_tick_move.fixed_update(
        {.dog_move = wide_eye::game::DogMoveInput{.world_z = -1.0}});
    if (!check(temperament_same_tick_move.current_snapshot().dog.position.z <
                       temperament_varied_scenario->dog.initial_state.position.z &&
                   evidence_with_id(
                       temperament_same_tick_move.current_snapshot().sheep_dog_pressure_evidence,
                       2) == near_nervous_pressure,
               "same_tick_dog_motor_move_does_not_alter_prior_state_temperament_evidence")) {
        return EXIT_FAILURE;
    }

    auto reversed_temperament_scenario = *temperament_varied_scenario;
    std::reverse(reversed_temperament_scenario.initial_sheep.begin(),
                 reversed_temperament_scenario.initial_sheep.end());
    wide_eye::game::GameplaySimulation reversed_temperament{reversed_temperament_scenario};
    reversed_temperament.fixed_update({});
    for (const auto& member : temperament_varied_after_one.sheep) {
        if (!check(member ==
                       sheep_with_id(reversed_temperament.current_snapshot().sheep, member.id),
                   "temperament_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(temperament_varied_after_one.sheep_dog_pressure_evidence,
                                    member.id) ==
                       evidence_with_id(
                           reversed_temperament.current_snapshot().sheep_dog_pressure_evidence,
                           member.id),
                   "temperament_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto temperament_state = wide_eye::game::gameplay_state_dump_json(temperament_varied);
    if (!check(
            temperament_state &&
                temperament_state.text.find("\"temperament\":\"ordinary\"") != std::string::npos &&
                temperament_state.text.find("\"temperament\":\"nervous\"") != std::string::npos &&
                temperament_state.text.find("\"temperament\":\"stubborn\"") != std::string::npos &&
                temperament_state.text.find("\"temperament_response_scale\":2") !=
                    std::string::npos &&
                temperament_state.text.find("\"temperament_response_scale\":0.5") !=
                    std::string::npos &&
                temperament_state.text.find("\"temperament_response_scale\":1") !=
                    std::string::npos,
            "state_dump_contains_sheep_temperament_and_applied_scale")) {
        return EXIT_FAILURE;
    }

    auto unknown_temperament_scenario = *temperament_varied_scenario;
    unknown_temperament_scenario.initial_sheep[0].temperament =
        static_cast<wide_eye::game::SheepTemperament>(255);
    const wide_eye::game::GameplaySimulation unknown_temperament_simulation{
        unknown_temperament_scenario};
    if (!check(wide_eye::game::gameplay_state_dump_json(unknown_temperament_simulation).error ==
                   wide_eye::game::GameplayContractError::non_finite_state,
               "state_dump_rejects_unknown_sheep_temperament")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation allocation_temperament{*temperament_varied_scenario};
    const std::size_t temperament_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_temperament.fixed_update({});
    }
    const std::size_t temperament_allocations = g_allocation_count - temperament_allocations_before;
    if (!check(temperament_allocations == 0, "temperament_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }

    for (std::uint32_t tick = 0; tick < 120; ++tick) {
        temperament_varied.fixed_update({});
    }
    temperament_varied.restart();
    bool temperament_restart_restores_labels = true;
    for (const auto& member : temperament_varied.current_snapshot().sheep) {
        temperament_restart_restores_labels =
            temperament_restart_restores_labels &&
            member.temperament == sheep_with_id(temperament_fixture, member.id).temperament;
    }
    if (!check(temperament_varied.current_snapshot() == temperament_initial &&
                   temperament_varied.previous_snapshot() == temperament_initial &&
                   temperament_restart_restores_labels,
               "temperament_restart_restores_the_fixture_including_labels")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation replay_a{*scenario};
    wide_eye::game::GameplaySimulation replay_b{*scenario};
    const wide_eye::game::GameplayReplay replay = sample_replay(replay_a);
    const auto replay_text = wide_eye::game::gameplay_replay_json(replay);
    if (!check(wide_eye::game::kGameplaySeedFormatVersion == 1 &&
                   wide_eye::game::kGameplayActionInputFormatVersion == 1 &&
                   wide_eye::game::kGameplayReplayFormatVersion == 1 &&
                   wide_eye::game::kGameplayStateDumpFormatVersion == 10,
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
        !check(state_a.text.starts_with("{\"schema\":\"wide-eye.gameplay-state\",\"version\":10,"
                                        "\"tick_rate\":60,\"scenario\":{"),
               "state_dump_schema_header") ||
        !check(state_a.text.find("\"current\":{\"tick\":3") != std::string::npos,
               "state_dump_contains_authoritative_tick") ||
        !check(state_a.text.find("\"sheep\":[{\"id\":1") != std::string::npos &&
                   state_a.text.find("\"id\":5") != std::string::npos &&
                   state_a.text.find("\"behavior\":\"settled\"") != std::string::npos &&
                   state_a.text.find("\"temperament\":\"ordinary\"") != std::string::npos &&
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

    std::cout
        << "authoritative_tick_hz=" << wide_eye::game::GameplaySimulation::kTicksPerSecond << '\n'
        << "fine_render_frames=" << fine_frames.size() << '\n'
        << "coarse_render_frames=" << coarse_frames.size() << '\n'
        << "authoritative_ticks=" << fine.snapshot.tick << '\n'
        << "cadence_state_equal=yes\n"
        << "replay_contract_version=" << wide_eye::game::kGameplayReplayFormatVersion << '\n'
        << "state_dump_contract_version=" << wide_eye::game::kGameplayStateDumpFormatVersion << '\n'
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
        << "dog_line_of_sight_clear_pressure_z=" << clear_sight.pressure_acceleration.z << '\n'
        << "dog_line_of_sight_control_left_wall_pressure_x=" << blocked_left_pressure.x << '\n'
        << "dog_line_of_sight_occluded_pressure="
        << std::hypot(left_wall_sight.pressure_acceleration.x,
                      left_wall_sight.pressure_acceleration.z)
        << '\n'
        << "dog_line_of_sight_gate_gap_pressure_z=" << gate_gap_sight.pressure_acceleration.z
        << '\n'
        << "dog_line_of_sight_closed_gate_blocks_gate_gap=yes\n"
        << "dog_line_of_sight_steady_state_allocations=" << sight_allocations << '\n'
        << "sheep_paddock_collision_fixture=paired_gate_state_control\n"
        << "sheep_paddock_collision_radius=" << wide_eye::game::kSheepCollisionRadius << '\n'
        << "sheep_left_wall_contact_tick=" << left_wall_contact.tick << '\n'
        << "sheep_left_wall_rest_z=" << left_wall_contact.state.position.z << '\n'
        << "sheep_closed_gate_rest_z=" << gate_contact.state.position.z << '\n'
        << "sheep_right_wall_free_axis_velocity_x=" << right_wall_contact.state.velocity.x << '\n'
        << "sheep_outer_bound_rest_x=" << bound_contact.state.position.x << '\n'
        << "sheep_open_gate_final_z=" << open_gate_sheep_two.position.z << '\n'
        << "sheep_untouched_control_final_x="
        << sheep_with_id(closed_gate_run.final_snapshot.sheep, 4).position.x << '\n'
        << "sheep_dog_driven_gate_contact_tick=" << driven_contact.tick << '\n'
        << "sheep_dog_driven_gate_contact_ticks=" << driven_contact.contact_ticks << '\n'
        << "sheep_dog_driven_pinned_pressure_z=" << driven_pressure.pressure_acceleration.z << '\n'
        << "sheep_collision_steady_state_allocations=" << collision_allocations << '\n'
        << "sheep_temperament_fixture=equal_stimulus_ring_paired_control\n"
        << "sheep_temperament_ring_distance=" << ordinary_pressure.dog_distance << '\n'
        << "sheep_temperament_ordinary_scale=" << ordinary_pressure.temperament_response_scale
        << '\n'
        << "sheep_temperament_nervous_scale=" << near_nervous_pressure.temperament_response_scale
        << '\n'
        << "sheep_temperament_stubborn_scale=" << near_stubborn_pressure.temperament_response_scale
        << '\n'
        << "sheep_temperament_ordinary_pressure="
        << std::hypot(ordinary_pressure.pressure_acceleration.x,
                      ordinary_pressure.pressure_acceleration.z)
        << '\n'
        << "sheep_temperament_nervous_pressure="
        << std::hypot(near_nervous_pressure.pressure_acceleration.x,
                      near_nervous_pressure.pressure_acceleration.z)
        << '\n'
        << "sheep_temperament_stubborn_pressure="
        << std::hypot(near_stubborn_pressure.pressure_acceleration.x,
                      near_stubborn_pressure.pressure_acceleration.z)
        << '\n'
        << "sheep_temperament_drift_ticks=" << kTemperamentDriftTicks << '\n'
        << "sheep_temperament_nervous_dog_range=" << nervous_range << '\n'
        << "sheep_temperament_stubborn_dog_range=" << stubborn_range << '\n'
        << "sheep_temperament_scales_social_terms=no\n"
        << "sheep_temperament_steady_state_allocations=" << temperament_allocations << '\n'
        << "repeated_local_replay_equal=yes\n"
        << "gameplay_simulation_result=pass\n";
    return EXIT_SUCCESS;
}
