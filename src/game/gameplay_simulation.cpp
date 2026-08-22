#include "game/gameplay_simulation.hpp"

#include "game/sheep_rules.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

namespace wide_eye::game {
namespace {

[[nodiscard]] SheepSocialEvidenceBuffer
empty_social_evidence(const SheepStateBuffer& sheep, std::size_t count) noexcept {
    SheepSocialEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < count; ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepDogPressureEvidenceBuffer
empty_dog_pressure_evidence(const SheepStateBuffer& sheep, std::size_t count) noexcept {
    SheepDogPressureEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < count; ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepCollisionEvidenceBuffer
empty_collision_evidence(const SheepStateBuffer& sheep, std::size_t count) noexcept {
    SheepCollisionEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < count; ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepAvoidanceEvidenceBuffer
empty_avoidance_evidence(const SheepStateBuffer& sheep, std::size_t count) noexcept {
    SheepAvoidanceEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < count; ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepCombinedInfluenceEvidenceBuffer
empty_combined_influence_evidence(const SheepStateBuffer& sheep, std::size_t count) noexcept {
    SheepCombinedInfluenceEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < count; ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepMotionLimitEvidenceBuffer
empty_motion_limit_evidence(const SheepStateBuffer& sheep, std::size_t count) noexcept {
    SheepMotionLimitEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < count; ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

// Scenario configuration is immutable after construction, so enabled-term
// bounds are validated once when the simulation is created rather than on
// every fixed tick.
void validate_social_response_configuration(const GameplayScenarioDefinition& scenario) noexcept {
    const SheepSeparationConfiguration& separation = scenario.sheep_separation;
    const SheepAttractionConfiguration& attraction = scenario.sheep_attraction;
    const SheepAlignmentConfiguration& alignment = scenario.sheep_alignment;
    const SheepDogPressureConfiguration& dog_pressure = scenario.sheep_dog_pressure;
    const SheepDogApproachConfiguration& dog_approach = scenario.sheep_dog_approach;
    const SheepDogFacingConfiguration& dog_facing = scenario.sheep_dog_facing;
    const SheepTemperamentConfiguration& temperament = scenario.sheep_temperament;
    const SheepAvoidanceConfiguration& avoidance = scenario.sheep_avoidance;
    const SheepCombinedInfluenceConfiguration& combined = scenario.sheep_combined_influence;
    const SheepMotionLimitConfiguration& motion_limit = scenario.sheep_motion_limit;
    const SheepBehaviorConfiguration& behavior = scenario.sheep_behavior;
    if (separation.enabled) {
        WIDE_EYE_ASSERT(std::isfinite(separation.radius) && separation.radius > 0.0,
                        "sheep separation radius must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(separation.maximum_acceleration) &&
                            separation.maximum_acceleration >= 0.0,
                        "sheep separation acceleration must be finite and non-negative");
    }
    if (attraction.enabled) {
        WIDE_EYE_ASSERT(std::isfinite(attraction.radius) && attraction.radius > 0.0,
                        "sheep attraction radius must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(attraction.maximum_acceleration) &&
                            attraction.maximum_acceleration >= 0.0,
                        "sheep attraction acceleration must be finite and non-negative");
        WIDE_EYE_ASSERT(attraction.neighbor_limit > 0 &&
                            attraction.neighbor_limit <= kMaximumSelectedAttractionNeighbors,
                        "sheep attraction neighbor limit must fit published evidence");
    }
    if (alignment.enabled) {
        WIDE_EYE_ASSERT(std::isfinite(alignment.radius) && alignment.radius > 0.0,
                        "sheep alignment radius must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(alignment.response_time_seconds) &&
                            alignment.response_time_seconds > 0.0,
                        "sheep alignment response time must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(alignment.maximum_acceleration) &&
                            alignment.maximum_acceleration >= 0.0,
                        "sheep alignment acceleration must be finite and non-negative");
        WIDE_EYE_ASSERT(alignment.neighbor_limit > 0 &&
                            alignment.neighbor_limit <= kMaximumSelectedAlignmentNeighbors,
                        "sheep alignment neighbor limit must fit published evidence");
    }
    // Arousal reads the same radius and the same linear falloff the dog terms
    // read, so a scenario that drives arousal needs a usable radius even when
    // every dog term is switched off.
    if (dog_pressure.enabled || dog_approach.enabled || dog_facing.enabled || behavior.enabled) {
        WIDE_EYE_ASSERT(std::isfinite(dog_pressure.radius) && dog_pressure.radius > 0.0,
                        "sheep dog-pressure radius must be finite and positive");
    }
    if (dog_pressure.enabled) {
        WIDE_EYE_ASSERT(std::isfinite(dog_pressure.maximum_acceleration) &&
                            dog_pressure.maximum_acceleration >= 0.0,
                        "sheep dog-pressure acceleration must be finite and non-negative");
    }
    if (dog_approach.enabled) {
        WIDE_EYE_ASSERT(std::isfinite(dog_approach.reference_speed) &&
                            dog_approach.reference_speed > 0.0,
                        "sheep dog-approach reference speed must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(dog_approach.maximum_acceleration) &&
                            dog_approach.maximum_acceleration >= 0.0,
                        "sheep dog-approach acceleration must be finite and non-negative");
    }
    if (dog_facing.enabled) {
        WIDE_EYE_ASSERT(std::isfinite(dog_facing.maximum_acceleration) &&
                            dog_facing.maximum_acceleration >= 0.0,
                        "sheep dog-facing acceleration must be finite and non-negative");
    }
    if (temperament.enabled) {
        // A zero or negative factor would not be a temperament: it would silence
        // or invert the dog terms, and the published scale could no longer be
        // read as "how much of the ordinary response this sheep produced".
        WIDE_EYE_ASSERT(std::isfinite(temperament.nervous_response_scale) &&
                            temperament.nervous_response_scale > 0.0,
                        "sheep nervous response scale must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(temperament.stubborn_response_scale) &&
                            temperament.stubborn_response_scale > 0.0,
                        "sheep stubborn response scale must be finite and positive");
    }
    if (avoidance.enabled) {
        // A zero or negative look-ahead is not a shorter look-ahead: the linear
        // falloff divides by it, and a sheep that looks nowhere cannot avoid
        // anything, which is a way of switching the term off rather than of
        // tuning it.
        WIDE_EYE_ASSERT(std::isfinite(avoidance.look_ahead_distance) &&
                            avoidance.look_ahead_distance > 0.0,
                        "sheep avoidance look-ahead distance must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(avoidance.maximum_acceleration) &&
                            avoidance.maximum_acceleration >= 0.0,
                        "sheep avoidance acceleration must be finite and non-negative");
    }
    if (combined.enabled) {
        // A zero bound would silence every steering term at once, which is a way
        // of disabling the flock rather than of bounding it, and the published
        // scale could no longer be read as "the fraction of the summed terms that
        // was applied".
        WIDE_EYE_ASSERT(std::isfinite(combined.maximum_acceleration) &&
                            combined.maximum_acceleration > 0.0,
                        "sheep combined-influence acceleration must be finite and positive");
    }
    if (motion_limit.enabled) {
        // A zero maximum speed would freeze the flock and a zero turn rate would
        // stop the heading following motion at all, which are ways of switching
        // the rule off rather than of limiting it.
        WIDE_EYE_ASSERT(std::isfinite(motion_limit.maximum_speed) &&
                            motion_limit.maximum_speed > 0.0,
                        "sheep maximum speed must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(motion_limit.maximum_turn_rate_radians_per_second) &&
                            motion_limit.maximum_turn_rate_radians_per_second > 0.0,
                        "sheep maximum turn rate must be finite and positive");
    }
    if (behavior.enabled) {
        // A zero or negative rate is not a slower response: arousal would stop
        // following its cause in one direction, which is a way of switching the
        // proxy off rather than of tuning it.
        WIDE_EYE_ASSERT(std::isfinite(behavior.rise_rate_per_second) &&
                            behavior.rise_rate_per_second > 0.0,
                        "sheep arousal rise rate must be finite and positive");
        WIDE_EYE_ASSERT(std::isfinite(behavior.recovery_rate_per_second) &&
                            behavior.recovery_rate_per_second > 0.0,
                        "sheep arousal recovery rate must be finite and positive");
        // The ladder has to be a ladder. Each band must be entered above where
        // it is left, or the hysteresis is not hysteresis, and the bands must
        // not cross, or two states would claim the same arousal.
        WIDE_EYE_ASSERT(std::isfinite(behavior.rest_arousal) &&
                            behavior.rest_arousal > kSheepMinimumArousal,
                        "sheep rest arousal must be finite and above the arousal minimum");
        WIDE_EYE_ASSERT(std::isfinite(behavior.alert_arousal) &&
                            behavior.alert_arousal > behavior.rest_arousal,
                        "sheep alert arousal must be finite and above rest arousal");
        WIDE_EYE_ASSERT(
            std::isfinite(behavior.driven_release_arousal) &&
                behavior.driven_release_arousal >= behavior.alert_arousal,
            "sheep driven release arousal must be finite and at or above alert arousal");
        WIDE_EYE_ASSERT(std::isfinite(behavior.driven_arousal) &&
                            behavior.driven_arousal > behavior.driven_release_arousal &&
                            behavior.driven_arousal <= kSheepMaximumArousal,
                        "sheep driven arousal must be finite, above its release, and within range");
        // Arousal is bounded by design, so a starting value outside the range is
        // a broken contract rather than a stronger sheep.
        for (std::size_t index = 0; index < scenario.sheep_count; ++index) {
            const SheepState& sheep = scenario.initial_sheep[index];
            WIDE_EYE_ASSERT(std::isfinite(sheep.arousal) && sheep.arousal >= kSheepMinimumArousal &&
                                sheep.arousal <= kSheepMaximumArousal,
                            "initial sheep arousal must be finite and within the arousal range");
        }
    }
}
// One authoritative tick of the flock. The rules themselves live in
// `sheep_rules.hpp`; what belongs here is the order they are applied in and the
// active member count they are applied over. That count is carried from the
// previous snapshot onto the next one, so it is the tick's own data rather than
// a constant, and the one line a later grow-or-shrink outcome replaces.
void advance_sheep_from_prior(const GameplaySnapshot& previous, GameplaySnapshot& next,
                              const GameplayScenarioDefinition& scenario,
                              const PaddockCollisionField& paddock,
                              SheepSpatialGrid& grid) noexcept {
    constexpr std::uint64_t kTicksPerLeg = 60;
    constexpr std::uint64_t kLegCount = 4;
    constexpr double kFixtureSpeed = 1.5;
    constexpr double kHalfPi = 1.57079632679489661923;
    constexpr double kPi = 3.14159265358979323846;
    constexpr std::array<Vec3, kLegCount> kVelocities{{
        {.z = -kFixtureSpeed},
        {.x = kFixtureSpeed},
        {.z = kFixtureSpeed},
        {.x = -kFixtureSpeed},
    }};
    constexpr std::array<double, kLegCount> kHeadings{{0.0, kHalfPi, kPi, -kHalfPi}};

    const SheepStateBuffer& prior = previous.sheep;
    const std::size_t member_count = previous.sheep_count;
    next.sheep_count = member_count;

    // Every fixture keeps the explicit prior-to-next pass so behavior cannot
    // acquire update-order dependence. The named motion fixture is scripted
    // presentation evidence, not a social-response implementation.
    for (std::size_t index = 0; index < member_count; ++index) {
        next.sheep[index] = prior[index];
        next.sheep_social_evidence[index] = {.subject_id = prior[index].id};
        next.sheep_dog_pressure_evidence[index] = {.subject_id = prior[index].id};
        next.sheep_collision_evidence[index] = {.subject_id = prior[index].id};
        next.sheep_avoidance_evidence[index] = {.subject_id = prior[index].id};
        next.sheep_combined_influence_evidence[index] = {.subject_id = prior[index].id};
        next.sheep_motion_limit_evidence[index] = {.subject_id = prior[index].id};
        if (scenario.sheep_fixture != SheepFixture::scripted_presentation_motion) {
            // The stationary fixture chooses no displacement, so the paddock has
            // nothing to resolve and no contact to publish. The social fixture
            // chooses one in the authoritative pass below and resolves it there.
            continue;
        }

        const std::size_t leg =
            static_cast<std::size_t>((previous.tick / kTicksPerLeg) % kLegCount);
        next.sheep[index].velocity = kVelocities[leg];
        next.sheep[index].heading_radians = kHeadings[leg];
        resolve_sheep_against_paddock(
            prior[index],
            {.x = kVelocities[leg].x * GameplaySimulation::kFixedDeltaSeconds,
             .z = kVelocities[leg].z * GameplaySimulation::kFixedDeltaSeconds},
            paddock, next.sheep[index], next.sheep_collision_evidence[index]);
    }

    if (scenario.sheep_fixture != SheepFixture::local_social_response) {
        return;
    }

    const SheepSeparationConfiguration& separation = scenario.sheep_separation;
    const SheepAttractionConfiguration& attraction = scenario.sheep_attraction;
    const SheepAlignmentConfiguration& alignment = scenario.sheep_alignment;
    const SheepAvoidanceConfiguration& avoidance = scenario.sheep_avoidance;
    const SheepCombinedInfluenceConfiguration& combined = scenario.sheep_combined_influence;
    const SheepMotionLimitConfiguration& motion_limit = scenario.sheep_motion_limit;
    const SheepBehaviorConfiguration& behavior = scenario.sheep_behavior;

    const double grid_cell_size = sheep_social_grid_cell_size(scenario);
    if (grid_cell_size > 0.0) {
        // The rebuild call stays outside the assertion so a future
        // release-disabled assert cannot silently remove it.
        const SpatialGridBuildError build_error =
            grid.rebuild(std::span{prior.data(), member_count}, grid_cell_size);
        WIDE_EYE_ASSERT(build_error == SpatialGridBuildError::none,
                        "valid sheep snapshot must rebuild the social-response grid");
        static_cast<void>(build_error);
    }

    std::array<SpatialNeighbor, kMaximumGameplaySheepCount - 1> separation_scratch{};
    std::array<SpatialNeighbor, kMaximumSelectedAttractionNeighbors> attraction_scratch{};
    std::array<SpatialNeighbor, kMaximumSelectedAlignmentNeighbors> alignment_scratch{};
    for (std::size_t index = 0; index < member_count; ++index) {
        SheepSocialEvidence& evidence = next.sheep_social_evidence[index];
        SheepDogPressureEvidence& dog_evidence = next.sheep_dog_pressure_evidence[index];
        SheepAvoidanceEvidence& avoidance_evidence = next.sheep_avoidance_evidence[index];
        SheepCombinedInfluenceEvidence& combined_evidence =
            next.sheep_combined_influence_evidence[index];

        evaluate_sheep_dog_stimulus(prior[index], previous.dog, scenario, paddock, dog_evidence);
        // Behavior runs immediately after the cause that drives it and before
        // any steering term, because it reads only prior state and contributes
        // nothing to the sum. Its position here is legibility, not ordering: no
        // term below reads the arousal or the label it writes.
        apply_sheep_behavior_transition(prior[index], behavior, dog_evidence.arousal_stimulus,
                                        next.sheep[index]);
        const SheepNeighborSelection selection =
            select_sheep_neighbors(grid, index, separation, attraction, alignment,
                                   separation_scratch, attraction_scratch, alignment_scratch);
        if (separation.enabled) {
            apply_sheep_separation(prior, index, separation, selection.separation,
                                   separation_scratch, evidence);
        }
        if (attraction.enabled) {
            apply_sheep_attraction(prior, index, attraction, selection.attraction,
                                   attraction_scratch, evidence);
        }
        if (alignment.enabled) {
            apply_sheep_alignment(prior, index, alignment, selection.alignment, alignment_scratch,
                                  evidence);
        }
        apply_sheep_avoidance(prior[index], avoidance, paddock, avoidance_evidence);

        apply_sheep_combined_influence(evidence, dog_evidence, avoidance_evidence, combined,
                                       combined_evidence, next.sheep[index]);
        // Speed and turning limit the result of integration, so they run after
        // the combined bound and still before collision: the paddock remains the
        // last authority over where the sheep actually ends up.
        apply_sheep_motion_limits(motion_limit, prior[index], next.sheep[index],
                                  next.sheep_motion_limit_evidence[index]);
        resolve_sheep_against_paddock(
            prior[index],
            {.x = next.sheep[index].velocity.x * GameplaySimulation::kFixedDeltaSeconds,
             .z = next.sheep[index].velocity.z * GameplaySimulation::kFixedDeltaSeconds},
            paddock, next.sheep[index], next.sheep_collision_evidence[index]);
    }
}

} // namespace

SheepState interpolate_sheep_state(const SheepState& previous, const SheepState& current,
                                   double alpha) noexcept {
    constexpr double kTwoPi = 6.28318530717958647692;
    const double bounded_alpha = std::isfinite(alpha) ? std::clamp(alpha, 0.0, 1.0) : 1.0;
    const auto interpolate = [bounded_alpha](double start, double end) {
        return start + (end - start) * bounded_alpha;
    };
    return {
        .id = current.id,
        .position = {.x = interpolate(previous.position.x, current.position.x),
                     .y = interpolate(previous.position.y, current.position.y),
                     .z = interpolate(previous.position.z, current.position.z)},
        .velocity = {.x = interpolate(previous.velocity.x, current.velocity.x),
                     .y = interpolate(previous.velocity.y, current.velocity.y),
                     .z = interpolate(previous.velocity.z, current.velocity.z)},
        .heading_radians = std::remainder(
            previous.heading_radians +
                std::remainder(current.heading_radians - previous.heading_radians, kTwoPi) *
                    bounded_alpha,
            kTwoPi),
        .arousal = interpolate(previous.arousal, current.arousal),
        .behavior = current.behavior,
        // Temperament is a fixed label rather than a continuous quantity, so
        // presentation reads the current one exactly as it reads behavior.
        .temperament = current.temperament,
        .grounded = current.grounded,
    };
}

GameplaySimulation::GameplaySimulation(GameplayScenarioDefinition scenario) noexcept
    : scenario_{scenario}, dog_{scenario.dog, scenario.gate_open}, paddock_{scenario.gate_open} {
    // The flock size is scenario data, and a scenario that asks for none or for
    // more members than a published buffer can hold is a broken contract rather
    // than a smaller or larger flock.
    WIDE_EYE_ASSERT(scenario_.sheep_count > 0 &&
                        scenario_.sheep_count <= kMaximumGameplaySheepCount,
                    "scenario sheep count must fit the published sheep buffers");
    if (scenario_.sheep_fixture == SheepFixture::local_social_response) {
        validate_social_response_configuration(scenario_);
    }
    current_.dog = dog_.state();
    current_.sheep_count = scenario_.sheep_count;
    current_.sheep = scenario_.initial_sheep;
    const std::size_t count = current_.sheep_count;
    current_.sheep_social_evidence = empty_social_evidence(current_.sheep, count);
    current_.sheep_dog_pressure_evidence = empty_dog_pressure_evidence(current_.sheep, count);
    current_.sheep_collision_evidence = empty_collision_evidence(current_.sheep, count);
    current_.sheep_avoidance_evidence = empty_avoidance_evidence(current_.sheep, count);
    current_.sheep_combined_influence_evidence =
        empty_combined_influence_evidence(current_.sheep, count);
    current_.sheep_motion_limit_evidence = empty_motion_limit_evidence(current_.sheep, count);
    previous_ = current_;
}

void GameplaySimulation::fixed_update(const GameplayTickInput& input) noexcept {
    WIDE_EYE_ASSERT(current_.tick < std::numeric_limits<std::uint64_t>::max(),
                    "authoritative gameplay tick overflow");

    previous_ = current_;
    if (input.dog_move.has_value()) {
        dog_.fixed_update(*input.dog_move, kFixedDeltaSeconds);
    }
    current_.tick += 1;
    current_.dog = dog_.state();
    advance_sheep_from_prior(previous_, current_, scenario_, paddock_, sheep_grid_);
}

void GameplaySimulation::restart() noexcept {
    dog_.restart();
    const std::size_t count = scenario_.sheep_count;
    current_ = {.tick = 0,
                .dog = dog_.state(),
                .sheep_count = count,
                .sheep = scenario_.initial_sheep,
                .sheep_social_evidence = empty_social_evidence(scenario_.initial_sheep, count),
                .sheep_dog_pressure_evidence =
                    empty_dog_pressure_evidence(scenario_.initial_sheep, count),
                .sheep_collision_evidence =
                    empty_collision_evidence(scenario_.initial_sheep, count),
                .sheep_avoidance_evidence =
                    empty_avoidance_evidence(scenario_.initial_sheep, count),
                .sheep_combined_influence_evidence =
                    empty_combined_influence_evidence(scenario_.initial_sheep, count),
                .sheep_motion_limit_evidence =
                    empty_motion_limit_evidence(scenario_.initial_sheep, count)};
    previous_ = current_;
}

const GameplaySnapshot& GameplaySimulation::previous_snapshot() const noexcept {
    return previous_;
}

const GameplaySnapshot& GameplaySimulation::current_snapshot() const noexcept {
    return current_;
}

GameplaySnapshot GameplaySimulation::interpolated_snapshot(double alpha) const noexcept {
    GameplaySnapshot result{
        .tick = current_.tick,
        .dog = interpolate_dog_state(previous_.dog, current_.dog, alpha),
        .sheep_count = current_.sheep_count,
        .sheep = current_.sheep,
        .sheep_social_evidence = current_.sheep_social_evidence,
        .sheep_dog_pressure_evidence = current_.sheep_dog_pressure_evidence,
        .sheep_collision_evidence = current_.sheep_collision_evidence,
        .sheep_avoidance_evidence = current_.sheep_avoidance_evidence,
        .sheep_combined_influence_evidence = current_.sheep_combined_influence_evidence,
        .sheep_motion_limit_evidence = current_.sheep_motion_limit_evidence,
    };
    for (std::size_t index = 0; index < result.sheep_count; ++index) {
        result.sheep[index] =
            interpolate_sheep_state(previous_.sheep[index], current_.sheep[index], alpha);
    }
    return result;
}

const GameplayScenarioDefinition& GameplaySimulation::scenario() const noexcept {
    return scenario_;
}

std::uint32_t GameplaySimulation::restart_count() const noexcept {
    return dog_.restart_count();
}

} // namespace wide_eye::game
