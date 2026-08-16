#pragma once

#include "core/performance.hpp"
#include "platform/input.hpp"
#include "platform/window_state.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace wide_eye::platform {

struct WindowFailure {
    std::string_view stage;
    bool report_sdl_error = false;
};

using WindowResult = std::optional<WindowFailure>;

struct WindowRunConfiguration {
    std::string_view result_name;
    int width;
    int height;
    bool use_opengl = false;
    bool bounded = true;
    bool hidden = false;
    bool resizable = false;
    bool require_depth_buffer = false;
    bool inject_high_severity_message = false;
    bool request_vsync = false;
    bool validate_window_events = false;
    bool render_bounded_frame = false;
    bool render_interactive_frames = false;
    bool enable_input = false;
    std::uint32_t performance_warmup_frames = 0;
    std::uint32_t performance_sample_frames = 0;
    std::string_view performance_scenario;
    std::optional<core::PerformanceBudget> performance_budget;
};

class WindowScenarioRunner {
  public:
    virtual ~WindowScenarioRunner() = default;

    [[nodiscard]] virtual WindowResult initialize();
    [[nodiscard]] virtual WindowResult fixed_update(const NamedActionSnapshot& input,
                                                    double fixed_delta_seconds);
    [[nodiscard]] virtual WindowResult prepare_performance_frame(double interpolation_alpha);
    [[nodiscard]] virtual WindowResult render_frame(const WindowState& window_state,
                                                    double interpolation_alpha) = 0;
    virtual void release_graphics_resources();
};

[[nodiscard]] int run_window(const WindowRunConfiguration& configuration,
                             WindowScenarioRunner& scenario);

} // namespace wide_eye::platform
