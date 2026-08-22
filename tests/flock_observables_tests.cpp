#include "game/flock_observables.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>

namespace {

using wide_eye::game::SheepBehaviorState;
using wide_eye::game::SheepState;
// Every fixture here is a five-member flock. The observable passes take a
// span, so the fixture type is a five-member array rather than the
// capacity-sized authoritative buffer: the span then carries the flock size
// the fixture actually means.
using FiveSheepBuffer = std::array<wide_eye::game::SheepState, 5>;
using wide_eye::game::Vec3;

constexpr double kPi = 3.14159265358979323846;
// The accepted `rest_arousal` from `SheepBehaviorConfiguration`. The timing pass
// takes it as an input rather than owning it, so the unit fixtures name it here
// exactly the way a scenario would supply it.
constexpr double kRestArousal = 0.125;

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "flock_observables_failure=" << name << '\n';
    }
    return condition;
}

bool near(double actual, double expected, double tolerance = 1.0e-12) {
    return std::abs(actual - expected) <= tolerance;
}

// Five sheep at distinct positions, all settled and unstimulated. The timing
// pass reads only IDs and behavior labels, so the geometry here exists solely to
// satisfy the shared validity rules.
FiveSheepBuffer settled_buffer() {
    FiveSheepBuffer sheep{};
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        sheep[index].id = static_cast<std::uint32_t>(index + 1);
        sheep[index].position = {.x = static_cast<double>(index)};
        sheep[index].grounded = true;
    }
    return sheep;
}

// One hand-authored observation: the first sheep carries the label and the
// cause, every other sheep stays settled with no stimulus at all. That is enough
// to drive every event, because pressure is "some sheep is stimulated", a
// response is "some sheep is alert or driven", and settled is "every sheep is".
struct TimingRun {
    wide_eye::game::FlockResponseTiming timing{};
    std::uint64_t tick = 0;
    bool rejected = false;
};

void step(TimingRun& run, SheepBehaviorState behavior, double stimulus,
          std::uint32_t component_count) {
    ++run.tick;
    FiveSheepBuffer sheep = settled_buffer();
    sheep[0].behavior = behavior;
    std::array<double, 5> stimuli{};
    stimuli[0] = stimulus;
    const auto next = wide_eye::game::advance_flock_response_timing(
        run.timing, run.tick, sheep, stimuli, component_count, kRestArousal);
    if (!next.has_value()) {
        run.rejected = true;
        return;
    }
    run.timing = *next;
}

bool measured(const std::optional<std::uint64_t>& value, std::uint64_t expected) {
    return value.has_value() && *value == expected;
}

} // namespace

int main() {
    const FiveSheepBuffer cross{{
        {.id = 1, .position = {.x = 0.0, .y = 2.0, .z = 0.0}, .velocity = {.x = 2.0}},
        {.id = 2, .position = {.x = 1.0, .y = 2.0, .z = 0.0}, .velocity = {.x = 4.0}},
        {.id = 3, .position = {.x = -1.0, .y = 2.0, .z = 0.0}, .velocity = {.x = 1.0}},
        {.id = 4, .position = {.x = 0.0, .y = 2.0, .z = 1.0}, .velocity = {.x = 3.0}},
        {.id = 5, .position = {.x = 0.0, .y = 2.0, .z = -1.0}, .velocity = {.x = 5.0}},
    }};
    constexpr std::array<std::uint32_t, 5> chosen{{0, 1, 2, 3, 4}};
    const auto cross_metrics =
        wide_eye::game::compute_flock_observables(cross, chosen, 1.0, std::nullopt);
    if (!check(cross_metrics.has_value(), "cross_fixture_is_valid") ||
        !check(cross_metrics->centroid == Vec3{.y = 2.0}, "centroid_is_arithmetic_mean") ||
        !check(near(cross_metrics->mean_radius, 0.8), "mean_planar_radius") ||
        !check(near(cross_metrics->polarization, 1.0), "polarization_uses_unit_headings") ||
        !check(near(cross_metrics->elongation, 0.0), "symmetric_cross_is_not_elongated") ||
        !check(near(cross_metrics->group_speed, 3.0), "group_speed_is_mean_member_speed") ||
        !check(near(cross_metrics->mean_nearest_neighbor_spacing, 1.0), "nearest_neighbor_mean") ||
        !check(cross_metrics->connected_component_count == 1,
               "threshold_edges_form_one_transitive_component") ||
        !check(cross_metrics->chosen_neighbors.total == 10 &&
                   cross_metrics->chosen_neighbors.minimum == 0 &&
                   cross_metrics->chosen_neighbors.maximum == 4 &&
                   near(cross_metrics->chosen_neighbors.mean, 2.0),
               "chosen_neighbor_count_summary") ||
        !check(cross_metrics->dog == wide_eye::game::FlockDogObservables{},
               "an_absent_dog_leaves_every_dog_observable_unevaluated_and_zero")) {
        return EXIT_FAILURE;
    }
    for (std::size_t index = 0; index < cross_metrics->member_count; ++index) {
        if (!check(near(cross_metrics->nearest_neighbor_spacing[index], 1.0),
                   "per_sheep_nearest_spacing")) {
            return EXIT_FAILURE;
        }
    }

    const FiveSheepBuffer line{{
        {.id = 10, .position = {.x = 0.0}, .velocity = {.x = 1.0}},
        {.id = 11, .position = {.x = 1.0}, .velocity = {.x = -1.0}},
        {.id = 12, .position = {.x = 2.0}},
        {.id = 13, .position = {.x = 10.0}, .velocity = {.z = 1.0}},
        {.id = 14, .position = {.x = 11.0}, .velocity = {.z = -1.0}},
    }};
    const auto line_metrics = wide_eye::game::compute_flock_observables(
        line, std::array<std::uint32_t, 5>{}, 1.0, std::nullopt);
    if (!check(line_metrics.has_value(), "line_fixture_is_valid") ||
        !check(near(line_metrics->centroid.x, 4.8), "offset_centroid") ||
        !check(near(line_metrics->mean_radius, 4.56), "offset_mean_radius") ||
        !check(near(line_metrics->elongation, 1.0), "collinear_group_is_maximally_elongated") ||
        !check(near(line_metrics->polarization, 0.0), "opposed_headings_have_zero_polarization") ||
        !check(near(line_metrics->group_speed, 0.8), "stationary_member_contributes_zero_speed") ||
        !check(line_metrics->connected_component_count == 2,
               "separated_chains_form_two_components")) {
        return EXIT_FAILURE;
    }

    // Dog bearing and distance at flock level. The cross's planar centroid is
    // exactly the origin, so a dog placed on an axis publishes an exact bearing
    // in the `atan2(x, -z)` convention: north is `0`, east is `+pi/2`, west is
    // `-pi/2`, and south is `+pi` rather than `-pi`.
    const auto due_east = wide_eye::game::compute_flock_observables(
        cross, chosen, 1.0, Vec3{.x = 5.0, .y = 2.0, .z = 0.0});
    const auto due_west = wide_eye::game::compute_flock_observables(
        cross, chosen, 1.0, Vec3{.x = -5.0, .y = 2.0, .z = 0.0});
    const auto due_south = wide_eye::game::compute_flock_observables(
        cross, chosen, 1.0, Vec3{.x = 0.0, .y = 2.0, .z = 5.0});
    const auto due_north = wide_eye::game::compute_flock_observables(
        cross, chosen, 1.0, Vec3{.x = 0.0, .y = 2.0, .z = -5.0});
    if (!check(due_east.has_value() && due_west.has_value() && due_south.has_value() &&
                   due_north.has_value(),
               "dog_fixtures_are_valid") ||
        !check(due_east->dog.evaluated && due_east->dog.bearing_defined &&
                   due_east->dog.centroid_distance == 5.0,
               "planar_centroid_distance_ignores_height") ||
        !check(near(due_north->dog.centroid_bearing_radians, 0.0) &&
                   near(due_east->dog.centroid_bearing_radians, 0.5 * kPi) &&
                   near(due_west->dog.centroid_bearing_radians, -0.5 * kPi) &&
                   near(due_south->dog.centroid_bearing_radians, kPi),
               "the_world_bearing_of_the_dog_from_the_centroid_is_signed_and_never_minus_pi") ||
        // The dog is due south, so the sheep at `z = +1` is both the nearest and
        // the rear-most; the sheep at `z = -1` is the furthest along the push
        // axis. The projection is exact because the axis is a unit basis vector.
        !check(due_south->dog.nearest_sheep_id == 4 && due_south->dog.nearest_distance == 4.0 &&
                   due_south->dog.rear_sheep_id == 4 && due_south->dog.rear_offset == -1.0 &&
                   due_south->dog.rear_distance == 4.0,
               "a_dog_south_of_a_cross_is_behind_the_southern_sheep")) {
        return EXIT_FAILURE;
    }

    // The rear-most sheep and the nearest sheep are different members whenever
    // the flock is not strung out along the push axis. Sheep 3 sits furthest
    // back along the axis but well off to the side; sheep 2 is closer to the dog
    // in a straight line and is not the one the dog is behind.
    const FiveSheepBuffer wedge{{
        {.id = 1, .position = {.x = 0.0, .z = 0.0}},
        {.id = 2, .position = {.x = 0.0, .z = 9.0}},
        {.id = 3, .position = {.x = 8.0, .z = 10.0}},
        {.id = 4, .position = {.x = -4.0, .z = 3.0}},
        {.id = 5, .position = {.x = 1.0, .z = 3.0}},
    }};
    const auto wedge_metrics = wide_eye::game::compute_flock_observables(
        wedge, std::array<std::uint32_t, 5>{}, 1.0, Vec3{.x = 1.0, .z = 13.0});
    if (!check(wedge_metrics.has_value() && wedge_metrics->centroid.x == 1.0 &&
                   wedge_metrics->centroid.z == 5.0,
               "wedge_centroid_is_exact") ||
        !check(wedge_metrics->dog.centroid_distance == 8.0 &&
                   near(wedge_metrics->dog.centroid_bearing_radians, kPi),
               "wedge_dog_stands_due_south_at_an_exact_distance") ||
        !check(wedge_metrics->dog.nearest_sheep_id == 2 &&
                   near(wedge_metrics->dog.nearest_distance, std::sqrt(17.0)),
               "the_nearest_sheep_is_the_one_closest_in_a_straight_line") ||
        !check(wedge_metrics->dog.rear_sheep_id == 3 && wedge_metrics->dog.rear_offset == -5.0 &&
                   near(wedge_metrics->dog.rear_distance, std::sqrt(58.0)),
               "the_rear_sheep_is_the_one_furthest_back_along_the_push_axis")) {
        return EXIT_FAILURE;
    }

    // Both selections break an exact tie on the lower ID. Sheep 1 and sheep 2
    // are mirror images across the push axis, so they tie on distance and on
    // projection at once.
    const FiveSheepBuffer mirrored{{
        {.id = 1, .position = {.x = -2.0, .z = -6.0}},
        {.id = 2, .position = {.x = 2.0, .z = -6.0}},
        {.id = 3, .position = {.x = 0.0, .z = 5.0}},
        {.id = 4, .position = {.x = -5.0, .z = 6.0}},
        {.id = 5, .position = {.x = 5.0, .z = 6.0}},
    }};
    const auto mirrored_metrics = wide_eye::game::compute_flock_observables(
        mirrored, std::array<std::uint32_t, 5>{}, 1.0, Vec3{.x = 0.0, .z = -10.0});
    const auto degenerate = wide_eye::game::compute_flock_observables(
        cross, chosen, 1.0, Vec3{.x = 0.0, .y = 2.0, .z = 0.0});
    if (!check(mirrored_metrics.has_value() && mirrored_metrics->centroid.x == 0.0 &&
                   mirrored_metrics->centroid.z == 1.0 &&
                   mirrored_metrics->dog.centroid_distance == 11.0 &&
                   mirrored_metrics->dog.centroid_bearing_radians == 0.0,
               "mirrored_fixture_puts_the_dog_due_north_at_an_exact_distance") ||
        !check(mirrored_metrics->dog.nearest_sheep_id == 1 &&
                   near(mirrored_metrics->dog.nearest_distance, std::sqrt(20.0)) &&
                   mirrored_metrics->dog.rear_sheep_id == 1 &&
                   mirrored_metrics->dog.rear_offset == -7.0,
               "an_exact_tie_selects_the_lower_sheep_id_rather_than_the_buffer_order") ||
        // A dog standing exactly on the centroid has no direction from it, so
        // there is no push axis and no rear member — but the nearest sheep is
        // still perfectly well defined.
        !check(degenerate.has_value() && degenerate->dog.evaluated &&
                   !degenerate->dog.bearing_defined && degenerate->dog.centroid_distance == 0.0 &&
                   degenerate->dog.centroid_bearing_radians == 0.0 &&
                   degenerate->dog.rear_sheep_id == 0 && degenerate->dog.rear_offset == 0.0 &&
                   degenerate->dog.rear_distance == 0.0 && degenerate->dog.nearest_sheep_id == 1 &&
                   degenerate->dog.nearest_distance == 0.0,
               "a_dog_exactly_on_the_centroid_has_a_distance_but_no_bearing_and_no_rear_sheep")) {
        return EXIT_FAILURE;
    }

    FiveSheepBuffer invalid = cross;
    invalid[1].id = invalid[0].id;
    if (!check(!wide_eye::game::compute_flock_observables(invalid, chosen, 1.0, std::nullopt),
               "duplicate_id_rejected") ||
        !check(!wide_eye::game::compute_flock_observables(
                   cross, chosen, std::numeric_limits<double>::quiet_NaN(), std::nullopt),
               "non_finite_threshold_rejected") ||
        !check(!wide_eye::game::compute_flock_observables(
                   cross, std::array<std::uint32_t, 5>{0, 0, 0, 0, 5}, 1.0, std::nullopt),
               "out_of_range_neighbor_count_rejected") ||
        !check(
            !wide_eye::game::compute_flock_observables(
                cross, chosen, 1.0, Vec3{.x = std::numeric_limits<double>::infinity(), .z = 1.0}),
            "non_finite_dog_position_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].velocity.x = std::numeric_limits<double>::infinity();
    if (!check(!wide_eye::game::compute_flock_observables(invalid, chosen, 1.0, std::nullopt),
               "non_finite_state_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].heading_radians = std::numeric_limits<double>::quiet_NaN();
    if (!check(!wide_eye::game::compute_flock_observables(invalid, chosen, 1.0, std::nullopt),
               "non_finite_heading_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].arousal = std::numeric_limits<double>::infinity();
    if (!check(!wide_eye::game::compute_flock_observables(invalid, chosen, 1.0, std::nullopt),
               "non_finite_arousal_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].behavior = static_cast<SheepBehaviorState>(255);
    if (!check(!wide_eye::game::compute_flock_observables(invalid, chosen, 1.0, std::nullopt),
               "unknown_behavior_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].temperament = static_cast<wide_eye::game::SheepTemperament>(255);
    if (!check(!wide_eye::game::compute_flock_observables(invalid, chosen, 1.0, std::nullopt),
               "unknown_temperament_rejected")) {
        return EXIT_FAILURE;
    }

    // One complete press: the cause arrives, the flock answers, the cause is
    // released, and the flock comes back to rest.
    TimingRun cycle;
    step(cycle, SheepBehaviorState::settled, 0.0, 1);
    if (!check(!cycle.rejected && cycle.timing.observations == 1 && !cycle.timing.pressure_acting &&
                   cycle.timing.flock_settled && cycle.timing.pressure_episodes == 0,
               "an_unpressed_settled_first_observation_opens_no_episode")) {
        return EXIT_FAILURE;
    }
    step(cycle, SheepBehaviorState::settled, 0.5, 1);
    step(cycle, SheepBehaviorState::alert, 0.5, 1);
    step(cycle, SheepBehaviorState::driven, 0.5, 1);
    step(cycle, SheepBehaviorState::driven, 0.5, 1);
    step(cycle, SheepBehaviorState::recovering, 0.0, 1);
    step(cycle, SheepBehaviorState::recovering, 0.0, 1);
    step(cycle, SheepBehaviorState::recovering, 0.0, 1);
    step(cycle, SheepBehaviorState::settled, 0.0, 1);
    if (!check(!cycle.rejected && cycle.timing.pressure_episodes == 1 &&
                   cycle.timing.pressure_onset_tick == 2 && !cycle.timing.pressure_episode_open,
               "pressure_onset_is_the_rising_edge_of_the_accepted_cause_test") ||
        !check(measured(cycle.timing.response_latency_ticks, 1),
               "response_latency_counts_ticks_from_the_cause_to_the_first_alert_sheep") ||
        !check(cycle.timing.releases == 1 && cycle.timing.release_tick == 6 &&
                   cycle.timing.unanswered_pressure_episodes == 0,
               "release_is_the_falling_edge_of_the_same_cause_test") ||
        !check(measured(cycle.timing.settle_ticks, 3) && !cycle.timing.settle_pending &&
                   cycle.timing.interrupted_settles == 0,
               "settle_time_counts_ticks_from_release_to_a_wholly_settled_flock") ||
        !check(cycle.timing.split_episodes == 0 && cycle.timing.ticks_split == 0 &&
                   !cycle.timing.rejoin_ticks.has_value(),
               "a_flock_that_never_splits_measures_no_split")) {
        return EXIT_FAILURE;
    }

    // A press nothing answers, and a flock that is already settled when it is
    // released. A stimulus above `rest_arousal` but below `alert_arousal` is
    // exactly this case in the accepted rule: it is a cause, and it never lifts
    // a sheep out of `settled`.
    TimingRun unanswered;
    step(unanswered, SheepBehaviorState::settled, 0.0, 1);
    for (int tick = 0; tick < 9; ++tick) {
        step(unanswered, SheepBehaviorState::settled, 0.1875, 1);
    }
    step(unanswered, SheepBehaviorState::settled, 0.0, 1);
    if (!check(!unanswered.rejected && unanswered.timing.pressure_episodes == 1 &&
                   unanswered.timing.unanswered_pressure_episodes == 1 &&
                   !unanswered.timing.response_latency_ticks.has_value(),
               "a_press_that_draws_no_response_records_no_latency") ||
        !check(measured(unanswered.timing.settle_ticks, 0),
               "a_flock_already_settled_at_release_settles_in_zero_ticks")) {
        return EXIT_FAILURE;
    }

    // A split that never rejoins. The episode stays open, so there is no rejoin
    // time to publish and the ticks spent broken keep accumulating.
    TimingRun broken;
    step(broken, SheepBehaviorState::settled, 0.0, 1);
    step(broken, SheepBehaviorState::settled, 0.0, 2);
    for (int tick = 0; tick < 8; ++tick) {
        step(broken, SheepBehaviorState::settled, 0.0, 2);
    }
    if (!check(!broken.rejected && broken.timing.split_episodes == 1 &&
                   broken.timing.split_onset_tick == 2 && broken.timing.split_episode_open &&
                   broken.timing.rejoins == 0 && !broken.timing.rejoin_ticks.has_value() &&
                   broken.timing.ticks_split == 9,
               "a_split_that_never_rejoins_publishes_no_rejoin_time") ||
        !check(!broken.timing.time_to_split_ticks.has_value(),
               "a_split_with_no_press_acting_has_no_time_to_split")) {
        return EXIT_FAILURE;
    }

    // A second split before the first rejoin. The count climbing from two to
    // three deepens the same episode rather than restarting the clock, because
    // rejoin time has to answer "how long until the flock was whole again".
    TimingRun deepening;
    step(deepening, SheepBehaviorState::driven, 0.5, 1);
    step(deepening, SheepBehaviorState::driven, 0.5, 2);
    step(deepening, SheepBehaviorState::driven, 0.5, 3);
    step(deepening, SheepBehaviorState::driven, 0.5, 2);
    step(deepening, SheepBehaviorState::driven, 0.5, 1);
    if (!check(!deepening.rejected && deepening.timing.split_episodes == 1 &&
                   deepening.timing.split_onset_tick == 2 &&
                   deepening.timing.peak_component_count == 3 && deepening.timing.ticks_split == 3,
               "a_deeper_split_before_a_rejoin_deepens_one_episode_instead_of_starting_two") ||
        !check(deepening.timing.rejoins == 1 && measured(deepening.timing.rejoin_ticks, 3) &&
                   !deepening.timing.split_episode_open,
               "rejoin_time_is_measured_from_the_first_tick_the_flock_left_one_component") ||
        !check(measured(deepening.timing.time_to_split_ticks, 1),
               "time_to_split_counts_from_the_pressure_onset_that_was_acting") ||
        // The press was already acting on the first observation, so that
        // observation is its onset and the driven sheep answers it instantly.
        !check(deepening.timing.pressure_episodes == 1 &&
                   deepening.timing.pressure_onset_tick == 1 &&
                   measured(deepening.timing.response_latency_ticks, 0),
               "a_cause_already_acting_on_the_first_observation_opens_its_episode_there")) {
        return EXIT_FAILURE;
    }

    // A cause that reappears during recovery. Settle time is measured from the
    // *last* release, so the pending measurement is discarded rather than
    // stretched, and the sheep that is still `recovering` when the new cause
    // arrives is not counted as a response to it.
    TimingRun interrupted;
    step(interrupted, SheepBehaviorState::settled, 0.0, 1);
    step(interrupted, SheepBehaviorState::settled, 0.5, 1);
    step(interrupted, SheepBehaviorState::driven, 0.5, 1);
    step(interrupted, SheepBehaviorState::recovering, 0.0, 1);
    step(interrupted, SheepBehaviorState::recovering, 0.0, 1);
    step(interrupted, SheepBehaviorState::recovering, 0.6, 1);
    if (!check(!interrupted.rejected && interrupted.timing.pressure_episodes == 2 &&
                   interrupted.timing.pressure_onset_tick == 6 &&
                   interrupted.timing.interrupted_settles == 1 &&
                   !interrupted.timing.settle_pending &&
                   !interrupted.timing.settle_ticks.has_value(),
               "a_cause_that_reappears_during_recovery_discards_the_pending_settle") ||
        !check(!interrupted.timing.response_latency_ticks.has_value(),
               "a_still_recovering_sheep_is_not_a_response_to_a_new_cause")) {
        return EXIT_FAILURE;
    }
    step(interrupted, SheepBehaviorState::alert, 0.6, 1);
    step(interrupted, SheepBehaviorState::driven, 0.6, 1);
    step(interrupted, SheepBehaviorState::recovering, 0.0, 1);
    step(interrupted, SheepBehaviorState::recovering, 0.0, 1);
    step(interrupted, SheepBehaviorState::recovering, 0.0, 1);
    step(interrupted, SheepBehaviorState::settled, 0.0, 1);
    if (!check(!interrupted.rejected && measured(interrupted.timing.response_latency_ticks, 1),
               "the_second_press_measures_its_own_latency") ||
        !check(interrupted.timing.releases == 2 && interrupted.timing.release_tick == 9 &&
                   measured(interrupted.timing.settle_ticks, 3) &&
                   interrupted.timing.interrupted_settles == 1 &&
                   interrupted.timing.unanswered_pressure_episodes == 0,
               "settle_time_is_measured_from_the_last_release_not_the_first")) {
        return EXIT_FAILURE;
    }

    // A press and a split already present on the very first observation. There
    // is no earlier tick to compare against, so both open their episode there.
    TimingRun immediate;
    step(immediate, SheepBehaviorState::driven, 0.5, 2);
    if (!check(!immediate.rejected && immediate.timing.pressure_episodes == 1 &&
                   immediate.timing.pressure_onset_tick == 1 &&
                   immediate.timing.split_episodes == 1 && immediate.timing.split_onset_tick == 1 &&
                   measured(immediate.timing.time_to_split_ticks, 0) &&
                   immediate.timing.peak_component_count == 2,
               "a_split_already_present_on_the_first_observation_opens_its_episode_there")) {
        return EXIT_FAILURE;
    }

    // The timing pass refuses what it cannot describe, exactly the way the
    // snapshot pass does.
    const FiveSheepBuffer valid = settled_buffer();
    const wide_eye::game::FlockResponseTiming fresh{};
    constexpr std::array<double, 5> quiet{};
    FiveSheepBuffer duplicate_ids = valid;
    duplicate_ids[1].id = duplicate_ids[0].id;
    FiveSheepBuffer unknown_behavior = valid;
    unknown_behavior[0].behavior = static_cast<SheepBehaviorState>(255);
    constexpr std::array<double, 5> above_range{{1.5, 0.0, 0.0, 0.0, 0.0}};
    constexpr std::array<double, 5> below_range{{-0.5, 0.0, 0.0, 0.0, 0.0}};
    const std::array<double, 5> non_finite{
        {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 0.0, 0.0}};
    if (!check(!wide_eye::game::advance_flock_response_timing(fresh, 1, duplicate_ids, quiet, 1,
                                                              kRestArousal),
               "timing_rejects_a_duplicate_sheep_id") ||
        !check(!wide_eye::game::advance_flock_response_timing(fresh, 1, unknown_behavior, quiet, 1,
                                                              kRestArousal),
               "timing_rejects_an_unknown_behavior_state") ||
        !check(!wide_eye::game::advance_flock_response_timing(fresh, 1, valid, above_range, 1,
                                                              kRestArousal) &&
                   !wide_eye::game::advance_flock_response_timing(fresh, 1, valid, below_range, 1,
                                                                  kRestArousal) &&
                   !wide_eye::game::advance_flock_response_timing(fresh, 1, valid, non_finite, 1,
                                                                  kRestArousal),
               "timing_rejects_a_stimulus_outside_the_stated_arousal_range") ||
        !check(!wide_eye::game::advance_flock_response_timing(fresh, 1, valid, quiet, 1, 1.5) &&
                   !wide_eye::game::advance_flock_response_timing(
                       fresh, 1, valid, quiet, 1, std::numeric_limits<double>::quiet_NaN()),
               "timing_rejects_a_rest_threshold_outside_the_stated_arousal_range") ||
        !check(!wide_eye::game::advance_flock_response_timing(fresh, 1, valid, quiet, 0,
                                                              kRestArousal) &&
                   !wide_eye::game::advance_flock_response_timing(fresh, 1, valid, quiet, 6,
                                                                  kRestArousal),
               "timing_rejects_a_component_count_no_five_sheep_can_produce")) {
        return EXIT_FAILURE;
    }
    const auto first =
        wide_eye::game::advance_flock_response_timing(fresh, 7, valid, quiet, 1, kRestArousal);
    if (!check(first.has_value() && first->tick == 7 && first->observations == 1,
               "the_first_observation_accepts_any_tick") ||
        !check(!wide_eye::game::advance_flock_response_timing(*first, 7, valid, quiet, 1,
                                                              kRestArousal) &&
                   !wide_eye::game::advance_flock_response_timing(*first, 6, valid, quiet, 1,
                                                                  kRestArousal),
               "a_repeated_or_rewound_tick_is_rejected_rather_than_folded_in")) {
        return EXIT_FAILURE;
    }

    std::cout << "flock_observables_result=pass\n";
    return EXIT_SUCCESS;
}
