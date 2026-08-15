#include "core/runtime.hpp"
#include "platform/window_state.hpp"
#include "render/png_writer.hpp"
#include "render/triangle_renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;
constexpr int kContextSmokeSize = 64;
constexpr int kOpenGlMajor = 4;
constexpr int kOpenGlMinor = 6;

enum class RunMode {
    interactive,
    window_smoke,
    context_smoke,
    triangle_smoke,
    voxel_cube_smoke,
    voxel_cube_debug_smoke,
    context_smoke_inject_high_severity,
};

using GlGetString = const GLubyte*(APIENTRY*)(GLenum);
using GlGetInteger = void(APIENTRY*)(GLenum, GLint*);
using GlEnable = void(APIENTRY*)(GLenum);

struct GlDebugState {
    mutable std::atomic_size_t high_severity_messages{0};
};

int fail(std::string_view result_name, std::string_view stage, bool report_sdl_error = true) {
    std::cerr << result_name << "=fail\n"
              << "failure_stage=" << stage << '\n';
    if (report_sdl_error) {
        std::cerr << "sdl_error=" << SDL_GetError() << '\n';
    }
    return EXIT_FAILURE;
}

void shut_down(SDL_Window* window, SDL_GLContext context) {
    wide_eye::core::log(wide_eye::core::LogLevel::info, "shutdown_begin",
                        "releasing OpenGL, window, and SDL resources");
    if (context != nullptr) {
        SDL_GL_DestroyContext(context);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    wide_eye::core::log(wide_eye::core::LogLevel::info, "shutdown_complete",
                        "SDL shutdown completed");
}

const char* gl_string(GlGetString get_string, GLenum name) {
    const GLubyte* value = get_string(name);
    return value == nullptr ? "<unavailable>" : reinterpret_cast<const char*>(value);
}

std::string_view gl_debug_source(GLenum source) {
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return "api";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "window_system";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "shader_compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "third_party";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "application";
    case GL_DEBUG_SOURCE_OTHER:
        return "other";
    default:
        return "unknown";
    }
}

std::string_view gl_debug_type(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "deprecated_behavior";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "undefined_behavior";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "portability";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "performance";
    case GL_DEBUG_TYPE_MARKER:
        return "marker";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "push_group";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "pop_group";
    case GL_DEBUG_TYPE_OTHER:
        return "other";
    default:
        return "unknown";
    }
}

std::string_view gl_debug_severity(GLenum severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        return "high";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "medium";
    case GL_DEBUG_SEVERITY_LOW:
        return "low";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "notification";
    default:
        return "unknown";
    }
}

void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar* message, const void* user_parameter) {
    const auto* state = static_cast<const GlDebugState*>(user_parameter);
    if (state != nullptr && severity == GL_DEBUG_SEVERITY_HIGH) {
        state->high_severity_messages.fetch_add(1, std::memory_order_relaxed);
    }

    std::cerr << "gl_debug_message severity=" << gl_debug_severity(severity)
              << " source=" << gl_debug_source(source) << " type=" << gl_debug_type(type)
              << " id=" << id << " text=";
    if (message == nullptr) {
        std::cerr << "<unavailable>";
    } else if (length > 0) {
        std::cerr.write(message, static_cast<std::streamsize>(length));
    }
    std::cerr << '\n';
}

bool refresh_pixel_size(SDL_Window* window, wide_eye::platform::WindowState& state) {
    int pixel_width = 0;
    int pixel_height = 0;
    if (!SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height)) {
        return false;
    }
    if (!state.apply(wide_eye::platform::WindowChange::pixel_size, pixel_width, pixel_height)) {
        return false;
    }

    const std::string message = "pixel_width=" + std::to_string(pixel_width) +
                                " pixel_height=" + std::to_string(pixel_height);
    wide_eye::core::log(wide_eye::core::LogLevel::info, "window_resized", message);
    return true;
}

bool apply_window_event(const SDL_Event& event, SDL_Window* window,
                        wide_eye::platform::WindowState& state) {
    using wide_eye::platform::WindowChange;

    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        static_cast<void>(state.apply(WindowChange::close_requested));
        wide_eye::core::log(wide_eye::core::LogLevel::info, "window_close_requested",
                            "clean shutdown requested");
        return true;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_RESIZED:
        return refresh_pixel_size(window, state);
    case SDL_EVENT_WINDOW_MINIMIZED:
        static_cast<void>(state.apply(WindowChange::minimized));
        wide_eye::core::log(wide_eye::core::LogLevel::info, "window_minimized",
                            "rendering suspended while minimized");
        return true;
    case SDL_EVENT_WINDOW_RESTORED:
        static_cast<void>(state.apply(WindowChange::restored));
        wide_eye::core::log(wide_eye::core::LogLevel::info, "window_restored",
                            "rendering resumed after restore");
        return refresh_pixel_size(window, state);
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        static_cast<void>(state.apply(WindowChange::focus_gained));
        wide_eye::core::log(wide_eye::core::LogLevel::info, "window_focus_gained",
                            "window input focus gained");
        return true;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        static_cast<void>(state.apply(WindowChange::focus_lost));
        wide_eye::core::log(wide_eye::core::LogLevel::info, "window_focus_lost",
                            "window input focus lost");
        return true;
    default:
        return true;
    }
}

bool run_window_event_mapping_smoke(SDL_Window* window, wide_eye::platform::WindowState& state) {
    SDL_Event event{};
    event.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
    if (!apply_window_event(event, window, state)) {
        return false;
    }

    event.type = SDL_EVENT_WINDOW_MINIMIZED;
    if (!apply_window_event(event, window, state) || state.drawable()) {
        return false;
    }

    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    if (!apply_window_event(event, window, state) || state.focused()) {
        return false;
    }

    event.type = SDL_EVENT_WINDOW_RESTORED;
    if (!apply_window_event(event, window, state) || !state.drawable()) {
        return false;
    }

    event.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    if (!apply_window_event(event, window, state) || !state.focused()) {
        return false;
    }

    event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    if (!apply_window_event(event, window, state) || !state.close_requested()) {
        return false;
    }

    std::cout << "sdl_window_events_handled=yes\n";
    return true;
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

int run_window(RunMode mode, const std::optional<std::filesystem::path>& capture_path) {
    const bool use_opengl = mode != RunMode::window_smoke;
    const bool smoke_mode = mode != RunMode::interactive;
    const bool render_triangle = mode == RunMode::triangle_smoke;
    const bool render_voxel_cube = mode == RunMode::interactive ||
                                   mode == RunMode::voxel_cube_smoke ||
                                   mode == RunMode::voxel_cube_debug_smoke;
    const bool render_voxel_cube_debug = mode == RunMode::voxel_cube_debug_smoke;
    const bool render_scene = render_triangle || render_voxel_cube;
    const bool inject_high_severity = mode == RunMode::context_smoke_inject_high_severity;
    const std::string_view result_name = mode == RunMode::window_smoke       ? "window_result"
                                         : mode == RunMode::triangle_smoke   ? "triangle_result"
                                         : mode == RunMode::voxel_cube_smoke ? "voxel_cube_result"
                                         : mode == RunMode::voxel_cube_debug_smoke
                                             ? "voxel_cube_debug_result"
                                             : "context_result";

    wide_eye::core::log(wide_eye::core::LogLevel::info, "runtime_start",
                        smoke_mode ? "starting bounded smoke" : "starting interactive runtime");
    if (use_opengl) {
        std::cout << "requested_gl=" << kOpenGlMajor << '.' << kOpenGlMinor << '\n' << std::flush;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return fail(result_name, "sdl_init");
    }

    if (use_opengl &&
        (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, kOpenGlMajor) ||
         !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, kOpenGlMinor) ||
         !SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) ||
         !SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG) ||
         !SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) ||
         !SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8))) {
        const int status = fail(result_name, "gl_attributes");
        SDL_Quit();
        return status;
    }

    const int window_width = smoke_mode && use_opengl ? kContextSmokeSize : kWindowWidth;
    const int window_height = smoke_mode && use_opengl ? kContextSmokeSize : kWindowHeight;
    SDL_WindowFlags window_flags = use_opengl ? SDL_WINDOW_OPENGL : 0;
    if (mode == RunMode::interactive) {
        window_flags |= SDL_WINDOW_RESIZABLE;
    } else if (use_opengl) {
        window_flags |= SDL_WINDOW_HIDDEN;
    }

    SDL_Window* window = SDL_CreateWindow("Wide Eye", window_width, window_height, window_flags);
    if (window == nullptr) {
        const int status = fail(result_name, "window_create");
        SDL_Quit();
        return status;
    }

    int initial_pixel_width = 0;
    int initial_pixel_height = 0;
    if (!SDL_GetWindowSizeInPixels(window, &initial_pixel_width, &initial_pixel_height) ||
        initial_pixel_width <= 0 || initial_pixel_height <= 0) {
        const int status = fail(result_name, "initial_drawable_size");
        shut_down(window, nullptr);
        return status;
    }
    wide_eye::platform::WindowState window_state{initial_pixel_width, initial_pixel_height};

    SDL_GLContext context = nullptr;
    PFNGLDEBUGMESSAGECALLBACKPROC debug_message_callback = nullptr;
    bool debug_callback_installed = false;
    GlDebugState debug_state;
    std::optional<wide_eye::render::TriangleRenderer> renderer;

    const auto clean_up = [&] {
        renderer.reset();
        if (debug_callback_installed) {
            debug_message_callback(nullptr, nullptr);
            debug_callback_installed = false;
        }
        shut_down(window, context);
    };

    if (use_opengl) {
        context = SDL_GL_CreateContext(window);
        if (context == nullptr) {
            const int status = fail(result_name, "context_create");
            clean_up();
            return status;
        }
        if (!SDL_GL_MakeCurrent(window, context)) {
            const int status = fail(result_name, "make_current");
            clean_up();
            return status;
        }

        const auto get_string = reinterpret_cast<GlGetString>(SDL_GL_GetProcAddress("glGetString"));
        const auto get_integer =
            reinterpret_cast<GlGetInteger>(SDL_GL_GetProcAddress("glGetIntegerv"));
        const auto gl_enable = reinterpret_cast<GlEnable>(SDL_GL_GetProcAddress("glEnable"));
        debug_message_callback = reinterpret_cast<PFNGLDEBUGMESSAGECALLBACKPROC>(
            SDL_GL_GetProcAddress("glDebugMessageCallback"));
        const auto debug_message_control = reinterpret_cast<PFNGLDEBUGMESSAGECONTROLPROC>(
            SDL_GL_GetProcAddress("glDebugMessageControl"));
        const auto debug_message_insert = reinterpret_cast<PFNGLDEBUGMESSAGEINSERTPROC>(
            SDL_GL_GetProcAddress("glDebugMessageInsert"));
        if (get_string == nullptr || get_integer == nullptr || gl_enable == nullptr ||
            debug_message_callback == nullptr || debug_message_control == nullptr ||
            (inject_high_severity && debug_message_insert == nullptr)) {
            const int status = fail(result_name, "gl_entry_points");
            clean_up();
            return status;
        }

        GLint actual_major = 0;
        GLint actual_minor = 0;
        GLint profile_mask = 0;
        GLint context_flags = 0;
        int depth_bits = 0;
        int stencil_bits = 0;
        get_integer(GL_MAJOR_VERSION, &actual_major);
        get_integer(GL_MINOR_VERSION, &actual_minor);
        get_integer(GL_CONTEXT_PROFILE_MASK, &profile_mask);
        get_integer(GL_CONTEXT_FLAGS, &context_flags);
        if (!SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth_bits) ||
            !SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil_bits)) {
            const int status = fail(result_name, "framebuffer_attributes");
            clean_up();
            return status;
        }

        std::cout << "gl_vendor=" << gl_string(get_string, GL_VENDOR) << '\n'
                  << "gl_renderer=" << gl_string(get_string, GL_RENDERER) << '\n'
                  << "gl_version=" << gl_string(get_string, GL_VERSION) << '\n'
                  << "glsl_version=" << gl_string(get_string, GL_SHADING_LANGUAGE_VERSION) << '\n'
                  << "actual_gl=" << actual_major << '.' << actual_minor << '\n'
                  << "core_profile="
                  << ((profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) != 0 ? "yes" : "no") << '\n'
                  << "debug_context="
                  << ((context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0 ? "yes" : "no") << '\n'
                  << "depth_bits=" << depth_bits << '\n'
                  << "stencil_bits=" << stencil_bits << '\n';

        const bool requested_version_met =
            actual_major > kOpenGlMajor ||
            (actual_major == kOpenGlMajor && actual_minor >= kOpenGlMinor);
        const bool core_profile = (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) != 0;
        const bool debug_context = (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0;
        const bool depth_buffer_available = !render_voxel_cube || depth_bits > 0;
        if (!requested_version_met || !core_profile || !debug_context || !depth_buffer_available) {
            const int status = fail(result_name, "context_validation");
            clean_up();
            return status;
        }

        debug_message_callback(gl_debug_callback, &debug_state);
        debug_callback_installed = true;
        gl_enable(GL_DEBUG_OUTPUT);
        gl_enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        debug_message_control(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        std::cout << "gl_debug_callback=installed\n";

        if (inject_high_severity) {
            constexpr GLuint message_id = 0x57494445;
            constexpr char message[] = "Wide Eye high-severity callback regression";
            debug_message_insert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, message_id,
                                 GL_DEBUG_SEVERITY_HIGH, static_cast<GLsizei>(sizeof(message) - 1),
                                 message);
        }

        if (mode == RunMode::interactive && !SDL_GL_SetSwapInterval(1)) {
            wide_eye::core::log(wide_eye::core::LogLevel::warning, "swap_interval_unavailable",
                                SDL_GetError());
        }

        if (render_scene) {
            renderer.emplace();
            if (!renderer->initialize(std::cerr)) {
                const int status = fail(result_name, "renderer_init", false);
                clean_up();
                return status;
            }
        }
    }

    wide_eye::core::MonotonicFrameClock frame_clock;
    wide_eye::core::FixedStepAccumulator fixed_step;
    frame_clock.reset();
    std::uint64_t rendered_frames = 0;
    bool running = true;

    if (mode == RunMode::window_smoke && !run_window_event_mapping_smoke(window, window_state)) {
        const int status = fail(result_name, "window_event_mapping", false);
        clean_up();
        return status;
    }

    if (smoke_mode && render_scene) {
        if (render_triangle) {
            renderer->render(window_state.pixel_width(), window_state.pixel_height());
            std::cout << "triangle_draw=issued\n";
            const wide_eye::render::TriangleSample sample =
                renderer->sample_center(window_state.pixel_width(), window_state.pixel_height());
            const bool sample_matches = wide_eye::render::is_expected_triangle_sample(sample);
            std::cout << "triangle_center_rgba=" << static_cast<unsigned int>(sample.red) << ','
                      << static_cast<unsigned int>(sample.green) << ','
                      << static_cast<unsigned int>(sample.blue) << ','
                      << static_cast<unsigned int>(sample.alpha) << '\n'
                      << "triangle_center_matches=" << (sample_matches ? "yes" : "no") << '\n';
            if (!sample_matches) {
                const int status = fail(result_name, "triangle_framebuffer_oracle", false);
                clean_up();
                return status;
            }
        } else {
            if (render_voxel_cube_debug) {
                renderer->render_voxel_cube_wireframe(window_state.pixel_width(),
                                                      window_state.pixel_height());
            } else {
                renderer->render_voxel_cube(window_state.pixel_width(),
                                            window_state.pixel_height());
            }
            std::cout << "scenario=voxel_cube_smoke\n"
                      << "voxel_cube_view="
                      << (render_voxel_cube_debug ? "wireframe_debug" : "normal") << '\n'
                      << "voxel_cube_draw=issued\n";
            const wide_eye::render::VoxelCubeSample sample = renderer->sample_voxel_cube_center(
                window_state.pixel_width(), window_state.pixel_height());
            const bool depth_state_matches = sample.depth_test_enabled &&
                                             sample.depth_function_less &&
                                             sample.depth_write_enabled;
            const bool sample_matches =
                render_voxel_cube_debug ? depth_state_matches
                                        : wide_eye::render::is_expected_voxel_cube_sample(sample);
            std::cout << "voxel_cube_center_rgba=" << static_cast<unsigned int>(sample.color.red)
                      << ',' << static_cast<unsigned int>(sample.color.green) << ','
                      << static_cast<unsigned int>(sample.color.blue) << ','
                      << static_cast<unsigned int>(sample.color.alpha) << '\n'
                      << "voxel_cube_center_depth=" << sample.depth << '\n'
                      << "depth_test_enabled=" << (sample.depth_test_enabled ? "yes" : "no") << '\n'
                      << "depth_function_less=" << (sample.depth_function_less ? "yes" : "no")
                      << '\n'
                      << "depth_write_enabled=" << (sample.depth_write_enabled ? "yes" : "no")
                      << '\n';
            if (render_voxel_cube_debug) {
                std::cout << "voxel_cube_depth_state_matches="
                          << (depth_state_matches ? "yes" : "no") << '\n';
            } else {
                std::cout << "voxel_cube_center_matches=" << (sample_matches ? "yes" : "no")
                          << '\n';
            }
            if (!sample_matches) {
                const int status = fail(result_name,
                                        render_voxel_cube_debug ? "voxel_cube_debug_depth_state"
                                                                : "voxel_cube_depth_oracle",
                                        false);
                clean_up();
                return status;
            }

            if (capture_path.has_value() || render_voxel_cube_debug) {
                const std::optional<wide_eye::render::Rgba8Frame> frame = renderer->capture_rgba8(
                    window_state.pixel_width(), window_state.pixel_height(), std::cerr);
                if (!frame.has_value()) {
                    const int status = fail(result_name, "capture_readback", false);
                    clean_up();
                    return status;
                }
                if (render_voxel_cube_debug) {
                    const std::size_t visible_pixels =
                        wide_eye::render::count_voxel_cube_wireframe_pixels(*frame);
                    const bool debug_frame_matches =
                        wide_eye::render::is_expected_voxel_cube_wireframe(*frame);
                    std::cout << "wireframe_visible_pixels=" << visible_pixels << '\n'
                              << "wireframe_frame_matches=" << (debug_frame_matches ? "yes" : "no")
                              << '\n';
                    if (!debug_frame_matches) {
                        const int status = fail(result_name, "voxel_cube_wireframe_oracle", false);
                        clean_up();
                        return status;
                    }
                }
                if (capture_path.has_value()) {
                    if (!wide_eye::render::write_png_rgba8(
                            *capture_path, frame->width, frame->height, frame->pixels, std::cerr)) {
                        const int status = fail(result_name, "capture_write", false);
                        clean_up();
                        return status;
                    }
                    std::cout << "capture_path=" << capture_path->string() << '\n'
                              << "capture_width=" << frame->width << '\n'
                              << "capture_height=" << frame->height << '\n'
                              << "capture_format=png_rgba8\n"
                              << "capture_result=pass\n";
                }
            }
        }

        if (!SDL_GL_SwapWindow(window)) {
            const int status = fail(result_name, "swap_window");
            clean_up();
            return status;
        }
        ++rendered_frames;
    }

    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (!apply_window_event(event, window, window_state)) {
                const int status = fail(result_name, "window_event");
                clean_up();
                return status;
            }
        }
        running = !window_state.close_requested();
        if (smoke_mode || !running) {
            break;
        }

        const wide_eye::core::FixedStepUpdate update = fixed_step.advance(frame_clock.sample());
        if (update.frame_delta_clamped) {
            wide_eye::core::log(wide_eye::core::LogLevel::warning, "frame_delta_clamped",
                                "frame delta exceeded 250 ms");
        }

        if (window_state.drawable()) {
            renderer->render_voxel_cube(window_state.pixel_width(), window_state.pixel_height());
            if (!SDL_GL_SwapWindow(window)) {
                const int status = fail(result_name, "swap_window");
                clean_up();
                return status;
            }
            ++rendered_frames;
        } else {
            SDL_Delay(16);
        }
    }

    if (use_opengl) {
        renderer.reset();
        const std::size_t high_severity_messages =
            debug_state.high_severity_messages.load(std::memory_order_relaxed);
        std::cout << "gl_debug_high_severity_messages=" << high_severity_messages << '\n';
        debug_message_callback(nullptr, nullptr);
        debug_callback_installed = false;
        if (high_severity_messages > 0) {
            std::cerr << result_name << "=fail\n"
                      << "failure_stage=gl_debug_high_severity\n";
            clean_up();
            return EXIT_FAILURE;
        }
    }

    const int sdl_version = SDL_GetVersion();
    const char* video_driver = SDL_GetCurrentVideoDriver();
    std::cout << "sdl_version=" << SDL_VERSIONNUM_MAJOR(sdl_version) << '.'
              << SDL_VERSIONNUM_MINOR(sdl_version) << '.' << SDL_VERSIONNUM_MICRO(sdl_version)
              << '\n'
              << "video_driver=" << (video_driver == nullptr ? "<unavailable>" : video_driver)
              << '\n'
              << "rendered_frames=" << rendered_frames << '\n'
              << "fixed_ticks=" << fixed_step.total_ticks() << '\n';

    clean_up();
    std::cout << result_name << "=pass\n";
    return EXIT_SUCCESS;
}

void print_usage(std::string_view executable) {
    std::cerr
        << "usage: " << executable
        << " [--version | --window-smoke | --context-smoke | --triangle-smoke | "
           "--voxel-cube-smoke [--capture <png-path>] | --window-state-smoke | --runtime-smoke | "
           "--voxel-cube-debug-smoke [--capture <png-path>] | "
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
        return run_window(RunMode::interactive, std::nullopt);
    }
    if (argc == 2 && std::string_view{argv[1]} == "--window-smoke") {
        return run_window(RunMode::window_smoke, std::nullopt);
    }
    if (argc == 2 && std::string_view{argv[1]} == "--context-smoke") {
        return run_window(RunMode::context_smoke, std::nullopt);
    }
    if (argc == 2 && std::string_view{argv[1]} == "--triangle-smoke") {
        return run_window(RunMode::triangle_smoke, std::nullopt);
    }
    if (argc == 2 && std::string_view{argv[1]} == "--voxel-cube-smoke") {
        return run_window(RunMode::voxel_cube_smoke, std::nullopt);
    }
    if (argc == 4 && std::string_view{argv[1]} == "--voxel-cube-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return run_window(RunMode::voxel_cube_smoke, std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--voxel-cube-debug-smoke") {
        return run_window(RunMode::voxel_cube_debug_smoke, std::nullopt);
    }
    if (argc == 4 && std::string_view{argv[1]} == "--voxel-cube-debug-smoke" &&
        std::string_view{argv[2]} == "--capture" && std::string_view{argv[3]}.size() > 0U) {
        return run_window(RunMode::voxel_cube_debug_smoke, std::filesystem::path{argv[3]});
    }
    if (argc == 2 && std::string_view{argv[1]} == "--context-smoke-inject-high-severity") {
        return run_window(RunMode::context_smoke_inject_high_severity, std::nullopt);
    }

    print_usage(argv[0]);
    return EXIT_FAILURE;
}
