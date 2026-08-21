#include "game/flock_observables.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace wide_eye::game {
namespace {

[[nodiscard]] double planar_distance(const Vec3& left, const Vec3& right) noexcept {
    return std::hypot(left.x - right.x, left.z - right.z);
}

[[nodiscard]] bool valid_state(const SheepState& sheep) noexcept {
    return sheep.id != 0 && std::isfinite(sheep.position.x) && std::isfinite(sheep.position.y) &&
           std::isfinite(sheep.position.z) && std::isfinite(sheep.velocity.x) &&
           std::isfinite(sheep.velocity.y) && std::isfinite(sheep.velocity.z) &&
           std::isfinite(sheep.heading_radians) && std::isfinite(sheep.arousal) &&
           is_known_sheep_behavior(sheep.behavior) && is_known_sheep_temperament(sheep.temperament);
}

// The identity and finite-value rules a published buffer must satisfy before
// either pass in this file will describe it. Both passes share one predicate so
// a snapshot the spatial pass rejects can never be one the timing pass folds in.
[[nodiscard]] bool valid_sheep_buffer(const SheepStateBuffer& sheep) noexcept {
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        if (!valid_state(sheep[index])) {
            return false;
        }
        for (std::size_t prior = 0; prior < index; ++prior) {
            if (sheep[index].id == sheep[prior].id) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

std::optional<FiveSheepObservables> compute_five_sheep_observables(
    const SheepStateBuffer& sheep,
    const std::array<std::uint32_t, kGameplaySheepCount>& chosen_neighbor_counts,
    double connectivity_distance, const std::optional<Vec3>& dog_position) noexcept {
    if (!std::isfinite(connectivity_distance) || connectivity_distance < 0.0 ||
        !valid_sheep_buffer(sheep)) {
        return std::nullopt;
    }
    for (const std::uint32_t count : chosen_neighbor_counts) {
        if (count >= sheep.size()) {
            return std::nullopt;
        }
    }
    if (dog_position.has_value() &&
        !(std::isfinite(dog_position->x) && std::isfinite(dog_position->y) &&
          std::isfinite(dog_position->z))) {
        return std::nullopt;
    }

    FiveSheepObservables result;
    for (const SheepState& member : sheep) {
        result.centroid.x += member.position.x;
        result.centroid.y += member.position.y;
        result.centroid.z += member.position.z;
    }
    const double member_count = static_cast<double>(sheep.size());
    result.centroid.x /= member_count;
    result.centroid.y /= member_count;
    result.centroid.z /= member_count;

    double covariance_xx = 0.0;
    double covariance_xz = 0.0;
    double covariance_zz = 0.0;
    double heading_x = 0.0;
    double heading_z = 0.0;
    std::size_t moving_count = 0;
    for (const SheepState& member : sheep) {
        const double offset_x = member.position.x - result.centroid.x;
        const double offset_z = member.position.z - result.centroid.z;
        result.mean_radius += std::hypot(offset_x, offset_z);
        covariance_xx += offset_x * offset_x;
        covariance_xz += offset_x * offset_z;
        covariance_zz += offset_z * offset_z;

        const double speed = std::hypot(member.velocity.x, member.velocity.z);
        result.group_speed += speed;
        if (speed > 0.0) {
            heading_x += member.velocity.x / speed;
            heading_z += member.velocity.z / speed;
            ++moving_count;
        }
    }
    result.mean_radius /= member_count;
    result.group_speed /= member_count;
    if (moving_count != 0) {
        result.polarization = std::hypot(heading_x, heading_z) / static_cast<double>(moving_count);
    }

    covariance_xx /= member_count;
    covariance_xz /= member_count;
    covariance_zz /= member_count;
    const double covariance_trace = covariance_xx + covariance_zz;
    if (covariance_trace > 0.0) {
        const double eigenvalue_delta =
            std::hypot(covariance_xx - covariance_zz, 2.0 * covariance_xz);
        result.elongation = std::clamp(eigenvalue_delta / covariance_trace, 0.0, 1.0);
    }

    std::array<std::size_t, kGameplaySheepCount> component_parent{};
    for (std::size_t index = 0; index < component_parent.size(); ++index) {
        component_parent[index] = index;
        result.nearest_neighbor_spacing[index] = std::numeric_limits<double>::infinity();
    }
    const auto find_root = [&component_parent](std::size_t index) {
        while (component_parent[index] != index) {
            index = component_parent[index];
        }
        return index;
    };
    for (std::size_t left = 0; left < sheep.size(); ++left) {
        for (std::size_t right = left + 1; right < sheep.size(); ++right) {
            const double distance = planar_distance(sheep[left].position, sheep[right].position);
            result.nearest_neighbor_spacing[left] =
                std::min(result.nearest_neighbor_spacing[left], distance);
            result.nearest_neighbor_spacing[right] =
                std::min(result.nearest_neighbor_spacing[right], distance);
            if (distance <= connectivity_distance) {
                const std::size_t left_root = find_root(left);
                const std::size_t right_root = find_root(right);
                if (left_root != right_root) {
                    component_parent[right_root] = left_root;
                }
            }
        }
    }
    for (const double spacing : result.nearest_neighbor_spacing) {
        result.mean_nearest_neighbor_spacing += spacing;
    }
    result.mean_nearest_neighbor_spacing /= member_count;
    for (std::size_t index = 0; index < component_parent.size(); ++index) {
        if (find_root(index) == index) {
            ++result.connected_component_count;
        }
    }

    result.chosen_neighbors.minimum = chosen_neighbor_counts.front();
    for (const std::uint32_t count : chosen_neighbor_counts) {
        result.chosen_neighbors.total += count;
        result.chosen_neighbors.minimum = std::min(result.chosen_neighbors.minimum, count);
        result.chosen_neighbors.maximum = std::max(result.chosen_neighbors.maximum, count);
    }
    result.chosen_neighbors.mean =
        static_cast<double>(result.chosen_neighbors.total) / member_count;

    if (dog_position.has_value()) {
        result.dog.evaluated = true;
        for (const SheepState& member : sheep) {
            const double distance = planar_distance(member.position, *dog_position);
            // An exact tie is broken on the lower ID rather than on buffer
            // order, so a symmetric flock publishes one answer.
            if (result.dog.nearest_sheep_id == 0 || distance < result.dog.nearest_distance ||
                (distance == result.dog.nearest_distance &&
                 member.id < result.dog.nearest_sheep_id)) {
                result.dog.nearest_sheep_id = member.id;
                result.dog.nearest_distance = distance;
            }
        }

        // The bearing is taken on the dog's offset *from* the centroid rather
        // than on the negated push axis, because `a - a` is `+0` while `-(a - a)`
        // is `-0`, and `atan2(-0, negative)` is `-pi` — outside the `(-pi, pi]`
        // range every other heading in this project is normalized to. A dog due
        // south of the flock must publish one bearing, not two.
        const double dog_offset_x = dog_position->x - result.centroid.x;
        const double dog_offset_z = dog_position->z - result.centroid.z;
        result.dog.centroid_distance = std::hypot(dog_offset_x, dog_offset_z);
        if (result.dog.centroid_distance > 0.0) {
            result.dog.bearing_defined = true;
            result.dog.centroid_bearing_radians = std::atan2(dog_offset_x, -dog_offset_z);
            // The push axis points from the dog toward the centroid, because the
            // accepted pressure term pushes a sheep directly away from the dog.
            const double unit_x = -dog_offset_x / result.dog.centroid_distance;
            const double unit_z = -dog_offset_z / result.dog.centroid_distance;
            for (const SheepState& member : sheep) {
                const double offset = (member.position.x - result.centroid.x) * unit_x +
                                      (member.position.z - result.centroid.z) * unit_z;
                if (result.dog.rear_sheep_id == 0 || offset < result.dog.rear_offset ||
                    (offset == result.dog.rear_offset && member.id < result.dog.rear_sheep_id)) {
                    result.dog.rear_sheep_id = member.id;
                    result.dog.rear_offset = offset;
                    result.dog.rear_distance = planar_distance(member.position, *dog_position);
                }
            }
        }
    }

    return result;
}

std::optional<FlockResponseTiming> advance_flock_response_timing(
    const FlockResponseTiming& previous, std::uint64_t tick, const SheepStateBuffer& sheep,
    const std::array<double, kGameplaySheepCount>& arousal_stimulus,
    std::uint32_t connected_component_count, double rest_arousal) noexcept {
    if (!std::isfinite(rest_arousal) || rest_arousal < kSheepMinimumArousal ||
        rest_arousal > kSheepMaximumArousal || connected_component_count == 0 ||
        connected_component_count > sheep.size() || !valid_sheep_buffer(sheep)) {
        return std::nullopt;
    }
    for (const double stimulus : arousal_stimulus) {
        if (!std::isfinite(stimulus) || stimulus < kSheepMinimumArousal ||
            stimulus > kSheepMaximumArousal) {
            return std::nullopt;
        }
    }
    // A fold has to see each tick once and in order, so a repeated or rewound
    // tick is an input error rather than something to average over.
    const bool first_observation = previous.observations == 0;
    if (!first_observation && tick <= previous.tick) {
        return std::nullopt;
    }

    FlockResponseTiming next = previous;
    next.observations = previous.observations + 1;
    next.tick = tick;
    next.connected_component_count = connected_component_count;
    next.split = connected_component_count > 1;
    next.pressure_acting = false;
    next.flock_engaged = false;
    next.flock_settled = true;
    for (const double stimulus : arousal_stimulus) {
        next.pressure_acting = next.pressure_acting || stimulus > rest_arousal;
    }
    for (const SheepState& member : sheep) {
        next.flock_engaged = next.flock_engaged || member.behavior == SheepBehaviorState::alert ||
                             member.behavior == SheepBehaviorState::driven;
        next.flock_settled = next.flock_settled && member.behavior == SheepBehaviorState::settled;
    }

    // Pressure onset and release. The first observation has no earlier tick to
    // compare against, so a press already acting on it is an onset there.
    const bool was_acting = !first_observation && previous.pressure_acting;
    if (next.pressure_acting && !was_acting) {
        next.pressure_episode_open = true;
        next.pressure_onset_tick = tick;
        ++next.pressure_episodes;
        next.response_latency_ticks.reset();
        // A cause that comes back during recovery ends the settle measurement
        // rather than lengthening it: settle time is measured from the *last*
        // release, and this one is no longer the last.
        if (next.settle_pending) {
            next.settle_pending = false;
            ++next.interrupted_settles;
        }
    } else if (!next.pressure_acting && was_acting) {
        if (!next.response_latency_ticks.has_value()) {
            ++next.unanswered_pressure_episodes;
        }
        next.pressure_episode_open = false;
        next.release_tick = tick;
        ++next.releases;
        next.settle_pending = true;
        next.settle_ticks.reset();
    }
    if (next.pressure_episode_open && !next.response_latency_ticks.has_value() &&
        next.flock_engaged) {
        next.response_latency_ticks = tick - next.pressure_onset_tick;
    }

    // Split and rejoin. An episode is "not one component", so a count that
    // climbs again while the flock is already broken deepens this episode
    // instead of restarting the clock that has to answer "how long until the
    // flock was whole again".
    const bool was_split = !first_observation && previous.split;
    if (next.split && !was_split) {
        next.split_episode_open = true;
        next.split_onset_tick = tick;
        ++next.split_episodes;
        next.peak_component_count = connected_component_count;
        next.rejoin_ticks.reset();
        next.time_to_split_ticks.reset();
        if (next.pressure_episode_open) {
            next.time_to_split_ticks = tick - next.pressure_onset_tick;
        }
    } else if (next.split) {
        next.peak_component_count = std::max(next.peak_component_count, connected_component_count);
    } else if (was_split) {
        next.split_episode_open = false;
        next.rejoin_ticks = tick - next.split_onset_tick;
        ++next.rejoins;
    }
    if (next.split) {
        ++next.ticks_split;
    }

    // Settle. Evaluated on the release tick as well, so a flock that never left
    // `settled` settles in zero ticks rather than in the first tick after it.
    if (next.settle_pending && next.flock_settled) {
        next.settle_ticks = tick - next.release_tick;
        next.settle_pending = false;
    }
    return next;
}

} // namespace wide_eye::game
