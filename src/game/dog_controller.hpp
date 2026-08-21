#pragma once

#include "game/math.hpp"
#include "game/paddock_collision.hpp"

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

    bool operator==(const DogControllerConfiguration&) const = default;
};

[[nodiscard]] DogState interpolate_dog_state(const DogState& previous, const DogState& current,
                                             double alpha) noexcept;

class DogController {
  public:
    static constexpr double kRadius = 0.42;
    static constexpr double kWalkSpeed = 4.5;
    static constexpr double kSprintSpeed = 8.0;
    static constexpr double kAcceleration = 22.0;
    static constexpr double kDeceleration = 30.0;
    static constexpr double kTurnRateRadiansPerSecond = 6.0;

    // Paddock gate state is world state owned by the scenario, so the motor
    // receives it rather than storing it in its own configuration.
    DogController(DogControllerConfiguration configuration, bool gate_open) noexcept;

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
