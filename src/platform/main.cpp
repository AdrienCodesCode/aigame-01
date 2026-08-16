#include "core/runtime.hpp"
#include "platform/scenario_runner.hpp"
#include "platform/window_state.hpp"

#include <SDL3/SDL_main.h>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;

std::optional<std::uint64_t> parse_tick(std::string_view text) {
    std::uint64_t tick = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), tick);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return tick;
}

int run_runtime_smoke() {
    wide_eye::core::log(wide_eye::core::LogLevel::info, "runtime_smoke",
                        "checking monotonic time and fixed 60 Hz accumulation");

    wide_eye::core::MonotonicFrameClock frame_clock;
    WIDE_EYE_ASSERT(frame_clock.sample() == std::chrono::nanoseconds::zero(),
                    "first monotonic frame sample must be zero");
    WIDE_EYE_ASSERT(frame_clock.sample() >= std::chrono::nanoseconds::zero(),
                    "monotonic frame sample must not be negative");

    wide_eye::core::FixedStepAccumulator fine_frames;
    wide_eye::core::FixedStepAccumulator coarse_frames;
    for (int frame = 0; frame < 100; ++frame) {
        static_cast<void>(fine_frames.advance(std::chrono::milliseconds{10}));
    }
    for (int frame = 0; frame < 10; ++frame) {
        static_cast<void>(coarse_frames.advance(std::chrono::milliseconds{100}));
    }

    WIDE_EYE_ASSERT(fine_frames.total_ticks() == 60, "one second must produce 60 fixed ticks");
    WIDE_EYE_ASSERT(coarse_frames.total_ticks() == fine_frames.total_ticks(),
                    "fixed ticks must not depend on render-frame partitioning");

    wide_eye::core::FixedStepAccumulator clamped_frame;
    const wide_eye::core::FixedStepUpdate clamped =
        clamped_frame.advance(std::chrono::milliseconds{300});
    WIDE_EYE_ASSERT(clamped.frame_delta_clamped, "oversized frame delta must be clamped");
    WIDE_EYE_ASSERT(clamped.ticks == 15, "250 ms clamp must produce exactly 15 ticks");
    WIDE_EYE_ASSERT(clamped.interpolation_alpha == 0.0,
                    "whole fixed ticks must leave no interpolation remainder");

    std::cout << "monotonic_clock=yes\n"
              << "fixed_tick_hz=" << wide_eye::core::FixedStepAccumulator::ticks_per_second << '\n'
              << "fine_frame_ticks=" << fine_frames.total_ticks() << '\n'
              << "coarse_frame_ticks=" << coarse_frames.total_ticks() << '\n'
              << "frame_delta_clamp_ms="
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     wide_eye::core::FixedStepAccumulator::maximum_frame_delta)
                     .count()
              << '\n'
              << "runtime_result=pass\n";
    return EXIT_SUCCESS;
}

int run_window_state_smoke() {
    using wide_eye::platform::WindowChange;
    wide_eye::platform::WindowState state{kWindowWidth, kWindowHeight};

    WIDE_EYE_ASSERT(state.drawable(), "new window must begin drawable");
    WIDE_EYE_ASSERT(state.apply(WindowChange::pixel_size, 1280, 720),
                    "positive resize must be accepted");
    WIDE_EYE_ASSERT(state.pixel_width() == 1280 && state.pixel_height() == 720,
                    "resize must update drawable pixel extent");
    WIDE_EYE_ASSERT(state.apply(WindowChange::pixel_size, 0, 0),
                    "zero-sized drawable transition must be accepted");
    WIDE_EYE_ASSERT(!state.drawable(), "zero-sized drawable must suspend drawing");
    WIDE_EYE_ASSERT(!state.apply(WindowChange::pixel_size, -1, 720),
                    "negative drawable extent must be rejected");
    WIDE_EYE_ASSERT(state.apply(WindowChange::pixel_size, 1280, 720),
                    "positive drawable extent must resume drawing");
    WIDE_EYE_ASSERT(state.apply(WindowChange::minimized), "minimize transition must be handled");
    WIDE_EYE_ASSERT(state.minimized() && !state.drawable(),
                    "minimized window must suspend drawing");
    WIDE_EYE_ASSERT(state.apply(WindowChange::focus_lost), "focus-loss transition must be handled");
    WIDE_EYE_ASSERT(!state.focused(), "focus-loss transition must update state");
    WIDE_EYE_ASSERT(state.apply(WindowChange::restored), "restore transition must be handled");
    WIDE_EYE_ASSERT(state.apply(WindowChange::focus_gained),
                    "focus-gain transition must be handled");
    WIDE_EYE_ASSERT(state.drawable() && state.focused(),
                    "restored focused window must be drawable");
    WIDE_EYE_ASSERT(state.apply(WindowChange::close_requested), "close transition must be handled");
    WIDE_EYE_ASSERT(state.close_requested(), "close transition must request shutdown");

    wide_eye::core::log(wide_eye::core::LogLevel::info, "window_state_smoke",
                        "resize, minimize, focus, restore, and close transitions passed");
    std::cout << "resize_handled=yes\n"
              << "minimize_handled=yes\n"
              << "focus_handled=yes\n"
              << "close_handled=yes\n"
              << "window_state_result=pass\n";
    return EXIT_SUCCESS;
}

[[noreturn]] void run_assertion_smoke() {
    WIDE_EYE_ASSERT(false, "intentional assertion smoke");
    std::abort();
}

void print_usage(std::string_view executable) {
    std::cerr
        << "usage: " << executable
        << " [--version | --window-smoke | --context-smoke | --triangle-smoke | "
           "--voxel-cube-smoke [--capture <png-path>] | --window-state-smoke | --runtime-smoke | "
           "--voxel-cube-debug-smoke [--capture <png-path>] | "
           "--paddock-smoke [--capture <png-path>] | "
           "--paddock-chunk-bounds-smoke [--capture <png-path>] | "
           "--paddock-face-normals-smoke [--capture <png-path>] | "
           "--paddock-wireframe-smoke [--capture <png-path>] | "
           "--paddock-mesh-statistics-smoke [--capture <png-path>] | "
           "--paddock-performance-smoke | --play-scenario <name> | "
           "--dog-scenario <name> | --dog-render-smoke <name> [--capture <png-path>] | "
           "--sheep-motion-render-smoke [--capture <png-path>] | "
           "--sheep-motion-render-smoke --tick <tick> --view <normal|debug> "
           "--capture <png-path> --state-dump <json-path> | "
           "--sheep-motion-performance-smoke | "
           "--assertion-smoke | "
           "--context-smoke-inject-high-severity]\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view{argv[1]} == "--version") {
        std::cout << "Wide Eye " << WIDE_EYE_VERSION << '\n';
        return EXIT_SUCCESS;
    }
    if (argc == 2 && std::string_view{argv[1]} == "--runtime-smoke") {
        return run_runtime_smoke();
    }
    if (argc == 2 && std::string_view{argv[1]} == "--window-state-smoke") {
        return run_window_state_smoke();
    }
    if (argc == 2 && std::string_view{argv[1]} == "--assertion-smoke") {
        run_assertion_smoke();
    }

    if (argc == 1) {
        return wide_eye::platform::run_interactive_scenario();
    }
    if (argc == 3 && std::string_view{argv[1]} == "--play-scenario" &&
        std::string_view{argv[2]}.size() > 0U) {
        return wide_eye::platform::run_interactive_scenario(argv[2]);
    }
    if (argc == 3 && std::string_view{argv[1]} == "--dog-scenario" &&
        std::string_view{argv[2]}.size() > 0U) {
        return wide_eye::platform::run_dog_headless_scenario(argv[2]);
    }
    if (argc == 3 && std::string_view{argv[1]} == "--dog-render-smoke" &&
        std::string_view{argv[2]}.size() > 0U) {
        return wide_eye::platform::run_dog_render_scenario(argv[2]);
    }
    if (argc == 5 && std::string_view{argv[1]} == "--dog-render-smoke" &&
        std::string_view{argv[2]}.size() > 0U && std::string_view{argv[3]} == "--capture" &&
        std::string_view{argv[4]}.size() > 0U) {
        return wide_eye::platform::run_dog_render_scenario(argv[2], std::filesystem::path{argv[4]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--sheep-motion-render-smoke") {
        return wide_eye::platform::run_sheep_motion_render_scenario();
    }
    if (argc == 4 && std::string_view{argv[1]} == "--sheep-motion-render-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return wide_eye::platform::run_sheep_motion_render_scenario(std::filesystem::path{argv[3]});
    }
    if (argc == 10 && std::string_view{argv[1]} == "--sheep-motion-render-smoke" &&
        std::string_view{argv[2]} == "--tick" && std::string_view{argv[4]} == "--view" &&
        (std::string_view{argv[5]} == "normal" || std::string_view{argv[5]} == "debug") &&
        std::string_view{argv[6]} == "--capture" && std::string_view{argv[7]}.size() > 0U &&
        std::string_view{argv[8]} == "--state-dump" && std::string_view{argv[9]}.size() > 0U) {
        const std::optional<std::uint64_t> tick = parse_tick(argv[3]);
        if (tick.has_value()) {
            return wide_eye::platform::run_sheep_motion_render_scenario(
                std::filesystem::path{argv[7]}, *tick, std::string_view{argv[5]} == "debug",
                std::filesystem::path{argv[9]});
        }
    }
    if (argc == 2 && std::string_view{argv[1]} == "--sheep-motion-performance-smoke") {
        return wide_eye::platform::run_sheep_motion_performance_scenario();
    }
    if (argc == 2 && std::string_view{argv[1]} == "--window-smoke") {
        return wide_eye::platform::run_window_smoke_scenario();
    }
    if (argc == 2 && std::string_view{argv[1]} == "--context-smoke") {
        return wide_eye::platform::run_context_smoke_scenario();
    }
    if (argc == 2 && std::string_view{argv[1]} == "--triangle-smoke") {
        return wide_eye::platform::run_triangle_smoke_scenario();
    }
    if (argc == 2 && std::string_view{argv[1]} == "--voxel-cube-smoke") {
        return wide_eye::platform::run_voxel_cube_smoke_scenario(std::nullopt);
    }
    if (argc == 4 && std::string_view{argv[1]} == "--voxel-cube-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return wide_eye::platform::run_voxel_cube_smoke_scenario(std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--voxel-cube-debug-smoke") {
        return wide_eye::platform::run_voxel_cube_debug_smoke_scenario(std::nullopt);
    }
    if (argc == 4 && std::string_view{argv[1]} == "--voxel-cube-debug-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return wide_eye::platform::run_voxel_cube_debug_smoke_scenario(
            std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--paddock-smoke") {
        return wide_eye::platform::run_handcrafted_paddock_scenario(std::nullopt);
    }
    if (argc == 2 && std::string_view{argv[1]} == "--paddock-performance-smoke") {
        return wide_eye::platform::run_handcrafted_paddock_performance_scenario();
    }
    if (argc == 4 && std::string_view{argv[1]} == "--paddock-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return wide_eye::platform::run_handcrafted_paddock_scenario(std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--paddock-chunk-bounds-smoke") {
        return wide_eye::platform::run_handcrafted_paddock_chunk_bounds_scenario(std::nullopt);
    }
    if (argc == 4 && std::string_view{argv[1]} == "--paddock-chunk-bounds-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return wide_eye::platform::run_handcrafted_paddock_chunk_bounds_scenario(
            std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--paddock-face-normals-smoke") {
        return wide_eye::platform::run_handcrafted_paddock_face_normals_scenario(std::nullopt);
    }
    if (argc == 4 && std::string_view{argv[1]} == "--paddock-face-normals-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return wide_eye::platform::run_handcrafted_paddock_face_normals_scenario(
            std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--paddock-wireframe-smoke") {
        return wide_eye::platform::run_handcrafted_paddock_wireframe_scenario(std::nullopt);
    }
    if (argc == 4 && std::string_view{argv[1]} == "--paddock-wireframe-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return wide_eye::platform::run_handcrafted_paddock_wireframe_scenario(
            std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--paddock-mesh-statistics-smoke") {
        return wide_eye::platform::run_handcrafted_paddock_mesh_statistics_scenario(std::nullopt);
    }
    if (argc == 4 && std::string_view{argv[1]} == "--paddock-mesh-statistics-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return wide_eye::platform::run_handcrafted_paddock_mesh_statistics_scenario(
            std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--context-smoke-inject-high-severity") {
        return wide_eye::platform::run_context_high_severity_scenario();
    }

    print_usage(argv[0]);
    return EXIT_FAILURE;
}
