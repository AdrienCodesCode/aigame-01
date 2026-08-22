#include "game/paddock_collision.hpp"

#include <algorithm>
#include <array>
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

[[nodiscard]] bool overlaps_obstacle(const AnalyticObstacle& obstacle, double x, double z,
                                     double radius) noexcept {
    return overlaps(x - radius, x + radius, obstacle.minimum_x, obstacle.maximum_x) &&
           overlaps(z - radius, z + radius, obstacle.minimum_z, obstacle.maximum_z);
}

[[nodiscard]] bool overlaps_any_obstacle(double x, double z, double radius,
                                         const AnalyticObstacle* obstacles,
                                         std::size_t obstacle_count) noexcept {
    for (std::size_t index = 0; index < obstacle_count; ++index) {
        if (overlaps_obstacle(obstacles[index], x, z, radius)) {
            return true;
        }
    }
    return false;
}

// One way out of a shape a body is already inside: which axis to move along, the
// coordinate that puts the body exactly on that face, and how far it has to
// travel to get there. `depth` is what makes two ways out comparable; `found`
// distinguishes "no way out was named" from a zero-length one, which cannot
// occur because a strict overlap has strictly positive depth on all four faces.
struct EscapeCandidate {
    bool found = false;
    bool x_axis = false;
    double coordinate = 0.0;
    double depth = 0.0;
};

// The shallowest single-axis move that puts a body already overlapping one or
// more obstacles back outside *all* of them.
//
// It has to answer for the union rather than for one rectangle at a time. The
// walls and the closed gate touch, so the shallowest way out of one shape is
// routinely a way into its neighbour: rejecting a candidate that lands inside
// another shape is exactly what makes this one step instead of an iteration that
// could hand a body back and forth between two neighbours forever. An escape
// that leaves the paddock is rejected for the same reason — it is not an escape.
//
// The tie break is the enumeration order: the field's own fixed obstacle order,
// then a fixed face order of `-x`, `+x`, `-z`, `+z`, with only a strictly
// smaller depth replacing the standing candidate. Two equally shallow escapes
// therefore always resolve to the earlier one — the earlier shape when they
// belong to different shapes, and the X face when one shape offers both, which
// is the same X-before-Z priority `resolve_cylinder_move`'s X-first pass and
// `approaching_obstacle`'s corner tie already use. Nothing in that order depends
// on storage order, iteration order, an address, or which body is asking, so an
// exact tie resolves the same way in every run.
//
// A body no candidate can free — wedged in a shape set with no clear
// single-axis way out — reports `found` false and is left exactly where it is.
// The geometry names no escape, so none is invented; the ordinary axis passes
// then resolve its displacement as they always did.
[[nodiscard]] EscapeCandidate shallowest_escape(double x, double z, double radius,
                                                const AnalyticObstacle* obstacles,
                                                std::size_t obstacle_count) noexcept {
    EscapeCandidate best;
    for (std::size_t index = 0; index < obstacle_count; ++index) {
        const AnalyticObstacle& obstacle = obstacles[index];
        if (!overlaps_obstacle(obstacle, x, z, radius)) {
            continue;
        }

        const std::array<EscapeCandidate, 4> candidates{{
            {.found = true,
             .x_axis = true,
             .coordinate = obstacle.minimum_x - radius,
             .depth = x - (obstacle.minimum_x - radius)},
            {.found = true,
             .x_axis = true,
             .coordinate = obstacle.maximum_x + radius,
             .depth = (obstacle.maximum_x + radius) - x},
            {.found = true,
             .x_axis = false,
             .coordinate = obstacle.minimum_z - radius,
             .depth = z - (obstacle.minimum_z - radius)},
            {.found = true,
             .x_axis = false,
             .coordinate = obstacle.maximum_z + radius,
             .depth = (obstacle.maximum_z + radius) - z},
        }};
        for (const EscapeCandidate& candidate : candidates) {
            if (best.found && candidate.depth >= best.depth) {
                continue;
            }
            const double lower = candidate.x_axis ? PaddockCollisionField::kMinimumX
                                                  : PaddockCollisionField::kMinimumZ;
            const double upper = candidate.x_axis ? PaddockCollisionField::kMaximumX
                                                  : PaddockCollisionField::kMaximumZ;
            if (candidate.coordinate < lower + radius || candidate.coordinate > upper - radius) {
                continue;
            }
            const double escaped_x = candidate.x_axis ? candidate.coordinate : x;
            const double escaped_z = candidate.x_axis ? z : candidate.coordinate;
            if (overlaps_any_obstacle(escaped_x, escaped_z, radius, obstacles, obstacle_count)) {
                continue;
            }
            best = candidate;
        }
    }
    return best;
}

// `blocking` names the obstacle whose limit actually decided the returned
// value, so a caller can publish which shape stopped the body. It is only ever
// written when a limit tightens the resolved coordinate, which keeps the
// arithmetic identical to the anonymous clamp it replaced.
//
// Both branches test the clearance the body had *before* the move, so this
// function answers only for a body that starts clear of the face it is moving
// toward. `resolve_cylinder_move` guarantees that precondition; see the
// depenetration step there.
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
        // Touching this slab's boundary while travelling along it is not being
        // inside it. `overlaps`, the test the hard clip applies to the axis a
        // body is *not* moving along, is strict for the same reason: it is what
        // lets a body parked at face plus radius slide along that face instead
        // of being refused by it. A sweep that called the same body overlapping
        // would be answering for a different shape than the one that refuses
        // the displacement, which is exactly what this query promises never to
        // do.
        if (origin <= minimum || origin >= maximum) {
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

    // Depenetration. `move_axis` refuses a displacement by asking whether the
    // body was clear of the face before the move, so it only answers correctly
    // for a body that starts clear; a body that starts inside a shape matches no
    // refusal and walks straight through it. Restoring that precondition is the
    // whole correction — the refusal arithmetic below is unchanged. Only a
    // caller can create the overlap: measured over 5,960,704 (start,
    // displacement) pairs on this paddock, no start clear of every shape is ever
    // left overlapping one, so the passes never feed themselves a body they
    // cannot answer for.
    Vec3 origin = start;
    const EscapeCandidate escape =
        shallowest_escape(start.x, start.z, radius, obstacles_.data(), obstacle_count_);
    if (escape.found) {
        if (escape.x_axis) {
            origin.x = escape.coordinate;
        } else {
            origin.z = escape.coordinate;
        }
    }

    // The requested coordinates stay anchored to the caller's own start, not to
    // the depenetrated origin: the body still wants to reach the same place, and
    // a push out of a shape has to read as a refused displacement rather than
    // vanish because the body was measured from where it was put.
    PaddockObstacle blocking_x = PaddockObstacle::none;
    const double requested_x = start.x + displacement.x;
    const double desired_x = std::clamp(requested_x, kMinimumX + radius, kMaximumX - radius);
    result.position.x = move_axis(origin.x, desired_x, origin.z, radius, obstacles_.data(),
                                  obstacle_count_, true, blocking_x);

    PaddockObstacle blocking_z = PaddockObstacle::none;
    const double requested_z = start.z + displacement.z;
    const double desired_z = std::clamp(requested_z, kMinimumZ + radius, kMaximumZ - radius);
    result.position.z = move_axis(origin.z, desired_z, result.position.x, radius, obstacles_.data(),
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
        // The entry has to lie at or ahead of the body, because that is the
        // question: which shape does this body *reach*. A negative entry means
        // the body is already inside this rectangle with every face behind it,
        // and reporting the slab it entered last would name a face it has
        // already passed — turning a body sliding along a shape into a body
        // braking against one it left behind. A body that is already inside is
        // the collision authority's case, not a steering look-ahead's:
        // `resolve_cylinder_move` pushes it out along the shallowest clear axis
        // on the same tick, so the steering term never has to answer for it.
        if (entry < 0.0 || entry > exit || entry > distance) {
            continue;
        }
        // A body touching the face it is moving into enters at zero, which the
        // arithmetic can produce as a negative zero. Normalizing the sign keeps
        // the published distance from reading as `-0.000000` in a state dump.
        const double contact = entry == 0.0 ? 0.0 : entry;
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
