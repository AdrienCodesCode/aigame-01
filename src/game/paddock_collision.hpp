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
// the axes whose requested coordinate the field refused this tick, and
// `obstacle` names the shape that refused it. A clipped axis with `none` was
// refused by the paddock's outer bounds, which are limits rather than obstacle
// shapes.
//
// Being pushed out of a shape the body started inside is reported through those
// same two flags rather than through a signal of its own, because it is the same
// event they already describe: the requested coordinate was inside an obstacle
// and the field refused it. The shape the body was inside is the shape that
// names itself in `obstacle`, and a caller that clears a clipped axis' velocity
// wants to clear it here too — a body being pushed out of a wall should not keep
// the speed it had into the wall. A separate flag would also be a contract
// member carried forever for an event only a badly placed fixture can produce:
// the field never leaves a body that started clear overlapping anything.
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
// way round is within reach. `lateral_escape` is zero when the two reachable
// edges are exactly equidistant, or when neither edge can be passed while the
// body's centre stays inside the paddock, because neither case names a usable
// side. A clear path reports `none` and leaves every field zero.
struct ObstacleApproach {
    PaddockObstacle obstacle = PaddockObstacle::none;
    double contact_distance = 0.0;
    Vec3 face_normal{};
    Vec3 lateral_escape{};
    double lateral_clearance = 0.0;
};

// Read-only description of the first point where a straight planar path leaves
// the field's finite ground. `face_normal` points back into grounded space, so a
// steering rule can remove only the component approaching that boundary instead
// of reversing the whole path. The current handcrafted ground is one rectangle;
// keeping that knowledge here prevents sheep rules from hard-coding its bounds.
// A path that stays grounded for the requested distance leaves every field zero.
struct GroundBoundaryApproach {
    bool boundary_ahead = false;
    double contact_distance = 0.0;
    Vec3 face_normal{};
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
    [[nodiscard]] GroundBoundaryApproach
    approaching_ground_boundary(Vec3 start, Vec3 direction, double distance) const noexcept;
    // Resolves one planar displacement of an upright cylinder and reports the
    // contact that produced the result. Callers that only need the position use
    // `move_cylinder`; callers that must publish why a body stopped use this.
    //
    // A body that starts overlapping an obstacle is pushed out first, along the
    // shallowest single-axis move that clears every shape at once and stays
    // inside the paddock, before its displacement is resolved. Two equally
    // shallow ways out resolve to the earlier one in the field's fixed obstacle
    // order and a fixed `-x`, `+x`, `-z`, `+z` face order, so an exact tie
    // answers the same way in every run; a body with no clear way out is left
    // where it is rather than given an invented one.
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
