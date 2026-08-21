#include "game/sheep_rules.hpp"

// The rules integrate over the authoritative fixed step, and there is exactly
// one of those. Reading it from the simulation that owns the scheduler is
// deliberate: a second locally defined delta would be a silent second clock.
#include "game/gameplay_simulation.hpp"

#include <algorithm>
#include <cmath>

namespace wide_eye::game {
namespace {

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

// The four behavior states, selected from the immutable prior state.
//
// **Arousal is a named game parameter, not a claim about animal physiology.**
// The rule below decides which of four *labels* a sheep carries; it does not
// model a nervous system and no part of this project has calibrated it against
// an animal.
//
// Every input is prior state: the prior label, the arousal that label produced,
// and the prior-state stimulus. Nothing here reads the state being written, so
// the same tick's arousal update cannot change the transition and the two can be
// read side by side in the dump.
//
// The rule is a Schmitt trigger on arousal, plus one release test on the cause.
// `rest_arousal` is both the bottom of the ladder and the "is a cause acting"
// test, because a stimulus too weak to lift a sheep out of rest is not pressure;
// using one number for both keeps them from disagreeing.
[[nodiscard]] SheepBehaviorState
next_sheep_behavior(const SheepState& prior, double stimulus,
                    const SheepBehaviorConfiguration& behavior) noexcept {
    if (stimulus <= behavior.rest_arousal) {
        // The cause has been released. A sheep still carrying arousal is
        // recovering — the state that exists so that release is a verb — and one
        // that has shed it is settled.
        return prior.arousal > behavior.rest_arousal ? SheepBehaviorState::recovering
                                                     : SheepBehaviorState::settled;
    }
    // A cause is acting. Each band is entered at its named arousal and left only
    // at a lower one, so a sheep sitting on an entry threshold holds its label
    // instead of alternating every tick.
    if (prior.arousal >= behavior.driven_arousal ||
        (prior.behavior == SheepBehaviorState::driven &&
         prior.arousal > behavior.driven_release_arousal)) {
        return SheepBehaviorState::driven;
    }
    // A sheep that is already engaged — alert, driven, or recovering from an
    // earlier press — stays alert down to rest rather than dropping back to
    // settled at the alert entry threshold.
    const bool engaged = prior.behavior != SheepBehaviorState::settled;
    if (prior.arousal >= behavior.alert_arousal ||
        (engaged && prior.arousal > behavior.rest_arousal)) {
        return SheepBehaviorState::alert;
    }
    return SheepBehaviorState::settled;
}

} // namespace

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

void evaluate_sheep_dog_stimulus(const SheepState& prior_sheep, const DogState& prior_dog,
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
    // The same linear distance falloff the three dog terms use, hoisted out of
    // the branch below because an exact overlap is the *strongest* proximity
    // rather than an absent one: there is no away direction to publish at zero
    // distance, but there is certainly a cause. Line of sight can only release
    // it inside the branch, where an occluder can exist at all — nothing can
    // stand between a pair at the same point.
    const double proximity =
        dog_distance < dog_pressure.radius ? 1.0 - dog_distance / dog_pressure.radius : 0.0;
    double arousal_visibility = 1.0;
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
        arousal_visibility = visibility;

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

    // How much pressure this sheep is under, as a dimensionless fraction rather
    // than as an acceleration. It is published whether or not the behavior
    // transitions are switched on, so a paired control publishes an identical
    // cause and only the applied arousal differs. The clamp matters: a nervous
    // sheep's response scale can carry the product above one, and arousal is a
    // bounded design parameter rather than an unbounded accumulation.
    dog_evidence.arousal_stimulus = std::clamp(arousal_visibility * proximity * response_scale,
                                               kSheepMinimumArousal, kSheepMaximumArousal);
}

// The arousal proxy and the transition it selects, written into the next sheep
// buffer from the immutable prior one.
//
// **Arousal is a named game parameter, not a claim about animal physiology.** It
// is a bounded `[0, 1]` design variable that follows the published dog stimulus
// at named rates; it is not a heart rate, a stress hormone, or a measured animal
// response.
//
// The follower never overshoots: when the stimulus is within one tick's budget
// the arousal is assigned exactly, so it settles on its cause instead of
// oscillating around it, and it stays inside `[0, 1]` because both endpoints do.
//
// This is deliberately observational: neither the arousal nor the state it
// selects is read by any steering term in this outcome. ADR 0009 records why one
// isolated variable at a time is worth more than a feedback loop that would
// re-derive every accepted per-term oracle.
void apply_sheep_behavior_transition(const SheepState& prior,
                                     const SheepBehaviorConfiguration& behavior, double stimulus,
                                     SheepState& next) noexcept {
    if (!behavior.enabled) {
        return;
    }
    const double rise_step = behavior.rise_rate_per_second * GameplaySimulation::kFixedDeltaSeconds;
    const double recovery_step =
        behavior.recovery_rate_per_second * GameplaySimulation::kFixedDeltaSeconds;
    const double difference = stimulus - prior.arousal;
    if (difference > rise_step) {
        next.arousal = prior.arousal + rise_step;
    } else if (difference < -recovery_step) {
        next.arousal = prior.arousal - recovery_step;
    } else {
        next.arousal = stimulus;
    }
    next.behavior = next_sheep_behavior(prior, stimulus, behavior);
}

double sheep_social_grid_cell_size(const GameplayScenarioDefinition& scenario) noexcept {
    const SheepSeparationConfiguration& separation = scenario.sheep_separation;
    const SheepAttractionConfiguration& attraction = scenario.sheep_attraction;
    const SheepAlignmentConfiguration& alignment = scenario.sheep_alignment;
    return std::max({separation.enabled ? separation.radius : 0.0,
                     attraction.enabled ? attraction.radius : 0.0,
                     alignment.enabled ? alignment.radius : 0.0});
}

SheepNeighborSelection select_sheep_neighbors(
    const SheepSpatialGrid& grid, std::size_t index, const SheepSeparationConfiguration& separation,
    const SheepAttractionConfiguration& attraction, const SheepAlignmentConfiguration& alignment,
    std::span<SpatialNeighbor> separation_scratch, std::span<SpatialNeighbor> attraction_scratch,
    std::span<SpatialNeighbor> alignment_scratch) noexcept {
    SheepNeighborSelection selection;
    if (separation.enabled) {
        selection.separation = grid.query_neighbors(index, separation.radius, separation_scratch);
        // Separation answers every close neighbour rather than a bounded set, so
        // a truncated result would silently drop a push. The caller's scratch
        // has to hold the whole flock minus the subject for that to hold at any
        // member count.
        WIDE_EYE_ASSERT(selection.separation.error == SpatialGridQueryError::none &&
                            !selection.separation.truncated(),
                        "separation query must return every close neighbor");
    }
    if (attraction.enabled) {
        selection.attraction = grid.query_neighbors(
            index, attraction.radius, attraction_scratch.first(attraction.neighbor_limit));
        WIDE_EYE_ASSERT(selection.attraction.error == SpatialGridQueryError::none,
                        "attraction query must succeed");
    }
    if (alignment.enabled) {
        selection.alignment = grid.query_neighbors(
            index, alignment.radius, alignment_scratch.first(alignment.neighbor_limit));
        WIDE_EYE_ASSERT(selection.alignment.error == SpatialGridQueryError::none,
                        "alignment query must succeed");
    }
    return selection;
}

void apply_sheep_separation(std::span<const SheepState> prior, std::size_t index,
                            const SheepSeparationConfiguration& separation,
                            const SpatialGridQueryResult& query,
                            std::span<const SpatialNeighbor> selected,
                            SheepSocialEvidence& evidence) noexcept {
    double acceleration_x = 0.0;
    double acceleration_z = 0.0;
    for (std::size_t neighbor_index = 0; neighbor_index < query.neighbor_count; ++neighbor_index) {
        const SpatialNeighbor& neighbor = selected[neighbor_index];
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

void apply_sheep_attraction(std::span<const SheepState> prior, std::size_t index,
                            const SheepAttractionConfiguration& attraction,
                            const SpatialGridQueryResult& query,
                            std::span<const SpatialNeighbor> selected,
                            SheepSocialEvidence& evidence) noexcept {
    evidence.attraction_neighbor_count = static_cast<std::uint32_t>(query.neighbor_count);
    evidence.attraction_candidate_count = static_cast<std::uint32_t>(query.within_radius_count);

    Vec3 selected_centroid{};
    for (std::size_t neighbor_index = 0; neighbor_index < query.neighbor_count; ++neighbor_index) {
        const SpatialNeighbor& neighbor = selected[neighbor_index];
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

void apply_sheep_alignment(std::span<const SheepState> prior, std::size_t index,
                           const SheepAlignmentConfiguration& alignment,
                           const SpatialGridQueryResult& query,
                           std::span<const SpatialNeighbor> selected,
                           SheepSocialEvidence& evidence) noexcept {
    evidence.alignment_neighbor_count = static_cast<std::uint32_t>(query.neighbor_count);
    evidence.alignment_candidate_count = static_cast<std::uint32_t>(query.within_radius_count);

    Vec3 selected_velocity{};
    for (std::size_t neighbor_index = 0; neighbor_index < query.neighbor_count; ++neighbor_index) {
        const SpatialNeighbor& neighbor = selected[neighbor_index];
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
void apply_sheep_avoidance(const SheepState& prior, const SheepAvoidanceConfiguration& avoidance,
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

void apply_sheep_combined_influence(const SheepSocialEvidence& social,
                                    const SheepDogPressureEvidence& dog_pressure,
                                    const SheepAvoidanceEvidence& avoidance,
                                    const SheepCombinedInfluenceConfiguration& combined,
                                    SheepCombinedInfluenceEvidence& evidence,
                                    SheepState& next) noexcept {
    double acceleration_x = social.separation_acceleration.x + social.attraction_acceleration.x +
                            social.alignment_acceleration.x + dog_pressure.pressure_acceleration.x +
                            dog_pressure.approach_acceleration.x +
                            dog_pressure.facing_acceleration.x + avoidance.avoidance_acceleration.x;
    double acceleration_z = social.separation_acceleration.z + social.attraction_acceleration.z +
                            social.alignment_acceleration.z + dog_pressure.pressure_acceleration.z +
                            dog_pressure.approach_acceleration.z +
                            dog_pressure.facing_acceleration.z + avoidance.avoidance_acceleration.z;

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
    evidence.bound_evaluated = true;
    evidence.summed_acceleration_magnitude = summed_magnitude;
    evidence.applied_scale = combined_scale;
    evidence.applied_acceleration = {.x = acceleration_x, .z = acceleration_z};

    next.velocity.x += acceleration_x * GameplaySimulation::kFixedDeltaSeconds;
    next.velocity.z += acceleration_z * GameplaySimulation::kFixedDeltaSeconds;
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
void apply_sheep_motion_limits(const SheepMotionLimitConfiguration& limits, const SheepState& prior,
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

} // namespace wide_eye::game
