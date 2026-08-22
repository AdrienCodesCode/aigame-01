#include "core/runtime.hpp"
#include "game/flock_observables.hpp"
#include "game/gameplay_replay.hpp"
#include "game/gameplay_simulation.hpp"
#include "game/sheep_rules.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

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

// `GameplaySimulation` is about 115 KiB, because `SheepSpatialGrid` carries the
// 1,000-member capacity-experiment ceiling while the game has five sheep, and
// every byte added to a published snapshot is multiplied by however many of them
// a frame holds. `main` and the oracles between them keep more than seventy alive
// at once, so **every** fixture in this file is held by pointer rather than by
// value; holding one by value cost about 115 KiB of stack and QA-002 recorded the
// silent SIGSEGV that eventually caused. `wide_eye.gameplay_simulation_stack_budget`
// is the guard that keeps this true. Every handle is constructed outside the
// windows that count allocations, so this does not weaken the zero-allocation
// oracles.
using SimulationHandle = std::unique_ptr<wide_eye::game::GameplaySimulation>;

// Published sheep state is a capacity-sized buffer plus an active count. Every
// pass that takes a flock takes the active range, never the whole buffer.
// A chosen-neighbour count of zero for each member of a five-member fixture:
// the observable pass takes the counts as an explicit input, and these
// fixtures are measuring something other than neighbour selection.
using NoChosenNeighbors = std::array<std::uint32_t, wide_eye::game::kDefaultGameplaySheepCount>;

[[nodiscard]] std::span<const wide_eye::game::SheepState>
active_sheep(const wide_eye::game::GameplaySnapshot& snapshot) noexcept {
    return {snapshot.sheep.data(), snapshot.sheep_count};
}

// The active range of any published per-member buffer. The buffers are sized at
// `kMaximumGameplaySheepCount` and the entries past the active count are filler
// no rule ever wrote, so iterating the whole buffer reads records that do not
// exist.
template <typename Record, std::size_t Capacity>
[[nodiscard]] std::span<const Record> active(const std::array<Record, Capacity>& buffer,
                                             std::size_t count) noexcept {
    return {buffer.data(), count};
}

// Fixtures a check only reads. The pointee is const so the read-only intent the
// by-value fixtures used to express with `const` survives the move to the heap.
using ConstSimulationHandle = std::unique_ptr<const wide_eye::game::GameplaySimulation>;

SimulationHandle make_simulation(const wide_eye::game::GameplayScenarioDefinition& scenario) {
    return std::make_unique<wide_eye::game::GameplaySimulation>(scenario);
}

// A named scenario, on the heap. `GameplayScenarioDefinition` carries the
// initial sheep buffer at the authoritative capacity, so it is 20 KiB, and this
// harness names about thirty scenarios inside one `main`. Holding them by value
// is the frame growth QA-002 recorded, one level up from the simulations.
using ScenarioHandle = std::unique_ptr<const wide_eye::game::GameplayScenarioDefinition>;

// A mutable heap copy of a named scenario, for the paired oracles that assert
// "the on scenario with its one switch turned off is exactly the off scenario".
[[nodiscard]] std::unique_ptr<wide_eye::game::GameplayScenarioDefinition>
mutable_scenario_copy(
    const std::unique_ptr<const wide_eye::game::GameplayScenarioDefinition>& scenario) {
    return std::make_unique<wide_eye::game::GameplayScenarioDefinition>(
        scenario != nullptr ? *scenario : wide_eye::game::GameplayScenarioDefinition{});
}

[[nodiscard]] ScenarioHandle named_scenario(std::string_view name) {
    const auto found = wide_eye::game::find_gameplay_scenario(name);
    if (!found.has_value()) {
        return nullptr;
    }
    return std::make_unique<const wide_eye::game::GameplayScenarioDefinition>(*found);
}

struct CadenceResult {
    wide_eye::game::GameplaySnapshot snapshot{};
    std::uint64_t scheduled_ticks = 0;
};

// Heap, not stack: a `GameplaySnapshot` is 116 KiB at the published capacity,
// and a fixture that holds one by value in `main` is the shape QA-002 recorded.
using CadenceHandle = std::unique_ptr<CadenceResult>;

// The discarded interpolated snapshot is 116 KiB and is materialized in its
// caller's frame, so it is discarded inside a frame that is reclaimed.
void observe_interpolated(const wide_eye::game::GameplaySimulation& simulation,
                          double alpha) {
    static_cast<void>(simulation.interpolated_snapshot(alpha));
}

CadenceHandle run_cadence(const wide_eye::game::GameplayScenarioDefinition& scenario,
                          std::span<const std::chrono::nanoseconds> frame_deltas) {
    auto result = std::make_unique<CadenceResult>();
    const auto before_observation = std::make_unique<wide_eye::game::GameplaySnapshot>();
    wide_eye::core::FixedStepAccumulator scheduler;
    const SimulationHandle simulation = make_simulation(scenario);

    for (const std::chrono::nanoseconds frame_delta : frame_deltas) {
        const wide_eye::core::FixedStepUpdate update = scheduler.advance(frame_delta);
        for (std::uint32_t index = 0; index < update.ticks; ++index) {
            simulation->fixed_update(input_for_tick(simulation->current_snapshot().tick));
        }

        *before_observation = simulation->current_snapshot();
        observe_interpolated(*simulation, update.interpolation_alpha);
        if (simulation->current_snapshot() != *before_observation) {
            return result;
        }
    }

    result->snapshot = simulation->current_snapshot();
    result->scheduled_ticks = scheduler.total_ticks();
    return result;
}

// `interpolated_snapshot` returns a whole 116 KiB snapshot by value, which is
// materialized in the caller's frame. These helpers keep that temporary inside
// a frame that is reclaimed on return rather than inside `main`.
[[nodiscard]] std::unique_ptr<wide_eye::game::GameplaySnapshot>
interpolated_on_heap(const wide_eye::game::GameplaySimulation& simulation, double alpha) {
    return std::make_unique<wide_eye::game::GameplaySnapshot>(
        simulation.interpolated_snapshot(alpha));
}

[[nodiscard]] bool interpolated_dog_equals(const wide_eye::game::GameplaySimulation& simulation,
                                           double alpha,
                                           const wide_eye::game::DogState& expected) {
    return simulation.interpolated_snapshot(alpha).dog == expected;
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

const wide_eye::game::SheepAvoidanceEvidence&
evidence_with_id(const wide_eye::game::SheepAvoidanceEvidenceBuffer& evidence, std::uint32_t id) {
    const auto member = std::find_if(evidence.begin(), evidence.end(), [id](const auto& candidate) {
        return candidate.subject_id == id;
    });
    if (member == evidence.end()) {
        std::abort();
    }
    return *member;
}

const wide_eye::game::SheepCombinedInfluenceEvidence&
evidence_with_id(const wide_eye::game::SheepCombinedInfluenceEvidenceBuffer& evidence,
                 std::uint32_t id) {
    const auto member = std::find_if(evidence.begin(), evidence.end(), [id](const auto& candidate) {
        return candidate.subject_id == id;
    });
    if (member == evidence.end()) {
        std::abort();
    }
    return *member;
}

const wide_eye::game::SheepMotionLimitEvidence&
evidence_with_id(const wide_eye::game::SheepMotionLimitEvidenceBuffer& evidence, std::uint32_t id) {
    const auto member = std::find_if(evidence.begin(), evidence.end(), [id](const auto& candidate) {
        return candidate.subject_id == id;
    });
    if (member == evidence.end()) {
        std::abort();
    }
    return *member;
}

// The unbounded sum the combined-influence bound acts on, added in the same left
// to right order the simulation adds it so an oracle can compare exactly instead
// of within a tolerance.
wide_eye::game::Vec3
summed_steering_terms(const wide_eye::game::SheepSocialEvidence& social,
                      const wide_eye::game::SheepDogPressureEvidence& dog,
                      const wide_eye::game::SheepAvoidanceEvidence& avoidance) {
    return {.x = social.separation_acceleration.x + social.attraction_acceleration.x +
                 social.alignment_acceleration.x + dog.pressure_acceleration.x +
                 dog.approach_acceleration.x + dog.facing_acceleration.x +
                 avoidance.avoidance_acceleration.x,
            .z = social.separation_acceleration.z + social.attraction_acceleration.z +
                 social.alignment_acceleration.z + dog.pressure_acceleration.z +
                 dog.approach_acceleration.z + dog.facing_acceleration.z +
                 avoidance.avoidance_acceleration.z};
}

// Applied acceleration is the summed terms after the combined-influence bound,
// so every per-term oracle compares against the sum scaled by the factor that
// sheep published rather than against the raw sum. The factor is exactly `1.0`
// wherever the bound is switched off or did not bind.
wide_eye::game::Vec3 bounded_terms(const wide_eye::game::Vec3& summed,
                                   const wide_eye::game::SheepCombinedInfluenceEvidence& evidence) {
    return {.x = summed.x * evidence.applied_scale, .z = summed.z * evidence.applied_scale};
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
    std::array<SheepContactRecord, wide_eye::game::kDefaultGameplaySheepCount>;

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

// Two whole snapshots by value, so this one lives on the heap for the same
// reason `CadenceResult` does.
using PaddockCollisionHandle = std::unique_ptr<PaddockCollisionRun>;

PaddockCollisionHandle
run_paddock_collision(const wide_eye::game::GameplayScenarioDefinition& scenario,
                      std::uint64_t ticks, std::uint64_t midpoint_tick) {
    auto result = std::make_unique<PaddockCollisionRun>();
    const SimulationHandle simulation = make_simulation(scenario);
    for (std::size_t index = 0; index < scenario.sheep_count; ++index) {
        result->contacts[index].evidence.subject_id = scenario.initial_sheep[index].id;
        result->contacts[index].minimum_x = scenario.initial_sheep[index].position.x;
        result->contacts[index].minimum_z = scenario.initial_sheep[index].position.z;
    }

    for (std::uint64_t tick = 1; tick <= ticks; ++tick) {
        simulation->fixed_update({});
        const auto& snapshot = simulation->current_snapshot();
        for (std::size_t index = 0; index < snapshot.sheep_count; ++index) {
            SheepContactRecord& record = result->contacts[index];
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
            record.prior =
                sheep_with_id(simulation->previous_snapshot().sheep, evidence.subject_id);
            record.state = snapshot.sheep[index];
            record.evidence = evidence;
        }
        if (tick == midpoint_tick) {
            result->midpoint = snapshot;
        }
    }

    result->final_snapshot = simulation->current_snapshot();
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

// What the combined-influence oracle observed, returned so the run report can
// name the numbers without keeping the fixtures alive in `main`.
struct CombinedInfluenceOracle {
    bool passed = false;
    double bound = 0.0;
    std::uint64_t drift_ticks = 0;
    wide_eye::game::SheepCombinedInfluenceEvidence over_bound{};
    wide_eye::game::SheepCombinedInfluenceEvidence over_bound_control{};
    wide_eye::game::SheepCombinedInfluenceEvidence under_bound{};
    wide_eye::game::SheepCombinedInfluenceEvidence idle_bound{};
    wide_eye::game::SheepCombinedInfluenceEvidence diagonal_bound{};
    double bounded_drift_z = 0.0;
    double unbounded_drift_z = 0.0;
    std::size_t allocations = 0;
};

CombinedInfluenceOracle run_combined_influence_oracle(
    const wide_eye::game::GameplayScenarioDefinition& stationary_fixture) {
    CombinedInfluenceOracle result;
    // Combined influence. Every social and dog term already bounds itself, but
    // nothing bounded their sum, so a sheep standing inside several overlapping
    // influences — or a nervous sheep whose temperament multiplies three dog
    // terms at once — was accelerated by an unbounded total. The rule is one
    // clamp on the summed vector: each per-term vector keeps its published value
    // and the sum alone is scaled down, so the accepted per-term evidence stays
    // exactly as inspectable as it was. The paired fixture puts two sheep over
    // the bound and two under it in the same tick, so "the bound binds" and "the
    // bound leaves an under-bound sheep alone" are observed against each other.
    constexpr double kCombinedInfluenceBound = 4.0;
    constexpr double kOverBoundSummedMagnitude = 8.0;
    constexpr double kOverBoundAppliedScale = 0.5;
    constexpr double kUnderBoundSummedMagnitude = 1.625;
    constexpr std::uint64_t kCombinedInfluenceDriftTicks = 120;
    result.bound = kCombinedInfluenceBound;
    result.drift_ticks = kCombinedInfluenceDriftTicks;
    const ScenarioHandle combined_off_scenario =
        named_scenario("sheep-combined-influence-off");
    const ScenarioHandle combined_on_scenario =
        named_scenario("sheep-combined-influence-on");
    auto combined_on_as_control = mutable_scenario_copy(combined_on_scenario);
    if (combined_off_scenario != nullptr) {
        combined_on_as_control->id = combined_off_scenario->id;
    }
    combined_on_as_control->sheep_combined_influence.enabled = false;
    if (!check(combined_off_scenario != nullptr && combined_on_scenario != nullptr &&
                   combined_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_combined_influence_off &&
                   combined_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_combined_influence_on &&
                   *combined_on_as_control == *combined_off_scenario &&
                   combined_on_scenario->sheep_combined_influence.enabled &&
                   !combined_off_scenario->sheep_combined_influence.enabled &&
                   combined_on_scenario->sheep_combined_influence.maximum_acceleration ==
                       kCombinedInfluenceBound &&
                   combined_off_scenario->version == 1 && combined_off_scenario->seed == 0 &&
                   combined_off_scenario->sheep_separation.enabled &&
                   combined_off_scenario->sheep_dog_pressure.enabled &&
                   combined_off_scenario->sheep_dog_approach.enabled &&
                   combined_off_scenario->sheep_dog_facing.enabled &&
                   combined_off_scenario->sheep_temperament.enabled &&
                   !combined_off_scenario->sheep_attraction.enabled &&
                   !combined_off_scenario->sheep_alignment.enabled &&
                   !combined_off_scenario->sheep_dog_line_of_sight.enabled,
               "paired_combined_influence_fixture_differs_only_by_the_bound_switch") ||
        // The magnitude is chosen, not arbitrary: no combination of influences
        // may accelerate a sheep harder than the strongest single influence the
        // flock already accepts on its own, which is close-range separation.
        !check(combined_on_scenario->sheep_combined_influence.maximum_acceleration ==
                       combined_on_scenario->sheep_separation.maximum_acceleration &&
                   combined_on_scenario->sheep_separation.maximum_acceleration >=
                       combined_on_scenario->sheep_dog_pressure.maximum_acceleration &&
                   combined_on_scenario->sheep_separation.maximum_acceleration >=
                       combined_on_scenario->sheep_dog_approach.maximum_acceleration &&
                   combined_on_scenario->sheep_separation.maximum_acceleration >=
                       combined_on_scenario->sheep_dog_facing.maximum_acceleration,
               "combined_bound_is_the_largest_single_accepted_term_maximum")) {
        return result;
    }

    const SimulationHandle combined_off = make_simulation(*combined_off_scenario);
    const SimulationHandle combined_on = make_simulation(*combined_on_scenario);
    const auto combined_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(combined_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& combined_initial = *combined_initial_holder;
    combined_off->fixed_update({});
    combined_on->fixed_update({});
    const auto combined_off_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(combined_off->current_snapshot());
    const wide_eye::game::GameplaySnapshot& combined_off_after_one = *combined_off_after_one_holder;
    const auto combined_on_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(combined_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& combined_on_after_one = *combined_on_after_one_holder;
    if (!check(combined_on->previous_snapshot() == combined_initial,
               "combined_influence_reads_immutable_prior_snapshot")) {
        return result;
    }

    for (const auto& on_combined :
         active(combined_on_after_one.sheep_combined_influence_evidence,
                combined_on_after_one.sheep_count)) {
        const std::uint32_t subject_id = on_combined.subject_id;
        const auto& off_combined =
            evidence_with_id(combined_off_after_one.sheep_combined_influence_evidence, subject_id);
        const auto& on_social =
            evidence_with_id(combined_on_after_one.sheep_social_evidence, subject_id);
        const auto& off_social =
            evidence_with_id(combined_off_after_one.sheep_social_evidence, subject_id);
        const auto& on_dog =
            evidence_with_id(combined_on_after_one.sheep_dog_pressure_evidence, subject_id);
        const auto& off_dog =
            evidence_with_id(combined_off_after_one.sheep_dog_pressure_evidence, subject_id);
        const auto summed = summed_steering_terms(
            on_social, on_dog,
            evidence_with_id(combined_on_after_one.sheep_avoidance_evidence, subject_id));
        const auto& prior_member = sheep_with_id(combined_initial.sheep, subject_id);
        const auto applied = [&prior_member](const wide_eye::game::SheepState& member) {
            return wide_eye::game::Vec3{.x = (member.velocity.x - prior_member.velocity.x) /
                                             wide_eye::game::GameplaySimulation::kFixedDeltaSeconds,
                                        .z =
                                            (member.velocity.z - prior_member.velocity.z) /
                                            wide_eye::game::GameplaySimulation::kFixedDeltaSeconds};
        };
        const auto on_applied = applied(sheep_with_id(combined_on_after_one.sheep, subject_id));
        if ( // The bound must not touch a single term. Every published social and
             // dog vector, and the pre-bound magnitude they sum to, must be
             // identical between the two members; only the applied result differs.
            !check(on_social == off_social && on_dog == off_dog &&
                       on_combined.summed_acceleration_magnitude ==
                           off_combined.summed_acceleration_magnitude,
                   "the_bound_leaves_every_published_per_term_vector_identical") ||
            !check(on_combined.bound_evaluated && off_combined.bound_evaluated &&
                       on_combined.applied_scale > 0.0 && on_combined.applied_scale <= 1.0 &&
                       std::abs(on_combined.summed_acceleration_magnitude -
                                std::hypot(summed.x, summed.z)) < 1.0e-12,
                   "published_pre_bound_magnitude_is_the_magnitude_of_the_summed_terms") ||
            // The defining identity, checked exactly: applied is the summed terms
            // times the published scale, in both components.
            !check(on_combined.applied_acceleration.x == summed.x * on_combined.applied_scale &&
                       on_combined.applied_acceleration.z == summed.z * on_combined.applied_scale,
                   "published_scale_times_the_summed_terms_is_the_applied_acceleration") ||
            !check(std::abs(on_applied.x - on_combined.applied_acceleration.x) < 1.0e-12 &&
                       std::abs(on_applied.z - on_combined.applied_acceleration.z) < 1.0e-12,
                   "published_applied_acceleration_is_what_integration_used") ||
            !check(std::hypot(on_combined.applied_acceleration.x,
                              on_combined.applied_acceleration.z) <=
                       kCombinedInfluenceBound + 1.0e-12,
                   "no_bounded_sheep_exceeds_the_combined_bound") ||
            // The off member is the accepted unbounded behavior: it must publish
            // exactly one and apply the raw sum bit for bit.
            !check(off_combined.applied_scale == 1.0 &&
                       off_combined.applied_acceleration.x == summed.x &&
                       off_combined.applied_acceleration.z == summed.z,
                   "the_off_member_publishes_scale_one_and_applies_the_raw_sum")) {
            return result;
        }
    }

    // Copies, not references: the restart oracle below rewinds this simulation.
    result.over_bound =
        evidence_with_id(combined_on_after_one.sheep_combined_influence_evidence, 1);
    result.over_bound_control =
        evidence_with_id(combined_off_after_one.sheep_combined_influence_evidence, 1);
    result.under_bound =
        evidence_with_id(combined_on_after_one.sheep_combined_influence_evidence, 3);
    result.idle_bound =
        evidence_with_id(combined_on_after_one.sheep_combined_influence_evidence, 4);
    result.diagonal_bound =
        evidence_with_id(combined_on_after_one.sheep_combined_influence_evidence, 5);
    const auto& over_bound_social =
        evidence_with_id(combined_on_after_one.sheep_social_evidence, 1);
    const auto& over_bound_dog =
        evidence_with_id(combined_on_after_one.sheep_dog_pressure_evidence, 1);
    const auto& diagonal_social = evidence_with_id(combined_on_after_one.sheep_social_evidence, 5);
    const auto& diagonal_dog =
        evidence_with_id(combined_on_after_one.sheep_dog_pressure_evidence, 5);
    const auto diagonal_summed =
        summed_steering_terms(diagonal_social, diagonal_dog,
                              evidence_with_id(combined_on_after_one.sheep_avoidance_evidence, 5));
    // Exact equality rather than a tolerance: the fixture is built so the
    // over-bound sheep's six terms sum to exactly twice the bound along one axis,
    // which makes the scale an exact power of two and the bounded magnitude
    // exactly the bound.
    if (!check(over_bound_dog.dog_distance == 3.0 &&
                   over_bound_dog.temperament_response_scale == 2.0 &&
                   over_bound_social.separation_acceleration.z == 1.5 &&
                   over_bound_dog.pressure_acceleration.z == 3.0 &&
                   over_bound_dog.approach_acceleration.z == 2.0 &&
                   over_bound_dog.facing_acceleration.z == 1.5,
               "over_bound_sheep_sees_four_overlapping_influences_on_one_axis") ||
        !check(result.over_bound.summed_acceleration_magnitude == kOverBoundSummedMagnitude &&
                   result.over_bound.applied_scale == kOverBoundAppliedScale &&
                   result.over_bound.applied_acceleration.z == kCombinedInfluenceBound &&
                   std::hypot(result.over_bound.applied_acceleration.x,
                              result.over_bound.applied_acceleration.z) == kCombinedInfluenceBound,
               "an_over_bound_sheep_is_accelerated_at_exactly_the_bound") ||
        // Direction preservation on the axis case: the summed and applied vectors
        // have no x component and the same positive z sign, so scaling changed
        // magnitude only.
        !check(result.over_bound.applied_acceleration.x == 0.0 &&
                   result.over_bound_control.applied_acceleration.x == 0.0 &&
                   result.over_bound_control.applied_acceleration.z == kOverBoundSummedMagnitude &&
                   result.over_bound.applied_acceleration.z > 0.0,
               "bounding_changes_magnitude_without_changing_direction") ||
        // The unbounded member really is over the bound, so the comparison is not
        // vacuous, and its sheep really moves further this tick.
        !check(result.over_bound_control.applied_scale == 1.0 &&
                   result.over_bound_control.summed_acceleration_magnitude >
                       kCombinedInfluenceBound &&
                   sheep_with_id(combined_off_after_one.sheep, 1).position.z >
                       sheep_with_id(combined_on_after_one.sheep, 1).position.z,
               "the_unbounded_control_exceeds_the_bound_and_moves_further") ||
        !check(result.under_bound.summed_acceleration_magnitude == kUnderBoundSummedMagnitude &&
                   result.under_bound.applied_scale == 1.0 &&
                   result.under_bound.applied_acceleration.z == kUnderBoundSummedMagnitude &&
                   sheep_with_id(combined_on_after_one.sheep, 3) ==
                       sheep_with_id(combined_off_after_one.sheep, 3),
               "an_under_bound_sheep_is_untouched_with_scale_exactly_one") ||
        !check(result.idle_bound.bound_evaluated &&
                   result.idle_bound.summed_acceleration_magnitude == 0.0 &&
                   result.idle_bound.applied_scale == 1.0 &&
                   result.idle_bound.applied_acceleration == wide_eye::game::Vec3{},
               "a_sheep_under_no_influence_publishes_scale_exactly_one") ||
        // The bound is not an axis artifact: the diagonal sheep is scaled to the
        // same magnitude and keeps its direction, measured as a zero cross
        // product against the unbounded sum.
        !check(result.diagonal_bound.summed_acceleration_magnitude > kCombinedInfluenceBound &&
                   result.diagonal_bound.applied_scale < 1.0 &&
                   result.diagonal_bound.applied_acceleration.x > 0.0 &&
                   result.diagonal_bound.applied_acceleration.z > 0.0 &&
                   std::abs(std::hypot(result.diagonal_bound.applied_acceleration.x,
                                       result.diagonal_bound.applied_acceleration.z) -
                            kCombinedInfluenceBound) < 1.0e-12 &&
                   std::abs(result.diagonal_bound.applied_acceleration.x * diagonal_summed.z -
                            result.diagonal_bound.applied_acceleration.z * diagonal_summed.x) <
                       1.0e-12,
               "the_bound_preserves_direction_off_axis_too")) {
        return result;
    }

    // A sum exactly equal to the bound is not over it. Raising the same
    // fixture's bound to the over-bound sheep's exact summed magnitude must
    // reproduce the unbounded control bit for bit.
    const auto combined_at_bound_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*combined_on_scenario);
    auto& combined_at_bound_scenario = *combined_at_bound_scenario_holder;
    combined_at_bound_scenario.sheep_combined_influence.maximum_acceleration =
        kOverBoundSummedMagnitude;
    const SimulationHandle combined_at_bound = make_simulation(combined_at_bound_scenario);
    combined_at_bound->fixed_update({});
    const auto& at_bound_evidence = evidence_with_id(
        combined_at_bound->current_snapshot().sheep_combined_influence_evidence, 1);
    if (!check(at_bound_evidence.summed_acceleration_magnitude == kOverBoundSummedMagnitude &&
                   at_bound_evidence.applied_scale == 1.0 &&
                   at_bound_evidence.applied_acceleration ==
                       result.over_bound_control.applied_acceleration &&
                   sheep_with_id(combined_at_bound->current_snapshot().sheep, 1) ==
                       sheep_with_id(combined_off_after_one.sheep, 1),
               "a_sum_exactly_at_the_bound_is_left_alone")) {
        return result;
    }

    // The bound is a per-tick rule, not a first-tick one: it must hold on every
    // tick of a run, and the unbounded control must actually breach it.
    const SimulationHandle combined_drift_on = make_simulation(*combined_on_scenario);
    const SimulationHandle combined_drift_off = make_simulation(*combined_off_scenario);
    bool combined_bound_holds_every_tick = true;
    bool combined_control_breaches_the_bound = false;
    bool combined_drift_is_contact_free = true;
    for (std::uint64_t tick = 0; tick < kCombinedInfluenceDriftTicks; ++tick) {
        combined_drift_on->fixed_update({});
        combined_drift_off->fixed_update({});
        for (const auto& contact : combined_drift_on->current_snapshot().sheep_collision_evidence) {
            combined_drift_is_contact_free =
                combined_drift_is_contact_free && !contact.clipped_x && !contact.clipped_z;
        }
        for (const auto& contact :
             combined_drift_off->current_snapshot().sheep_collision_evidence) {
            combined_drift_is_contact_free =
                combined_drift_is_contact_free && !contact.clipped_x && !contact.clipped_z;
        }
        for (const auto& evidence :
             combined_drift_on->current_snapshot().sheep_combined_influence_evidence) {
            combined_bound_holds_every_tick =
                combined_bound_holds_every_tick &&
                std::hypot(evidence.applied_acceleration.x, evidence.applied_acceleration.z) <=
                    kCombinedInfluenceBound + 1.0e-12;
        }
        for (const auto& evidence :
             combined_drift_off->current_snapshot().sheep_combined_influence_evidence) {
            combined_control_breaches_the_bound =
                combined_control_breaches_the_bound ||
                std::hypot(evidence.applied_acceleration.x, evidence.applied_acceleration.z) >
                    kCombinedInfluenceBound;
        }
    }
    result.bounded_drift_z =
        sheep_with_id(combined_drift_on->current_snapshot().sheep, 1).position.z;
    result.unbounded_drift_z =
        sheep_with_id(combined_drift_off->current_snapshot().sheep, 1).position.z;
    if (!check(combined_bound_holds_every_tick && combined_control_breaches_the_bound,
               "the_bound_holds_on_every_tick_while_the_control_breaches_it") ||
        // Neither run may touch the paddock, so the distance difference is pure
        // steering rather than one twin being stopped by a limit.
        !check(combined_drift_is_contact_free && result.unbounded_drift_z > result.bounded_drift_z,
               "a_bounded_sheep_is_driven_less_far_than_its_unbounded_twin")) {
        return result;
    }

    const auto reversed_combined_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*combined_on_scenario);
    auto& reversed_combined_scenario = *reversed_combined_scenario_holder;
    std::reverse(reversed_combined_scenario.initial_sheep.begin(),
                 reversed_combined_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_combined_scenario.sheep_count));
    const SimulationHandle reversed_combined = make_simulation(reversed_combined_scenario);
    reversed_combined->fixed_update({});
    for (const auto& member :
         active(combined_on_after_one.sheep, combined_on_after_one.sheep_count)) {
        if (!check(member == sheep_with_id(reversed_combined->current_snapshot().sheep, member.id),
                   "combined_influence_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(combined_on_after_one.sheep_combined_influence_evidence,
                                    member.id) ==
                       evidence_with_id(
                           reversed_combined->current_snapshot().sheep_combined_influence_evidence,
                           member.id),
                   "combined_influence_evidence_is_stable_under_reversed_storage")) {
            return result;
        }
    }

    const auto combined_state = wide_eye::game::gameplay_state_dump_json(*combined_on);
    // A fixture that sums no steering terms must publish the bound as
    // unevaluated rather than as a silent scale of one.
    const SimulationHandle combined_unevaluated = make_simulation(stationary_fixture);
    combined_unevaluated->fixed_update({});
    const auto unevaluated_state = wide_eye::game::gameplay_state_dump_json(*combined_unevaluated);
    if (!check(combined_state &&
                   combined_state.text.find("\"sheep_combined_influence_evidence\":[{"
                                            "\"subject_id\":1,\"bound_evaluated\":true,"
                                            "\"summed_acceleration_magnitude\":8,"
                                            "\"applied_scale\":0.5,\"applied_acceleration\":{"
                                            "\"x\":0,\"y\":0,\"z\":4}}") != std::string::npos &&
                   combined_state.text.find("\"applied_scale\":1,") != std::string::npos,
               "state_dump_contains_the_combined_influence_bound_evidence") ||
        !check(unevaluated_state && unevaluated_state.text.find(
                                        "\"bound_evaluated\":false,\"summed_acceleration_"
                                        "magnitude\":0,\"applied_scale\":0,") != std::string::npos,
               "a_fixture_that_sums_no_terms_publishes_an_unevaluated_bound")) {
        return result;
    }

    const SimulationHandle allocation_combined = make_simulation(*combined_on_scenario);
    const std::size_t combined_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_combined->fixed_update({});
    }
    result.allocations = g_allocation_count - combined_allocations_before;
    if (!check(result.allocations == 0, "combined_influence_fixed_updates_do_not_allocate")) {
        return result;
    }

    for (std::uint32_t tick = 0; tick < 120; ++tick) {
        combined_on->fixed_update({});
    }
    combined_on->restart();
    if (!check(combined_on->current_snapshot() == combined_initial &&
                   combined_on->previous_snapshot() == combined_initial,
               "combined_influence_restart_restores_the_paired_fixture")) {
        return result;
    }

    result.passed = true;
    return result;
}

// What the motion-limit oracle observed, returned so the run report can name the
// numbers without keeping the fixtures alive in `main`.
struct MotionLimitOracle {
    bool passed = false;
    double maximum_speed = 0.0;
    double maximum_turn_rate = 0.0;
    double turn_budget = 0.0;
    std::uint64_t reversal_ticks = 0;
    std::uint64_t reversal_completion_tick = 0;
    double reversal_heading_before_completion = 0.0;
    std::uint64_t drift_ticks = 0;
    wide_eye::game::SheepMotionLimitEvidence axis_clamp{};
    wide_eye::game::SheepMotionLimitEvidence diagonal_clamp{};
    wide_eye::game::SheepMotionLimitEvidence under_limit{};
    wide_eye::game::SheepMotionLimitEvidence stationary{};
    wide_eye::game::SheepMotionLimitEvidence reversal{};
    double unlimited_axis_speed = 0.0;
    double accumulation_maximum = 0.0;
    double unlimited_peak_speed = 0.0;
    double limited_peak_speed = 0.0;
    std::size_t allocations = 0;
};

MotionLimitOracle run_motion_limit_oracle() {
    MotionLimitOracle result;
    // Bounded speed and turning. The combined-influence bound limits how hard a
    // sheep may be accelerated; it does not limit how fast that acceleration can
    // make a sheep travel, and nothing at all decided which way a sheep faced.
    // These two limits act on the result of integration instead: the planar
    // speed is clamped with its direction preserved, and the heading is rotated
    // toward the direction of that motion by at most one turn budget per tick.
    // The paired fixture enables no steering term at all, so every number below
    // is exact arithmetic on the fixture's own initial velocities rather than a
    // tolerance around an integrated one.
    constexpr double kMaximumSpeed = 5.0;
    constexpr double kMaximumTurnRate = 3.75;
    constexpr double kOverLimitSpeed = 12.5;
    constexpr double kOverLimitScale = 0.4;
    constexpr double kUnderLimitSpeed = 2.0;
    constexpr double kStationaryHeading = 1.0;
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kTwoPi = 6.28318530717958647692;
    constexpr std::uint64_t kBudgetTicks = 50;
    constexpr std::uint64_t kMotionLimitDriftTicks = 60;
    constexpr std::uint64_t kBearingTicks = 4;
    constexpr double kAccumulationMaximumSpeed = 2.0;
    constexpr std::uint64_t kAccumulationTicks = 240;
    const double turn_budget =
        kMaximumTurnRate * wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    result.maximum_speed = kMaximumSpeed;
    result.maximum_turn_rate = kMaximumTurnRate;
    result.turn_budget = turn_budget;
    result.reversal_ticks = kBudgetTicks;
    result.drift_ticks = kMotionLimitDriftTicks;
    result.accumulation_maximum = kAccumulationMaximumSpeed;

    const ScenarioHandle limit_off_scenario =
        named_scenario("sheep-motion-limit-off");
    const ScenarioHandle limit_on_scenario = named_scenario("sheep-motion-limit-on");
    auto limit_on_as_control = mutable_scenario_copy(limit_on_scenario);
    if (limit_off_scenario != nullptr) {
        limit_on_as_control->id = limit_off_scenario->id;
    }
    limit_on_as_control->sheep_motion_limit.enabled = false;
    if (!check(limit_off_scenario != nullptr && limit_on_scenario != nullptr &&
                   limit_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_motion_limit_off &&
                   limit_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_motion_limit_on &&
                   *limit_on_as_control == *limit_off_scenario &&
                   limit_on_scenario->sheep_motion_limit.enabled &&
                   !limit_off_scenario->sheep_motion_limit.enabled &&
                   limit_on_scenario->sheep_motion_limit.maximum_speed == kMaximumSpeed &&
                   limit_on_scenario->sheep_motion_limit.maximum_turn_rate_radians_per_second ==
                       kMaximumTurnRate &&
                   limit_off_scenario->version == 1 && limit_off_scenario->seed == 0,
               "paired_motion_limit_fixture_differs_only_by_the_limit_switch") ||
        // The fixture must isolate the two limits: any enabled steering term
        // would make the observed velocities integration results rather than the
        // fixture's own exact numbers.
        !check(!limit_off_scenario->sheep_separation.enabled &&
                   !limit_off_scenario->sheep_attraction.enabled &&
                   !limit_off_scenario->sheep_alignment.enabled &&
                   !limit_off_scenario->sheep_dog_pressure.enabled &&
                   !limit_off_scenario->sheep_dog_approach.enabled &&
                   !limit_off_scenario->sheep_dog_facing.enabled &&
                   !limit_off_scenario->sheep_dog_line_of_sight.enabled &&
                   !limit_off_scenario->sheep_temperament.enabled &&
                   !limit_off_scenario->sheep_combined_influence.enabled,
               "the_motion_limit_fixture_enables_no_steering_term") ||
        // The magnitudes are chosen, not arbitrary. A sheep must outrun a walking
        // dog and never outrun a sprinting one, and it must not turn as fast as
        // the dog that is cutting across it.
        !check(kMaximumSpeed > wide_eye::game::DogController::kWalkSpeed &&
                   kMaximumSpeed < wide_eye::game::DogController::kSprintSpeed &&
                   kMaximumTurnRate < wide_eye::game::DogController::kTurnRateRadiansPerSecond,
               "sheep_limits_sit_inside_the_accepted_dog_motor_limits") ||
        // The per-tick budget is an exact binary fraction, which is what lets
        // every turn observation below be an equality instead of a tolerance.
        !check(turn_budget == 0.0625, "the_per_tick_turn_budget_is_exactly_one_sixteenth_radian")) {
        return result;
    }

    const SimulationHandle limit_off = make_simulation(*limit_off_scenario);
    const SimulationHandle limit_on = make_simulation(*limit_on_scenario);
    const auto limit_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(limit_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& limit_initial = *limit_initial_holder;
    limit_off->fixed_update({});
    limit_on->fixed_update({});
    const auto limit_off_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(limit_off->current_snapshot());
    const wide_eye::game::GameplaySnapshot& limit_off_after_one = *limit_off_after_one_holder;
    const auto limit_on_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(limit_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& limit_on_after_one = *limit_on_after_one_holder;
    if (!check(limit_on->previous_snapshot() == limit_initial,
               "motion_limits_read_the_immutable_prior_snapshot")) {
        return result;
    }

    for (const auto& on_limit :
         active(limit_on_after_one.sheep_motion_limit_evidence, limit_on_after_one.sheep_count)) {
        const std::uint32_t subject_id = on_limit.subject_id;
        const auto& off_limit =
            evidence_with_id(limit_off_after_one.sheep_motion_limit_evidence, subject_id);
        const auto& prior_member = sheep_with_id(limit_initial.sheep, subject_id);
        const auto& on_member = sheep_with_id(limit_on_after_one.sheep, subject_id);
        const double on_speed = std::hypot(on_member.velocity.x, on_member.velocity.z);
        if ( // Neither limit may rewrite a steering term. Every published social,
             // dog, and combined-influence record must be identical between the
             // two members; only the motion the sheep ended up with differs.
            !check(
                evidence_with_id(limit_on_after_one.sheep_social_evidence, subject_id) ==
                        evidence_with_id(limit_off_after_one.sheep_social_evidence, subject_id) &&
                    evidence_with_id(limit_on_after_one.sheep_dog_pressure_evidence, subject_id) ==
                        evidence_with_id(limit_off_after_one.sheep_dog_pressure_evidence,
                                         subject_id) &&
                    evidence_with_id(limit_on_after_one.sheep_combined_influence_evidence,
                                     subject_id) ==
                        evidence_with_id(limit_off_after_one.sheep_combined_influence_evidence,
                                         subject_id),
                "the_limits_leave_every_published_steering_record_identical") ||
            !check(on_limit.limit_evaluated && !off_limit.limit_evaluated &&
                       off_limit ==
                           wide_eye::game::SheepMotionLimitEvidence{.subject_id = subject_id},
                   "the_off_member_publishes_an_unevaluated_zeroed_limit_record") ||
            !check(on_limit.applied_speed_scale > 0.0 && on_limit.applied_speed_scale <= 1.0 &&
                       on_limit.applied_speed <= on_limit.integrated_speed &&
                       on_limit.applied_speed <= kMaximumSpeed,
                   "no_limited_sheep_ends_a_tick_faster_than_the_maximum") ||
            // The published applied speed is the speed integration actually left
            // the sheep with, before collision may refuse an axis.
            !check(std::abs(on_speed - on_limit.applied_speed) < 1.0e-12,
                   "published_applied_speed_is_the_speed_the_sheep_kept") ||
            // Heading may only ever move by one budget, whichever member.
            !check(std::abs(on_limit.heading_change_radians) <= turn_budget &&
                       (on_limit.motion_heading_followed ||
                        on_member.heading_radians == prior_member.heading_radians),
                   "a_heading_moves_by_at_most_one_turn_budget_and_only_when_moving")) {
            return result;
        }
    }

    // Copies, not references: the restart oracle below rewinds this simulation.
    result.axis_clamp = evidence_with_id(limit_on_after_one.sheep_motion_limit_evidence, 1);
    result.diagonal_clamp = evidence_with_id(limit_on_after_one.sheep_motion_limit_evidence, 2);
    result.under_limit = evidence_with_id(limit_on_after_one.sheep_motion_limit_evidence, 3);
    result.stationary = evidence_with_id(limit_on_after_one.sheep_motion_limit_evidence, 4);
    result.reversal = evidence_with_id(limit_on_after_one.sheep_motion_limit_evidence, 5);
    const auto& axis_on = sheep_with_id(limit_on_after_one.sheep, 1);
    const auto& axis_off = sheep_with_id(limit_off_after_one.sheep, 1);
    const auto& diagonal_on = sheep_with_id(limit_on_after_one.sheep, 2);
    const auto& diagonal_off = sheep_with_id(limit_off_after_one.sheep, 2);
    result.unlimited_axis_speed = std::hypot(axis_off.velocity.x, axis_off.velocity.z);
    // Exact equalities rather than tolerances: the fixture drives one sheep at
    // exactly two and a half times the maximum along one axis, so the clamp's
    // scale is exactly 0.4 and the clamped speed is exactly the maximum.
    if (!check(result.axis_clamp.integrated_speed == kOverLimitSpeed &&
                   result.axis_clamp.applied_speed_scale == kOverLimitScale &&
                   result.axis_clamp.applied_speed == kMaximumSpeed &&
                   axis_on.velocity == wide_eye::game::Vec3{.z = kMaximumSpeed},
               "an_over_limit_sheep_keeps_exactly_the_maximum_speed") ||
        // Direction preservation on the axis case: no x component appears and
        // the z sign is unchanged, so the clamp changed magnitude only.
        !check(axis_on.velocity.x == 0.0 && axis_off.velocity.x == 0.0 &&
                   axis_off.velocity.z == kOverLimitSpeed && axis_on.velocity.z > 0.0 &&
                   sheep_with_id(limit_off_after_one.sheep, 1).position.z >
                       sheep_with_id(limit_on_after_one.sheep, 1).position.z,
               "clamping_changes_speed_without_changing_direction") ||
        // The clamp is not an axis artifact: the 3-4-5 diagonal sheep lands on
        // exactly (3, 4) and keeps its direction, measured as a zero cross
        // product against the unclamped velocity.
        !check(result.diagonal_clamp.integrated_speed == kOverLimitSpeed &&
                   result.diagonal_clamp.applied_speed_scale == kOverLimitScale &&
                   result.diagonal_clamp.applied_speed == kMaximumSpeed &&
                   diagonal_on.velocity == wide_eye::game::Vec3{.x = 3.0, .z = 4.0} &&
                   diagonal_on.velocity.x * diagonal_off.velocity.z -
                           diagonal_on.velocity.z * diagonal_off.velocity.x ==
                       0.0,
               "the_clamp_preserves_direction_off_axis_too") ||
        // An under-limit sheep already facing its motion must be untouched: same
        // scale of exactly one, and a state bit-identical to the off member.
        !check(result.under_limit.integrated_speed == kUnderLimitSpeed &&
                   result.under_limit.applied_speed_scale == 1.0 &&
                   result.under_limit.applied_speed == kUnderLimitSpeed &&
                   result.under_limit.heading_change_radians == 0.0 &&
                   sheep_with_id(limit_on_after_one.sheep, 3) ==
                       sheep_with_id(limit_off_after_one.sheep, 3),
               "an_under_limit_sheep_is_untouched_with_scale_exactly_one") ||
        // A sheep that is not moving keeps the way it was facing rather than
        // snapping to the zero heading `atan2(0, 0)` would produce.
        !check(!result.stationary.motion_heading_followed &&
                   result.stationary.integrated_speed == 0.0 &&
                   result.stationary.applied_speed_scale == 1.0 &&
                   result.stationary.motion_heading_radians == 0.0 &&
                   result.stationary.heading_change_radians == 0.0 &&
                   sheep_with_id(limit_on_after_one.sheep, 4).heading_radians ==
                       kStationaryHeading &&
                   sheep_with_id(limit_on_after_one.sheep, 4) ==
                       sheep_with_id(limit_off_after_one.sheep, 4),
               "a_stationary_sheep_keeps_its_heading_instead_of_facing_noise") ||
        // The reversing sheep is under the speed maximum, so the turn rate is the
        // only limit acting on it, and one tick moves its heading by exactly one
        // budget toward the direction it is actually travelling.
        !check(result.reversal.applied_speed_scale == 1.0 &&
                   result.reversal.motion_heading_followed &&
                   result.reversal.motion_heading_radians == kPi &&
                   result.reversal.heading_change_radians == turn_budget &&
                   sheep_with_id(limit_on_after_one.sheep, 5).heading_radians == turn_budget &&
                   sheep_with_id(limit_off_after_one.sheep, 5).heading_radians == 0.0,
               "a_reversed_sheep_turns_by_exactly_one_budget_per_tick") ||
        // An already aligned sheep is not rotated at all, so following motion
        // costs nothing once the heading has caught up.
        !check(result.axis_clamp.motion_heading_followed &&
                   result.axis_clamp.motion_heading_radians == kPi &&
                   result.axis_clamp.heading_change_radians == 0.0 &&
                   axis_on.heading_radians == kPi,
               "a_sheep_already_facing_its_motion_is_not_rotated")) {
        return result;
    }

    // The turn rate is a per-tick budget, not a first-tick one. The reversing
    // sheep must spend exactly one budget on every tick until the remaining
    // rotation is smaller than a budget, then land exactly on its motion
    // direction and stay there.
    const SimulationHandle limit_turn = make_simulation(*limit_on_scenario);
    bool reversal_spends_exactly_one_budget = true;
    bool reversal_bearing_uses_the_prior_heading = true;
    bool reversal_bearing_differs_from_the_new_heading = true;
    for (std::uint64_t tick = 1; tick <= kBudgetTicks; ++tick) {
        limit_turn->fixed_update({});
        const auto& previous = limit_turn->previous_snapshot();
        const auto& current = limit_turn->current_snapshot();
        const auto& member = sheep_with_id(current.sheep, 5);
        const auto& evidence = evidence_with_id(current.sheep_motion_limit_evidence, 5);
        reversal_spends_exactly_one_budget =
            reversal_spends_exactly_one_budget && evidence.heading_change_radians == turn_budget &&
            member.heading_radians == static_cast<double>(tick) * turn_budget;
        if (tick > kBearingTicks) {
            continue;
        }
        // The published dog bearing is relative to the prior sheep heading. A
        // heading that now changes during the same tick must not reach back and
        // alter it.
        const auto& prior_member = sheep_with_id(previous.sheep, 5);
        const double dog_offset_x = previous.dog.position.x - prior_member.position.x;
        const double dog_offset_z = previous.dog.position.z - prior_member.position.z;
        const double dog_direction = std::atan2(dog_offset_x, -dog_offset_z);
        const double from_prior_heading =
            std::remainder(dog_direction - prior_member.heading_radians, kTwoPi);
        const double from_new_heading =
            std::remainder(dog_direction - member.heading_radians, kTwoPi);
        const auto& dog_evidence = evidence_with_id(current.sheep_dog_pressure_evidence, 5);
        reversal_bearing_uses_the_prior_heading =
            reversal_bearing_uses_the_prior_heading &&
            dog_evidence.dog_relative_bearing_radians == from_prior_heading;
        reversal_bearing_differs_from_the_new_heading =
            reversal_bearing_differs_from_the_new_heading &&
            dog_evidence.dog_relative_bearing_radians != from_new_heading;
    }
    result.reversal_heading_before_completion =
        sheep_with_id(limit_turn->current_snapshot().sheep, 5).heading_radians;
    // Copies, not references: the snapshot these describe is overwritten by the
    // very next tick this oracle runs.
    limit_turn->fixed_update({});
    const wide_eye::game::SheepState completed =
        sheep_with_id(limit_turn->current_snapshot().sheep, 5);
    const wide_eye::game::SheepMotionLimitEvidence completed_evidence =
        evidence_with_id(limit_turn->current_snapshot().sheep_motion_limit_evidence, 5);
    result.reversal_completion_tick = kBudgetTicks + 1;
    limit_turn->fixed_update({});
    const wide_eye::game::SheepState settled =
        sheep_with_id(limit_turn->current_snapshot().sheep, 5);
    const wide_eye::game::SheepMotionLimitEvidence settled_evidence =
        evidence_with_id(limit_turn->current_snapshot().sheep_motion_limit_evidence, 5);
    if (!check(reversal_spends_exactly_one_budget,
               "a_far_target_costs_exactly_one_budget_on_every_tick") ||
        !check(reversal_bearing_uses_the_prior_heading &&
                   reversal_bearing_differs_from_the_new_heading,
               "a_turning_sheep_still_publishes_its_bearing_against_the_prior_heading") ||
        // The last step of the turn is shorter than a budget, so the sheep lands
        // exactly on its motion direction rather than overshooting it.
        !check(completed.heading_radians == kPi &&
                   completed_evidence.heading_change_radians ==
                       kPi - static_cast<double>(kBudgetTicks) * turn_budget &&
                   completed_evidence.heading_change_radians < turn_budget,
               "a_target_within_one_budget_is_reached_exactly") ||
        !check(settled.heading_radians == kPi && settled_evidence.heading_change_radians == 0.0,
               "an_arrived_heading_stays_on_its_motion_direction")) {
        return result;
    }

    // The off member must reproduce today's behavior exactly: with no steering
    // term enabled and no limits, every sheep keeps its fixture velocity and its
    // fixture heading for the whole window, and neither member touches the
    // paddock, so the difference between them is the limits alone.
    const SimulationHandle limit_drift_off = make_simulation(*limit_off_scenario);
    const SimulationHandle limit_drift_on = make_simulation(*limit_on_scenario);
    bool off_member_is_unchanged = true;
    bool limit_drift_is_contact_free = true;
    bool limited_speed_holds_every_tick = true;
    for (std::uint64_t tick = 0; tick < kMotionLimitDriftTicks; ++tick) {
        limit_drift_off->fixed_update({});
        limit_drift_on->fixed_update({});
        const auto& off_snapshot = limit_drift_off->current_snapshot();
        const auto& on_snapshot = limit_drift_on->current_snapshot();
        for (const auto& member : active(off_snapshot.sheep, off_snapshot.sheep_count)) {
            const auto& fixture_member =
                sheep_with_id(limit_off_scenario->initial_sheep, member.id);
            off_member_is_unchanged = off_member_is_unchanged &&
                                      member.velocity == fixture_member.velocity &&
                                      member.heading_radians == fixture_member.heading_radians;
        }
        for (const auto& member : active(on_snapshot.sheep, on_snapshot.sheep_count)) {
            limited_speed_holds_every_tick =
                limited_speed_holds_every_tick &&
                std::hypot(member.velocity.x, member.velocity.z) <= kMaximumSpeed + 1.0e-12;
        }
        for (const auto& contact :
             active(off_snapshot.sheep_collision_evidence, off_snapshot.sheep_count)) {
            limit_drift_is_contact_free =
                limit_drift_is_contact_free && !contact.clipped_x && !contact.clipped_z;
        }
        for (const auto& contact :
             active(on_snapshot.sheep_collision_evidence, on_snapshot.sheep_count)) {
            limit_drift_is_contact_free =
                limit_drift_is_contact_free && !contact.clipped_x && !contact.clipped_z;
        }
    }
    if (!check(off_member_is_unchanged,
               "the_off_member_reproduces_unbounded_speed_and_a_fixed_heading") ||
        !check(limited_speed_holds_every_tick && limit_drift_is_contact_free,
               "the_speed_maximum_holds_on_every_tick_without_paddock_contact") ||
        // Visible as motion, not only as evidence: the limited sheep is left
        // behind by its unlimited twin over the same window.
        !check(sheep_with_id(limit_drift_off->current_snapshot().sheep, 1).position.z >
                   sheep_with_id(limit_drift_on->current_snapshot().sheep, 1).position.z,
               "a_limited_sheep_travels_less_far_than_its_unlimited_twin")) {
        return result;
    }

    // The heading floor is a real boundary rather than a comment: a sheep moving
    // below it keeps its heading, and the same sheep moving just above it turns.
    const auto below_floor_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*limit_on_scenario);
    auto& below_floor_scenario = *below_floor_scenario_holder;
    below_floor_scenario.initial_sheep[3].velocity.x =
        wide_eye::game::kSheepHeadingMotionSpeedFloor / 1000.0;
    const auto above_floor_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*limit_on_scenario);
    auto& above_floor_scenario = *above_floor_scenario_holder;
    above_floor_scenario.initial_sheep[3].velocity.x =
        wide_eye::game::kSheepHeadingMotionSpeedFloor * 1000.0;
    const SimulationHandle below_floor = make_simulation(below_floor_scenario);
    const SimulationHandle above_floor = make_simulation(above_floor_scenario);
    below_floor->fixed_update({});
    above_floor->fixed_update({});
    const auto& below_evidence =
        evidence_with_id(below_floor->current_snapshot().sheep_motion_limit_evidence, 4);
    const auto& above_evidence =
        evidence_with_id(above_floor->current_snapshot().sheep_motion_limit_evidence, 4);
    if (!check(!below_evidence.motion_heading_followed && below_evidence.applied_speed > 0.0 &&
                   sheep_with_id(below_floor->current_snapshot().sheep, 4).heading_radians ==
                       kStationaryHeading,
               "motion_below_the_heading_floor_leaves_the_heading_alone") ||
        !check(above_evidence.motion_heading_followed &&
                   above_evidence.heading_change_radians == turn_budget &&
                   sheep_with_id(above_floor->current_snapshot().sheep, 4).heading_radians ==
                       kStationaryHeading + turn_budget,
               "motion_above_the_heading_floor_turns_the_heading")) {
        return result;
    }

    // The fixture above starts a sheep over the maximum; this observes the case
    // the limit exists for, where integration accumulates the speed. The
    // accepted combined-influence arrangement is driven for four seconds with a
    // lowered maximum, so the clamp binds on a speed the steering terms produced.
    const ScenarioHandle accumulation_named = named_scenario("sheep-combined-influence-on");
    auto accumulation_scenario = std::make_unique<wide_eye::game::GameplayScenarioDefinition>(
        accumulation_named != nullptr ? *accumulation_named
                                      : wide_eye::game::GameplayScenarioDefinition{});
    const SimulationHandle accumulation_control = make_simulation(*accumulation_scenario);
    accumulation_scenario->sheep_motion_limit = {.enabled = true,
                                                 .maximum_speed = kAccumulationMaximumSpeed};
    const SimulationHandle accumulation_limited = make_simulation(*accumulation_scenario);
    bool accumulation_is_contact_free = true;
    for (std::uint64_t tick = 0; tick < kAccumulationTicks; ++tick) {
        accumulation_control->fixed_update({});
        accumulation_limited->fixed_update({});
        for (const auto& member : accumulation_control->current_snapshot().sheep) {
            result.unlimited_peak_speed = std::max(
                result.unlimited_peak_speed, std::hypot(member.velocity.x, member.velocity.z));
        }
        for (const auto& member : accumulation_limited->current_snapshot().sheep) {
            result.limited_peak_speed = std::max(result.limited_peak_speed,
                                                 std::hypot(member.velocity.x, member.velocity.z));
        }
        for (const auto& contact :
             accumulation_limited->current_snapshot().sheep_collision_evidence) {
            accumulation_is_contact_free =
                accumulation_is_contact_free && !contact.clipped_x && !contact.clipped_z;
        }
    }
    if (!check(result.unlimited_peak_speed > kAccumulationMaximumSpeed &&
                   result.limited_peak_speed <= kAccumulationMaximumSpeed + 1.0e-12 &&
                   accumulation_is_contact_free,
               "an_accumulated_speed_is_clamped_where_the_control_runs_past_the_maximum")) {
        return result;
    }

    const auto reversed_limit_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*limit_on_scenario);
    auto& reversed_limit_scenario = *reversed_limit_scenario_holder;
    std::reverse(reversed_limit_scenario.initial_sheep.begin(),
                 reversed_limit_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_limit_scenario.sheep_count));
    const SimulationHandle reversed_limit = make_simulation(reversed_limit_scenario);
    reversed_limit->fixed_update({});
    for (const auto& member : active(limit_on_after_one.sheep, limit_on_after_one.sheep_count)) {
        if (!check(member == sheep_with_id(reversed_limit->current_snapshot().sheep, member.id),
                   "motion_limit_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(limit_on_after_one.sheep_motion_limit_evidence, member.id) ==
                    evidence_with_id(reversed_limit->current_snapshot().sheep_motion_limit_evidence,
                                     member.id),
                "motion_limit_evidence_is_stable_under_reversed_storage")) {
            return result;
        }
    }

    const auto limit_state = wide_eye::game::gameplay_state_dump_json(*limit_on);
    const auto control_state = wide_eye::game::gameplay_state_dump_json(*limit_off);
    if (!check(limit_state &&
                   limit_state.text.find(
                       "\"sheep_motion_limit_evidence\":[{\"subject_id\":1,"
                       "\"limit_evaluated\":true,\"integrated_speed\":12.5,"
                       "\"applied_speed_scale\":0.40000000000000002,\"applied_speed\":5,"
                       "\"motion_heading_followed\":true,\"motion_heading_radians\":"
                       "3.1415926535897931,\"heading_change_radians\":0}") != std::string::npos,
               "state_dump_contains_the_motion_limit_evidence") ||
        !check(control_state && control_state.text.find(
                                    "\"limit_evaluated\":false,\"integrated_speed\":0,"
                                    "\"applied_speed_scale\":0,\"applied_speed\":0,"
                                    "\"motion_heading_followed\":false,") != std::string::npos,
               "a_fixture_without_the_limits_publishes_an_unevaluated_record")) {
        return result;
    }

    const SimulationHandle allocation_limit = make_simulation(*limit_on_scenario);
    const std::size_t limit_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_limit->fixed_update({});
    }
    result.allocations = g_allocation_count - limit_allocations_before;
    if (!check(result.allocations == 0, "motion_limit_fixed_updates_do_not_allocate")) {
        return result;
    }

    for (std::uint32_t tick = 0; tick < 120; ++tick) {
        limit_on->fixed_update({});
    }
    limit_on->restart();
    if (!check(limit_on->current_snapshot() == limit_initial &&
                   limit_on->previous_snapshot() == limit_initial,
               "motion_limit_restart_restores_the_paired_fixture")) {
        return result;
    }

    result.passed = true;
    return result;
}

// What the avoidance oracle observed, returned so the run report can name the
// numbers without keeping the fixtures alive in `main`.
struct AvoidanceOracle {
    bool passed = false;
    double look_ahead = 0.0;
    double maximum_acceleration = 0.0;
    double probe_distance = 0.0;
    std::uint64_t contact_ticks = 0;
    wide_eye::game::SheepAvoidanceEvidence wall_head_on{};
    wide_eye::game::SheepAvoidanceEvidence wall_near_end{};
    wide_eye::game::SheepAvoidanceEvidence closed_gate{};
    wide_eye::game::SheepAvoidanceEvidence drop{};
    wide_eye::game::SheepAvoidanceEvidence parallel{};
    std::uint32_t on_contacts = 0;
    std::uint32_t off_contacts = 0;
    std::uint64_t off_first_wall_contact_tick = 0;
    std::uint64_t off_first_drop_contact_tick = 0;
    double on_closest_wall_gap = 0.0;
    double on_deflected_x = 0.0;
    double off_deflected_x = 0.0;
    double on_drop_rest_x = 0.0;
    double off_drop_rest_x = 0.0;
    double overwhelmed_maximum = 0.0;
    std::uint64_t overwhelmed_contact_tick = 0;
    double overwhelmed_rest_z = 0.0;
    double bounded_maximum = 0.0;
    wide_eye::game::SheepCombinedInfluenceEvidence bounded_drop{};
    std::size_t allocations = 0;
};

AvoidanceOracle run_avoidance_oracle() {
    AvoidanceOracle result;
    // Obstacle and drop avoidance. Every accepted rule so far decides how hard a
    // sheep is pushed and how fast it may end up moving; none of them looks at
    // where the sheep is going. A sheep driven at a wall therefore ran into it
    // and was stopped by the hard collision authority, which is a boundary doing
    // a steering job. This term probes ahead along the sheep's own direction of
    // travel and publishes one more acceleration vector, summed and bounded with
    // the rest. It deliberately replaces nothing: `resolve_sheep_against_paddock`
    // is still the last positional authority, and the oracle below proves both
    // that avoidance makes the clip unnecessary and that the clip still fires
    // when avoidance is switched off or overwhelmed.
    //
    // The paired fixture enables no other steering term, so every first-tick
    // number is exact arithmetic on the fixture's own velocities.
    constexpr double kLookAhead = 6.25;
    constexpr double kMaximumAcceleration = 4.0;
    constexpr double kProbeDistance = 3.125;
    constexpr double kProbeMagnitude = 2.0;
    constexpr double kDropDistance = 4.0;
    constexpr double kDropMagnitude = kMaximumAcceleration * (1.0 - kDropDistance / kLookAhead);
    constexpr double kParallelWallGap = 3.5;
    constexpr double kOutOfReachClearance = 6.5;
    constexpr std::uint64_t kAvoidanceContactTicks = 240;
    constexpr double kOverwhelmedMaximum = 0.25;
    constexpr double kBoundedMaximum = 0.72;
    constexpr double kWallFaceZ = 16.5;
    constexpr double kSheepSpeed = 3.0;
    result.look_ahead = kLookAhead;
    result.maximum_acceleration = kMaximumAcceleration;
    result.probe_distance = kProbeDistance;
    result.contact_ticks = kAvoidanceContactTicks;
    result.overwhelmed_maximum = kOverwhelmedMaximum;
    result.bounded_maximum = kBoundedMaximum;

    const ScenarioHandle avoid_off_scenario = named_scenario("sheep-avoidance-off");
    const ScenarioHandle avoid_on_scenario = named_scenario("sheep-avoidance-on");
    auto avoid_on_as_control = mutable_scenario_copy(avoid_on_scenario);
    if (avoid_off_scenario != nullptr) {
        avoid_on_as_control->id = avoid_off_scenario->id;
    }
    avoid_on_as_control->sheep_avoidance.enabled = false;
    if (!check(
            avoid_off_scenario != nullptr && avoid_on_scenario != nullptr &&
                avoid_off_scenario->id == wide_eye::game::GameplayScenarioId::sheep_avoidance_off &&
                avoid_on_scenario->id == wide_eye::game::GameplayScenarioId::sheep_avoidance_on &&
                *avoid_on_as_control == *avoid_off_scenario &&
                avoid_on_scenario->sheep_avoidance.enabled &&
                !avoid_off_scenario->sheep_avoidance.enabled &&
                avoid_on_scenario->sheep_avoidance.look_ahead_distance == kLookAhead &&
                avoid_on_scenario->sheep_avoidance.maximum_acceleration == kMaximumAcceleration &&
                avoid_off_scenario->version == 1 && avoid_off_scenario->seed == 0,
            "paired_avoidance_fixture_differs_only_by_the_avoidance_switch") ||
        // The fixture must isolate avoidance: any other enabled term would make
        // the observed accelerations a sum rather than this term's own vector.
        !check(!avoid_off_scenario->sheep_separation.enabled &&
                   !avoid_off_scenario->sheep_attraction.enabled &&
                   !avoid_off_scenario->sheep_alignment.enabled &&
                   !avoid_off_scenario->sheep_dog_pressure.enabled &&
                   !avoid_off_scenario->sheep_dog_approach.enabled &&
                   !avoid_off_scenario->sheep_dog_facing.enabled &&
                   !avoid_off_scenario->sheep_dog_line_of_sight.enabled &&
                   !avoid_off_scenario->sheep_temperament.enabled &&
                   !avoid_off_scenario->sheep_combined_influence.enabled &&
                   !avoid_off_scenario->sheep_motion_limit.enabled,
               "the_avoidance_fixture_enables_no_other_steering_term") ||
        // The magnitudes are derived, not picked. The maximum is the strongest
        // single influence the flock already accepts, and the look-ahead is
        // exactly the room that maximum needs, under the linear falloff, to
        // bring a sheep travelling at the accepted maximum speed to rest at the
        // face it is heading for.
        !check(kMaximumAcceleration ==
                       wide_eye::game::SheepSeparationConfiguration{}.maximum_acceleration &&
                   kMaximumAcceleration ==
                       wide_eye::game::SheepCombinedInfluenceConfiguration{}.maximum_acceleration &&
                   kLookAhead == wide_eye::game::SheepMotionLimitConfiguration{}.maximum_speed *
                                     wide_eye::game::SheepMotionLimitConfiguration{}.maximum_speed /
                                     kMaximumAcceleration,
               "the_avoidance_magnitudes_are_derived_from_accepted_values")) {
        return result;
    }

    const SimulationHandle avoid_off = make_simulation(*avoid_off_scenario);
    const SimulationHandle avoid_on = make_simulation(*avoid_on_scenario);
    const auto avoid_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(avoid_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& avoid_initial = *avoid_initial_holder;
    avoid_off->fixed_update({});
    avoid_on->fixed_update({});
    const auto avoid_off_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(avoid_off->current_snapshot());
    const wide_eye::game::GameplaySnapshot& avoid_off_after_one = *avoid_off_after_one_holder;
    const auto avoid_on_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(avoid_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& avoid_on_after_one = *avoid_on_after_one_holder;
    if (!check(avoid_on->previous_snapshot() == avoid_initial,
               "avoidance_reads_the_immutable_prior_snapshot")) {
        return result;
    }

    for (const auto& on_avoidance :
         active(avoid_on_after_one.sheep_avoidance_evidence, avoid_on_after_one.sheep_count)) {
        const std::uint32_t subject_id = on_avoidance.subject_id;
        const auto& off_avoidance =
            evidence_with_id(avoid_off_after_one.sheep_avoidance_evidence, subject_id);
        const auto& on_combined =
            evidence_with_id(avoid_on_after_one.sheep_combined_influence_evidence, subject_id);
        const auto& prior_member = sheep_with_id(avoid_initial.sheep, subject_id);
        const auto& on_member = sheep_with_id(avoid_on_after_one.sheep, subject_id);
        const wide_eye::game::Vec3 applied{
            .x = (on_member.velocity.x - prior_member.velocity.x) /
                 wide_eye::game::GameplaySimulation::kFixedDeltaSeconds,
            .z = (on_member.velocity.z - prior_member.velocity.z) /
                 wide_eye::game::GameplaySimulation::kFixedDeltaSeconds};
        if ( // Avoidance may not rewrite another term. Every published social,
             // dog, and motion-limit record must be identical between the two
             // members; only this term's own vector differs.
            !check(
                evidence_with_id(avoid_on_after_one.sheep_social_evidence, subject_id) ==
                        evidence_with_id(avoid_off_after_one.sheep_social_evidence, subject_id) &&
                    evidence_with_id(avoid_on_after_one.sheep_dog_pressure_evidence, subject_id) ==
                        evidence_with_id(avoid_off_after_one.sheep_dog_pressure_evidence,
                                         subject_id) &&
                    evidence_with_id(avoid_on_after_one.sheep_motion_limit_evidence, subject_id) ==
                        evidence_with_id(avoid_off_after_one.sheep_motion_limit_evidence,
                                         subject_id),
                "avoidance_leaves_every_other_published_record_identical") ||
            !check(on_avoidance.avoidance_evaluated && !off_avoidance.avoidance_evaluated &&
                       off_avoidance ==
                           wide_eye::game::SheepAvoidanceEvidence{.subject_id = subject_id},
                   "the_off_member_publishes_an_unevaluated_zeroed_avoidance_record") ||
            // The term holds itself to its own named maximum, exactly as every
            // other term does.
            !check(std::hypot(on_avoidance.avoidance_acceleration.x,
                              on_avoidance.avoidance_acceleration.z) <=
                       kMaximumAcceleration + 1.0e-12,
                   "no_avoidance_vector_exceeds_the_terms_own_maximum") ||
            // Avoidance is a term in the sum, not a private correction: with it
            // as the only enabled term the summed and applied acceleration is
            // exactly the vector it published.
            !check(on_combined.applied_acceleration == on_avoidance.avoidance_acceleration &&
                       on_combined.applied_scale == 1.0 &&
                       std::abs(on_combined.summed_acceleration_magnitude -
                                std::hypot(on_avoidance.avoidance_acceleration.x,
                                           on_avoidance.avoidance_acceleration.z)) < 1.0e-12,
                   "the_published_avoidance_vector_is_what_the_sum_carried") ||
            !check(std::abs(applied.x - on_avoidance.avoidance_acceleration.x) < 1.0e-12 &&
                       std::abs(applied.z - on_avoidance.avoidance_acceleration.z) < 1.0e-12,
                   "the_published_avoidance_vector_is_what_integration_used") ||
            // A named distance needs a named shape, and a push needs one of the
            // two things this term can see.
            !check((on_avoidance.obstacle != wide_eye::game::PaddockObstacle::none ||
                    on_avoidance.obstacle_distance == 0.0) &&
                       (on_avoidance.obstacle != wide_eye::game::PaddockObstacle::none ||
                        on_avoidance.drop_ahead ||
                        on_avoidance.avoidance_acceleration == wide_eye::game::Vec3{}),
                   "an_avoidance_push_always_names_the_thing_it_avoided")) {
            return result;
        }
    }

    // Copies, not references: the restart oracle below rewinds this simulation.
    result.wall_head_on = evidence_with_id(avoid_on_after_one.sheep_avoidance_evidence, 1);
    result.wall_near_end = evidence_with_id(avoid_on_after_one.sheep_avoidance_evidence, 2);
    result.closed_gate = evidence_with_id(avoid_on_after_one.sheep_avoidance_evidence, 3);
    result.drop = evidence_with_id(avoid_on_after_one.sheep_avoidance_evidence, 4);
    result.parallel = evidence_with_id(avoid_on_after_one.sheep_avoidance_evidence, 5);
    // Exact equalities rather than tolerances: every driven sheep starts exactly
    // half a look-ahead from the face it is heading at, so the linear falloff is
    // exactly one half and the magnitude is exactly half the maximum.
    if (!check(result.wall_head_on.obstacle == wide_eye::game::PaddockObstacle::left_wall &&
                   result.wall_head_on.obstacle_distance == kProbeDistance &&
                   !result.wall_head_on.drop_ahead &&
                   result.wall_head_on.avoidance_acceleration ==
                       wide_eye::game::Vec3{.z = kProbeMagnitude},
               "a_sheep_heading_at_a_wall_is_pushed_exactly_away_from_it") ||
        // The pure push is not laziness: this sheep's nearer free edge is
        // further away than it can see, so the geometry named no way round.
        !check(kOutOfReachClearance > kLookAhead &&
                   result.wall_head_on.avoidance_acceleration.x == 0.0,
               "a_way_round_beyond_the_look_ahead_is_not_taken") ||
        // The steer-aside case: the same wall, the same magnitude, but with a
        // reachable free edge the push is turned exactly halfway toward it, so
        // the two components are exactly equal and both positive.
        !check(result.wall_near_end.obstacle == wide_eye::game::PaddockObstacle::left_wall &&
                   result.wall_near_end.obstacle_distance == kProbeDistance &&
                   result.wall_near_end.avoidance_acceleration.x ==
                       result.wall_near_end.avoidance_acceleration.z &&
                   result.wall_near_end.avoidance_acceleration.x > 0.0 &&
                   std::abs(std::hypot(result.wall_near_end.avoidance_acceleration.x,
                                       result.wall_near_end.avoidance_acceleration.z) -
                            kProbeMagnitude) < 1.0e-12,
               "a_sheep_with_a_reachable_way_round_is_steered_aside_as_well_as_slowed") ||
        // A closed gate is avoided by name, and this sheep sits exactly between
        // the gate's two free edges, so neither is nearer and no side is
        // invented.
        !check(result.closed_gate.obstacle == wide_eye::game::PaddockObstacle::gate &&
                   result.closed_gate.obstacle_distance == kProbeDistance &&
                   result.closed_gate.avoidance_acceleration ==
                       wide_eye::game::Vec3{.z = kProbeMagnitude},
               "a_closed_gate_is_avoided_by_name_and_an_exact_tie_invents_no_side") ||
        // The drop is the ground boundary, not an obstacle: no shape is named,
        // and its exact distance gives the same falloff as an obstacle.
        !check(result.drop.drop_ahead &&
                   result.drop.obstacle == wide_eye::game::PaddockObstacle::none &&
                   result.drop.obstacle_distance == 0.0 &&
                   result.drop.avoidance_acceleration == wide_eye::game::Vec3{.x = kDropMagnitude},
               "a_sheep_heading_at_the_drop_gets_the_graded_inward_response") ||
        // Direction, not proximity: this sheep is closer to the wall than the
        // look-ahead but running parallel to it, so it is untouched and stays
        // bit-identical to the control.
        !check(result.parallel.avoidance_evaluated &&
                   result.parallel.obstacle == wide_eye::game::PaddockObstacle::none &&
                   !result.parallel.drop_ahead &&
                   result.parallel.avoidance_acceleration == wide_eye::game::Vec3{} &&
                   kParallelWallGap < kLookAhead &&
                   sheep_with_id(avoid_on_after_one.sheep, 5) ==
                       sheep_with_id(avoid_off_after_one.sheep, 5),
               "a_sheep_running_parallel_to_a_wall_is_left_exactly_alone")) {
        return result;
    }

    // A sheep heading away from the same wall it was heading at is untouched
    // too, so the term reads travel direction rather than position.
    const auto away_on_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_on_scenario);
    auto& away_on_scenario = *away_on_scenario_holder;
    away_on_scenario.initial_sheep[0].velocity.z = kSheepSpeed;
    const auto away_off_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_off_scenario);
    auto& away_off_scenario = *away_off_scenario_holder;
    away_off_scenario.initial_sheep[0].velocity.z = kSheepSpeed;
    const SimulationHandle away_on = make_simulation(away_on_scenario);
    const SimulationHandle away_off = make_simulation(away_off_scenario);
    away_on->fixed_update({});
    away_off->fixed_update({});
    const auto& away_evidence =
        evidence_with_id(away_on->current_snapshot().sheep_avoidance_evidence, 1);
    // A sheep with no measurable motion has no direction to probe, so the term
    // does not run for it at all rather than steering away from rounding noise.
    const auto still_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_on_scenario);
    auto& still_scenario = *still_scenario_holder;
    still_scenario.initial_sheep[0].velocity = {};
    const SimulationHandle still = make_simulation(still_scenario);
    still->fixed_update({});
    const auto& still_evidence =
        evidence_with_id(still->current_snapshot().sheep_avoidance_evidence, 1);
    if (!check(away_evidence.avoidance_evaluated &&
                   away_evidence.obstacle == wide_eye::game::PaddockObstacle::none &&
                   away_evidence.avoidance_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(away_on->current_snapshot().sheep, 1) ==
                       sheep_with_id(away_off->current_snapshot().sheep, 1),
               "a_sheep_heading_away_from_a_wall_is_left_exactly_alone") ||
        !check(!still_evidence.avoidance_evaluated &&
                   still_evidence == wide_eye::game::SheepAvoidanceEvidence{.subject_id = 1},
               "a_sheep_with_no_direction_of_travel_publishes_an_unevaluated_record")) {
        return result;
    }

    // The look-ahead boundary is continuous rather than a step: a shape exactly
    // at the look-ahead distance is named with a vector of exactly zero, and one
    // just beyond it is not seen at all.
    const auto at_boundary_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_on_scenario);
    auto& at_boundary_scenario = *at_boundary_scenario_holder;
    at_boundary_scenario.initial_sheep[0].position.z = kWallFaceZ + kLookAhead;
    const auto past_boundary_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_on_scenario);
    auto& past_boundary_scenario = *past_boundary_scenario_holder;
    past_boundary_scenario.initial_sheep[0].position.z = kWallFaceZ + kLookAhead + 0.05;
    const SimulationHandle at_boundary = make_simulation(at_boundary_scenario);
    const SimulationHandle past_boundary = make_simulation(past_boundary_scenario);
    at_boundary->fixed_update({});
    past_boundary->fixed_update({});
    const auto& at_boundary_evidence =
        evidence_with_id(at_boundary->current_snapshot().sheep_avoidance_evidence, 1);
    const auto& past_boundary_evidence =
        evidence_with_id(past_boundary->current_snapshot().sheep_avoidance_evidence, 1);
    if (!check(at_boundary_evidence.obstacle == wide_eye::game::PaddockObstacle::left_wall &&
                   at_boundary_evidence.obstacle_distance == kLookAhead &&
                   at_boundary_evidence.avoidance_acceleration == wide_eye::game::Vec3{},
               "a_shape_exactly_at_the_look_ahead_is_named_with_a_zero_push") ||
        !check(past_boundary_evidence.obstacle == wide_eye::game::PaddockObstacle::none &&
                   past_boundary_evidence.avoidance_acceleration == wide_eye::game::Vec3{},
               "a_shape_beyond_the_look_ahead_is_not_seen")) {
        return result;
    }

    // Avoidance is bounded with the other terms rather than escaping the bound.
    // The drop sheep's graded push is exactly known, so a combined bound at half
    // that must publish a scale of exactly one half and halve the applied vector
    // while leaving this term's published vector alone.
    const auto bounded_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_on_scenario);
    auto& bounded_scenario = *bounded_scenario_holder;
    bounded_scenario.sheep_combined_influence = {.enabled = true,
                                                 .maximum_acceleration = kBoundedMaximum};
    const SimulationHandle bounded = make_simulation(bounded_scenario);
    bounded->fixed_update({});
    result.bounded_drop =
        evidence_with_id(bounded->current_snapshot().sheep_combined_influence_evidence, 4);
    const auto& bounded_avoidance =
        evidence_with_id(bounded->current_snapshot().sheep_avoidance_evidence, 4);
    if (!check(bounded_avoidance == result.drop &&
                   result.bounded_drop.summed_acceleration_magnitude == kDropMagnitude &&
                   result.bounded_drop.applied_scale == kBoundedMaximum / kDropMagnitude &&
                   result.bounded_drop.applied_acceleration ==
                       wide_eye::game::Vec3{.x = kBoundedMaximum},
               "the_combined_bound_scales_avoidance_exactly_as_it_scales_any_term")) {
        return result;
    }

    // The invariant this term exists for: a sheep driven at a wall or at the
    // drop must stop needing the hard clip, while the control must show that the
    // clip is exactly what it needed before. Neither claim is worth anything
    // without the other.
    const SimulationHandle contact_on = make_simulation(*avoid_on_scenario);
    const SimulationHandle contact_off = make_simulation(*avoid_off_scenario);
    result.on_closest_wall_gap = std::numeric_limits<double>::infinity();
    bool off_member_holds_its_fixture_velocity = true;
    std::array<bool, wide_eye::game::kDefaultGameplaySheepCount> off_contacted{};
    for (std::uint64_t tick = 1; tick <= kAvoidanceContactTicks; ++tick) {
        contact_on->fixed_update({});
        contact_off->fixed_update({});
        const auto& on_snapshot = contact_on->current_snapshot();
        const auto& off_snapshot = contact_off->current_snapshot();
        for (std::size_t index = 0; index < on_snapshot.sheep_count; ++index) {
            const auto& on_contact = on_snapshot.sheep_collision_evidence[index];
            const auto& off_contact = off_snapshot.sheep_collision_evidence[index];
            if (on_contact.clipped_x || on_contact.clipped_z) {
                ++result.on_contacts;
            }
            if (off_contact.clipped_x || off_contact.clipped_z) {
                ++result.off_contacts;
                if (!off_contacted[index]) {
                    off_contacted[index] = true;
                    if (off_contact.obstacle != wide_eye::game::PaddockObstacle::none &&
                        result.off_first_wall_contact_tick == 0) {
                        result.off_first_wall_contact_tick = tick;
                    }
                    if (off_contact.obstacle == wide_eye::game::PaddockObstacle::none &&
                        result.off_first_drop_contact_tick == 0) {
                        result.off_first_drop_contact_tick = tick;
                    }
                }
            }
            // Until the paddock refuses it, an off-member sheep must still be
            // travelling at exactly the velocity its fixture gave it: that is
            // what "the control reproduces today's behavior" means here.
            if (!off_contacted[index]) {
                off_member_holds_its_fixture_velocity =
                    off_member_holds_its_fixture_velocity &&
                    off_snapshot.sheep[index].velocity ==
                        avoid_off_scenario->initial_sheep[index].velocity;
            }
        }
        for (std::uint32_t id = 1; id <= 3; ++id) {
            result.on_closest_wall_gap =
                std::min(result.on_closest_wall_gap,
                         sheep_with_id(on_snapshot.sheep, id).position.z - kWallFaceZ);
        }
    }
    result.on_deflected_x = sheep_with_id(contact_on->current_snapshot().sheep, 2).position.x;
    result.off_deflected_x = sheep_with_id(contact_off->current_snapshot().sheep, 2).position.x;
    result.on_drop_rest_x = sheep_with_id(contact_on->current_snapshot().sheep, 4).position.x;
    result.off_drop_rest_x = sheep_with_id(contact_off->current_snapshot().sheep, 4).position.x;
    if (!check(result.on_contacts == 0 && result.on_closest_wall_gap > 0.0,
               "no_avoiding_sheep_ever_needs_the_hard_clip") ||
        !check(result.off_contacts == 4 && result.off_first_wall_contact_tick != 0 &&
                   result.off_first_drop_contact_tick != 0,
               "every_driven_control_sheep_needed_the_hard_clip") ||
        !check(off_member_holds_its_fixture_velocity,
               "the_off_member_reproduces_straight_line_travel_until_the_paddock_refuses_it") ||
        // Visible as motion, not only as evidence: the steered sheep ends the
        // window displaced along the wall while its control is pinned against
        // it, and the drop sheep rests clear of the bound its control is held
        // at.
        !check(result.on_deflected_x > result.off_deflected_x &&
                   result.off_deflected_x == avoid_off_scenario->initial_sheep[1].position.x &&
                   result.on_drop_rest_x > result.off_drop_rest_x,
               "an_avoiding_sheep_is_visibly_steered_where_its_control_is_pinned")) {
        return result;
    }

    // Avoidance must never be able to hide a collision failure. Weakened until
    // it cannot stop the same sheep, the term still publishes its push and the
    // hard clip still refuses the displacement and still names the wall.
    const auto overwhelmed_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_on_scenario);
    auto& overwhelmed_scenario = *overwhelmed_scenario_holder;
    overwhelmed_scenario.sheep_avoidance.maximum_acceleration = kOverwhelmedMaximum;
    const SimulationHandle overwhelmed = make_simulation(overwhelmed_scenario);
    bool overwhelmed_named_the_wall = false;
    // The avoidance record as it stood on the tick the clip fired, so the oracle
    // can say the term was live and still lost rather than that it had given up.
    // A sheep the paddock has already pinned has no velocity left, so its later
    // records are correctly unevaluated.
    wide_eye::game::SheepAvoidanceEvidence overwhelmed_push{};
    for (std::uint64_t tick = 1; tick <= kAvoidanceContactTicks; ++tick) {
        overwhelmed->fixed_update({});
        const auto& contact = overwhelmed->current_snapshot().sheep_collision_evidence[0];
        if ((contact.clipped_x || contact.clipped_z) && result.overwhelmed_contact_tick == 0) {
            result.overwhelmed_contact_tick = tick;
            overwhelmed_named_the_wall =
                contact.obstacle == wide_eye::game::PaddockObstacle::left_wall;
            overwhelmed_push =
                evidence_with_id(overwhelmed->current_snapshot().sheep_avoidance_evidence, 1);
        }
    }
    result.overwhelmed_rest_z = sheep_with_id(overwhelmed->current_snapshot().sheep, 1).position.z;
    if (!check(result.overwhelmed_contact_tick != 0 && overwhelmed_named_the_wall &&
                   result.overwhelmed_rest_z == kWallFaceZ &&
                   overwhelmed_push.avoidance_evaluated &&
                   overwhelmed_push.obstacle == wide_eye::game::PaddockObstacle::left_wall &&
                   overwhelmed_push.avoidance_acceleration.z > 0.0,
               "an_overwhelmed_avoidance_term_still_leaves_the_hard_clip_in_charge") ||
        // The clip fires later than it did without avoidance, so the weakened
        // term is doing something rather than nothing.
        !check(result.overwhelmed_contact_tick > result.off_first_wall_contact_tick,
               "even_a_weak_avoidance_term_delays_the_contact_it_cannot_prevent")) {
        return result;
    }

    const auto reversed_avoidance_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_on_scenario);
    auto& reversed_avoidance_scenario = *reversed_avoidance_scenario_holder;
    std::reverse(reversed_avoidance_scenario.initial_sheep.begin(),
                 reversed_avoidance_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_avoidance_scenario.sheep_count));
    const SimulationHandle reversed_avoidance = make_simulation(reversed_avoidance_scenario);
    reversed_avoidance->fixed_update({});
    for (const auto& member : active(avoid_on_after_one.sheep, avoid_on_after_one.sheep_count)) {
        if (!check(member == sheep_with_id(reversed_avoidance->current_snapshot().sheep, member.id),
                   "avoidance_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(avoid_on_after_one.sheep_avoidance_evidence, member.id) ==
                    evidence_with_id(
                        reversed_avoidance->current_snapshot().sheep_avoidance_evidence, member.id),
                "avoidance_evidence_is_stable_under_reversed_storage")) {
            return result;
        }
    }

    const auto avoidance_state = wide_eye::game::gameplay_state_dump_json(*avoid_on);
    const auto control_state = wide_eye::game::gameplay_state_dump_json(*avoid_off);
    if (!check(avoidance_state &&
                   avoidance_state.text.find(
                       "\"sheep_avoidance_evidence\":[{\"subject_id\":1,"
                       "\"avoidance_evaluated\":true,\"obstacle_ahead\":\"left_wall\","
                       "\"obstacle_distance\":3.125,\"drop_ahead\":false,"
                       "\"avoidance_acceleration\":{\"x\":0,\"y\":0,\"z\":2}}") !=
                       std::string::npos &&
                   avoidance_state.text.find("\"obstacle_ahead\":\"gate\"") != std::string::npos &&
                   avoidance_state.text.find("\"drop_ahead\":true,\"avoidance_acceleration\":{"
                                             "\"x\":") != std::string::npos,
               "state_dump_contains_the_avoidance_evidence") ||
        !check(control_state &&
                   control_state.text.find(
                       "\"avoidance_evaluated\":false,\"obstacle_ahead\":\"none\","
                       "\"obstacle_distance\":0,\"drop_ahead\":false,") != std::string::npos,
               "a_fixture_without_avoidance_publishes_an_unevaluated_record")) {
        return result;
    }

    const SimulationHandle allocation_avoidance = make_simulation(*avoid_on_scenario);
    const std::size_t avoidance_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_avoidance->fixed_update({});
    }
    result.allocations = g_allocation_count - avoidance_allocations_before;
    if (!check(result.allocations == 0, "avoidance_fixed_updates_do_not_allocate")) {
        return result;
    }

    for (std::uint32_t tick = 0; tick < 120; ++tick) {
        avoid_on->fixed_update({});
    }
    avoid_on->restart();
    if (!check(avoid_on->current_snapshot() == avoid_initial &&
                   avoid_on->previous_snapshot() == avoid_initial,
               "avoidance_restart_restores_the_paired_fixture")) {
        return result;
    }

    result.passed = true;
    return result;
}

// The names the state dump already uses, so the run report can say which shape a
// contact-face answer named without exporting a formatting helper from `game`.
std::string_view obstacle_name(wide_eye::game::PaddockObstacle obstacle) {
    switch (obstacle) {
    case wide_eye::game::PaddockObstacle::none:
        return "none";
    case wide_eye::game::PaddockObstacle::left_wall:
        return "left_wall";
    case wide_eye::game::PaddockObstacle::right_wall:
        return "right_wall";
    case wide_eye::game::PaddockObstacle::gate:
        return "gate";
    }
    return "unknown";
}

// What the contact-face regression observed, returned so the run report can name
// the numbers without keeping the fixtures alive in `main`.
struct AvoidanceContactOracle {
    bool passed = false;
    std::uint64_t ticks = 0;
    std::uint64_t clip_ticks = 0;
    std::uint64_t parallel_flaps = 0;
    std::uint64_t turning_flaps = 0;
    std::uint64_t grazing_flaps = 0;
    std::uint64_t grazing_flap_allowance = 0;
    double contact_line_z = 0.0;
    double minimum_published_distance = 0.0;
    double into_face_magnitude = 0.0;
    wide_eye::game::SheepAvoidanceEvidence parallel_contact{};
    wide_eye::game::SheepAvoidanceEvidence just_clear{};
    wide_eye::game::SheepAvoidanceEvidence into_face{};
    wide_eye::game::SheepAvoidanceEvidence drop_at_bound{};
    wide_eye::game::SheepAvoidanceEvidence drop_past_bound{};
};

AvoidanceContactOracle run_avoidance_contact_oracle() {
    AvoidanceContactOracle result;
    // QA-003. The hard clip parks a sheep at a wall face plus one body radius
    // and then lets it slide along that face, because the overlap test it uses
    // for the axis a body is *not* moving along is strict. The look-ahead sweep
    // used to disagree: it called the same body a contact at distance zero, and
    // named as "the face it meets" whichever slab the body had entered last —
    // for a body travelling along a face, one it had already passed. The linear
    // falloff turned that zero distance into the term's full maximum, pointing
    // backwards along the sheep's own travel, and because that push moved the
    // sheep off the contact line the next tick saw nothing at all, so a sheep
    // held on a face alternated between the maximum and exactly zero.
    //
    // The fix is in the query: a reported contact now always lies at or ahead of
    // the body, and touching a rectangle's boundary while travelling along it is
    // not being inside it. This oracle pins both the geometry and the dynamics,
    // because the defect was only visible when something held a sheep on the
    // line for many consecutive ticks.
    using wide_eye::game::PaddockObstacle;
    using wide_eye::game::Vec3;
    constexpr double kLookAhead = 6.25;
    constexpr double kMaximumAcceleration = 4.0;
    constexpr double kRadius = wide_eye::game::kSheepCollisionRadius;
    // Every paddock shape's north face is at `z = 16`, so one contact line runs
    // the width of the paddock and a sheep can slide along all three shapes.
    constexpr double kContactLineZ = 16.0 + kRadius;
    constexpr double kSlideSpeed = 1.0;
    constexpr double kTurnSpeed = 0.5;
    constexpr double kReferenceSpeed = 5.0;
    const double kTurningClosingScale = 2.0 * kTurnSpeed * std::sqrt(2.0) / kReferenceSpeed;
    const double kTurningResponseMagnitude = kMaximumAcceleration * kTurningClosingScale;
    constexpr double kJustClear = 0.01;
    constexpr double kDropPastMagnitude =
        kMaximumAcceleration * (1.0 - (kLookAhead - kJustClear) / kLookAhead);
    constexpr std::uint64_t kContactTicks = 240;
    result.ticks = kContactTicks;
    result.contact_line_z = kContactLineZ;
    result.minimum_published_distance = std::numeric_limits<double>::infinity();

    const wide_eye::game::PaddockCollisionField closed_field{false};
    // The query on its own, at the exact positions the clip produces. A body on
    // the line travelling along it reaches nothing; the same body turning into
    // the face reaches it *now*, at a distance of exactly zero and through the
    // face it is actually entering; and a body already inside the expanded
    // rectangle — the QA-001 radius band — has no face ahead of it at all.
    const auto along_the_face = closed_field.approaching_obstacle({.x = 7.0, .z = kContactLineZ},
                                                                  {.x = 1.0}, kLookAhead, kRadius);
    const auto into_the_face = closed_field.approaching_obstacle(
        {.x = 7.0, .z = kContactLineZ}, {.x = 1.0, .z = -1.0}, kLookAhead, kRadius);
    const auto inside_the_band = closed_field.approaching_obstacle(
        {.x = 7.0, .z = 16.0 + kRadius / 2.0}, {.x = 1.0}, kLookAhead, kRadius);
    const auto just_clear_along = closed_field.approaching_obstacle(
        {.x = 7.0, .z = kContactLineZ + kJustClear}, {.x = 1.0}, kLookAhead, kRadius);
    const auto half_clear_into = closed_field.approaching_obstacle(
        {.x = 7.0, .z = kContactLineZ + kRadius}, {.z = -1.0}, kLookAhead, kRadius);
    const auto at_the_look_ahead = closed_field.approaching_obstacle(
        {.x = 7.0, .z = kContactLineZ + kLookAhead}, {.z = -1.0}, kLookAhead, kRadius);
    const auto ground_at_the_look_ahead = closed_field.approaching_ground_boundary(
        {.x = kLookAhead, .z = 27.0}, {.x = -1.0}, kLookAhead);
    const auto ground_just_inside_the_look_ahead = closed_field.approaching_ground_boundary(
        {.x = kLookAhead - kJustClear, .z = 27.0}, {.x = -1.0}, kLookAhead);
    if (!check(along_the_face.obstacle == PaddockObstacle::none &&
                   along_the_face.face_normal == Vec3{} &&
                   just_clear_along.obstacle == PaddockObstacle::none,
               "a_body_travelling_along_a_face_it_touches_reaches_no_obstacle") ||
        !check(into_the_face.obstacle == PaddockObstacle::left_wall &&
                   into_the_face.contact_distance == 0.0 &&
                   into_the_face.face_normal == Vec3{.z = 1.0},
               "a_body_turning_into_the_face_it_touches_reaches_it_at_zero") ||
        !check(inside_the_band.obstacle == PaddockObstacle::none,
               "a_body_already_inside_an_expanded_rectangle_reaches_no_obstacle") ||
        // The distances either side of the contact line are unchanged, so the
        // near boundary is now continuous in the same way the far one was.
        !check(half_clear_into.obstacle == PaddockObstacle::left_wall &&
                   half_clear_into.contact_distance == kRadius &&
                   at_the_look_ahead.obstacle == PaddockObstacle::left_wall &&
                   at_the_look_ahead.contact_distance == kLookAhead,
               "the_distances_either_side_of_the_contact_line_are_unchanged") ||
        !check(!ground_at_the_look_ahead.boundary_ahead &&
                   ground_just_inside_the_look_ahead.boundary_ahead &&
                   ground_just_inside_the_look_ahead.contact_distance == kLookAhead - kJustClear &&
                   ground_just_inside_the_look_ahead.face_normal == Vec3{.x = 1.0},
               "the_ground_query_is_continuous_at_the_inclusive_outer_boundary")) {
        return result;
    }

    // The minimized QA-005 corner: the right wall's nearer +X edge is the
    // paddock's own cylinder limit, so it is not a route a sheep body can pass.
    // The only geometry-named lateral lies beyond this look-ahead. A simultaneous
    // drop therefore pushes inward in X while the wall pushes outward in Z;
    // neither response points toward the unreachable corner as the old rule did.
    constexpr wide_eye::game::SheepState kCornerSheep{
        .id = 1,
        .position = {.x = 26.9, .y = 1.0, .z = 16.6},
        .velocity = {.x = 0.08, .z = -0.01},
        .grounded = true,
    };
    const auto corner_obstacle = closed_field.approaching_obstacle(
        kCornerSheep.position, kCornerSheep.velocity, kLookAhead, kRadius);
    wide_eye::game::SheepAvoidanceEvidence corner_avoidance{.subject_id = kCornerSheep.id};
    wide_eye::game::apply_sheep_avoidance(kCornerSheep, {.enabled = true}, closed_field,
                                          corner_avoidance);
    if (!check(corner_obstacle.obstacle == PaddockObstacle::right_wall &&
                   corner_obstacle.lateral_escape == Vec3{.x = -1.0} &&
                   corner_obstacle.lateral_clearance > kLookAhead && corner_avoidance.drop_ahead &&
                   corner_avoidance.avoidance_acceleration.x < 0.0 &&
                   corner_avoidance.avoidance_acceleration.z > 0.0,
               "the_right_wall_drop_corner_points_only_toward_reachable_safe_space")) {
        return result;
    }

    // The dynamics. Two sheep sit on the contact line while a stationary dog
    // presses them onto it, which is the configuration QA-003 reproduced: the
    // clip refuses the `z` axis every tick and clears that velocity component,
    // so each of them begins every tick exactly on the line travelling exactly
    // along it. A third starts a hundredth clear of the same line, which is
    // QA-005's grazing case rather than this one. Two more sit either side of
    // the drop boundary. Every one of them is at least one body radius clear of
    // every obstacle face, so none of them starts inside the radius band QA-001
    // is about.
    const ScenarioHandle avoid_on_scenario = named_scenario("sheep-avoidance-on");
    if (!check(avoid_on_scenario != nullptr, "the_contact_fixture_has_a_paired_parent")) {
        return result;
    }
    const auto contact_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*avoid_on_scenario);
    auto& contact_scenario = *contact_scenario_holder;
    contact_scenario.dog.initial_state = {
        .position = {.x = 8.0, .y = 1.0, .z = 19.5}, .heading_radians = 0.0, .grounded = true};
    // Dog pressure is the influence that holds a sheep against a face in play,
    // and it is the one QA-003 observed doing it. Nothing else is enabled, so
    // every published avoidance vector below is the term's own answer.
    contact_scenario.sheep_dog_pressure = {.enabled = true};
    contact_scenario.initial_sheep = {{
        {.id = 1,
         .position = {.x = 9.0, .y = 1.0, .z = kContactLineZ},
         .velocity = {.x = kSlideSpeed},
         .heading_radians = 1.57079632679489661923,
         .grounded = true},
        {.id = 2,
         .position = {.x = 11.0, .y = 1.0, .z = kContactLineZ + kJustClear},
         .velocity = {.x = kSlideSpeed},
         .heading_radians = 1.57079632679489661923,
         .grounded = true},
        {.id = 3,
         .position = {.x = 13.0, .y = 1.0, .z = kContactLineZ},
         .velocity = {.x = kSlideSpeed, .z = -kTurnSpeed},
         .heading_radians = 1.57079632679489661923,
         .grounded = true},
        // Either side of the drop boundary, out of the dog's reach so their
        // first-tick answers are the drop half alone. The probe of the first
        // lands exactly on the paddock's own bound, where the ground is still
        // finite; the probe of the second lands a hundredth past it.
        {.id = 4,
         .position = {.x = kLookAhead, .y = 1.0, .z = 27.0},
         .velocity = {.x = -3.0},
         .heading_radians = -1.57079632679489661923,
         .grounded = true},
        {.id = 5,
         .position = {.x = kLookAhead - kJustClear, .y = 1.0, .z = 29.0},
         .velocity = {.x = -3.0},
         .heading_radians = -1.57079632679489661923,
         .grounded = true},
    }};

    const SimulationHandle contact = make_simulation(contact_scenario);
    std::array<Vec3, 3> previous_push{};
    std::array<bool, 3> previous_live{};
    bool contact_line_held = true;
    bool every_published_distance_is_forward = true;
    for (std::uint64_t tick = 1; tick <= kContactTicks; ++tick) {
        contact->fixed_update({});
        const auto& snapshot = contact->current_snapshot();
        if (tick == 1) {
            result.parallel_contact = evidence_with_id(snapshot.sheep_avoidance_evidence, 1);
            result.just_clear = evidence_with_id(snapshot.sheep_avoidance_evidence, 2);
            result.into_face = evidence_with_id(snapshot.sheep_avoidance_evidence, 3);
            result.drop_at_bound = evidence_with_id(snapshot.sheep_avoidance_evidence, 4);
            result.drop_past_bound = evidence_with_id(snapshot.sheep_avoidance_evidence, 5);
            result.into_face_magnitude = std::hypot(result.into_face.avoidance_acceleration.x,
                                                    result.into_face.avoidance_acceleration.z);
        }
        for (std::uint32_t id = 1; id <= 3; ++id) {
            const auto& member = sheep_with_id(snapshot.sheep, id);
            const auto& contact_evidence = evidence_with_id(snapshot.sheep_collision_evidence, id);
            const auto& avoid = evidence_with_id(snapshot.sheep_avoidance_evidence, id);
            const std::size_t slot = id - 1;
            // Sheep 2 starts a hundredth clear and is pressed down onto the line
            // within the window, so the assertion is that no sheep is ever
            // *below* the line and that the ones on it stay exactly on it.
            contact_line_held = contact_line_held && member.position.z >= kContactLineZ;
            if (contact_evidence.clipped_z) {
                ++result.clip_ticks;
                contact_line_held = contact_line_held && member.position.z == kContactLineZ;
            }
            if (avoid.obstacle != PaddockObstacle::none) {
                every_published_distance_is_forward =
                    every_published_distance_is_forward && avoid.obstacle_distance >= 0.0;
                result.minimum_published_distance =
                    std::min(result.minimum_published_distance, avoid.obstacle_distance);
            }
            const Vec3 push = avoid.avoidance_acceleration;
            const bool live = std::hypot(push.x, push.z) > 1.0e-6;
            bool unsettled = false;
            if (tick > 1) {
                unsettled =
                    live != previous_live[slot] ||
                    (live && push.x * previous_push[slot].x + push.z * previous_push[slot].z < 0.0);
            }
            if (unsettled) {
                if (id == 1) {
                    ++result.parallel_flaps;
                } else if (id == 3) {
                    ++result.turning_flaps;
                } else {
                    ++result.grazing_flaps;
                }
            }
            previous_push[slot] = push;
            previous_live[slot] = live;
        }
    }

    if (!check(result.parallel_contact.avoidance_evaluated &&
                   result.parallel_contact.obstacle == PaddockObstacle::none &&
                   result.parallel_contact.obstacle_distance == 0.0 &&
                   !result.parallel_contact.drop_ahead &&
                   result.parallel_contact.avoidance_acceleration == Vec3{},
               "a_sheep_touching_the_face_it_slides_along_is_left_alone") ||
        !check(result.just_clear.avoidance_evaluated &&
                   result.just_clear.obstacle == PaddockObstacle::none &&
                   result.just_clear.avoidance_acceleration == Vec3{},
               "a_sheep_a_hundredth_clear_of_the_same_face_answers_the_same") ||
        // "Already as close as the geometry allows" is what a contact distance
        // of zero now means. The response points through the face the sheep is
        // entering — outward, plus the way round — and is proportional to the
        // component of travel that is actually entering it.
        !check(result.into_face.avoidance_evaluated &&
                   result.into_face.obstacle == PaddockObstacle::left_wall &&
                   result.into_face.obstacle_distance == 0.0 &&
                   result.into_face.avoidance_acceleration.z > 0.0 &&
                   result.into_face.avoidance_acceleration.x ==
                       result.into_face.avoidance_acceleration.z &&
                   std::abs(result.into_face_magnitude - kTurningResponseMagnitude) < 1.0e-12,
               "a_sheep_touching_a_face_gets_an_approach_proportional_outward_response")) {
        return result;
    }

    if (!check(result.clip_ticks > 0 && contact_line_held,
               "the_hard_clip_held_the_flock_on_the_contact_line_throughout") ||
        !check(every_published_distance_is_forward && result.minimum_published_distance >= 0.0,
               "every_published_contact_distance_lies_ahead_of_the_sheep") ||
        // The defect itself: before the fix these two sheep alternated between
        // the term's full maximum and exactly zero for as long as the dog held
        // them on the line. A settled term switches once, when the sheep stops
        // turning into the face, and never again.
        !check(result.parallel_flaps == 0,
               "a_sheep_held_on_a_contact_face_publishes_one_settled_answer") ||
        !check(result.turning_flaps <= 1,
               "a_sheep_that_stops_turning_into_a_face_settles_once_and_stays_settled")) {
        return result;
    }

    // Sheep 2 is pressed toward the same line from a hundredth clear. The
    // response now scales with real closing speed, so it settles inside the same
    // five-flaps-per-hundred-ticks allowance as every continuous steering term.
    constexpr std::uint64_t kGrazingFlapAllowance = 12;
    result.grazing_flap_allowance = kGrazingFlapAllowance;
    if (!check(result.grazing_flaps <= kGrazingFlapAllowance,
               "a_grazing_sheep_flaps_no_more_than_its_recorded_allowance")) {
        return result;
    }

    // The drop boundary is continuous too: a path ending on the inclusive
    // ground boundary gets nothing, while one crossing it by a hundredth gets
    // only the corresponding fraction of the maximum back into grounded space.
    if (!check(result.drop_at_bound.avoidance_evaluated && !result.drop_at_bound.drop_ahead &&
                   result.drop_at_bound.avoidance_acceleration == Vec3{},
               "a_probe_landing_exactly_on_the_paddock_bound_is_not_a_drop") ||
        !check(result.drop_past_bound.drop_ahead &&
                   result.drop_past_bound.avoidance_acceleration == Vec3{.x = kDropPastMagnitude},
               "a_probe_crossing_the_paddock_bound_gets_a_graded_inward_response")) {
        return result;
    }

    result.passed = true;
    return result;
}

// What the QA-001 depenetration regression observed, returned so the run report
// can name the numbers without keeping the fixtures alive in `main`.
struct BandPassthroughOracle {
    bool passed = false;
    double on_the_face_z = 0.0;
    double inside_the_band_z = 0.0;
    double at_one_radius_z = 0.0;
    double a_radius_clear_z = 0.0;
    wide_eye::game::Vec3 fully_inside{};
    wide_eye::game::Vec3 axis_tie{};
    wide_eye::game::Vec3 shape_tie{};
    wide_eye::game::Vec3 two_shape_wedge{};
    double dog_on_the_face_z = 0.0;
    double dog_at_one_radius_z = 0.0;
    std::uint64_t witness_ticks = 0;
    std::uint64_t witness_overlap_ticks = 0;
    double witness_minimum_z = 0.0;
};

// Whether one upright cylinder overlaps one analytic paddock rectangle, written
// out here rather than borrowed from the field so the regression measures the
// geometry itself instead of re-asking the code under test.
bool body_overlaps_rectangle(double x, double z, double radius, double minimum_x, double maximum_x,
                             double minimum_z, double maximum_z) {
    return x + radius > minimum_x && x - radius < maximum_x && z + radius > minimum_z &&
           z - radius < maximum_z;
}

bool body_overlaps_closed_paddock(double x, double z, double radius) {
    return body_overlaps_rectangle(x, z, radius, 1.0, 14.0, 14.0, 16.0) ||
           body_overlaps_rectangle(x, z, radius, 18.0, 31.0, 14.0, 16.0) ||
           body_overlaps_rectangle(x, z, radius, 14.0, 18.0, 15.0, 16.0);
}

BandPassthroughOracle run_band_passthrough_oracle() {
    BandPassthroughOracle result;
    // QA-001. `move_axis` refuses a displacement by asking whether the body was
    // clear of the face *before* the move, so it only answers correctly for a
    // body that starts clear. A body that starts inside an obstacle's radius
    // band matched no refusal, walked into the shape, through it, and out the
    // far side, and nothing pushed it back out. The fix restores the
    // precondition instead of changing the refusal: an overlapping body is
    // pushed out along the shallowest single-axis move that clears every shape
    // at once, and the displacement is then resolved exactly as before.
    //
    // No suite covered a body starting inside the band, which is why the defect
    // survived the whole sheep-collision outcome. This one covers the five cases
    // that matter — on a face, inside the band, at exactly one radius, fully
    // inside a shape, and at a corner where two ways out are equally shallow —
    // at both body radii the field is asked about, plus the named scenario the
    // defect was reproduced in.
    using wide_eye::game::CylinderMoveResult;
    using wide_eye::game::PaddockCollisionField;
    using wide_eye::game::PaddockObstacle;
    using wide_eye::game::Vec3;
    constexpr double kSheepRadius = wide_eye::game::kSheepCollisionRadius;
    constexpr double kDogRadius = wide_eye::game::DogController::kRadius;
    // The closed gate spans `x[14, 18]`, `z[15, 16]`, so its north face is at
    // `z = 16` and a body of radius `r` rests against it at `16 + r`.
    constexpr double kGateFaceZ = 16.0;
    constexpr double kStep = -0.05;
    const PaddockCollisionField closed_field{false};

    const CylinderMoveResult on_the_face = closed_field.resolve_cylinder_move(
        {.x = 15.0, .z = kGateFaceZ}, {.z = kStep}, kSheepRadius);
    const CylinderMoveResult inside_the_band =
        closed_field.resolve_cylinder_move({.x = 15.0, .z = 16.4}, {.z = kStep}, kSheepRadius);
    const CylinderMoveResult at_one_radius = closed_field.resolve_cylinder_move(
        {.x = 15.0, .z = kGateFaceZ + kSheepRadius}, {.z = kStep}, kSheepRadius);
    const CylinderMoveResult a_radius_clear =
        closed_field.resolve_cylinder_move({.x = 15.0, .z = 17.0}, {.z = kStep}, kSheepRadius);
    result.on_the_face_z = on_the_face.position.z;
    result.inside_the_band_z = inside_the_band.position.z;
    result.at_one_radius_z = at_one_radius.position.z;
    result.a_radius_clear_z = a_radius_clear.position.z;

    if (!check(on_the_face.position.z == kGateFaceZ + kSheepRadius && on_the_face.clipped_z &&
                   on_the_face.obstacle == PaddockObstacle::gate,
               "a_body_on_a_face_is_put_back_on_it_rather_than_let_through") ||
        !check(inside_the_band.position.z == kGateFaceZ + kSheepRadius &&
                   inside_the_band.clipped_z && inside_the_band.obstacle == PaddockObstacle::gate,
               "a_body_inside_the_radius_band_is_put_back_on_the_face") ||
        // The accepted case. A body at exactly one radius was already refused
        // before this correction and has to answer identically after it.
        !check(at_one_radius.position.z == kGateFaceZ + kSheepRadius && at_one_radius.clipped_z &&
                   at_one_radius.obstacle == PaddockObstacle::gate,
               "a_body_at_exactly_one_radius_is_still_refused") ||
        !check(a_radius_clear.position.z == 17.0 + kStep && !a_radius_clear.clipped_z &&
                   a_radius_clear.obstacle == PaddockObstacle::none,
               "a_body_a_full_radius_clear_still_moves_freely")) {
        return result;
    }

    // Fully inside the gate, at the exact centre of its expanded rectangle in
    // `x`. Both `x` escapes are `2.5` deep and both `z` escapes are `1.0`, so
    // the shallowest pair is the `z` one and the fixed `-x`, `+x`, `-z`, `+z`
    // face order takes `-z`. The body leaves through the south face and the gate
    // then refuses the rest of its displacement.
    const CylinderMoveResult fully_inside =
        closed_field.resolve_cylinder_move({.x = 16.0, .z = 15.5}, {.z = kStep}, kSheepRadius);
    result.fully_inside = fully_inside.position;
    // An exact tie between one `x` face and one `z` face of the *same* shape.
    // Inside the left wall at `(1.5, 14.5)`, `-x` and `-z` are both `1.0` deep
    // and both land clear, so the face order decides it and X wins, exactly as
    // the X-first resolve pass and the look-ahead's corner tie already do.
    const CylinderMoveResult axis_tie =
        closed_field.resolve_cylinder_move({.x = 1.5, .z = 14.5}, {}, kSheepRadius);
    result.axis_tie = axis_tie.position;
    // An exact tie between two *different* shapes. At `(18.25, 16.25)` the
    // gate's `+x` face and the right wall's `+z` face are both `0.25` deep, so
    // the field's own obstacle order decides it and the right wall wins.
    const CylinderMoveResult shape_tie =
        closed_field.resolve_cylinder_move({.x = 18.25, .z = 16.25}, {}, kSheepRadius);
    result.shape_tie = shape_tie.position;
    // Wedged in the overlap of two shapes, where the shallowest way out of each
    // one is a way into the other. Taking only escapes that clear *every* shape
    // is what makes this one step instead of a body handed back and forth.
    const CylinderMoveResult two_shape_wedge =
        closed_field.resolve_cylinder_move({.x = 18.0, .z = 15.0}, {}, kSheepRadius);
    result.two_shape_wedge = two_shape_wedge.position;

    if (!check(fully_inside.position.x == 16.0 && fully_inside.position.z == 15.0 - kSheepRadius &&
                   fully_inside.clipped_z && fully_inside.obstacle == PaddockObstacle::gate,
               "a_body_fully_inside_a_shape_leaves_through_its_shallowest_face") ||
        !check(axis_tie.position.x == 1.0 - kSheepRadius && axis_tie.position.z == 14.5 &&
                   axis_tie.clipped_x && !axis_tie.clipped_z &&
                   axis_tie.obstacle == PaddockObstacle::left_wall,
               "two_equally_shallow_faces_of_one_shape_resolve_to_the_x_face") ||
        !check(shape_tie.position.x == 18.25 && shape_tie.position.z == 16.0 + kSheepRadius &&
                   shape_tie.clipped_z && shape_tie.obstacle == PaddockObstacle::right_wall,
               "two_equally_shallow_shapes_resolve_to_the_earlier_shape") ||
        !check(two_shape_wedge.position.x == 18.0 &&
                   two_shape_wedge.position.z == 14.0 - kSheepRadius &&
                   !body_overlaps_closed_paddock(two_shape_wedge.position.x,
                                                 two_shape_wedge.position.z, kSheepRadius),
               "a_body_wedged_between_two_shapes_leaves_both_in_one_step")) {
        return result;
    }

    // Every one of those corrected positions has to be a fixed point: a body the
    // field has already put on a face must not be pushed again on the next tick,
    // or a resting body would jitter for as long as it rested.
    bool every_correction_is_a_fixed_point = true;
    for (const Vec3& corrected :
         {on_the_face.position, inside_the_band.position, at_one_radius.position,
          fully_inside.position, axis_tie.position, shape_tie.position, two_shape_wedge.position}) {
        const CylinderMoveResult settled = closed_field.resolve_cylinder_move(
            {.x = corrected.x, .z = corrected.z}, {}, kSheepRadius);
        every_correction_is_a_fixed_point = every_correction_is_a_fixed_point &&
                                            settled.position.x == corrected.x &&
                                            settled.position.z == corrected.z;
    }

    // The dog motor asks the same field at its own radius. It is the older
    // caller and the one whose behavior is accepted, so both ends of the rule
    // are pinned here: a dog on a face is pushed off it, and a dog at exactly
    // one radius answers exactly as it always did.
    const CylinderMoveResult dog_on_the_face =
        closed_field.resolve_cylinder_move({.x = 15.0, .z = kGateFaceZ}, {.z = kStep}, kDogRadius);
    const CylinderMoveResult dog_at_one_radius = closed_field.resolve_cylinder_move(
        {.x = 15.0, .z = kGateFaceZ + kDogRadius}, {.z = kStep}, kDogRadius);
    result.dog_on_the_face_z = dog_on_the_face.position.z;
    result.dog_at_one_radius_z = dog_at_one_radius.position.z;

    if (!check(every_correction_is_a_fixed_point,
               "a_body_the_field_has_already_corrected_is_not_corrected_again") ||
        !check(dog_on_the_face.position.z == kGateFaceZ + kDogRadius && dog_on_the_face.clipped_z &&
                   dog_on_the_face.obstacle == PaddockObstacle::gate,
               "the_dog_radius_answers_the_band_the_same_way") ||
        !check(dog_at_one_radius.position.z == kGateFaceZ + kDogRadius &&
                   dog_at_one_radius.clipped_z &&
                   dog_at_one_radius.obstacle == PaddockObstacle::gate,
               "the_accepted_dog_refusal_at_one_radius_is_unchanged")) {
        return result;
    }

    // The named-scenario witness. `sheep-dog-facing-off` and `-on` place sheep 4
    // at exactly `(15, 16)`, on the closed gate's north face, and that placement
    // is kept deliberately: it is the only fixture in the game that reproduces
    // this defect, and moving it would also move the accepted first-tick 3-4-5
    // facing measurement it carries. Driven with no dog input, that sheep used
    // to end `400` ticks south of the wall line having crossed the closed gate.
    constexpr std::uint64_t kWitnessTicks = 400;
    result.witness_ticks = kWitnessTicks;
    result.witness_minimum_z = std::numeric_limits<double>::infinity();
    for (const std::string_view name : {"sheep-dog-facing-off", "sheep-dog-facing-on"}) {
        const ScenarioHandle scenario = named_scenario(name);
        if (!check(scenario != nullptr, "the_band_witness_fixtures_exist")) {
            return result;
        }
        const SimulationHandle simulation = make_simulation(*scenario);
        for (std::uint64_t tick = 0; tick < kWitnessTicks; ++tick) {
            simulation->fixed_update({});
            const auto& sheep = simulation->current_snapshot().sheep;
            for (const auto& member : sheep) {
                if (body_overlaps_closed_paddock(member.position.x, member.position.z,
                                                 kSheepRadius)) {
                    ++result.witness_overlap_ticks;
                }
            }
            result.witness_minimum_z = std::min(result.witness_minimum_z, sheep[3].position.z);
        }
    }

    if (!check(result.witness_overlap_ticks == 0,
               "no_published_sheep_ever_occupies_a_paddock_shape") ||
        !check(result.witness_minimum_z == kGateFaceZ + kSheepRadius,
               "the_witness_sheep_never_gets_south_of_the_gate_face")) {
        return result;
    }

    result.passed = true;
    return result;
}

// What the behavior-transition oracle observed, returned so the run report can
// name the numbers without keeping the fixtures alive in `main`.
struct BehaviorTransitionOracle {
    bool passed = false;
    double rise_rate = 0.0;
    double recovery_rate = 0.0;
    double rise_step = 0.0;
    double recovery_step = 0.0;
    double rest_arousal = 0.0;
    double alert_arousal = 0.0;
    double driven_release_arousal = 0.0;
    double driven_arousal = 0.0;
    double stimulus_radius = 0.0;
    std::uint64_t cycle_ticks = 0;
    std::uint64_t alert_tick = 0;
    std::uint64_t driven_tick = 0;
    std::uint64_t recovering_tick = 0;
    std::uint64_t settled_tick = 0;
    double alert_prior_arousal = 0.0;
    double driven_prior_arousal = 0.0;
    double recovering_prior_arousal = 0.0;
    double recovering_release_stimulus = 0.0;
    double settled_prior_arousal = 0.0;
    double peak_arousal = 0.0;
    double control_arousal = 0.0;
    double band_stimulus = 0.0;
    double band_arousal = 0.0;
    std::uint32_t band_alert_changes = 0;
    std::uint32_t band_driven_changes = 0;
    double boundary_stimulus = 0.0;
    double boundary_arousal = 0.0;
    std::uint64_t boundary_driven_ticks = 0;
    double stubborn_stimulus = 0.0;
    double stubborn_arousal = 0.0;
    double clamped_stimulus = 0.0;
    std::uint64_t adversarial_ticks = 0;
    std::uint32_t adversarial_changes = 0;
    std::uint32_t adversarial_late_changes = 0;
    std::uint32_t adversarial_threshold_flips = 0;
    std::uint32_t adversarial_above = 0;
    std::uint32_t adversarial_below = 0;
    std::uint64_t scripted_alert_tick = 0;
    std::uint64_t scripted_driven_tick = 0;
    std::uint64_t scripted_recovering_tick = 0;
    std::uint64_t scripted_settled_tick = 0;
    double scripted_peak_arousal = 0.0;
    std::size_t allocations = 0;
};

// One sheep's published behavior history, recorded so the oracle can name the
// exact tick of a transition and the prior state that produced it.
struct BehaviorTransitionRecord {
    std::uint64_t tick = 0;
    double prior_arousal = 0.0;
    double stimulus = 0.0;
    double arousal = 0.0;
    wide_eye::game::SheepBehaviorState from = wide_eye::game::SheepBehaviorState::settled;
    wide_eye::game::SheepBehaviorState to = wide_eye::game::SheepBehaviorState::settled;
};

BehaviorTransitionOracle run_behavior_transition_oracle() {
    BehaviorTransitionOracle result;
    // Behavior transitions and the arousal proxy. Every accepted rule so far
    // decides how a sheep is pushed; none of them says what the sheep *is*. The
    // two fields that were supposed to say it — `arousal` and `behavior` — have
    // existed in the authoritative buffer since the first sheep and have never
    // been written, so every sheep in the game has been permanently `settled`
    // with an arousal of exactly zero.
    //
    // Arousal is a **named game parameter, not a claim about animal
    // physiology**: a bounded `[0, 1]` design variable that follows the
    // published dog stimulus at named rates. The rule maps it, plus whether a
    // cause is acting, onto the four accepted labels with explicit hysteresis.
    //
    // The paired fixture enables no steering term at all, so nothing can
    // accelerate a sheep and every number below is exact arithmetic on the
    // fixture's own geometry.
    using wide_eye::game::SheepBehaviorState;
    constexpr double kRiseRate = 1.875;
    constexpr double kRecoveryRate = 0.234375;
    constexpr double kRestArousal = 0.125;
    constexpr double kAlertArousal = 0.25;
    constexpr double kDrivenReleaseArousal = 0.5;
    constexpr double kDrivenArousal = 0.75;
    constexpr double kStimulusRadius = 8.0;
    constexpr double kSubjectSpeed = 3.75;
    constexpr std::uint64_t kCycleTicks = 400;
    constexpr std::uint64_t kBandTicks = 240;
    constexpr double kBandArousal = 0.625;
    constexpr std::uint64_t kAdversarialTicks = 400;
    constexpr double kAdversarialHoldSpeed = 0.1171875;
    constexpr std::uint64_t kAdversarialSettleTick = 100;
    constexpr std::uint64_t kScriptedApproachTicks = 90;
    constexpr std::uint64_t kScriptedHoldTicks = 150;
    constexpr std::uint64_t kScriptedTicks = 700;
    const double rise_step = kRiseRate * wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    const double recovery_step =
        kRecoveryRate * wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    result.rise_rate = kRiseRate;
    result.recovery_rate = kRecoveryRate;
    result.rise_step = rise_step;
    result.recovery_step = recovery_step;
    result.rest_arousal = kRestArousal;
    result.alert_arousal = kAlertArousal;
    result.driven_release_arousal = kDrivenReleaseArousal;
    result.driven_arousal = kDrivenArousal;
    result.stimulus_radius = kStimulusRadius;
    result.cycle_ticks = kCycleTicks;
    result.adversarial_ticks = kAdversarialTicks;

    const ScenarioHandle behavior_off_scenario =
        named_scenario("sheep-behavior-transitions-off");
    const ScenarioHandle behavior_on_scenario =
        named_scenario("sheep-behavior-transitions-on");
    auto behavior_on_as_control = mutable_scenario_copy(behavior_on_scenario);
    if (behavior_off_scenario != nullptr) {
        behavior_on_as_control->id = behavior_off_scenario->id;
    }
    behavior_on_as_control->sheep_behavior.enabled = false;
    if (!check(behavior_off_scenario != nullptr && behavior_on_scenario != nullptr &&
                   behavior_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_behavior_transitions_off &&
                   behavior_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_behavior_transitions_on &&
                   *behavior_on_as_control == *behavior_off_scenario &&
                   behavior_on_scenario->sheep_behavior.enabled &&
                   !behavior_off_scenario->sheep_behavior.enabled &&
                   behavior_off_scenario->version == 1 && behavior_off_scenario->seed == 0,
               "paired_behavior_fixture_differs_only_by_the_transition_switch") ||
        // The fixture must isolate the transitions: any enabled steering term
        // would move a sheep and make the stimulus curve inexact. Temperament is
        // deliberately enabled — with every dog term off it produces no vector
        // at all, and its response scale is exactly the variable two of these
        // sheep exist to show.
        !check(!behavior_off_scenario->sheep_separation.enabled &&
                   !behavior_off_scenario->sheep_attraction.enabled &&
                   !behavior_off_scenario->sheep_alignment.enabled &&
                   !behavior_off_scenario->sheep_dog_pressure.enabled &&
                   !behavior_off_scenario->sheep_dog_approach.enabled &&
                   !behavior_off_scenario->sheep_dog_facing.enabled &&
                   !behavior_off_scenario->sheep_dog_line_of_sight.enabled &&
                   !behavior_off_scenario->sheep_avoidance.enabled &&
                   !behavior_off_scenario->sheep_combined_influence.enabled &&
                   !behavior_off_scenario->sheep_motion_limit.enabled &&
                   behavior_off_scenario->sheep_temperament.enabled,
               "the_behavior_fixture_enables_no_steering_term") ||
        // The rates and thresholds are exact binary fractions at 60 Hz, which is
        // what lets every arousal value below be pinned with equality. Rise is
        // exactly eight times recovery: pressure is felt in about half a second
        // and shed over about four, because the accepted loop makes release the
        // slow half of the pressure/release pair.
        !check(behavior_on_scenario->sheep_behavior.rise_rate_per_second == kRiseRate &&
                   behavior_on_scenario->sheep_behavior.recovery_rate_per_second == kRecoveryRate &&
                   kRiseRate == 8.0 * kRecoveryRate && rise_step == 1.0 / 32.0 &&
                   recovery_step == 1.0 / 256.0,
               "the_arousal_rates_are_exact_binary_fractions_at_sixty_hertz") ||
        // A ladder, not four unordered numbers: each band is entered above where
        // it is left and the bands do not cross.
        !check(behavior_on_scenario->sheep_behavior.rest_arousal == kRestArousal &&
                   behavior_on_scenario->sheep_behavior.alert_arousal == kAlertArousal &&
                   behavior_on_scenario->sheep_behavior.driven_release_arousal ==
                       kDrivenReleaseArousal &&
                   behavior_on_scenario->sheep_behavior.driven_arousal == kDrivenArousal &&
                   wide_eye::game::kSheepMinimumArousal < kRestArousal &&
                   kRestArousal < kAlertArousal && kAlertArousal <= kDrivenReleaseArousal &&
                   kDrivenReleaseArousal < kDrivenArousal &&
                   kDrivenArousal <= wide_eye::game::kSheepMaximumArousal,
               "the_four_arousal_thresholds_form_an_ordered_hysteretic_ladder") ||
        // The geometry is chosen so one tick of the subject's travel moves the
        // stimulus by exactly one part in 128.
        !check(behavior_on_scenario->sheep_dog_pressure.radius == kStimulusRadius &&
                   behavior_on_scenario->initial_sheep[0].velocity.x == -kSubjectSpeed &&
                   kSubjectSpeed * wide_eye::game::GameplaySimulation::kFixedDeltaSeconds ==
                       1.0 / 16.0 &&
                   (kSubjectSpeed * wide_eye::game::GameplaySimulation::kFixedDeltaSeconds) /
                           kStimulusRadius ==
                       1.0 / 128.0,
               "one_subject_tick_moves_the_stimulus_by_exactly_one_part_in_one_hundred_and_"
               "twenty_eight")) {
        return result;
    }

    const SimulationHandle behavior_off = make_simulation(*behavior_off_scenario);
    const SimulationHandle behavior_on = make_simulation(*behavior_on_scenario);
    const auto behavior_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(behavior_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& behavior_initial = *behavior_initial_holder;

    std::array<BehaviorTransitionRecord, 8> subject_transitions{};
    std::size_t subject_transition_count = 0;
    SheepBehaviorState subject_state = behavior_initial.sheep[0].behavior;
    bool only_behavior_differs = true;
    bool control_is_settled_at_zero = true;
    bool recovering_only_after_release = true;
    bool settled_never_hides_a_recovery = true;
    bool rise_is_exactly_one_thirty_second = true;
    bool recovery_is_exactly_one_two_hundred_and_fifty_sixth = true;
    bool boundary_never_leaves_driven = true;
    bool boundary_seen_driven = false;
    std::uint64_t boundary_driven_ticks = 0;
    for (std::uint64_t tick = 1; tick <= kCycleTicks; ++tick) {
        behavior_off->fixed_update({});
        behavior_on->fixed_update({});
        const auto& on_snapshot = behavior_on->current_snapshot();
        const auto& off_snapshot = behavior_off->current_snapshot();
        const auto& on_prior = behavior_on->previous_snapshot();
        for (std::size_t index = 0; index < on_snapshot.sheep_count; ++index) {
            const auto& on_member = on_snapshot.sheep[index];
            const auto& off_member = off_snapshot.sheep[index];
            const auto& stimulus = on_snapshot.sheep_dog_pressure_evidence[index];
            // The transitions are observational: with the switch off every
            // published field must be identical, and with it on only the two
            // fields the rule writes may differ. That is the whole claim that
            // behavior does not feed back into steering.
            wide_eye::game::SheepState on_without_behavior = on_member;
            on_without_behavior.arousal = off_member.arousal;
            on_without_behavior.behavior = off_member.behavior;
            only_behavior_differs = only_behavior_differs && on_without_behavior == off_member &&
                                    stimulus == off_snapshot.sheep_dog_pressure_evidence[index] &&
                                    on_snapshot.sheep_social_evidence[index] ==
                                        off_snapshot.sheep_social_evidence[index] &&
                                    on_snapshot.sheep_collision_evidence[index] ==
                                        off_snapshot.sheep_collision_evidence[index] &&
                                    on_snapshot.sheep_avoidance_evidence[index] ==
                                        off_snapshot.sheep_avoidance_evidence[index] &&
                                    on_snapshot.sheep_combined_influence_evidence[index] ==
                                        off_snapshot.sheep_combined_influence_evidence[index] &&
                                    on_snapshot.sheep_motion_limit_evidence[index] ==
                                        off_snapshot.sheep_motion_limit_evidence[index];
            control_is_settled_at_zero = control_is_settled_at_zero && off_member.arousal == 0.0 &&
                                         off_member.behavior == SheepBehaviorState::settled;
            // `recovering` is the released state and nothing else: it can never
            // be published while a cause is acting, and a released sheep that
            // still carries arousal can never be published as settled.
            const bool cause_acting = stimulus.arousal_stimulus > kRestArousal;
            const double prior_arousal = on_prior.sheep[index].arousal;
            if (on_member.behavior == SheepBehaviorState::recovering) {
                recovering_only_after_release =
                    recovering_only_after_release && !cause_acting && prior_arousal > kRestArousal;
            }
            if (on_member.behavior == SheepBehaviorState::settled && !cause_acting) {
                settled_never_hides_a_recovery =
                    settled_never_hides_a_recovery && prior_arousal <= kRestArousal;
            }
        }

        // Rates: the in-band sheep climbs by exactly one rise budget per tick
        // until it reaches its cause, and the subject sheds exactly one recovery
        // budget per tick for every tick it spends recovering.
        const auto& band_member = sheep_with_id(on_snapshot.sheep, 3);
        const auto& band_prior = sheep_with_id(on_prior.sheep, 3);
        if (band_member.arousal < kBandArousal) {
            rise_is_exactly_one_thirty_second =
                rise_is_exactly_one_thirty_second &&
                band_member.arousal - band_prior.arousal == rise_step;
        }
        const auto& subject = sheep_with_id(on_snapshot.sheep, 1);
        const auto& subject_prior = sheep_with_id(on_prior.sheep, 1);
        if (subject.behavior == SheepBehaviorState::recovering) {
            recovery_is_exactly_one_two_hundred_and_fifty_sixth =
                recovery_is_exactly_one_two_hundred_and_fifty_sixth &&
                subject.arousal - subject_prior.arousal == -recovery_step;
        }
        // The nervous sheep's cause holds its arousal at exactly the driven
        // entry threshold, so once it is driven it sits on that boundary for the
        // rest of the run.
        const auto& boundary_member = sheep_with_id(on_snapshot.sheep, 4);
        boundary_seen_driven =
            boundary_seen_driven || boundary_member.behavior == SheepBehaviorState::driven;
        if (boundary_seen_driven) {
            ++boundary_driven_ticks;
            boundary_never_leaves_driven = boundary_never_leaves_driven &&
                                           boundary_member.behavior == SheepBehaviorState::driven &&
                                           boundary_member.arousal == kDrivenArousal;
        }

        result.peak_arousal = std::max(result.peak_arousal, subject.arousal);
        if (subject.behavior != subject_state &&
            subject_transition_count < subject_transitions.size()) {
            subject_transitions[subject_transition_count] = {
                .tick = tick,
                .prior_arousal = subject_prior.arousal,
                .stimulus =
                    evidence_with_id(on_snapshot.sheep_dog_pressure_evidence, 1).arousal_stimulus,
                .arousal = subject.arousal,
                .from = subject_state,
                .to = subject.behavior};
            ++subject_transition_count;
            subject_state = subject.behavior;
        }
    }
    result.boundary_driven_ticks = boundary_driven_ticks;

    const auto& final_on = behavior_on->current_snapshot();
    result.control_arousal = sheep_with_id(final_on.sheep, 2).arousal;
    result.band_arousal = sheep_with_id(final_on.sheep, 3).arousal;
    result.band_stimulus =
        evidence_with_id(final_on.sheep_dog_pressure_evidence, 3).arousal_stimulus;
    result.boundary_arousal = sheep_with_id(final_on.sheep, 4).arousal;
    result.boundary_stimulus =
        evidence_with_id(final_on.sheep_dog_pressure_evidence, 4).arousal_stimulus;
    result.stubborn_arousal = sheep_with_id(final_on.sheep, 5).arousal;
    result.stubborn_stimulus =
        evidence_with_id(final_on.sheep_dog_pressure_evidence, 5).arousal_stimulus;
    if (subject_transition_count == 4) {
        result.alert_tick = subject_transitions[0].tick;
        result.alert_prior_arousal = subject_transitions[0].prior_arousal;
        result.driven_tick = subject_transitions[1].tick;
        result.driven_prior_arousal = subject_transitions[1].prior_arousal;
        result.recovering_tick = subject_transitions[2].tick;
        result.recovering_prior_arousal = subject_transitions[2].prior_arousal;
        result.recovering_release_stimulus = subject_transitions[2].stimulus;
        result.settled_tick = subject_transitions[3].tick;
        result.settled_prior_arousal = subject_transitions[3].prior_arousal;
    }

    if (!check(only_behavior_differs, "the_transitions_change_nothing_but_arousal_and_behavior") ||
        !check(control_is_settled_at_zero,
               "a_fixture_with_the_transitions_off_stays_settled_at_exactly_zero_arousal") ||
        // The one sequence the roadmap item names, in one deterministic run.
        !check(subject_transition_count == 4 &&
                   subject_transitions[0].from == SheepBehaviorState::settled &&
                   subject_transitions[0].to == SheepBehaviorState::alert &&
                   subject_transitions[1].to == SheepBehaviorState::driven &&
                   subject_transitions[2].to == SheepBehaviorState::recovering &&
                   subject_transitions[3].to == SheepBehaviorState::settled,
               "one_run_walks_settled_alert_driven_recovering_settled_in_that_order") ||
        // Each transition is produced by the *prior* arousal, so the arousal
        // published beside a new label is already one tick past the threshold
        // that selected it.
        !check(
            subject_transitions[0].prior_arousal == kAlertArousal &&
                subject_transitions[1].prior_arousal == kDrivenArousal &&
                subject_transitions[3].prior_arousal == kRestArousal,
            "each_ladder_transition_is_produced_by_the_prior_arousal_exactly_at_its_threshold") ||
        // Release, not decay, is what produces `recovering`: the cause fell to
        // rest while the sheep was still carrying most of its arousal.
        !check(subject_transitions[2].from == SheepBehaviorState::driven &&
                   subject_transitions[2].stimulus <= kRestArousal &&
                   subject_transitions[2].prior_arousal > kDrivenReleaseArousal,
               "recovering_is_entered_by_release_while_the_sheep_is_still_aroused") ||
        !check(recovering_only_after_release && settled_never_hides_a_recovery,
               "recovering_and_settled_are_separated_by_whether_a_cause_is_acting") ||
        !check(rise_is_exactly_one_thirty_second &&
                   recovery_is_exactly_one_two_hundred_and_fifty_sixth,
               "arousal_rises_and_decays_at_exactly_the_configured_rates") ||
        !check(result.peak_arousal == wide_eye::game::kSheepMaximumArousal,
               "an_exact_dog_overlap_drives_the_subject_to_full_arousal") ||
        // The in-fixture control: a dog exactly at the stimulus radius is not a
        // cause at all.
        !check(result.control_arousal == 0.0 &&
                   sheep_with_id(final_on.sheep, 2).behavior == SheepBehaviorState::settled &&
                   evidence_with_id(final_on.sheep_dog_pressure_evidence, 2).dog_distance ==
                       kStimulusRadius &&
                   evidence_with_id(final_on.sheep_dog_pressure_evidence, 2).arousal_stimulus ==
                       0.0,
               "a_dog_exactly_at_the_stimulus_radius_raises_no_arousal_at_all") ||
        // Temperament scales the cause, not the rule: two sheep at the same
        // exact distance end at opposite ends of the ladder.
        !check(result.boundary_stimulus == kDrivenArousal &&
                   result.boundary_arousal == kDrivenArousal &&
                   sheep_with_id(final_on.sheep, 4).behavior == SheepBehaviorState::driven &&
                   result.stubborn_stimulus == 0.1875 &&
                   result.stubborn_arousal == result.stubborn_stimulus &&
                   sheep_with_id(final_on.sheep, 5).behavior == SheepBehaviorState::settled &&
                   evidence_with_id(final_on.sheep_dog_pressure_evidence, 4).dog_distance ==
                       evidence_with_id(final_on.sheep_dog_pressure_evidence, 5).dog_distance,
               "temperament_scales_the_arousal_stimulus_from_an_identical_cause") ||
        // The literal boundary case: a sheep whose arousal is exactly the driven
        // entry threshold on every tick publishes `driven` on every one of them.
        !check(boundary_driven_ticks > kCycleTicks / 2 && boundary_never_leaves_driven,
               "a_sheep_resting_exactly_on_an_entry_threshold_never_flaps")) {
        return result;
    }

    // Hysteresis in its defining form. Two runs of the same sheep under the same
    // constant cause, holding exactly the same arousal inside the driven
    // hysteresis band, differ only in the label they started from — and publish
    // two different labels for the whole window. No rule that reads arousal
    // alone can produce that.
    const auto band_alert_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*behavior_on_scenario);
    auto& band_alert_scenario = *band_alert_scenario_holder;
    band_alert_scenario.initial_sheep[2].arousal = kBandArousal;
    band_alert_scenario.initial_sheep[2].behavior = SheepBehaviorState::alert;
    auto band_driven_scenario = band_alert_scenario;
    band_driven_scenario.initial_sheep[2].behavior = SheepBehaviorState::driven;
    const SimulationHandle band_alert = make_simulation(band_alert_scenario);
    const SimulationHandle band_driven = make_simulation(band_driven_scenario);
    bool band_arousal_is_constant = true;
    SheepBehaviorState band_alert_state = SheepBehaviorState::alert;
    SheepBehaviorState band_driven_state = SheepBehaviorState::driven;
    for (std::uint64_t tick = 1; tick <= kBandTicks; ++tick) {
        band_alert->fixed_update({});
        band_driven->fixed_update({});
        const auto& alert_member = sheep_with_id(band_alert->current_snapshot().sheep, 3);
        const auto& driven_member = sheep_with_id(band_driven->current_snapshot().sheep, 3);
        band_arousal_is_constant = band_arousal_is_constant &&
                                   alert_member.arousal == kBandArousal &&
                                   driven_member.arousal == kBandArousal;
        if (alert_member.behavior != band_alert_state) {
            ++result.band_alert_changes;
            band_alert_state = alert_member.behavior;
        }
        if (driven_member.behavior != band_driven_state) {
            ++result.band_driven_changes;
            band_driven_state = driven_member.behavior;
        }
    }
    if (!check(kDrivenReleaseArousal < kBandArousal && kBandArousal < kDrivenArousal &&
                   band_arousal_is_constant,
               "both_band_runs_hold_the_same_arousal_inside_the_driven_hysteresis_band") ||
        !check(result.band_alert_changes == 0 && result.band_driven_changes == 0 &&
                   band_alert_state == SheepBehaviorState::alert &&
                   band_driven_state == SheepBehaviorState::driven,
               "the_same_arousal_holds_two_different_stable_labels_from_two_prior_states")) {
        return result;
    }

    // The adversarial case the hysteresis exists for: a cause tuned so the
    // published arousal crosses the driven entry threshold in one direction or
    // the other on nearly every tick. The rule must not follow it. The
    // counterfactual is computed from the published arousal series rather than
    // from a second rule in the engine: a threshold-only rule using the same
    // `driven_arousal` would have changed the label once per crossing.
    const auto adversarial_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*behavior_on_scenario);
    auto& adversarial_scenario = *adversarial_scenario_holder;
    adversarial_scenario.initial_sheep[0].position = {.x = 24.0, .y = 1.0, .z = 20.0};
    adversarial_scenario.initial_sheep[0].velocity = {.x = kAdversarialHoldSpeed};
    const SimulationHandle adversarial = make_simulation(adversarial_scenario);
    SheepBehaviorState adversarial_state = SheepBehaviorState::settled;
    bool adversarial_above_threshold = false;
    bool adversarial_first_sample = true;
    for (std::uint64_t tick = 1; tick <= kAdversarialTicks; ++tick) {
        // A dog that reverses every single tick. Its wiggle moves the stimulus,
        // and therefore the arousal, up and down on alternating ticks.
        const wide_eye::game::GameplayTickInput input{
            .dog_move = wide_eye::game::DogMoveInput{.world_x = tick % 2 == 1 ? 1.0 : -1.0}};
        adversarial->fixed_update(input);
        const auto& member = sheep_with_id(adversarial->current_snapshot().sheep, 1);
        if (member.behavior != adversarial_state) {
            ++result.adversarial_changes;
            if (tick > kAdversarialSettleTick) {
                ++result.adversarial_late_changes;
            }
            adversarial_state = member.behavior;
        }
        const bool above = member.arousal >= kDrivenArousal;
        if (tick > kAdversarialSettleTick) {
            if (above) {
                ++result.adversarial_above;
            } else {
                ++result.adversarial_below;
            }
        }
        if (!adversarial_first_sample && above != adversarial_above_threshold) {
            ++result.adversarial_threshold_flips;
        }
        adversarial_first_sample = false;
        adversarial_above_threshold = above;
    }
    if (!check(result.adversarial_above > 100 && result.adversarial_below > 100 &&
                   result.adversarial_threshold_flips > kAdversarialTicks / 2,
               "the_adversarial_cause_really_does_cross_the_threshold_on_alternating_ticks") ||
        !check(result.adversarial_late_changes == 0 &&
                   adversarial_state == SheepBehaviorState::driven,
               "the_hysteretic_rule_holds_one_label_while_a_threshold_only_rule_would_flap")) {
        return result;
    }

    // The same four-state sequence when the *dog* is the body that moves: it
    // walks in, holds, and leaves under scripted input. The numbers are no
    // longer exact — the dog motor's acceleration is not a binary fraction — but
    // the sequence and its cause are the same, so the result does not depend on
    // which body the fixture chose to move.
    const auto scripted_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*behavior_on_scenario);
    auto& scripted_scenario = *scripted_scenario_holder;
    scripted_scenario.initial_sheep[0].velocity = {};
    const SimulationHandle scripted = make_simulation(scripted_scenario);
    SheepBehaviorState scripted_state = SheepBehaviorState::settled;
    for (std::uint64_t tick = 1; tick <= kScriptedTicks; ++tick) {
        wide_eye::game::GameplayTickInput input{.dog_move = wide_eye::game::DogMoveInput{}};
        if (tick <= kScriptedApproachTicks) {
            input.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0};
        } else if (tick > kScriptedHoldTicks) {
            input.dog_move = wide_eye::game::DogMoveInput{.world_x = -1.0};
        }
        scripted->fixed_update(input);
        const auto& member = sheep_with_id(scripted->current_snapshot().sheep, 1);
        result.scripted_peak_arousal = std::max(result.scripted_peak_arousal, member.arousal);
        if (member.behavior == scripted_state) {
            continue;
        }
        scripted_state = member.behavior;
        switch (scripted_state) {
        case SheepBehaviorState::alert:
            if (result.scripted_alert_tick == 0) {
                result.scripted_alert_tick = tick;
            }
            break;
        case SheepBehaviorState::driven:
            if (result.scripted_driven_tick == 0) {
                result.scripted_driven_tick = tick;
            }
            break;
        case SheepBehaviorState::recovering:
            if (result.scripted_recovering_tick == 0) {
                result.scripted_recovering_tick = tick;
            }
            break;
        case SheepBehaviorState::settled:
            if (result.scripted_driven_tick != 0 && result.scripted_settled_tick == 0) {
                result.scripted_settled_tick = tick;
            }
            break;
        }
    }
    if (!check(result.scripted_alert_tick != 0 &&
                   result.scripted_driven_tick > result.scripted_alert_tick &&
                   result.scripted_recovering_tick > result.scripted_driven_tick &&
                   result.scripted_settled_tick > result.scripted_recovering_tick &&
                   scripted_state == SheepBehaviorState::settled,
               "a_dog_that_approaches_holds_and_leaves_walks_the_same_four_states")) {
        return result;
    }

    // A nervous sheep close to the dog would carry the product above one.
    // Arousal is a bounded design parameter, so the stimulus clamps instead.
    const auto clamped_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*behavior_on_scenario);
    auto& clamped_scenario = *clamped_scenario_holder;
    clamped_scenario.initial_sheep[3].position = {.x = 22.0, .y = 1.0, .z = 21.0};
    const SimulationHandle clamped = make_simulation(clamped_scenario);
    clamped->fixed_update({});
    const auto& clamped_evidence =
        evidence_with_id(clamped->current_snapshot().sheep_dog_pressure_evidence, 4);
    result.clamped_stimulus = clamped_evidence.arousal_stimulus;
    if (!check(clamped_evidence.dog_distance == 1.0 &&
                   clamped_evidence.temperament_response_scale == 2.0 &&
                   result.clamped_stimulus == wide_eye::game::kSheepMaximumArousal,
               "a_stimulus_above_the_arousal_maximum_is_clamped_rather_than_accumulated")) {
        return result;
    }

    // The transition reads prior state, so the same tick's dog motion cannot
    // change it: a fixture fed a full dog move on its first tick publishes
    // exactly the arousal, label, and stimulus of one fed nothing at all.
    const SimulationHandle prior_state_moved = make_simulation(*behavior_on_scenario);
    const SimulationHandle prior_state_still = make_simulation(*behavior_on_scenario);
    prior_state_moved->fixed_update(
        {.dog_move = wide_eye::game::DogMoveInput{.world_x = -1.0, .sprint = true}});
    prior_state_still->fixed_update({});
    bool prior_state_is_immune = prior_state_moved->current_snapshot().dog.position.x !=
                                 prior_state_still->current_snapshot().dog.position.x;
    for (const auto& member : prior_state_still->current_snapshot().sheep) {
        const auto& moved_member =
            sheep_with_id(prior_state_moved->current_snapshot().sheep, member.id);
        prior_state_is_immune =
            prior_state_is_immune && moved_member.arousal == member.arousal &&
            moved_member.behavior == member.behavior &&
            evidence_with_id(prior_state_moved->current_snapshot().sheep_dog_pressure_evidence,
                             member.id) ==
                evidence_with_id(prior_state_still->current_snapshot().sheep_dog_pressure_evidence,
                                 member.id);
    }
    if (!check(prior_state_is_immune,
               "the_same_ticks_dog_move_cannot_alter_the_transition_it_reads_prior_state_for")) {
        return result;
    }

    const auto reversed_behavior_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*behavior_on_scenario);
    auto& reversed_behavior_scenario = *reversed_behavior_scenario_holder;
    std::reverse(reversed_behavior_scenario.initial_sheep.begin(),
                 reversed_behavior_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_behavior_scenario.sheep_count));
    const SimulationHandle reversed_behavior = make_simulation(reversed_behavior_scenario);
    for (std::uint64_t tick = 1; tick <= 120; ++tick) {
        reversed_behavior->fixed_update({});
    }
    const SimulationHandle forward_behavior = make_simulation(*behavior_on_scenario);
    for (std::uint64_t tick = 1; tick <= 120; ++tick) {
        forward_behavior->fixed_update({});
    }
    for (const auto& member : forward_behavior->current_snapshot().sheep) {
        if (!check(member == sheep_with_id(reversed_behavior->current_snapshot().sheep, member.id),
                   "behavior_and_arousal_are_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(forward_behavior->current_snapshot().sheep_dog_pressure_evidence,
                                 member.id) ==
                    evidence_with_id(
                        reversed_behavior->current_snapshot().sheep_dog_pressure_evidence,
                        member.id),
                "the_arousal_stimulus_is_stable_under_reversed_storage")) {
            return result;
        }
    }

    const auto behavior_state = wide_eye::game::gameplay_state_dump_json(*behavior_on);
    const auto behavior_control_state = wide_eye::game::gameplay_state_dump_json(*behavior_off);
    if (!check(behavior_state &&
                   behavior_state.text.find("\"arousal\":0.75,\"behavior\":\"driven\","
                                            "\"temperament\":\"nervous\"") != std::string::npos &&
                   behavior_state.text.find("\"temperament_response_scale\":2,"
                                            "\"arousal_stimulus\":0.75") != std::string::npos &&
                   behavior_state.text.find("\"arousal\":0.625,\"behavior\":\"alert\"") !=
                       std::string::npos,
               "state_dump_contains_the_driven_and_alert_evidence") ||
        !check(behavior_control_state &&
                   behavior_control_state.text.find("\"arousal\":0,\"behavior\":\"settled\","
                                                    "\"temperament\":\"nervous\"") !=
                       std::string::npos &&
                   behavior_control_state.text.find("\"arousal_stimulus\":0.75") !=
                       std::string::npos,
               "a_fixture_without_the_transitions_publishes_the_same_cause_and_no_arousal")) {
        return result;
    }

    // The writer still refuses a state it cannot describe. Both fixtures below
    // keep the transitions switched off so the scenario's own construction
    // checks do not reject the value first: the point is that the *contract*
    // rejects it.
    const auto unknown_behavior_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*behavior_off_scenario);
    auto& unknown_behavior_scenario = *unknown_behavior_scenario_holder;
    unknown_behavior_scenario.initial_sheep[0].behavior = static_cast<SheepBehaviorState>(255);
    const auto out_of_range_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*behavior_off_scenario);
    auto& out_of_range_scenario = *out_of_range_scenario_holder;
    out_of_range_scenario.initial_sheep[0].arousal = 1.5;
    const SimulationHandle unknown_behavior = make_simulation(unknown_behavior_scenario);
    const SimulationHandle out_of_range = make_simulation(out_of_range_scenario);
    if (!check(wide_eye::game::gameplay_state_dump_json(*unknown_behavior).error ==
                   wide_eye::game::GameplayContractError::non_finite_state,
               "an_unknown_behavior_state_is_still_rejected") ||
        !check(wide_eye::game::gameplay_state_dump_json(*out_of_range).error ==
                   wide_eye::game::GameplayContractError::non_finite_state,
               "an_arousal_outside_its_stated_range_is_rejected")) {
        return result;
    }

    const SimulationHandle allocation_behavior = make_simulation(*behavior_on_scenario);
    const std::size_t behavior_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_behavior->fixed_update({});
    }
    result.allocations = g_allocation_count - behavior_allocations_before;
    if (!check(result.allocations == 0, "behavior_fixed_updates_do_not_allocate")) {
        return result;
    }

    // Restart restores the starting contract exactly, including a non-zero
    // starting arousal and a non-settled starting label.
    behavior_on->restart();
    band_driven->restart();
    bool restart_restores_arousal_and_behavior = true;
    for (const auto& member : band_driven->current_snapshot().sheep) {
        const auto& fixture = sheep_with_id(band_driven_scenario.initial_sheep, member.id);
        restart_restores_arousal_and_behavior = restart_restores_arousal_and_behavior &&
                                                member.arousal == fixture.arousal &&
                                                member.behavior == fixture.behavior;
    }
    if (!check(behavior_on->current_snapshot() == behavior_initial &&
                   behavior_on->previous_snapshot() == behavior_initial,
               "behavior_restart_restores_the_paired_fixture") ||
        !check(restart_restores_arousal_and_behavior &&
                   sheep_with_id(band_driven->current_snapshot().sheep, 3).arousal ==
                       kBandArousal &&
                   sheep_with_id(band_driven->current_snapshot().sheep, 3).behavior ==
                       SheepBehaviorState::driven,
               "behavior_restart_restores_a_non_zero_starting_arousal_and_label")) {
        return result;
    }

    result.passed = true;
    return result;
}

struct FlockResponseOracle {
    bool passed = false;
    double connectivity_distance = 0.0;
    double rest_arousal = 0.0;
    std::uint64_t scripted_ticks = 0;
    std::uint64_t pressure_onset_tick = 0;
    std::uint64_t response_tick = 0;
    std::uint64_t response_latency_ticks = 0;
    std::uint64_t release_tick = 0;
    std::uint64_t settle_tick = 0;
    std::uint64_t settle_ticks = 0;
    std::uint32_t pressure_episodes = 0;
    std::uint32_t releases = 0;
    std::uint32_t unanswered_pressure_episodes = 0;
    std::uint32_t interrupted_settles = 0;
    std::uint32_t scripted_split_episodes = 0;
    std::uint32_t scripted_rejoins = 0;
    std::uint64_t scripted_ticks_split = 0;
    wide_eye::game::FlockDogObservables onset_dog{};
    wide_eye::game::FlockDogObservables closest_dog{};
    std::uint64_t closest_tick = 0;
    wide_eye::game::FlockDogObservables release_dog{};
    double release_peak_arousal = 0.0;
    std::uint64_t passing_ticks = 0;
    std::uint64_t passing_pressure_onset_tick = 0;
    std::uint64_t passing_response_latency_ticks = 0;
    std::uint64_t passing_rejoin_tick = 0;
    std::uint64_t passing_rejoin_ticks = 0;
    std::uint64_t passing_second_split_tick = 0;
    std::uint64_t passing_time_to_split_ticks = 0;
    std::uint32_t passing_split_episodes = 0;
    std::uint32_t passing_rejoins = 0;
    std::uint64_t passing_ticks_split = 0;
    std::size_t allocations = 0;
};

// One tick of the published cause, taken by ID rather than by buffer index so
// the fold cannot silently depend on storage order.
std::array<double, wide_eye::game::kDefaultGameplaySheepCount>
published_arousal_stimulus(const wide_eye::game::GameplaySnapshot& snapshot) {
    std::array<double, wide_eye::game::kDefaultGameplaySheepCount> stimulus{};
    for (std::size_t index = 0; index < snapshot.sheep_count; ++index) {
        stimulus[index] =
            evidence_with_id(snapshot.sheep_dog_pressure_evidence, snapshot.sheep[index].id)
                .arousal_stimulus;
    }
    return stimulus;
}

// The scenario-level measurement of the four new timing definitions and the
// flock-level dog geometry, on published state rather than on hand-authored
// values. Both fixtures are `sheep-behavior-transitions-on`, which enables no
// steering term at all, so every number below is a consequence of the fixture's
// own geometry and the accepted arousal rule and of nothing else.
//
// The two halves exist because one fixture cannot show both. With no steering
// term enabled nothing accelerates a sheep, so the *scripted* half — where the
// dog walks in, holds, and leaves under scripted input while every sheep stands
// still — is the one that releases the whole flock and therefore the only one
// that can measure a settle time; its component count never changes. The
// *passing* half leaves the fixture's own moving sheep alone, and that sheep
// carries the flock in and out of one connected component, which is what a
// split and a rejoin are.
FlockResponseOracle run_flock_response_oracle() {
    FlockResponseOracle result;
    using wide_eye::game::SheepBehaviorState;
    // The connectivity distance is a caller-owned parameter of the observable
    // pass, not an accepted game rule, so this oracle names its own and says
    // why: at `5.0` the fixture's four standing sheep are exactly one component
    // and its moving sheep joins and leaves that component on exact ticks, which
    // is what makes the split and rejoin measurements below arithmetic rather
    // than approximate.
    constexpr double kConnectivityDistance = 5.0;
    constexpr std::uint64_t kScriptedApproachTicks = 200;
    constexpr std::uint64_t kScriptedHoldTicks = 300;
    constexpr std::uint64_t kScriptedTicks = 1000;
    constexpr std::uint64_t kPassingTicks = 400;
    // The fixture's dog starts five units from a nervous sheep, which is a cause
    // acting from the first tick and therefore a press with no rising edge to
    // measure. This half moves the dog's start west of every sheep's stimulus
    // radius so the approach itself is what opens the episode: "the dog arrives"
    // has to be an event before "how long did the flock take to answer it" can
    // be a duration.
    constexpr wide_eye::game::Vec3 kScriptedDogStart{.x = 8.0, .y = 1.0, .z = 20.0};
    const ScenarioHandle scenario = named_scenario("sheep-behavior-transitions-on");
    if (!check(scenario != nullptr && scenario->sheep_behavior.enabled,
               "the_flock_response_fixture_is_the_named_behavior_transition_scenario")) {
        return result;
    }
    result.connectivity_distance = kConnectivityDistance;
    result.rest_arousal = scenario->sheep_behavior.rest_arousal;
    result.scripted_ticks = kScriptedTicks;
    result.passing_ticks = kPassingTicks;

    const auto scripted_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*scenario);
    auto& scripted_scenario = *scripted_scenario_holder;
    scripted_scenario.initial_sheep[0].velocity = {};
    scripted_scenario.dog.initial_state.position = kScriptedDogStart;
    const SimulationHandle scripted = make_simulation(scripted_scenario);
    wide_eye::game::FlockResponseTiming timing{};
    bool folded = true;
    bool observables_valid = true;
    double closest_distance = std::numeric_limits<double>::infinity();
    std::uint64_t previous_pressure_episodes = 0;
    std::uint64_t previous_releases = 0;
    for (std::uint64_t tick = 1; tick <= kScriptedTicks; ++tick) {
        wide_eye::game::GameplayTickInput input{.dog_move = wide_eye::game::DogMoveInput{}};
        if (tick <= kScriptedApproachTicks) {
            input.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0};
        } else if (tick > kScriptedHoldTicks) {
            input.dog_move = wide_eye::game::DogMoveInput{.world_x = -1.0};
        }
        scripted->fixed_update(input);
        const auto& snapshot = scripted->current_snapshot();
        const auto observables = wide_eye::game::compute_flock_observables(
            active_sheep(snapshot), NoChosenNeighbors{},
            kConnectivityDistance, snapshot.dog.position);
        if (!observables.has_value()) {
            observables_valid = false;
            break;
        }
        const auto next = wide_eye::game::advance_flock_response_timing(
            timing, tick, active_sheep(snapshot),
                      published_arousal_stimulus(snapshot),
            observables->connected_component_count, scenario->sheep_behavior.rest_arousal);
        if (!next.has_value()) {
            folded = false;
            break;
        }
        timing = *next;
        if (timing.pressure_episodes != previous_pressure_episodes) {
            previous_pressure_episodes = timing.pressure_episodes;
            result.onset_dog = observables->dog;
        }
        if (timing.releases != previous_releases) {
            previous_releases = timing.releases;
            result.release_dog = observables->dog;
            for (const auto& member : active(snapshot.sheep, snapshot.sheep_count)) {
                result.release_peak_arousal = std::max(result.release_peak_arousal, member.arousal);
            }
        }
        if (observables->dog.centroid_distance < closest_distance) {
            closest_distance = observables->dog.centroid_distance;
            result.closest_dog = observables->dog;
            result.closest_tick = tick;
        }
    }
    result.pressure_onset_tick = timing.pressure_onset_tick;
    result.response_latency_ticks = timing.response_latency_ticks.value_or(0);
    result.response_tick = timing.pressure_onset_tick + result.response_latency_ticks;
    result.release_tick = timing.release_tick;
    result.settle_ticks = timing.settle_ticks.value_or(0);
    result.settle_tick = timing.release_tick + result.settle_ticks;
    result.pressure_episodes = timing.pressure_episodes;
    result.releases = timing.releases;
    result.unanswered_pressure_episodes = timing.unanswered_pressure_episodes;
    result.interrupted_settles = timing.interrupted_settles;
    result.scripted_split_episodes = timing.split_episodes;
    result.scripted_rejoins = timing.rejoins;
    result.scripted_ticks_split = timing.ticks_split;
    if (!check(observables_valid, "every_published_scripted_snapshot_is_describable") ||
        !check(folded, "every_published_scripted_snapshot_folds_into_the_timing_record") ||
        // One press, answered, released, and settled: the whole pressure/release
        // pair the accepted loop is built on, measured in ticks for the first
        // time.
        !check(timing.pressure_episodes == 1 && timing.releases == 1 &&
                   timing.unanswered_pressure_episodes == 0 && timing.interrupted_settles == 0,
               "the_scripted_dog_presses_the_flock_exactly_once_and_releases_it_once") ||
        !check(timing.response_latency_ticks.has_value() && timing.settle_ticks.has_value(),
               "the_scripted_press_measures_a_latency_and_a_settle_time") ||
        !check(result.response_tick > result.pressure_onset_tick &&
                   result.release_tick > result.response_tick &&
                   result.settle_tick > result.release_tick,
               "cause_then_response_then_release_then_settled_in_that_order") ||
        // The measured settle time has to be consistent with the accepted
        // recovery rate rather than merely plausible: a flock carrying this much
        // arousal when the cause lifted cannot shed it faster than the rule
        // allows, whatever the fixture's geometry.
        !check(static_cast<double>(result.settle_ticks) *
                       (scenario->sheep_behavior.recovery_rate_per_second *
                        wide_eye::game::GameplaySimulation::kFixedDeltaSeconds) >=
                   result.release_peak_arousal - scenario->sheep_behavior.rest_arousal,
               "the_settle_time_is_at_least_what_the_accepted_recovery_rate_needs") ||
        !check(timing.flock_settled && !timing.pressure_acting && !timing.flock_engaged,
               "the_run_ends_with_the_whole_flock_settled_and_no_cause_acting") ||
        // Nothing moves in this half, so the four standing sheep and the one
        // stopped sheep are two components for the whole run: a split with no
        // rejoin, on published state rather than on a hand-authored count.
        !check(timing.split_episodes == 1 && timing.rejoins == 0 &&
                   !timing.rejoin_ticks.has_value() && timing.ticks_split == kScriptedTicks,
               "a_flock_that_never_closes_publishes_a_split_and_no_rejoin")) {
        return result;
    }

    // The flock-level dog geometry. The dog walks east into a flock whose
    // centroid is east of it, so it closes and then retreats; the rear sheep is
    // the member it is furthest behind and is a different sheep from the nearest
    // one once the dog is inside the group.
    if (!check(result.onset_dog.evaluated && result.onset_dog.bearing_defined &&
                   result.closest_dog.evaluated && result.release_dog.evaluated,
               "the_flock_level_dog_observables_are_evaluated_at_every_sampled_tick") ||
        !check(result.closest_dog.centroid_distance < result.onset_dog.centroid_distance &&
                   result.release_dog.centroid_distance > result.closest_dog.centroid_distance,
               "the_dog_closes_on_the_centroid_and_then_leaves_it") ||
        // The two selections answer different questions, and this fixture shows
        // it: with the dog still outside the group they name the same sheep,
        // and with the dog inside it the member it is furthest behind is no
        // longer the member it is closest to.
        !check(result.onset_dog.nearest_sheep_id == result.onset_dog.rear_sheep_id &&
                   result.closest_dog.nearest_sheep_id != result.closest_dog.rear_sheep_id,
               "the_rear_member_and_the_nearest_member_are_not_the_same_sheep") ||
        !check(result.onset_dog.rear_sheep_id != 0 && result.closest_dog.rear_sheep_id != 0 &&
                   result.onset_dog.rear_offset < 0.0 && result.closest_dog.rear_offset < 0.0 &&
                   result.closest_dog.rear_distance > result.closest_dog.nearest_distance,
               "the_rear_member_is_named_and_lies_behind_the_centroid_on_the_push_axis")) {
        return result;
    }

    // The passing half: the fixture's own moving sheep, left exactly as the
    // scenario defines it, with a stationary dog. That sheep starts outside the
    // standing group, closes into it, and leaves again, so the flock rejoins and
    // then splits at ticks that are exact arithmetic on a constant velocity.
    const SimulationHandle passing = make_simulation(*scenario);
    wide_eye::game::FlockResponseTiming passing_timing{};
    bool passing_folded = true;
    std::uint32_t previous_rejoins = 0;
    std::uint32_t previous_split_episodes = 0;
    for (std::uint64_t tick = 1; tick <= kPassingTicks; ++tick) {
        passing->fixed_update({});
        const auto& snapshot = passing->current_snapshot();
        const auto observables = wide_eye::game::compute_flock_observables(
            active_sheep(snapshot), NoChosenNeighbors{},
            kConnectivityDistance, snapshot.dog.position);
        const auto next =
            observables.has_value()
                ? wide_eye::game::advance_flock_response_timing(
                      passing_timing, tick, active_sheep(snapshot),
                      published_arousal_stimulus(snapshot),
                      observables->connected_component_count, scenario->sheep_behavior.rest_arousal)
                : std::nullopt;
        if (!next.has_value()) {
            passing_folded = false;
            break;
        }
        passing_timing = *next;
        if (passing_timing.rejoins != previous_rejoins) {
            previous_rejoins = passing_timing.rejoins;
            result.passing_rejoin_tick = tick;
            result.passing_rejoin_ticks = passing_timing.rejoin_ticks.value_or(0);
        }
        if (passing_timing.split_episodes != previous_split_episodes) {
            previous_split_episodes = passing_timing.split_episodes;
            if (passing_timing.split_episodes == 2) {
                result.passing_second_split_tick = tick;
            }
        }
    }
    result.passing_pressure_onset_tick = passing_timing.pressure_onset_tick;
    result.passing_response_latency_ticks = passing_timing.response_latency_ticks.value_or(0);
    result.passing_split_episodes = passing_timing.split_episodes;
    result.passing_rejoins = passing_timing.rejoins;
    result.passing_ticks_split = passing_timing.ticks_split;
    result.passing_time_to_split_ticks = passing_timing.time_to_split_ticks.value_or(0);
    if (!check(passing_folded, "every_published_passing_snapshot_folds_into_the_timing_record") ||
        // A nervous sheep five units from the stationary dog carries a stimulus
        // of exactly `0.75` from the first tick, so this fixture is under
        // pressure for its whole run and never releases: it can measure a
        // latency and a split, and deliberately cannot measure a settle time.
        !check(passing_timing.pressure_onset_tick == 1 && passing_timing.pressure_episode_open &&
                   passing_timing.releases == 0 && !passing_timing.settle_ticks.has_value(),
               "a_fixture_whose_cause_never_lifts_measures_no_settle_time") ||
        !check(passing_timing.response_latency_ticks.has_value(),
               "the_standing_press_still_measures_a_response_latency") ||
        !check(passing_timing.split_episodes == 2 && passing_timing.rejoins == 1 &&
                   result.passing_rejoin_ticks == result.passing_rejoin_tick - 1,
               "the_passing_sheep_closes_the_flock_once_and_opens_it_once") ||
        !check(result.passing_second_split_tick > result.passing_rejoin_tick &&
                   result.passing_time_to_split_ticks ==
                       result.passing_second_split_tick - passing_timing.pressure_onset_tick,
               "time_to_split_is_measured_from_the_press_that_was_already_acting") ||
        !check(passing_timing.ticks_split ==
                   result.passing_rejoin_tick - 1 +
                       (kPassingTicks - result.passing_second_split_tick + 1),
               "the_ticks_spent_split_are_the_two_open_ended_windows")) {
        return result;
    }

    // The record is a fold over published state, so re-running the same
    // scenario reproduces it exactly, and reversing the storage order of the
    // fixture cannot change it.
    const auto reversed_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*scenario);
    auto& reversed_scenario = *reversed_scenario_holder;
    std::reverse(reversed_scenario.initial_sheep.begin(),
                 reversed_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_scenario.sheep_count));
    const SimulationHandle repeated = make_simulation(*scenario);
    const SimulationHandle reversed = make_simulation(reversed_scenario);
    wide_eye::game::FlockResponseTiming repeated_timing{};
    wide_eye::game::FlockResponseTiming reversed_timing{};
    bool repeatable = true;
    for (std::uint64_t tick = 1; tick <= kPassingTicks; ++tick) {
        repeated->fixed_update({});
        reversed->fixed_update({});
        for (const auto* simulation : {repeated.get(), reversed.get()}) {
            const auto& snapshot = simulation->current_snapshot();
            const auto observables = wide_eye::game::compute_flock_observables(
                active_sheep(snapshot), NoChosenNeighbors{},
                kConnectivityDistance, snapshot.dog.position);
            auto& target = simulation == repeated.get() ? repeated_timing : reversed_timing;
            const auto next =
                observables.has_value()
                    ? wide_eye::game::advance_flock_response_timing(
                          target, tick, active_sheep(snapshot),
                      published_arousal_stimulus(snapshot),
                          observables->connected_component_count,
                          scenario->sheep_behavior.rest_arousal)
                    : std::nullopt;
            if (!next.has_value()) {
                repeatable = false;
                break;
            }
            target = *next;
        }
    }
    if (!check(repeatable && repeated_timing == passing_timing,
               "a_second_run_of_the_same_scenario_folds_to_an_identical_record") ||
        !check(reversed_timing == passing_timing,
               "reversing_the_fixtures_storage_order_folds_to_an_identical_record")) {
        return result;
    }

    // Neither pass allocates on the observation path.
    const SimulationHandle allocation_fixture = make_simulation(*scenario);
    wide_eye::game::FlockResponseTiming allocation_timing{};
    bool allocation_folded = true;
    const std::size_t allocations_before = g_allocation_count;
    for (std::uint64_t tick = 1; tick <= 600; ++tick) {
        allocation_fixture->fixed_update({});
        const auto& snapshot = allocation_fixture->current_snapshot();
        const auto observables = wide_eye::game::compute_flock_observables(
            active_sheep(snapshot), NoChosenNeighbors{},
            kConnectivityDistance, snapshot.dog.position);
        const auto next =
            observables.has_value()
                ? wide_eye::game::advance_flock_response_timing(
                      allocation_timing, tick, active_sheep(snapshot),
                      published_arousal_stimulus(snapshot),
                      observables->connected_component_count, scenario->sheep_behavior.rest_arousal)
                : std::nullopt;
        allocation_folded = allocation_folded && next.has_value();
        if (next.has_value()) {
            allocation_timing = *next;
        }
    }
    result.allocations = g_allocation_count - allocations_before;
    if (!check(allocation_folded && result.allocations == 0,
               "observing_the_flock_does_not_allocate")) {
        return result;
    }

    result.passed = true;
    return result;
}

// The seven published steering terms plus the applied acceleration they are
// summed and bounded into. Every stability measure below is taken per sheep and
// per entry of this list, because "steering is stable" is a claim about each
// influence as well as about the total.
inline constexpr std::size_t kSteeringTermCount = 8;

constexpr std::array<const char*, kSteeringTermCount> kSteeringTermNames{
    "separation",   "attraction", "alignment", "dog_pressure",
    "dog_approach", "dog_facing", "avoidance", "applied"};

// One tick of scripted dog input for the stability run. The dog walks north into
// the flock, holds, and retreats, so every dog term rises and is released inside
// one run instead of being sampled at one fixed geometry, and the arousal ladder
// is walked in both directions.
wide_eye::game::GameplayTickInput stability_input_for_tick(std::uint64_t tick) {
    constexpr std::uint64_t kPressTicks = 250;
    constexpr std::uint64_t kHoldTicks = 350;
    if (tick < kPressTicks) {
        return {.dog_move = wide_eye::game::DogMoveInput{.world_z = -1.0}};
    }
    if (tick < kHoldTicks) {
        // A zero move is not an absent one: the dog motor still runs and
        // decelerates, where an absent input would suspend it and freeze a
        // velocity the approach term would keep reading.
        return {.dog_move = wide_eye::game::DogMoveInput{}};
    }
    return {.dog_move = wide_eye::game::DogMoveInput{.world_z = 1.0}};
}

// A 64-bit FNV-1a digest of one canonical state dump. Recording one digest per
// tick lets a later run be compared against the *whole* published sequence
// rather than against its last tick, without keeping six hundred
// two-kilobyte snapshots alive. `0` means the writer refused the state, which
// the caller checks rather than silently comparing two failures.
std::uint64_t state_digest(const wide_eye::game::GameplaySimulation& simulation) {
    const auto dump = wide_eye::game::gameplay_state_dump_json(simulation);
    if (!dump) {
        return 0;
    }
    std::uint64_t digest = 14695981039346656037ULL;
    for (const char character : dump.text) {
        digest ^= static_cast<std::uint64_t>(static_cast<unsigned char>(character));
        digest *= 1099511628211ULL;
    }
    return digest;
}

// What the randomness-and-stability oracle observed, returned so the run report
// can name the numbers without keeping the fixtures alive in `main`.
struct SteeringStabilityOracle {
    bool passed = false;
    std::size_t scenarios = 0;
    std::uint64_t determinism_ticks = 0;
    std::uint64_t determinism_comparisons = 0;
    std::uint64_t seeds = 0;
    std::uint64_t stability_ticks = 0;
    std::uint64_t sheep_samples = 0;
    std::array<std::uint64_t, kSteeringTermCount> term_active_samples{};
    std::array<std::uint64_t, kSteeringTermCount> term_flaps{};
    std::array<std::uint64_t, kSteeringTermCount> term_flap_runs{};
    std::array<std::uint64_t, kSteeringTermCount> term_flap_allowance{};
    std::uint64_t unexplained_acceleration_samples = 0;
    std::uint64_t unexplained_scale_samples = 0;
    std::uint64_t unexplained_velocity_samples = 0;
    std::uint64_t unexplained_position_samples = 0;
    std::uint64_t combined_bound_breaches = 0;
    std::uint64_t term_bound_breaches = 0;
    std::uint64_t speed_breaches = 0;
    std::uint64_t turn_breaches = 0;
    std::uint64_t arousal_breaches = 0;
    std::uint64_t non_finite_samples = 0;
    std::uint64_t clipped_samples = 0;
    std::uint64_t drop_ahead_samples = 0;
    std::uint64_t occluded_samples = 0;
    std::uint64_t label_changes = 0;
    std::uint64_t minimum_label_dwell = 0;
    std::uint64_t label_round_trips = 0;
    std::uint64_t fast_label_round_trips = 0;
    std::uint64_t label_change_allowance_per_hundred = 0;
    double closest_wall_gap = 0.0;
    double peak_summed_magnitude = 0.0;
    double peak_applied_magnitude = 0.0;
    double peak_speed = 0.0;
    double peak_turn = 0.0;
    double peak_arousal = 0.0;
    std::size_t allocations = 0;
};

// ------------------------------------------------------------ flock scale
// The authoritative flock size is scenario-owned data, so a scenario can carry
// more members than the accepted five. `fifty-sheep-paddock` is the fixture
// that proves it, and this oracle asserts only what a larger flock has to
// satisfy to be authoritative at all: that the fixture is physically placeable,
// that every published record describes a real member, that nothing writes past
// the active count, that the tick allocates nothing, and that the whole
// published sequence is reproducible. It deliberately asserts nothing about the
// motion fifty sheep produce; no owner has reviewed that.
struct FlockScaleOracle {
    bool passed = false;
    std::size_t published_count = 0;
    std::size_t capacity = wide_eye::game::kMaximumGameplaySheepCount;
    std::uint64_t ticks = 0;
    std::size_t allocations = 0;
    double minimum_start_separation = 0.0;
    std::size_t state_dump_bytes = 0;
    std::size_t state_dump_member_records = 0;
};

[[nodiscard]] bool member_records_match_flock(const wide_eye::game::GameplaySnapshot& snapshot) {
    if (snapshot.sheep_count == 0 || snapshot.sheep_count > snapshot.sheep.size()) {
        return false;
    }
    for (std::size_t index = 0; index < snapshot.sheep_count; ++index) {
        const wide_eye::game::SheepState& member = snapshot.sheep[index];
        if (member.id != index + 1 || !std::isfinite(member.position.x) ||
            !std::isfinite(member.position.z) || !std::isfinite(member.velocity.x) ||
            !std::isfinite(member.velocity.z) || !std::isfinite(member.heading_radians) ||
            member.arousal < wide_eye::game::kSheepMinimumArousal ||
            member.arousal > wide_eye::game::kSheepMaximumArousal ||
            member.position.x < wide_eye::game::PaddockCollisionField::kMinimumX ||
            member.position.x > wide_eye::game::PaddockCollisionField::kMaximumX ||
            member.position.z < wide_eye::game::PaddockCollisionField::kMinimumZ ||
            member.position.z > wide_eye::game::PaddockCollisionField::kMaximumZ ||
            snapshot.sheep_social_evidence[index].subject_id != member.id ||
            snapshot.sheep_dog_pressure_evidence[index].subject_id != member.id ||
            snapshot.sheep_collision_evidence[index].subject_id != member.id ||
            snapshot.sheep_avoidance_evidence[index].subject_id != member.id ||
            snapshot.sheep_combined_influence_evidence[index].subject_id != member.id ||
            snapshot.sheep_motion_limit_evidence[index].subject_id != member.id) {
            return false;
        }
    }
    // Nothing past the active count exists. A rule that wrote there would be
    // publishing a member no contract describes, and it would show up here
    // rather than in whatever read it later.
    for (std::size_t index = snapshot.sheep_count; index < snapshot.sheep.size(); ++index) {
        if (!(snapshot.sheep[index] == wide_eye::game::SheepState{}) ||
            !(snapshot.sheep_social_evidence[index] == wide_eye::game::SheepSocialEvidence{}) ||
            !(snapshot.sheep_dog_pressure_evidence[index] ==
              wide_eye::game::SheepDogPressureEvidence{}) ||
            !(snapshot.sheep_collision_evidence[index] ==
              wide_eye::game::SheepCollisionEvidence{}) ||
            !(snapshot.sheep_avoidance_evidence[index] ==
              wide_eye::game::SheepAvoidanceEvidence{}) ||
            !(snapshot.sheep_combined_influence_evidence[index] ==
              wide_eye::game::SheepCombinedInfluenceEvidence{}) ||
            !(snapshot.sheep_motion_limit_evidence[index] ==
              wide_eye::game::SheepMotionLimitEvidence{})) {
            return false;
        }
    }
    return true;
}

FlockScaleOracle run_flock_scale_oracle() {
    constexpr std::uint64_t kScaleTicks = 600;
    FlockScaleOracle result;
    result.ticks = kScaleTicks;

    const ScenarioHandle scenario = named_scenario("fifty-sheep-paddock");
    if (!check(scenario != nullptr, "the_fifty_sheep_scenario_exists")) {
        return result;
    }
    result.published_count = scenario->sheep_count;

    // Placement is asked of the analytic paddock rather than trusted from the
    // constants beside the fixture. A body clear of every shape and inside the
    // bounds is left exactly where it is by a zero displacement; a body that
    // started inside a wall would be pushed out and would say so.
    const wide_eye::game::PaddockCollisionField paddock{scenario->gate_open};
    bool every_member_starts_clear = true;
    double minimum_separation = std::numeric_limits<double>::infinity();
    for (std::size_t index = 0; index < scenario->sheep_count; ++index) {
        const wide_eye::game::SheepState& member = scenario->initial_sheep[index];
        const wide_eye::game::CylinderMoveResult resolved = paddock.resolve_cylinder_move(
            member.position, wide_eye::game::Vec3{}, wide_eye::game::kSheepCollisionRadius);
        every_member_starts_clear = every_member_starts_clear &&
                                    resolved.position == member.position && !resolved.clipped_x &&
                                    !resolved.clipped_z &&
                                    resolved.obstacle == wide_eye::game::PaddockObstacle::none &&
                                    member.id == index + 1;
        for (std::size_t other = index + 1; other < scenario->sheep_count; ++other) {
            minimum_separation = std::min(
                minimum_separation, planar_distance(member, scenario->initial_sheep[other]));
        }
    }
    result.minimum_start_separation = minimum_separation;

    const SimulationHandle simulation = make_simulation(*scenario);
    const SimulationHandle repeat = make_simulation(*scenario);
    const bool published_count_is_scenario_owned =
        simulation->current_snapshot().sheep_count == scenario->sheep_count &&
        simulation->previous_snapshot().sheep_count == scenario->sheep_count;
    bool every_tick_is_coherent = member_records_match_flock(simulation->current_snapshot());
    bool repeats_exactly = true;

    // The allocation window opens after construction, so it measures the fixed
    // tick rather than the one-time setup the scenario paid for.
    const std::size_t allocations_before = g_allocation_count;
    for (std::uint64_t tick = 0; tick < kScaleTicks; ++tick) {
        simulation->fixed_update(input_for_tick(tick));
        every_tick_is_coherent =
            every_tick_is_coherent && member_records_match_flock(simulation->current_snapshot());
    }
    result.allocations = g_allocation_count - allocations_before;

    for (std::uint64_t tick = 0; tick < kScaleTicks; ++tick) {
        repeat->fixed_update(input_for_tick(tick));
    }
    repeats_exactly = repeat->current_snapshot() == simulation->current_snapshot() &&
                      repeat->previous_snapshot() == simulation->previous_snapshot();

    const auto dump = wide_eye::game::gameplay_state_dump_json(*simulation);
    if (dump) {
        result.state_dump_bytes = dump.text.size();
        std::size_t records = 0;
        for (std::size_t at = dump.text.find("\"temperament\":"); at != std::string::npos;
             at = dump.text.find("\"temperament\":", at + 1)) {
            ++records;
        }
        result.state_dump_member_records = records;
    }

    repeat->restart();
    const bool restart_republishes_the_fixture =
        repeat->current_snapshot().sheep_count == scenario->sheep_count &&
        repeat->current_snapshot().sheep == scenario->initial_sheep &&
        repeat->current_snapshot() == repeat->previous_snapshot();

    result.passed =
        check(result.published_count == 50, "the_scale_fixture_publishes_fifty_members") &&
        check(result.published_count <= wide_eye::game::kMaximumGameplaySheepCount &&
                  wide_eye::game::kMaximumGameplaySheepCount <=
                      wide_eye::game::SheepSpatialGrid::kMaximumMemberCount,
              "the_capacity_fits_the_published_buffers_and_the_spatial_grid") &&
        check(every_member_starts_clear,
              "every_member_starts_clear_of_every_paddock_shape_and_bound") &&
        check(minimum_separation >= 2.0 * wide_eye::game::kSheepCollisionRadius,
              "no_member_starts_inside_another") &&
        check(published_count_is_scenario_owned, "the_published_count_is_the_scenario_count") &&
        check(every_tick_is_coherent,
              "every_published_record_describes_a_member_and_nothing_past_the_count") &&
        check(result.allocations == 0, "fifty_members_tick_without_heap_allocation") &&
        check(repeats_exactly, "the_fifty_member_sequence_is_reproducible") &&
        check(restart_republishes_the_fixture, "restart_republishes_the_fifty_member_fixture") &&
        check(dump && result.state_dump_member_records == 2 * result.published_count,
              "the_state_dump_writes_one_record_per_active_member");
    return result;
}

SteeringStabilityOracle run_steering_stability_oracle() {
    SteeringStabilityOracle result;
    // Randomness, stability, and attribution. The roadmap item asks for evidence
    // that randomness never masks unstable or unexplained steering. The honest
    // first half of that is that **there is no randomness**: the scenario
    // contract carries a seed, and every accepted rule is a pure function of the
    // immutable prior state, so nothing consumes it. This oracle pins that
    // property instead of testing a random system, and then proves the steering
    // is stable and attributable on its own terms, so that the day a rule does
    // start consuming the seed, an existing check fails rather than a behavior
    // quietly becoming irreproducible.
    //
    // No randomness, jitter, or noise is added here. Adding one to test it would
    // be a design decision nobody has approved.
    using wide_eye::game::GameplayScenarioDefinition;
    using wide_eye::game::GameplayScenarioId;
    using wide_eye::game::GameplaySnapshot;
    using wide_eye::game::SheepBehaviorState;
    using wide_eye::game::Vec3;
    constexpr double kFixedDelta = wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    constexpr std::uint64_t kDeterminismTicks = 300;
    constexpr std::uint64_t kStabilityTicks = 600;
    // Accelerations here are of order one, so a term below this magnitude is
    // rounding residue rather than an influence, and asking whether it "reversed"
    // would be asking about noise.
    constexpr double kLiveTermMagnitude = 1.0e-6;
    // Every bound below is exact arithmetic on the fixture's own values, but a
    // magnitude recovered with `hypot` from a vector that was scaled to that
    // magnitude can land one unit in the last place above it.
    constexpr double kBoundTolerance = 1.0e-9;

    // A named scenario the game already runs is the only honest determinism
    // sample: a term that consumed hidden entropy would only have to do it in
    // one of them, so every scenario in the table is swept rather than only the
    // fixture this outcome adds.
    const ScenarioHandle diagnostic =
        named_scenario("sheep-all-influences-diagnostic");
    if (!check(diagnostic != nullptr, "all_influences_diagnostic_scenario_exists")) {
        return result;
    }
    if (!check(diagnostic->version == 1 && diagnostic->seed == 0,
               "all_influences_diagnostic_is_version_one_seed_zero") ||
        !check(diagnostic->sheep_separation.enabled && diagnostic->sheep_attraction.enabled &&
                   diagnostic->sheep_alignment.enabled && diagnostic->sheep_dog_pressure.enabled &&
                   diagnostic->sheep_dog_approach.enabled && diagnostic->sheep_dog_facing.enabled &&
                   diagnostic->sheep_dog_line_of_sight.enabled &&
                   diagnostic->sheep_temperament.enabled && diagnostic->sheep_avoidance.enabled &&
                   diagnostic->sheep_combined_influence.enabled &&
                   diagnostic->sheep_motion_limit.enabled && diagnostic->sheep_behavior.enabled,
               "all_influences_diagnostic_enables_every_rule")) {
        return result;
    }

    result.determinism_ticks = kDeterminismTicks;
    constexpr std::uint16_t kScenarioIdCeiling = 64;
    for (std::uint16_t raw = 0; raw < kScenarioIdCeiling; ++raw) {
        const auto id = static_cast<GameplayScenarioId>(raw);
        const std::string_view name = wide_eye::game::gameplay_scenario_name(id);
        if (name == "unknown") {
            continue;
        }
        const ScenarioHandle definition = named_scenario(name);
        if (!check(definition != nullptr, "every_named_scenario_resolves")) {
            return result;
        }
        ++result.scenarios;

        // Two simulations of one scenario, advanced tick by tick in lockstep so
        // they are interleaved in time rather than run one after the other, and
        // compared on every tick rather than at the end: a divergence that
        // cancelled itself would otherwise be invisible.
        const SimulationHandle left = make_simulation(*definition);
        const SimulationHandle right = make_simulation(*definition);
        for (std::uint64_t tick = 0; tick < kDeterminismTicks; ++tick) {
            const auto input = input_for_tick(left->current_snapshot().tick);
            left->fixed_update(input);
            right->fixed_update(input);
            ++result.determinism_comparisons;
            if (!check(left->current_snapshot() == right->current_snapshot() &&
                           left->previous_snapshot() == right->previous_snapshot(),
                       "two_simulations_of_one_scenario_publish_one_sequence")) {
                return result;
            }
        }

        // The canonical dump as well as the snapshot: `operator==` on a double
        // cannot see the sign bit of a zero, and the dump is the text a reviewer
        // actually compares.
        const auto left_text = wide_eye::game::gameplay_state_dump_json(*left);
        const auto right_text = wide_eye::game::gameplay_state_dump_json(*right);
        if (!check(left_text && right_text && left_text.text == right_text.text,
                   "repeated_scenario_state_dumps_are_byte_identical")) {
            return result;
        }

        // A restarted simulation is the same starting contract in an object that
        // has already run, which is the case a fresh construction cannot cover.
        left->restart();
        for (std::uint64_t tick = 0; tick < kDeterminismTicks; ++tick) {
            left->fixed_update(input_for_tick(left->current_snapshot().tick));
        }
        if (!check(left->current_snapshot() == right->current_snapshot(),
                   "restarted_simulation_reproduces_the_same_sequence")) {
            return result;
        }
    }
    // The sweep enumerates the ID space rather than a list, so it covers every
    // named scenario by construction; this floor only guards against the ceiling
    // above being outgrown or a scenario disappearing, and a new scenario is
    // expected to raise it rather than to fail it.
    if (!check(result.scenarios >= 30, "every_named_scenario_was_swept")) {
        return result;
    }

    // The seed is carried by the scenario contract and consumed by nothing. That
    // is today's truth, and it is recorded as an equality so it cannot change
    // silently: the first rule that reads the seed makes this check fail. When
    // that day comes the answer is to replace this with a statistical check on
    // the seeded distribution, not to delete it.
    result.seeds = 3;
    constexpr std::array<std::uint64_t, 3> kSeeds{0, 1, 0x9E3779B97F4A7C15ULL};
    GameplayScenarioDefinition seed_variant = *diagnostic;
    seed_variant.seed = kSeeds[1];
    GameplayScenarioDefinition other_seed_variant = *diagnostic;
    other_seed_variant.seed = kSeeds[2];
    {
        const SimulationHandle zero_seed = make_simulation(*diagnostic);
        const SimulationHandle one_seed = make_simulation(seed_variant);
        const SimulationHandle far_seed = make_simulation(other_seed_variant);
        if (!check(zero_seed->scenario().seed == kSeeds[0] &&
                       one_seed->scenario().seed == kSeeds[1] &&
                       far_seed->scenario().seed == kSeeds[2],
                   "seed_variants_really_carry_different_seeds")) {
            return result;
        }
        for (std::uint64_t tick = 0; tick < kStabilityTicks; ++tick) {
            const auto input = stability_input_for_tick(tick);
            zero_seed->fixed_update(input);
            one_seed->fixed_update(input);
            far_seed->fixed_update(input);
            if (!check(zero_seed->current_snapshot() == one_seed->current_snapshot() &&
                           zero_seed->current_snapshot() == far_seed->current_snapshot(),
                       "no_accepted_rule_consumes_the_scenario_seed")) {
                return result;
            }
        }
    }

    // Authoritative state cannot depend on render cadence, which is the only
    // wall-clock quantity anywhere near the fixed tick: the same authoritative
    // ticks scheduled from a hundred 10 ms frames and from ten 100 ms frames
    // must publish one state.
    {
        using namespace std::chrono_literals;
        std::array<std::chrono::nanoseconds, 100> fine_frames{};
        fine_frames.fill(10ms);
        std::array<std::chrono::nanoseconds, 10> coarse_frames{};
        coarse_frames.fill(100ms);
        const CadenceHandle fine = run_cadence(*diagnostic, fine_frames);
        const CadenceHandle coarse = run_cadence(*diagnostic, coarse_frames);
        if (!check(fine->scheduled_ticks == 60 && fine->snapshot.tick == 60 &&
                       fine->snapshot == coarse->snapshot,
                   "all_influences_state_ignores_render_cadence")) {
            return result;
        }
    }

    // The stability and attribution run. One long run of the maximal fixture,
    // measured on every tick and for every sheep.
    result.stability_ticks = kStabilityTicks;
    const auto& separation = diagnostic->sheep_separation;
    const auto& attraction = diagnostic->sheep_attraction;
    const auto& alignment = diagnostic->sheep_alignment;
    const auto& dog_pressure = diagnostic->sheep_dog_pressure;
    const auto& dog_approach = diagnostic->sheep_dog_approach;
    const auto& dog_facing = diagnostic->sheep_dog_facing;
    const auto& avoidance = diagnostic->sheep_avoidance;
    const auto& combined = diagnostic->sheep_combined_influence;
    const auto& motion_limit = diagnostic->sheep_motion_limit;
    const auto& behavior = diagnostic->sheep_behavior;
    const double turn_budget = motion_limit.maximum_turn_rate_radians_per_second * kFixedDelta;
    const double arousal_step =
        std::max(behavior.rise_rate_per_second, behavior.recovery_rate_per_second) * kFixedDelta;

    std::array<std::array<Vec3, kSteeringTermCount>, wide_eye::game::kDefaultGameplaySheepCount>
        previous_terms{};
    std::array<std::array<bool, kSteeringTermCount>, wide_eye::game::kDefaultGameplaySheepCount>
        previous_term_live{};
    std::array<std::array<std::uint64_t, kSteeringTermCount>,
               wide_eye::game::kDefaultGameplaySheepCount>
        flaps{};
    std::array<std::array<std::uint64_t, kSteeringTermCount>,
               wide_eye::game::kDefaultGameplaySheepCount>
        flap_run{};
    std::array<std::array<std::uint64_t, kSteeringTermCount>,
               wide_eye::game::kDefaultGameplaySheepCount>
        longest_flap_run{};
    std::array<std::uint64_t, wide_eye::game::kDefaultGameplaySheepCount> last_label_change{};
    std::array<SheepBehaviorState, wide_eye::game::kDefaultGameplaySheepCount> label_before_change{};
    std::array<std::uint64_t, wide_eye::game::kDefaultGameplaySheepCount> sheep_label_changes{};
    std::array<double, wide_eye::game::kDefaultGameplaySheepCount> minimum_z{};
    std::array<std::uint64_t, kStabilityTicks> digests{};
    result.minimum_label_dwell = kStabilityTicks;
    result.closest_wall_gap = std::numeric_limits<double>::infinity();

    const SimulationHandle stability = make_simulation(*diagnostic);
    for (std::size_t index = 0; index < wide_eye::game::kDefaultGameplaySheepCount; ++index) {
        minimum_z[index] = diagnostic->initial_sheep[index].position.z;
    }

    for (std::uint64_t tick = 1; tick <= kStabilityTicks; ++tick) {
        stability->fixed_update(stability_input_for_tick(tick - 1));
        digests[tick - 1] = state_digest(*stability);
        if (!check(digests[tick - 1] != 0, "every_stability_tick_publishes_a_valid_state")) {
            return result;
        }
        const GameplaySnapshot& previous = stability->previous_snapshot();
        const GameplaySnapshot& current = stability->current_snapshot();
        for (std::size_t index = 0; index < current.sheep_count; ++index) {
            const auto& prior = previous.sheep[index];
            const auto& next = current.sheep[index];
            const auto& social = current.sheep_social_evidence[index];
            const auto& dog = current.sheep_dog_pressure_evidence[index];
            const auto& avoid = current.sheep_avoidance_evidence[index];
            const auto& bound = current.sheep_combined_influence_evidence[index];
            const auto& motion = current.sheep_motion_limit_evidence[index];
            const auto& collision = current.sheep_collision_evidence[index];
            ++result.sheep_samples;

            // Attribution: every applied acceleration is exactly the published
            // per-term vectors, summed in the published order and scaled by the
            // published factor. This is the check that makes unexplained
            // steering detectable at all — an influence that acted without
            // publishing itself would break this equality on the tick it acted.
            const Vec3 summed = summed_steering_terms(social, dog, avoid);
            const Vec3 expected_applied = bounded_terms(summed, bound);
            const double summed_magnitude = std::hypot(summed.x, summed.z);
            const double expected_scale =
                combined.enabled && summed_magnitude > combined.maximum_acceleration
                    ? combined.maximum_acceleration / summed_magnitude
                    : 1.0;
            if (!bound.bound_evaluated || bound.applied_acceleration.x != expected_applied.x ||
                bound.applied_acceleration.z != expected_applied.z) {
                ++result.unexplained_acceleration_samples;
            }
            if (bound.summed_acceleration_magnitude != summed_magnitude ||
                bound.applied_scale != expected_scale) {
                ++result.unexplained_scale_samples;
            }

            // Attribution of the motion itself: the published velocity is the
            // prior velocity plus the applied acceleration, scaled by the
            // published speed factor, with a refused axis cleared. Nothing else
            // may move a sheep.
            const double speed_scale = motion.limit_evaluated ? motion.applied_speed_scale : 1.0;
            const double expected_velocity_x =
                collision.clipped_x
                    ? 0.0
                    : (prior.velocity.x + bound.applied_acceleration.x * kFixedDelta) * speed_scale;
            const double expected_velocity_z =
                collision.clipped_z
                    ? 0.0
                    : (prior.velocity.z + bound.applied_acceleration.z * kFixedDelta) * speed_scale;
            if (next.velocity.x != expected_velocity_x || next.velocity.z != expected_velocity_z) {
                ++result.unexplained_velocity_samples;
            }
            if (!collision.clipped_x &&
                next.position.x != prior.position.x + next.velocity.x * kFixedDelta) {
                ++result.unexplained_position_samples;
            }
            if (!collision.clipped_z &&
                next.position.z != prior.position.z + next.velocity.z * kFixedDelta) {
                ++result.unexplained_position_samples;
            }

            const std::array<Vec3, kSteeringTermCount> terms{
                social.separation_acceleration, social.attraction_acceleration,
                social.alignment_acceleration,  dog.pressure_acceleration,
                dog.approach_acceleration,      dog.facing_acceleration,
                avoid.avoidance_acceleration,   bound.applied_acceleration};
            // The dog terms are scaled by the sheep's own temperament, so the
            // maximum each one may reach is the configured maximum times the
            // factor the sheep published this tick.
            const double response = dog.temperament_response_scale;
            const std::array<double, kSteeringTermCount> maxima{
                separation.maximum_acceleration,
                attraction.maximum_acceleration,
                alignment.maximum_acceleration,
                dog_pressure.maximum_acceleration * response,
                dog_approach.maximum_acceleration * response,
                dog_facing.maximum_acceleration * response,
                avoidance.maximum_acceleration,
                combined.maximum_acceleration};

            for (std::size_t term = 0; term < kSteeringTermCount; ++term) {
                const double magnitude = std::hypot(terms[term].x, terms[term].z);
                if (!std::isfinite(magnitude)) {
                    ++result.non_finite_samples;
                    continue;
                }
                if (magnitude > maxima[term] + kBoundTolerance) {
                    if (term + 1 == kSteeringTermCount) {
                        ++result.combined_bound_breaches;
                    } else {
                        ++result.term_bound_breaches;
                    }
                }
                const bool live = magnitude > kLiveTermMagnitude;
                if (live) {
                    ++result.term_active_samples[term];
                }
                // A **flap** is one tick on which a term did not settle against
                // the tick before it: either it switched itself on or off, or it
                // reversed direction while both ticks were real influences. Both
                // halves matter, because a term that alternates between its
                // maximum and exactly nothing never reverses a direction and
                // would otherwise be invisible. A term that alternated on every
                // tick scores one hundred flaps per hundred ticks; a term that
                // changed sides once as the dog crossed it scores one. The
                // longest run of consecutive flaps separates the two even more
                // sharply than the count does.
                bool unsettled = false;
                if (tick > 1) {
                    if (live != previous_term_live[index][term]) {
                        unsettled = true;
                    } else if (live) {
                        const double dot = terms[term].x * previous_terms[index][term].x +
                                           terms[term].z * previous_terms[index][term].z;
                        unsettled = dot < 0.0;
                    }
                }
                if (unsettled) {
                    ++flaps[index][term];
                    ++flap_run[index][term];
                    longest_flap_run[index][term] =
                        std::max(longest_flap_run[index][term], flap_run[index][term]);
                } else {
                    flap_run[index][term] = 0;
                }
                previous_terms[index][term] = terms[term];
                previous_term_live[index][term] = live;
            }

            const double speed = std::hypot(next.velocity.x, next.velocity.z);
            if (!std::isfinite(next.position.x) || !std::isfinite(next.position.z) ||
                !std::isfinite(speed) || !std::isfinite(next.arousal) ||
                !std::isfinite(next.heading_radians)) {
                ++result.non_finite_samples;
            }
            if (motion.limit_evaluated &&
                motion.applied_speed > motion_limit.maximum_speed + kBoundTolerance) {
                ++result.speed_breaches;
            }
            if (speed > motion_limit.maximum_speed + kBoundTolerance) {
                ++result.speed_breaches;
            }
            if (std::abs(motion.heading_change_radians) > turn_budget + kBoundTolerance) {
                ++result.turn_breaches;
            }
            if (next.arousal < wide_eye::game::kSheepMinimumArousal ||
                next.arousal > wide_eye::game::kSheepMaximumArousal ||
                std::abs(next.arousal - prior.arousal) > arousal_step + kBoundTolerance ||
                !wide_eye::game::is_known_sheep_behavior(next.behavior)) {
                ++result.arousal_breaches;
            }
            if (collision.clipped_x || collision.clipped_z) {
                ++result.clipped_samples;
            }
            if (avoid.drop_ahead) {
                ++result.drop_ahead_samples;
            }
            if (dog.dog_line_of_sight_blocked) {
                ++result.occluded_samples;
            }
            if (next.behavior != prior.behavior) {
                // A **round trip** is the label going back to the one it held
                // before its last change, which is what oscillation looks like.
                // Two changes on consecutive ticks are not: walking down the
                // ladder from alert through recovering to settled is a monotone
                // descent, and calling that a flap would make the measure fire
                // on the rule working. A round trip inside the window below is
                // faster than the recovery rate can carry arousal across the
                // narrowest band, so it cannot be a genuine second response.
                constexpr std::uint64_t kLabelRoundTripWindow = 32;
                if (sheep_label_changes[index] > 0 && next.behavior == label_before_change[index]) {
                    ++result.label_round_trips;
                    if (tick - last_label_change[index] < kLabelRoundTripWindow) {
                        ++result.fast_label_round_trips;
                    }
                }
                ++result.label_changes;
                ++sheep_label_changes[index];
                result.minimum_label_dwell =
                    std::min(result.minimum_label_dwell, tick - last_label_change[index]);
                label_before_change[index] = prior.behavior;
                last_label_change[index] = tick;
            }

            minimum_z[index] = std::min(minimum_z[index], next.position.z);
            result.peak_summed_magnitude = std::max(result.peak_summed_magnitude, summed_magnitude);
            result.peak_applied_magnitude =
                std::max(result.peak_applied_magnitude,
                         std::hypot(bound.applied_acceleration.x, bound.applied_acceleration.z));
            result.peak_speed = std::max(result.peak_speed, speed);
            result.peak_turn = std::max(result.peak_turn, std::abs(motion.heading_change_radians));
            result.peak_arousal = std::max(result.peak_arousal, next.arousal);
        }
    }

    for (std::size_t index = 0; index < wide_eye::game::kDefaultGameplaySheepCount; ++index) {
        for (std::size_t term = 0; term < kSteeringTermCount; ++term) {
            result.term_flaps[term] = std::max(result.term_flaps[term], flaps[index][term]);
            result.term_flap_runs[term] =
                std::max(result.term_flap_runs[term], longest_flap_run[index][term]);
        }
        // The closed wall line plus one sheep body radius is where the analytic
        // paddock stops a sheep that arrives from open ground. A sheep north of
        // it would mean the barrier passed a body, which is the QA-001 failure
        // mode rather than a steering result.
        result.closest_wall_gap = std::min(result.closest_wall_gap, minimum_z[index] - 16.0);
    }

    // Stability bounds. A term that alternated on every tick would score one
    // hundred flaps per hundred ticks. Five per hundred with no run longer than
    // four separates "changed sides as the dog crossed" from "flapping" and now
    // applies equally to avoidance and the sum that carries it. QA-005 removed
    // their former binary-response exception: this run measures avoidance at
    // `0.67` flaps per hundred with a run of `1`, and the applied sum at `1.5`
    // with a run of `2`.
    constexpr std::uint64_t kContinuousFlapAllowance = 5;
    constexpr std::uint64_t kContinuousFlapRunAllowance = 4;
    result.term_flap_allowance = {kContinuousFlapAllowance, kContinuousFlapAllowance,
                                  kContinuousFlapAllowance, kContinuousFlapAllowance,
                                  kContinuousFlapAllowance, kContinuousFlapAllowance,
                                  kContinuousFlapAllowance, kContinuousFlapAllowance};
    result.label_change_allowance_per_hundred = 5;
    bool flaps_bounded = true;
    for (std::size_t term = 0; term < kSteeringTermCount; ++term) {
        flaps_bounded =
            flaps_bounded &&
            result.term_flaps[term] * 100 <= result.term_flap_allowance[term] * kStabilityTicks &&
            result.term_flap_runs[term] <= kContinuousFlapRunAllowance;
    }

    if (!check(result.unexplained_acceleration_samples == 0,
               "every_applied_acceleration_is_exactly_the_published_terms") ||
        !check(result.unexplained_scale_samples == 0,
               "every_published_bound_scale_explains_its_own_arithmetic") ||
        !check(result.unexplained_velocity_samples == 0,
               "every_published_velocity_is_explained_by_published_evidence") ||
        !check(result.unexplained_position_samples == 0,
               "every_unrefused_position_is_the_integrated_one") ||
        !check(result.combined_bound_breaches == 0, "the_combined_bound_holds_on_every_tick") ||
        !check(result.term_bound_breaches == 0, "every_term_holds_its_own_maximum_every_tick") ||
        !check(result.speed_breaches == 0, "the_speed_limit_holds_on_every_tick") ||
        !check(result.turn_breaches == 0, "the_turn_budget_holds_on_every_tick") ||
        !check(result.arousal_breaches == 0, "arousal_stays_bounded_and_rate_limited") ||
        !check(result.non_finite_samples == 0, "no_published_steering_value_is_non_finite")) {
        return result;
    }
    if (!check(flaps_bounded, "no_steering_term_flaps_beyond_its_recorded_allowance") ||
        !check(result.label_changes * 100 <=
                   result.label_change_allowance_per_hundred * kStabilityTicks,
               "the_behavior_label_does_not_oscillate") ||
        !check(result.fast_label_round_trips == 0,
               "no_behavior_label_returns_inside_its_hysteresis_dwell")) {
        return result;
    }
    bool every_term_acted = true;
    for (std::size_t term = 0; term < kSteeringTermCount; ++term) {
        every_term_acted = every_term_acted && result.term_active_samples[term] > 0;
    }
    if (!check(every_term_acted, "every_steering_term_acted_during_the_run") ||
        !check(result.clipped_samples > 0, "the_run_pressed_the_flock_into_the_wall_line") ||
        !check(result.closest_wall_gap >= 0.5 - 1.0e-12, "no_sheep_crossed_the_closed_wall_line")) {
        return result;
    }

    // The same published sequence from an object constructed later, in memory
    // filled with a poison pattern rather than zeroes, and at a different
    // address. Nothing outside the scenario contract — construction time, object
    // identity, or whatever the allocator left in the storage — may reach the
    // published state.
    {
        // The storage is a byte vector rather than an array `new` so that the
        // allocation and the deallocation both go through this file's own scalar
        // `operator new`/`operator delete`, and it is filled with a pattern
        // rather than left zeroed so that an uninitialized read would produce a
        // different answer here than in an ordinary heap fixture.
        static_assert(alignof(wide_eye::game::GameplaySimulation) <= alignof(std::max_align_t),
                      "the poisoned arena must be aligned for the simulation it holds");
        std::vector<std::byte> arena(sizeof(wide_eye::game::GameplaySimulation), std::byte{0xAB});
        auto* poisoned = new (arena.data()) wide_eye::game::GameplaySimulation{*diagnostic};
        bool poisoned_matches = true;
        for (std::uint64_t tick = 1; tick <= kStabilityTicks; ++tick) {
            poisoned->fixed_update(stability_input_for_tick(tick - 1));
            poisoned_matches = poisoned_matches && state_digest(*poisoned) == digests[tick - 1];
        }
        const bool poisoned_final = poisoned->current_snapshot() == stability->current_snapshot();
        poisoned->~GameplaySimulation();
        if (!check(poisoned_matches && poisoned_final,
                   "a_later_simulation_in_poisoned_storage_publishes_the_same_sequence")) {
            return result;
        }
    }

    // The same published sequence after a restart, compared on every tick rather
    // than only at the last one. The canonical dump is deliberately *not* the
    // comparison here: it reports `restart_count`, which is the one thing a
    // restarted object is supposed to publish differently, so the restarted run
    // is compared against a fresh simulation's snapshots instead.
    {
        stability->restart();
        const SimulationHandle restart_reference = make_simulation(*diagnostic);
        bool restart_matches =
            stability->current_snapshot() == restart_reference->current_snapshot();
        for (std::uint64_t tick = 1; tick <= kStabilityTicks; ++tick) {
            const auto input = stability_input_for_tick(tick - 1);
            stability->fixed_update(input);
            restart_reference->fixed_update(input);
            restart_matches = restart_matches && stability->current_snapshot() ==
                                                     restart_reference->current_snapshot();
        }
        if (!check(restart_matches && stability->restart_count() == 1 &&
                       restart_reference->restart_count() == 0,
                   "a_restarted_run_publishes_the_same_sequence")) {
            return result;
        }
    }

    // Storage order is not state. The reversed fixture carries the same five
    // sheep in the opposite buffer order, so a rule that depended on iteration
    // order — or on one sheep's address relative to another's — would publish a
    // different result for the same ID.
    {
        GameplayScenarioDefinition reversed_scenario = *diagnostic;
        std::reverse(reversed_scenario.initial_sheep.begin(),
                     reversed_scenario.initial_sheep.begin() +
                         static_cast<std::ptrdiff_t>(reversed_scenario.sheep_count));
        const SimulationHandle forward = make_simulation(*diagnostic);
        const SimulationHandle reversed = make_simulation(reversed_scenario);
        bool reversed_matches = true;
        for (std::uint64_t tick = 1; tick <= kStabilityTicks && reversed_matches; ++tick) {
            const auto input = stability_input_for_tick(tick - 1);
            forward->fixed_update(input);
            reversed->fixed_update(input);
            const auto& forward_snapshot = forward->current_snapshot();
            const auto& reversed_snapshot = reversed->current_snapshot();
            reversed_matches = reversed_matches && forward_snapshot.dog == reversed_snapshot.dog;
            for (const auto& member :
                 active(forward_snapshot.sheep, forward_snapshot.sheep_count)) {
                reversed_matches =
                    reversed_matches &&
                    sheep_with_id(reversed_snapshot.sheep, member.id) == member &&
                    evidence_with_id(reversed_snapshot.sheep_social_evidence, member.id) ==
                        evidence_with_id(forward_snapshot.sheep_social_evidence, member.id) &&
                    evidence_with_id(reversed_snapshot.sheep_dog_pressure_evidence, member.id) ==
                        evidence_with_id(forward_snapshot.sheep_dog_pressure_evidence, member.id) &&
                    evidence_with_id(reversed_snapshot.sheep_collision_evidence, member.id) ==
                        evidence_with_id(forward_snapshot.sheep_collision_evidence, member.id) &&
                    evidence_with_id(reversed_snapshot.sheep_avoidance_evidence, member.id) ==
                        evidence_with_id(forward_snapshot.sheep_avoidance_evidence, member.id) &&
                    evidence_with_id(reversed_snapshot.sheep_combined_influence_evidence,
                                     member.id) ==
                        evidence_with_id(forward_snapshot.sheep_combined_influence_evidence,
                                         member.id) &&
                    evidence_with_id(reversed_snapshot.sheep_motion_limit_evidence, member.id) ==
                        evidence_with_id(forward_snapshot.sheep_motion_limit_evidence, member.id);
            }
        }
        if (!check(reversed_matches, "reversed_storage_publishes_the_same_result_per_id")) {
            return result;
        }
    }

    const SimulationHandle allocation_probe = make_simulation(*diagnostic);
    const std::size_t allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_probe->fixed_update({});
    }
    result.allocations = g_allocation_count - allocations_before;
    if (!check(result.allocations == 0, "all_influences_fixed_updates_do_not_allocate")) {
        return result;
    }

    result.passed = true;
    return result;
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
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepAvoidanceEvidence>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepAvoidanceEvidenceBuffer>);
    static_assert(std::is_trivially_copyable_v<wide_eye::game::SheepCombinedInfluenceEvidence>);
    static_assert(
        std::is_trivially_copyable_v<wide_eye::game::SheepCombinedInfluenceEvidenceBuffer>);

    const ScenarioHandle scenario = named_scenario("paddock-start");
    if (!check(scenario != nullptr, "scenario_available") ||
        !check(wide_eye::game::GameplaySimulation::kTicksPerSecond ==
                   wide_eye::core::FixedStepAccumulator::ticks_per_second,
               "single_fixed_rate_definition")) {
        return EXIT_FAILURE;
    }

    std::array<std::chrono::nanoseconds, 100> fine_frames{};
    fine_frames.fill(10ms);
    std::array<std::chrono::nanoseconds, 10> coarse_frames{};
    coarse_frames.fill(100ms);

    const CadenceHandle fine = run_cadence(*scenario, fine_frames);
    const CadenceHandle coarse = run_cadence(*scenario, coarse_frames);
    if (!check(fine->scheduled_ticks == 60 && coarse->scheduled_ticks == 60,
               "one_second_schedules_sixty_ticks") ||
        !check(fine->snapshot.tick == 60 && coarse->snapshot.tick == 60,
               "gameplay_consumes_every_scheduled_tick") ||
        !check(fine->snapshot == coarse->snapshot, "authoritative_state_ignores_render_cadence")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle simulation = make_simulation(*scenario);
    const auto initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(simulation->current_snapshot());
    const wide_eye::game::GameplaySnapshot& initial = *initial_holder;
    bool initial_sheep_valid =
        initial.sheep_count == wide_eye::game::kDefaultGameplaySheepCount;
    for (std::size_t index = 0; index < initial.sheep_count; ++index) {
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

    simulation->fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_z = -1.0}});
    if (!check(simulation->previous_snapshot() == initial, "previous_snapshot_is_prior_tick") ||
        !check(simulation->current_snapshot().tick == 1 &&
                   simulation->current_snapshot().dog != initial.dog,
               "fixed_update_publishes_current_tick") ||
        !check(simulation->previous_snapshot().sheep == initial.sheep &&
                   simulation->current_snapshot().sheep == initial.sheep,
               "sheep_next_state_reads_immutable_prior") ||
        !check(interpolated_dog_equals(*simulation, 0.0, initial.dog) &&
                   interpolated_dog_equals(*simulation, 1.0,
                                           simulation->current_snapshot().dog),
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

    const auto dog_before_suspension = simulation->current_snapshot().dog;
    simulation->fixed_update({});
    if (!check(simulation->current_snapshot().tick == 2,
               "suspended_motor_still_advances_authoritative_tick") ||
        !check(simulation->current_snapshot().dog == dog_before_suspension,
               "suspended_motor_preserves_dog_state")) {
        return EXIT_FAILURE;
    }

    simulation->restart();
    if (!check(simulation->current_snapshot() == initial &&
                   simulation->previous_snapshot() == initial,
               "restart_restores_coherent_snapshots") ||
        !check(simulation->restart_count() == 1, "restart_count_preserved")) {
        return EXIT_FAILURE;
    }

    const std::size_t allocations_before_updates = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        simulation->fixed_update(input_for_tick(tick));
    }
    const std::size_t steady_state_allocations = g_allocation_count - allocations_before_updates;
    if (!check(steady_state_allocations == 0, "fixed_updates_do_not_allocate_per_agent")) {
        return EXIT_FAILURE;
    }
    simulation->restart();

    const ScenarioHandle motion_scenario = named_scenario("presentation-motion");
    if (!check(motion_scenario != nullptr &&
                   motion_scenario->sheep_fixture ==
                       wide_eye::game::SheepFixture::scripted_presentation_motion,
               "named_presentation_motion_fixture_available")) {
        return EXIT_FAILURE;
    }
    const SimulationHandle motion_a = make_simulation(*motion_scenario);
    const SimulationHandle motion_b = make_simulation(*motion_scenario);
    const auto motion_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(motion_a->current_snapshot());
    const wide_eye::game::GameplaySnapshot& motion_initial = *motion_initial_holder;
    for (std::uint64_t tick = 0; tick < 61; ++tick) {
        motion_a->fixed_update({});
        motion_b->fixed_update({});
    }
    const auto motion_mid_turn_holder = interpolated_on_heap(*motion_a, 0.5);
    const wide_eye::game::GameplaySnapshot& motion_mid_turn = *motion_mid_turn_holder;
    bool all_sheep_scripted = true;
    for (std::size_t index = 0; index < motion_mid_turn.sheep_count; ++index) {
        const auto& sheep = motion_mid_turn.sheep[index];
        all_sheep_scripted = all_sheep_scripted && sheep.id == index + 1 &&
                             sheep.position.z < motion_initial.sheep[index].position.z &&
                             sheep.position.x > motion_initial.sheep[index].position.x &&
                             sheep.behavior == wide_eye::game::SheepBehaviorState::settled &&
                             sheep.arousal == 0.0 && sheep.grounded;
    }
    if (!check(motion_a->current_snapshot() == motion_b->current_snapshot(),
               "presentation_motion_repeats_exactly") ||
        !check(all_sheep_scripted, "presentation_motion_moves_all_without_behavior") ||
        !check(std::abs(motion_mid_turn.sheep.front().heading_radians -
                        0.25 * 3.14159265358979323846) < 1.0e-12,
               "presentation_motion_interpolates_turn") ||
        !check(motion_a->previous_snapshot().sheep.front().position.x <
                   motion_a->current_snapshot().sheep.front().position.x,
               "presentation_motion_publishes_prior_and_current")) {
        return EXIT_FAILURE;
    }
    motion_a->restart();
    if (!check(motion_a->current_snapshot() == motion_initial &&
                   motion_a->previous_snapshot() == motion_initial,
               "presentation_motion_restart_is_exact")) {
        return EXIT_FAILURE;
    }

    const ScenarioHandle separation_scenario =
        named_scenario("sheep-only-separation");
    if (!check(separation_scenario != nullptr &&
                   separation_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_only_separation &&
                   separation_scenario->sheep_fixture ==
                       wide_eye::game::SheepFixture::local_social_response &&
                   separation_scenario->sheep_separation.enabled &&
                   !separation_scenario->sheep_attraction.enabled,
               "named_sheep_only_separation_fixture_available")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle separation = make_simulation(*separation_scenario);
    const auto separation_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(separation->current_snapshot());
    const wide_eye::game::GameplaySnapshot& separation_initial = *separation_initial_holder;
    if (!check(planar_distance(sheep_with_id(separation_initial.sheep, 1),
                               sheep_with_id(separation_initial.sheep, 2)) == 0.0,
               "separation_fixture_starts_with_exact_overlap")) {
        return EXIT_FAILURE;
    }

    separation->fixed_update({});
    const auto& separation_member_one_evidence =
        evidence_with_id(separation->current_snapshot().sheep_social_evidence, 1);
    if (!check(separation->previous_snapshot() == separation_initial,
               "separation_reads_immutable_prior_snapshot") ||
        !check(separation_acceleration_is_bounded(
                   *separation, separation_scenario->sheep_separation.maximum_acceleration),
               "overlap_recovery_acceleration_is_bounded") ||
        !check(sheep_with_id(separation->current_snapshot().sheep, 3).velocity ==
                       wide_eye::game::Vec3{} &&
                   sheep_with_id(separation->current_snapshot().sheep, 4).velocity ==
                       wide_eye::game::Vec3{} &&
                   sheep_with_id(separation->current_snapshot().sheep, 5).velocity ==
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
        separation->fixed_update({});
        if (!check(separation_acceleration_is_bounded(
                       *separation, separation_scenario->sheep_separation.maximum_acceleration),
                   "separation_acceleration_is_bounded")) {
            return EXIT_FAILURE;
        }
    }
    const auto separation_final_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(separation->current_snapshot());
    const wide_eye::game::GameplaySnapshot& separation_final = *separation_final_holder;
    if (!check(separation->previous_snapshot().sheep != separation_final.sheep,
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

    const auto reversed_separation_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*separation_scenario);
    auto& reversed_separation_scenario = *reversed_separation_scenario_holder;
    std::reverse(reversed_separation_scenario.initial_sheep.begin(),
                 reversed_separation_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_separation_scenario.sheep_count));
    const SimulationHandle reversed_separation = make_simulation(reversed_separation_scenario);
    for (std::uint64_t tick = 0; tick < kSeparationTicks; ++tick) {
        reversed_separation->fixed_update({});
    }
    for (const auto& member : active(separation_final.sheep, separation_final.sheep_count)) {
        if (!check(member ==
                       sheep_with_id(reversed_separation->current_snapshot().sheep, member.id),
                   "separation_result_is_stable_by_id_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const SimulationHandle allocation_separation = make_simulation(*separation_scenario);
    const std::size_t separation_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_separation->fixed_update({});
    }
    const std::size_t separation_allocations = g_allocation_count - separation_allocations_before;
    if (!check(separation_allocations == 0, "separation_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    separation->restart();
    if (!check(separation->current_snapshot() == separation_initial &&
                   separation->previous_snapshot() == separation_initial,
               "separation_restart_restores_overlap_fixture")) {
        return EXIT_FAILURE;
    }

    const ScenarioHandle attraction_scenario =
        named_scenario("sheep-only-attraction");
    if (!check(attraction_scenario != nullptr &&
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

    const SimulationHandle attraction = make_simulation(*attraction_scenario);
    const auto attraction_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(attraction->current_snapshot());
    const wide_eye::game::GameplaySnapshot& attraction_initial = *attraction_initial_holder;
    attraction->fixed_update({});
    const auto attraction_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(attraction->current_snapshot());
    const wide_eye::game::GameplaySnapshot& attraction_after_one = *attraction_after_one_holder;
    const auto& subject_one_evidence =
        evidence_with_id(attraction_after_one.sheep_social_evidence, 1);
    const auto& subject_one = sheep_with_id(attraction_after_one.sheep, 1);
    // Applied acceleration is the summed terms after the combined-influence
    // bound, so the expected value is the published term scaled by the factor
    // that sheep published. This fixture never reaches the bound, so the factor
    // is exactly one and the accepted arithmetic is unchanged.
    const auto subject_one_expected =
        bounded_terms(subject_one_evidence.attraction_acceleration,
                      evidence_with_id(attraction_after_one.sheep_combined_influence_evidence, 1));
    const double subject_one_acceleration_x =
        subject_one.velocity.x / wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    const double subject_one_acceleration_z =
        subject_one.velocity.z / wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
    if (!check(attraction->previous_snapshot() == attraction_initial,
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
        !check(std::abs(subject_one_acceleration_x - subject_one_expected.x) < 1.0e-12 &&
                   std::abs(subject_one_acceleration_z - subject_one_expected.z) < 1.0e-12,
               "published_attraction_matches_bounded_applied_acceleration")) {
        return EXIT_FAILURE;
    }

    for (const auto& evidence :
         active(attraction_after_one.sheep_social_evidence, attraction_after_one.sheep_count)) {
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

    const auto reversed_attraction_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*attraction_scenario);
    auto& reversed_attraction_scenario = *reversed_attraction_scenario_holder;
    std::reverse(reversed_attraction_scenario.initial_sheep.begin(),
                 reversed_attraction_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_attraction_scenario.sheep_count));
    const SimulationHandle reversed_attraction = make_simulation(reversed_attraction_scenario);
    reversed_attraction->fixed_update({});
    for (const auto& member :
         active(attraction_after_one.sheep, attraction_after_one.sheep_count)) {
        if (!check(member ==
                       sheep_with_id(reversed_attraction->current_snapshot().sheep, member.id),
                   "attraction_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(attraction_after_one.sheep_social_evidence, member.id) ==
                    evidence_with_id(reversed_attraction->current_snapshot().sheep_social_evidence,
                                     member.id),
                "chosen_neighbor_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto attraction_state = wide_eye::game::gameplay_state_dump_json(*attraction);
    if (!check(attraction_state &&
                   attraction_state.text.find(
                       "\"subject_id\":1,\"attraction_neighbor_ids\":[2,3],"
                       "\"attraction_neighbor_count\":2,\"attraction_candidate_count\":4") !=
                       std::string::npos,
               "state_dump_contains_exact_chosen_neighbor_evidence")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle allocation_attraction = make_simulation(*attraction_scenario);
    const std::size_t attraction_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_attraction->fixed_update({});
    }
    const std::size_t attraction_allocations = g_allocation_count - attraction_allocations_before;
    if (!check(attraction_allocations == 0, "attraction_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    attraction->restart();
    if (!check(attraction->current_snapshot() == attraction_initial &&
                   attraction->previous_snapshot() == attraction_initial,
               "attraction_restart_restores_dense_fixture")) {
        return EXIT_FAILURE;
    }

    const ScenarioHandle alignment_off_scenario =
        named_scenario("sheep-alignment-off");
    const ScenarioHandle alignment_on_scenario = named_scenario("sheep-alignment-on");
    auto alignment_on_as_control = mutable_scenario_copy(alignment_on_scenario);
    if (alignment_off_scenario != nullptr) {
        alignment_on_as_control->id = alignment_off_scenario->id;
    }
    alignment_on_as_control->sheep_alignment.enabled = false;
    if (!check(alignment_off_scenario != nullptr && alignment_on_scenario != nullptr &&
                   alignment_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_alignment_off &&
                   alignment_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_alignment_on &&
                   *alignment_on_as_control == *alignment_off_scenario &&
                   alignment_on_scenario->sheep_alignment.enabled &&
                   alignment_on_scenario->sheep_alignment.neighbor_limit ==
                       wide_eye::game::kMaximumSelectedAlignmentNeighbors,
               "paired_alignment_fixture_differs_only_by_alignment_switch")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle alignment_off = make_simulation(*alignment_off_scenario);
    const SimulationHandle alignment_on = make_simulation(*alignment_on_scenario);
    const auto alignment_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(alignment_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& alignment_initial = *alignment_initial_holder;
    alignment_off->fixed_update({});
    alignment_on->fixed_update({});
    const auto alignment_off_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(alignment_off->current_snapshot());
    const wide_eye::game::GameplaySnapshot& alignment_off_after_one =
        *alignment_off_after_one_holder;
    const auto alignment_on_after_one_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(alignment_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& alignment_on_after_one = *alignment_on_after_one_holder;
    const auto& alignment_evidence =
        evidence_with_id(alignment_on_after_one.sheep_social_evidence, 1);
    const auto& aligned_subject = sheep_with_id(alignment_on_after_one.sheep, 1);
    const auto& unaligned_subject = sheep_with_id(alignment_off_after_one.sheep, 1);
    const auto alignment_expected = bounded_terms(
        alignment_evidence.alignment_acceleration,
        evidence_with_id(alignment_on_after_one.sheep_combined_influence_evidence, 1));
    if (!check(alignment_on->previous_snapshot() == alignment_initial,
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
                        alignment_expected.x) < 1.0e-12 &&
                   std::abs((aligned_subject.velocity.z - alignment_initial.sheep[0].velocity.z) /
                                wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                            alignment_expected.z) < 1.0e-12,
               "published_alignment_matches_bounded_applied_acceleration") ||
        !check(unaligned_subject.velocity == alignment_initial.sheep[0].velocity &&
                   evidence_with_id(alignment_off_after_one.sheep_social_evidence, 1)
                           .alignment_acceleration == wide_eye::game::Vec3{},
               "alignment_off_preserves_velocity_and_zeroes_evidence")) {
        return EXIT_FAILURE;
    }

    for (const auto& evidence :
         active(alignment_on_after_one.sheep_social_evidence, alignment_on_after_one.sheep_count)) {
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
        alignment_off->fixed_update({});
        alignment_on->fixed_update({});
    }
    constexpr std::array<std::uint32_t, wide_eye::game::kDefaultGameplaySheepCount> kNoNeighbors{};
    const auto alignment_off_observables = wide_eye::game::compute_flock_observables(
        active_sheep(alignment_off->current_snapshot()), kNoNeighbors, 3.0,
        alignment_off->current_snapshot().dog.position);
    const auto alignment_on_observables = wide_eye::game::compute_flock_observables(
        active_sheep(alignment_on->current_snapshot()), kNoNeighbors, 3.0,
        alignment_on->current_snapshot().dog.position);
    if (!check(alignment_off_observables.has_value() && alignment_on_observables.has_value() &&
                   alignment_on_observables->polarization >
                       alignment_off_observables->polarization + 0.05,
               "alignment_on_improves_directional_agreement_over_paired_control")) {
        return EXIT_FAILURE;
    }

    const auto reversed_alignment_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*alignment_on_scenario);
    auto& reversed_alignment_scenario = *reversed_alignment_scenario_holder;
    std::reverse(reversed_alignment_scenario.initial_sheep.begin(),
                 reversed_alignment_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_alignment_scenario.sheep_count));
    const SimulationHandle reversed_alignment = make_simulation(reversed_alignment_scenario);
    for (std::uint64_t tick = 0; tick < kAlignmentComparisonTicks; ++tick) {
        reversed_alignment->fixed_update({});
    }
    for (const auto& member : alignment_on->current_snapshot().sheep) {
        if (!check(member == sheep_with_id(reversed_alignment->current_snapshot().sheep, member.id),
                   "alignment_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(alignment_on->current_snapshot().sheep_social_evidence,
                                    member.id) ==
                       evidence_with_id(
                           reversed_alignment->current_snapshot().sheep_social_evidence, member.id),
                   "alignment_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto alignment_state = wide_eye::game::gameplay_state_dump_json(*alignment_on);
    if (!check(alignment_state &&
                   alignment_state.text.find("\"alignment_neighbor_ids\":[") != std::string::npos &&
                   alignment_state.text.find("\"alignment_acceleration\":{") != std::string::npos,
               "state_dump_contains_alignment_selection_and_influence_evidence")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle allocation_alignment = make_simulation(*alignment_on_scenario);
    const std::size_t alignment_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_alignment->fixed_update({});
    }
    const std::size_t alignment_allocations = g_allocation_count - alignment_allocations_before;
    if (!check(alignment_allocations == 0, "alignment_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    alignment_on->restart();
    if (!check(alignment_on->current_snapshot() == alignment_initial &&
                   alignment_on->previous_snapshot() == alignment_initial,
               "alignment_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    const ScenarioHandle dog_pressure_off_scenario =
        named_scenario("sheep-dog-pressure-off");
    const ScenarioHandle dog_pressure_on_scenario =
        named_scenario("sheep-dog-pressure-on");
    auto dog_pressure_on_as_control = mutable_scenario_copy(dog_pressure_on_scenario);
    if (dog_pressure_off_scenario != nullptr) {
        dog_pressure_on_as_control->id = dog_pressure_off_scenario->id;
    }
    dog_pressure_on_as_control->sheep_dog_pressure.enabled = false;
    if (!check(dog_pressure_off_scenario != nullptr && dog_pressure_on_scenario != nullptr &&
                   dog_pressure_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_pressure_off &&
                   dog_pressure_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_pressure_on &&
                   *dog_pressure_on_as_control == *dog_pressure_off_scenario &&
                   dog_pressure_on_scenario->sheep_dog_pressure.enabled,
               "paired_dog_pressure_fixture_differs_only_by_pressure_switch")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle dog_pressure_off = make_simulation(*dog_pressure_off_scenario);
    const SimulationHandle dog_pressure_on = make_simulation(*dog_pressure_on_scenario);
    const auto dog_pressure_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(dog_pressure_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& dog_pressure_initial = *dog_pressure_initial_holder;
    dog_pressure_off->fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0}});
    dog_pressure_on->fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0}});
    const auto& dog_pressure_off_after_one = dog_pressure_off->current_snapshot();
    const auto& dog_pressure_on_after_one = dog_pressure_on->current_snapshot();
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
    if (!check(dog_pressure_on->previous_snapshot() == dog_pressure_initial,
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

    for (const auto& on_evidence :
         active(dog_pressure_on_after_one.sheep_dog_pressure_evidence,
                dog_pressure_on_after_one.sheep_count)) {
        const auto& off_evidence = evidence_with_id(
            dog_pressure_off_after_one.sheep_dog_pressure_evidence, on_evidence.subject_id);
        const auto& current_member =
            sheep_with_id(dog_pressure_on_after_one.sheep, on_evidence.subject_id);
        const auto& prior_member =
            sheep_with_id(dog_pressure_initial.sheep, on_evidence.subject_id);
        const auto expected = bounded_terms(
            on_evidence.pressure_acceleration,
            evidence_with_id(dog_pressure_on_after_one.sheep_combined_influence_evidence,
                             on_evidence.subject_id));
        if (!check(off_evidence.stimulus_evaluated == on_evidence.stimulus_evaluated &&
                       off_evidence.dog_distance == on_evidence.dog_distance &&
                       off_evidence.dog_relative_bearing_radians ==
                           on_evidence.dog_relative_bearing_radians &&
                       off_evidence.pressure_acceleration == wide_eye::game::Vec3{},
                   "pressure_control_publishes_same_geometry_without_influence") ||
            !check(std::abs((current_member.velocity.x - prior_member.velocity.x) /
                                wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                            expected.x) < 1.0e-12 &&
                       std::abs((current_member.velocity.z - prior_member.velocity.z) /
                                    wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                                expected.z) < 1.0e-12,
                   "published_dog_pressure_matches_bounded_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    const auto overlapping_dog_pressure_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*dog_pressure_on_scenario);
    auto& overlapping_dog_pressure_scenario = *overlapping_dog_pressure_scenario_holder;
    overlapping_dog_pressure_scenario.initial_sheep[0].position =
        overlapping_dog_pressure_scenario.dog.initial_state.position;
    const SimulationHandle overlapping_dog_pressure =
        make_simulation(overlapping_dog_pressure_scenario);
    overlapping_dog_pressure->fixed_update({});
    const auto& overlap_evidence = evidence_with_id(
        overlapping_dog_pressure->current_snapshot().sheep_dog_pressure_evidence, 1);
    if (!check(overlap_evidence.stimulus_evaluated && overlap_evidence.dog_distance == 0.0 &&
                   overlap_evidence.dog_relative_bearing_radians == 0.0 &&
                   overlap_evidence.pressure_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(overlapping_dog_pressure->current_snapshot().sheep, 1).velocity ==
                       wide_eye::game::Vec3{},
               "exact_dog_overlap_does_not_invent_pressure_direction")) {
        return EXIT_FAILURE;
    }

    const auto reversed_dog_pressure_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*dog_pressure_on_scenario);
    auto& reversed_dog_pressure_scenario = *reversed_dog_pressure_scenario_holder;
    std::reverse(reversed_dog_pressure_scenario.initial_sheep.begin(),
                 reversed_dog_pressure_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_dog_pressure_scenario.sheep_count));
    const SimulationHandle reversed_dog_pressure = make_simulation(reversed_dog_pressure_scenario);
    reversed_dog_pressure->fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0}});
    for (const auto& member :
         active(dog_pressure_on_after_one.sheep, dog_pressure_on_after_one.sheep_count)) {
        if (!check(member ==
                       sheep_with_id(reversed_dog_pressure->current_snapshot().sheep, member.id),
                   "dog_pressure_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(dog_pressure_on_after_one.sheep_dog_pressure_evidence,
                                    member.id) ==
                       evidence_with_id(
                           reversed_dog_pressure->current_snapshot().sheep_dog_pressure_evidence,
                           member.id),
                   "dog_pressure_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto dog_pressure_state = wide_eye::game::gameplay_state_dump_json(*dog_pressure_on);
    if (!check(dog_pressure_state &&
                   dog_pressure_state.text.find("\"sheep_dog_pressure_evidence\":[") !=
                       std::string::npos &&
                   dog_pressure_state.text.find("\"dog_relative_bearing_radians\":") !=
                       std::string::npos &&
                   dog_pressure_state.text.find("\"pressure_acceleration\":{") != std::string::npos,
               "state_dump_contains_dog_stimulus_and_pressure_evidence")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle allocation_dog_pressure = make_simulation(*dog_pressure_on_scenario);
    const std::size_t dog_pressure_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_dog_pressure->fixed_update({});
    }
    const std::size_t dog_pressure_allocations =
        g_allocation_count - dog_pressure_allocations_before;
    if (!check(dog_pressure_allocations == 0, "dog_pressure_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    dog_pressure_on->restart();
    if (!check(dog_pressure_on->current_snapshot() == dog_pressure_initial &&
                   dog_pressure_on->previous_snapshot() == dog_pressure_initial,
               "dog_pressure_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    const ScenarioHandle approach_off_scenario =
        named_scenario("sheep-dog-approach-off");
    const ScenarioHandle approach_on_scenario =
        named_scenario("sheep-dog-approach-on");
    auto approach_on_as_control = mutable_scenario_copy(approach_on_scenario);
    if (approach_off_scenario != nullptr) {
        approach_on_as_control->id = approach_off_scenario->id;
    }
    approach_on_as_control->sheep_dog_approach.enabled = false;
    if (!check(approach_off_scenario != nullptr && approach_on_scenario != nullptr &&
                   approach_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_approach_off &&
                   approach_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_approach_on &&
                   *approach_on_as_control == *approach_off_scenario &&
                   approach_on_scenario->sheep_dog_approach.enabled &&
                   approach_off_scenario->sheep_dog_pressure.enabled &&
                   approach_off_scenario->sheep_dog_pressure ==
                       approach_on_scenario->sheep_dog_pressure,
               "paired_approach_fixture_differs_only_by_approach_switch")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle approach_off = make_simulation(*approach_off_scenario);
    const SimulationHandle approach_on = make_simulation(*approach_on_scenario);
    const auto approach_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(approach_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& approach_initial = *approach_initial_holder;
    approach_off->fixed_update({});
    approach_on->fixed_update({});
    const auto& approach_off_after_one = approach_off->current_snapshot();
    const auto& approach_on_after_one = approach_on->current_snapshot();
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
    if (!check(approach_on->previous_snapshot() == approach_initial,
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

    for (const auto& on_evidence :
         active(approach_on_after_one.sheep_dog_pressure_evidence,
                approach_on_after_one.sheep_count)) {
        const auto& off_evidence = evidence_with_id(
            approach_off_after_one.sheep_dog_pressure_evidence, on_evidence.subject_id);
        const auto& current_member =
            sheep_with_id(approach_on_after_one.sheep, on_evidence.subject_id);
        const auto& prior_member = sheep_with_id(approach_initial.sheep, on_evidence.subject_id);
        const auto expected = bounded_terms(
            {.x = on_evidence.pressure_acceleration.x + on_evidence.approach_acceleration.x,
             .z = on_evidence.pressure_acceleration.z + on_evidence.approach_acceleration.z},
            evidence_with_id(approach_on_after_one.sheep_combined_influence_evidence,
                             on_evidence.subject_id));
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
                            expected.x) < 1.0e-12 &&
                       std::abs((current_member.velocity.z - prior_member.velocity.z) /
                                    wide_eye::game::GameplaySimulation::kFixedDeltaSeconds -
                                expected.z) < 1.0e-12,
                   "published_dog_terms_match_bounded_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    // The dog motor changes velocity within the same tick. Approach evidence must
    // still describe the prior state that caused the published sheep result.
    const SimulationHandle approach_same_tick_move = make_simulation(*approach_on_scenario);
    approach_same_tick_move->fixed_update(
        {.dog_move = wide_eye::game::DogMoveInput{.world_x = -1.0}});
    if (!check(approach_same_tick_move->current_snapshot().dog.velocity.x != 4.0 &&
                   evidence_with_id(
                       approach_same_tick_move->current_snapshot().sheep_dog_pressure_evidence,
                       1) == head_on_approach,
               "same_tick_dog_motor_change_does_not_alter_prior_state_approach")) {
        return EXIT_FAILURE;
    }

    const auto overlapping_approach_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*approach_on_scenario);
    auto& overlapping_approach_scenario = *overlapping_approach_scenario_holder;
    overlapping_approach_scenario.initial_sheep[0].position =
        overlapping_approach_scenario.dog.initial_state.position;
    const SimulationHandle overlapping_approach = make_simulation(overlapping_approach_scenario);
    overlapping_approach->fixed_update({});
    const auto& overlap_approach_evidence =
        evidence_with_id(overlapping_approach->current_snapshot().sheep_dog_pressure_evidence, 1);
    if (!check(overlap_approach_evidence.stimulus_evaluated &&
                   overlap_approach_evidence.dog_distance == 0.0 &&
                   overlap_approach_evidence.dog_approach_speed == 0.0 &&
                   overlap_approach_evidence.approach_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(overlapping_approach->current_snapshot().sheep, 1).velocity ==
                       wide_eye::game::Vec3{},
               "exact_dog_overlap_does_not_invent_approach_direction")) {
        return EXIT_FAILURE;
    }

    const auto reversed_approach_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*approach_on_scenario);
    auto& reversed_approach_scenario = *reversed_approach_scenario_holder;
    std::reverse(reversed_approach_scenario.initial_sheep.begin(),
                 reversed_approach_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_approach_scenario.sheep_count));
    const SimulationHandle reversed_approach = make_simulation(reversed_approach_scenario);
    reversed_approach->fixed_update({});
    for (const auto& member :
         active(approach_on_after_one.sheep, approach_on_after_one.sheep_count)) {
        if (!check(member == sheep_with_id(reversed_approach->current_snapshot().sheep, member.id),
                   "approach_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(approach_on_after_one.sheep_dog_pressure_evidence, member.id) ==
                       evidence_with_id(
                           reversed_approach->current_snapshot().sheep_dog_pressure_evidence,
                           member.id),
                   "approach_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto approach_state = wide_eye::game::gameplay_state_dump_json(*approach_on);
    if (!check(approach_state &&
                   approach_state.text.find("\"dog_approach_speed\":") != std::string::npos &&
                   approach_state.text.find("\"approach_acceleration\":{") != std::string::npos,
               "state_dump_contains_dog_approach_stimulus_and_influence")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle allocation_approach = make_simulation(*approach_on_scenario);
    const std::size_t approach_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_approach->fixed_update({});
    }
    const std::size_t approach_allocations = g_allocation_count - approach_allocations_before;
    if (!check(approach_allocations == 0, "approach_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    approach_on->restart();
    if (!check(approach_on->current_snapshot() == approach_initial &&
                   approach_on->previous_snapshot() == approach_initial,
               "approach_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    const ScenarioHandle facing_off_scenario = named_scenario("sheep-dog-facing-off");
    const ScenarioHandle facing_on_scenario = named_scenario("sheep-dog-facing-on");
    auto facing_on_as_control = mutable_scenario_copy(facing_on_scenario);
    if (facing_off_scenario != nullptr) {
        facing_on_as_control->id = facing_off_scenario->id;
    }
    facing_on_as_control->sheep_dog_facing.enabled = false;
    if (!check(
            facing_off_scenario != nullptr && facing_on_scenario != nullptr &&
                facing_off_scenario->id ==
                    wide_eye::game::GameplayScenarioId::sheep_dog_facing_off &&
                facing_on_scenario->id == wide_eye::game::GameplayScenarioId::sheep_dog_facing_on &&
                *facing_on_as_control == *facing_off_scenario &&
                facing_on_scenario->sheep_dog_facing.enabled &&
                facing_off_scenario->sheep_dog_pressure.enabled &&
                facing_off_scenario->sheep_dog_pressure == facing_on_scenario->sheep_dog_pressure &&
                !facing_off_scenario->sheep_dog_approach.enabled &&
                facing_off_scenario->dog.initial_state.velocity == wide_eye::game::Vec3{},
            "paired_facing_fixture_differs_only_by_facing_switch")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle facing_off = make_simulation(*facing_off_scenario);
    const SimulationHandle facing_on = make_simulation(*facing_on_scenario);
    const auto facing_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(facing_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& facing_initial = *facing_initial_holder;
    facing_off->fixed_update({});
    facing_on->fixed_update({});
    const auto& facing_off_after_one = facing_off->current_snapshot();
    const auto& facing_on_after_one = facing_on->current_snapshot();
    const auto ahead_facing = evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 1);
    const auto abeam_facing = evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 2);
    const auto behind_facing = evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 3);
    const auto diagonal_facing =
        evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 4);
    const auto outside_facing =
        evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, 5);
    if (!check(facing_on->previous_snapshot() == facing_initial,
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

    for (const auto& on_evidence :
         active(facing_on_after_one.sheep_dog_pressure_evidence, facing_on_after_one.sheep_count)) {
        const auto& off_evidence = evidence_with_id(
            facing_off_after_one.sheep_dog_pressure_evidence, on_evidence.subject_id);
        const auto& current_member =
            sheep_with_id(facing_on_after_one.sheep, on_evidence.subject_id);
        const auto& prior_member = sheep_with_id(facing_initial.sheep, on_evidence.subject_id);
        const auto expected = bounded_terms(
            {.x = on_evidence.pressure_acceleration.x + on_evidence.facing_acceleration.x,
             .z = on_evidence.pressure_acceleration.z + on_evidence.facing_acceleration.z},
            evidence_with_id(facing_on_after_one.sheep_combined_influence_evidence,
                             on_evidence.subject_id));
        // Integration is not the last authority: the paddock is. Sheep 4 of this
        // fixture stands on the closed gate's face, so from the first tick the
        // field pushes it off and clears the refused axis under the accepted
        // contact rule (QA-001). An axis the field refused therefore has to
        // publish exactly that instead of the integrated acceleration, and every
        // axis it did not refuse still has to match the published terms exactly.
        const auto& contact =
            evidence_with_id(facing_on_after_one.sheep_collision_evidence, on_evidence.subject_id);
        const double applied_x = (current_member.velocity.x - prior_member.velocity.x) /
                                 wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
        const double applied_z = (current_member.velocity.z - prior_member.velocity.z) /
                                 wide_eye::game::GameplaySimulation::kFixedDeltaSeconds;
        const bool x_matches = contact.clipped_x ? current_member.velocity.x == 0.0
                                                 : std::abs(applied_x - expected.x) < 1.0e-12;
        const bool z_matches = contact.clipped_z ? current_member.velocity.z == 0.0
                                                 : std::abs(applied_z - expected.z) < 1.0e-12;
        if (!check(off_evidence.stimulus_evaluated == on_evidence.stimulus_evaluated &&
                       off_evidence.dog_distance == on_evidence.dog_distance &&
                       off_evidence.dog_relative_bearing_radians ==
                           on_evidence.dog_relative_bearing_radians &&
                       off_evidence.dog_approach_speed == on_evidence.dog_approach_speed &&
                       off_evidence.dog_facing_alignment == on_evidence.dog_facing_alignment &&
                       off_evidence.pressure_acceleration == on_evidence.pressure_acceleration &&
                       off_evidence.facing_acceleration == wide_eye::game::Vec3{},
                   "facing_control_preserves_accepted_distance_only_pressure") ||
            !check(x_matches && z_matches,
                   "published_facing_term_matches_bounded_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    // Facing must be read from the dog's heading rather than assumed from the
    // fixture layout. Turning the same dog through half a turn swaps which sheep
    // it looks at without moving any position.
    const auto reversed_heading_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*facing_on_scenario);
    auto& reversed_heading_scenario = *reversed_heading_scenario_holder;
    reversed_heading_scenario.dog.initial_state.heading_radians = 3.14159265358979323846;
    const SimulationHandle reversed_heading = make_simulation(reversed_heading_scenario);
    reversed_heading->fixed_update({});
    const auto& reversed_ahead =
        evidence_with_id(reversed_heading->current_snapshot().sheep_dog_pressure_evidence, 1);
    const auto& reversed_behind =
        evidence_with_id(reversed_heading->current_snapshot().sheep_dog_pressure_evidence, 3);
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
    const SimulationHandle facing_same_tick_turn = make_simulation(*facing_on_scenario);
    facing_same_tick_turn->fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_x = 1.0}});
    if (!check(facing_same_tick_turn->current_snapshot().dog.heading_radians != 0.0 &&
                   evidence_with_id(
                       facing_same_tick_turn->current_snapshot().sheep_dog_pressure_evidence, 1) ==
                       ahead_facing,
               "same_tick_dog_motor_turn_does_not_alter_prior_state_facing")) {
        return EXIT_FAILURE;
    }

    const auto overlapping_facing_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*facing_on_scenario);
    auto& overlapping_facing_scenario = *overlapping_facing_scenario_holder;
    overlapping_facing_scenario.initial_sheep[0].position =
        overlapping_facing_scenario.dog.initial_state.position;
    const SimulationHandle overlapping_facing = make_simulation(overlapping_facing_scenario);
    overlapping_facing->fixed_update({});
    const auto& overlap_facing_evidence =
        evidence_with_id(overlapping_facing->current_snapshot().sheep_dog_pressure_evidence, 1);
    if (!check(overlap_facing_evidence.stimulus_evaluated &&
                   overlap_facing_evidence.dog_distance == 0.0 &&
                   overlap_facing_evidence.dog_facing_alignment == 0.0 &&
                   overlap_facing_evidence.facing_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(overlapping_facing->current_snapshot().sheep, 1).velocity ==
                       wide_eye::game::Vec3{},
               "exact_dog_overlap_does_not_invent_facing_direction")) {
        return EXIT_FAILURE;
    }

    const auto reversed_facing_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*facing_on_scenario);
    auto& reversed_facing_scenario = *reversed_facing_scenario_holder;
    std::reverse(reversed_facing_scenario.initial_sheep.begin(),
                 reversed_facing_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_facing_scenario.sheep_count));
    const SimulationHandle reversed_facing = make_simulation(reversed_facing_scenario);
    reversed_facing->fixed_update({});
    for (const auto& member : active(facing_on_after_one.sheep, facing_on_after_one.sheep_count)) {
        if (!check(member == sheep_with_id(reversed_facing->current_snapshot().sheep, member.id),
                   "facing_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(facing_on_after_one.sheep_dog_pressure_evidence, member.id) ==
                    evidence_with_id(
                        reversed_facing->current_snapshot().sheep_dog_pressure_evidence, member.id),
                "facing_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto facing_state = wide_eye::game::gameplay_state_dump_json(*facing_on);
    if (!check(facing_state &&
                   facing_state.text.find("\"dog_facing_alignment\":") != std::string::npos &&
                   facing_state.text.find("\"facing_acceleration\":{") != std::string::npos,
               "state_dump_contains_dog_facing_stimulus_and_influence")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle allocation_facing = make_simulation(*facing_on_scenario);
    const std::size_t facing_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_facing->fixed_update({});
    }
    const std::size_t facing_allocations = g_allocation_count - facing_allocations_before;
    if (!check(facing_allocations == 0, "facing_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    facing_on->restart();
    if (!check(facing_on->current_snapshot() == facing_initial &&
                   facing_on->previous_snapshot() == facing_initial,
               "facing_restart_restores_paired_fixture")) {
        return EXIT_FAILURE;
    }

    const ScenarioHandle sight_off_scenario =
        named_scenario("sheep-dog-line-of-sight-off");
    const ScenarioHandle sight_on_scenario =
        named_scenario("sheep-dog-line-of-sight-on");
    auto sight_on_as_control = mutable_scenario_copy(sight_on_scenario);
    if (sight_off_scenario != nullptr) {
        sight_on_as_control->id = sight_off_scenario->id;
    }
    sight_on_as_control->sheep_dog_line_of_sight.enabled = false;
    if (!check(sight_off_scenario != nullptr && sight_on_scenario != nullptr &&
                   sight_off_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_line_of_sight_off &&
                   sight_on_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_dog_line_of_sight_on &&
                   *sight_on_as_control == *sight_off_scenario &&
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

    const SimulationHandle sight_off = make_simulation(*sight_off_scenario);
    const SimulationHandle sight_on = make_simulation(*sight_on_scenario);
    const auto sight_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(sight_on->current_snapshot());
    const wide_eye::game::GameplaySnapshot& sight_initial = *sight_initial_holder;
    sight_off->fixed_update({});
    sight_on->fixed_update({});
    const auto& sight_off_after_one = sight_off->current_snapshot();
    const auto& sight_on_after_one = sight_on->current_snapshot();
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
    if (!check(sight_on->previous_snapshot() == sight_initial,
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

    for (const auto& on_evidence :
         active(sight_on_after_one.sheep_dog_pressure_evidence, sight_on_after_one.sheep_count)) {
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
        const auto on_expected =
            bounded_terms(on_evidence.pressure_acceleration,
                          evidence_with_id(sight_on_after_one.sheep_combined_influence_evidence,
                                           on_evidence.subject_id));
        const auto off_expected =
            bounded_terms(off_evidence.pressure_acceleration,
                          evidence_with_id(sight_off_after_one.sheep_combined_influence_evidence,
                                           on_evidence.subject_id));
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
            !check(std::abs(on_applied.x - on_expected.x) < 1.0e-12 &&
                       std::abs(on_applied.z - on_expected.z) < 1.0e-12 &&
                       std::abs(off_applied.x - off_expected.x) < 1.0e-12 &&
                       std::abs(off_applied.z - off_expected.z) < 1.0e-12,
                   "published_line_of_sight_terms_match_bounded_applied_acceleration")) {
            return EXIT_FAILURE;
        }
    }

    // The gate is world state, not fixture layout: closing it must hide the dog
    // from the sheep that was watching through the opening and leave every other
    // sight line unchanged.
    const auto closed_gate_sight_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*sight_on_scenario);
    auto& closed_gate_sight_scenario = *closed_gate_sight_scenario_holder;
    closed_gate_sight_scenario.gate_open = false;
    const SimulationHandle closed_gate_sight = make_simulation(closed_gate_sight_scenario);
    closed_gate_sight->fixed_update({});
    const auto& closed_gate_evidence =
        closed_gate_sight->current_snapshot().sheep_dog_pressure_evidence;
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
    const auto combined_sight_on_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*sight_on_scenario);
    auto& combined_sight_on_scenario = *combined_sight_on_scenario_holder;
    combined_sight_on_scenario.dog.initial_state.velocity = {.z = 3.0};
    combined_sight_on_scenario.sheep_dog_approach.enabled = true;
    combined_sight_on_scenario.sheep_dog_facing.enabled = true;
    auto combined_sight_off_scenario = combined_sight_on_scenario;
    combined_sight_off_scenario.id = sight_off_scenario->id;
    combined_sight_off_scenario.sheep_dog_line_of_sight.enabled = false;
    const SimulationHandle combined_sight_on = make_simulation(combined_sight_on_scenario);
    const SimulationHandle combined_sight_off = make_simulation(combined_sight_off_scenario);
    combined_sight_on->fixed_update({});
    combined_sight_off->fixed_update({});
    const auto& combined_on_blocked =
        evidence_with_id(combined_sight_on->current_snapshot().sheep_dog_pressure_evidence, 2);
    const auto& combined_off_blocked =
        evidence_with_id(combined_sight_off->current_snapshot().sheep_dog_pressure_evidence, 2);
    const auto& combined_on_visible =
        evidence_with_id(combined_sight_on->current_snapshot().sheep_dog_pressure_evidence, 3);
    const auto& combined_off_visible =
        evidence_with_id(combined_sight_off->current_snapshot().sheep_dog_pressure_evidence, 3);
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
                   sheep_with_id(combined_sight_on->current_snapshot().sheep, 2).velocity ==
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
    const SimulationHandle sight_same_tick_move = make_simulation(*sight_on_scenario);
    sight_same_tick_move->fixed_update({.dog_move = wide_eye::game::DogMoveInput{.world_z = 1.0}});
    if (!check(sight_same_tick_move->current_snapshot().dog.position.z >
                       sight_on_scenario->dog.initial_state.position.z &&
                   evidence_with_id(
                       sight_same_tick_move->current_snapshot().sheep_dog_pressure_evidence, 2) ==
                       left_wall_sight,
               "same_tick_dog_motor_move_does_not_alter_prior_state_sight")) {
        return EXIT_FAILURE;
    }

    const auto overlapping_sight_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*sight_on_scenario);
    auto& overlapping_sight_scenario = *overlapping_sight_scenario_holder;
    overlapping_sight_scenario.initial_sheep[0].position =
        overlapping_sight_scenario.dog.initial_state.position;
    const SimulationHandle overlapping_sight = make_simulation(overlapping_sight_scenario);
    overlapping_sight->fixed_update({});
    const auto& overlap_sight_evidence =
        evidence_with_id(overlapping_sight->current_snapshot().sheep_dog_pressure_evidence, 1);
    if (!check(overlap_sight_evidence.stimulus_evaluated &&
                   overlap_sight_evidence.dog_distance == 0.0 &&
                   !overlap_sight_evidence.dog_line_of_sight_blocked &&
                   overlap_sight_evidence.dog_line_of_sight_occluder ==
                       wide_eye::game::PaddockObstacle::none &&
                   overlap_sight_evidence.pressure_acceleration == wide_eye::game::Vec3{} &&
                   sheep_with_id(overlapping_sight->current_snapshot().sheep, 1).velocity ==
                       wide_eye::game::Vec3{},
               "exact_dog_overlap_does_not_invent_an_occluder")) {
        return EXIT_FAILURE;
    }

    const auto reversed_sight_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*sight_on_scenario);
    auto& reversed_sight_scenario = *reversed_sight_scenario_holder;
    std::reverse(reversed_sight_scenario.initial_sheep.begin(),
                 reversed_sight_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_sight_scenario.sheep_count));
    const SimulationHandle reversed_sight = make_simulation(reversed_sight_scenario);
    reversed_sight->fixed_update({});
    for (const auto& member : active(sight_on_after_one.sheep, sight_on_after_one.sheep_count)) {
        if (!check(member == sheep_with_id(reversed_sight->current_snapshot().sheep, member.id),
                   "line_of_sight_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(sight_on_after_one.sheep_dog_pressure_evidence, member.id) ==
                    evidence_with_id(reversed_sight->current_snapshot().sheep_dog_pressure_evidence,
                                     member.id),
                "line_of_sight_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto sight_state = wide_eye::game::gameplay_state_dump_json(*sight_on);
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

    const SimulationHandle allocation_sight = make_simulation(*sight_on_scenario);
    const std::size_t sight_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_sight->fixed_update({});
    }
    const std::size_t sight_allocations = g_allocation_count - sight_allocations_before;
    if (!check(sight_allocations == 0, "line_of_sight_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }
    sight_on->restart();
    if (!check(sight_on->current_snapshot() == sight_initial &&
                   sight_on->previous_snapshot() == sight_initial,
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
    const ScenarioHandle collision_closed_scenario =
        named_scenario("sheep-paddock-collision-closed-gate");
    const ScenarioHandle collision_open_scenario =
        named_scenario("sheep-paddock-collision-open-gate");
    auto collision_open_as_control = mutable_scenario_copy(collision_open_scenario);
    if (collision_closed_scenario != nullptr) {
        collision_open_as_control->id = collision_closed_scenario->id;
    }
    collision_open_as_control->gate_open = false;
    if (!check(collision_closed_scenario != nullptr && collision_open_scenario != nullptr &&
                   collision_closed_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_paddock_collision_closed_gate &&
                   collision_open_scenario->id ==
                       wide_eye::game::GameplayScenarioId::sheep_paddock_collision_open_gate &&
                   *collision_open_as_control == *collision_closed_scenario &&
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

    const PaddockCollisionHandle closed_gate_run = run_paddock_collision(
        *collision_closed_scenario, kPaddockCollisionTicks, kPaddockCollisionMidpointTick);
    const PaddockCollisionHandle open_gate_run = run_paddock_collision(
        *collision_open_scenario, kPaddockCollisionTicks, kPaddockCollisionMidpointTick);
    const SheepContactRecord& left_wall_contact = contact_with_id(closed_gate_run->contacts, 1);
    const SheepContactRecord& gate_contact = contact_with_id(closed_gate_run->contacts, 2);
    const SheepContactRecord& right_wall_contact = contact_with_id(closed_gate_run->contacts, 3);
    const SheepContactRecord& untouched_contact = contact_with_id(closed_gate_run->contacts, 4);
    const SheepContactRecord& bound_contact = contact_with_id(closed_gate_run->contacts, 5);
    const auto& closed_gate_sheep_two = sheep_with_id(closed_gate_run->final_snapshot.sheep, 2);
    if (!check(left_wall_contact.observed && left_wall_contact.evidence.clipped_z &&
                   !left_wall_contact.evidence.clipped_x &&
                   left_wall_contact.evidence.obstacle ==
                       wide_eye::game::PaddockObstacle::left_wall &&
                   left_wall_contact.prior.position.z > kWallRestZ &&
                   left_wall_contact.state.position.z == kWallRestZ &&
                   left_wall_contact.state.velocity.z == 0.0 &&
                   left_wall_contact.minimum_z == kWallRestZ &&
                   sheep_with_id(closed_gate_run->final_snapshot.sheep, 1).position.z == kWallRestZ,
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
                   sheep_with_id(closed_gate_run->final_snapshot.sheep, 3).position.z ==
                       kWallRestZ &&
                   sheep_with_id(closed_gate_run->final_snapshot.sheep, 3).position.x < 4.0,
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
    const SheepContactRecord& open_gate_contact = contact_with_id(open_gate_run->contacts, 2);
    const auto& open_gate_sheep_two = sheep_with_id(open_gate_run->final_snapshot.sheep, 2);
    if (!check(sheep_with_id(open_gate_run->midpoint.sheep, 2).position.z < 15.0 &&
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
        sheep_with_id(closed_gate_run->final_snapshot.sheep, 4) ==
            sheep_with_id(open_gate_run->final_snapshot.sheep, 4) &&
        sheep_with_id(closed_gate_run->final_snapshot.sheep, 4) == unclipped;
    for (const std::uint32_t id : {1U, 3U, 4U, 5U}) {
        untouched_matches_control =
            untouched_matches_control &&
            sheep_with_id(closed_gate_run->final_snapshot.sheep, id) ==
                sheep_with_id(open_gate_run->final_snapshot.sheep, id) &&
            evidence_with_id(closed_gate_run->final_snapshot.sheep_social_evidence, id) ==
                evidence_with_id(open_gate_run->final_snapshot.sheep_social_evidence, id) &&
            evidence_with_id(closed_gate_run->final_snapshot.sheep_dog_pressure_evidence, id) ==
                evidence_with_id(open_gate_run->final_snapshot.sheep_dog_pressure_evidence, id) &&
            evidence_with_id(closed_gate_run->final_snapshot.sheep_collision_evidence, id) ==
                evidence_with_id(open_gate_run->final_snapshot.sheep_collision_evidence, id) &&
            contact_with_id(closed_gate_run->contacts, id).tick ==
                contact_with_id(open_gate_run->contacts, id).tick;
    }
    if (!check(untouched_matches_control,
               "collision_authority_leaves_a_non_contacting_sheep_untouched")) {
        return EXIT_FAILURE;
    }

    // A dog placed north of the gate line must physically drive one sheep into
    // the closed gate and be unable to push it through, while the same fixture
    // with the gate open lets that sheep out.
    const auto driven_closed_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*collision_closed_scenario);
    auto& driven_closed_scenario = *driven_closed_scenario_holder;
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
    const PaddockCollisionHandle driven_closed_run = run_paddock_collision(
        driven_closed_scenario, kDrivenCollisionTicks, kPaddockCollisionMidpointTick);
    const PaddockCollisionHandle driven_open_run = run_paddock_collision(
        driven_open_scenario, kDrivenCollisionTicks, kPaddockCollisionMidpointTick);
    const SheepContactRecord& driven_contact = contact_with_id(driven_closed_run->contacts, 2);
    const auto& driven_closed_two = sheep_with_id(driven_closed_run->final_snapshot.sheep, 2);
    const auto& driven_open_two = sheep_with_id(driven_open_run->final_snapshot.sheep, 2);
    const auto& driven_pressure =
        evidence_with_id(driven_closed_run->final_snapshot.sheep_dog_pressure_evidence, 2);
    const auto& driven_collision =
        evidence_with_id(driven_closed_run->final_snapshot.sheep_collision_evidence, 2);
    if (!check(driven_contact.observed &&
                   driven_contact.evidence.obstacle == wide_eye::game::PaddockObstacle::gate &&
                   driven_closed_two.position.z == kWallRestZ &&
                   driven_closed_two.velocity.z == 0.0 &&
                   driven_contact.contact_ticks == kDrivenCollisionTicks - driven_contact.tick + 1,
               "a_closed_gate_holds_a_dog_driven_sheep_on_every_pushed_tick") ||
        !check(!contact_with_id(driven_open_run->contacts, 2).observed &&
                   sheep_with_id(driven_open_run->midpoint.sheep, 2).position.z < 15.0 &&
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

    const auto reversed_collision_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*collision_closed_scenario);
    auto& reversed_collision_scenario = *reversed_collision_scenario_holder;
    std::reverse(reversed_collision_scenario.initial_sheep.begin(),
                 reversed_collision_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_collision_scenario.sheep_count));
    const PaddockCollisionHandle reversed_collision_run = run_paddock_collision(
        reversed_collision_scenario, kPaddockCollisionTicks, kPaddockCollisionMidpointTick);
    for (const auto& member :
         active(closed_gate_run->final_snapshot.sheep,
                closed_gate_run->final_snapshot.sheep_count)) {
        const SheepContactRecord& expected = contact_with_id(closed_gate_run->contacts, member.id);
        const SheepContactRecord& observed =
            contact_with_id(reversed_collision_run->contacts, member.id);
        if (!check(member == sheep_with_id(reversed_collision_run->final_snapshot.sheep, member.id),
                   "collision_result_is_stable_by_id_under_reversed_storage") ||
            !check(
                evidence_with_id(closed_gate_run->final_snapshot.sheep_collision_evidence,
                                 member.id) ==
                    evidence_with_id(reversed_collision_run->final_snapshot.sheep_collision_evidence,
                                     member.id),
                "collision_evidence_is_stable_under_reversed_storage") ||
            !check(expected.observed == observed.observed && expected.tick == observed.tick &&
                       expected.contact_ticks == observed.contact_ticks &&
                       expected.state == observed.state && expected.evidence == observed.evidence,
                   "first_contact_is_stable_by_id_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const SimulationHandle collision_dump = make_simulation(driven_closed_scenario);
    for (std::uint64_t tick = 0; tick < 100; ++tick) {
        collision_dump->fixed_update({});
    }
    const auto collision_state = wide_eye::game::gameplay_state_dump_json(*collision_dump);
    if (!check(
            collision_state &&
                collision_state.text.find("\"sheep_collision_evidence\":[") != std::string::npos &&
                collision_state.text.find("\"clipped_z\":true") != std::string::npos &&
                collision_state.text.find("\"contact_obstacle\":\"gate\"") != std::string::npos &&
                collision_state.text.find("\"contact_obstacle\":\"none\"") != std::string::npos,
            "state_dump_contains_sheep_collision_contact_and_obstacle")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle allocation_collision = make_simulation(*collision_closed_scenario);
    const std::size_t collision_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_collision->fixed_update({});
    }
    const std::size_t collision_allocations = g_allocation_count - collision_allocations_before;
    if (!check(collision_allocations == 0, "collision_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle collision_restart = make_simulation(*collision_closed_scenario);
    const auto collision_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(collision_restart->current_snapshot());
    const wide_eye::game::GameplaySnapshot& collision_initial = *collision_initial_holder;
    for (std::uint64_t tick = 0; tick < 120; ++tick) {
        collision_restart->fixed_update({});
    }
    collision_restart->restart();
    if (!check(collision_restart->current_snapshot() == collision_initial &&
                   collision_restart->previous_snapshot() == collision_initial,
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
    const ScenarioHandle temperament_neutral_scenario =
        named_scenario("sheep-temperament-neutral");
    const ScenarioHandle temperament_varied_scenario =
        named_scenario("sheep-temperament-varied");
    auto temperament_varied_as_control = mutable_scenario_copy(temperament_varied_scenario);
    if (temperament_neutral_scenario != nullptr) {
        temperament_varied_as_control->id = temperament_neutral_scenario->id;
    }
    temperament_varied_as_control->sheep_temperament.enabled = false;
    if (!check(
            temperament_neutral_scenario != nullptr && temperament_varied_scenario != nullptr &&
                temperament_neutral_scenario->id ==
                    wide_eye::game::GameplayScenarioId::sheep_temperament_neutral &&
                temperament_varied_scenario->id ==
                    wide_eye::game::GameplayScenarioId::sheep_temperament_varied &&
                *temperament_varied_as_control == *temperament_neutral_scenario &&
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

    const SimulationHandle temperament_neutral = make_simulation(*temperament_neutral_scenario);
    const SimulationHandle temperament_varied = make_simulation(*temperament_varied_scenario);
    const auto temperament_initial_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(temperament_varied->current_snapshot());
    const wide_eye::game::GameplaySnapshot& temperament_initial = *temperament_initial_holder;
    temperament_neutral->fixed_update({});
    temperament_varied->fixed_update({});
    const auto& temperament_neutral_after_one = temperament_neutral->current_snapshot();
    const auto& temperament_varied_after_one = temperament_varied->current_snapshot();
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
    if (!check(temperament_varied->previous_snapshot() == temperament_initial,
               "temperament_reads_immutable_prior_snapshot")) {
        return EXIT_FAILURE;
    }

    for (const auto& varied_evidence :
         active(temperament_varied_after_one.sheep_dog_pressure_evidence,
                temperament_varied_after_one.sheep_count)) {
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
        const auto varied_expected = bounded_terms(
            varied_evidence.pressure_acceleration,
            evidence_with_id(temperament_varied_after_one.sheep_combined_influence_evidence,
                             subject_id));
        const auto neutral_expected = bounded_terms(
            neutral_evidence.pressure_acceleration,
            evidence_with_id(temperament_neutral_after_one.sheep_combined_influence_evidence,
                             subject_id));
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
            !check(std::abs(varied_applied.x - varied_expected.x) < 1.0e-12 &&
                       std::abs(varied_applied.z - varied_expected.z) < 1.0e-12 &&
                       std::abs(neutral_applied.x - neutral_expected.x) < 1.0e-12 &&
                       std::abs(neutral_applied.z - neutral_expected.z) < 1.0e-12,
                   "published_temperament_terms_match_bounded_applied_acceleration")) {
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
    const auto all_ordinary_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*temperament_varied_scenario);
    auto& all_ordinary_scenario = *all_ordinary_scenario_holder;
    for (std::size_t index = 0; index < all_ordinary_scenario.sheep_count; ++index) {
        wide_eye::game::SheepState& member = all_ordinary_scenario.initial_sheep[index];
        member.temperament = wide_eye::game::SheepTemperament::ordinary;
    }
    auto all_ordinary_control_scenario = all_ordinary_scenario;
    all_ordinary_control_scenario.id = temperament_neutral_scenario->id;
    all_ordinary_control_scenario.sheep_temperament.enabled = false;
    const SimulationHandle all_ordinary = make_simulation(all_ordinary_scenario);
    const SimulationHandle all_ordinary_control = make_simulation(all_ordinary_control_scenario);
    bool all_ordinary_is_neutral = true;
    for (std::uint64_t tick = 0; tick < kTemperamentDriftTicks; ++tick) {
        all_ordinary->fixed_update({});
        all_ordinary_control->fixed_update({});
        all_ordinary_is_neutral =
            all_ordinary_is_neutral &&
            all_ordinary->current_snapshot() == all_ordinary_control->current_snapshot();
    }
    if (!check(all_ordinary_is_neutral,
               "an_all_ordinary_flock_is_identical_with_the_factor_on_or_off")) {
        return EXIT_FAILURE;
    }

    // The vector-level ratio should also be visible as motion: over the same
    // ticks, from the same bearing, the nervous sheep must end further from the
    // dog than its mirrored stubborn twin, with neither having touched anything
    // so the comparison stays pure steering.
    const SimulationHandle temperament_drift = make_simulation(*temperament_varied_scenario);
    bool temperament_drift_is_contact_free = true;
    for (std::uint64_t tick = 0; tick < kTemperamentDriftTicks; ++tick) {
        temperament_drift->fixed_update({});
        for (const auto& contact : temperament_drift->current_snapshot().sheep_collision_evidence) {
            temperament_drift_is_contact_free =
                temperament_drift_is_contact_free && !contact.clipped_x && !contact.clipped_z &&
                contact.obstacle == wide_eye::game::PaddockObstacle::none;
        }
    }
    const auto& drift_dog = temperament_drift->current_snapshot().dog;
    const auto dog_range = [&drift_dog](const wide_eye::game::SheepState& member) {
        return std::hypot(member.position.x - drift_dog.position.x,
                          member.position.z - drift_dog.position.z);
    };
    const double nervous_range =
        dog_range(sheep_with_id(temperament_drift->current_snapshot().sheep, 2));
    const double stubborn_range =
        dog_range(sheep_with_id(temperament_drift->current_snapshot().sheep, 3));
    if (!check(temperament_drift_is_contact_free && stubborn_range > kTemperamentRingDistance &&
                   nervous_range > stubborn_range,
               "a_nervous_sheep_outruns_its_mirrored_stubborn_twin")) {
        return EXIT_FAILURE;
    }

    // Temperament scales the whole dog response, not only the distance term, so
    // a derived fixture that also enables approach and facing must scale all
    // three vectors by the same published factor while the stimulus that
    // produced them stays identical.
    const auto combined_temperament_varied_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*temperament_varied_scenario);
    auto& combined_temperament_varied_scenario = *combined_temperament_varied_scenario_holder;
    combined_temperament_varied_scenario.dog.initial_state.velocity = {.z = -3.0};
    combined_temperament_varied_scenario.sheep_dog_approach.enabled = true;
    combined_temperament_varied_scenario.sheep_dog_facing.enabled = true;
    auto combined_temperament_neutral_scenario = combined_temperament_varied_scenario;
    combined_temperament_neutral_scenario.id = temperament_neutral_scenario->id;
    combined_temperament_neutral_scenario.sheep_temperament.enabled = false;
    const SimulationHandle combined_temperament_varied =
        make_simulation(combined_temperament_varied_scenario);
    const SimulationHandle combined_temperament_neutral =
        make_simulation(combined_temperament_neutral_scenario);
    combined_temperament_varied->fixed_update({});
    combined_temperament_neutral->fixed_update({});
    bool combined_temperament_scales_every_term = true;
    for (const std::uint32_t subject_id : {1U, 2U, 3U, 4U, 5U}) {
        const auto& varied_evidence = evidence_with_id(
            combined_temperament_varied->current_snapshot().sheep_dog_pressure_evidence,
            subject_id);
        const auto& neutral_evidence = evidence_with_id(
            combined_temperament_neutral->current_snapshot().sheep_dog_pressure_evidence,
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
        combined_temperament_varied->current_snapshot().sheep_dog_pressure_evidence, 1);
    const auto& combined_diagonal = evidence_with_id(
        combined_temperament_varied->current_snapshot().sheep_dog_pressure_evidence, 2);
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
    const auto social_temperament_varied_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*temperament_varied_scenario);
    auto& social_temperament_varied_scenario = *social_temperament_varied_scenario_holder;
    social_temperament_varied_scenario.sheep_separation.enabled = true;
    social_temperament_varied_scenario.sheep_attraction.enabled = true;
    social_temperament_varied_scenario.sheep_alignment.enabled = true;
    auto social_temperament_neutral_scenario = social_temperament_varied_scenario;
    social_temperament_neutral_scenario.id = temperament_neutral_scenario->id;
    social_temperament_neutral_scenario.sheep_temperament.enabled = false;
    const SimulationHandle social_temperament_varied =
        make_simulation(social_temperament_varied_scenario);
    const SimulationHandle social_temperament_neutral =
        make_simulation(social_temperament_neutral_scenario);
    social_temperament_varied->fixed_update({});
    social_temperament_neutral->fixed_update({});
    bool temperament_leaves_social_terms_alone = true;
    for (const auto& varied_social :
         social_temperament_varied->current_snapshot().sheep_social_evidence) {
        temperament_leaves_social_terms_alone =
            temperament_leaves_social_terms_alone &&
            varied_social ==
                evidence_with_id(
                    social_temperament_neutral->current_snapshot().sheep_social_evidence,
                    varied_social.subject_id);
    }
    const auto& social_ordinary_evidence =
        evidence_with_id(social_temperament_varied->current_snapshot().sheep_social_evidence, 1);
    const auto& social_nervous_dog = evidence_with_id(
        social_temperament_varied->current_snapshot().sheep_dog_pressure_evidence, 2);
    const auto& social_neutral_dog = evidence_with_id(
        social_temperament_neutral->current_snapshot().sheep_dog_pressure_evidence, 2);
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
    const SimulationHandle temperament_same_tick_move =
        make_simulation(*temperament_varied_scenario);
    temperament_same_tick_move->fixed_update(
        {.dog_move = wide_eye::game::DogMoveInput{.world_z = -1.0}});
    if (!check(temperament_same_tick_move->current_snapshot().dog.position.z <
                       temperament_varied_scenario->dog.initial_state.position.z &&
                   evidence_with_id(
                       temperament_same_tick_move->current_snapshot().sheep_dog_pressure_evidence,
                       2) == near_nervous_pressure,
               "same_tick_dog_motor_move_does_not_alter_prior_state_temperament_evidence")) {
        return EXIT_FAILURE;
    }

    const auto reversed_temperament_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*temperament_varied_scenario);
    auto& reversed_temperament_scenario = *reversed_temperament_scenario_holder;
    std::reverse(reversed_temperament_scenario.initial_sheep.begin(),
                 reversed_temperament_scenario.initial_sheep.begin() +
                     static_cast<std::ptrdiff_t>(reversed_temperament_scenario.sheep_count));
    const SimulationHandle reversed_temperament = make_simulation(reversed_temperament_scenario);
    reversed_temperament->fixed_update({});
    for (const auto& member :
         active(temperament_varied_after_one.sheep, temperament_varied_after_one.sheep_count)) {
        if (!check(member ==
                       sheep_with_id(reversed_temperament->current_snapshot().sheep, member.id),
                   "temperament_result_is_stable_by_id_under_reversed_storage") ||
            !check(evidence_with_id(temperament_varied_after_one.sheep_dog_pressure_evidence,
                                    member.id) ==
                       evidence_with_id(
                           reversed_temperament->current_snapshot().sheep_dog_pressure_evidence,
                           member.id),
                   "temperament_evidence_is_stable_under_reversed_storage")) {
            return EXIT_FAILURE;
        }
    }

    const auto temperament_state = wide_eye::game::gameplay_state_dump_json(*temperament_varied);
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

    const auto unknown_temperament_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*temperament_varied_scenario);
    auto& unknown_temperament_scenario = *unknown_temperament_scenario_holder;
    unknown_temperament_scenario.initial_sheep[0].temperament =
        static_cast<wide_eye::game::SheepTemperament>(255);
    const ConstSimulationHandle unknown_temperament_simulation =
        make_simulation(unknown_temperament_scenario);
    if (!check(wide_eye::game::gameplay_state_dump_json(*unknown_temperament_simulation).error ==
                   wide_eye::game::GameplayContractError::non_finite_state,
               "state_dump_rejects_unknown_sheep_temperament")) {
        return EXIT_FAILURE;
    }

    const SimulationHandle allocation_temperament = make_simulation(*temperament_varied_scenario);
    const std::size_t temperament_allocations_before = g_allocation_count;
    for (std::uint32_t tick = 0; tick < 600; ++tick) {
        allocation_temperament->fixed_update({});
    }
    const std::size_t temperament_allocations = g_allocation_count - temperament_allocations_before;
    if (!check(temperament_allocations == 0, "temperament_fixed_updates_do_not_allocate")) {
        return EXIT_FAILURE;
    }

    for (std::uint32_t tick = 0; tick < 120; ++tick) {
        temperament_varied->fixed_update({});
    }
    temperament_varied->restart();
    bool temperament_restart_restores_labels = true;
    for (const auto& member : temperament_varied->current_snapshot().sheep) {
        temperament_restart_restores_labels =
            temperament_restart_restores_labels &&
            member.temperament == sheep_with_id(temperament_fixture, member.id).temperament;
    }
    if (!check(temperament_varied->current_snapshot() == temperament_initial &&
                   temperament_varied->previous_snapshot() == temperament_initial &&
                   temperament_restart_restores_labels,
               "temperament_restart_restores_the_fixture_including_labels")) {
        return EXIT_FAILURE;
    }

    // The combined-influence oracle owns its own frame. `GameplaySimulation` is
    // ~114 KiB because of the spatial grid's capacity-experiment ceiling, and
    // this `main` already keeps roughly seventy of them alive at once, so a new
    // paired fixture declared here would overflow the default 8 MiB stack.
    const CombinedInfluenceOracle combined_influence = run_combined_influence_oracle(*scenario);
    if (!combined_influence.passed) {
        return EXIT_FAILURE;
    }

    // The motion-limit oracle owns its own frame for the same reason.
    const MotionLimitOracle motion_limit = run_motion_limit_oracle();
    if (!motion_limit.passed) {
        return EXIT_FAILURE;
    }

    // The avoidance oracle owns its own frame for the same reason.
    const AvoidanceOracle avoidance = run_avoidance_oracle();
    if (!avoidance.passed) {
        return EXIT_FAILURE;
    }

    // The QA-003 contact-face regression owns its own frame for the same reason.
    const AvoidanceContactOracle avoidance_contact = run_avoidance_contact_oracle();
    if (!avoidance_contact.passed) {
        return EXIT_FAILURE;
    }

    // The QA-001 radius-band regression owns its own frame for the same reason.
    const BandPassthroughOracle band_passthrough = run_band_passthrough_oracle();
    if (!band_passthrough.passed) {
        return EXIT_FAILURE;
    }

    // The behavior-transition oracle owns its own frame for the same reason.
    const BehaviorTransitionOracle behavior = run_behavior_transition_oracle();
    if (!behavior.passed) {
        return EXIT_FAILURE;
    }

    // The flock-response oracle owns its own frame for the same reason.
    const FlockResponseOracle flock_response = run_flock_response_oracle();
    if (!flock_response.passed) {
        return EXIT_FAILURE;
    }

    // The randomness-and-stability oracle owns its own frame for the same
    // reason, and it sweeps every named scenario, so it holds more fixtures than
    // any other single check in this file.
    const SteeringStabilityOracle stability = run_steering_stability_oracle();
    if (!stability.passed) {
        return EXIT_FAILURE;
    }

    // The scale fixture owns its own frame for the same reason: it holds two
    // fifty-member simulations and a state dump of both their snapshots.
    const FlockScaleOracle flock_scale = run_flock_scale_oracle();
    if (!flock_scale.passed) {
        return EXIT_FAILURE;
    }

    const SimulationHandle replay_a = make_simulation(*scenario);
    const SimulationHandle replay_b = make_simulation(*scenario);
    const wide_eye::game::GameplayReplay replay = sample_replay(*replay_a);
    const auto replay_text = wide_eye::game::gameplay_replay_json(replay);
    if (!check(wide_eye::game::kGameplaySeedFormatVersion == 1 &&
                   wide_eye::game::kGameplayActionInputFormatVersion == 1 &&
                   wide_eye::game::kGameplayReplayFormatVersion == 1 &&
                   wide_eye::game::kGameplayStateDumpFormatVersion == 15,
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
        !check(wide_eye::game::apply_gameplay_replay(*replay_a, replay) ==
                       wide_eye::game::GameplayContractError::none &&
                   wide_eye::game::apply_gameplay_replay(*replay_b, replay) ==
                       wide_eye::game::GameplayContractError::none &&
                   replay_a->current_snapshot() == replay_b->current_snapshot(),
               "repeated_local_replay_state_equal")) {
        return EXIT_FAILURE;
    }

    const auto state_a = wide_eye::game::gameplay_state_dump_json(*replay_a);
    const auto state_b = wide_eye::game::gameplay_state_dump_json(*replay_b);
    if (!check(state_a && state_b && state_a.text == state_b.text,
               "canonical_state_dump_repeats") ||
        !check(state_a.text.starts_with("{\"schema\":\"wide-eye.gameplay-state\",\"version\":15,"
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
    const auto before_rejection_holder =
        std::make_unique<wide_eye::game::GameplaySnapshot>(replay_a->current_snapshot());
    const wide_eye::game::GameplaySnapshot& before_rejection = *before_rejection_holder;
    if (!check(wide_eye::game::apply_gameplay_replay(*replay_a, incompatible) ==
                   wide_eye::game::GameplayContractError::unsupported_replay_version,
               "unsupported_replay_version_rejected") ||
        !check(replay_a->current_snapshot() == before_rejection,
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
    const SimulationHandle fresh_simulation = make_simulation(*scenario);
    if (!check(wide_eye::game::validate_gameplay_replay(incompatible, *fresh_simulation) ==
                   wide_eye::game::GameplayContractError::scenario_mismatch,
               "scenario_seed_mismatch_rejected")) {
        return EXIT_FAILURE;
    }
    if (!check(wide_eye::game::validate_gameplay_replay(replay, *replay_a) ==
                   wide_eye::game::GameplayContractError::simulation_not_at_replay_start,
               "replay_requires_initial_tick")) {
        return EXIT_FAILURE;
    }

    const auto invalid_state_scenario_holder =
        std::make_unique<wide_eye::game::GameplayScenarioDefinition>(*scenario);
    auto& invalid_state_scenario = *invalid_state_scenario_holder;
    invalid_state_scenario.dog.initial_state.position.x = std::numeric_limits<double>::quiet_NaN();
    const ConstSimulationHandle invalid_state_simulation = make_simulation(invalid_state_scenario);
    if (!check(wide_eye::game::gameplay_state_dump_json(*invalid_state_simulation).error ==
                   wide_eye::game::GameplayContractError::non_finite_state,
               "state_dump_rejects_non_finite_json")) {
        return EXIT_FAILURE;
    }
    invalid_state_scenario = *scenario;
    invalid_state_scenario.id = static_cast<wide_eye::game::GameplayScenarioId>(255);
    const ConstSimulationHandle unknown_state_simulation = make_simulation(invalid_state_scenario);
    if (!check(wide_eye::game::gameplay_state_dump_json(*unknown_state_simulation).error ==
                   wide_eye::game::GameplayContractError::unknown_scenario,
               "state_dump_rejects_unknown_scenario")) {
        return EXIT_FAILURE;
    }

    std::cout
        << "authoritative_tick_hz=" << wide_eye::game::GameplaySimulation::kTicksPerSecond << '\n'
        << "fine_render_frames=" << fine_frames.size() << '\n'
        << "coarse_render_frames=" << coarse_frames.size() << '\n'
        << "authoritative_ticks=" << fine->snapshot.tick << '\n'
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
        << sheep_with_id(closed_gate_run->final_snapshot.sheep, 4).position.x << '\n'
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
        << "sheep_combined_influence_fixture=overlapping_influence_paired_control\n"
        << "sheep_combined_influence_bound=" << combined_influence.bound << '\n'
        << "sheep_combined_influence_over_bound_summed_magnitude="
        << combined_influence.over_bound.summed_acceleration_magnitude << '\n'
        << "sheep_combined_influence_over_bound_applied_scale="
        << combined_influence.over_bound.applied_scale << '\n'
        << "sheep_combined_influence_over_bound_applied_z="
        << combined_influence.over_bound.applied_acceleration.z << '\n'
        << "sheep_combined_influence_control_applied_z="
        << combined_influence.over_bound_control.applied_acceleration.z << '\n'
        << "sheep_combined_influence_under_bound_summed_magnitude="
        << combined_influence.under_bound.summed_acceleration_magnitude << '\n'
        << "sheep_combined_influence_under_bound_applied_scale="
        << combined_influence.under_bound.applied_scale << '\n'
        << "sheep_combined_influence_idle_applied_scale="
        << combined_influence.idle_bound.applied_scale << '\n'
        << "sheep_combined_influence_diagonal_summed_magnitude="
        << combined_influence.diagonal_bound.summed_acceleration_magnitude << '\n'
        << "sheep_combined_influence_diagonal_applied_scale="
        << combined_influence.diagonal_bound.applied_scale << '\n'
        << "sheep_combined_influence_diagonal_applied_magnitude="
        << std::hypot(combined_influence.diagonal_bound.applied_acceleration.x,
                      combined_influence.diagonal_bound.applied_acceleration.z)
        << '\n'
        << "sheep_combined_influence_drift_ticks=" << combined_influence.drift_ticks << '\n'
        << "sheep_combined_influence_bounded_drift_z=" << combined_influence.bounded_drift_z << '\n'
        << "sheep_combined_influence_unbounded_drift_z=" << combined_influence.unbounded_drift_z
        << '\n'
        << "sheep_combined_influence_steady_state_allocations=" << combined_influence.allocations
        << '\n'
        << "sheep_motion_limit_fixture=exact_velocity_paired_control\n"
        << "sheep_motion_limit_maximum_speed=" << motion_limit.maximum_speed << '\n'
        << "sheep_motion_limit_maximum_turn_rate=" << motion_limit.maximum_turn_rate << '\n'
        << "sheep_motion_limit_turn_budget_radians=" << motion_limit.turn_budget << '\n'
        << "sheep_motion_limit_heading_floor=" << wide_eye::game::kSheepHeadingMotionSpeedFloor
        << '\n'
        << "sheep_motion_limit_axis_integrated_speed=" << motion_limit.axis_clamp.integrated_speed
        << '\n'
        << "sheep_motion_limit_axis_applied_scale=" << motion_limit.axis_clamp.applied_speed_scale
        << '\n'
        << "sheep_motion_limit_axis_applied_speed=" << motion_limit.axis_clamp.applied_speed << '\n'
        << "sheep_motion_limit_unlimited_axis_speed=" << motion_limit.unlimited_axis_speed << '\n'
        << "sheep_motion_limit_diagonal_applied_scale="
        << motion_limit.diagonal_clamp.applied_speed_scale << '\n'
        << "sheep_motion_limit_diagonal_applied_speed=" << motion_limit.diagonal_clamp.applied_speed
        << '\n'
        << "sheep_motion_limit_under_limit_applied_scale="
        << motion_limit.under_limit.applied_speed_scale << '\n'
        << "sheep_motion_limit_stationary_heading_followed="
        << (motion_limit.stationary.motion_heading_followed ? "yes" : "no") << '\n'
        << "sheep_motion_limit_reversal_motion_heading="
        << motion_limit.reversal.motion_heading_radians << '\n'
        << "sheep_motion_limit_reversal_first_change="
        << motion_limit.reversal.heading_change_radians << '\n'
        << "sheep_motion_limit_reversal_budget_ticks=" << motion_limit.reversal_ticks << '\n'
        << "sheep_motion_limit_reversal_heading_before_completion="
        << motion_limit.reversal_heading_before_completion << '\n'
        << "sheep_motion_limit_reversal_completion_tick=" << motion_limit.reversal_completion_tick
        << '\n'
        << "sheep_motion_limit_drift_ticks=" << motion_limit.drift_ticks << '\n'
        << "sheep_motion_limit_accumulation_maximum=" << motion_limit.accumulation_maximum << '\n'
        << "sheep_motion_limit_accumulation_unlimited_peak=" << motion_limit.unlimited_peak_speed
        << '\n'
        << "sheep_motion_limit_accumulation_limited_peak=" << motion_limit.limited_peak_speed
        << '\n'
        << "sheep_motion_limit_turn_limits_motion=no\n"
        << "sheep_motion_limit_steady_state_allocations=" << motion_limit.allocations << '\n'
        << "sheep_avoidance_fixture=exact_velocity_paired_control\n"
        << "sheep_avoidance_look_ahead=" << avoidance.look_ahead << '\n'
        << "sheep_avoidance_maximum_acceleration=" << avoidance.maximum_acceleration << '\n'
        << "sheep_avoidance_probe_distance=" << avoidance.probe_distance << '\n'
        << "sheep_avoidance_wall_head_on_obstacle_distance="
        << avoidance.wall_head_on.obstacle_distance << '\n'
        << "sheep_avoidance_wall_head_on_x=" << avoidance.wall_head_on.avoidance_acceleration.x
        << '\n'
        << "sheep_avoidance_wall_head_on_z=" << avoidance.wall_head_on.avoidance_acceleration.z
        << '\n'
        << "sheep_avoidance_wall_near_end_x=" << avoidance.wall_near_end.avoidance_acceleration.x
        << '\n'
        << "sheep_avoidance_wall_near_end_z=" << avoidance.wall_near_end.avoidance_acceleration.z
        << '\n'
        << "sheep_avoidance_closed_gate_z=" << avoidance.closed_gate.avoidance_acceleration.z
        << '\n'
        << "sheep_avoidance_drop_ahead=" << (avoidance.drop.drop_ahead ? "yes" : "no") << '\n'
        << "sheep_avoidance_drop_x=" << avoidance.drop.avoidance_acceleration.x << '\n'
        << "sheep_avoidance_parallel_magnitude="
        << std::hypot(avoidance.parallel.avoidance_acceleration.x,
                      avoidance.parallel.avoidance_acceleration.z)
        << '\n'
        << "sheep_avoidance_contact_window_ticks=" << avoidance.contact_ticks << '\n'
        << "sheep_avoidance_on_contact_ticks=" << avoidance.on_contacts << '\n'
        << "sheep_avoidance_off_contact_ticks=" << avoidance.off_contacts << '\n'
        << "sheep_avoidance_off_first_wall_contact_tick=" << avoidance.off_first_wall_contact_tick
        << '\n'
        << "sheep_avoidance_off_first_drop_contact_tick=" << avoidance.off_first_drop_contact_tick
        << '\n'
        << "sheep_avoidance_on_closest_wall_gap=" << avoidance.on_closest_wall_gap << '\n'
        << "sheep_avoidance_on_steered_x=" << avoidance.on_deflected_x << '\n'
        << "sheep_avoidance_off_pinned_x=" << avoidance.off_deflected_x << '\n'
        << "sheep_avoidance_on_drop_rest_x=" << avoidance.on_drop_rest_x << '\n'
        << "sheep_avoidance_off_drop_rest_x=" << avoidance.off_drop_rest_x << '\n'
        << "sheep_avoidance_overwhelmed_maximum=" << avoidance.overwhelmed_maximum << '\n'
        << "sheep_avoidance_overwhelmed_contact_tick=" << avoidance.overwhelmed_contact_tick << '\n'
        << "sheep_avoidance_overwhelmed_rest_z=" << avoidance.overwhelmed_rest_z << '\n'
        << "sheep_avoidance_combined_bound=" << avoidance.bounded_maximum << '\n'
        << "sheep_avoidance_combined_summed_magnitude="
        << avoidance.bounded_drop.summed_acceleration_magnitude << '\n'
        << "sheep_avoidance_combined_applied_scale=" << avoidance.bounded_drop.applied_scale << '\n'
        << "sheep_avoidance_combined_applied_x=" << avoidance.bounded_drop.applied_acceleration.x
        << '\n'
        << "sheep_avoidance_replaces_hard_collision=no\n"
        << "sheep_avoidance_verified_drop=paddock_edge_only\n"
        << "sheep_avoidance_steady_state_allocations=" << avoidance.allocations << '\n'
        << "sheep_avoidance_contact_face_fixture=dog_pressed_against_the_wall_line\n"
        << "sheep_avoidance_contact_face_ticks=" << avoidance_contact.ticks << '\n'
        << "sheep_avoidance_contact_face_line_z=" << avoidance_contact.contact_line_z << '\n'
        << "sheep_avoidance_contact_face_clip_ticks=" << avoidance_contact.clip_ticks << '\n'
        << "sheep_avoidance_contact_face_parallel_obstacle="
        << obstacle_name(avoidance_contact.parallel_contact.obstacle) << '\n'
        << "sheep_avoidance_contact_face_parallel_magnitude="
        << std::hypot(avoidance_contact.parallel_contact.avoidance_acceleration.x,
                      avoidance_contact.parallel_contact.avoidance_acceleration.z)
        << '\n'
        << "sheep_avoidance_contact_face_just_clear_magnitude="
        << std::hypot(avoidance_contact.just_clear.avoidance_acceleration.x,
                      avoidance_contact.just_clear.avoidance_acceleration.z)
        << '\n'
        << "sheep_avoidance_contact_face_into_face_obstacle="
        << obstacle_name(avoidance_contact.into_face.obstacle) << '\n'
        << "sheep_avoidance_contact_face_into_face_distance="
        << avoidance_contact.into_face.obstacle_distance << '\n'
        << "sheep_avoidance_contact_face_into_face_magnitude="
        << avoidance_contact.into_face_magnitude << '\n'
        << "sheep_avoidance_contact_face_minimum_published_distance="
        << avoidance_contact.minimum_published_distance << '\n'
        << "sheep_avoidance_contact_face_parallel_flaps=" << avoidance_contact.parallel_flaps
        << '\n'
        << "sheep_avoidance_contact_face_turning_flaps=" << avoidance_contact.turning_flaps << '\n'
        << "sheep_avoidance_contact_face_grazing_flaps=" << avoidance_contact.grazing_flaps << '\n'
        << "sheep_avoidance_contact_face_grazing_flap_allowance="
        << avoidance_contact.grazing_flap_allowance << '\n'
        << "sheep_avoidance_contact_face_drop_at_bound="
        << (avoidance_contact.drop_at_bound.drop_ahead ? "yes" : "no") << '\n'
        << "sheep_avoidance_contact_face_drop_past_bound="
        << (avoidance_contact.drop_past_bound.drop_ahead ? "yes" : "no") << '\n'
        << "paddock_band_passthrough_on_face_z=" << band_passthrough.on_the_face_z << '\n'
        << "paddock_band_passthrough_inside_band_z=" << band_passthrough.inside_the_band_z << '\n'
        << "paddock_band_passthrough_at_one_radius_z=" << band_passthrough.at_one_radius_z << '\n'
        << "paddock_band_passthrough_a_radius_clear_z=" << band_passthrough.a_radius_clear_z << '\n'
        << "paddock_band_passthrough_fully_inside=" << band_passthrough.fully_inside.x << ','
        << band_passthrough.fully_inside.z << '\n'
        << "paddock_band_passthrough_axis_tie=" << band_passthrough.axis_tie.x << ','
        << band_passthrough.axis_tie.z << '\n'
        << "paddock_band_passthrough_shape_tie=" << band_passthrough.shape_tie.x << ','
        << band_passthrough.shape_tie.z << '\n'
        << "paddock_band_passthrough_two_shape_wedge=" << band_passthrough.two_shape_wedge.x << ','
        << band_passthrough.two_shape_wedge.z << '\n'
        << "paddock_band_passthrough_dog_on_face_z=" << band_passthrough.dog_on_the_face_z << '\n'
        << "paddock_band_passthrough_dog_at_one_radius_z=" << band_passthrough.dog_at_one_radius_z
        << '\n'
        << "paddock_band_passthrough_witness_ticks=" << band_passthrough.witness_ticks << '\n'
        << "paddock_band_passthrough_witness_overlap_ticks="
        << band_passthrough.witness_overlap_ticks << '\n'
        << "paddock_band_passthrough_witness_minimum_z=" << band_passthrough.witness_minimum_z
        << '\n'
        << "sheep_behavior_fixture=exact_stimulus_curve_paired_control\n"
        << "sheep_behavior_arousal_is_physiological=no\n"
        << "sheep_behavior_arousal_range=[" << wide_eye::game::kSheepMinimumArousal << ','
        << wide_eye::game::kSheepMaximumArousal << "]\n"
        << "sheep_behavior_rise_rate=" << behavior.rise_rate << '\n'
        << "sheep_behavior_recovery_rate=" << behavior.recovery_rate << '\n'
        << "sheep_behavior_rise_step=" << behavior.rise_step << '\n'
        << "sheep_behavior_recovery_step=" << behavior.recovery_step << '\n'
        << "sheep_behavior_rest_arousal=" << behavior.rest_arousal << '\n'
        << "sheep_behavior_alert_arousal=" << behavior.alert_arousal << '\n'
        << "sheep_behavior_driven_release_arousal=" << behavior.driven_release_arousal << '\n'
        << "sheep_behavior_driven_arousal=" << behavior.driven_arousal << '\n'
        << "sheep_behavior_stimulus_radius=" << behavior.stimulus_radius << '\n'
        << "sheep_behavior_cycle_ticks=" << behavior.cycle_ticks << '\n'
        << "sheep_behavior_alert_tick=" << behavior.alert_tick << '\n'
        << "sheep_behavior_alert_prior_arousal=" << behavior.alert_prior_arousal << '\n'
        << "sheep_behavior_driven_tick=" << behavior.driven_tick << '\n'
        << "sheep_behavior_driven_prior_arousal=" << behavior.driven_prior_arousal << '\n'
        << "sheep_behavior_recovering_tick=" << behavior.recovering_tick << '\n'
        << "sheep_behavior_recovering_prior_arousal=" << behavior.recovering_prior_arousal << '\n'
        << "sheep_behavior_recovering_release_stimulus=" << behavior.recovering_release_stimulus
        << '\n'
        << "sheep_behavior_settled_tick=" << behavior.settled_tick << '\n'
        << "sheep_behavior_settled_prior_arousal=" << behavior.settled_prior_arousal << '\n'
        << "sheep_behavior_peak_arousal=" << behavior.peak_arousal << '\n'
        << "sheep_behavior_control_arousal=" << behavior.control_arousal << '\n'
        << "sheep_behavior_band_stimulus=" << behavior.band_stimulus << '\n'
        << "sheep_behavior_band_arousal=" << behavior.band_arousal << '\n'
        << "sheep_behavior_band_alert_label_changes=" << behavior.band_alert_changes << '\n'
        << "sheep_behavior_band_driven_label_changes=" << behavior.band_driven_changes << '\n'
        << "sheep_behavior_boundary_stimulus=" << behavior.boundary_stimulus << '\n'
        << "sheep_behavior_boundary_arousal=" << behavior.boundary_arousal << '\n'
        << "sheep_behavior_boundary_driven_ticks=" << behavior.boundary_driven_ticks << '\n'
        << "sheep_behavior_stubborn_stimulus=" << behavior.stubborn_stimulus << '\n'
        << "sheep_behavior_stubborn_arousal=" << behavior.stubborn_arousal << '\n'
        << "sheep_behavior_clamped_stimulus=" << behavior.clamped_stimulus << '\n'
        << "sheep_behavior_adversarial_ticks=" << behavior.adversarial_ticks << '\n'
        << "sheep_behavior_adversarial_threshold_crossings=" << behavior.adversarial_threshold_flips
        << '\n'
        << "sheep_behavior_adversarial_ticks_above=" << behavior.adversarial_above << '\n'
        << "sheep_behavior_adversarial_ticks_below=" << behavior.adversarial_below << '\n'
        << "sheep_behavior_adversarial_label_changes=" << behavior.adversarial_changes << '\n'
        << "sheep_behavior_adversarial_late_label_changes=" << behavior.adversarial_late_changes
        << '\n'
        << "sheep_behavior_scripted_dog_alert_tick=" << behavior.scripted_alert_tick << '\n'
        << "sheep_behavior_scripted_dog_driven_tick=" << behavior.scripted_driven_tick << '\n'
        << "sheep_behavior_scripted_dog_recovering_tick=" << behavior.scripted_recovering_tick
        << '\n'
        << "sheep_behavior_scripted_dog_settled_tick=" << behavior.scripted_settled_tick << '\n'
        << "sheep_behavior_scripted_dog_peak_arousal=" << behavior.scripted_peak_arousal << '\n'
        << "sheep_behavior_feeds_back_into_steering=no\n"
        << "sheep_behavior_steady_state_allocations=" << behavior.allocations << '\n'
        << "flock_response_fixture=behavior_transitions_scripted_dog_and_passing_sheep\n"
        << "flock_response_feeds_back_into_steering=no\n"
        << "flock_response_connectivity_distance=" << flock_response.connectivity_distance << '\n'
        << "flock_response_rest_arousal=" << flock_response.rest_arousal << '\n'
        << "flock_response_scripted_ticks=" << flock_response.scripted_ticks << '\n'
        << "flock_response_pressure_onset_tick=" << flock_response.pressure_onset_tick << '\n'
        << "flock_response_response_tick=" << flock_response.response_tick << '\n'
        << "flock_response_latency_ticks=" << flock_response.response_latency_ticks << '\n'
        << "flock_response_release_tick=" << flock_response.release_tick << '\n'
        << "flock_response_settled_tick=" << flock_response.settle_tick << '\n'
        << "flock_response_settle_ticks=" << flock_response.settle_ticks << '\n'
        << "flock_response_release_peak_arousal=" << flock_response.release_peak_arousal << '\n'
        << "flock_response_pressure_episodes=" << flock_response.pressure_episodes << '\n'
        << "flock_response_releases=" << flock_response.releases << '\n'
        << "flock_response_unanswered_pressure_episodes="
        << flock_response.unanswered_pressure_episodes << '\n'
        << "flock_response_interrupted_settles=" << flock_response.interrupted_settles << '\n'
        << "flock_response_scripted_split_episodes=" << flock_response.scripted_split_episodes
        << '\n'
        << "flock_response_scripted_rejoins=" << flock_response.scripted_rejoins << '\n'
        << "flock_response_scripted_ticks_split=" << flock_response.scripted_ticks_split << '\n'
        << "flock_response_onset_centroid_distance=" << flock_response.onset_dog.centroid_distance
        << '\n'
        << "flock_response_onset_centroid_bearing_radians="
        << flock_response.onset_dog.centroid_bearing_radians << '\n'
        << "flock_response_onset_nearest_sheep=" << flock_response.onset_dog.nearest_sheep_id
        << '\n'
        << "flock_response_onset_nearest_distance=" << flock_response.onset_dog.nearest_distance
        << '\n'
        << "flock_response_onset_rear_sheep=" << flock_response.onset_dog.rear_sheep_id << '\n'
        << "flock_response_onset_rear_distance=" << flock_response.onset_dog.rear_distance << '\n'
        << "flock_response_onset_rear_offset=" << flock_response.onset_dog.rear_offset << '\n'
        << "flock_response_closest_tick=" << flock_response.closest_tick << '\n'
        << "flock_response_closest_centroid_distance="
        << flock_response.closest_dog.centroid_distance << '\n'
        << "flock_response_closest_centroid_bearing_radians="
        << flock_response.closest_dog.centroid_bearing_radians << '\n'
        << "flock_response_closest_nearest_sheep=" << flock_response.closest_dog.nearest_sheep_id
        << '\n'
        << "flock_response_closest_nearest_distance=" << flock_response.closest_dog.nearest_distance
        << '\n'
        << "flock_response_closest_rear_sheep=" << flock_response.closest_dog.rear_sheep_id << '\n'
        << "flock_response_closest_rear_distance=" << flock_response.closest_dog.rear_distance
        << '\n'
        << "flock_response_closest_rear_offset=" << flock_response.closest_dog.rear_offset << '\n'
        << "flock_response_release_centroid_distance="
        << flock_response.release_dog.centroid_distance << '\n'
        << "flock_response_release_centroid_bearing_radians="
        << flock_response.release_dog.centroid_bearing_radians << '\n'
        << "flock_response_release_nearest_distance=" << flock_response.release_dog.nearest_distance
        << '\n'
        << "flock_response_passing_ticks=" << flock_response.passing_ticks << '\n'
        << "flock_response_passing_pressure_onset_tick="
        << flock_response.passing_pressure_onset_tick << '\n'
        << "flock_response_passing_latency_ticks=" << flock_response.passing_response_latency_ticks
        << '\n'
        << "flock_response_passing_rejoin_tick=" << flock_response.passing_rejoin_tick << '\n'
        << "flock_response_passing_rejoin_ticks=" << flock_response.passing_rejoin_ticks << '\n'
        << "flock_response_passing_second_split_tick=" << flock_response.passing_second_split_tick
        << '\n'
        << "flock_response_passing_time_to_split_ticks="
        << flock_response.passing_time_to_split_ticks << '\n'
        << "flock_response_passing_split_episodes=" << flock_response.passing_split_episodes << '\n'
        << "flock_response_passing_rejoins=" << flock_response.passing_rejoins << '\n'
        << "flock_response_passing_ticks_split=" << flock_response.passing_ticks_split << '\n'
        << "flock_response_steady_state_allocations=" << flock_response.allocations << '\n'
        << "sheep_steering_stability_fixture=all_influences_diagnostic\n"
        << "sheep_steering_stability_is_tuned_gameplay=no\n"
        << "sheep_steering_stability_simulation_contains_randomness=no\n"
        << "sheep_steering_stability_scenarios_swept=" << stability.scenarios << '\n'
        << "sheep_steering_stability_determinism_ticks_per_scenario=" << stability.determinism_ticks
        << '\n'
        << "sheep_steering_stability_tick_comparisons=" << stability.determinism_comparisons << '\n'
        << "sheep_steering_stability_seeds_compared=" << stability.seeds << '\n'
        << "sheep_steering_stability_run_ticks=" << stability.stability_ticks << '\n'
        << "sheep_steering_stability_sheep_samples=" << stability.sheep_samples << '\n'
        << "sheep_steering_stability_unexplained_acceleration_samples="
        << stability.unexplained_acceleration_samples << '\n'
        << "sheep_steering_stability_unexplained_scale_samples="
        << stability.unexplained_scale_samples << '\n'
        << "sheep_steering_stability_unexplained_velocity_samples="
        << stability.unexplained_velocity_samples << '\n'
        << "sheep_steering_stability_unexplained_position_samples="
        << stability.unexplained_position_samples << '\n'
        << "sheep_steering_stability_combined_bound_breaches=" << stability.combined_bound_breaches
        << '\n'
        << "sheep_steering_stability_term_bound_breaches=" << stability.term_bound_breaches << '\n'
        << "sheep_steering_stability_speed_breaches=" << stability.speed_breaches << '\n'
        << "sheep_steering_stability_turn_breaches=" << stability.turn_breaches << '\n'
        << "sheep_steering_stability_arousal_breaches=" << stability.arousal_breaches << '\n'
        << "sheep_steering_stability_non_finite_samples=" << stability.non_finite_samples << '\n'
        << "sheep_steering_stability_label_changes=" << stability.label_changes << '\n'
        << "sheep_steering_stability_label_change_allowance_per_hundred_ticks="
        << stability.label_change_allowance_per_hundred << '\n'
        << "sheep_steering_stability_minimum_label_dwell_ticks=" << stability.minimum_label_dwell
        << '\n'
        << "sheep_steering_stability_label_round_trips=" << stability.label_round_trips << '\n'
        << "sheep_steering_stability_fast_label_round_trips=" << stability.fast_label_round_trips
        << '\n'
        << "sheep_steering_stability_clipped_samples=" << stability.clipped_samples << '\n'
        << "sheep_steering_stability_drop_ahead_samples=" << stability.drop_ahead_samples << '\n'
        << "sheep_steering_stability_occluded_samples=" << stability.occluded_samples << '\n'
        << "sheep_steering_stability_closest_wall_gap=" << stability.closest_wall_gap << '\n'
        << "sheep_steering_stability_peak_summed_magnitude=" << stability.peak_summed_magnitude
        << '\n'
        << "sheep_steering_stability_peak_applied_magnitude=" << stability.peak_applied_magnitude
        << '\n'
        << "sheep_steering_stability_peak_speed=" << stability.peak_speed << '\n'
        << "sheep_steering_stability_peak_turn_radians=" << stability.peak_turn << '\n'
        << "sheep_steering_stability_peak_arousal=" << stability.peak_arousal << '\n'
        << "sheep_steering_stability_steady_state_allocations=" << stability.allocations << '\n'
        << "repeated_local_replay_equal=yes\n";
    for (std::size_t term = 0; term < kSteeringTermCount; ++term) {
        std::cout << "sheep_steering_stability_" << kSteeringTermNames[term]
                  << "_active_samples=" << stability.term_active_samples[term] << '\n'
                  << "sheep_steering_stability_" << kSteeringTermNames[term]
                  << "_worst_sheep_flaps=" << stability.term_flaps[term] << '\n'
                  << "sheep_steering_stability_" << kSteeringTermNames[term]
                  << "_worst_sheep_flap_run=" << stability.term_flap_runs[term] << '\n'
                  << "sheep_steering_stability_" << kSteeringTermNames[term]
                  << "_flap_allowance_per_hundred_ticks=" << stability.term_flap_allowance[term]
                  << '\n';
    }
    std::cout << "flock_scale_scenario=fifty-sheep-paddock\n"
              << "flock_scale_published_members=" << flock_scale.published_count << '\n'
              << "flock_scale_buffer_capacity=" << flock_scale.capacity << '\n'
              << "flock_scale_grid_capacity="
              << wide_eye::game::SheepSpatialGrid::kMaximumMemberCount << '\n'
              << "flock_scale_ticks=" << flock_scale.ticks << '\n'
              << "flock_scale_steady_state_allocations=" << flock_scale.allocations << '\n'
              << "flock_scale_minimum_start_separation="
              << flock_scale.minimum_start_separation << '\n'
              << "flock_scale_state_dump_member_records="
              << flock_scale.state_dump_member_records << '\n'
              << "flock_scale_state_dump_bytes=" << flock_scale.state_dump_bytes << '\n'
              << "flock_scale_meaning=scale_fixture_not_accepted_tuned_gameplay\n";
    std::cout << "gameplay_simulation_result=pass\n";
    return EXIT_SUCCESS;
}
