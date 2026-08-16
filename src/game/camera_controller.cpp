#include "game/camera_controller.hpp"

#include <algorithm>
#include <cmath>

namespace wide_eye::game {
namespace {

constexpr Vec3 kFreeCameraEye{.x = 40.0, .y = 24.0, .z = 46.0};
constexpr Vec3 kFreeCameraTarget{.x = 15.5, .y = 2.5, .z = 14.5};
constexpr double kHalfPi = 1.57079632679489661923;
constexpr double kTwoPi = 6.28318530717958647692;

[[nodiscard]] Vec3 direction_from_angles(double yaw, double pitch) noexcept {
    const double horizontal = std::cos(pitch);
    return {
        .x = std::sin(yaw) * horizontal,
        .y = std::sin(pitch),
        .z = -std::cos(yaw) * horizontal,
    };
}

[[nodiscard]] double finite_unit_input(double value) noexcept {
    return std::isfinite(value) ? std::clamp(value, -1.0, 1.0) : 0.0;
}

[[nodiscard]] double finite_delta(double value) noexcept {
    return std::isfinite(value) ? value : 0.0;
}

[[nodiscard]] double interpolate_angle(double previous, double current, double alpha) noexcept {
    return std::remainder(previous + std::remainder(current - previous, kTwoPi) * alpha, kTwoPi);
}

} // namespace

Vec3 resolve_camera_relative_move(double gameplay_yaw, double move_right,
                                  double move_forward) noexcept {
    const double yaw = std::isfinite(gameplay_yaw) ? gameplay_yaw : 0.0;
    const Vec3 forward = direction_from_angles(yaw, 0.0);
    const Vec3 right{.x = -forward.z, .y = 0.0, .z = forward.x};
    const double right_input = finite_unit_input(move_right);
    const double forward_input = finite_unit_input(move_forward);
    Vec3 result{
        .x = right.x * right_input + forward.x * forward_input,
        .y = 0.0,
        .z = right.z * right_input + forward.z * forward_input,
    };
    const double magnitude = std::hypot(result.x, result.z);
    if (magnitude > 1.0) {
        result.x /= magnitude;
        result.z /= magnitude;
    }
    return result;
}

CameraState interpolate_camera_state(const CameraState& previous, const CameraState& current,
                                     double alpha) noexcept {
    if (previous.mode != current.mode) {
        return current;
    }
    const double bounded_alpha = std::isfinite(alpha) ? std::clamp(alpha, 0.0, 1.0) : 1.0;
    const auto interpolate = [bounded_alpha](double start, double end) {
        return start + (end - start) * bounded_alpha;
    };
    return {
        .mode = current.mode,
        .gameplay_yaw =
            interpolate_angle(previous.gameplay_yaw, current.gameplay_yaw, bounded_alpha),
        .gameplay_pitch = interpolate(previous.gameplay_pitch, current.gameplay_pitch),
        .free_eye = {.x = interpolate(previous.free_eye.x, current.free_eye.x),
                     .y = interpolate(previous.free_eye.y, current.free_eye.y),
                     .z = interpolate(previous.free_eye.z, current.free_eye.z)},
        .free_yaw = interpolate_angle(previous.free_yaw, current.free_yaw, bounded_alpha),
        .free_pitch = interpolate(previous.free_pitch, current.free_pitch),
    };
}

CameraPose camera_pose(const DogState& dog, const CameraState& camera) noexcept {
    if (camera.mode == CameraMode::free_debug) {
        const Vec3 direction = direction_from_angles(camera.free_yaw, camera.free_pitch);
        return {
            .eye = camera.free_eye,
            .target = {.x = camera.free_eye.x + direction.x,
                       .y = camera.free_eye.y + direction.y,
                       .z = camera.free_eye.z + direction.z},
        };
    }

    constexpr double kTargetHeight = 1.4;
    constexpr double kOrbitDistance = 12.0;
    const Vec3 direction = direction_from_angles(camera.gameplay_yaw, camera.gameplay_pitch);
    const Vec3 target{
        .x = dog.position.x, .y = dog.position.y + kTargetHeight, .z = dog.position.z};
    return {
        .eye = {.x = target.x - direction.x * kOrbitDistance,
                .y = target.y - direction.y * kOrbitDistance,
                .z = target.z - direction.z * kOrbitDistance},
        .target = target,
    };
}

CameraController::CameraController(const DogState& dog) noexcept {
    state_.gameplay_yaw = dog.heading_radians;
    reset_free_camera();
}

void CameraController::fixed_update(const DogState&, const CameraControlInput& input,
                                    double fixed_delta_seconds) noexcept {
    if (input.toggle_mode) {
        state_.mode =
            state_.mode == CameraMode::gameplay ? CameraMode::free_debug : CameraMode::gameplay;
        return;
    }
    if (!std::isfinite(fixed_delta_seconds) || fixed_delta_seconds <= 0.0) {
        return;
    }

    const double yaw_change = finite_unit_input(input.look_right_rate) *
                                  kStickLookRadiansPerSecond * fixed_delta_seconds +
                              finite_delta(input.look_right_delta) * kMouseRadiansPerPixel;
    const double pitch_change =
        finite_unit_input(input.look_up_rate) * kStickLookRadiansPerSecond * fixed_delta_seconds +
        finite_delta(input.look_up_delta) * kMouseRadiansPerPixel;
    if (state_.mode == CameraMode::gameplay) {
        state_.gameplay_yaw = std::remainder(state_.gameplay_yaw + yaw_change, kTwoPi);
        state_.gameplay_pitch = std::clamp(state_.gameplay_pitch + pitch_change,
                                           kMinimumGameplayPitch, kMaximumGameplayPitch);
        return;
    }

    constexpr double kMoveSpeed = 12.0;
    state_.free_yaw = std::remainder(state_.free_yaw + yaw_change, kTwoPi);
    state_.free_pitch =
        std::clamp(state_.free_pitch + pitch_change, -kHalfPi + 0.08, kHalfPi - 0.08);

    const Vec3 forward = direction_from_angles(state_.free_yaw, 0.0);
    const Vec3 right{.x = -forward.z, .y = 0.0, .z = forward.x};
    const double move_right = std::clamp(input.move_right, -1.0, 1.0);
    const double move_forward = std::clamp(input.move_forward, -1.0, 1.0);
    const double rise = std::clamp(input.rise, -1.0, 1.0);
    state_.free_eye.x +=
        (right.x * move_right + forward.x * move_forward) * kMoveSpeed * fixed_delta_seconds;
    state_.free_eye.y += rise * kMoveSpeed * fixed_delta_seconds;
    state_.free_eye.z +=
        (right.z * move_right + forward.z * move_forward) * kMoveSpeed * fixed_delta_seconds;
}

void CameraController::restart(const DogState& dog) noexcept {
    state_ = {};
    state_.gameplay_yaw = dog.heading_radians;
    reset_free_camera();
}

CameraMode CameraController::mode() const noexcept {
    return state_.mode;
}

const CameraState& CameraController::state() const noexcept {
    return state_;
}

CameraPose CameraController::pose(const DogState& dog) const noexcept {
    return camera_pose(dog, state_);
}

void CameraController::reset_free_camera() noexcept {
    state_.free_eye = kFreeCameraEye;
    const Vec3 direction{
        .x = kFreeCameraTarget.x - kFreeCameraEye.x,
        .y = kFreeCameraTarget.y - kFreeCameraEye.y,
        .z = kFreeCameraTarget.z - kFreeCameraEye.z,
    };
    state_.free_yaw = std::atan2(direction.x, -direction.z);
    state_.free_pitch = std::atan2(direction.y, std::hypot(direction.x, direction.z));
}

} // namespace wide_eye::game
