#include "platform/input.hpp"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_scancode.h>
#include <algorithm>
#include <cmath>

namespace wide_eye::platform {
namespace {

[[nodiscard]] constexpr std::size_t action_index(NamedAction action) noexcept {
    return static_cast<std::size_t>(action);
}

[[nodiscard]] float normalized_stick_axis(std::int16_t value) noexcept {
    constexpr float kDeadZone = 8'000.0F;
    constexpr float kMaximum = 32'767.0F;
    const float input = static_cast<float>(value);
    const float magnitude = std::abs(input);
    if (magnitude <= kDeadZone) {
        return 0.0F;
    }
    const float normalized = (magnitude - kDeadZone) / (kMaximum - kDeadZone);
    return std::copysign(std::clamp(normalized, 0.0F, 1.0F), input);
}

} // namespace

NamedActionValue NamedActionSnapshot::action(NamedAction action) const noexcept {
    const std::size_t index = action_index(action);
    return index < actions_.size() ? actions_[index] : NamedActionValue{};
}

float NamedActionSnapshot::signed_axis(NamedAction negative, NamedAction positive) const noexcept {
    return std::clamp(action(positive).value - action(negative).value, -1.0F, 1.0F);
}

MouseLookDelta NamedActionSnapshot::mouse_look_delta() const noexcept {
    return mouse_look_delta_;
}

void NamedInputState::apply_event(const SDL_Event& event) noexcept {
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        mouse_look_delta_.right += event.motion.xrel;
        mouse_look_delta_.up -= event.motion.yrel;
        return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        const bool down = event.type == SDL_EVENT_KEY_DOWN;
        switch (event.key.scancode) {
        case SDL_SCANCODE_W:
            set_keyboard(NamedAction::move_forward, down);
            break;
        case SDL_SCANCODE_S:
            set_keyboard(NamedAction::move_backward, down);
            break;
        case SDL_SCANCODE_A:
            set_keyboard(NamedAction::move_left, down);
            break;
        case SDL_SCANCODE_D:
            set_keyboard(NamedAction::move_right, down);
            break;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            set_keyboard(NamedAction::sprint, down);
            break;
        case SDL_SCANCODE_R:
            set_keyboard(NamedAction::restart, down);
            break;
        case SDL_SCANCODE_TAB:
            set_keyboard(NamedAction::toggle_camera, down);
            break;
        case SDL_SCANCODE_E:
            set_keyboard(NamedAction::camera_rise, down);
            break;
        case SDL_SCANCODE_Q:
            set_keyboard(NamedAction::camera_fall, down);
            break;
        case SDL_SCANCODE_LEFT:
            set_keyboard(NamedAction::camera_look_left, down);
            break;
        case SDL_SCANCODE_RIGHT:
            set_keyboard(NamedAction::camera_look_right, down);
            break;
        case SDL_SCANCODE_UP:
            set_keyboard(NamedAction::camera_look_up, down);
            break;
        case SDL_SCANCODE_DOWN:
            set_keyboard(NamedAction::camera_look_down, down);
            break;
        default:
            break;
        }
        return;
    }

    if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        update_gamepad_axis(event.gaxis.axis, event.gaxis.value);
        return;
    }
    if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
        const float value = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? 1.0F : 0.0F;
        switch (static_cast<SDL_GamepadButton>(event.gbutton.button)) {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            set_gamepad(NamedAction::sprint, value);
            break;
        case SDL_GAMEPAD_BUTTON_START:
            set_gamepad(NamedAction::restart, value);
            break;
        case SDL_GAMEPAD_BUTTON_BACK:
            set_gamepad(NamedAction::toggle_camera, value);
            break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            set_gamepad(NamedAction::camera_rise, value);
            break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            set_gamepad(NamedAction::camera_fall, value);
            break;
        default:
            break;
        }
    }
}

void NamedInputState::clear_keyboard() noexcept {
    for (std::size_t index = 0; index < keyboard_.size(); ++index) {
        const float previous = combined_value(static_cast<NamedAction>(index));
        keyboard_[index] = 0.0F;
        record_rising_edge(static_cast<NamedAction>(index), previous,
                           combined_value(static_cast<NamedAction>(index)));
    }
}

void NamedInputState::clear_gamepad() noexcept {
    for (std::size_t index = 0; index < gamepad_.size(); ++index) {
        const float previous = combined_value(static_cast<NamedAction>(index));
        gamepad_[index] = 0.0F;
        record_rising_edge(static_cast<NamedAction>(index), previous,
                           combined_value(static_cast<NamedAction>(index)));
    }
}

void NamedInputState::clear_transients() noexcept {
    pressed_.fill(false);
    mouse_look_delta_ = {};
}

void NamedInputState::consume_presses() noexcept {
    pressed_.fill(false);
}

void NamedInputState::consume_transients() noexcept {
    clear_transients();
}

NamedActionSnapshot NamedInputState::snapshot() const noexcept {
    NamedActionSnapshot result;
    for (std::size_t index = 0; index < result.actions_.size(); ++index) {
        const auto action = static_cast<NamedAction>(index);
        result.actions_[index] = {
            .value = combined_value(action),
            .pressed = pressed_[index],
        };
    }
    result.mouse_look_delta_ = mouse_look_delta_;
    return result;
}

void NamedInputState::set_keyboard(NamedAction action, bool down) noexcept {
    const float previous = combined_value(action);
    keyboard_[action_index(action)] = down ? 1.0F : 0.0F;
    record_rising_edge(action, previous, combined_value(action));
}

void NamedInputState::set_gamepad(NamedAction action, float value) noexcept {
    const float previous = combined_value(action);
    gamepad_[action_index(action)] = std::clamp(value, 0.0F, 1.0F);
    record_rising_edge(action, previous, combined_value(action));
}

void NamedInputState::update_gamepad_axis(std::uint8_t axis, std::int16_t value) noexcept {
    const float normalized = normalized_stick_axis(value);
    const auto update_pair = [&](NamedAction negative, NamedAction positive, float signed_value) {
        set_gamepad(negative, std::max(-signed_value, 0.0F));
        set_gamepad(positive, std::max(signed_value, 0.0F));
    };

    switch (static_cast<SDL_GamepadAxis>(axis)) {
    case SDL_GAMEPAD_AXIS_LEFTX:
        update_pair(NamedAction::move_left, NamedAction::move_right, normalized);
        break;
    case SDL_GAMEPAD_AXIS_LEFTY:
        update_pair(NamedAction::move_forward, NamedAction::move_backward, normalized);
        break;
    case SDL_GAMEPAD_AXIS_RIGHTX:
        update_pair(NamedAction::camera_look_left, NamedAction::camera_look_right, normalized);
        break;
    case SDL_GAMEPAD_AXIS_RIGHTY:
        update_pair(NamedAction::camera_look_up, NamedAction::camera_look_down, normalized);
        break;
    default:
        break;
    }
}

void NamedInputState::record_rising_edge(NamedAction action, float previous,
                                         float current) noexcept {
    constexpr float kPressedThreshold = 0.5F;
    if (previous <= kPressedThreshold && current > kPressedThreshold) {
        pressed_[action_index(action)] = true;
    }
}

float NamedInputState::combined_value(NamedAction action) const noexcept {
    const std::size_t index = action_index(action);
    return std::max(keyboard_[index], gamepad_[index]);
}

const char* named_action_name(NamedAction action) noexcept {
    switch (action) {
    case NamedAction::move_forward:
        return "move_forward";
    case NamedAction::move_backward:
        return "move_backward";
    case NamedAction::move_left:
        return "move_left";
    case NamedAction::move_right:
        return "move_right";
    case NamedAction::sprint:
        return "sprint";
    case NamedAction::restart:
        return "restart";
    case NamedAction::toggle_camera:
        return "toggle_camera";
    case NamedAction::camera_rise:
        return "camera_rise";
    case NamedAction::camera_fall:
        return "camera_fall";
    case NamedAction::camera_look_left:
        return "camera_look_left";
    case NamedAction::camera_look_right:
        return "camera_look_right";
    case NamedAction::camera_look_up:
        return "camera_look_up";
    case NamedAction::camera_look_down:
        return "camera_look_down";
    case NamedAction::count:
        return "unknown";
    }
    return "unknown";
}

} // namespace wide_eye::platform
