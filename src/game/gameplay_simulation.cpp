#include "game/gameplay_simulation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

namespace wide_eye::game {
namespace {

[[nodiscard]] SheepSocialEvidenceBuffer
empty_social_evidence(const SheepStateBuffer& sheep) noexcept {
    SheepSocialEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepDogPressureEvidenceBuffer
empty_dog_pressure_evidence(const SheepStateBuffer& sheep) noexcept {
    SheepDogPressureEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepCollisionEvidenceBuffer
empty_collision_evidence(const SheepStateBuffer& sheep) noexcept {
    SheepCollisionEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepAvoidanceEvidenceBuffer
empty_avoidance_evidence(const SheepStateBuffer& sheep) noexcept {
    SheepAvoidanceEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepCombinedInfluenceEvidenceBuffer
empty_combined_influence_evidence(const SheepStateBuffer& sheep) noexcept {
    SheepCombinedInfluenceEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

[[nodiscard]] SheepMotionLimitEvidenceBuffer
empty_motion_limit_evidence(const SheepStateBuffer& sheep) noexcept {
    SheepMotionLimitEvidenceBuffer evidence{};
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        evidence[index].subject_id = sheep[index].id;
    }
    return evidence;
}

// The one positional authority for a sheep. Every fixture chooses a desired
// planar displacement; the analytic paddock decides where the sheep actually
// ends up, using the same field and the same clipping the dog motor collides
// with. Voxel faces and render meshes never participate, and the steering terms
// that produced `displacement` keep their published vectors: collision is a
// separate, later stage rather than a hidden steering correction.
void resolve_sheep_against_paddock(const SheepState& prior, Vec3 displacement,
                                   const PaddockCollisionField& paddock, SheepState& next,
                                   SheepCollisionEvidence& evidence) noexcept {
    const CylinderMoveResult resolved =
        paddock.resolve_cylinder_move(prior.position, displacement, kSheepCollisionRadius);
    next.position = resolved.position;
    // The dog's accepted contact rule: a clipped axis loses its velocity on the
    // first contact tick, so a sheep held against a wall cannot accumulate speed
    // into it and then shoot away when the wall ends.
    if (resolved.clipped_x) {
        next.velocity.x = 0.0;
    }
    if (resolved.clipped_z) {
        next.velocity.z = 0.0;
    }
    next.grounded = std::isfinite(next.position.y);
    evidence.clipped_x = resolved.clipped_x;
    evidence.clipped_z = resolved.clipped_z;
    evidence.obstacle = resolved.obstacle;
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
    if (dog_pressure.enabled || dog_approach.enabled || dog_facing.enabled) {
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
}

// How much of the ordinary dog response this sheep produces. `ordinary` is
// exactly neutral by definition rather than by configuration, and a scenario
// with the factor switched off is neutral for every sheep, so a paired control
// reproduces the accepted arithmetic bit for bit while still carrying the same
// temperaments in its fixture.
[[nodiscard]] double
sheep_temperament_response_scale(SheepTemperament temperament,
                                 const SheepTemperamentConfiguration& configuration) noexcept {
    if (!configuration.enabled) {
        return 1.0;
    }
    switch (temperament) {
    case SheepTemperament::ordinary:
        break;
    case SheepTemperament::nervous:
        return configuration.nervous_response_scale;
    case SheepTemperament::stubborn:
        return configuration.stubborn_response_scale;
    }
    return 1.0;
}

void evaluate_dog_stimulus(const SheepState& prior_sheep, const DogState& prior_dog,
                           const GameplayScenarioDefinition& scenario,
                           const PaddockCollisionField& paddock,
                           SheepDogPressureEvidence& dog_evidence) noexcept {
    const SheepDogPressureConfiguration& dog_pressure = scenario.sheep_dog_pressure;
    const SheepDogApproachConfiguration& dog_approach = scenario.sheep_dog_approach;
    const SheepDogFacingConfiguration& dog_facing = scenario.sheep_dog_facing;
    const SheepDogLineOfSightConfiguration& line_of_sight = scenario.sheep_dog_line_of_sight;
    constexpr double kTwoPi = 6.28318530717958647692;
    const double dog_offset_x = prior_dog.position.x - prior_sheep.position.x;
    const double dog_offset_z = prior_dog.position.z - prior_sheep.position.z;
    const double dog_distance = std::hypot(dog_offset_x, dog_offset_z);
    dog_evidence.stimulus_evaluated = true;
    dog_evidence.dog_distance = dog_distance;
    // Temperament is a property of the prior sheep rather than of the geometry,
    // so it is published as soon as the stimulus is evaluated. That keeps it
    // readable even where an exact overlap leaves every geometric term zero, and
    // it keeps "evaluated" and "scaled by something meaningful" the same thing.
    const double response_scale =
        sheep_temperament_response_scale(prior_sheep.temperament, scenario.sheep_temperament);
    dog_evidence.temperament_response_scale = response_scale;
    if (dog_distance > 0.0) {
        dog_evidence.dog_relative_bearing_radians = std::remainder(
            std::atan2(dog_offset_x, -dog_offset_z) - prior_sheep.heading_radians, kTwoPi);
        // Away direction and approach speed share one unit vector so the two
        // dog terms cannot disagree about where the dog is.
        const double away_x = -dog_offset_x / dog_distance;
        const double away_z = -dog_offset_z / dog_distance;
        dog_evidence.dog_approach_speed =
            prior_dog.velocity.x * away_x + prior_dog.velocity.z * away_z;
        // Heading zero is the -z forward direction used by the dog motor, so
        // facing alignment is the cosine between that forward direction and
        // the same dog-to-sheep unit vector.
        const double forward_x = std::sin(prior_dog.heading_radians);
        const double forward_z = -std::cos(prior_dog.heading_radians);
        dog_evidence.dog_facing_alignment =
            std::clamp(forward_x * away_x + forward_z * away_z, -1.0, 1.0);
        // Sight is tested from the immutable prior positions against the same
        // analytic shapes the dog collides with, so a wall or closed gate that
        // stops the dog also hides it.
        const PaddockObstacle occluder =
            paddock.blocking_obstacle(prior_sheep.position.x, prior_sheep.position.z,
                                      prior_dog.position.x, prior_dog.position.z);
        dog_evidence.dog_line_of_sight_blocked = occluder != PaddockObstacle::none;
        dog_evidence.dog_line_of_sight_occluder = occluder;
        // Visibility is binary and multiplies the other dog terms: an occluded
        // dog releases them instead of adding a fourth vector, so the published
        // per-term vectors stay the applied vectors. Multiplying by exactly 1.0
        // leaves the accepted visible-case arithmetic unchanged.
        const double visibility =
            line_of_sight.enabled && dog_evidence.dog_line_of_sight_blocked ? 0.0 : 1.0;

        // A fully released term keeps its zeroed default rather than a scaled
        // vector, because scaling an away direction by zero would publish a
        // signed zero and make two identically released states differ in the
        // canonical state dump.
        //
        // The temperament scale is the last factor of every magnitude, so an
        // ordinary sheep multiplies by exactly 1.0 and reproduces the accepted
        // arithmetic bit for bit. It scales the response only: distance,
        // bearing, approach speed, facing alignment, and the sight line are the
        // stimulus and stay identical between two differently tempered sheep.
        if (visibility > 0.0) {
            const double falloff =
                dog_distance < dog_pressure.radius ? 1.0 - dog_distance / dog_pressure.radius : 0.0;
            if (dog_pressure.enabled) {
                const double magnitude =
                    visibility * falloff * dog_pressure.maximum_acceleration * response_scale;
                dog_evidence.pressure_acceleration = {.x = away_x * magnitude,
                                                      .z = away_z * magnitude};
            }
            if (dog_approach.enabled && dog_evidence.dog_approach_speed > 0.0) {
                // Only a closing dog adds pressure; a leaving dog releases it
                // rather than pulling the sheep back.
                const double response =
                    std::min(dog_evidence.dog_approach_speed / dog_approach.reference_speed, 1.0);
                const double magnitude = visibility * falloff * dog_approach.maximum_acceleration *
                                         response * response_scale;
                dog_evidence.approach_acceleration = {.x = away_x * magnitude,
                                                      .z = away_z * magnitude};
            }
            if (dog_facing.enabled && dog_evidence.dog_facing_alignment > 0.0) {
                // Only a dog looking toward the sheep adds pressure; a dog
                // looking away releases it rather than pulling the sheep back.
                const double magnitude = visibility * falloff * dog_facing.maximum_acceleration *
                                         dog_evidence.dog_facing_alignment * response_scale;
                dog_evidence.facing_acceleration = {.x = away_x * magnitude,
                                                    .z = away_z * magnitude};
            }
        }
    }
    // Exact overlap has no geometric away direction and nothing can stand
    // between the pair. Publish the zero bearing/approach/facing/sight data
    // instead of inventing a hidden random or ID-based turn.
}

void apply_separation(const SheepStateBuffer& prior, std::size_t index,
                      const SheepSeparationConfiguration& separation, const SheepSpatialGrid& grid,
                      std::span<SpatialNeighbor> neighbor_scratch,
                      SheepSocialEvidence& evidence) noexcept {
    const SpatialGridQueryResult query =
        grid.query_neighbors(index, separation.radius, neighbor_scratch);
    WIDE_EYE_ASSERT(query.error == SpatialGridQueryError::none && !query.truncated(),
                    "five-sheep separation query must return every close neighbor");

    double acceleration_x = 0.0;
    double acceleration_z = 0.0;
    for (std::size_t neighbor_index = 0; neighbor_index < query.neighbor_count; ++neighbor_index) {
        const SpatialNeighbor& neighbor = neighbor_scratch[neighbor_index];
        double direction_x = prior[index].position.x - prior[neighbor.member_index].position.x;
        double direction_z = prior[index].position.z - prior[neighbor.member_index].position.z;
        if (neighbor.distance > 0.0) {
            direction_x /= neighbor.distance;
            direction_z /= neighbor.distance;
        } else {
            // Exact overlap has no geometric direction. The stable-ID tie break is
            // antisymmetric, deterministic, and independent of buffer/update order.
            direction_x = prior[index].id < neighbor.id ? -1.0 : 1.0;
            direction_z = 0.0;
        }
        const double weight = 1.0 - neighbor.distance / separation.radius;
        acceleration_x += direction_x * weight * separation.maximum_acceleration;
        acceleration_z += direction_z * weight * separation.maximum_acceleration;
    }

    const double acceleration_length = std::hypot(acceleration_x, acceleration_z);
    if (acceleration_length > separation.maximum_acceleration) {
        const double scale = separation.maximum_acceleration / acceleration_length;
        acceleration_x *= scale;
        acceleration_z *= scale;
    }
    evidence.separation_acceleration = {.x = acceleration_x, .z = acceleration_z};
}

void apply_attraction(const SheepStateBuffer& prior, std::size_t index,
                      const SheepAttractionConfiguration& attraction, const SheepSpatialGrid& grid,
                      std::span<SpatialNeighbor> neighbor_scratch,
                      SheepSocialEvidence& evidence) noexcept {
    const SpatialGridQueryResult query = grid.query_neighbors(
        index, attraction.radius, neighbor_scratch.first(attraction.neighbor_limit));
    WIDE_EYE_ASSERT(query.error == SpatialGridQueryError::none,
                    "five-sheep attraction query must succeed");

    evidence.attraction_neighbor_count = static_cast<std::uint32_t>(query.neighbor_count);
    evidence.attraction_candidate_count = static_cast<std::uint32_t>(query.within_radius_count);

    Vec3 selected_centroid{};
    for (std::size_t neighbor_index = 0; neighbor_index < query.neighbor_count; ++neighbor_index) {
        const SpatialNeighbor& neighbor = neighbor_scratch[neighbor_index];
        evidence.attraction_neighbor_ids[neighbor_index] = neighbor.id;
        selected_centroid.x += prior[neighbor.member_index].position.x;
        selected_centroid.z += prior[neighbor.member_index].position.z;
    }
    if (query.neighbor_count != 0) {
        const double selected_count = static_cast<double>(query.neighbor_count);
        selected_centroid.x /= selected_count;
        selected_centroid.z /= selected_count;
        const double offset_x = selected_centroid.x - prior[index].position.x;
        const double offset_z = selected_centroid.z - prior[index].position.z;
        const double scale = attraction.maximum_acceleration / attraction.radius;
        double acceleration_x = offset_x * scale;
        double acceleration_z = offset_z * scale;
        const double acceleration_length = std::hypot(acceleration_x, acceleration_z);
        if (acceleration_length > attraction.maximum_acceleration) {
            const double bounded_scale = attraction.maximum_acceleration / acceleration_length;
            acceleration_x *= bounded_scale;
            acceleration_z *= bounded_scale;
        }
        evidence.attraction_acceleration = {.x = acceleration_x, .z = acceleration_z};
    }
}

void apply_alignment(const SheepStateBuffer& prior, std::size_t index,
                     const SheepAlignmentConfiguration& alignment, const SheepSpatialGrid& grid,
                     std::span<SpatialNeighbor> neighbor_scratch,
                     SheepSocialEvidence& evidence) noexcept {
    const SpatialGridQueryResult query = grid.query_neighbors(
        index, alignment.radius, neighbor_scratch.first(alignment.neighbor_limit));
    WIDE_EYE_ASSERT(query.error == SpatialGridQueryError::none,
                    "five-sheep alignment query must succeed");

    evidence.alignment_neighbor_count = static_cast<std::uint32_t>(query.neighbor_count);
    evidence.alignment_candidate_count = static_cast<std::uint32_t>(query.within_radius_count);

    Vec3 selected_velocity{};
    for (std::size_t neighbor_index = 0; neighbor_index < query.neighbor_count; ++neighbor_index) {
        const SpatialNeighbor& neighbor = neighbor_scratch[neighbor_index];
        evidence.alignment_neighbor_ids[neighbor_index] = neighbor.id;
        selected_velocity.x += prior[neighbor.member_index].velocity.x;
        selected_velocity.z += prior[neighbor.member_index].velocity.z;
    }
    if (query.neighbor_count != 0) {
        const double selected_count = static_cast<double>(query.neighbor_count);
        selected_velocity.x /= selected_count;
        selected_velocity.z /= selected_count;
        double acceleration_x =
            (selected_velocity.x - prior[index].velocity.x) / alignment.response_time_seconds;
        double acceleration_z =
            (selected_velocity.z - prior[index].velocity.z) / alignment.response_time_seconds;
        const double acceleration_length = std::hypot(acceleration_x, acceleration_z);
        if (acceleration_length > alignment.maximum_acceleration) {
            const double bounded_scale = alignment.maximum_acceleration / acceleration_length;
            acceleration_x *= bounded_scale;
            acceleration_z *= bounded_scale;
        }
        evidence.alignment_acceleration = {.x = acceleration_x, .z = acceleration_z};
    }
}

// Obstacle and drop avoidance. This is a steering term and nothing more: it
// publishes one acceleration vector, that vector joins the sum, and the combined
// bound scales it exactly as it scales every other term.
// `resolve_sheep_against_paddock` is still the last positional authority and is
// not reordered, weakened, or consulted here — the point of the term is that the
// authority has less work to do, not that it has less power.
//
// Both halves probe along the sheep's own direction of travel, taken from the
// immutable prior velocity. A sheep at or below `kSheepHeadingMotionSpeedFloor`
// is not measurably going anywhere, so there is no direction to probe: the term
// publishes an unevaluated record instead of steering away from a direction that
// rounding invented, exactly as the heading rule refuses to face one.
//
// The obstacle half asks the analytic paddock which shape the sheep's swept body
// reaches first, then pushes away from the face it would enter. When the
// geometry names a nearer free edge of that same shape *and* the edge is within
// the look-ahead — that is, when there is a way round the sheep could actually
// reach — the direction along the face toward it is added, so the sheep steers
// around the shape rather than only braking against it. An edge further away
// than the sheep can see is not a way round, and a sheep exactly between the two
// edges has no nearer one; both keep the pure away-from-the-face push rather
// than committing to a side that the geometry did not name.
//
// The drop half is deliberately cruder, because `ground_height` answers a point
// question rather than a distance one: the ground under the look-ahead point
// either exists or does not. When it does not, the term pushes straight back
// along the sheep's own approach, which is the only direction away from a drop
// that a point query names. Grading that response, or steering along the edge
// instead of retreating from it, would need a boundary shape the query does not
// expose and the flat paddock could not exercise.
void apply_avoidance(const SheepState& prior, const SheepAvoidanceConfiguration& avoidance,
                     const PaddockCollisionField& paddock,
                     SheepAvoidanceEvidence& evidence) noexcept {
    if (!avoidance.enabled) {
        return;
    }
    const double speed = std::hypot(prior.velocity.x, prior.velocity.z);
    if (speed <= kSheepHeadingMotionSpeedFloor) {
        return;
    }

    const double travel_x = prior.velocity.x / speed;
    const double travel_z = prior.velocity.z / speed;
    evidence.avoidance_evaluated = true;

    double acceleration_x = 0.0;
    double acceleration_z = 0.0;
    const ObstacleApproach approach =
        paddock.approaching_obstacle(prior.position, {.x = travel_x, .z = travel_z},
                                     avoidance.look_ahead_distance, kSheepCollisionRadius);
    if (approach.obstacle != PaddockObstacle::none) {
        evidence.obstacle = approach.obstacle;
        evidence.obstacle_distance = approach.contact_distance;
        double direction_x = approach.face_normal.x;
        double direction_z = approach.face_normal.z;
        if (approach.lateral_escape != Vec3{} &&
            approach.lateral_clearance <= avoidance.look_ahead_distance) {
            direction_x += approach.lateral_escape.x;
            direction_z += approach.lateral_escape.z;
        }
        // The same linear falloff shape the accepted dog terms use, over the
        // look-ahead instead of over a radius: nothing at all at the far end,
        // the full maximum at contact. A shape exactly at the look-ahead
        // distance therefore publishes a named obstacle and a zero vector, so
        // the boundary is continuous rather than a step.
        const double urgency = 1.0 - approach.contact_distance / avoidance.look_ahead_distance;
        const double direction_length = std::hypot(direction_x, direction_z);
        if (urgency > 0.0 && direction_length > 0.0) {
            const double magnitude = avoidance.maximum_acceleration * urgency / direction_length;
            acceleration_x += direction_x * magnitude;
            acceleration_z += direction_z * magnitude;
        }
    }

    const double probe_x = prior.position.x + travel_x * avoidance.look_ahead_distance;
    const double probe_z = prior.position.z + travel_z * avoidance.look_ahead_distance;
    if (!std::isfinite(paddock.ground_height(probe_x, probe_z))) {
        evidence.drop_ahead = true;
        acceleration_x -= travel_x * avoidance.maximum_acceleration;
        acceleration_z -= travel_z * avoidance.maximum_acceleration;
    }

    // One term, one maximum. The two halves are summed and the total is held to
    // the same named maximum, exactly as close-range separation holds the sum of
    // its per-neighbour pushes, so avoidance cannot become the strongest
    // influence in the flock by reacting to two things at once.
    const double acceleration_length = std::hypot(acceleration_x, acceleration_z);
    if (acceleration_length > avoidance.maximum_acceleration) {
        const double scale = avoidance.maximum_acceleration / acceleration_length;
        acceleration_x *= scale;
        acceleration_z *= scale;
    }
    evidence.avoidance_acceleration = {.x = acceleration_x, .z = acceleration_z};
}

// The two limits that act on the result of integration rather than on any
// steering term, applied after the combined-influence bound and before the
// paddock resolves the displacement. Neither one rewrites a published
// acceleration vector: the terms still asked for exactly what they published,
// and this record is where the difference between that request and the motion it
// produced is recorded.
//
// Heading follows motion, not steering, and it is limited to one turn budget per
// tick using the same shortest-arc rule the dog motor uses. It is derived from
// the immutable prior heading rather than from `next`, so this tick's published
// dog bearing — which is relative to that prior heading — cannot be changed
// retroactively by the rotation this function applies.
//
// The turn rate limits which way the sheep faces and nothing else. It does not
// constrain the direction the sheep travels, so a sheep pushed sideways still
// moves sideways while its heading catches up; ADR 0007 records why slaving
// motion to heading is a separate motion-model decision.
void apply_motion_limits(const SheepMotionLimitConfiguration& limits, const SheepState& prior,
                         SheepState& next, SheepMotionLimitEvidence& evidence) noexcept {
    if (!limits.enabled) {
        return;
    }

    const double integrated_speed = std::hypot(next.velocity.x, next.velocity.z);
    double speed_scale = 1.0;
    if (integrated_speed > limits.maximum_speed) {
        // Scaling both components preserves the direction integration produced;
        // a sheep under the maximum takes no arithmetic at all, so it stays
        // byte-identical rather than merely multiplied by one.
        speed_scale = limits.maximum_speed / integrated_speed;
        next.velocity.x *= speed_scale;
        next.velocity.z *= speed_scale;
    }

    evidence.limit_evaluated = true;
    evidence.integrated_speed = integrated_speed;
    evidence.applied_speed_scale = speed_scale;
    evidence.applied_speed = std::hypot(next.velocity.x, next.velocity.z);
    if (evidence.applied_speed <= kSheepHeadingMotionSpeedFloor) {
        // A sheep that is not measurably moving has no motion direction to face.
        // Keeping the prior heading is the only answer that does not invent one.
        return;
    }

    // Heading zero is the -z forward direction the dog motor and the dog-facing
    // term already use.
    const double motion_heading = std::atan2(next.velocity.x, -next.velocity.z);
    const double turn_budget =
        limits.maximum_turn_rate_radians_per_second * GameplaySimulation::kFixedDeltaSeconds;
    constexpr double kTwoPi = 6.28318530717958647692;
    next.heading_radians = approach_angle(prior.heading_radians, motion_heading, turn_budget);
    evidence.motion_heading_followed = true;
    evidence.motion_heading_radians = motion_heading;
    evidence.heading_change_radians =
        std::remainder(next.heading_radians - prior.heading_radians, kTwoPi);
}

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

    // Every fixture keeps the explicit prior-to-next pass so behavior cannot
    // acquire update-order dependence. The named motion fixture is scripted
    // presentation evidence, not a social-response implementation.
    for (std::size_t index = 0; index < prior.size(); ++index) {
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

    const double grid_cell_size = std::max({separation.enabled ? separation.radius : 0.0,
                                            attraction.enabled ? attraction.radius : 0.0,
                                            alignment.enabled ? alignment.radius : 0.0});
    if (grid_cell_size > 0.0) {
        // The rebuild call stays outside the assertion so a future
        // release-disabled assert cannot silently remove it.
        const SpatialGridBuildError build_error = grid.rebuild(prior, grid_cell_size);
        WIDE_EYE_ASSERT(build_error == SpatialGridBuildError::none,
                        "valid sheep snapshot must rebuild the social-response grid");
        static_cast<void>(build_error);
    }

    std::array<SpatialNeighbor, kGameplaySheepCount - 1> separation_scratch{};
    std::array<SpatialNeighbor, kMaximumSelectedAttractionNeighbors> attraction_scratch{};
    std::array<SpatialNeighbor, kMaximumSelectedAlignmentNeighbors> alignment_scratch{};
    for (std::size_t index = 0; index < prior.size(); ++index) {
        SheepSocialEvidence& evidence = next.sheep_social_evidence[index];
        SheepDogPressureEvidence& dog_evidence = next.sheep_dog_pressure_evidence[index];
        SheepAvoidanceEvidence& avoidance_evidence = next.sheep_avoidance_evidence[index];
        SheepCombinedInfluenceEvidence& combined_evidence =
            next.sheep_combined_influence_evidence[index];

        evaluate_dog_stimulus(prior[index], previous.dog, scenario, paddock, dog_evidence);
        if (separation.enabled) {
            apply_separation(prior, index, separation, grid, separation_scratch, evidence);
        }
        if (attraction.enabled) {
            apply_attraction(prior, index, attraction, grid, attraction_scratch, evidence);
        }
        if (alignment.enabled) {
            apply_alignment(prior, index, alignment, grid, alignment_scratch, evidence);
        }
        apply_avoidance(prior[index], avoidance, paddock, avoidance_evidence);

        double acceleration_x =
            evidence.separation_acceleration.x + evidence.attraction_acceleration.x +
            evidence.alignment_acceleration.x + dog_evidence.pressure_acceleration.x +
            dog_evidence.approach_acceleration.x + dog_evidence.facing_acceleration.x +
            avoidance_evidence.avoidance_acceleration.x;
        double acceleration_z =
            evidence.separation_acceleration.z + evidence.attraction_acceleration.z +
            evidence.alignment_acceleration.z + dog_evidence.pressure_acceleration.z +
            dog_evidence.approach_acceleration.z + dog_evidence.facing_acceleration.z +
            avoidance_evidence.avoidance_acceleration.z;

        // The one place the terms become a single acceleration is the one place
        // the combined bound belongs. It scales the sum, never an individual
        // term, so every vector published above stays exactly what its term
        // produced and only this record explains the difference. A sum that
        // stays under the bound takes no arithmetic at all, so an under-bound
        // scenario is byte-identical rather than merely multiplied by one.
        const double summed_magnitude = std::hypot(acceleration_x, acceleration_z);
        double combined_scale = 1.0;
        if (combined.enabled && summed_magnitude > combined.maximum_acceleration) {
            combined_scale = combined.maximum_acceleration / summed_magnitude;
            acceleration_x *= combined_scale;
            acceleration_z *= combined_scale;
        }
        combined_evidence.bound_evaluated = true;
        combined_evidence.summed_acceleration_magnitude = summed_magnitude;
        combined_evidence.applied_scale = combined_scale;
        combined_evidence.applied_acceleration = {.x = acceleration_x, .z = acceleration_z};

        next.sheep[index].velocity.x += acceleration_x * GameplaySimulation::kFixedDeltaSeconds;
        next.sheep[index].velocity.z += acceleration_z * GameplaySimulation::kFixedDeltaSeconds;
        // Speed and turning limit the result of integration, so they run after
        // the combined bound and still before collision: the paddock remains the
        // last authority over where the sheep actually ends up.
        apply_motion_limits(motion_limit, prior[index], next.sheep[index],
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
    if (scenario_.sheep_fixture == SheepFixture::local_social_response) {
        validate_social_response_configuration(scenario_);
    }
    current_.dog = dog_.state();
    current_.sheep = scenario_.initial_sheep;
    current_.sheep_social_evidence = empty_social_evidence(current_.sheep);
    current_.sheep_dog_pressure_evidence = empty_dog_pressure_evidence(current_.sheep);
    current_.sheep_collision_evidence = empty_collision_evidence(current_.sheep);
    current_.sheep_avoidance_evidence = empty_avoidance_evidence(current_.sheep);
    current_.sheep_combined_influence_evidence = empty_combined_influence_evidence(current_.sheep);
    current_.sheep_motion_limit_evidence = empty_motion_limit_evidence(current_.sheep);
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
    current_ = {.tick = 0,
                .dog = dog_.state(),
                .sheep = scenario_.initial_sheep,
                .sheep_social_evidence = empty_social_evidence(scenario_.initial_sheep),
                .sheep_dog_pressure_evidence = empty_dog_pressure_evidence(scenario_.initial_sheep),
                .sheep_collision_evidence = empty_collision_evidence(scenario_.initial_sheep),
                .sheep_avoidance_evidence = empty_avoidance_evidence(scenario_.initial_sheep),
                .sheep_combined_influence_evidence =
                    empty_combined_influence_evidence(scenario_.initial_sheep),
                .sheep_motion_limit_evidence =
                    empty_motion_limit_evidence(scenario_.initial_sheep)};
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
        .sheep = current_.sheep,
        .sheep_social_evidence = current_.sheep_social_evidence,
        .sheep_dog_pressure_evidence = current_.sheep_dog_pressure_evidence,
        .sheep_collision_evidence = current_.sheep_collision_evidence,
        .sheep_avoidance_evidence = current_.sheep_avoidance_evidence,
        .sheep_combined_influence_evidence = current_.sheep_combined_influence_evidence,
        .sheep_motion_limit_evidence = current_.sheep_motion_limit_evidence,
    };
    for (std::size_t index = 0; index < result.sheep.size(); ++index) {
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
