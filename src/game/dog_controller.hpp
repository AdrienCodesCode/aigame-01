#pragma once

#include "game/math.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace wide_eye::game {

struct DogState {
    Vec3 position{};
    Vec3 velocity{};
    double heading_radians = 0.0;
    bool grounded = false;

    bool operator==(const DogState&) const = default;
};

struct DogMoveInput {
    // Normalized desired movement in world-space X/Z. Input mapping belongs to
    // gameplay orchestration, not to the motor or collision layer.
    double world_x = 0.0;
    double world_z = 0.0;
    bool sprint = false;
};

struct DogControllerConfiguration {
    DogState initial_state{};
    bool gate_open = false;
};

[[nodiscard]] DogState interpolate_dog_state(const DogState& previous, const DogState& current,
                                             double alpha) noexcept;

struct AnalyticObstacle {
    double minimum_x = 0.0;
    double maximum_x = 0.0;
    double minimum_z = 0.0;
    double maximum_z = 0.0;
};

// Collision truth for the handcrafted paddock. These analytic bounds are
// intentionally independent of voxel faces and renderer mesh topology.
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
    [[nodiscard]] bool gate_open() const noexcept;
    [[nodiscard]] std::size_t obstacle_count() const noexcept;

  private:
    std::array<AnalyticObstacle, 3> obstacles_{};
    std::size_t obstacle_count_ = 0;
    bool gate_open_ = false;
};

class DogController {
  public:
    static constexpr double kRadius = 0.42;
    static constexpr double kWalkSpeed = 4.5;
    static constexpr double kSprintSpeed = 8.0;
    static constexpr double kAcceleration = 22.0;
    static constexpr double kDeceleration = 30.0;
    static constexpr double kTurnRateRadiansPerSecond = 6.0;

    explicit DogController(DogControllerConfiguration configuration) noexcept;

    void fixed_update(const DogMoveInput& input, double fixed_delta_seconds) noexcept;
    void restart() noexcept;

    [[nodiscard]] const DogState& state() const noexcept;
    [[nodiscard]] std::uint64_t tick() const noexcept;
    [[nodiscard]] std::uint32_t restart_count() const noexcept;

  private:
    DogControllerConfiguration configuration_;
    PaddockCollisionField collision_;
    DogState state_{};
    std::uint64_t tick_ = 0;
    std::uint32_t restart_count_ = 0;
};

} // namespace wide_eye::game
