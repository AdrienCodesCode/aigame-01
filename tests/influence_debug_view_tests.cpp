// Headless oracle for the presentation-side influence debug view.
//
// The view's geometry is a pure function of one published snapshot, so
// everything except the upload and the draw can be checked without an OpenGL
// context. That is deliberate: the development host exposes only OpenGL 4.5 and
// cannot create the 4.6 Core context the engine asks for, so this file is the
// part of the debug view that is actually verified here. What it cannot cover —
// whether the drawn frame is legible to a person — is the owner's review.

#include "game/flock_observables.hpp"
#include "game/gameplay_replay.hpp"
#include "game/gameplay_simulation.hpp"
#include "render/influence_debug_view.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <string_view>

std::size_t g_influence_debug_allocation_count = 0;

void* operator new(std::size_t size) {
    ++g_influence_debug_allocation_count;
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

using wide_eye::game::DogMoveInput;
using wide_eye::game::GameplayScenarioDefinition;
using wide_eye::game::GameplaySimulation;
using wide_eye::game::GameplaySnapshot;
using wide_eye::game::GameplayTickInput;
using wide_eye::game::Vec3;
using wide_eye::render::DebugSegment;
using wide_eye::render::DebugSegmentRole;
using wide_eye::render::InfluenceChannel;
using wide_eye::render::InfluenceDebugFrame;
using wide_eye::render::influence_channel_color;
using wide_eye::render::influence_channel_index;
using wide_eye::render::kInfluenceChannelCount;

constexpr std::array<std::string_view, 31> kScenarioNames{{
    "paddock-start",
    "presentation-motion",
    "wall-contact",
    "closed-gate",
    "open-gate",
    "sheep-only-separation",
    "sheep-only-attraction",
    "sheep-alignment-off",
    "sheep-alignment-on",
    "sheep-dog-pressure-off",
    "sheep-dog-pressure-on",
    "sheep-dog-approach-off",
    "sheep-dog-approach-on",
    "sheep-dog-facing-off",
    "sheep-dog-facing-on",
    "sheep-dog-line-of-sight-off",
    "sheep-dog-line-of-sight-on",
    "sheep-paddock-collision-closed-gate",
    "sheep-paddock-collision-open-gate",
    "sheep-temperament-neutral",
    "sheep-temperament-varied",
    "sheep-combined-influence-off",
    "sheep-combined-influence-on",
    "sheep-motion-limit-off",
    "sheep-motion-limit-on",
    "sheep-avoidance-off",
    "sheep-avoidance-on",
    "sheep-behavior-transitions-off",
    "sheep-behavior-transitions-on",
    "sheep-all-influences-diagnostic",
    "fifty-sheep-paddock",
}};

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "influence_debug_failure=" << name << '\n';
    }
    return condition;
}

// The scripted dog route the other per-outcome oracles use, so a frame built
// here is comparable with the state dumps those runs produced.
GameplayTickInput input_for_tick(std::uint64_t tick) {
    if (tick < 40) {
        return {.dog_move = DogMoveInput{.world_z = -1.0}};
    }
    if (tick < 90) {
        return {.dog_move = DogMoveInput{.world_x = 0.75, .world_z = -0.5}};
    }
    if (tick < 150) {
        return {.dog_move = DogMoveInput{.world_x = -1.0, .world_z = 0.5, .sprint = true}};
    }
    return {.dog_move = DogMoveInput{.world_x = 0.25, .world_z = 1.0}};
}

// FNV-1a over the fields rather than over the object's bytes: hashing padding
// would make the digest depend on the ABI instead of on the drawn geometry.
constexpr std::uint64_t kFnvOffsetBasis = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

void mix_bytes(std::uint64_t& digest, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t index = 0; index < size; ++index) {
        digest ^= bytes[index];
        digest *= kFnvPrime;
    }
}

void mix_float(std::uint64_t& digest, float value) {
    mix_bytes(digest, &value, sizeof(value));
}

void mix_frame(std::uint64_t& digest, const InfluenceDebugFrame& frame) {
    mix_bytes(digest, &frame.segment_count, sizeof(frame.segment_count));
    for (std::size_t index = 0; index < frame.segment_count; ++index) {
        const DebugSegment& segment = frame.segments[index];
        for (std::size_t axis = 0; axis < 3; ++axis) {
            mix_float(digest, segment.start[axis]);
            mix_float(digest, segment.end[axis]);
            mix_float(digest, segment.color[axis]);
        }
        const auto role = static_cast<std::uint8_t>(segment.role);
        const auto channel = static_cast<std::uint8_t>(segment.channel);
        mix_bytes(digest, &role, sizeof(role));
        mix_bytes(digest, &channel, sizeof(channel));
        mix_bytes(digest, &segment.subject_id, sizeof(segment.subject_id));
        mix_bytes(digest, &segment.object_id, sizeof(segment.object_id));
    }
}

[[nodiscard]] double segment_length(const DebugSegment& segment) {
    const double dx = static_cast<double>(segment.end[0]) - static_cast<double>(segment.start[0]);
    const double dy = static_cast<double>(segment.end[1]) - static_cast<double>(segment.start[1]);
    const double dz = static_cast<double>(segment.end[2]) - static_cast<double>(segment.start[2]);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

[[nodiscard]] bool near_equal(double left, double right, double tolerance) {
    return std::abs(left - right) <= tolerance;
}

// Float coordinates come from double world positions, so every geometric
// comparison here uses a tolerance that is generous against float rounding at
// paddock scale and still far below anything a person could see.
constexpr double kGeometryTolerance = 1.0e-4;

struct ChannelSource {
    Vec3 acceleration;
    bool evaluated;
};

[[nodiscard]] std::array<ChannelSource, kInfluenceChannelCount>
channel_sources(const GameplaySnapshot& snapshot, std::size_t index) {
    const auto& social = snapshot.sheep_social_evidence[index];
    const auto& dog = snapshot.sheep_dog_pressure_evidence[index];
    const auto& avoidance = snapshot.sheep_avoidance_evidence[index];
    const auto& combined = snapshot.sheep_combined_influence_evidence[index];
    return {{
        {social.separation_acceleration, true},
        {social.attraction_acceleration, true},
        {social.alignment_acceleration, true},
        {dog.pressure_acceleration, dog.stimulus_evaluated},
        {dog.approach_acceleration, dog.stimulus_evaluated},
        {dog.facing_acceleration, dog.stimulus_evaluated},
        {avoidance.avoidance_acceleration, avoidance.avoidance_evaluated},
        {combined.applied_acceleration, combined.bound_evaluated},
    }};
}

} // namespace

int main() {
    // ---------------------------------------------------------------- sweep
    // Every named scenario, every tick: the invariants that must hold for any
    // published state at all, plus the aggregate digest that makes an
    // unintended change to the drawn geometry visible.
    std::uint64_t sweep_digest = kFnvOffsetBasis;
    std::size_t worst_segment_count = 0;
    std::size_t total_arrows = 0;
    std::size_t total_attraction_links = 0;
    std::size_t total_alignment_links = 0;
    std::size_t total_heading_targets = 0;
    std::size_t total_unresolved_neighbors = 0;
    std::size_t scenarios_with_clamped_arrows = 0;
    bool every_influence_is_planar = true;
    bool every_frame_within_capacity = true;
    bool every_lane_has_exactly_one_tick = true;
    bool every_arrow_matches_published_vector = true;
    bool every_applied_stroke_carries_a_bright_casing = true;
    bool the_applied_lane_color_is_still_near_black = true;
    bool every_link_matches_published_id = true;
    bool every_arousal_bar_matches_published_arousal = true;
    bool every_behavior_rung_count_matches_label = true;
    bool every_target_matches_published_motion_heading = true;
    bool every_balance_point_matches_published_observables = true;

    // The casing is only half of a pair, and the pair only works because the
    // other half is at the dark end of the ramp. Quietly brightening the core
    // instead would leave both strokes light and lose the grass case.
    {
        const std::array<float, 3> applied_color =
            influence_channel_color(InfluenceChannel::applied);
        the_applied_lane_color_is_still_near_black =
            applied_color[0] <= 0.10F && applied_color[1] <= 0.10F && applied_color[2] <= 0.10F;
    }

    // One caller-owned frame, refilled. At the published capacity the frame is
    // over half a megabyte, so a per-tick stack copy would be a stack overflow
    // rather than a fixture.
    const auto sweep_frame = std::make_unique<InfluenceDebugFrame>();
    for (const std::string_view name : kScenarioNames) {
        const auto scenario = wide_eye::game::find_gameplay_scenario(name);
        if (!check(scenario.has_value(), "scenario_available")) {
            return EXIT_FAILURE;
        }
        const auto simulation = std::make_unique<GameplaySimulation>(*scenario);
        std::uint32_t clamped_in_scenario = 0;
        for (std::uint64_t tick = 0; tick < 240; ++tick) {
            simulation->fixed_update(input_for_tick(tick));
            const GameplaySnapshot& snapshot = simulation->current_snapshot();
            wide_eye::render::build_influence_debug_frame(snapshot, *scenario, *sweep_frame);
            const InfluenceDebugFrame& frame = *sweep_frame;

            mix_frame(sweep_digest, frame);
            worst_segment_count = std::max(worst_segment_count, frame.segment_count);
            total_arrows += frame.arrow_count;
            total_attraction_links += frame.attraction_link_count;
            total_alignment_links += frame.alignment_link_count;
            total_heading_targets += frame.heading_target_count;
            total_unresolved_neighbors += frame.unresolved_neighbor_count;
            clamped_in_scenario += frame.clamped_arrow_count;
            every_frame_within_capacity =
                every_frame_within_capacity && frame.segment_count <= frame.segments.size();

            for (std::size_t index = 0; index < snapshot.sheep_count; ++index) {
                const auto& sheep = snapshot.sheep[index];
                const auto sources = channel_sources(snapshot, index);

                // Planarity. The view draws arrows in the ground plane and drops
                // any vertical component; this equality is what makes that
                // lossless rather than a silent approximation.
                for (const ChannelSource& source : sources) {
                    every_influence_is_planar =
                        every_influence_is_planar && source.acceleration.y == 0.0;
                }

                // Lane ticks and arrows, per channel, against the published
                // vector that produced them.
                for (std::size_t lane = 0; lane < kInfluenceChannelCount; ++lane) {
                    const auto channel = static_cast<InfluenceChannel>(lane);
                    std::size_t tick_count = 0;
                    std::size_t shaft_count = 0;
                    double drawn_length = 0.0;
                    double drawn_direction_x = 0.0;
                    double drawn_direction_z = 0.0;
                    for (std::size_t s = 0; s < frame.segment_count; ++s) {
                        const DebugSegment& segment = frame.segments[s];
                        if (segment.subject_id != sheep.id || segment.channel != channel) {
                            continue;
                        }
                        if (segment.role == DebugSegmentRole::lane_tick) {
                            ++tick_count;
                            const double expected_height =
                                sheep.position.y + wide_eye::render::kInfluenceLaneBaseHeight +
                                static_cast<double>(lane) * wide_eye::render::kInfluenceLaneSpacing;
                            every_lane_has_exactly_one_tick =
                                every_lane_has_exactly_one_tick &&
                                near_equal(static_cast<double>(segment.start[1]), expected_height,
                                           kGeometryTolerance);
                        } else if (segment.role == DebugSegmentRole::influence_shaft) {
                            ++shaft_count;
                            drawn_length = segment_length(segment);
                            drawn_direction_x = static_cast<double>(segment.end[0]) -
                                                static_cast<double>(segment.start[0]);
                            drawn_direction_z = static_cast<double>(segment.end[2]) -
                                                static_cast<double>(segment.start[2]);
                        }
                    }
                    every_lane_has_exactly_one_tick =
                        every_lane_has_exactly_one_tick && tick_count == 1;

                    const Vec3& acceleration = sources[lane].acceleration;
                    const double magnitude = std::hypot(acceleration.x, acceleration.z);
                    const double expected_length =
                        std::min(magnitude * wide_eye::render::kInfluenceArrowScaleSecondsSquared,
                                 wide_eye::render::kInfluenceArrowMaximumLength);
                    const bool expect_arrow =
                        magnitude * wide_eye::render::kInfluenceArrowScaleSecondsSquared >=
                        wide_eye::render::kInfluenceArrowMinimumLength;
                    const std::size_t expected_shafts =
                        expect_arrow ? (channel == InfluenceChannel::applied ? 2U : 1U) : 0U;
                    bool lane_matches = shaft_count == expected_shafts;
                    if (expect_arrow && lane_matches) {
                        lane_matches =
                            near_equal(drawn_length, expected_length, kGeometryTolerance) &&
                            near_equal(drawn_direction_x,
                                       acceleration.x / magnitude * expected_length,
                                       kGeometryTolerance) &&
                            near_equal(drawn_direction_z,
                                       acceleration.z / magnitude * expected_length,
                                       kGeometryTolerance);
                    }
                    every_arrow_matches_published_vector =
                        every_arrow_matches_published_vector && lane_matches;
                }

                // The `applied` casing. `applied` is the only steering lane
                // drawn without a hue, so luminance is its only separation
                // channel, and near-black alone loses it against the paddock
                // wall, the gate, the sheep's own legs, and the mast it hangs
                // off. Every near-black `applied` stroke therefore carries a
                // bright second stroke. Nothing else in the suite would notice
                // if that came back out: the framebuffer oracle passes on lane
                // pixel counts and the sweep digest is printed rather than
                // compared. This is the assertion that would.
                {
                    const ChannelSource& applied =
                        sources[influence_channel_index(InfluenceChannel::applied)];
                    const double applied_magnitude =
                        std::hypot(applied.acceleration.x, applied.acceleration.z);
                    const bool applied_arrow_drawn =
                        applied_magnitude * wide_eye::render::kInfluenceArrowScaleSecondsSquared >=
                        wide_eye::render::kInfluenceArrowMinimumLength;
                    // One for the lane tick when the bound published a result,
                    // plus a shaft and two barbs when an arrow was drawn.
                    const std::size_t expected_casings =
                        (applied.evaluated ? 1U : 0U) + (applied_arrow_drawn ? 3U : 0U);
                    std::size_t casing_count = 0;
                    bool every_casing_is_bright = true;
                    for (std::size_t s = 0; s < frame.segment_count; ++s) {
                        const DebugSegment& segment = frame.segments[s];
                        if (segment.subject_id != sheep.id ||
                            segment.role != DebugSegmentRole::applied_casing) {
                            continue;
                        }
                        ++casing_count;
                        every_casing_is_bright =
                            every_casing_is_bright &&
                            segment.channel == InfluenceChannel::applied &&
                            segment.color[0] >= 0.70F && segment.color[1] >= 0.70F &&
                            segment.color[2] >= 0.70F;
                    }
                    every_applied_stroke_carries_a_bright_casing =
                        every_applied_stroke_carries_a_bright_casing &&
                        casing_count == expected_casings && every_casing_is_bright;
                }

                // Chosen neighbours: each drawn link names a published ID and
                // ends on that sheep's published position.
                const auto& social = snapshot.sheep_social_evidence[index];
                for (std::size_t s = 0; s < frame.segment_count; ++s) {
                    const DebugSegment& segment = frame.segments[s];
                    if (segment.subject_id != sheep.id) {
                        continue;
                    }
                    const bool is_attraction = segment.role == DebugSegmentRole::attraction_link;
                    const bool is_alignment = segment.role == DebugSegmentRole::alignment_link;
                    if (!is_attraction && !is_alignment) {
                        continue;
                    }
                    bool published = false;
                    if (is_attraction) {
                        for (std::uint32_t slot = 0; slot < social.attraction_neighbor_count;
                             ++slot) {
                            published = published ||
                                        social.attraction_neighbor_ids[slot] == segment.object_id;
                        }
                    } else {
                        for (std::uint32_t slot = 0; slot < social.alignment_neighbor_count;
                             ++slot) {
                            published = published ||
                                        social.alignment_neighbor_ids[slot] == segment.object_id;
                        }
                    }
                    bool endpoints_match = false;
                    for (const auto& candidate :
                         std::span{snapshot.sheep.data(), snapshot.sheep_count}) {
                        if (candidate.id != segment.object_id) {
                            continue;
                        }
                        endpoints_match = near_equal(static_cast<double>(segment.start[0]),
                                                     sheep.position.x, kGeometryTolerance) &&
                                          near_equal(static_cast<double>(segment.start[2]),
                                                     sheep.position.z, kGeometryTolerance) &&
                                          near_equal(static_cast<double>(segment.end[0]),
                                                     candidate.position.x, kGeometryTolerance) &&
                                          near_equal(static_cast<double>(segment.end[2]),
                                                     candidate.position.z, kGeometryTolerance);
                    }
                    every_link_matches_published_id =
                        every_link_matches_published_id && published && endpoints_match;
                }

                // Arousal bar height, behavior rung count, and the target ray.
                std::size_t rung_count = 0;
                std::size_t target_count = 0;
                for (std::size_t s = 0; s < frame.segment_count; ++s) {
                    const DebugSegment& segment = frame.segments[s];
                    if (segment.subject_id != sheep.id) {
                        continue;
                    }
                    if (segment.role == DebugSegmentRole::arousal_bar) {
                        every_arousal_bar_matches_published_arousal =
                            every_arousal_bar_matches_published_arousal &&
                            near_equal(segment_length(segment),
                                       sheep.arousal * wide_eye::render::kArousalBarHeight,
                                       kGeometryTolerance);
                    } else if (segment.role == DebugSegmentRole::behavior_rung) {
                        ++rung_count;
                    } else if (segment.role == DebugSegmentRole::heading_target) {
                        ++target_count;
                        const auto& motion = snapshot.sheep_motion_limit_evidence[index];
                        const double dx = static_cast<double>(segment.end[0]) -
                                          static_cast<double>(segment.start[0]);
                        const double dz = static_cast<double>(segment.end[2]) -
                                          static_cast<double>(segment.start[2]);
                        every_target_matches_published_motion_heading =
                            every_target_matches_published_motion_heading &&
                            near_equal(std::atan2(dx, -dz), motion.motion_heading_radians, 1.0e-4);
                    }
                }
                every_behavior_rung_count_matches_label =
                    every_behavior_rung_count_matches_label &&
                    rung_count == static_cast<std::size_t>(sheep.behavior) + 1U;
                const auto& motion = snapshot.sheep_motion_limit_evidence[index];
                const std::size_t expected_targets =
                    motion.limit_evaluated && motion.motion_heading_followed ? 1U : 0U;
                every_target_matches_published_motion_heading =
                    every_target_matches_published_motion_heading &&
                    target_count == expected_targets;
            }

            // Balance point, recomputed from the published observables the same
            // way the header defines it.
            if (frame.balance_point_defined) {
                const double bearing = frame.flock_dog.centroid_bearing_radians;
                const double push_x = -std::sin(bearing);
                const double push_z = std::cos(bearing);
                const double expected_x = frame.centroid.x + push_x * frame.flock_dog.rear_offset;
                const double expected_z = frame.centroid.z + push_z * frame.flock_dog.rear_offset;
                const bool behind =
                    frame.flock_dog.centroid_distance >= -frame.flock_dog.rear_offset;
                std::size_t ring_segments = 0;
                std::size_t breach_segments = 0;
                for (std::size_t s = 0; s < frame.segment_count; ++s) {
                    if (frame.segments[s].role == DebugSegmentRole::balance_point) {
                        ++ring_segments;
                    } else if (frame.segments[s].role == DebugSegmentRole::balance_breached) {
                        ++breach_segments;
                    }
                }
                every_balance_point_matches_published_observables =
                    every_balance_point_matches_published_observables &&
                    near_equal(frame.balance_point.x, expected_x, kGeometryTolerance) &&
                    near_equal(frame.balance_point.z, expected_z, kGeometryTolerance) &&
                    frame.dog_behind_flock == behind &&
                    ring_segments ==
                        wide_eye::render::kBalanceRingSegmentCount + 1U && // ring plus stem
                    breach_segments == (behind ? 0U : 2U);
            }
        }
        if (clamped_in_scenario > 0) {
            ++scenarios_with_clamped_arrows;
        }
    }

    if (!check(every_influence_is_planar, "every_published_influence_is_exactly_planar") ||
        !check(every_frame_within_capacity, "every_frame_within_declared_capacity") ||
        !check(worst_segment_count <= wide_eye::render::kMaximumInfluenceDebugSegments,
               "worst_frame_within_declared_capacity") ||
        !check(every_lane_has_exactly_one_tick, "every_lane_has_exactly_one_tick") ||
        !check(every_arrow_matches_published_vector, "every_arrow_matches_published_vector") ||
        !check(every_applied_stroke_carries_a_bright_casing,
               "every_applied_stroke_carries_a_bright_casing") ||
        !check(the_applied_lane_color_is_still_near_black,
               "the_applied_lane_color_is_still_near_black") ||
        !check(every_link_matches_published_id, "every_link_matches_published_neighbor_id") ||
        !check(total_unresolved_neighbors == 0, "every_published_neighbor_id_resolves") ||
        !check(every_arousal_bar_matches_published_arousal,
               "every_arousal_bar_matches_published_arousal") ||
        !check(every_behavior_rung_count_matches_label,
               "every_behavior_rung_count_matches_label") ||
        !check(every_target_matches_published_motion_heading,
               "every_target_matches_published_motion_heading") ||
        !check(every_balance_point_matches_published_observables,
               "every_balance_point_matches_published_observables")) {
        return EXIT_FAILURE;
    }

    // --------------------------------------------------- paired clamp evidence
    // The bound is what keeps a sum inside the largest accepted single-term
    // maximum, so its paired control is the one place an arrow legitimately runs
    // past the drawn ceiling. Pinning both halves keeps the clamp indicator
    // meaningful instead of decorative.
    std::uint32_t bounded_clamped = 0;
    std::uint32_t unbounded_clamped = 0;
    for (const std::string_view name : {std::string_view{"sheep-combined-influence-on"},
                                        std::string_view{"sheep-combined-influence-off"}}) {
        const auto scenario = wide_eye::game::find_gameplay_scenario(name);
        const auto simulation = std::make_unique<GameplaySimulation>(*scenario);
        std::uint32_t clamped = 0;
        for (std::uint64_t tick = 0; tick < 240; ++tick) {
            simulation->fixed_update(input_for_tick(tick));
            wide_eye::render::build_influence_debug_frame(simulation->current_snapshot(),
                                                          *scenario, *sweep_frame);
            clamped += sweep_frame->clamped_arrow_count;
        }
        if (name == "sheep-combined-influence-on") {
            bounded_clamped = clamped;
        } else {
            unbounded_clamped = clamped;
        }
    }
    if (!check(bounded_clamped == 0, "bounded_scenario_draws_no_clamped_arrow") ||
        !check(unbounded_clamped > 0, "unbounded_control_draws_a_clamped_arrow")) {
        return EXIT_FAILURE;
    }

    // ----------------------------------------------------------- determinism
    // The same tick must produce the same frame: twice from one simulation, and
    // again from a restarted one advanced over the same route.
    const auto diagnostic =
        wide_eye::game::find_gameplay_scenario("sheep-all-influences-diagnostic");
    if (!check(diagnostic.has_value(), "diagnostic_scenario_available")) {
        return EXIT_FAILURE;
    }
    const auto deterministic = std::make_unique<GameplaySimulation>(*diagnostic);
    for (std::uint64_t tick = 0; tick < 120; ++tick) {
        deterministic->fixed_update(input_for_tick(tick));
    }
    const auto first = std::make_unique<InfluenceDebugFrame>();
    wide_eye::render::build_influence_debug_frame(deterministic->current_snapshot(),
                                                  *diagnostic, *first);
    const auto second = std::make_unique<InfluenceDebugFrame>();
    wide_eye::render::build_influence_debug_frame(deterministic->current_snapshot(),
                                                  *diagnostic, *second);
    deterministic->restart();
    for (std::uint64_t tick = 0; tick < 120; ++tick) {
        deterministic->fixed_update(input_for_tick(tick));
    }
    const auto after_restart = std::make_unique<InfluenceDebugFrame>();
    wide_eye::render::build_influence_debug_frame(deterministic->current_snapshot(),
                                                  *diagnostic, *after_restart);
    if (!check(*first == *second, "repeated_build_is_identical") ||
        !check(*first == *after_restart, "restarted_run_rebuilds_an_identical_frame") ||
        !check(first->segment_count > 0, "diagnostic_frame_is_not_empty")) {
        return EXIT_FAILURE;
    }

    // --------------------------------------------- presentation does not steer
    // Two runs of the same scenario over the same route, one of which builds a
    // debug frame on every tick, must publish the same canonical state dump.
    bool observation_is_inert = true;
    for (const std::string_view name : kScenarioNames) {
        const auto scenario = wide_eye::game::find_gameplay_scenario(name);
        const auto observed = std::make_unique<GameplaySimulation>(*scenario);
        const auto untouched = std::make_unique<GameplaySimulation>(*scenario);
        for (std::uint64_t tick = 0; tick < 240; ++tick) {
            observed->fixed_update(input_for_tick(tick));
            wide_eye::render::build_influence_debug_frame(observed->current_snapshot(),
                                                          *scenario, *sweep_frame);
            untouched->fixed_update(input_for_tick(tick));
            observation_is_inert = observation_is_inert &&
                                   observed->current_snapshot() == untouched->current_snapshot();
        }
        const auto observed_dump = wide_eye::game::gameplay_state_dump_json(*observed);
        const auto untouched_dump = wide_eye::game::gameplay_state_dump_json(*untouched);
        observation_is_inert = observation_is_inert && observed_dump && untouched_dump &&
                               observed_dump.text == untouched_dump.text;
    }
    if (!check(observation_is_inert, "building_a_debug_frame_changes_no_published_state")) {
        return EXIT_FAILURE;
    }

    // ----------------------------------------------------------- allocation
    // The frame storage is acquired before the counter is read, so the loop
    // measures the builder rather than one allocation of its output.
    const auto warm_frame = std::make_unique<InfluenceDebugFrame>();
    const std::size_t allocations_before = g_influence_debug_allocation_count;
    std::uint64_t warm_digest = kFnvOffsetBasis;
    for (int repeat = 0; repeat < 600; ++repeat) {
        wide_eye::render::build_influence_debug_frame(deterministic->current_snapshot(),
                                                      *diagnostic, *warm_frame);
        mix_frame(warm_digest, *warm_frame);
    }
    const std::size_t build_allocations = g_influence_debug_allocation_count - allocations_before;
    if (!check(build_allocations == 0, "building_a_debug_frame_does_not_allocate")) {
        return EXIT_FAILURE;
    }

    std::cout << "influence_debug_scenarios=" << kScenarioNames.size() << '\n'
              << "influence_debug_ticks_per_scenario=240\n"
              << "influence_debug_channels=" << kInfluenceChannelCount << '\n'
              << "influence_debug_segment_capacity="
              << wide_eye::render::kMaximumInfluenceDebugSegments << '\n'
              << "influence_debug_worst_segment_count=" << worst_segment_count << '\n'
              << "influence_debug_total_arrows=" << total_arrows << '\n'
              << "influence_debug_total_attraction_links=" << total_attraction_links << '\n'
              << "influence_debug_total_alignment_links=" << total_alignment_links << '\n'
              << "influence_debug_total_heading_targets=" << total_heading_targets << '\n'
              << "influence_debug_unresolved_neighbor_ids=" << total_unresolved_neighbors << '\n'
              << "influence_debug_scenarios_with_clamped_arrows=" << scenarios_with_clamped_arrows
              << '\n'
              << "influence_debug_bounded_clamped_arrows=" << bounded_clamped << '\n'
              << "influence_debug_unbounded_clamped_arrows=" << unbounded_clamped << '\n'
              << "influence_debug_arrow_scale_seconds_squared="
              << wide_eye::render::kInfluenceArrowScaleSecondsSquared << '\n'
              << "influence_debug_arrow_maximum_length="
              << wide_eye::render::kInfluenceArrowMaximumLength << '\n'
              << "influence_debug_diagnostic_segment_count=" << first->segment_count << '\n'
              << "influence_debug_build_allocations=" << build_allocations << '\n'
              << "influence_debug_sweep_digest=" << sweep_digest << '\n'
              << "influence_debug_view_result=pass\n";
    return EXIT_SUCCESS;
}
