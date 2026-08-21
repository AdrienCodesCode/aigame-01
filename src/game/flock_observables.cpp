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

} // namespace

std::optional<FiveSheepObservables> compute_five_sheep_observables(
    const SheepStateBuffer& sheep,
    const std::array<std::uint32_t, kGameplaySheepCount>& chosen_neighbor_counts,
    double connectivity_distance) noexcept {
    if (!std::isfinite(connectivity_distance) || connectivity_distance < 0.0) {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        if (!valid_state(sheep[index]) || chosen_neighbor_counts[index] >= sheep.size()) {
            return std::nullopt;
        }
        for (std::size_t prior = 0; prior < index; ++prior) {
            if (sheep[index].id == sheep[prior].id) {
                return std::nullopt;
            }
        }
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
    return result;
}

} // namespace wide_eye::game
