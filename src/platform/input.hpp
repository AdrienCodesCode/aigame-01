#pragma once

#include <SDL3/SDL_events.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace wide_eye::platform {

enum class NamedAction : std::uint8_t {
    move_forward,
    move_backward,
    move_left,
    move_right,
    sprint,
    restart,
    toggle_camera,
    camera_rise,
    camera_fall,
    camera_look_left,
    camera_look_right,
    camera_look_up,
    camera_look_down,
    toggle_mouse_capture,
    count,
};

inline constexpr std::size_t kNamedActionCount = static_cast<std::size_t>(NamedAction::count);

struct NamedActionValue {
    float value = 0.0F;
    bool pressed = false;
};

struct MouseLookDelta {
    float right = 0.0F;
    float up = 0.0F;

    bool operator==(const MouseLookDelta&) const = default;
};

class NamedActionSnapshot {
  public:
    [[nodiscard]] NamedActionValue action(NamedAction action) const noexcept;
    [[nodiscard]] float signed_axis(NamedAction negative, NamedAction positive) const noexcept;
    [[nodiscard]] MouseLookDelta mouse_look_delta() const noexcept;

  private:
    std::array<NamedActionValue, kNamedActionCount> actions_{};
    MouseLookDelta mouse_look_delta_{};
    friend class NamedInputState;
};

// Translates keyboard and SDL's standardized gamepad mapping into named
// actions. Gameplay receives snapshots only and never depends on physical keys,
// button labels, or SDL event layout.
class NamedInputState {
  public:
    void apply_event(const SDL_Event& event) noexcept;
    void clear_keyboard() noexcept;
    void clear_gamepad() noexcept;
    void clear_transients() noexcept;
    void consume_presses() noexcept;
    void consume_transients() noexcept;

    // Reads and clears one action's rising edge. Window-lifetime actions are
    // handled once per physical press by their owner instead of waiting for the
    // fixed-tick consumer, which may not run in a given frame.
    [[nodiscard]] bool consume_press(NamedAction action) noexcept;

    // Discards only the accumulated relative mouse deltas, leaving other
    // pending rising presses intact. Used across a pointer-capture change so
    // stale motion cannot become a camera jump without eating a same-frame
    // press such as restart.
    void clear_mouse_look() noexcept;

    [[nodiscard]] NamedActionSnapshot snapshot() const noexcept;

  private:
    void set_keyboard(NamedAction action, bool down) noexcept;
    void set_gamepad(NamedAction action, float value) noexcept;
    void update_gamepad_axis(std::uint8_t axis, std::int16_t value) noexcept;
    void record_rising_edge(NamedAction action, float previous, float current) noexcept;
    [[nodiscard]] float combined_value(NamedAction action) const noexcept;

    std::array<float, kNamedActionCount> keyboard_{};
    std::array<float, kNamedActionCount> gamepad_{};
    std::array<bool, kNamedActionCount> pressed_{};
    MouseLookDelta mouse_look_delta_{};
};

[[nodiscard]] const char* named_action_name(NamedAction action) noexcept;

} // namespace wide_eye::platform
