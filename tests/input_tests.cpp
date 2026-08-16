#include "platform/input.hpp"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_scancode.h>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "input_failure=" << name << '\n';
    }
    return condition;
}

SDL_Event keyboard_event(Uint32 type, SDL_Scancode scancode) {
    SDL_Event event{};
    event.type = type;
    event.key.scancode = scancode;
    return event;
}

SDL_Event gamepad_axis_event(SDL_GamepadAxis axis, Sint16 value) {
    SDL_Event event{};
    event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    event.gaxis.axis = static_cast<Uint8>(axis);
    event.gaxis.value = value;
    return event;
}

SDL_Event gamepad_button_event(Uint32 type, SDL_GamepadButton button) {
    SDL_Event event{};
    event.type = type;
    event.gbutton.button = static_cast<Uint8>(button);
    return event;
}

SDL_Event mouse_motion_event(float x_relative, float y_relative) {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.xrel = x_relative;
    event.motion.yrel = y_relative;
    return event;
}

} // namespace

int main() {
    using wide_eye::platform::NamedAction;
    wide_eye::platform::NamedInputState input;

    input.apply_event(keyboard_event(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W));
    auto snapshot = input.snapshot();
    if (!check(snapshot.action(NamedAction::move_forward).value == 1.0F &&
                   snapshot.action(NamedAction::move_forward).pressed,
               "keyboard_to_named_move") ||
        !check(snapshot.signed_axis(NamedAction::move_backward, NamedAction::move_forward) == 1.0F,
               "keyboard_move_axis")) {
        return EXIT_FAILURE;
    }
    input.consume_presses();
    if (!check(!input.snapshot().action(NamedAction::move_forward).pressed, "press_consumption")) {
        return EXIT_FAILURE;
    }

    input.apply_event(mouse_motion_event(3.5F, -2.0F));
    input.apply_event(mouse_motion_event(-1.0F, 5.0F));
    snapshot = input.snapshot();
    if (!check(snapshot.mouse_look_delta() ==
                   wide_eye::platform::MouseLookDelta{.right = 2.5F, .up = -3.0F},
               "mouse_motion_accumulates_with_view_signs") ||
        !check(input.snapshot().mouse_look_delta() == snapshot.mouse_look_delta(),
               "mouse_motion_retained_until_fixed_tick")) {
        return EXIT_FAILURE;
    }
    input.consume_transients();
    if (!check(input.snapshot().mouse_look_delta() == wide_eye::platform::MouseLookDelta{},
               "mouse_motion_consumed_once")) {
        return EXIT_FAILURE;
    }

    input.apply_event(mouse_motion_event(8.0F, 4.0F));
    input.clear_transients();
    if (!check(input.snapshot().mouse_look_delta() == wide_eye::platform::MouseLookDelta{},
               "focus_clear_discards_mouse_motion")) {
        return EXIT_FAILURE;
    }

    input.apply_event(keyboard_event(SDL_EVENT_KEY_UP, SDL_SCANCODE_W));
    input.apply_event(gamepad_axis_event(SDL_GAMEPAD_AXIS_LEFTY, -32'768));
    snapshot = input.snapshot();
    if (!check(snapshot.action(NamedAction::move_forward).value > 0.99F,
               "gamepad_left_stick_move") ||
        !check(snapshot.signed_axis(NamedAction::move_backward, NamedAction::move_forward) > 0.99F,
               "gamepad_move_axis")) {
        return EXIT_FAILURE;
    }

    input.apply_event(gamepad_axis_event(SDL_GAMEPAD_AXIS_RIGHTX, 32'767));
    input.apply_event(gamepad_button_event(SDL_EVENT_GAMEPAD_BUTTON_DOWN, SDL_GAMEPAD_BUTTON_BACK));
    input.apply_event(
        gamepad_button_event(SDL_EVENT_GAMEPAD_BUTTON_DOWN, SDL_GAMEPAD_BUTTON_START));
    snapshot = input.snapshot();
    if (!check(snapshot.action(NamedAction::camera_look_right).value > 0.99F,
               "gamepad_right_stick_camera") ||
        !check(snapshot.action(NamedAction::toggle_camera).pressed, "gamepad_camera_toggle") ||
        !check(snapshot.action(NamedAction::restart).pressed, "gamepad_restart")) {
        return EXIT_FAILURE;
    }

    input.apply_event(mouse_motion_event(4.0F, -3.0F));
    snapshot = input.snapshot();
    if (!check(snapshot.action(NamedAction::camera_look_right).value > 0.99F &&
                   snapshot.mouse_look_delta() ==
                       wide_eye::platform::MouseLookDelta{.right = 4.0F, .up = 3.0F},
               "mouse_delta_coexists_with_stick_rate")) {
        return EXIT_FAILURE;
    }
    input.consume_transients();

    input.clear_gamepad();
    snapshot = input.snapshot();
    if (!check(snapshot.action(NamedAction::move_forward).value == 0.0F &&
                   snapshot.action(NamedAction::camera_look_right).value == 0.0F,
               "gamepad_disconnect_clears_actions")) {
        return EXIT_FAILURE;
    }

    std::cout << "keyboard_bindings=WASD,Shift,R,Tab,QE,Arrows\n"
              << "controller_bindings=left_stick,right_stick,South,Back,Start,Shoulders\n"
              << "input_actions=" << wide_eye::platform::kNamedActionCount << '\n'
              << "input_result=pass\n";
    return EXIT_SUCCESS;
}
