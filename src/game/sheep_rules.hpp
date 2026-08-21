#pragma once

#include "game/dog_controller.hpp"
#include "game/gameplay_scenario.hpp"
#include "game/paddock_collision.hpp"
#include "game/sheep_spatial_grid.hpp"
#include "game/sheep_state.hpp"

#include <cstddef>
#include <span>

namespace wide_eye::game {

// The accepted per-sheep rules, expressed as functions over immutable prior
// state. They are declared here rather than hidden inside the simulation's
// translation unit for one reason: a rule that only one caller can reach can
// only be measured through that caller. `GameplaySimulation` drives them over
// its fixed five-member buffer and remains the sole authoritative caller; a
// non-player diagnostic can drive the same functions over a larger caller-owned
// span without the authoritative contract growing a member.
//
// Every function takes prior state and writes one published record or one field
// of the next state. None of them reads the state being written, none allocates,
// and none holds a clock, so the order in which a caller sweeps a flock through
// them is a caller decision rather than part of the rule.
// [ADR 0010](../../docs/decisions/0010-diagnostic-flock-scale-over-shared-sheep-rules.md)
// records why the diagnostic shares these functions instead of copying them.

// The one positional authority for a sheep. Every fixture chooses a desired
// planar displacement; the analytic paddock decides where the sheep actually
// ends up, using the same field and the same clipping the dog motor collides
// with. Voxel faces and render meshes never participate, and the steering terms
// that produced `displacement` keep their published vectors: collision is a
// separate, later stage rather than a hidden steering correction.
void resolve_sheep_against_paddock(const SheepState& prior, Vec3 displacement,
                                   const PaddockCollisionField& paddock, SheepState& next,
                                   SheepCollisionEvidence& evidence) noexcept;

// The whole dog stimulus for one sheep: distance, bearing, closing speed, facing
// cosine, sight line, temperament scale, the three applied dog vectors, and the
// dimensionless arousal stimulus the behavior rule follows.
void evaluate_sheep_dog_stimulus(const SheepState& prior_sheep, const DogState& prior_dog,
                                 const GameplayScenarioDefinition& scenario,
                                 const PaddockCollisionField& paddock,
                                 SheepDogPressureEvidence& dog_evidence) noexcept;

// The arousal proxy and the four-state transition it selects, written into the
// next sheep from the immutable prior one.
void apply_sheep_behavior_transition(const SheepState& prior,
                                     const SheepBehaviorConfiguration& behavior, double stimulus,
                                     SheepState& next) noexcept;

// The cell size the enabled social terms share. Zero means no social term is
// enabled and the grid is not rebuilt at all.
[[nodiscard]] double
sheep_social_grid_cell_size(const GameplayScenarioDefinition& scenario) noexcept;

// What one sheep's three social queries selected this tick. The counts belong
// with the selection because the accepted evidence publishes them: neighbour
// selection is a measurable stage of its own, not an implementation detail of
// the term that consumes it.
struct SheepNeighborSelection {
    SpatialGridQueryResult separation{};
    SpatialGridQueryResult attraction{};
    SpatialGridQueryResult alignment{};
};

// Runs the grid queries for every enabled social term into caller-owned scratch
// and returns their results. A disabled term is not queried and keeps a default
// result. The scratch spans stay valid and unmodified until the matching
// `apply_sheep_*` call reads them, which is what lets a caller time selection
// separately from the arithmetic that consumes it.
[[nodiscard]] SheepNeighborSelection select_sheep_neighbors(
    const SheepSpatialGrid& grid, std::size_t index, const SheepSeparationConfiguration& separation,
    const SheepAttractionConfiguration& attraction, const SheepAlignmentConfiguration& alignment,
    std::span<SpatialNeighbor> separation_scratch, std::span<SpatialNeighbor> attraction_scratch,
    std::span<SpatialNeighbor> alignment_scratch) noexcept;

void apply_sheep_separation(std::span<const SheepState> prior, std::size_t index,
                            const SheepSeparationConfiguration& separation,
                            const SpatialGridQueryResult& query,
                            std::span<const SpatialNeighbor> selected,
                            SheepSocialEvidence& evidence) noexcept;

void apply_sheep_attraction(std::span<const SheepState> prior, std::size_t index,
                            const SheepAttractionConfiguration& attraction,
                            const SpatialGridQueryResult& query,
                            std::span<const SpatialNeighbor> selected,
                            SheepSocialEvidence& evidence) noexcept;

void apply_sheep_alignment(std::span<const SheepState> prior, std::size_t index,
                           const SheepAlignmentConfiguration& alignment,
                           const SpatialGridQueryResult& query,
                           std::span<const SpatialNeighbor> selected,
                           SheepSocialEvidence& evidence) noexcept;

// Obstacle and drop avoidance. Both halves are terrain queries against the same
// analytic paddock the collision authority uses.
void apply_sheep_avoidance(const SheepState& prior, const SheepAvoidanceConfiguration& avoidance,
                           const PaddockCollisionField& paddock,
                           SheepAvoidanceEvidence& evidence) noexcept;

// The one place the published terms become a single acceleration, so the bound
// that limits their sum and the integration that consumes it cannot end up in
// two different orders in two different callers.
void apply_sheep_combined_influence(const SheepSocialEvidence& social,
                                    const SheepDogPressureEvidence& dog_pressure,
                                    const SheepAvoidanceEvidence& avoidance,
                                    const SheepCombinedInfluenceConfiguration& combined,
                                    SheepCombinedInfluenceEvidence& evidence,
                                    SheepState& next) noexcept;

// The two limits that act on the result of integration rather than on any
// steering term, applied after the combined bound and before the paddock
// resolves the displacement.
void apply_sheep_motion_limits(const SheepMotionLimitConfiguration& limits, const SheepState& prior,
                               SheepState& next, SheepMotionLimitEvidence& evidence) noexcept;

} // namespace wide_eye::game
