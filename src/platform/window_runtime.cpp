#include "platform/window_runtime.hpp"

#include "core/performance.hpp"
#include "core/runtime.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <glad/gl.h>
#include <iostream>
#include <string>
#include <vector>

namespace wide_eye::platform {

WindowResult WindowScenarioRunner::initialize() {
    return std::nullopt;
}

WindowResult WindowScenarioRunner::fixed_update(const NamedActionSnapshot&, double) {
    return std::nullopt;
}

WindowResult WindowScenarioRunner::prepare_performance_frame(double) {
    return std::nullopt;
}

void WindowScenarioRunner::release_graphics_resources() {}

namespace {

constexpr int kOpenGlMajor = 4;
constexpr int kOpenGlMinor = 6;

struct GlDebugState {
    mutable std::atomic_size_t high_severity_messages{0};
};

int fail(std::string_view result_name, const WindowFailure& failure) {
    std::cerr << result_name << "=fail\n"
              << "failure_stage=" << failure.stage << '\n';
    if (failure.report_sdl_error) {
        std::cerr << "sdl_error=" << SDL_GetError() << '\n';
    }
    return EXIT_FAILURE;
}

void shut_down(SDL_Window* window, SDL_GLContext context) {
    core::log(core::LogLevel::info, "shutdown_begin",
              "releasing OpenGL, window, and SDL resources");
    if (context != nullptr) {
        SDL_GL_DestroyContext(context);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    core::log(core::LogLevel::info, "shutdown_complete", "SDL shutdown completed");
}

const char* gl_string(GLenum name) {
    const GLubyte* value = glGetString(name);
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

void GLAD_API_PTR gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                    GLsizei length, const GLchar* message,
                                    const void* user_parameter) {
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

bool refresh_pixel_size(SDL_Window* window, WindowState& state) {
    int pixel_width = 0;
    int pixel_height = 0;
    if (!SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height)) {
        return false;
    }
    if (!state.apply(WindowChange::pixel_size, pixel_width, pixel_height)) {
        return false;
    }

    const std::string message = "pixel_width=" + std::to_string(pixel_width) +
                                " pixel_height=" + std::to_string(pixel_height);
    core::log(core::LogLevel::info, "window_resized", message);
    return true;
}

bool apply_window_event(const SDL_Event& event, SDL_Window* window, WindowState& state) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        static_cast<void>(state.apply(WindowChange::close_requested));
        core::log(core::LogLevel::info, "window_close_requested", "clean shutdown requested");
        return true;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_RESIZED:
        return refresh_pixel_size(window, state);
    case SDL_EVENT_WINDOW_MINIMIZED:
        static_cast<void>(state.apply(WindowChange::minimized));
        core::log(core::LogLevel::info, "window_minimized", "rendering suspended while minimized");
        return true;
    case SDL_EVENT_WINDOW_RESTORED:
        static_cast<void>(state.apply(WindowChange::restored));
        core::log(core::LogLevel::info, "window_restored", "rendering resumed after restore");
        return refresh_pixel_size(window, state);
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        static_cast<void>(state.apply(WindowChange::focus_gained));
        core::log(core::LogLevel::info, "window_focus_gained", "window input focus gained");
        return true;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        static_cast<void>(state.apply(WindowChange::focus_lost));
        core::log(core::LogLevel::info, "window_focus_lost", "window input focus lost");
        return true;
    default:
        return true;
    }
}

bool run_window_event_mapping_smoke(SDL_Window* window, WindowState& state) {
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

void write_duration_statistics(std::string_view prefix,
                               const core::DurationStatistics& statistics) {
    std::cout << prefix << "_minimum_ns=" << statistics.minimum_ns << '\n'
              << prefix << "_median_ns=" << statistics.median_ns << '\n'
              << prefix << "_p95_ns=" << statistics.p95_ns << '\n'
              << prefix << "_p99_ns=" << statistics.p99_ns << '\n'
              << prefix << "_maximum_ns=" << statistics.maximum_ns << '\n';
}

} // namespace

int run_window(const WindowRunConfiguration& configuration, WindowScenarioRunner& scenario) {
    core::log(core::LogLevel::info, "runtime_start",
              configuration.bounded ? "starting bounded smoke" : "starting interactive runtime");
    if (configuration.use_opengl) {
        std::cout << "requested_gl=" << kOpenGlMajor << '.' << kOpenGlMinor << '\n' << std::flush;
    }

    SDL_InitFlags init_flags = SDL_INIT_VIDEO;
    if (configuration.enable_input) {
        init_flags |= SDL_INIT_GAMEPAD;
    }
    if (!SDL_Init(init_flags)) {
        return fail(configuration.result_name, {"sdl_init", true});
    }

    if (configuration.use_opengl &&
        (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, kOpenGlMajor) ||
         !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, kOpenGlMinor) ||
         !SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) ||
         !SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG) ||
         !SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) ||
         !SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8))) {
        const int status = fail(configuration.result_name, {"gl_attributes", true});
        SDL_Quit();
        return status;
    }

    SDL_WindowFlags window_flags = configuration.use_opengl ? SDL_WINDOW_OPENGL : 0;
    if (configuration.resizable) {
        window_flags |= SDL_WINDOW_RESIZABLE;
    }
    if (configuration.hidden) {
        window_flags |= SDL_WINDOW_HIDDEN;
    }

    SDL_Window* window =
        SDL_CreateWindow("Wide Eye", configuration.width, configuration.height, window_flags);
    if (window == nullptr) {
        const int status = fail(configuration.result_name, {"window_create", true});
        SDL_Quit();
        return status;
    }

    int initial_pixel_width = 0;
    int initial_pixel_height = 0;
    if (!SDL_GetWindowSizeInPixels(window, &initial_pixel_width, &initial_pixel_height) ||
        initial_pixel_width <= 0 || initial_pixel_height <= 0) {
        const int status = fail(configuration.result_name, {"initial_drawable_size", true});
        shut_down(window, nullptr);
        return status;
    }
    WindowState window_state{initial_pixel_width, initial_pixel_height};

    SDL_GLContext context = nullptr;
    SDL_Gamepad* active_gamepad = nullptr;
    bool debug_callback_installed = false;
    bool relative_mouse_enabled = false;
    GlDebugState debug_state;

    const auto clean_up = [&] {
        scenario.release_graphics_resources();
        if (relative_mouse_enabled) {
            static_cast<void>(SDL_SetWindowRelativeMouseMode(window, false));
            relative_mouse_enabled = false;
        }
        if (active_gamepad != nullptr) {
            SDL_CloseGamepad(active_gamepad);
            active_gamepad = nullptr;
        }
        if (debug_callback_installed) {
            glDebugMessageCallback(nullptr, nullptr);
            debug_callback_installed = false;
        }
        shut_down(window, context);
    };

    if (configuration.use_opengl) {
        context = SDL_GL_CreateContext(window);
        if (context == nullptr) {
            const int status = fail(configuration.result_name, {"context_create", true});
            clean_up();
            return status;
        }
        if (!SDL_GL_MakeCurrent(window, context)) {
            const int status = fail(configuration.result_name, {"make_current", true});
            clean_up();
            return status;
        }

        const int loaded_version = gladLoadGL(SDL_GL_GetProcAddress);
        if (loaded_version == 0) {
            const int status = fail(configuration.result_name, {"gl_loader", true});
            clean_up();
            return status;
        }
        std::cout << "loaded_gl=" << GLAD_VERSION_MAJOR(loaded_version) << '.'
                  << GLAD_VERSION_MINOR(loaded_version) << '\n';

        GLint actual_major = 0;
        GLint actual_minor = 0;
        GLint profile_mask = 0;
        GLint context_flags = 0;
        int depth_bits = 0;
        int stencil_bits = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &actual_major);
        glGetIntegerv(GL_MINOR_VERSION, &actual_minor);
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
        glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
        if (!SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth_bits) ||
            !SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil_bits)) {
            const int status = fail(configuration.result_name, {"framebuffer_attributes", true});
            clean_up();
            return status;
        }

        std::cout << "gl_vendor=" << gl_string(GL_VENDOR) << '\n'
                  << "gl_renderer=" << gl_string(GL_RENDERER) << '\n'
                  << "gl_version=" << gl_string(GL_VERSION) << '\n'
                  << "glsl_version=" << gl_string(GL_SHADING_LANGUAGE_VERSION) << '\n'
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
        const bool depth_buffer_available = !configuration.require_depth_buffer || depth_bits > 0;
        if (!requested_version_met || !core_profile || !debug_context || !depth_buffer_available) {
            const int status = fail(configuration.result_name, {"context_validation", true});
            clean_up();
            return status;
        }

        glDebugMessageCallback(gl_debug_callback, &debug_state);
        debug_callback_installed = true;
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        std::cout << "gl_debug_callback=installed\n";

        if (configuration.inject_high_severity_message) {
            constexpr GLuint message_id = 0x57494445;
            constexpr char message[] = "Wide Eye high-severity callback regression";
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, message_id,
                                 GL_DEBUG_SEVERITY_HIGH, static_cast<GLsizei>(sizeof(message) - 1),
                                 message);
        }

        if (configuration.request_vsync && !SDL_GL_SetSwapInterval(1)) {
            core::log(core::LogLevel::warning, "swap_interval_unavailable", SDL_GetError());
        }
    }

    if (const WindowResult failure = scenario.initialize(); failure.has_value()) {
        const int status = fail(configuration.result_name, *failure);
        clean_up();
        return status;
    }

    core::MonotonicFrameClock frame_clock;
    core::FixedStepAccumulator fixed_step;
    NamedInputState named_input;
    if (configuration.enable_input) {
        if (!SDL_SetWindowRelativeMouseMode(window, true)) {
            const int status = fail(configuration.result_name, {"relative_mouse_capture", true});
            clean_up();
            return status;
        }
        relative_mouse_enabled = true;
        std::cout << "relative_mouse_mode=enabled\n";
    }
    frame_clock.reset();
    std::uint64_t rendered_frames = 0;
    bool running = true;

    if (configuration.validate_window_events &&
        !run_window_event_mapping_smoke(window, window_state)) {
        const int status = fail(configuration.result_name, {"window_event_mapping", false});
        clean_up();
        return status;
    }

    if (configuration.performance_sample_frames > 0) {
        if (!configuration.use_opengl || configuration.performance_scenario.empty()) {
            const int status =
                fail(configuration.result_name, {"performance_configuration", false});
            clean_up();
            return status;
        }

        std::vector<std::uint64_t> presentation_preparation_samples;
        std::vector<std::uint64_t> cpu_submission_samples;
        std::vector<std::uint64_t> gpu_samples;
        std::vector<std::uint64_t> synchronized_frame_samples;
        presentation_preparation_samples.reserve(configuration.performance_sample_frames);
        cpu_submission_samples.reserve(configuration.performance_sample_frames);
        gpu_samples.reserve(configuration.performance_sample_frames);
        synchronized_frame_samples.reserve(configuration.performance_sample_frames);

        GLuint query = 0;
        glGenQueries(1, &query);
        if (query == 0) {
            const int status = fail(configuration.result_name, {"performance_query_create", false});
            clean_up();
            return status;
        }

        const std::uint32_t total_frames =
            configuration.performance_warmup_frames + configuration.performance_sample_frames;
        for (std::uint32_t frame = 0; frame < total_frames; ++frame) {
            const auto frame_begin = std::chrono::steady_clock::now();
            if (const WindowResult failure = scenario.prepare_performance_frame(0.5);
                failure.has_value()) {
                glDeleteQueries(1, &query);
                const int status = fail(configuration.result_name, *failure);
                clean_up();
                return status;
            }
            const auto preparation_end = std::chrono::steady_clock::now();
            glBeginQuery(GL_TIME_ELAPSED, query);
            if (const WindowResult failure = scenario.render_frame(window_state, 0.5);
                failure.has_value()) {
                glEndQuery(GL_TIME_ELAPSED);
                glDeleteQueries(1, &query);
                const int status = fail(configuration.result_name, *failure);
                clean_up();
                return status;
            }
            glEndQuery(GL_TIME_ELAPSED);
            const auto submission_end = std::chrono::steady_clock::now();
            GLuint64 gpu_elapsed_ns = 0;
            glGetQueryObjectui64v(query, GL_QUERY_RESULT, &gpu_elapsed_ns);
            if (!SDL_GL_SwapWindow(window)) {
                glDeleteQueries(1, &query);
                const int status = fail(configuration.result_name, {"swap_window", true});
                clean_up();
                return status;
            }
            const auto frame_end = std::chrono::steady_clock::now();
            ++rendered_frames;

            if (frame >= configuration.performance_warmup_frames) {
                presentation_preparation_samples.push_back(
                    static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                   preparation_end - frame_begin)
                                                   .count()));
                cpu_submission_samples.push_back(
                    static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                   submission_end - preparation_end)
                                                   .count()));
                gpu_samples.push_back(static_cast<std::uint64_t>(gpu_elapsed_ns));
                synchronized_frame_samples.push_back(static_cast<std::uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(frame_end - frame_begin)
                        .count()));
            }
        }
        glDeleteQueries(1, &query);

        const auto preparation_statistics =
            core::summarize_durations(presentation_preparation_samples);
        const auto cpu_statistics = core::summarize_durations(cpu_submission_samples);
        const auto gpu_statistics = core::summarize_durations(gpu_samples);
        const auto synchronized_statistics = core::summarize_durations(synchronized_frame_samples);
        const auto memory = core::sample_process_memory();
        if (!preparation_statistics.has_value() || !cpu_statistics.has_value() ||
            !gpu_statistics.has_value() || !synchronized_statistics.has_value() ||
            !memory.has_value()) {
            const int status = fail(configuration.result_name, {"performance_statistics", false});
            clean_up();
            return status;
        }

        constexpr std::uint64_t kLowP95BudgetNs = 16'670'000;
        constexpr std::uint64_t kLowP99BudgetNs = 25'000'000;
        constexpr std::uint64_t kLowMemoryBudgetBytes = 1'073'741'824;
        const bool within_provisional_low_budget =
            synchronized_statistics->p95_ns <= kLowP95BudgetNs &&
            synchronized_statistics->p99_ns <= kLowP99BudgetNs &&
            memory->peak_rss_bytes <= kLowMemoryBudgetBytes;
        std::cout << "performance_scenario=" << configuration.performance_scenario << '\n'
                  << "performance_warmup_frames=" << configuration.performance_warmup_frames << '\n'
                  << "performance_sample_frames=" << configuration.performance_sample_frames << '\n'
                  << "performance_timing_mode=serialized_gpu_query_and_swap\n";
        write_duration_statistics("snapshot_presentation_preparation", *preparation_statistics);
        write_duration_statistics("cpu_submission", *cpu_statistics);
        write_duration_statistics("gpu_render", *gpu_statistics);
        write_duration_statistics("synchronized_frame", *synchronized_statistics);
        std::cout << "process_rss_bytes=" << memory->current_rss_bytes << '\n'
                  << "process_peak_rss_bytes=" << memory->peak_rss_bytes << '\n'
                  << "provisional_low_p95_budget_ns=" << kLowP95BudgetNs << '\n'
                  << "provisional_low_p99_budget_ns=" << kLowP99BudgetNs << '\n'
                  << "provisional_low_memory_budget_bytes=" << kLowMemoryBudgetBytes << '\n'
                  << "within_provisional_low_budget="
                  << (within_provisional_low_budget ? "yes" : "no") << '\n';
    } else if (configuration.render_bounded_frame) {
        if (const WindowResult failure = scenario.render_frame(window_state, 1.0);
            failure.has_value()) {
            const int status = fail(configuration.result_name, *failure);
            clean_up();
            return status;
        }
        if (!SDL_GL_SwapWindow(window)) {
            const int status = fail(configuration.result_name, {"swap_window", true});
            clean_up();
            return status;
        }
        ++rendered_frames;
    }

    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (configuration.enable_input) {
                named_input.apply_event(event);
            }
            if (configuration.enable_input && event.type == SDL_EVENT_GAMEPAD_ADDED &&
                active_gamepad == nullptr) {
                active_gamepad = SDL_OpenGamepad(event.gdevice.which);
                if (active_gamepad != nullptr) {
                    std::cout << "gamepad_connected=yes\n";
                } else {
                    core::log(core::LogLevel::warning, "gamepad_open_failed", SDL_GetError());
                }
            } else if (configuration.enable_input && event.type == SDL_EVENT_GAMEPAD_REMOVED &&
                       active_gamepad != nullptr &&
                       SDL_GetGamepadID(active_gamepad) == event.gdevice.which) {
                SDL_CloseGamepad(active_gamepad);
                active_gamepad = nullptr;
                named_input.clear_gamepad();
                std::cout << "gamepad_connected=no\n";
            }
            if (!apply_window_event(event, window, window_state)) {
                const int status = fail(configuration.result_name, {"window_event", true});
                clean_up();
                return status;
            }
            if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
                named_input.clear_keyboard();
                named_input.clear_transients();
                if (relative_mouse_enabled) {
                    if (!SDL_SetWindowRelativeMouseMode(window, false)) {
                        const int status =
                            fail(configuration.result_name, {"relative_mouse_release", true});
                        clean_up();
                        return status;
                    }
                    relative_mouse_enabled = false;
                }
            } else if (configuration.enable_input && event.type == SDL_EVENT_WINDOW_FOCUS_GAINED &&
                       !relative_mouse_enabled) {
                named_input.clear_transients();
                if (!SDL_SetWindowRelativeMouseMode(window, true)) {
                    const int status =
                        fail(configuration.result_name, {"relative_mouse_recapture", true});
                    clean_up();
                    return status;
                }
                relative_mouse_enabled = true;
            }
        }
        running = !window_state.close_requested();
        if (configuration.bounded || !running) {
            break;
        }

        const core::FixedStepUpdate update = fixed_step.advance(frame_clock.sample());
        if (update.frame_delta_clamped) {
            core::log(core::LogLevel::warning, "frame_delta_clamped",
                      "frame delta exceeded 250 ms");
        }

        for (std::uint32_t tick = 0; tick < update.ticks; ++tick) {
            if (const WindowResult failure =
                    scenario.fixed_update(named_input.snapshot(), 1.0 / 60.0);
                failure.has_value()) {
                const int status = fail(configuration.result_name, *failure);
                clean_up();
                return status;
            }
            named_input.consume_transients();
        }

        if (window_state.drawable()) {
            if (configuration.render_interactive_frames) {
                if (const WindowResult failure =
                        scenario.render_frame(window_state, update.interpolation_alpha);
                    failure.has_value()) {
                    const int status = fail(configuration.result_name, *failure);
                    clean_up();
                    return status;
                }
            }
            if (!SDL_GL_SwapWindow(window)) {
                const int status = fail(configuration.result_name, {"swap_window", true});
                clean_up();
                return status;
            }
            ++rendered_frames;
        } else {
            SDL_Delay(16);
        }
    }

    if (configuration.use_opengl) {
        scenario.release_graphics_resources();
        const std::size_t high_severity_messages =
            debug_state.high_severity_messages.load(std::memory_order_relaxed);
        std::cout << "gl_debug_high_severity_messages=" << high_severity_messages << '\n';
        glDebugMessageCallback(nullptr, nullptr);
        debug_callback_installed = false;
        if (high_severity_messages > 0) {
            std::cerr << configuration.result_name << "=fail\n"
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
    std::cout << configuration.result_name << "=pass\n";
    return EXIT_SUCCESS;
}

} // namespace wide_eye::platform
