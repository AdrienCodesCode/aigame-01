#pragma once

#include "game/dog_controller.hpp"
#include "game/sheep_state.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace wide_eye::game {

enum class GameplayScenarioId : std::uint8_t {
    paddock_start,
    wall_contact,
    closed_gate,
    open_gate,
    presentation_motion,
    sheep_only_separation,
    sheep_only_attraction,
    sheep_alignment_off,
    sheep_alignment_on,
    sheep_dog_pressure_off,
    sheep_dog_pressure_on,
    sheep_dog_approach_off,
    sheep_dog_approach_on,
    sheep_dog_facing_off,
    sheep_dog_facing_on,
    sheep_dog_line_of_sight_off,
    sheep_dog_line_of_sight_on,
    sheep_paddock_collision_closed_gate,
    sheep_paddock_collision_open_gate,
    sheep_temperament_neutral,
    sheep_temperament_varied,
    sheep_combined_influence_off,
    sheep_combined_influence_on,
    sheep_motion_limit_off,
    sheep_motion_limit_on,
    sheep_avoidance_off,
    sheep_avoidance_on,
    sheep_behavior_transitions_off,
    sheep_behavior_transitions_on,
    sheep_all_influences_diagnostic,
    fifty_sheep_paddock,
};

enum class SheepFixture : std::uint8_t {
    stationary,
    scripted_presentation_motion,
    local_social_response,
};

struct SheepSeparationConfiguration {
    bool enabled = false;
    double radius = 1.0;
    double maximum_acceleration = 4.0;

    bool operator==(const SheepSeparationConfiguration&) const = default;
};

struct SheepAttractionConfiguration {
    bool enabled = false;
    double radius = 4.0;
    double maximum_acceleration = 1.5;
    std::uint32_t neighbor_limit = 2;

    bool operator==(const SheepAttractionConfiguration&) const = default;
};

struct SheepAlignmentConfiguration {
    bool enabled = false;
    double radius = 3.0;
    double response_time_seconds = 1.0;
    double maximum_acceleration = 1.5;
    std::uint32_t neighbor_limit = 1;

    bool operator==(const SheepAlignmentConfiguration&) const = default;
};

struct SheepDogPressureConfiguration {
    bool enabled = false;
    double radius = 6.0;
    double maximum_acceleration = 3.0;

    bool operator==(const SheepDogPressureConfiguration&) const = default;
};

// Approach velocity is a separate dog-stimulus variable layered on the accepted
// distance-only pressure geometry. It shares the pressure radius and linear
// distance falloff so the radius boundary stays continuous, and it responds only
// to a closing dog. `reference_speed` is the closing speed at which the response
// saturates; faster approaches do not exceed `maximum_acceleration`.
struct SheepDogApproachConfiguration {
    bool enabled = false;
    double reference_speed = 3.0;
    double maximum_acceleration = 2.0;

    bool operator==(const SheepDogApproachConfiguration&) const = default;
};

// Facing is a third separate dog-stimulus variable layered on the same accepted
// distance-only pressure geometry. It shares the pressure radius and linear
// distance falloff so the radius boundary stays continuous, and it responds only
// to a dog whose prior forward direction points toward the sheep. The response
// is the positive part of the published facing alignment, so a dog looking away
// releases rather than pulling.
struct SheepDogFacingConfiguration {
    bool enabled = false;
    double maximum_acceleration = 1.5;

    bool operator==(const SheepDogFacingConfiguration&) const = default;
};

// Line of sight is a fourth separate dog-stimulus variable. It adds no vector of
// its own: an occluded dog releases the pressure, approach, and facing terms, so
// the published per-term vectors stay the applied vectors. Visibility is binary
// against the same analytic paddock obstacles the dog collides with, because a
// soft falloff would need a shape this project has not observed yet.
struct SheepDogLineOfSightConfiguration {
    bool enabled = false;

    bool operator==(const SheepDogLineOfSightConfiguration&) const = default;
};

// Temperament is the fifth dog-stimulus variable and the only one that is a
// property of the sheep rather than of the dog. Like line of sight it adds no
// vector of its own: it scales the pressure, approach, and facing responses a
// sheep produces from an unchanged stimulus, so a nervous and a stubborn sheep
// standing at the same distance and bearing publish the same geometry and move
// differently. `ordinary` is not configurable because it is defined as exactly
// neutral: it multiplies by `1.0` and therefore reproduces the accepted
// arithmetic bit for bit. The two configured factors are exact powers of two so
// the per-temperament ratio is exactly representable and a paired oracle can pin
// it with equality rather than a tolerance. Their magnitudes are a provisional
// legibility choice — doubling and halving — not a measured or biological value.
struct SheepTemperamentConfiguration {
    bool enabled = false;
    double nervous_response_scale = 2.0;
    double stubborn_response_scale = 0.5;

    bool operator==(const SheepTemperamentConfiguration&) const = default;
};

// Obstacle and drop avoidance: the steering term whose job is to make the hard
// collision authority unnecessary. It is a term like every other one — switched
// on independently, publishing its own vector, summed with the rest, and scaled
// by the combined bound below — and it deliberately replaces nothing.
// `resolve_sheep_against_paddock` remains the last positional authority, because
// a steering nudge that can be overwhelmed is not a boundary.
//
// `look_ahead_distance` is `6.25` world units. It is derived rather than picked:
// under the distance falloff and closing-speed response below, a term of maximum
// `A` acting over a look-ahead `L` can stop the reference speed
// `sqrt(A * L)`. With the accepted `5.0` maximum sheep speed and the `4.0`
// maximum below, `L` is exactly `25 / 4`. The term therefore has just enough
// room to stop the fastest sheep the game allows, and no more.
//
// `maximum_acceleration` is `4.0` world units/s^2, the same value as close-range
// separation's maximum and the combined bound: no avoidance push is stronger
// than the strongest single influence the flock already accepts. It bounds the
// term as a whole, so an obstacle and a drop reacted to in the same tick sum to
// at most this, exactly as separation's per-neighbour pushes do.
//
// Both magnitudes inherit the provisional status of the accepted values they are
// derived from; neither is measured or tuned. Response magnitude falls with
// boundary distance and actual speed into the boundary, while direction comes
// from the analytic face or ground normal. A sheep standing beside a wall or
// running parallel to one still feels nothing at all.
struct SheepAvoidanceConfiguration {
    bool enabled = false;
    double look_ahead_distance = 6.25;
    double maximum_acceleration = 4.0;

    bool operator==(const SheepAvoidanceConfiguration&) const = default;
};

// The one rule that limits what every social and dog term can do to a sheep
// together. Each term above already bounds itself, but their sum was unbounded,
// so a sheep inside several overlapping influences — or a nervous sheep whose
// temperament multiplies three dog terms at once — could be accelerated harder
// than any single accepted influence allows. The combination rule is a clamp on
// the summed vector: every per-term vector keeps its published value and the sum
// is scaled down as a whole when it exceeds `maximum_acceleration`. That keeps
// the existing per-term evidence exactly as inspectable as it was, which a
// prioritized or per-term-weighted scheme would destroy by silently rewriting
// the individual vectors. `maximum_acceleration` is set to the largest single
// accepted per-term maximum — close-range separation's `4.0` — so no combination
// of influences may accelerate a sheep harder than the strongest influence the
// flock already accepts on its own. That magnitude is a provisional legibility
// choice, not a measured or tuned value.
struct SheepCombinedInfluenceConfiguration {
    bool enabled = false;
    double maximum_acceleration = 4.0;

    bool operator==(const SheepCombinedInfluenceConfiguration&) const = default;
};

// The two limits that act on the result of integration rather than on any
// steering term. The combined bound above limits how hard a sheep may be
// accelerated; it does not limit how fast a sheep may end up moving, because
// acceleration applied for long enough accumulates without limit, and it does
// not limit how quickly a sheep may change which way it faces.
//
// `maximum_speed` is `5.0` world units/s: faster than the dog's accepted `4.5`
// walk, so a frightened sheep escapes a walking dog and the player has to
// sprint, and slower than the dog's accepted `8.0` sprint, so the dog can always
// overtake and flank the flock. A herding game in which a sheep can outrun the
// dog has no core loop.
//
// `maximum_turn_rate_radians_per_second` is `3.75`, against the dog motor's
// accepted `6.0`. The rule is that a sheep may not turn as fast as the dog
// herding it, because cutting across a turning animal is the player's main
// lever. Among the rates below the dog's whose per-tick budget at 60 Hz is an
// exactly representable binary fraction, `3.75` is the largest: its budget is
// exactly `1/16` radian per tick, which lets a paired oracle pin the turn with
// equality rather than with a tolerance.
//
// Both magnitudes are provisional legibility choices, not measured or tuned
// values. The turn rate limits the sheep's heading only; see
// [ADR 0007](../../docs/decisions/0007-bounded-sheep-speed-and-turning.md) for
// why motion is deliberately not constrained to that heading yet.
struct SheepMotionLimitConfiguration {
    bool enabled = false;
    double maximum_speed = 5.0;
    double maximum_turn_rate_radians_per_second = 3.75;

    bool operator==(const SheepMotionLimitConfiguration&) const = default;
};

// The arousal/recovery proxy and the four behavior states it selects.
//
// **Arousal is a named game parameter, not a claim about animal physiology.**
// Nothing here is a heart rate, a stress hormone, or a measured animal
// response; the accepted design calls player-facing "stress" shorthand for "an
// arousal/recovery design variable, not a claimed physiological model", and the
// research record asks for exactly this caution. Every magnitude below is a
// provisional legibility choice rather than a measured or tuned value.
//
// The proxy is a **rate-limited follower of the published dog stimulus**:
// arousal moves toward that stimulus at up to `rise_rate_per_second` while the
// stimulus is higher and at up to `recovery_rate_per_second` while it is lower.
// Because both endpoints lie in `[0, 1]`, arousal stays in `[0, 1]` without a
// clamp, and because it never overshoots it cannot oscillate on its own.
//
// `rise_rate_per_second` is `1.875` and `recovery_rate_per_second` is exactly
// one eighth of it. Under a saturated stimulus a sheep reaches full arousal in
// 32 ticks — about half a second — and needs 256 ticks — about four seconds — to
// shed it again, because the accepted loop makes *release* the slow, deliberate
// half of the pressure/release pair: "widen or pause to preserve cohesion and
// let sheep settle". Both rates are chosen so that one tick at 60 Hz is an
// exactly representable binary fraction — `1/32` and `1/256` — which lets a
// paired oracle pin them with equality rather than with a tolerance.
//
// The four thresholds map arousal onto the four states as a Schmitt trigger:
// each band is *entered* at its named arousal and *left* only at a lower one, so
// a sheep whose arousal hovers on an entry threshold cannot alternate between
// two labels on consecutive ticks. `rest_arousal` is the bottom of the ladder
// and does double duty as the "is a cause acting at all" test on the stimulus: a
// stimulus that cannot lift a sheep out of rest is not pressure, so the two
// cannot disagree about whether this sheep is under pressure. That leaves two
// hysteresis bands — `rest_arousal`..`alert_arousal` and
// `driven_release_arousal`..`driven_arousal` — whose widths, divided by the
// recovery rate, are a guaranteed minimum dwell: 32 ticks in `alert` and 64
// ticks in `driven` before either can be left downward.
//
// Arousal deliberately does **not** feed back into steering in this outcome; it
// is published state only. ADR 0009 records why.
struct SheepBehaviorConfiguration {
    bool enabled = false;
    double rise_rate_per_second = 1.875;
    double recovery_rate_per_second = 0.234375;
    double rest_arousal = 0.125;
    double alert_arousal = 0.25;
    double driven_release_arousal = 0.5;
    double driven_arousal = 0.75;

    bool operator==(const SheepBehaviorConfiguration&) const = default;
};

// Owns the complete deterministic starting contract for one game scenario.
// Controller-specific configuration stays nested under its subsystem, while
// sheep, objective, and future fixture state remain owned at the game level.
struct GameplayScenarioDefinition {
    GameplayScenarioId id = GameplayScenarioId::paddock_start;
    std::uint32_t version = 1;
    std::uint64_t seed = 0;
    DogControllerConfiguration dog{};
    // Paddock gate state is world state: the dog motor collides with it and the
    // sheep line-of-sight query needs it, so neither owns it.
    bool gate_open = false;
    SheepFixture sheep_fixture = SheepFixture::stationary;
    // How many members of `initial_sheep` this scenario actually simulates. The
    // buffer is sized at `kMaximumGameplaySheepCount`; this is the count every
    // loop, publication, observable, and evidence buffer iterates, and the
    // entries past it are never read, written, published, or serialized.
    //
    // The scenario supplies the **initial** value only. `GameplaySimulation`
    // copies it onto the published snapshot and carries it from there, so the
    // active count is data the simulation owns rather than a constant frozen at
    // construction. Nothing bakes it into a type or a template parameter, which
    // is what a later outcome that grows or shrinks a flock during play needs:
    // that outcome changes where the next tick's count comes from, not the shape
    // of any buffer, snapshot, or published record.
    //
    // Must be at least one and at most `kMaximumGameplaySheepCount`; the
    // simulation asserts both before the first tick.
    std::size_t sheep_count = kDefaultGameplaySheepCount;
    SheepStateBuffer initial_sheep = kDefaultGameplaySheepStates;
    SheepSeparationConfiguration sheep_separation{};
    SheepAttractionConfiguration sheep_attraction{};
    SheepAlignmentConfiguration sheep_alignment{};
    SheepDogPressureConfiguration sheep_dog_pressure{};
    SheepDogApproachConfiguration sheep_dog_approach{};
    SheepDogFacingConfiguration sheep_dog_facing{};
    SheepDogLineOfSightConfiguration sheep_dog_line_of_sight{};
    SheepTemperamentConfiguration sheep_temperament{};
    SheepAvoidanceConfiguration sheep_avoidance{};
    SheepCombinedInfluenceConfiguration sheep_combined_influence{};
    SheepMotionLimitConfiguration sheep_motion_limit{};
    SheepBehaviorConfiguration sheep_behavior{};

    bool operator==(const GameplayScenarioDefinition&) const = default;
};

[[nodiscard]] std::optional<GameplayScenarioDefinition>
find_gameplay_scenario(std::string_view name) noexcept;
[[nodiscard]] std::string_view gameplay_scenario_name(GameplayScenarioId scenario) noexcept;

} // namespace wide_eye::game
