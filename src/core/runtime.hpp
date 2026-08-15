#pragma once

#include <chrono>
#include <cstdint>
#include <source_location>
#include <string_view>

namespace wide_eye::core {

enum class LogLevel : std::uint8_t {
    debug,
    info,
    warning,
    error,
    fatal,
};

void log(LogLevel level, std::string_view event, std::string_view message);

[[noreturn]] void
assertion_failed(std::string_view expression, std::string_view message,
                 const std::source_location& location = std::source_location::current()) noexcept;

class MonotonicFrameClock {
  public:
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::nanoseconds;
    using TimePoint = std::chrono::time_point<Clock, Duration>;

    static_assert(Clock::is_steady);

    void reset();
    [[nodiscard]] Duration sample();

  private:
    TimePoint previous_{};
    bool has_previous_ = false;
};

struct FixedStepUpdate {
    std::uint32_t ticks = 0;
    double interpolation_alpha = 0.0;
    bool frame_delta_clamped = false;
};

class FixedStepAccumulator {
  public:
    static constexpr std::uint32_t ticks_per_second = 60;
    static constexpr std::chrono::nanoseconds maximum_frame_delta = std::chrono::milliseconds{250};

    [[nodiscard]] FixedStepUpdate advance(std::chrono::nanoseconds frame_delta);
    [[nodiscard]] std::uint64_t total_ticks() const;

  private:
    static constexpr std::uint64_t scaled_tick = 1'000'000'000ULL;

    std::uint64_t scaled_accumulator_ = 0;
    std::uint64_t total_ticks_ = 0;
};

} // namespace wide_eye::core

#define WIDE_EYE_ASSERT(condition, message)                                                        \
    ((condition) ? static_cast<void>(0) : ::wide_eye::core::assertion_failed(#condition, (message)))
