#include "game/paddock_collision.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace wide_eye::game {
namespace {

constexpr AnalyticObstacle kLeftWall{
    .id = PaddockObstacle::left_wall,
    .minimum_x = 1.0,
    .maximum_x = 14.0,
    .minimum_z = 14.0,
    .maximum_z = 16.0,
};
constexpr AnalyticObstacle kRightWall{
    .id = PaddockObstacle::right_wall,
    .minimum_x = 18.0,
    .maximum_x = 31.0,
    .minimum_z = 14.0,
    .maximum_z = 16.0,
};
constexpr AnalyticObstacle kClosedGate{
    .id = PaddockObstacle::gate,
    .minimum_x = 14.0,
    .maximum_x = 18.0,
    .minimum_z = 15.0,
    .maximum_z = 16.0,
};

[[nodiscard]] bool overlaps(double minimum_a, double maximum_a, double minimum_b,
                            double maximum_b) noexcept {
    return maximum_a > minimum_b && minimum_a < maximum_b;
}

[[nodiscard]] double move_axis(double start, double desired, double other, double radius,
                               const AnalyticObstacle* obstacles, std::size_t obstacle_count,
                               bool x_axis) noexcept {
    double resolved = desired;
    for (std::size_t index = 0; index < obstacle_count; ++index) {
        const AnalyticObstacle& obstacle = obstacles[index];
        const double other_minimum = x_axis ? obstacle.minimum_z : obstacle.minimum_x;
        const double other_maximum = x_axis ? obstacle.maximum_z : obstacle.maximum_x;
        if (!overlaps(other - radius, other + radius, other_minimum, other_maximum)) {
            continue;
        }

        const double obstacle_minimum = x_axis ? obstacle.minimum_x : obstacle.minimum_z;
        const double obstacle_maximum = x_axis ? obstacle.maximum_x : obstacle.maximum_z;
        if (desired > start && start + radius <= obstacle_minimum &&
            desired + radius > obstacle_minimum) {
            resolved = std::min(resolved, obstacle_minimum - radius);
        } else if (desired < start && start - radius >= obstacle_maximum &&
                   desired - radius < obstacle_maximum) {
            resolved = std::max(resolved, obstacle_maximum + radius);
        }
    }
    return resolved;
}

// Clips the parameter range of one planar segment against one axis slab. A zero
// direction is handled explicitly so an axis-aligned sight line never divides by
// zero and never needs a tuned epsilon.
[[nodiscard]] bool clip_segment_axis(double origin, double direction, double minimum,
                                     double maximum, double& entry, double& exit) noexcept {
    if (direction == 0.0) {
        return origin >= minimum && origin <= maximum;
    }
    double near_fraction = (minimum - origin) / direction;
    double far_fraction = (maximum - origin) / direction;
    if (near_fraction > far_fraction) {
        std::swap(near_fraction, far_fraction);
    }
    entry = std::max(entry, near_fraction);
    exit = std::min(exit, far_fraction);
    return entry <= exit;
}

[[nodiscard]] bool segment_touches(const AnalyticObstacle& obstacle, double from_x, double from_z,
                                   double to_x, double to_z) noexcept {
    double entry = 0.0;
    double exit = 1.0;
    return clip_segment_axis(from_x, to_x - from_x, obstacle.minimum_x, obstacle.maximum_x, entry,
                             exit) &&
           clip_segment_axis(from_z, to_z - from_z, obstacle.minimum_z, obstacle.maximum_z, entry,
                             exit);
}

} // namespace

PaddockCollisionField::PaddockCollisionField(bool gate_open) noexcept : gate_open_{gate_open} {
    obstacles_[0] = kLeftWall;
    obstacles_[1] = kRightWall;
    obstacle_count_ = 2;
    if (!gate_open_) {
        obstacles_[obstacle_count_] = kClosedGate;
        ++obstacle_count_;
    }
}

double PaddockCollisionField::ground_height(double x, double z) const noexcept {
    if (!std::isfinite(x) || !std::isfinite(z) || x < kMinimumX || x > kMaximumX || z < kMinimumZ ||
        z > kMaximumZ) {
        return -std::numeric_limits<double>::infinity();
    }
    return kGroundHeight;
}

Vec3 PaddockCollisionField::move_cylinder(Vec3 start, Vec3 displacement,
                                          double radius) const noexcept {
    if (!std::isfinite(start.x) || !std::isfinite(start.z) || !std::isfinite(displacement.x) ||
        !std::isfinite(displacement.z) || !std::isfinite(radius) || radius <= 0.0) {
        return start;
    }

    Vec3 resolved = start;
    const double desired_x =
        std::clamp(start.x + displacement.x, kMinimumX + radius, kMaximumX - radius);
    resolved.x =
        move_axis(start.x, desired_x, start.z, radius, obstacles_.data(), obstacle_count_, true);

    const double desired_z =
        std::clamp(start.z + displacement.z, kMinimumZ + radius, kMaximumZ - radius);
    resolved.z = move_axis(start.z, desired_z, resolved.x, radius, obstacles_.data(),
                           obstacle_count_, false);
    resolved.y = ground_height(resolved.x, resolved.z);
    return resolved;
}

PaddockObstacle PaddockCollisionField::blocking_obstacle(double from_x, double from_z, double to_x,
                                                         double to_z) const noexcept {
    if (!std::isfinite(from_x) || !std::isfinite(from_z) || !std::isfinite(to_x) ||
        !std::isfinite(to_z)) {
        return PaddockObstacle::none;
    }

    for (std::size_t index = 0; index < obstacle_count_; ++index) {
        if (segment_touches(obstacles_[index], from_x, from_z, to_x, to_z)) {
            return obstacles_[index].id;
        }
    }
    return PaddockObstacle::none;
}

bool PaddockCollisionField::gate_open() const noexcept {
    return gate_open_;
}

std::size_t PaddockCollisionField::obstacle_count() const noexcept {
    return obstacle_count_;
}

} // namespace wide_eye::game
