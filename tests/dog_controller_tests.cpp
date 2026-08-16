#include "game/camera_controller.hpp"
#include "game/dog_controller.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "dog_controller_failure=" << name << '\n';
    }
    return condition;
}

bool approximately(double left, double right, double tolerance = 1.0e-10) {
    return std::abs(left - right) <= tolerance;
}

wide_eye::game::DogState run_forward(std::string_view scenario_name, int tick_count) {
    const auto scenario = wide_eye::game::find_dog_scenario(scenario_name);
    if (!scenario.has_value()) {
        return {};
    }
    wide_eye::game::DogController dog{*scenario};
    for (int tick = 0; tick < tick_count; ++tick) {
        dog.fixed_update({.world_z = -1.0}, 1.0 / 60.0);
    }
    return dog.state();
}

} // namespace

int main() {
    using wide_eye::game::CameraControlInput;
    using wide_eye::game::CameraController;
    using wide_eye::game::CameraMode;
    using wide_eye::game::CameraState;
    using wide_eye::game::DogController;
    using wide_eye::game::DogState;
    using wide_eye::game::PaddockCollisionField;

    const auto start = wide_eye::game::find_dog_scenario("paddock-start");
    const auto wall = wide_eye::game::find_dog_scenario("wall-contact");
    const auto closed_gate = wide_eye::game::find_dog_scenario("closed-gate");
    const auto open_gate = wide_eye::game::find_dog_scenario("open-gate");
    if (!check(start.has_value() && wall.has_value() && closed_gate.has_value() &&
                   open_gate.has_value() &&
                   !wide_eye::game::find_dog_scenario("unknown").has_value(),
               "named_scenario_selection")) {
        return EXIT_FAILURE;
    }

    constexpr double kHalfPi = 1.57079632679489661923;
    constexpr double kPi = 3.14159265358979323846;
    const auto forward_at_zero = wide_eye::game::resolve_camera_relative_move(0.0, 0.0, 1.0);
    const auto right_at_zero = wide_eye::game::resolve_camera_relative_move(0.0, 1.0, 0.0);
    const auto forward_at_ninety = wide_eye::game::resolve_camera_relative_move(kHalfPi, 0.0, 1.0);
    const auto left_at_ninety = wide_eye::game::resolve_camera_relative_move(kHalfPi, -1.0, 0.0);
    const auto diagonal = wide_eye::game::resolve_camera_relative_move(0.0, 1.0, 1.0);
    if (!check(approximately(forward_at_zero.x, 0.0) && approximately(forward_at_zero.z, -1.0),
               "camera_yaw_zero_forward") ||
        !check(approximately(right_at_zero.x, 1.0) && approximately(right_at_zero.z, 0.0),
               "camera_yaw_zero_right") ||
        !check(approximately(forward_at_ninety.x, 1.0) && approximately(forward_at_ninety.z, 0.0),
               "camera_yaw_ninety_forward") ||
        !check(approximately(left_at_ninety.x, 0.0) && approximately(left_at_ninety.z, -1.0),
               "camera_yaw_ninety_left") ||
        !check(approximately(std::hypot(diagonal.x, diagonal.z), 1.0),
               "camera_relative_diagonal_normalized")) {
        return EXIT_FAILURE;
    }

    CameraController orbit_camera{start->initial_state};
    const CameraState orbit_initial = orbit_camera.state();
    const auto orbit_initial_pose = orbit_camera.pose(start->initial_state);
    orbit_camera.fixed_update(start->initial_state,
                              CameraControlInput{.look_right_delta = 100.0, .look_up_delta = 50.0},
                              1.0 / 60.0);
    if (!check(approximately(orbit_camera.state().gameplay_yaw, orbit_initial.gameplay_yaw + 0.3),
               "mouse_delta_changes_gameplay_yaw") ||
        !check(
            approximately(orbit_camera.state().gameplay_pitch, orbit_initial.gameplay_pitch + 0.15),
            "mouse_delta_changes_gameplay_pitch") ||
        !check(orbit_camera.pose(start->initial_state).eye != orbit_initial_pose.eye,
               "mouse_orbits_stationary_dog") ||
        !check(start->initial_state.heading_radians == 0.0,
               "mouse_orbit_does_not_change_dog_heading")) {
        return EXIT_FAILURE;
    }

    CameraController delta_at_sixty{start->initial_state};
    CameraController delta_at_thirty{start->initial_state};
    delta_at_sixty.fixed_update(start->initial_state, CameraControlInput{.look_right_delta = 20.0},
                                1.0 / 60.0);
    delta_at_thirty.fixed_update(start->initial_state, CameraControlInput{.look_right_delta = 20.0},
                                 1.0 / 30.0);
    if (!check(approximately(delta_at_sixty.state().gameplay_yaw,
                             delta_at_thirty.state().gameplay_yaw),
               "mouse_delta_independent_of_fixed_delta")) {
        return EXIT_FAILURE;
    }

    DogState previous_interpolation = start->initial_state;
    DogState current_interpolation = start->initial_state;
    previous_interpolation.position.x = 2.0;
    current_interpolation.position.x = 6.0;
    previous_interpolation.heading_radians = kPi - 0.1;
    current_interpolation.heading_radians = -kPi + 0.1;
    const DogState halfway_dog =
        wide_eye::game::interpolate_dog_state(previous_interpolation, current_interpolation, 0.5);
    CameraState previous_camera = orbit_initial;
    CameraState current_camera = orbit_initial;
    previous_camera.gameplay_yaw = kPi - 0.1;
    current_camera.gameplay_yaw = -kPi + 0.1;
    const CameraState halfway_camera =
        wide_eye::game::interpolate_camera_state(previous_camera, current_camera, 0.5);
    if (!check(approximately(halfway_dog.position.x, 4.0), "dog_interpolation_midpoint") ||
        !check(approximately(std::abs(halfway_dog.heading_radians), kPi),
               "dog_interpolation_shortest_angle") ||
        !check(approximately(std::abs(halfway_camera.gameplay_yaw), kPi),
               "camera_interpolation_shortest_angle") ||
        !check(wide_eye::game::interpolate_dog_state(previous_interpolation, current_interpolation,
                                                     0.0)
                       .position == previous_interpolation.position,
               "dog_interpolation_alpha_zero") ||
        !check(wide_eye::game::interpolate_dog_state(previous_interpolation, current_interpolation,
                                                     1.0)
                       .position == current_interpolation.position,
               "dog_interpolation_alpha_one")) {
        return EXIT_FAILURE;
    }

    PaddockCollisionField closed_collision;
    const auto wall_sweep = closed_collision.move_cylinder({.x = 8.0, .y = 1.0, .z = 20.0},
                                                           {.z = -40.0}, DogController::kRadius);
    const auto gate_sweep = closed_collision.move_cylinder({.x = 16.0, .y = 1.0, .z = 20.0},
                                                           {.z = -40.0}, DogController::kRadius);
    if (!check(std::abs(wall_sweep.z - (16.0 + DogController::kRadius)) < 1.0e-12,
               "wall_sweep_no_tunneling") ||
        !check(std::abs(gate_sweep.z - (16.0 + DogController::kRadius)) < 1.0e-12,
               "closed_gate_sweep_no_tunneling")) {
        return EXIT_FAILURE;
    }

    PaddockCollisionField open_collision{true};
    const auto open_sweep = open_collision.move_cylinder({.x = 16.0, .y = 1.0, .z = 20.0},
                                                         {.z = -8.0}, DogController::kRadius);
    if (!check(open_sweep.z < 14.0, "open_gate_passage") ||
        !check(open_sweep.y == PaddockCollisionField::kGroundHeight,
               "predictable_ground_contact") ||
        !check(closed_collision.obstacle_count() == 3 && open_collision.obstacle_count() == 2,
               "analytic_gate_ownership")) {
        return EXIT_FAILURE;
    }

    const auto wall_run = run_forward("wall-contact", 240);
    const auto wall_repeat = run_forward("wall-contact", 240);
    const auto gate_run = run_forward("closed-gate", 240);
    const auto open_run = run_forward("open-gate", 240);
    if (!check(wall_run == wall_repeat, "repeated_local_determinism") ||
        !check(wall_run.position.z >= 16.0 + DogController::kRadius, "wall_tick_no_tunneling") ||
        !check(gate_run.position.z >= 16.0 + DogController::kRadius, "gate_tick_no_tunneling") ||
        !check(open_run.position.z < 14.0, "open_gate_tick_passage") ||
        !check(wall_run.grounded && gate_run.grounded && open_run.grounded,
               "grounded_after_motion")) {
        return EXIT_FAILURE;
    }

    auto diagonal_scenario = *start;
    diagonal_scenario.initial_state.heading_radians = kPi * 0.25;
    DogController cardinal_motor{*start};
    DogController diagonal_motor{diagonal_scenario};
    cardinal_motor.fixed_update({.world_z = -1.0}, 1.0 / 60.0);
    constexpr double kDiagonalComponent = 0.70710678118654752440;
    diagonal_motor.fixed_update({.world_x = kDiagonalComponent, .world_z = -kDiagonalComponent},
                                1.0 / 60.0);
    if (!check(
            approximately(
                std::hypot(cardinal_motor.state().velocity.x, cardinal_motor.state().velocity.z),
                std::hypot(diagonal_motor.state().velocity.x, diagonal_motor.state().velocity.z)),
            "planar_acceleration_has_no_diagonal_bonus")) {
        return EXIT_FAILURE;
    }

    DogController reversal_motor{*start};
    for (int tick = 0; tick < 45; ++tick) {
        reversal_motor.fixed_update({.world_z = -1.0}, 1.0 / 60.0);
    }
    const double forward_speed =
        std::hypot(reversal_motor.state().velocity.x, reversal_motor.state().velocity.z);
    reversal_motor.fixed_update({.world_z = 1.0}, 1.0 / 60.0);
    const double first_reversal_speed =
        std::hypot(reversal_motor.state().velocity.x, reversal_motor.state().velocity.z);
    if (!check(first_reversal_speed < forward_speed, "hard_reversal_decelerates") ||
        !check(reversal_motor.state().heading_radians > 0.0 &&
                   reversal_motor.state().heading_radians <=
                       DogController::kTurnRateRadiansPerSecond / 60.0 + 1.0e-12,
               "hard_reversal_begins_bounded_body_turn")) {
        return EXIT_FAILURE;
    }
    for (int tick = 0; tick < 60; ++tick) {
        reversal_motor.fixed_update({.world_z = 1.0}, 1.0 / 60.0);
    }
    if (!check(reversal_motor.state().velocity.z > 0.0, "hard_reversal_accelerates_after_turn") ||
        !check(approximately(std::abs(reversal_motor.state().heading_radians), kPi, 1.0e-9),
               "hard_reversal_heading_converges")) {
        return EXIT_FAILURE;
    }

    CameraController steering_camera{start->initial_state};
    DogController steering_dog{*start};
    steering_camera.fixed_update(steering_dog.state(),
                                 CameraControlInput{.look_right_delta = 100.0}, 1.0 / 60.0);
    const auto curved_intent = wide_eye::game::resolve_camera_relative_move(
        steering_camera.state().gameplay_yaw, 0.0, 1.0);
    steering_dog.fixed_update({.world_x = curved_intent.x, .world_z = curved_intent.z}, 1.0 / 60.0);
    if (!check(steering_dog.state().position.x > start->initial_state.position.x &&
                   steering_dog.state().position.z < start->initial_state.position.z,
               "held_forward_uses_updated_camera_yaw_same_tick") ||
        !check(approximately(steering_camera.state().gameplay_yaw, 0.3),
               "gameplay_camera_yaw_independent_from_dog_body")) {
        return EXIT_FAILURE;
    }

    const double placed_gameplay_yaw = steering_camera.state().gameplay_yaw;
    steering_camera.fixed_update(steering_dog.state(), CameraControlInput{.toggle_mode = true},
                                 1.0 / 60.0);
    steering_camera.fixed_update(steering_dog.state(),
                                 CameraControlInput{.move_forward = 1.0, .look_right_delta = 25.0},
                                 1.0 / 60.0);
    steering_camera.fixed_update(steering_dog.state(), CameraControlInput{.toggle_mode = true},
                                 1.0 / 60.0);
    if (!check(steering_camera.mode() == CameraMode::gameplay &&
                   approximately(steering_camera.state().gameplay_yaw, placed_gameplay_yaw),
               "debug_camera_does_not_replace_gameplay_yaw")) {
        return EXIT_FAILURE;
    }

    CameraController repeated_camera_a{start->initial_state};
    CameraController repeated_camera_b{start->initial_state};
    DogController repeated_dog_a{*start};
    DogController repeated_dog_b{*start};
    for (int tick = 0; tick < 120; ++tick) {
        const double mouse_delta = static_cast<double>((tick % 5) - 2);
        const double move_right = tick < 40 ? 0.25 : (tick < 80 ? -0.5 : 0.0);
        const CameraControlInput camera_input{.look_right_delta = mouse_delta};
        repeated_camera_a.fixed_update(repeated_dog_a.state(), camera_input, 1.0 / 60.0);
        repeated_camera_b.fixed_update(repeated_dog_b.state(), camera_input, 1.0 / 60.0);
        const auto intent_a = wide_eye::game::resolve_camera_relative_move(
            repeated_camera_a.state().gameplay_yaw, move_right, 1.0);
        const auto intent_b = wide_eye::game::resolve_camera_relative_move(
            repeated_camera_b.state().gameplay_yaw, move_right, 1.0);
        repeated_dog_a.fixed_update({.world_x = intent_a.x, .world_z = intent_a.z}, 1.0 / 60.0);
        repeated_dog_b.fixed_update({.world_x = intent_b.x, .world_z = intent_b.z}, 1.0 / 60.0);
    }
    if (!check(repeated_camera_a.state() == repeated_camera_b.state() &&
                   repeated_dog_a.state() == repeated_dog_b.state(),
               "repeated_controller_sequence_is_deterministic")) {
        return EXIT_FAILURE;
    }

    DogController dog{*start};
    const auto initial_state = dog.state();
    CameraController turn_camera{initial_state};
    const auto initial_turn_pose = turn_camera.pose(initial_state);
    dog.fixed_update({.world_x = 1.0, .sprint = true}, 1.0 / 60.0);
    turn_camera.fixed_update(dog.state(), CameraControlInput{}, 1.0 / 60.0);
    const auto first_turn_pose = turn_camera.pose(dog.state());
    const double first_turn_camera_displacement =
        std::hypot(first_turn_pose.eye.x - initial_turn_pose.eye.x,
                   first_turn_pose.eye.z - initial_turn_pose.eye.z);
    if (!check(dog.state() != initial_state && dog.tick() == 1, "dog_moves_on_fixed_tick") ||
        !check(std::abs(dog.state().heading_radians) <=
                   DogController::kTurnRateRadiansPerSecond / 60.0 + 1.0e-12,
               "dog_heading_turn_rate_bounded") ||
        !check(first_turn_camera_displacement < 0.75,
               "gameplay_camera_does_not_jump_on_first_turn")) {
        return EXIT_FAILURE;
    }
    for (int tick = 1; tick < 30; ++tick) {
        dog.fixed_update({.world_x = 1.0, .sprint = true}, 1.0 / 60.0);
    }
    if (!check(std::abs(dog.state().heading_radians - 1.57079632679489661923) < 1.0e-12,
               "dog_heading_converges_to_input_direction") ||
        !check(std::abs(dog.state().velocity.x - DogController::kSprintSpeed) < 1.0e-12,
               "sprint_speed_survives_smoothed_turn")) {
        return EXIT_FAILURE;
    }
    dog.restart();
    if (!check(dog.state() == initial_state && dog.tick() == 0 && dog.restart_count() == 1,
               "restart_restores_named_scenario")) {
        return EXIT_FAILURE;
    }

    CameraController camera{dog.state()};
    const auto gameplay_pose = camera.pose(dog.state());
    camera.fixed_update(dog.state(), CameraControlInput{.toggle_mode = true}, 1.0 / 60.0);
    const auto debug_pose = camera.pose(dog.state());
    camera.fixed_update(dog.state(), CameraControlInput{.move_forward = 1.0}, 1.0);
    const auto moved_debug_pose = camera.pose(dog.state());
    if (!check(camera.mode() == CameraMode::free_debug, "free_debug_camera_toggle") ||
        !check(debug_pose.eye != gameplay_pose.eye, "camera_modes_are_distinct") ||
        !check(moved_debug_pose.eye != debug_pose.eye, "free_debug_camera_moves")) {
        return EXIT_FAILURE;
    }
    camera.restart(dog.state());
    if (!check(camera.mode() == CameraMode::gameplay &&
                   camera.pose(dog.state()).eye == gameplay_pose.eye,
               "camera_restart")) {
        return EXIT_FAILURE;
    }

    std::cout << "dog_scenarios=4\n"
              << "collision_shape=upright_cylinder\n"
              << "collision_source=analytic_paddock\n"
              << "fixed_tick_hz=60\n"
              << "dog_controller_result=pass\n";
    return EXIT_SUCCESS;
}
