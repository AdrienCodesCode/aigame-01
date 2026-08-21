#include "platform/window_state.hpp"

#include "core/runtime.hpp"

namespace wide_eye::platform {

WindowState::WindowState(int pixel_width, int pixel_height)
    : pixel_width_{pixel_width}, pixel_height_{pixel_height} {
    WIDE_EYE_ASSERT(pixel_width > 0, "initial window pixel width must be positive");
    WIDE_EYE_ASSERT(pixel_height > 0, "initial window pixel height must be positive");
}

bool WindowState::apply(WindowChange change, int pixel_width, int pixel_height) {
    switch (change) {
    case WindowChange::pixel_size:
        if (pixel_width < 0 || pixel_height < 0) {
            return false;
        }
        pixel_width_ = pixel_width;
        pixel_height_ = pixel_height;
        return true;
    case WindowChange::minimized:
        minimized_ = true;
        return true;
    case WindowChange::restored:
        minimized_ = false;
        return true;
    case WindowChange::focus_gained:
        focused_ = true;
        return true;
    case WindowChange::focus_lost:
        focused_ = false;
        return true;
    case WindowChange::close_requested:
        close_requested_ = true;
        return true;
    case WindowChange::mouse_capture_requested:
        mouse_capture_requested_ = true;
        return true;
    case WindowChange::mouse_capture_toggled:
        mouse_capture_requested_ = !mouse_capture_requested_;
        return true;
    }

    return false;
}

int WindowState::pixel_width() const {
    return pixel_width_;
}

int WindowState::pixel_height() const {
    return pixel_height_;
}

bool WindowState::minimized() const {
    return minimized_;
}

bool WindowState::focused() const {
    return focused_;
}

bool WindowState::close_requested() const {
    return close_requested_;
}

bool WindowState::drawable() const {
    return !minimized_ && pixel_width_ > 0 && pixel_height_ > 0;
}

bool WindowState::mouse_capture_requested() const {
    return mouse_capture_requested_;
}

bool WindowState::mouse_captured() const {
    return mouse_capture_requested_ && focused_;
}

} // namespace wide_eye::platform
