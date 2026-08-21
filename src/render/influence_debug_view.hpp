#pragma once

#include "game/flock_observables.hpp"
#include "game/gameplay_scenario.hpp"
#include "game/gameplay_simulation.hpp"
#include "game/math.hpp"
#include "game/sheep_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace wide_eye::render {

// Turns one published gameplay tick into the line primitives that explain it.
//
// This unit deliberately contains no OpenGL, no SDL, and no window: it is a
// pure function from a published snapshot plus the scenario's own read-only
// contract to a fixed-capacity list of world-space segments. That split is what
// makes the view checkable on a host with no usable OpenGL context — the
// geometry, the evidence-to-primitive mapping, and the determinism are all
// testable without a display, and only the upload and the draw need one.
//
// Presentation reads published state and never writes it. Nothing here calls
// into a rule, and no value below is read back by the simulation.

// The ordered influence lanes. Order is the identity of a lane: every arrow for
// a sheep is drawn at a fixed height above it, one lane per channel, and the
// mast carries one countable tick per lane. A reviewer who cannot rely on color
// counts ticks up from the sheep's back to name the term.
//
// The first seven are the seven accepted steering terms, each of which publishes
// its own acceleration vector. `applied` is **not** a term: it is the result the
// combined-influence bound handed to integration, drawn last and with a doubled
// shaft so it cannot be mistaken for one of the causes above it.
enum class InfluenceChannel : std::uint8_t {
    separation,
    attraction,
    alignment,
    dog_pressure,
    dog_approach,
    dog_facing,
    avoidance,
    applied,
};

inline constexpr std::size_t kInfluenceChannelCount = 8;

[[nodiscard]] constexpr std::size_t influence_channel_index(InfluenceChannel channel) noexcept {
    return static_cast<std::size_t>(channel);
}

[[nodiscard]] std::string_view influence_channel_name(InfluenceChannel channel) noexcept;

// The lane palette, exposed so that the renderer's framebuffer oracle counts the
// colours the frame builder actually emits rather than a second copy of them.
[[nodiscard]] std::array<float, 3> influence_channel_color(InfluenceChannel channel) noexcept;

// What a segment is for. The role is the machine-readable half of the legend:
// a headless oracle asserts counts and geometry per role without re-deriving the
// drawing rules, and a frame dump stays greppable.
enum class DebugSegmentRole : std::uint8_t {
    // Per sheep.
    mast,             // vertical spine carrying the lane ticks
    lane_tick,        // one per channel, countable from the bottom
    influence_shaft,  // the arrow body; length encodes magnitude (see below)
    influence_head,   // one of the two barbs at the arrow tip
    attraction_link,  // subject -> a published chosen attraction neighbour
    alignment_link,   // subject -> the published chosen alignment neighbour
    arousal_scale,    // a scenario-owned arousal threshold, or full scale
    arousal_bar,      // the sheep's own arousal, as a fraction of full scale
    behavior_rung,    // countable rungs: settled 1, alert 2, driven 3, recovering 4
    heading_previous, // dashed: where the sheep faced before this tick's turn
    heading_current,  // short solid tick: where the turn rate actually left it
    heading_target,   // solid ray: the motion heading the turn rate aims at
    // Flock level.
    flock_centroid,   // ground cross at the published centroid
    push_axis,        // dog -> centroid, the axis the pressure term acts along
    balance_point,    // ring at the balance point defined below
    balance_breached, // cross through that ring when the dog is not behind the flock
    rear_member,      // stem over the published rear-most member
};

[[nodiscard]] std::string_view debug_segment_role_name(DebugSegmentRole role) noexcept;

// One world-space line. `subject_id` is the sheep the segment belongs to, or `0`
// for a flock-level marker; `object_id` is the neighbour a link points at, and
// is `0` otherwise. Carrying both IDs is what lets an oracle assert that a drawn
// link matches a published chosen-neighbour ID rather than merely that some line
// exists between two sheep. `channel` is meaningful for the lane, arrow, and
// link roles; every other role carries `separation` as an unread filler.
struct DebugSegment {
    std::array<float, 3> start{};
    std::array<float, 3> end{};
    std::array<float, 3> color{};
    DebugSegmentRole role = DebugSegmentRole::mast;
    InfluenceChannel channel = InfluenceChannel::separation;
    std::uint32_t subject_id = 0;
    std::uint32_t object_id = 0;

    bool operator==(const DebugSegment&) const = default;
};

// ---------------------------------------------------------------------------
// Encoding. Every constant below is a presentation choice owned here; none of
// them is an accepted gameplay value, and changing one cannot change a rule.
// ---------------------------------------------------------------------------

// **The stated arrow scale.** An arrow's length in world units is its
// acceleration magnitude in world units per second squared multiplied by this
// value. It has units of seconds squared, and `0.5 s^2` is exactly one half of
// one second squared, so an arrow is *the distance that term alone would carry a
// sheep from rest in one second*. Under the accepted maxima the largest single
// term is `4.0` units/s^2, which draws a `2.0`-unit arrow — two sheep body
// diameters, and one and a third of the accepted `1.5`-unit separation spacing.
inline constexpr double kInfluenceArrowScaleSecondsSquared = 0.5;

// Longest arrow that will be drawn. `2.5` units is above every accepted per-term
// maximum at the scale above, so a clamped arrow always means the drawn quantity
// exceeded the largest accepted single-term maximum — which the unbounded sum in
// `sheep-combined-influence-off` does, by design. `clamped_arrow_count` reports
// it rather than letting a run-away vector leave the paddock silently.
inline constexpr double kInfluenceArrowMaximumLength = 2.5;

// Below this magnitude a term publishes no arrow, only its lane tick. A term
// that ran and produced nothing and a term that is switched off are therefore
// both tickless-but-arrowless; `SheepDogPressureEvidence::stimulus_evaluated`
// and `SheepAvoidanceEvidence::avoidance_evaluated` are what separate the two,
// and they choose the lane tick's color.
inline constexpr double kInfluenceArrowMinimumLength = 1.0e-6;

inline constexpr double kInfluenceLaneBaseHeight = 1.60;
inline constexpr double kInfluenceLaneSpacing = 0.30;
inline constexpr double kInfluenceLaneTickLength = 0.22;
inline constexpr double kInfluenceArrowHeadAngleRadians = 0.42;
inline constexpr double kInfluenceArrowHeadFraction = 0.28;
inline constexpr double kInfluenceArrowHeadMaximumLength = 0.32;
inline constexpr double kInfluenceAppliedShaftOffset = 0.05;
inline constexpr double kAttractionLinkHeight = 0.12;
inline constexpr double kAlignmentLinkHeight = 0.30;
inline constexpr double kArousalBarOffsetX = 0.85;
inline constexpr double kArousalBarHeight = 2.0;
inline constexpr double kArousalTickLength = 0.20;
inline constexpr double kBehaviorRungBaseHeight = kArousalBarHeight + 0.24;
inline constexpr double kBehaviorRungSpacing = 0.18;
inline constexpr double kBehaviorRungLength = 0.34;
inline constexpr double kHeadingRayLength = 1.70;
inline constexpr double kHeadingRayHeight = 0.06;
inline constexpr double kHeadingDashGap = 0.22;
inline constexpr double kHeadingTargetExtraLength = 0.45;
inline constexpr double kFlockMarkerStemHeight = 2.60;
inline constexpr double kFlockCentroidArmLength = 0.70;
inline constexpr double kBalanceRingRadius = 0.90;
inline constexpr std::size_t kBalanceRingSegmentCount = 8;

// The connectivity distance `compute_five_sheep_observables` needs in order to
// return at all. **No connectivity distance is accepted**, so this view
// deliberately draws nothing derived from it and publishes no component count;
// the value matches the one the scenario oracle uses only so that a reader
// comparing the two is not confused by a third number.
inline constexpr double kInfluenceDebugConnectivityDistance = 5.0;

// Worst-case primitive counts, written as the sum they actually are so that
// adding a marker without raising the ceiling fails to compile rather than
// silently truncating a frame.
inline constexpr std::size_t kMaximumInfluenceDebugSegmentsPerSheep =
    1                                  // mast
    + kInfluenceChannelCount           // one lane tick per channel
    + (kInfluenceChannelCount - 1) * 3 // seven term arrows: shaft plus two barbs
    + 4                                // the applied arrow: doubled shaft plus two barbs
    + game::kMaximumSelectedAttractionNeighbors + game::kMaximumSelectedAlignmentNeighbors +
    5    // four arousal thresholds plus full scale
    + 1  // the arousal bar
    + 4  // at most four behavior rungs
    + 2  // two dashes of the previous heading
    + 1  // the target heading ray
    + 1; // the current heading tick

inline constexpr std::size_t kMaximumFlockDebugSegments = 2   // centroid cross arms
                                                          + 1 // centroid stem
                                                          + 1 // push axis
                                                          + kBalanceRingSegmentCount // balance ring
                                                          + 1                        // balance stem
                                                          + 2  // the breach cross through the ring
                                                          + 1; // rear-member stem

inline constexpr std::size_t kMaximumInfluenceDebugSegments =
    game::kGameplaySheepCount * kMaximumInfluenceDebugSegmentsPerSheep + kMaximumFlockDebugSegments;

// One tick's worth of debug geometry plus the flock-level quantities it was
// derived from, so a capture and a dump can be checked against each other.
//
// **Target** is `SheepMotionLimitEvidence::motion_heading_radians`: the heading
// the accepted turn limit is rotating the sheep toward. It is the only published
// per-sheep goal quantity in the simulation — it is literally the `target`
// argument of `approach_angle` — and it is what explains a sheep that is not
// moving the way its arrows point. There is no objective loop and therefore no
// destination, waypoint, or pen to call a target instead, and inventing one
// would give presentation an authority no rule has.
//
// **Balance point** is `centroid + rear_offset * u`, where `u` is the unit
// vector from the dog toward the flock centroid and `rear_offset` is the
// published along-axis offset of the rear-most member. That point has an exact
// meaning under the accepted pressure rule, which pushes each sheep directly
// away from the dog: a member at along-axis offset `s` is pushed down the axis
// exactly when `centroid_distance + s >= 0`, so every member is driven away from
// the dog exactly when `centroid_distance >= -rear_offset` — that is, when the
// dog is at or beyond the balance point. `dog_behind_flock` records that
// comparison, and the drawn ring carries a cross when it is false.
struct InfluenceDebugFrame {
    std::array<DebugSegment, kMaximumInfluenceDebugSegments> segments{};
    std::size_t segment_count = 0;

    std::uint64_t tick = 0;
    std::uint32_t clamped_arrow_count = 0;
    std::uint32_t arrow_count = 0;
    std::uint32_t attraction_link_count = 0;
    std::uint32_t alignment_link_count = 0;
    std::uint32_t heading_target_count = 0;
    std::uint32_t unresolved_neighbor_count = 0;

    bool flock_markers_present = false;
    bool balance_point_defined = false;
    bool dog_behind_flock = false;
    game::Vec3 centroid{};
    game::Vec3 balance_point{};
    game::FlockDogObservables flock_dog{};

    bool operator==(const InfluenceDebugFrame&) const = default;
};

// Builds the frame for one published tick. `snapshot` supplies every drawn
// value; `scenario` supplies only the read-only arousal thresholds the bar's
// scale marks come from, so those marks cannot drift from the accepted
// transition rule. Allocation-free and deterministic: the same snapshot always
// produces the same frame, byte for byte.
[[nodiscard]] InfluenceDebugFrame
build_influence_debug_frame(const game::GameplaySnapshot& snapshot,
                            const game::GameplayScenarioDefinition& scenario) noexcept;

} // namespace wide_eye::render
