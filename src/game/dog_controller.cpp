#include "game/dog_controller.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace wide_eye::game {
namespace {

constexpr AnalyticObstacle kLeftWall{
    .minimum_x = 1.0,
    .maximum_x = 14.0,
    .minimum_z = 14.0,
    .maximum_z = 16.0,
};
constexpr AnalyticObstacle kRightWall{
    .minimum_x = 18.0,
    .maximum_x = 31.0,
    .minimum_z = 14.0,
    .maximum_z = 16.0,
};
constexpr AnalyticObstacle kClosedGate{
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

[[nodiscard]] double approach_angle(double current, double target, double maximum_change) noexcept {
    constexpr double kTwoPi = 6.28318530717958647692;
    const double shortest_delta = std::remainder(target - current, kTwoPi);
    return std::remainder(current + std::clamp(shortest_delta, -maximum_change, maximum_change),
                          kTwoPi);
}

[[nodiscard]] double finite_unit_input(double value) noexcept {
    return std::isfinite(value) ? std::clamp(value, -1.0, 1.0) : 0.0;
}

[[nodiscard]] double turn_speed_scale(double absolute_heading_error) noexcept {
    constexpr double kFullSpeedError = 0.52359877559829887308; // 30 degrees
    constexpr double kStoppedError = 2.09439510239319549231;   // 120 degrees
    if (absolute_heading_error <= kFullSpeedError) {
        return 1.0;
    }
    if (absolute_heading_error >= kStoppedError) {
        return 0.0;
    }
    return 1.0 - (absolute_heading_error - kFullSpeedError) / (kStoppedError - kFullSpeedError);
}

void approach_planar_velocity(Vec3& velocity, double target_x, double target_z,
                              double maximum_change) noexcept {
    const double change_x = target_x - velocity.x;
    const double change_z = target_z - velocity.z;
    const double change_length = std::hypot(change_x, change_z);
    if (change_length <= maximum_change || change_length <= 1.0e-12) {
        velocity.x = target_x;
        velocity.z = target_z;
        return;
    }
    const double scale = maximum_change / change_length;
    velocity.x += change_x * scale;
    velocity.z += change_z * scale;
}

} // namespace

DogState interpolate_dog_state(const DogState& previous, const DogState& current,
                               double alpha) noexcept {
    constexpr double kTwoPi = 6.28318530717958647692;
    const double bounded_alpha = std::isfinite(alpha) ? std::clamp(alpha, 0.0, 1.0) : 1.0;
    const auto interpolate = [bounded_alpha](double start, double end) {
        return start + (end - start) * bounded_alpha;
    };
    DogState result{
        .position = {.x = interpolate(previous.position.x, current.position.x),
                     .y = interpolate(previous.position.y, current.position.y),
                     .z = interpolate(previous.position.z, current.position.z)},
        .velocity = {.x = interpolate(previous.velocity.x, current.velocity.x),
                     .y = interpolate(previous.velocity.y, current.velocity.y),
                     .z = interpolate(previous.velocity.z, current.velocity.z)},
        .heading_radians = std::remainder(
            previous.heading_radians +
                std::remainder(current.heading_radians - previous.heading_radians, kTwoPi) *
                    bounded_alpha,
            kTwoPi),
        .grounded = current.grounded,
    };
    return result;
}

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

bool PaddockCollisionField::gate_open() const noexcept {
    return gate_open_;
}

std::size_t PaddockCollisionField::obstacle_count() const noexcept {
    return obstacle_count_;
}

DogController::DogController(DogControllerConfiguration configuration) noexcept
    : configuration_{configuration}, collision_{configuration.gate_open},
      state_{configuration.initial_state} {
    state_.position.y = collision_.ground_height(state_.position.x, state_.position.z);
    state_.grounded = std::isfinite(state_.position.y);
}

void DogController::fixed_update(const DogMoveInput& input, double fixed_delta_seconds) noexcept {
    if (!std::isfinite(fixed_delta_seconds) || fixed_delta_seconds <= 0.0) {
        return;
    }

    double desired_x = finite_unit_input(input.world_x);
    double desired_z = finite_unit_input(input.world_z);
    const double raw_magnitude = std::hypot(desired_x, desired_z);
    const double input_magnitude = std::min(raw_magnitude, 1.0);
    if (raw_magnitude > 1.0) {
        desired_x /= raw_magnitude;
        desired_z /= raw_magnitude;
    } else if (raw_magnitude > 1.0e-12) {
        desired_x /= raw_magnitude;
        desired_z /= raw_magnitude;
    }

    double heading_speed_scale = 1.0;
    if (input_magnitude > 1.0e-12) {
        const double target_heading = std::atan2(desired_x, -desired_z);
        state_.heading_radians = approach_angle(state_.heading_radians, target_heading,
                                                kTurnRateRadiansPerSecond * fixed_delta_seconds);
        const double remaining_error = std::abs(
            std::remainder(target_heading - state_.heading_radians, 6.28318530717958647692));
        heading_speed_scale = turn_speed_scale(remaining_error);
    }

    const double requested_speed = (input.sprint ? kSprintSpeed : kWalkSpeed) * input_magnitude;
    const double target_speed = requested_speed * heading_speed_scale;
    const double target_velocity_x = desired_x * target_speed;
    const double target_velocity_z = desired_z * target_speed;
    const double current_speed_squared =
        state_.velocity.x * state_.velocity.x + state_.velocity.z * state_.velocity.z;
    const double change_dot_current = (target_velocity_x - state_.velocity.x) * state_.velocity.x +
                                      (target_velocity_z - state_.velocity.z) * state_.velocity.z;
    const double velocity_rate =
        current_speed_squared > 1.0e-12 && change_dot_current < 0.0 ? kDeceleration : kAcceleration;
    approach_planar_velocity(state_.velocity, target_velocity_x, target_velocity_z,
                             velocity_rate * fixed_delta_seconds);

    const Vec3 previous = state_.position;
    const Vec3 requested_displacement{.x = state_.velocity.x * fixed_delta_seconds,
                                      .y = 0.0,
                                      .z = state_.velocity.z * fixed_delta_seconds};
    state_.position = collision_.move_cylinder(state_.position, requested_displacement, kRadius);
    constexpr double kCollisionDisplacementTolerance = 1.0e-12;
    const double resolved_x = state_.position.x - previous.x;
    const double resolved_z = state_.position.z - previous.z;
    if (std::abs(resolved_x - requested_displacement.x) > kCollisionDisplacementTolerance) {
        state_.velocity.x = 0.0;
    }
    if (std::abs(resolved_z - requested_displacement.z) > kCollisionDisplacementTolerance) {
        state_.velocity.z = 0.0;
    }
    state_.velocity.y = 0.0;
    state_.grounded = std::isfinite(state_.position.y);

    ++tick_;
}

void DogController::restart() noexcept {
    state_ = configuration_.initial_state;
    state_.position.y = collision_.ground_height(state_.position.x, state_.position.z);
    state_.grounded = std::isfinite(state_.position.y);
    tick_ = 0;
    ++restart_count_;
}

const DogState& DogController::state() const noexcept {
    return state_;
}

std::uint64_t DogController::tick() const noexcept {
    return tick_;
}

std::uint32_t DogController::restart_count() const noexcept {
    return restart_count_;
}

} // namespace wide_eye::game
