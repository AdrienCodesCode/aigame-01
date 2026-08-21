#pragma once

namespace wide_eye::platform {

enum class WindowChange {
    pixel_size,
    minimized,
    restored,
    focus_gained,
    focus_lost,
    close_requested,
    mouse_capture_requested,
    mouse_capture_toggled,
};

class WindowState {
  public:
    WindowState(int pixel_width, int pixel_height);

    [[nodiscard]] bool apply(WindowChange change, int pixel_width = 0, int pixel_height = 0);
    [[nodiscard]] int pixel_width() const;
    [[nodiscard]] int pixel_height() const;
    [[nodiscard]] bool minimized() const;
    [[nodiscard]] bool focused() const;
    [[nodiscard]] bool close_requested() const;
    [[nodiscard]] bool drawable() const;

    // Player intent to hold the pointer, independent of whether the window can
    // currently hold it.
    [[nodiscard]] bool mouse_capture_requested() const;
    // The capture state the window should actually be in. An unfocused window
    // never holds the pointer, and regaining focus restores only a capture the
    // player still wants.
    [[nodiscard]] bool mouse_captured() const;

  private:
    int pixel_width_;
    int pixel_height_;
    bool minimized_ = false;
    bool focused_ = true;
    bool close_requested_ = false;
    bool mouse_capture_requested_ = false;
};

} // namespace wide_eye::platform
