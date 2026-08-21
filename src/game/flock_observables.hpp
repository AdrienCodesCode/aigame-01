#pragma once

#include "game/math.hpp"
#include "game/sheep_state.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace wide_eye::game {

struct ChosenNeighborCountSummary {
    std::uint32_t total = 0;
    std::uint32_t minimum = 0;
    std::uint32_t maximum = 0;
    double mean = 0.0;

    bool operator==(const ChosenNeighborCountSummary&) const = default;
};

// Dog-relative summaries of the *flock*. Every sheep already publishes its own
// `dog_distance` and `dog_relative_bearing_radians` on the dog-stimulus record;
// what has been missing is the flock-level answer, and this is it. Like every
// other observable here these are ground-plane (x/z) values.
//
// - `centroid_distance` is the planar distance from the dog to the flock
//   centroid: one number for how much of a cause the dog is to the group as a
//   whole rather than to whichever sheep happens to be closest.
// - `centroid_bearing_radians` is the world bearing **of the dog seen from the
//   centroid**, in the same `atan2(x, -z)` convention every heading in this
//   project uses, normalized to `(-pi, pi]`. It answers "which side of the flock
//   is the dog standing on", and with a target direction that is the whole of
//   what a handler controls, because the accepted pressure term pushes each
//   sheep directly away from the dog. It is deliberately a *world* bearing
//   rather than one relative to the flock: the flock has no single heading —
//   `polarization` is zero for a standing flock — and a bearing that becomes
//   undefined exactly when the flock stops moving would be undefined exactly
//   when a handler most needs it. The per-sheep bearing can stay sheep-relative
//   because a sheep always has a heading; this one cannot.
// - `rear_sheep_id`, `rear_distance`, and `rear_offset` name the **rear-most
//   sheep relative to the dog**: the member whose offset from the centroid
//   projects least far along the push axis, the unit vector pointing from the
//   dog toward the centroid. The most negative `rear_offset` is therefore the
//   member the dog is furthest behind. Herding pressure is applied from behind,
//   so this is the sheep that receives it first and whose movement carries the
//   rest, and it is a different member from the nearest one whenever the flock
//   is not strung out along that axis.
// - `nearest_sheep_id` and `nearest_distance` are the smallest planar
//   dog-to-sheep distance in the flock. Under the accepted linear falloff that
//   member carries the largest dog stimulus, so it is the first that can leave
//   `settled`; it is the flock-level number a distance-keyed threshold would
//   have to use.
//
// Both selections break an exact tie on the lowest sheep ID, so a symmetric
// flock publishes one answer rather than a storage-order-dependent one.
//
// `evaluated` is false when the caller supplied no dog position, and every other
// field then stays zero. `bearing_defined` is false in the one degenerate case
// of a dog standing exactly on the centroid, where there is no direction between
// them and therefore no push axis and no rear member; `centroid_bearing_radians`
// and the three rear fields stay zero. The test is exact equality rather than a
// noise floor because this quantity is a coincidence of two positions, not the
// residue of cancelling terms.
struct FlockDogObservables {
    bool evaluated = false;
    bool bearing_defined = false;
    double centroid_distance = 0.0;
    double centroid_bearing_radians = 0.0;
    std::uint32_t nearest_sheep_id = 0;
    double nearest_distance = 0.0;
    std::uint32_t rear_sheep_id = 0;
    double rear_distance = 0.0;
    double rear_offset = 0.0;

    bool operator==(const FlockDogObservables&) const = default;
};

// Ground-plane observables for the fixed five-sheep Tracer 2 snapshot. The
// connectivity distance, chosen-neighbor counts, and dog position are explicit
// inputs so a caller can supply them from published state without running a
// second neighbor-selection path or acquiring a dependency on the dog motor.
// This function only reads published state; it neither chooses neighbors nor
// mutates simulation state.
struct FiveSheepObservables {
    Vec3 centroid{};
    double mean_radius = 0.0;
    double polarization = 0.0;
    // Bounded planar covariance anisotropy: 0 is isotropic and 1 is collinear.
    double elongation = 0.0;
    double group_speed = 0.0;
    std::array<double, kGameplaySheepCount> nearest_neighbor_spacing{};
    double mean_nearest_neighbor_spacing = 0.0;
    std::uint32_t connected_component_count = 0;
    ChosenNeighborCountSummary chosen_neighbors{};
    FlockDogObservables dog{};

    bool operator==(const FiveSheepObservables&) const = default;
};

[[nodiscard]] std::optional<FiveSheepObservables> compute_five_sheep_observables(
    const SheepStateBuffer& sheep,
    const std::array<std::uint32_t, kGameplaySheepCount>& chosen_neighbor_counts,
    double connectivity_distance, const std::optional<Vec3>& dog_position) noexcept;

// The temporal half of the flock observables. Bearing, distance, spacing, and
// shape are properties of one published snapshot; response latency, split and
// rejoin time, and settle time are elapsed ticks between events and cannot be.
//
// That temporal state is a **plain value the caller owns and threads**:
// `advance_flock_response_timing` takes the previous record and returns the next
// one, exactly the way each tick derives the next sheep buffer from the
// immutable prior buffer. `compute_five_sheep_observables` therefore stays a
// pure pass, and no clock, mutable static, or hidden singleton exists anywhere
// in this file. The record is fixed-size, allocation-free, comparable, and
// reproducible from a replay by folding it over the same published snapshots
// again.
//
// The caller supplies the published state each event is defined against, the
// way the pass above takes chosen-neighbor counts rather than selecting
// neighbors: the authoritative sheep buffer, the per-sheep `arousal_stimulus`
// from the dog-stimulus evidence, the connected-component count from the pass
// above, and the scenario's own `rest_arousal`. Observing is not steering:
// nothing here is read by any rule.
//
// Four events, each named against a rule that already exists rather than
// against a second definition invented here:
//
// - **Pressure acting** — some sheep's `arousal_stimulus` exceeds
//   `rest_arousal`. This is not a new threshold: it is exactly the test
//   ADR 0009's transition rule uses to decide whether a cause is acting at all,
//   which is what keeps "the flock is under pressure" from disagreeing with
//   "this sheep has been released". Its rising edge is *pressure onset* and its
//   falling edge is *release*.
// - **Response** — some sheep publishes `alert` or `driven`. Those are exactly
//   the two labels that mean a cause is acting on that sheep. `recovering` is
//   deliberately excluded, so a sheep still shedding arousal from an earlier
//   press is never counted as the flock answering a new one.
//   `response_latency_ticks` is the response tick minus the pressure-onset tick,
//   and stays absent for a press nothing answers.
// - **Split** — more than one connected component, the observable the pass above
//   already computes from the caller's connectivity distance. A split *episode*
//   runs from the tick the count first leaves one to the tick it returns to one.
//   A count that climbs again while the flock is already broken deepens the same
//   episode instead of starting a second one, because `rejoin_ticks` has to mean
//   "how long until the flock was whole again"; `peak_component_count` keeps the
//   deepening visible rather than discarding it. `time_to_split_ticks` is how
//   long the flock held together after pressure onset, and is absent for a split
//   that happened with no press acting.
// - **Settled** — every sheep publishes `settled`, the accepted Schmitt
//   trigger's own bottom label rather than a second rest test. `settle_ticks` is
//   measured from the *last* release: a cause that reappears while the flock is
//   still recovering discards the pending measurement and counts an
//   `interrupted_settle`, and the next release starts a fresh one. A flock that
//   is already wholly settled when pressure is released settles in zero ticks.
//
// Each of the four durations describes the most recent episode of its own kind
// and is cleared when a new episode of that kind opens, so a stale measurement
// can never be read as the current one. The first observation establishes the
// baseline: a press or a split already present on it counts as an onset there,
// because that is the first tick this record has seen.
struct FlockResponseTiming {
    // Observation bookkeeping. `tick` is whatever tick the caller last supplied;
    // a caller that observes once per fixed tick reads every tick field below as
    // an authoritative simulation tick.
    std::uint64_t observations = 0;
    std::uint64_t tick = 0;

    // Classification of the most recent observation.
    bool pressure_acting = false;
    bool flock_engaged = false;
    bool flock_settled = false;
    bool split = false;
    std::uint32_t connected_component_count = 0;

    // Response latency. `pressure_episodes` counts onsets, including one still
    // open; `unanswered_pressure_episodes` counts only released presses that
    // never drew a response.
    bool pressure_episode_open = false;
    std::uint64_t pressure_onset_tick = 0;
    std::uint32_t pressure_episodes = 0;
    std::uint32_t unanswered_pressure_episodes = 0;
    std::optional<std::uint64_t> response_latency_ticks{};

    // Split and rejoin. `peak_component_count` is the deepest count seen inside
    // the open or most recent split episode, and is zero before the first split.
    bool split_episode_open = false;
    std::uint64_t split_onset_tick = 0;
    std::uint32_t split_episodes = 0;
    std::uint32_t rejoins = 0;
    std::uint32_t peak_component_count = 0;
    std::uint64_t ticks_split = 0;
    std::optional<std::uint64_t> time_to_split_ticks{};
    std::optional<std::uint64_t> rejoin_ticks{};

    // Settle. `releases` counts observed falling edges of pressure.
    bool settle_pending = false;
    std::uint64_t release_tick = 0;
    std::uint32_t releases = 0;
    std::uint32_t interrupted_settles = 0;
    std::optional<std::uint64_t> settle_ticks{};

    bool operator==(const FlockResponseTiming&) const = default;
};

// Returns the record that follows `previous`, or `std::nullopt` for an input
// this pass cannot describe: the same sheep-validity and duplicate-ID rules the
// snapshot pass applies, a stimulus outside
// `[kSheepMinimumArousal, kSheepMaximumArousal]`, a `rest_arousal` outside the
// same range, a component count outside `[1, kGameplaySheepCount]`, or a tick
// that does not advance. `previous` must be a default-constructed record or one
// this function returned.
[[nodiscard]] std::optional<FlockResponseTiming> advance_flock_response_timing(
    const FlockResponseTiming& previous, std::uint64_t tick, const SheepStateBuffer& sheep,
    const std::array<double, kGameplaySheepCount>& arousal_stimulus,
    std::uint32_t connected_component_count, double rest_arousal) noexcept;

} // namespace wide_eye::game
