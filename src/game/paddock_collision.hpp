#pragma once

#include "game/math.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace wide_eye::game {

// Names each analytic paddock shape so a collision or occlusion result can say
// which one produced it instead of publishing an anonymous boolean.
enum class PaddockObstacle : std::uint8_t {
    none,
    left_wall,
    right_wall,
    gate,
};

[[nodiscard]] constexpr bool is_known_paddock_obstacle(PaddockObstacle obstacle) noexcept {
    switch (obstacle) {
    case PaddockObstacle::none:
    case PaddockObstacle::left_wall:
    case PaddockObstacle::right_wall:
    case PaddockObstacle::gate:
        return true;
    }
    return false;
}

struct AnalyticObstacle {
    PaddockObstacle id = PaddockObstacle::none;
    double minimum_x = 0.0;
    double maximum_x = 0.0;
    double minimum_z = 0.0;
    double maximum_z = 0.0;
};

// Records what one analytic move actually did, so a body that stops can be
// explained instead of halting mysteriously. `clipped_x` and `clipped_z` name
// the axes whose requested displacement the field refused this tick, and
// `obstacle` names the shape that refused it. A clipped axis with `none` was
// refused by the paddock's outer bounds, which are limits rather than obstacle
// shapes.
struct CylinderMoveResult {
    Vec3 position{};
    bool clipped_x = false;
    bool clipped_z = false;
    PaddockObstacle obstacle = PaddockObstacle::none;
};

// Read-only description of the first analytic obstacle a moving body would
// reach along one straight planar path. A steering rule needs this *before* the
// collision authority has to refuse a displacement, and it needs more than an
// identity to act on: `face_normal` is the outward unit normal of the face the
// body would enter, so a rule can push away from the shape, and
// `lateral_escape` is the unit direction along that face toward the nearer free
// edge, so a rule can steer around the shape instead of only braking against it.
// `lateral_clearance` is how far that edge is, so a caller can decide whether a
// way round is within reach. `lateral_escape` is zero when the two edges are
// exactly equidistant, because neither is nearer and choosing a side would be a
// hidden tie break. A clear path reports `none` and leaves every field zero.
struct ObstacleApproach {
    PaddockObstacle obstacle = PaddockObstacle::none;
    double contact_distance = 0.0;
    Vec3 face_normal{};
    Vec3 lateral_escape{};
    double lateral_clearance = 0.0;
};

// Collision and occlusion truth for the handcrafted paddock. These analytic
// bounds are intentionally independent of voxel faces and renderer mesh
// topology. The field is owned by `game` rather than by the dog motor because
// sheep rules need the same paddock shapes; a dog-named header would make sheep
// behavior depend on a controller boundary.
class PaddockCollisionField {
  public:
    static constexpr double kMinimumX = 0.0;
    static constexpr double kMaximumX = 32.0;
    static constexpr double kMinimumZ = 0.0;
    static constexpr double kMaximumZ = 32.0;
    static constexpr double kGroundHeight = 1.0;

    explicit PaddockCollisionField(bool gate_open = false) noexcept;

    [[nodiscard]] double ground_height(double x, double z) const noexcept;
    // Resolves one planar displacement of an upright cylinder and reports the
    // contact that produced the result. Callers that only need the position use
    // `move_cylinder`; callers that must publish why a body stopped use this.
    [[nodiscard]] CylinderMoveResult resolve_cylinder_move(Vec3 start, Vec3 displacement,
                                                           double radius) const noexcept;
    [[nodiscard]] Vec3 move_cylinder(Vec3 start, Vec3 displacement, double radius) const noexcept;
    // Reports the first obstacle in the paddock's fixed order that the planar
    // segment touches, or `none` when the segment is clear. The test is a
    // zero-width sight line: a segment that exactly grazes an obstacle edge
    // counts as blocked, and a non-finite endpoint reports no obstacle rather
    // than inventing one.
    [[nodiscard]] PaddockObstacle blocking_obstacle(double from_x, double from_z, double to_x,
                                                    double to_z) const noexcept;
    // Reports the first obstacle a body of `radius` would reach while
    // travelling from `start` along `direction` for `distance`, sweeping the
    // same radius-expanded rectangles `resolve_cylinder_move` stops it against,
    // so the shape a rule steers around and the shape that refuses a
    // displacement can never be two different shapes. The nearest obstacle
    // wins and the paddock's fixed order breaks an exact tie, as it does for
    // the sight line. `direction` need not be normalized; a zero, non-finite,
    // or zero-length path reports no obstacle rather than inventing one.
    // "Would reach" is literal: the reported contact always lies at or ahead of
    // `start`, so a body travelling along a face it already touches, or already
    // inside a shape's expanded rectangle, reports no obstacle rather than a
    // contact at distance zero against a face behind it. Touching a rectangle's
    // boundary while travelling along it is not being inside it, exactly as the
    // hard clip's own overlap test is strict.
    [[nodiscard]] ObstacleApproach approaching_obstacle(Vec3 start, Vec3 direction, double distance,
                                                        double radius) const noexcept;
    [[nodiscard]] bool gate_open() const noexcept;
    [[nodiscard]] std::size_t obstacle_count() const noexcept;

  private:
    std::array<AnalyticObstacle, 3> obstacles_{};
    std::size_t obstacle_count_ = 0;
    bool gate_open_ = false;
};

} // namespace wide_eye::game
