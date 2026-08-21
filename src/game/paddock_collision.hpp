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
    [[nodiscard]] Vec3 move_cylinder(Vec3 start, Vec3 displacement, double radius) const noexcept;
    // Reports the first obstacle in the paddock's fixed order that the planar
    // segment touches, or `none` when the segment is clear. The test is a
    // zero-width sight line: a segment that exactly grazes an obstacle edge
    // counts as blocked, and a non-finite endpoint reports no obstacle rather
    // than inventing one.
    [[nodiscard]] PaddockObstacle blocking_obstacle(double from_x, double from_z, double to_x,
                                                    double to_z) const noexcept;
    [[nodiscard]] bool gate_open() const noexcept;
    [[nodiscard]] std::size_t obstacle_count() const noexcept;

  private:
    std::array<AnalyticObstacle, 3> obstacles_{};
    std::size_t obstacle_count_ = 0;
    bool gate_open_ = false;
};

} // namespace wide_eye::game
