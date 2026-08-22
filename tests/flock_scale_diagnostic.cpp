// Non-player flock-scale diagnostic.
//
// It answers one question the authoritative five-sheep path cannot: what the
// accepted sheep rules cost per stage as the member count grows. It drives the
// *same* functions `GameplaySimulation` drives — `sheep_rules.hpp` — over a
// caller-owned span of any length, so no published contract, buffer, or format
// grows a member to serve a measurement. `--validate-only` asserts only what is
// genuinely deterministic and is the registered CTest; `--benchmark` adds the
// host timings, which are indicative development-host costs and not a budget
// result on any named target.
//
// The oracle that makes the rest credible is the five-member comparison: at the
// authoritative member count this harness must reproduce `GameplaySimulation`'s
// published snapshot exactly, tick for tick, or the larger runs are measuring
// something other than the game.

#include "core/performance.hpp"
#include "game/dog_controller.hpp"
#include "game/gameplay_scenario.hpp"
#include "game/gameplay_simulation.hpp"
#include "game/paddock_collision.hpp"
#include "game/sheep_rules.hpp"
#include "game/sheep_spatial_grid.hpp"
#include "game/sheep_state.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <span>
#include <string_view>
#include <vector>

std::size_t g_flock_scale_allocation_count = 0;

void* operator new(std::size_t size) {
    ++g_flock_scale_allocation_count;
    if (void* allocation = std::malloc(size)) {
        return allocation;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
}

void operator delete(void* allocation) noexcept {
    std::free(allocation);
}

void operator delete(void* allocation, std::size_t) noexcept {
    std::free(allocation);
}

void operator delete[](void* allocation) noexcept {
    std::free(allocation);
}

void operator delete[](void* allocation, std::size_t) noexcept {
    std::free(allocation);
}

namespace {

using wide_eye::game::DogController;
using wide_eye::game::DogMoveInput;
using wide_eye::game::DogState;
using wide_eye::game::GameplayScenarioDefinition;
using wide_eye::game::GameplaySimulation;
using wide_eye::game::GameplaySnapshot;
using wide_eye::game::GameplayTickInput;
using wide_eye::game::PaddockCollisionField;
using wide_eye::game::PaddockObstacle;
using wide_eye::game::SheepAvoidanceEvidence;
using wide_eye::game::SheepCollisionEvidence;
using wide_eye::game::SheepCombinedInfluenceEvidence;
using wide_eye::game::SheepDogPressureEvidence;
using wide_eye::game::SheepMotionLimitEvidence;
using wide_eye::game::SheepNeighborSelection;
using wide_eye::game::SheepSocialEvidence;
using wide_eye::game::SheepSpatialGrid;
using wide_eye::game::SheepState;
using wide_eye::game::SheepTemperament;
using wide_eye::game::SpatialGridBuildError;
using wide_eye::game::SpatialNeighbor;
using wide_eye::game::Vec3;

using Clock = std::chrono::steady_clock;

// The four member counts the roadmap item names. The accepted five-member
// flock stays first, because it is the only one with a `GameplaySimulation`
// reference to compare against.
constexpr std::array<std::size_t, 4> kMemberCounts{5, 14, 25, 100};
static_assert(kMemberCounts.front() == wide_eye::game::kDefaultGameplaySheepCount);
static_assert(kMemberCounts.back() <= SheepSpatialGrid::kMaximumMemberCount);

constexpr std::uint64_t kValidateTicks = 240;
constexpr std::uint64_t kBenchmarkTicks = 600;

// The fixture is a square lattice at a fixed spacing, so density is the same at
// every member count and the footprint is what grows. That is deliberate: it
// makes the neighbour-selection trend a statement about the uniform grid rather
// than about a paddock that was quietly getting more crowded.
//
// `kLatticeSpacing` sits just above the accepted `1.0` separation radius, so a
// resting lattice is not already pushing itself apart on tick zero.
constexpr double kLatticeSpacing = 1.25;
// The lattice origin is the flock's north-west corner. Every analytic paddock
// obstacle ends at `z = 16.0` (`kLeftWall`, `kRightWall`, and `kClosedGate` in
// `src/game/paddock_collision.cpp`), so an origin at `z = 18.0` places every
// member at least `2.0` from the nearest obstacle face — four times the
// `kSheepCollisionRadius` clearance QA-001 asked a fixture to keep.
constexpr double kLatticeOriginX = 8.0;
constexpr double kLatticeOriginZ = 18.0;
constexpr double kLatticeGroundHeight = PaddockCollisionField::kGroundHeight;
// Probe length used to re-check that clearance against the accepted collision
// field itself rather than against the constants quoted above.
constexpr double kObstacleClearanceProbe = 0.5;

// The dog starts west of the lattice, inside the accepted `6.0` stimulus radius
// of its nearest members, and is scripted so that every dog term, the sight
// line, and the paddock collision authority are all exercised rather than left
// idle at zero.
constexpr DogState kDiagnosticDogState{
    .position = {.x = 7.0, .y = kLatticeGroundHeight, .z = 22.0},
    .heading_radians = 0.0,
    .grounded = true,
};

[[nodiscard]] DogMoveInput scripted_dog_input(std::uint64_t tick) noexcept {
    switch ((tick / 60U) % 4U) {
    case 0:
        return {.world_x = 1.0};
    case 1:
        return {.world_z = -1.0, .sprint = true};
    case 2:
        return {.world_x = -1.0};
    default:
        return {.world_z = 1.0};
    }
}

[[nodiscard]] std::size_t lattice_columns(std::size_t members) noexcept {
    std::size_t columns = 1;
    while (columns * columns < members) {
        ++columns;
    }
    return columns;
}

[[nodiscard]] std::vector<SheepState> make_lattice(std::size_t members) {
    constexpr std::array<SheepTemperament, 3> kTemperaments{
        SheepTemperament::ordinary,
        SheepTemperament::nervous,
        SheepTemperament::stubborn,
    };
    const std::size_t columns = lattice_columns(members);
    std::vector<SheepState> flock;
    flock.reserve(members);
    for (std::size_t index = 0; index < members; ++index) {
        const auto column = static_cast<double>(index % columns);
        const auto row = static_cast<double>(index / columns);
        flock.push_back(SheepState{
            .id = static_cast<std::uint32_t>(index + 1),
            .position = {.x = kLatticeOriginX + column * kLatticeSpacing,
                         .y = kLatticeGroundHeight,
                         .z = kLatticeOriginZ + row * kLatticeSpacing},
            .heading_radians = 0.0,
            // Round-robin temperaments keep the dog-response scale off its
            // neutral value for two thirds of the flock, so the temperament
            // factor is measured rather than skipped.
            .temperament = kTemperaments[index % kTemperaments.size()],
            .grounded = true,
        });
    }
    return flock;
}

// Per-tick nanosecond cost of each stage the roadmap item asks for separately.
// `total` is the whole tick, and `unattributed` is what the named stages do not
// account for: the single dog motor update, the loop scaffolding, and the six
// clock reads themselves. Publishing the residual rather than folding it into a
// stage keeps the decomposition honest.
struct StageDurations {
    std::uint64_t snapshot_ns = 0;
    std::uint64_t grid_build_ns = 0;
    std::uint64_t neighbor_selection_ns = 0;
    std::uint64_t terrain_avoidance_ns = 0;
    std::uint64_t behavior_ns = 0;
    std::uint64_t terrain_collision_ns = 0;
    std::uint64_t total_ns = 0;
};

// Deterministic, host-independent work counters. These are the scaling evidence
// that survives a noisy shared development host: the grid inspects exactly this
// many candidates for exactly these queries on exactly this fixture, on any
// machine and in any build configuration.
struct WorkCounters {
    std::uint64_t separation_inspected = 0;
    std::uint64_t separation_within = 0;
    std::uint64_t separation_selected = 0;
    std::uint64_t attraction_inspected = 0;
    std::uint64_t attraction_within = 0;
    std::uint64_t alignment_inspected = 0;
    std::uint64_t alignment_within = 0;
    std::uint64_t avoidance_evaluated = 0;
    std::uint64_t obstacle_named = 0;
    std::uint64_t collision_clipped = 0;
    std::uint64_t sight_line_blocked = 0;
    std::uint64_t bound_bound = 0;
    std::uint64_t speed_clamped = 0;

    bool operator==(const WorkCounters&) const = default;
};

// One diagnostic flock of arbitrary size, held entirely in caller-owned heap
// storage. It is deliberately *not* a `GameplaySimulation`: it publishes no
// snapshot, has no replay contract, and nothing in the engine can reach it.
// What it shares with the authoritative tick is every rule it applies.
//
// The tick is swept stage by stage rather than sheep by sheep. Both orders
// produce identical state, because every rule reads only the immutable prior
// buffer and the prior dog and writes only its own member's records — and the
// five-member reference check proves it rather than asserting it. Sweeping by
// stage is what makes six clock reads per tick enough to decompose the cost;
// timing each stage per sheep would add hundreds of clock reads to a tick that
// costs microseconds.
class DiagnosticFlock {
  public:
    DiagnosticFlock(GameplayScenarioDefinition scenario, std::vector<SheepState> initial_sheep)
        : scenario_{scenario}, initial_sheep_{std::move(initial_sheep)},
          dog_{scenario.dog, scenario.gate_open}, paddock_{scenario.gate_open},
          grid_{std::make_unique<SheepSpatialGrid>()}, prior_{initial_sheep_},
          next_{initial_sheep_}, social_(initial_sheep_.size()),
          dog_pressure_(initial_sheep_.size()), collision_(initial_sheep_.size()),
          avoidance_(initial_sheep_.size()), combined_(initial_sheep_.size()),
          motion_(initial_sheep_.size()), selection_(initial_sheep_.size()),
          separation_scratch_(initial_sheep_.size() * (initial_sheep_.size() - 1)),
          attraction_scratch_(initial_sheep_.size() *
                              wide_eye::game::kMaximumSelectedAttractionNeighbors),
          alignment_scratch_(initial_sheep_.size() *
                             wide_eye::game::kMaximumSelectedAlignmentNeighbors),
          grid_cell_size_{wide_eye::game::sheep_social_grid_cell_size(scenario)} {}

    [[nodiscard]] std::size_t member_count() const noexcept {
        return prior_.size();
    }
    [[nodiscard]] std::span<const SheepState> sheep() const noexcept {
        return prior_;
    }
    [[nodiscard]] const DogState& dog() const noexcept {
        return dog_state_;
    }
    [[nodiscard]] const std::vector<SheepSocialEvidence>& social() const noexcept {
        return social_;
    }
    [[nodiscard]] const std::vector<SheepDogPressureEvidence>& dog_pressure() const noexcept {
        return dog_pressure_;
    }
    [[nodiscard]] const std::vector<SheepCollisionEvidence>& collision() const noexcept {
        return collision_;
    }
    [[nodiscard]] const std::vector<SheepAvoidanceEvidence>& avoidance() const noexcept {
        return avoidance_;
    }
    [[nodiscard]] const std::vector<SheepCombinedInfluenceEvidence>& combined() const noexcept {
        return combined_;
    }
    [[nodiscard]] const std::vector<SheepMotionLimitEvidence>& motion() const noexcept {
        return motion_;
    }
    [[nodiscard]] double grid_cell_size() const noexcept {
        return grid_cell_size_;
    }
    [[nodiscard]] const WorkCounters& work() const noexcept {
        return work_;
    }
    [[nodiscard]] bool healthy() const noexcept {
        return healthy_;
    }

    void restart() noexcept {
        dog_.restart();
        dog_state_ = dog_.state();
        prior_ = initial_sheep_;
        next_ = initial_sheep_;
        for (std::size_t index = 0; index < prior_.size(); ++index) {
            social_[index] = {.subject_id = prior_[index].id};
            dog_pressure_[index] = {.subject_id = prior_[index].id};
            collision_[index] = {.subject_id = prior_[index].id};
            avoidance_[index] = {.subject_id = prior_[index].id};
            combined_[index] = {.subject_id = prior_[index].id};
            motion_[index] = {.subject_id = prior_[index].id};
        }
        work_ = {};
        tick_ = 0;
        healthy_ = true;
    }

    StageDurations advance(const DogMoveInput& input) noexcept {
        constexpr double kDelta = GameplaySimulation::kFixedDeltaSeconds;
        const std::size_t members = prior_.size();
        StageDurations stages;

        const auto tick_start = Clock::now();

        // Snapshot publication: the prior buffer is carried forward and the six
        // read-only evidence records are reset. This is what one published
        // per-sheep snapshot costs at this member count.
        for (std::size_t index = 0; index < members; ++index) {
            next_[index] = prior_[index];
            social_[index] = {.subject_id = prior_[index].id};
            dog_pressure_[index] = {.subject_id = prior_[index].id};
            collision_[index] = {.subject_id = prior_[index].id};
            avoidance_[index] = {.subject_id = prior_[index].id};
            combined_[index] = {.subject_id = prior_[index].id};
            motion_[index] = {.subject_id = prior_[index].id};
        }
        const auto after_snapshot = Clock::now();

        // The sheep read the dog as it was *before* this tick's move, exactly as
        // `GameplaySimulation` hands them `previous.dog`.
        const DogState prior_dog = dog_state_;
        dog_.fixed_update(input, kDelta);
        dog_state_ = dog_.state();

        const auto before_grid = Clock::now();
        if (grid_cell_size_ > 0.0) {
            const SpatialGridBuildError build_error =
                grid_->rebuild(std::span<const SheepState>{prior_}, grid_cell_size_);
            healthy_ = healthy_ && build_error == SpatialGridBuildError::none;
        }
        const auto after_grid = Clock::now();

        for (std::size_t index = 0; index < members; ++index) {
            selection_[index] = wide_eye::game::select_sheep_neighbors(
                *grid_, index, scenario_.sheep_separation, scenario_.sheep_attraction,
                scenario_.sheep_alignment, separation_scratch_for(index),
                attraction_scratch_for(index), alignment_scratch_for(index));
        }
        const auto after_selection = Clock::now();

        for (std::size_t index = 0; index < members; ++index) {
            wide_eye::game::apply_sheep_avoidance(prior_[index], scenario_.sheep_avoidance,
                                                  paddock_, avoidance_[index]);
        }
        const auto after_avoidance = Clock::now();

        const std::span<const SheepState> prior_view{prior_};
        for (std::size_t index = 0; index < members; ++index) {
            wide_eye::game::evaluate_sheep_dog_stimulus(prior_[index], prior_dog, scenario_,
                                                        paddock_, dog_pressure_[index]);
            wide_eye::game::apply_sheep_behavior_transition(prior_[index], scenario_.sheep_behavior,
                                                            dog_pressure_[index].arousal_stimulus,
                                                            next_[index]);
            if (scenario_.sheep_separation.enabled) {
                wide_eye::game::apply_sheep_separation(
                    prior_view, index, scenario_.sheep_separation, selection_[index].separation,
                    separation_scratch_for(index), social_[index]);
            }
            if (scenario_.sheep_attraction.enabled) {
                wide_eye::game::apply_sheep_attraction(
                    prior_view, index, scenario_.sheep_attraction, selection_[index].attraction,
                    attraction_scratch_for(index), social_[index]);
            }
            if (scenario_.sheep_alignment.enabled) {
                wide_eye::game::apply_sheep_alignment(prior_view, index, scenario_.sheep_alignment,
                                                      selection_[index].alignment,
                                                      alignment_scratch_for(index), social_[index]);
            }
            wide_eye::game::apply_sheep_combined_influence(
                social_[index], dog_pressure_[index], avoidance_[index],
                scenario_.sheep_combined_influence, combined_[index], next_[index]);
            wide_eye::game::apply_sheep_motion_limits(scenario_.sheep_motion_limit, prior_[index],
                                                      next_[index], motion_[index]);
        }
        const auto after_behavior = Clock::now();

        for (std::size_t index = 0; index < members; ++index) {
            wide_eye::game::resolve_sheep_against_paddock(
                prior_[index],
                {.x = next_[index].velocity.x * kDelta, .z = next_[index].velocity.z * kDelta},
                paddock_, next_[index], collision_[index]);
        }
        const auto after_collision = Clock::now();

        prior_.swap(next_);
        ++tick_;

        const auto elapsed = [](Clock::time_point from, Clock::time_point to) {
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(to - from).count());
        };
        stages.snapshot_ns = elapsed(tick_start, after_snapshot);
        stages.grid_build_ns = elapsed(before_grid, after_grid);
        stages.neighbor_selection_ns = elapsed(after_grid, after_selection);
        stages.terrain_avoidance_ns = elapsed(after_selection, after_avoidance);
        stages.behavior_ns = elapsed(after_avoidance, after_behavior);
        stages.terrain_collision_ns = elapsed(after_behavior, after_collision);
        stages.total_ns = elapsed(tick_start, after_collision);
        return stages;
    }

    // Folded outside the measured stages on purpose: counting is diagnostic
    // bookkeeping and has no place inside a cost measurement.
    void accumulate_work() noexcept {
        for (std::size_t index = 0; index < prior_.size(); ++index) {
            const SheepNeighborSelection& selection = selection_[index];
            work_.separation_inspected += selection.separation.inspected_candidate_count;
            work_.separation_within += selection.separation.within_radius_count;
            work_.separation_selected += selection.separation.neighbor_count;
            work_.attraction_inspected += selection.attraction.inspected_candidate_count;
            work_.attraction_within += selection.attraction.within_radius_count;
            work_.alignment_inspected += selection.alignment.inspected_candidate_count;
            work_.alignment_within += selection.alignment.within_radius_count;
            work_.avoidance_evaluated += avoidance_[index].avoidance_evaluated ? 1U : 0U;
            work_.obstacle_named += avoidance_[index].obstacle != PaddockObstacle::none ? 1U : 0U;
            work_.collision_clipped +=
                collision_[index].clipped_x || collision_[index].clipped_z ? 1U : 0U;
            work_.sight_line_blocked += dog_pressure_[index].dog_line_of_sight_blocked ? 1U : 0U;
            work_.bound_bound += combined_[index].applied_scale < 1.0 ? 1U : 0U;
            work_.speed_clamped += motion_[index].applied_speed_scale < 1.0 ? 1U : 0U;
        }
    }

    [[nodiscard]] bool finite_and_inside_paddock() const noexcept {
        for (const SheepState& member : prior_) {
            if (!std::isfinite(member.position.x) || !std::isfinite(member.position.z) ||
                !std::isfinite(member.velocity.x) || !std::isfinite(member.velocity.z) ||
                !std::isfinite(member.heading_radians) || !std::isfinite(member.arousal)) {
                return false;
            }
            if (member.position.x < PaddockCollisionField::kMinimumX ||
                member.position.x > PaddockCollisionField::kMaximumX ||
                member.position.z < PaddockCollisionField::kMinimumZ ||
                member.position.z > PaddockCollisionField::kMaximumZ) {
                return false;
            }
        }
        return true;
    }

  private:
    [[nodiscard]] std::span<SpatialNeighbor> separation_scratch_for(std::size_t index) noexcept {
        const std::size_t stride = prior_.size() - 1;
        return std::span<SpatialNeighbor>{separation_scratch_}.subspan(index * stride, stride);
    }
    [[nodiscard]] std::span<SpatialNeighbor> attraction_scratch_for(std::size_t index) noexcept {
        constexpr std::size_t stride = wide_eye::game::kMaximumSelectedAttractionNeighbors;
        return std::span<SpatialNeighbor>{attraction_scratch_}.subspan(index * stride, stride);
    }
    [[nodiscard]] std::span<SpatialNeighbor> alignment_scratch_for(std::size_t index) noexcept {
        constexpr std::size_t stride = wide_eye::game::kMaximumSelectedAlignmentNeighbors;
        return std::span<SpatialNeighbor>{alignment_scratch_}.subspan(index * stride, stride);
    }

    GameplayScenarioDefinition scenario_;
    std::vector<SheepState> initial_sheep_;
    DogController dog_;
    PaddockCollisionField paddock_;
    // The grid still carries its 1,000-member capacity-experiment ceiling, so it
    // is about 115 KiB whatever the flock size. QA-002 is the reason it lives
    // behind a pointer here rather than inside a stack frame.
    std::unique_ptr<SheepSpatialGrid> grid_;
    DogState dog_state_{};
    std::vector<SheepState> prior_;
    std::vector<SheepState> next_;
    std::vector<SheepSocialEvidence> social_;
    std::vector<SheepDogPressureEvidence> dog_pressure_;
    std::vector<SheepCollisionEvidence> collision_;
    std::vector<SheepAvoidanceEvidence> avoidance_;
    std::vector<SheepCombinedInfluenceEvidence> combined_;
    std::vector<SheepMotionLimitEvidence> motion_;
    std::vector<SheepNeighborSelection> selection_;
    std::vector<SpatialNeighbor> separation_scratch_;
    std::vector<SpatialNeighbor> attraction_scratch_;
    std::vector<SpatialNeighbor> alignment_scratch_;
    WorkCounters work_{};
    double grid_cell_size_ = 0.0;
    std::uint64_t tick_ = 0;
    bool healthy_ = true;
};

bool check(bool condition, std::string_view stage) {
    if (condition) {
        return true;
    }
    std::cerr << "flock_scale_result=fail\n"
              << "failure_stage=" << stage << '\n';
    return false;
}

// The diagnostic scenario is the accepted maximal fixture — every steering term,
// both dog-independent limits, the sight line, temperament, and the behavior
// transitions switched on — with only the starting placement replaced.
[[nodiscard]] GameplayScenarioDefinition diagnostic_scenario(std::span<const SheepState> lattice) {
    GameplayScenarioDefinition scenario =
        wide_eye::game::find_gameplay_scenario("sheep-all-influences-diagnostic").value();
    scenario.dog.initial_state = kDiagnosticDogState;
    // Only the five-member reference constructs a `GameplaySimulation` from this;
    // the diagnostic flock carries its own storage. Filling it keeps the two
    // fixtures identical where they overlap. A lattice larger than a published
    // buffer can hold is still a legal diagnostic flock, so the scenario carries
    // whatever part of it fits.
    scenario.sheep_count =
        std::min(lattice.size(), wide_eye::game::kMaximumGameplaySheepCount);
    for (std::size_t index = 0; index < scenario.sheep_count; ++index) {
        scenario.initial_sheep[index] = lattice[index];
    }
    return scenario;
}

// Every lattice member must stand clear of every obstacle face, and the check
// asks the accepted collision field rather than trusting the constants quoted
// beside the origin. A body of the real sheep radius probing half a body length
// along each axis must reach nothing.
[[nodiscard]] bool lattice_is_clear(const PaddockCollisionField& paddock,
                                    std::span<const SheepState> lattice) noexcept {
    constexpr std::array<Vec3, 4> kProbeDirections{Vec3{.x = 1.0}, Vec3{.x = -1.0}, Vec3{.z = 1.0},
                                                   Vec3{.z = -1.0}};
    for (const SheepState& member : lattice) {
        if (member.position.x - wide_eye::game::kSheepCollisionRadius <
                PaddockCollisionField::kMinimumX ||
            member.position.x + wide_eye::game::kSheepCollisionRadius >
                PaddockCollisionField::kMaximumX ||
            member.position.z - wide_eye::game::kSheepCollisionRadius <
                PaddockCollisionField::kMinimumZ ||
            member.position.z + wide_eye::game::kSheepCollisionRadius >
                PaddockCollisionField::kMaximumZ) {
            return false;
        }
        for (const Vec3 direction : kProbeDirections) {
            if (paddock
                    .approaching_obstacle(member.position, direction, kObstacleClearanceProbe,
                                          wide_eye::game::kSheepCollisionRadius)
                    .obstacle != PaddockObstacle::none) {
                return false;
            }
        }
    }
    return true;
}

// The oracle. At the authoritative member count the diagnostic must reproduce
// the published snapshot exactly — sheep, dog, and all six evidence records —
// or every larger measurement is describing a different game.
[[nodiscard]] bool matches_published_snapshot(const GameplaySnapshot& snapshot,
                                              const DiagnosticFlock& flock) noexcept {
    if (!(snapshot.dog == flock.dog())) {
        return false;
    }
    for (std::size_t index = 0; index < snapshot.sheep_count; ++index) {
        if (!(snapshot.sheep[index] == flock.sheep()[index]) ||
            !(snapshot.sheep_social_evidence[index] == flock.social()[index]) ||
            !(snapshot.sheep_dog_pressure_evidence[index] == flock.dog_pressure()[index]) ||
            !(snapshot.sheep_collision_evidence[index] == flock.collision()[index]) ||
            !(snapshot.sheep_avoidance_evidence[index] == flock.avoidance()[index]) ||
            !(snapshot.sheep_combined_influence_evidence[index] == flock.combined()[index]) ||
            !(snapshot.sheep_motion_limit_evidence[index] == flock.motion()[index])) {
            return false;
        }
    }
    return true;
}

struct MemberRun {
    std::size_t members = 0;
    std::size_t columns = 0;
    std::uint64_t ticks = 0;
    std::size_t setup_allocations = 0;
    std::size_t measured_allocations = 0;
    double grid_cell_size = 0.0;
    WorkCounters work{};
    bool deterministic = false;
    bool finite = false;
    std::vector<std::uint64_t> snapshot_ns;
    std::vector<std::uint64_t> grid_build_ns;
    std::vector<std::uint64_t> neighbor_selection_ns;
    std::vector<std::uint64_t> terrain_avoidance_ns;
    std::vector<std::uint64_t> behavior_ns;
    std::vector<std::uint64_t> terrain_collision_ns;
    std::vector<std::uint64_t> total_ns;
};

[[nodiscard]] MemberRun run_member_count(std::size_t members, std::uint64_t ticks) {
    MemberRun run;
    run.members = members;
    run.columns = lattice_columns(members);
    run.ticks = ticks;

    const std::vector<SheepState> lattice = make_lattice(members);
    const GameplayScenarioDefinition scenario = diagnostic_scenario(lattice);

    const std::size_t allocations_before_setup = g_flock_scale_allocation_count;
    auto flock = std::make_unique<DiagnosticFlock>(scenario, lattice);
    flock->restart();
    run.snapshot_ns.resize(ticks);
    run.grid_build_ns.resize(ticks);
    run.neighbor_selection_ns.resize(ticks);
    run.terrain_avoidance_ns.resize(ticks);
    run.behavior_ns.resize(ticks);
    run.terrain_collision_ns.resize(ticks);
    run.total_ns.resize(ticks);
    run.setup_allocations = g_flock_scale_allocation_count - allocations_before_setup;
    run.grid_cell_size = flock->grid_cell_size();

    // One untimed warm-up tick pays for the first-touch page faults on the
    // storage above, so the first measured tick is not reported as the worst.
    static_cast<void>(flock->advance(scripted_dog_input(0)));
    flock->restart();

    const std::size_t allocations_before_ticks = g_flock_scale_allocation_count;
    for (std::uint64_t tick = 0; tick < ticks; ++tick) {
        const StageDurations stages = flock->advance(scripted_dog_input(tick));
        const auto slot = static_cast<std::size_t>(tick);
        run.snapshot_ns[slot] = stages.snapshot_ns;
        run.grid_build_ns[slot] = stages.grid_build_ns;
        run.neighbor_selection_ns[slot] = stages.neighbor_selection_ns;
        run.terrain_avoidance_ns[slot] = stages.terrain_avoidance_ns;
        run.behavior_ns[slot] = stages.behavior_ns;
        run.terrain_collision_ns[slot] = stages.terrain_collision_ns;
        run.total_ns[slot] = stages.total_ns;
        flock->accumulate_work();
    }
    run.measured_allocations = g_flock_scale_allocation_count - allocations_before_ticks;
    run.work = flock->work();
    run.finite = flock->finite_and_inside_paddock() && flock->healthy();

    // A second independent flock over the same fixture must produce the same
    // final state and the same work counters. There is no randomness anywhere in
    // the simulation, so this is a tripwire rather than a statistical claim.
    auto repeat = std::make_unique<DiagnosticFlock>(scenario, lattice);
    repeat->restart();
    for (std::uint64_t tick = 0; tick < ticks; ++tick) {
        static_cast<void>(repeat->advance(scripted_dog_input(tick)));
        repeat->accumulate_work();
    }
    run.deterministic = repeat->work() == run.work;
    for (std::size_t index = 0; index < members && run.deterministic; ++index) {
        run.deterministic = flock->sheep()[index] == repeat->sheep()[index] &&
                            flock->social()[index] == repeat->social()[index] &&
                            flock->dog_pressure()[index] == repeat->dog_pressure()[index] &&
                            flock->collision()[index] == repeat->collision()[index] &&
                            flock->avoidance()[index] == repeat->avoidance()[index] &&
                            flock->combined()[index] == repeat->combined()[index] &&
                            flock->motion()[index] == repeat->motion()[index];
    }
    return run;
}

// The five-member reference: `GameplaySimulation` and the diagnostic, same
// scenario, same scripted dog, compared every tick.
[[nodiscard]] std::int64_t first_reference_mismatch_tick(std::uint64_t ticks) {
    const std::vector<SheepState> lattice =
        make_lattice(wide_eye::game::kDefaultGameplaySheepCount);
    const GameplayScenarioDefinition scenario = diagnostic_scenario(lattice);
    auto simulation = std::make_unique<GameplaySimulation>(scenario);
    auto flock = std::make_unique<DiagnosticFlock>(scenario, lattice);
    flock->restart();
    if (!matches_published_snapshot(simulation->current_snapshot(), *flock)) {
        return -2;
    }
    for (std::uint64_t tick = 0; tick < ticks; ++tick) {
        const DogMoveInput input = scripted_dog_input(tick);
        simulation->fixed_update(GameplayTickInput{.dog_move = input});
        static_cast<void>(flock->advance(input));
        if (!matches_published_snapshot(simulation->current_snapshot(), *flock)) {
            return static_cast<std::int64_t>(tick);
        }
    }
    return -1;
}

void print_stage(std::size_t members, std::string_view name,
                 const std::vector<std::uint64_t>& samples) {
    const auto statistics = wide_eye::core::summarize_durations(samples);
    if (!statistics.has_value()) {
        return;
    }
    std::cout << "flock_scale_stage members=" << members << " name=" << name
              << " samples=" << samples.size() << " minimum_ns=" << statistics->minimum_ns
              << " median_ns=" << statistics->median_ns << " p95_ns=" << statistics->p95_ns
              << " p99_ns=" << statistics->p99_ns << " maximum_ns=" << statistics->maximum_ns
              << '\n';
}

void print_work(const MemberRun& run) {
    const auto sheep_ticks = static_cast<std::uint64_t>(run.members) * run.ticks;
    std::cout << "flock_scale_fixture members=" << run.members << " columns=" << run.columns
              << " spacing=1.25 origin_x=8 origin_z=18 ticks=" << run.ticks
              << " sheep_ticks=" << sheep_ticks << " grid_cell_size=" << run.grid_cell_size << '\n'
              << "flock_scale_work members=" << run.members
              << " separation_inspected=" << run.work.separation_inspected
              << " separation_within=" << run.work.separation_within
              << " separation_selected=" << run.work.separation_selected
              << " attraction_inspected=" << run.work.attraction_inspected
              << " attraction_within=" << run.work.attraction_within
              << " alignment_inspected=" << run.work.alignment_inspected
              << " alignment_within=" << run.work.alignment_within << '\n'
              << "flock_scale_events members=" << run.members
              << " avoidance_evaluated=" << run.work.avoidance_evaluated
              << " obstacle_named=" << run.work.obstacle_named
              << " collision_clipped=" << run.work.collision_clipped
              << " sight_line_blocked=" << run.work.sight_line_blocked
              << " combined_bound_bound=" << run.work.bound_bound
              << " speed_clamped=" << run.work.speed_clamped << '\n'
              << "flock_scale_allocations members=" << run.members
              << " setup=" << run.setup_allocations
              << " measured_ticks=" << run.measured_allocations << '\n';
}

} // namespace

int main(int argc, char** argv) {
    const bool run_benchmark = argc == 2 && std::string_view(argv[1]) == "--benchmark";
    const bool validate_only = argc == 2 && std::string_view(argv[1]) == "--validate-only";
    if (!run_benchmark && !validate_only) {
        std::cerr << "usage: wide_eye_flock_scale_diagnostic --validate-only|--benchmark\n"
                  << "flock_scale_result=fail\n"
                  << "failure_stage=arguments\n";
        return EXIT_FAILURE;
    }
    const std::uint64_t ticks = run_benchmark ? kBenchmarkTicks : kValidateTicks;

    if (!check(
            wide_eye::game::find_gameplay_scenario("sheep-all-influences-diagnostic").has_value(),
            "diagnostic_scenario_available")) {
        return EXIT_FAILURE;
    }

    const PaddockCollisionField paddock{false};
    for (const std::size_t members : kMemberCounts) {
        const std::vector<SheepState> lattice = make_lattice(members);
        if (!check(lattice.size() == members, "lattice_size") ||
            !check(lattice_is_clear(paddock, lattice), "lattice_obstacle_clearance")) {
            return EXIT_FAILURE;
        }
    }

    const std::int64_t mismatch_tick = first_reference_mismatch_tick(kValidateTicks);
    if (!check(mismatch_tick == -1, "five_member_reference_equality")) {
        std::cerr << "reference_mismatch_tick=" << mismatch_tick << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "flock_scale_schema=1\n"
              << "flock_scale_scenario=sheep-all-influences-diagnostic\n"
              << "flock_scale_authoritative_members="
              << wide_eye::game::kDefaultGameplaySheepCount << '\n'
              << "flock_scale_grid_capacity=" << SheepSpatialGrid::kMaximumMemberCount << '\n'
              << "flock_scale_reference members=" << wide_eye::game::kDefaultGameplaySheepCount
              << " ticks=" << kValidateTicks << " published_snapshot_equal=yes\n"
              << "flock_scale_timing_status=" << (run_benchmark ? "measured" : "not_measured")
              << '\n'
              << "flock_scale_timing_meaning=indicative_development_host_cost_not_a_budget\n";

    std::vector<MemberRun> runs;
    runs.reserve(kMemberCounts.size());
    for (const std::size_t members : kMemberCounts) {
        runs.push_back(run_member_count(members, ticks));
        const MemberRun& run = runs.back();
        if (!check(run.measured_allocations == 0, "measured_tick_allocations") ||
            !check(run.deterministic, "repeated_run_determinism") ||
            !check(run.finite, "finite_state_inside_paddock") ||
            !check(run.work.separation_selected == run.work.separation_within,
                   "separation_selects_every_close_neighbor")) {
            std::cerr << "failure_members=" << run.members << '\n';
            return EXIT_FAILURE;
        }
        print_work(run);
    }

    if (run_benchmark) {
        for (const MemberRun& run : runs) {
            print_stage(run.members, "snapshot", run.snapshot_ns);
            print_stage(run.members, "spatial_grid_build", run.grid_build_ns);
            print_stage(run.members, "neighbor_selection", run.neighbor_selection_ns);
            print_stage(run.members, "terrain_query_avoidance", run.terrain_avoidance_ns);
            print_stage(run.members, "behavior", run.behavior_ns);
            print_stage(run.members, "terrain_query_collision", run.terrain_collision_ns);
            print_stage(run.members, "total", run.total_ns);
        }
    }

    if (const auto memory = wide_eye::core::sample_process_memory(); memory.has_value()) {
        std::cout << "flock_scale_process_rss_bytes=" << memory->current_rss_bytes << '\n'
                  << "flock_scale_process_peak_rss_bytes=" << memory->peak_rss_bytes << '\n';
    }

    std::cout << "flock_scale_result=pass\n";
    return EXIT_SUCCESS;
}
