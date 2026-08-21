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

// `blocking` names the obstacle whose limit actually decided the returned
// value, so a caller can publish which shape stopped the body. It is only ever
// written when a limit tightens the resolved coordinate, which keeps the
// arithmetic identical to the anonymous clamp it replaced.
[[nodiscard]] double move_axis(double start, double desired, double other, double radius,
                               const AnalyticObstacle* obstacles, std::size_t obstacle_count,
                               bool x_axis, PaddockObstacle& blocking) noexcept {
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
            const double limit = obstacle_minimum - radius;
            if (limit < resolved) {
                resolved = limit;
                blocking = obstacle.id;
            }
        } else if (desired < start && start - radius >= obstacle_maximum &&
                   desired - radius < obstacle_maximum) {
            const double limit = obstacle_maximum + radius;
            if (limit > resolved) {
                resolved = limit;
                blocking = obstacle.id;
            }
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

// The parameter range over which one planar axis of a swept point lies inside
// one slab. A zero direction is handled explicitly, exactly as
// `clip_segment_axis` does, so an axis-aligned path never divides by zero: that
// axis either always overlaps the slab or never does.
struct AxisSpan {
    double near_fraction = 0.0;
    double far_fraction = 0.0;
    bool intersects = false;
};

[[nodiscard]] AxisSpan slab_span(double origin, double direction, double minimum,
                                 double maximum) noexcept {
    if (direction == 0.0) {
        if (origin < minimum || origin > maximum) {
            return {};
        }
        return {.near_fraction = -std::numeric_limits<double>::infinity(),
                .far_fraction = std::numeric_limits<double>::infinity(),
                .intersects = true};
    }
    double near_fraction = (minimum - origin) / direction;
    double far_fraction = (maximum - origin) / direction;
    if (near_fraction > far_fraction) {
        std::swap(near_fraction, far_fraction);
    }
    return {.near_fraction = near_fraction, .far_fraction = far_fraction, .intersects = true};
}

// Which way along one obstacle face the nearer free edge lies, and how far away
// it is. A body exactly between the two edges has no nearer one, so the sign is
// zero rather than an invented side. A body already past an edge has no distance
// left to cover, so its clearance is zero rather than negative.
struct LateralEscape {
    double sign = 0.0;
    double clearance = 0.0;
};

[[nodiscard]] LateralEscape nearer_free_edge(double position, double minimum,
                                             double maximum) noexcept {
    const double to_minimum = position - minimum;
    const double to_maximum = maximum - position;
    if (to_minimum < to_maximum) {
        return {.sign = -1.0, .clearance = std::max(to_minimum, 0.0)};
    }
    if (to_maximum < to_minimum) {
        return {.sign = 1.0, .clearance = std::max(to_maximum, 0.0)};
    }
    return {.sign = 0.0, .clearance = std::max(to_minimum, 0.0)};
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

CylinderMoveResult PaddockCollisionField::resolve_cylinder_move(Vec3 start, Vec3 displacement,
                                                                double radius) const noexcept {
    CylinderMoveResult result{.position = start};
    if (!std::isfinite(start.x) || !std::isfinite(start.z) || !std::isfinite(displacement.x) ||
        !std::isfinite(displacement.z) || !std::isfinite(radius) || radius <= 0.0) {
        // A rejected move is not a contact: the body keeps its position and the
        // result reports no obstacle rather than inventing one.
        return result;
    }

    PaddockObstacle blocking_x = PaddockObstacle::none;
    const double requested_x = start.x + displacement.x;
    const double desired_x = std::clamp(requested_x, kMinimumX + radius, kMaximumX - radius);
    result.position.x = move_axis(start.x, desired_x, start.z, radius, obstacles_.data(),
                                  obstacle_count_, true, blocking_x);

    PaddockObstacle blocking_z = PaddockObstacle::none;
    const double requested_z = start.z + displacement.z;
    const double desired_z = std::clamp(requested_z, kMinimumZ + radius, kMaximumZ - radius);
    result.position.z = move_axis(start.z, desired_z, result.position.x, radius, obstacles_.data(),
                                  obstacle_count_, false, blocking_z);
    result.position.y = ground_height(result.position.x, result.position.z);

    // An unobstructed axis reproduces the requested coordinate exactly, so the
    // comparison needs no tolerance: any difference is a clip.
    result.clipped_x = result.position.x != requested_x;
    result.clipped_z = result.position.z != requested_z;
    // The X pass resolves first, so it names the obstacle when the two axes are
    // stopped by different shapes. An axis stopped only by the paddock's outer
    // bounds leaves `none` standing, because the bounds are not obstacles.
    result.obstacle = blocking_x != PaddockObstacle::none ? blocking_x : blocking_z;
    return result;
}

Vec3 PaddockCollisionField::move_cylinder(Vec3 start, Vec3 displacement,
                                          double radius) const noexcept {
    return resolve_cylinder_move(start, displacement, radius).position;
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

ObstacleApproach PaddockCollisionField::approaching_obstacle(Vec3 start, Vec3 direction,
                                                             double distance,
                                                             double radius) const noexcept {
    ObstacleApproach result;
    const double direction_length = std::hypot(direction.x, direction.z);
    if (!std::isfinite(start.x) || !std::isfinite(start.z) || !std::isfinite(direction_length) ||
        direction_length <= 0.0 || !std::isfinite(distance) || distance <= 0.0 ||
        !std::isfinite(radius) || radius < 0.0) {
        // A path with no length, no direction, or no finite geometry is not a
        // clear path: it is a question the field cannot answer, so it reports no
        // obstacle instead of an unfounded all-clear.
        return result;
    }

    const double unit_x = direction.x / direction_length;
    const double unit_z = direction.z / direction_length;
    for (std::size_t index = 0; index < obstacle_count_; ++index) {
        const AnalyticObstacle& obstacle = obstacles_[index];
        const double minimum_x = obstacle.minimum_x - radius;
        const double maximum_x = obstacle.maximum_x + radius;
        const double minimum_z = obstacle.minimum_z - radius;
        const double maximum_z = obstacle.maximum_z + radius;
        const AxisSpan span_x = slab_span(start.x, unit_x, minimum_x, maximum_x);
        const AxisSpan span_z = slab_span(start.z, unit_z, minimum_z, maximum_z);
        if (!span_x.intersects || !span_z.intersects) {
            continue;
        }

        const double entry = std::max(span_x.near_fraction, span_z.near_fraction);
        const double exit = std::min(span_x.far_fraction, span_z.far_fraction);
        if (entry > exit || exit < 0.0 || entry > distance) {
            continue;
        }
        // A body whose swept rectangle already overlaps this obstacle reaches it
        // at zero distance rather than at a negative one. Pushing that body out
        // is a steering caller's decision; this query only reports the geometry.
        const double contact = std::max(entry, 0.0);
        if (result.obstacle != PaddockObstacle::none && contact >= result.contact_distance) {
            continue;
        }

        result.obstacle = obstacle.id;
        result.contact_distance = contact;
        // The axis whose slab is entered last is the face the body meets, and
        // the X pass wins an exact corner tie — the same ordering
        // `resolve_cylinder_move` already uses when two axes disagree. A zero
        // direction component leaves that axis at negative infinity, so it can
        // never be the entered face.
        if (span_x.near_fraction >= span_z.near_fraction) {
            const LateralEscape escape = nearer_free_edge(start.z, minimum_z, maximum_z);
            result.face_normal = {.x = unit_x > 0.0 ? -1.0 : 1.0};
            result.lateral_escape = {.z = escape.sign};
            result.lateral_clearance = escape.clearance;
        } else {
            const LateralEscape escape = nearer_free_edge(start.x, minimum_x, maximum_x);
            result.face_normal = {.z = unit_z > 0.0 ? -1.0 : 1.0};
            result.lateral_escape = {.x = escape.sign};
            result.lateral_clearance = escape.clearance;
        }
    }
    return result;
}

bool PaddockCollisionField::gate_open() const noexcept {
    return gate_open_;
}

std::size_t PaddockCollisionField::obstacle_count() const noexcept {
    return obstacle_count_;
}

} // namespace wide_eye::game
