#pragma once

#include "game/dog_controller.hpp"

#include <cstdint>

namespace wide_eye::game {

enum class CameraMode : std::uint8_t {
    gameplay,
    free_debug,
};

struct CameraPose {
    Vec3 eye{};
    Vec3 target{};
};

struct CameraControlInput {
    double move_right = 0.0;
    double move_forward = 0.0;
    double rise = 0.0;
    double look_right_rate = 0.0;
    double look_up_rate = 0.0;
    double look_right_delta = 0.0;
    double look_up_delta = 0.0;
    bool toggle_mode = false;
};

struct CameraState {
    CameraMode mode = CameraMode::gameplay;
    double gameplay_yaw = 0.0;
    double gameplay_pitch = -0.55;
    Vec3 free_eye{};
    double free_yaw = 0.0;
    double free_pitch = 0.0;

    bool operator==(const CameraState&) const = default;
};

[[nodiscard]] Vec3 resolve_camera_relative_move(double gameplay_yaw, double move_right,
                                                double move_forward) noexcept;
[[nodiscard]] CameraState interpolate_camera_state(const CameraState& previous,
                                                   const CameraState& current,
                                                   double alpha) noexcept;
[[nodiscard]] CameraPose camera_pose(const DogState& dog, const CameraState& camera) noexcept;

class CameraController {
  public:
    static constexpr double kMouseRadiansPerPixel = 0.003;
    static constexpr double kStickLookRadiansPerSecond = 1.8;
    static constexpr double kMinimumGameplayPitch = -1.25;
    static constexpr double kMaximumGameplayPitch = 0.25;

    explicit CameraController(const DogState& dog) noexcept;

    void fixed_update(const DogState& dog, const CameraControlInput& input,
                      double fixed_delta_seconds) noexcept;
    void restart(const DogState& dog) noexcept;

    [[nodiscard]] CameraMode mode() const noexcept;
    [[nodiscard]] const CameraState& state() const noexcept;
    [[nodiscard]] CameraPose pose(const DogState& dog) const noexcept;

  private:
    void reset_free_camera() noexcept;

    CameraState state_{};
};

} // namespace wide_eye::game
