#pragma once

#include "game/math.hpp"
#include "game/paddock_collision.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace wide_eye::game {

// The four behavior states the accepted design asks a sheep to distinguish "so
// sheep do not move like constant-speed boids". They are selected from the
// arousal parameter below and from whether a cause is acting on the sheep right
// now; see `SheepBehaviorConfiguration` for the thresholds and
// [ADR 0009](../../docs/decisions/0009-behavior-transitions-and-arousal.md) for
// the rule.
//
// - `settled` — this sheep is not engaged: either no cause has lifted its
//   arousal as far as the alert threshold, or it has already come all the way
//   back down to rest. The grazing state.
// - `alert` — a cause is acting and arousal has passed the alert threshold but
//   not the driven one: the sheep has noticed, and is not yet being moved.
// - `driven` — a cause is acting and arousal has passed the driven threshold.
// - `recovering` — the cause has been released and arousal has not yet returned
//   to rest. This is what makes release a verb: it is reachable only after
//   pressure stops, and it is the one state a sheep can never be in while a
//   cause is still acting on it.
enum class SheepBehaviorState : std::uint8_t {
    settled,
    alert,
    driven,
    recovering,
};

[[nodiscard]] constexpr bool is_known_sheep_behavior(SheepBehaviorState behavior) noexcept {
    switch (behavior) {
    case SheepBehaviorState::settled:
    case SheepBehaviorState::alert:
    case SheepBehaviorState::driven:
    case SheepBehaviorState::recovering:
        return true;
    }
    return false;
}

// Persistent per-sheep response label. The accepted design makes the dog's
// effective pressure depend on "the sheep's temperament", so temperament is a
// property of the sheep and travels with it in the authoritative buffer rather
// than living in a dog, fixture-index, or presentation structure. Unlike
// `SheepBehaviorState`, which the transition rule rewrites every tick, this
// label never changes during a run: it is part of the scenario's starting
// contract. The three names are game-readable design categories, not validated
// biological ones.
enum class SheepTemperament : std::uint8_t {
    ordinary,
    nervous,
    stubborn,
};

[[nodiscard]] constexpr bool is_known_sheep_temperament(SheepTemperament temperament) noexcept {
    switch (temperament) {
    case SheepTemperament::ordinary:
    case SheepTemperament::nervous:
    case SheepTemperament::stubborn:
        return true;
    }
    return false;
}

// Arousal is a **named game parameter**, not a claim about animal physiology.
// It is not a heart rate, a stress hormone, a cortisol level, or any measured
// animal response, and nothing in this project has calibrated it against an
// animal. It is a bounded design variable in `[0, 1]` that decides which of the
// four behavior states a sheep is in, exactly as the accepted design intends:
// "`Stress` is player-facing shorthand for an arousal/recovery design variable,
// not a claimed physiological model." The research record asks for the same
// caution — name it `arousal` internally until evidence establishes what it
// represents — so the name is deliberately the internal one.
inline constexpr double kSheepMinimumArousal = 0.0;
inline constexpr double kSheepMaximumArousal = 1.0;

struct SheepState {
    std::uint32_t id = 0;
    Vec3 position{};
    Vec3 velocity{};
    double heading_radians = 0.0;
    // Bounded to `[kSheepMinimumArousal, kSheepMaximumArousal]`. See above: a
    // game parameter, not a physiological measurement.
    double arousal = 0.0;
    SheepBehaviorState behavior = SheepBehaviorState::settled;
    SheepTemperament temperament = SheepTemperament::ordinary;
    bool grounded = false;

    bool operator==(const SheepState&) const = default;
};

inline constexpr std::size_t kGameplaySheepCount = 5;
using SheepStateBuffer = std::array<SheepState, kGameplaySheepCount>;

inline constexpr std::size_t kMaximumSelectedAttractionNeighbors = 2;
inline constexpr std::size_t kMaximumSelectedAlignmentNeighbors = 1;

// Planar body radius the analytic paddock uses when it resolves one sheep's
// displacement. This is a sheep rule owned here rather than a borrowed dog
// constant or a render-proxy dimension: the accepted close-range separation
// radius is the 1-unit spacing at which sheep stop pushing each other apart, so
// half of it is the largest body radius under which two resting sheep are not
// already interpenetrating. Sheep-versus-sheep body collision is not
// implemented; only the paddock boundary reads this value.
inline constexpr double kSheepCollisionRadius = 0.5;

// Read-only causal evidence published with each authoritative sheep snapshot.
// IDs, rather than buffer indexes, keep the evidence meaningful when storage
// order changes. Candidate count records the density seen inside the attraction
// radius even when the selected set is truncated by its fixed bound.
struct SheepSocialEvidence {
    std::uint32_t subject_id = 0;
    std::array<std::uint32_t, kMaximumSelectedAttractionNeighbors> attraction_neighbor_ids{};
    std::uint32_t attraction_neighbor_count = 0;
    std::uint32_t attraction_candidate_count = 0;
    std::array<std::uint32_t, kMaximumSelectedAlignmentNeighbors> alignment_neighbor_ids{};
    std::uint32_t alignment_neighbor_count = 0;
    std::uint32_t alignment_candidate_count = 0;
    Vec3 separation_acceleration{};
    Vec3 attraction_acceleration{};
    Vec3 alignment_acceleration{};

    bool operator==(const SheepSocialEvidence&) const = default;
};

using SheepSocialEvidenceBuffer = std::array<SheepSocialEvidence, kGameplaySheepCount>;

// Dog stimulus remains separate from social evidence so paired controls can
// publish identical geometry while independently switching the applied term.
// Distance, bearing, approach speed, facing alignment, and line-of-sight
// blocking describe the immutable prior state that caused the next authoritative
// sheep state. Bearing is signed relative to sheep heading. Approach speed is
// the prior-state component of dog velocity along the dog-to-sheep direction:
// positive when the dog closes, negative when it leaves. Facing alignment is the
// cosine between the prior dog forward direction and the dog-to-sheep direction:
// `1` when the dog looks straight at the sheep, `0` abeam, `-1` when it looks
// directly away. Line of sight names the analytic paddock obstacle that hides
// the dog, so an occlusion result can be attributed to a wall or a closed gate
// rather than to an anonymous flag. The temperament response scale is the factor
// the prior sheep's temperament applied to every dog term this tick, published
// so a sheep that moves differently from an identically placed neighbour is
// explainable from the dump instead of from reading the scenario table. Each
// applied term keeps its own acceleration vector so one switch can be isolated
// without hiding it inside a combined total; an occluded dog releases all three
// vectors rather than adding a fourth.
//
// `arousal_stimulus` is the same dog stimulus expressed as a dimensionless
// `[0, 1]` fraction instead of as an acceleration: the shared linear distance
// falloff, released by an occluded sight line and scaled by the same
// temperament factor as every dog term. It lives here rather than in a parallel
// per-sheep array because it *is* dog-stimulus evidence — the dog is the only
// cause that drives arousal in this outcome. A new array also used to cost stack
// the harness did not have; that constraint was retired with QA-002, so the
// ownership argument is now the whole reason. It is published whenever the
// stimulus was evaluated, in a scenario with the behavior transitions switched
// off as well as on, so a paired control publishes an identical cause and only
// the applied arousal differs.
struct SheepDogPressureEvidence {
    std::uint32_t subject_id = 0;
    bool stimulus_evaluated = false;
    double dog_distance = 0.0;
    double dog_relative_bearing_radians = 0.0;
    double dog_approach_speed = 0.0;
    double dog_facing_alignment = 0.0;
    bool dog_line_of_sight_blocked = false;
    PaddockObstacle dog_line_of_sight_occluder = PaddockObstacle::none;
    double temperament_response_scale = 0.0;
    double arousal_stimulus = 0.0;
    Vec3 pressure_acceleration{};
    Vec3 approach_acceleration{};
    Vec3 facing_acceleration{};

    bool operator==(const SheepDogPressureEvidence&) const = default;
};

using SheepDogPressureEvidenceBuffer = std::array<SheepDogPressureEvidence, kGameplaySheepCount>;

// Positional evidence published with each authoritative sheep snapshot. The
// steering terms above describe what a sheep tried to do; this records what the
// paddock allowed, so a stopped sheep is explainable instead of mysterious.
// `clipped_x` and `clipped_z` name the axes the analytic field refused this
// tick — the same axes whose velocity is cleared — and `obstacle` names the
// shape that refused them. A clipped axis with `none` was stopped by the
// paddock's outer bounds. Collision is a separate, later authority than
// steering: it never rewrites a published acceleration vector.
struct SheepCollisionEvidence {
    std::uint32_t subject_id = 0;
    bool clipped_x = false;
    bool clipped_z = false;
    PaddockObstacle obstacle = PaddockObstacle::none;

    bool operator==(const SheepCollisionEvidence&) const = default;
};

using SheepCollisionEvidenceBuffer = std::array<SheepCollisionEvidence, kGameplaySheepCount>;

// Read-only evidence for the steering term that tries to make the hard collision
// authority unnecessary. Avoidance is a term like the others: it publishes its
// own acceleration vector, that vector is summed with the rest, and the combined
// bound scales it exactly as it scales everything else. It is not a second
// collision rule and it never moves a sheep by itself.
//
// `avoidance_evaluated` is false for a sheep the term did not run for, which is
// a scenario with the term switched off and also a sheep whose prior planar
// speed is at or below `kSheepHeadingMotionSpeedFloor`: avoidance reads where
// the sheep is *going*, and a sheep that is not measurably moving is not about
// to run into anything, so there is no direction to probe and none is invented.
// `obstacle` names the analytic paddock shape the look-ahead path reaches first
// and `obstacle_distance` is how far along that path it lies; a named obstacle
// exactly at the look-ahead distance publishes a zero vector, because the
// response falls off linearly to nothing there. `drop_ahead` is true when the
// ground under the look-ahead point is not finite. Today the paddock is flat and
// its ground is finite everywhere inside its own bounds, so the only drop that
// exists is the paddock edge.
struct SheepAvoidanceEvidence {
    std::uint32_t subject_id = 0;
    bool avoidance_evaluated = false;
    bool drop_ahead = false;
    PaddockObstacle obstacle = PaddockObstacle::none;
    double obstacle_distance = 0.0;
    Vec3 avoidance_acceleration{};

    bool operator==(const SheepAvoidanceEvidence&) const = default;
};

using SheepAvoidanceEvidenceBuffer = std::array<SheepAvoidanceEvidence, kGameplaySheepCount>;

// Read-only evidence for the single bound that limits what every published
// steering term can do to one sheep together. Each social and dog term keeps its
// own maximum, but nothing previously stopped their sum, so a sheep standing
// inside several overlapping influences was accelerated by an unbounded total.
// The bound scales the sum rather than any individual term, so the vectors above
// stay exactly what the terms produced and this record is the only place the
// difference between "what the terms asked for" and "what integration used"
// lives. `applied_scale` is exactly `1.0` whenever the bound did not bind,
// including when it is switched off, so a scenario under the limit is
// arithmetically untouched. `summed_acceleration_magnitude` records how hard the
// terms wanted to push before the bound, which is the one number that explains a
// scale below `1.0` without re-adding six vectors by hand.
struct SheepCombinedInfluenceEvidence {
    std::uint32_t subject_id = 0;
    bool bound_evaluated = false;
    double summed_acceleration_magnitude = 0.0;
    double applied_scale = 0.0;
    Vec3 applied_acceleration{};

    bool operator==(const SheepCombinedInfluenceEvidence&) const = default;
};

using SheepCombinedInfluenceEvidenceBuffer =
    std::array<SheepCombinedInfluenceEvidence, kGameplaySheepCount>;

// Planar speed below which a sheep is treated as standing still for the purpose
// of choosing which way it faces. A sheep whose steering terms exactly cancel
// ends the tick with a velocity of exactly zero, and `atan2(0, 0)` is `0`, so
// without this floor an idle sheep would snap to face north; a residue left by
// two order-one terms cancelling is around `1e-16` and its direction is pure
// rounding noise. At `1e-9` world units/s a sheep covers `1.7e-11` units in one
// tick and under a micrometre in an hour of play, so no observable motion is
// discarded by keeping the previous heading instead. This is a numerical-noise
// floor rather than a design parameter, so it is a sheep rule here rather than a
// scenario-owned value.
inline constexpr double kSheepHeadingMotionSpeedFloor = 1.0e-9;

// Read-only evidence for the two limits that act on the result of integration
// rather than on any steering term. The combined-influence bound above limits
// how hard a sheep may be accelerated; these limit how fast it may end up
// moving and how quickly it may turn. `integrated_speed` is the planar speed the
// applied acceleration produced, before the clamp, and `applied_speed_scale` is
// the factor the clamp applied to it — exactly `1.0` whenever the clamp did not
// bind, so an under-limit sheep is arithmetically untouched. `applied_speed` is
// the speed integration actually left the sheep with, before collision refuses
// any axis.
//
// Heading follows motion rather than steering: `motion_heading_radians` is the
// direction of that applied velocity and `heading_change_radians` is the signed
// rotation the turn rate allowed toward it this tick.
// `motion_heading_followed` is false for a sheep moving slower than
// `kSheepHeadingMotionSpeedFloor`, which keeps its previous heading instead of
// facing numerical noise, and both heading fields then stay zero.
struct SheepMotionLimitEvidence {
    std::uint32_t subject_id = 0;
    bool limit_evaluated = false;
    bool motion_heading_followed = false;
    double integrated_speed = 0.0;
    double applied_speed_scale = 0.0;
    double applied_speed = 0.0;
    double motion_heading_radians = 0.0;
    double heading_change_radians = 0.0;

    bool operator==(const SheepMotionLimitEvidence&) const = default;
};

using SheepMotionLimitEvidenceBuffer = std::array<SheepMotionLimitEvidence, kGameplaySheepCount>;

inline constexpr SheepStateBuffer kDefaultGameplaySheepStates{{
    {.id = 1,
     .position = {.x = 14.5, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 16.0, .y = 1.0, .z = 19.5},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 17.5, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 15.25, .y = 1.0, .z = 21.5},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 16.75, .y = 1.0, .z = 21.5},
     .heading_radians = 0.0,
     .grounded = true},
}};

} // namespace wide_eye::game
