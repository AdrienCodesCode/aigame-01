#include "render/influence_debug_view.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace wide_eye::render {
namespace {

// One color per lane. Hue alone is never the only channel — the lane index is
// countable off the mast — but the palette still avoids putting the two terms a
// reviewer most often compares, attraction and separation, on the red/green axis
// that the common forms of color blindness confuse. `applied` is deliberately
// near-black: it is the only line that has to read as a *result* against both
// the grass and the sky.
constexpr std::array<std::array<float, 3>, kInfluenceChannelCount> kChannelColors{{
    {{1.00F, 0.45F, 0.12F}}, // separation: orange
    {{0.24F, 0.86F, 0.38F}}, // attraction: green
    {{0.16F, 0.82F, 0.96F}}, // alignment: cyan
    {{1.00F, 0.22F, 0.72F}}, // dog pressure: magenta
    {{0.64F, 0.42F, 1.00F}}, // dog approach: violet
    {{1.00F, 0.90F, 0.18F}}, // dog facing: yellow
    {{0.16F, 0.34F, 1.00F}}, // avoidance: blue
    {{0.04F, 0.04F, 0.06F}}, // applied: near-black
}};

// The four behavior labels, in `SheepBehaviorState` order. The rung count is the
// color-independent half of the same statement.
constexpr std::array<std::array<float, 3>, 4> kBehaviorColors{{
    {{0.34F, 0.76F, 0.36F}}, // settled
    {{0.96F, 0.86F, 0.24F}}, // alert
    {{0.96F, 0.34F, 0.18F}}, // driven
    {{0.36F, 0.60F, 0.96F}}, // recovering
}};

constexpr std::array<float, 3> kMastColor{0.14F, 0.14F, 0.17F};
// A lane whose term did not run this tick. Distinct from every channel color and
// from the mast, so "switched off or skipped" never reads as "produced zero".
constexpr std::array<float, 3> kNotEvaluatedColor{0.46F, 0.46F, 0.50F};
constexpr std::array<float, 3> kArousalScaleColor{0.58F, 0.58F, 0.62F};
constexpr std::array<float, 3> kHeadingPreviousColor{0.42F, 0.42F, 0.46F};
constexpr std::array<float, 3> kHeadingCurrentColor{0.98F, 0.98F, 0.98F};
constexpr std::array<float, 3> kHeadingTargetColor{1.00F, 0.72F, 0.26F};
constexpr std::array<float, 3> kCentroidColor{0.98F, 0.98F, 0.98F};
constexpr std::array<float, 3> kPushAxisColor{0.60F, 0.60F, 0.66F};
constexpr std::array<float, 3> kBalanceColor{1.00F, 0.36F, 0.06F};

constexpr double kTwoPi = 6.28318530717958647692;

[[nodiscard]] std::array<float, 3> to_float3(const game::Vec3& value) noexcept {
    return {static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z)};
}

[[nodiscard]] game::Vec3 offset(const game::Vec3& base, double x, double y, double z) noexcept {
    return {.x = base.x + x, .y = base.y + y, .z = base.z + z};
}

// Every heading in this project is measured with the same `atan2(x, -z)`
// convention, so a forward direction is always this.
[[nodiscard]] game::Vec3 heading_direction(double heading_radians) noexcept {
    return {.x = std::sin(heading_radians), .y = 0.0, .z = -std::cos(heading_radians)};
}

// Bounded appender. Overflow is impossible while the ceilings in the header hold,
// but silently truncating a diagnostic frame would be the worst possible failure
// mode for a view whose whole purpose is to be trusted, so the guard stays.
void append(InfluenceDebugFrame& frame, DebugSegmentRole role, InfluenceChannel channel,
            std::uint32_t subject_id, std::uint32_t object_id, const game::Vec3& start,
            const game::Vec3& end, const std::array<float, 3>& color) noexcept {
    if (frame.segment_count >= frame.segments.size()) {
        return;
    }
    frame.segments[frame.segment_count] = {
        .start = to_float3(start),
        .end = to_float3(end),
        .color = color,
        .role = role,
        .channel = channel,
        .subject_id = subject_id,
        .object_id = object_id,
    };
    ++frame.segment_count;
}

// Draws one influence arrow in the ground plane and reports whether it drew
// anything. Every published influence vector is exactly planar, which the
// headless oracle pins as an equality; the planar magnitude is therefore the
// whole magnitude, and a barb rotated about world up is the readable arrowhead
// from the elevated review camera.
bool append_arrow(InfluenceDebugFrame& frame, InfluenceChannel channel, std::uint32_t subject_id,
                  const game::Vec3& origin, const game::Vec3& acceleration,
                  bool doubled_shaft) noexcept {
    const double magnitude = std::hypot(acceleration.x, acceleration.z);
    if (!std::isfinite(magnitude) || magnitude <= 0.0) {
        return false;
    }
    const double unclamped_length = magnitude * kInfluenceArrowScaleSecondsSquared;
    if (unclamped_length < kInfluenceArrowMinimumLength) {
        return false;
    }
    const double length = std::min(unclamped_length, kInfluenceArrowMaximumLength);
    if (unclamped_length > kInfluenceArrowMaximumLength) {
        ++frame.clamped_arrow_count;
    }
    const double direction_x = acceleration.x / magnitude;
    const double direction_z = acceleration.z / magnitude;
    const game::Vec3 tip = offset(origin, direction_x * length, 0.0, direction_z * length);
    const std::array<float, 3>& color = kChannelColors[influence_channel_index(channel)];

    if (doubled_shaft) {
        // A perpendicular in the ground plane. Line width is not portable in a
        // core profile, so "heavier" has to be a second line rather than a
        // thicker one.
        const double lateral_x = -direction_z * kInfluenceAppliedShaftOffset;
        const double lateral_z = direction_x * kInfluenceAppliedShaftOffset;
        append(frame, DebugSegmentRole::influence_shaft, channel, subject_id, 0,
               offset(origin, lateral_x, 0.0, lateral_z), offset(tip, lateral_x, 0.0, lateral_z),
               color);
        append(frame, DebugSegmentRole::influence_shaft, channel, subject_id, 0,
               offset(origin, -lateral_x, 0.0, -lateral_z),
               offset(tip, -lateral_x, 0.0, -lateral_z), color);
    } else {
        append(frame, DebugSegmentRole::influence_shaft, channel, subject_id, 0, origin, tip,
               color);
    }

    const double head_length =
        std::min(kInfluenceArrowHeadMaximumLength, length * kInfluenceArrowHeadFraction);
    const double back_angle = std::atan2(-direction_z, -direction_x);
    for (const double sign : {1.0, -1.0}) {
        const double angle = back_angle + sign * kInfluenceArrowHeadAngleRadians;
        append(frame, DebugSegmentRole::influence_head, channel, subject_id, 0, tip,
               offset(tip, std::cos(angle) * head_length, 0.0, std::sin(angle) * head_length),
               color);
    }
    ++frame.arrow_count;
    return true;
}

void append_ray(InfluenceDebugFrame& frame, DebugSegmentRole role, std::uint32_t subject_id,
                const game::Vec3& base, double heading_radians, double from_distance,
                double to_distance, const std::array<float, 3>& color) noexcept {
    const game::Vec3 direction = heading_direction(heading_radians);
    append(frame, role, InfluenceChannel::separation, subject_id, 0,
           offset(base, direction.x * from_distance, 0.0, direction.z * from_distance),
           offset(base, direction.x * to_distance, 0.0, direction.z * to_distance), color);
}

[[nodiscard]] const game::SheepState* find_sheep(const game::SheepStateBuffer& sheep,
                                                 std::uint32_t id) noexcept {
    for (const game::SheepState& candidate : sheep) {
        if (candidate.id == id) {
            return &candidate;
        }
    }
    return nullptr;
}

void append_sheep_segments(InfluenceDebugFrame& frame, const game::GameplaySnapshot& snapshot,
                           const game::GameplayScenarioDefinition& scenario,
                           std::size_t index) noexcept {
    const game::SheepState& sheep = snapshot.sheep[index];
    const game::SheepSocialEvidence& social = snapshot.sheep_social_evidence[index];
    const game::SheepDogPressureEvidence& dog = snapshot.sheep_dog_pressure_evidence[index];
    const game::SheepAvoidanceEvidence& avoidance = snapshot.sheep_avoidance_evidence[index];
    const game::SheepCombinedInfluenceEvidence& combined =
        snapshot.sheep_combined_influence_evidence[index];
    const game::SheepMotionLimitEvidence& motion = snapshot.sheep_motion_limit_evidence[index];
    const std::uint32_t id = sheep.id;

    const double top_lane_height =
        kInfluenceLaneBaseHeight +
        static_cast<double>(kInfluenceChannelCount - 1) * kInfluenceLaneSpacing;
    append(frame, DebugSegmentRole::mast, InfluenceChannel::separation, id, 0,
           offset(sheep.position, 0.0, kInfluenceLaneBaseHeight - kInfluenceLaneSpacing, 0.0),
           offset(sheep.position, 0.0, top_lane_height, 0.0), kMastColor);

    // Lane order is the legend. `evaluated` decides only the tick's color, so a
    // term that ran and produced exactly zero stays distinguishable from one
    // that never ran at all.
    const std::array<game::Vec3, kInfluenceChannelCount> accelerations{{
        social.separation_acceleration,
        social.attraction_acceleration,
        social.alignment_acceleration,
        dog.pressure_acceleration,
        dog.approach_acceleration,
        dog.facing_acceleration,
        avoidance.avoidance_acceleration,
        combined.applied_acceleration,
    }};
    const std::array<bool, kInfluenceChannelCount> evaluated{{
        true, // the three social terms always run
        true,
        true,
        dog.stimulus_evaluated,
        dog.stimulus_evaluated,
        dog.stimulus_evaluated,
        avoidance.avoidance_evaluated,
        combined.bound_evaluated,
    }};

    for (std::size_t lane = 0; lane < kInfluenceChannelCount; ++lane) {
        const auto channel = static_cast<InfluenceChannel>(lane);
        const double lane_height =
            kInfluenceLaneBaseHeight + static_cast<double>(lane) * kInfluenceLaneSpacing;
        const game::Vec3 lane_origin = offset(sheep.position, 0.0, lane_height, 0.0);
        append(frame, DebugSegmentRole::lane_tick, channel, id, 0, lane_origin,
               offset(lane_origin, kInfluenceLaneTickLength, 0.0, 0.0),
               evaluated[lane] ? kChannelColors[lane] : kNotEvaluatedColor);
        static_cast<void>(append_arrow(frame, channel, id, lane_origin, accelerations[lane],
                                       channel == InfluenceChannel::applied));
    }

    // Chosen neighbours. The published IDs are the authority; a link is drawn
    // only when the ID resolves to a sheep in the same snapshot, and one that
    // does not is counted rather than quietly dropped.
    const game::Vec3 attraction_base = offset(sheep.position, 0.0, kAttractionLinkHeight, 0.0);
    for (std::uint32_t slot = 0;
         slot < social.attraction_neighbor_count && slot < social.attraction_neighbor_ids.size();
         ++slot) {
        const std::uint32_t neighbor_id = social.attraction_neighbor_ids[slot];
        const game::SheepState* neighbor = find_sheep(snapshot.sheep, neighbor_id);
        if (neighbor == nullptr) {
            ++frame.unresolved_neighbor_count;
            continue;
        }
        append(frame, DebugSegmentRole::attraction_link, InfluenceChannel::attraction, id,
               neighbor_id, attraction_base,
               offset(neighbor->position, 0.0, kAttractionLinkHeight, 0.0),
               kChannelColors[influence_channel_index(InfluenceChannel::attraction)]);
        ++frame.attraction_link_count;
    }
    const game::Vec3 alignment_base = offset(sheep.position, 0.0, kAlignmentLinkHeight, 0.0);
    for (std::uint32_t slot = 0;
         slot < social.alignment_neighbor_count && slot < social.alignment_neighbor_ids.size();
         ++slot) {
        const std::uint32_t neighbor_id = social.alignment_neighbor_ids[slot];
        const game::SheepState* neighbor = find_sheep(snapshot.sheep, neighbor_id);
        if (neighbor == nullptr) {
            ++frame.unresolved_neighbor_count;
            continue;
        }
        append(frame, DebugSegmentRole::alignment_link, InfluenceChannel::alignment, id,
               neighbor_id, alignment_base,
               offset(neighbor->position, 0.0, kAlignmentLinkHeight, 0.0),
               kChannelColors[influence_channel_index(InfluenceChannel::alignment)]);
        ++frame.alignment_link_count;
    }

    // Arousal, against the scenario's own thresholds rather than against numbers
    // repeated here, so the scale marks cannot drift from the accepted rule.
    const game::Vec3 bar_base = offset(sheep.position, kArousalBarOffsetX, 0.02, 0.0);
    const std::array<double, 5> scale_fractions{
        scenario.sheep_behavior.rest_arousal, scenario.sheep_behavior.alert_arousal,
        scenario.sheep_behavior.driven_release_arousal, scenario.sheep_behavior.driven_arousal,
        game::kSheepMaximumArousal};
    for (const double fraction : scale_fractions) {
        const game::Vec3 tick = offset(bar_base, 0.0, fraction * kArousalBarHeight, 0.0);
        append(frame, DebugSegmentRole::arousal_scale, InfluenceChannel::separation, id, 0, tick,
               offset(tick, 0.0, 0.0, kArousalTickLength), kArousalScaleColor);
    }
    const std::size_t behavior_index =
        static_cast<std::size_t>(sheep.behavior) < kBehaviorColors.size()
            ? static_cast<std::size_t>(sheep.behavior)
            : 0U;
    append(frame, DebugSegmentRole::arousal_bar, InfluenceChannel::separation, id, 0, bar_base,
           offset(bar_base, 0.0, sheep.arousal * kArousalBarHeight, 0.0),
           kBehaviorColors[behavior_index]);
    for (std::size_t rung = 0; rung <= behavior_index; ++rung) {
        const game::Vec3 base =
            offset(bar_base, 0.0,
                   kBehaviorRungBaseHeight + static_cast<double>(rung) * kBehaviorRungSpacing, 0.0);
        append(frame, DebugSegmentRole::behavior_rung, InfluenceChannel::separation, id, 0, base,
               offset(base, 0.0, 0.0, kBehaviorRungLength), kBehaviorColors[behavior_index]);
    }

    // Heading: where the sheep faced, where the turn rate left it, and where it
    // is aiming. The gap between the first two is what the accepted turn limit
    // achieved this tick; the gap between the last two is what it still owes.
    const game::Vec3 heading_base = offset(sheep.position, 0.0, kHeadingRayHeight, 0.0);
    const double previous_heading = sheep.heading_radians - motion.heading_change_radians;
    const double first_dash_end = 0.5 * (kHeadingRayLength - kHeadingDashGap);
    append_ray(frame, DebugSegmentRole::heading_previous, id, heading_base, previous_heading, 0.0,
               std::max(first_dash_end, 0.0), kHeadingPreviousColor);
    append_ray(frame, DebugSegmentRole::heading_previous, id, heading_base, previous_heading,
               std::max(first_dash_end + kHeadingDashGap, 0.0), kHeadingRayLength,
               kHeadingPreviousColor);
    append_ray(frame, DebugSegmentRole::heading_current, id, heading_base, sheep.heading_radians,
               0.0, kHeadingRayLength, kHeadingCurrentColor);
    if (motion.limit_evaluated && motion.motion_heading_followed) {
        append_ray(frame, DebugSegmentRole::heading_target, id, heading_base,
                   motion.motion_heading_radians, 0.0,
                   kHeadingRayLength + kHeadingTargetExtraLength, kHeadingTargetColor);
        ++frame.heading_target_count;
    }
}

void append_flock_segments(InfluenceDebugFrame& frame,
                           const game::GameplaySnapshot& snapshot) noexcept {
    std::array<std::uint32_t, game::kGameplaySheepCount> chosen_neighbor_counts{};
    for (std::size_t index = 0; index < chosen_neighbor_counts.size(); ++index) {
        chosen_neighbor_counts[index] =
            snapshot.sheep_social_evidence[index].attraction_neighbor_count;
    }
    const std::optional<game::FiveSheepObservables> observables =
        game::compute_five_sheep_observables(snapshot.sheep, chosen_neighbor_counts,
                                             kInfluenceDebugConnectivityDistance,
                                             std::optional<game::Vec3>{snapshot.dog.position});
    if (!observables.has_value()) {
        return;
    }
    frame.flock_markers_present = true;
    frame.centroid = observables->centroid;
    frame.flock_dog = observables->dog;

    const game::Vec3 centroid_ground = offset(observables->centroid, 0.0, 0.05, 0.0);
    append(frame, DebugSegmentRole::flock_centroid, InfluenceChannel::separation, 0, 0,
           offset(centroid_ground, -kFlockCentroidArmLength, 0.0, 0.0),
           offset(centroid_ground, kFlockCentroidArmLength, 0.0, 0.0), kCentroidColor);
    append(frame, DebugSegmentRole::flock_centroid, InfluenceChannel::separation, 0, 0,
           offset(centroid_ground, 0.0, 0.0, -kFlockCentroidArmLength),
           offset(centroid_ground, 0.0, 0.0, kFlockCentroidArmLength), kCentroidColor);
    append(frame, DebugSegmentRole::flock_centroid, InfluenceChannel::separation, 0, 0,
           centroid_ground, offset(centroid_ground, 0.0, kFlockMarkerStemHeight, 0.0),
           kCentroidColor);

    if (!observables->dog.evaluated || !observables->dog.bearing_defined) {
        return;
    }

    // The push axis, taken from the published bearing rather than re-derived
    // from the two positions, so what is drawn is what the observable says.
    // `centroid_bearing_radians` is the bearing *of the dog seen from the
    // centroid*, so the axis the pressure term drives along is its reverse.
    const game::Vec3 centroid_to_dog = heading_direction(observables->dog.centroid_bearing_radians);
    const game::Vec3 push_axis{.x = -centroid_to_dog.x, .y = 0.0, .z = -centroid_to_dog.z};
    append(frame, DebugSegmentRole::push_axis, InfluenceChannel::separation, 0, 0,
           offset(centroid_ground, centroid_to_dog.x * observables->dog.centroid_distance, 0.0,
                  centroid_to_dog.z * observables->dog.centroid_distance),
           centroid_ground, kPushAxisColor);

    const double rear_offset = observables->dog.rear_offset;
    frame.balance_point =
        offset(observables->centroid, push_axis.x * rear_offset, 0.0, push_axis.z * rear_offset);
    frame.balance_point_defined = true;
    frame.dog_behind_flock = observables->dog.centroid_distance >= -rear_offset;

    const game::Vec3 ring_center = offset(frame.balance_point, 0.0, 0.05, 0.0);
    for (std::size_t step = 0; step < kBalanceRingSegmentCount; ++step) {
        const double angle_start =
            kTwoPi * static_cast<double>(step) / static_cast<double>(kBalanceRingSegmentCount);
        const double angle_end =
            kTwoPi * static_cast<double>(step + 1) / static_cast<double>(kBalanceRingSegmentCount);
        append(frame, DebugSegmentRole::balance_point, InfluenceChannel::separation, 0, 0,
               offset(ring_center, std::cos(angle_start) * kBalanceRingRadius, 0.0,
                      std::sin(angle_start) * kBalanceRingRadius),
               offset(ring_center, std::cos(angle_end) * kBalanceRingRadius, 0.0,
                      std::sin(angle_end) * kBalanceRingRadius),
               kBalanceColor);
    }
    append(frame, DebugSegmentRole::balance_point, InfluenceChannel::separation, 0, 0, ring_center,
           offset(ring_center, 0.0, kFlockMarkerStemHeight, 0.0), kBalanceColor);
    if (!frame.dog_behind_flock) {
        // A cross through the ring, so "the dog is inside the flock's rear" is
        // a shape rather than only a color.
        const double arm = kBalanceRingRadius * 0.70710678118654752440;
        append(frame, DebugSegmentRole::balance_breached, InfluenceChannel::separation, 0, 0,
               offset(ring_center, -arm, 0.0, -arm), offset(ring_center, arm, 0.0, arm),
               kBalanceColor);
        append(frame, DebugSegmentRole::balance_breached, InfluenceChannel::separation, 0, 0,
               offset(ring_center, -arm, 0.0, arm), offset(ring_center, arm, 0.0, -arm),
               kBalanceColor);
    }

    if (const game::SheepState* rear = find_sheep(snapshot.sheep, observables->dog.rear_sheep_id);
        rear != nullptr) {
        append(frame, DebugSegmentRole::rear_member, InfluenceChannel::separation,
               observables->dog.rear_sheep_id, 0, offset(rear->position, 0.0, 0.05, 0.0),
               offset(rear->position, 0.0, kFlockMarkerStemHeight, 0.0), kBalanceColor);
    }
}

} // namespace

std::array<float, 3> influence_channel_color(InfluenceChannel channel) noexcept {
    const std::size_t lane = influence_channel_index(channel);
    return lane < kChannelColors.size() ? kChannelColors[lane] : kChannelColors.front();
}

std::string_view influence_channel_name(InfluenceChannel channel) noexcept {
    switch (channel) {
    case InfluenceChannel::separation:
        return "separation";
    case InfluenceChannel::attraction:
        return "attraction";
    case InfluenceChannel::alignment:
        return "alignment";
    case InfluenceChannel::dog_pressure:
        return "dog_pressure";
    case InfluenceChannel::dog_approach:
        return "dog_approach";
    case InfluenceChannel::dog_facing:
        return "dog_facing";
    case InfluenceChannel::avoidance:
        return "avoidance";
    case InfluenceChannel::applied:
        return "applied";
    }
    return "unknown";
}

std::string_view debug_segment_role_name(DebugSegmentRole role) noexcept {
    switch (role) {
    case DebugSegmentRole::mast:
        return "mast";
    case DebugSegmentRole::lane_tick:
        return "lane_tick";
    case DebugSegmentRole::influence_shaft:
        return "influence_shaft";
    case DebugSegmentRole::influence_head:
        return "influence_head";
    case DebugSegmentRole::attraction_link:
        return "attraction_link";
    case DebugSegmentRole::alignment_link:
        return "alignment_link";
    case DebugSegmentRole::arousal_scale:
        return "arousal_scale";
    case DebugSegmentRole::arousal_bar:
        return "arousal_bar";
    case DebugSegmentRole::behavior_rung:
        return "behavior_rung";
    case DebugSegmentRole::heading_previous:
        return "heading_previous";
    case DebugSegmentRole::heading_current:
        return "heading_current";
    case DebugSegmentRole::heading_target:
        return "heading_target";
    case DebugSegmentRole::flock_centroid:
        return "flock_centroid";
    case DebugSegmentRole::push_axis:
        return "push_axis";
    case DebugSegmentRole::balance_point:
        return "balance_point";
    case DebugSegmentRole::balance_breached:
        return "balance_breached";
    case DebugSegmentRole::rear_member:
        return "rear_member";
    }
    return "unknown";
}

InfluenceDebugFrame
build_influence_debug_frame(const game::GameplaySnapshot& snapshot,
                            const game::GameplayScenarioDefinition& scenario) noexcept {
    InfluenceDebugFrame frame{};
    frame.tick = snapshot.tick;
    for (std::size_t index = 0; index < snapshot.sheep.size(); ++index) {
        append_sheep_segments(frame, snapshot, scenario, index);
    }
    append_flock_segments(frame, snapshot);
    return frame;
}

} // namespace wide_eye::render
