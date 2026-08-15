#include "core/runtime.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <mutex>

namespace wide_eye::core {
namespace {

std::mutex log_mutex;

std::string_view level_name(LogLevel level) {
    switch (level) {
    case LogLevel::debug:
        return "debug";
    case LogLevel::info:
        return "info";
    case LogLevel::warning:
        return "warning";
    case LogLevel::error:
        return "error";
    case LogLevel::fatal:
        return "fatal";
    }

    return "unknown";
}

} // namespace

void log(LogLevel level, std::string_view event, std::string_view message) {
    const std::scoped_lock lock{log_mutex};
    std::ostream& output =
        level == LogLevel::debug || level == LogLevel::info ? std::cout : std::cerr;
    output << "log level=" << level_name(level) << " event=" << event << " message=\"" << message
           << "\"\n"
           << std::flush;
}

[[noreturn]] void assertion_failed(std::string_view expression, std::string_view message,
                                   const std::source_location& location) noexcept {
    const std::scoped_lock lock{log_mutex};
    std::cerr << "log level=fatal event=assertion_failed expression=\"" << expression
              << "\" message=\"" << message << "\" file=\"" << location.file_name()
              << "\" line=" << location.line() << '\n'
              << std::flush;
    std::_Exit(EXIT_FAILURE);
}

void MonotonicFrameClock::reset() {
    previous_ = std::chrono::time_point_cast<Duration>(Clock::now());
    has_previous_ = true;
}

MonotonicFrameClock::Duration MonotonicFrameClock::sample() {
    const TimePoint current = std::chrono::time_point_cast<Duration>(Clock::now());
    if (!has_previous_) {
        previous_ = current;
        has_previous_ = true;
        return Duration::zero();
    }

    WIDE_EYE_ASSERT(current >= previous_, "steady clock moved backwards");
    const Duration elapsed = current - previous_;
    previous_ = current;
    return elapsed;
}

FixedStepUpdate FixedStepAccumulator::advance(std::chrono::nanoseconds frame_delta) {
    WIDE_EYE_ASSERT(frame_delta >= std::chrono::nanoseconds::zero(),
                    "fixed-step frame delta must be non-negative");

    const bool clamped = frame_delta > maximum_frame_delta;
    const auto accepted_delta = std::min(frame_delta, maximum_frame_delta);
    const auto accepted_nanoseconds = static_cast<std::uint64_t>(accepted_delta.count());
    scaled_accumulator_ += accepted_nanoseconds * ticks_per_second;

    const auto pending_ticks = scaled_accumulator_ / scaled_tick;
    WIDE_EYE_ASSERT(pending_ticks <= static_cast<std::uint64_t>(UINT32_MAX),
                    "fixed-step tick count exceeds result capacity");
    const auto ticks = static_cast<std::uint32_t>(pending_ticks);
    scaled_accumulator_ %= scaled_tick;
    total_ticks_ += ticks;

    return FixedStepUpdate{
        .ticks = ticks,
        .interpolation_alpha =
            static_cast<double>(scaled_accumulator_) / static_cast<double>(scaled_tick),
        .frame_delta_clamped = clamped,
    };
}

std::uint64_t FixedStepAccumulator::total_ticks() const {
    return total_ticks_;
}

} // namespace wide_eye::core
