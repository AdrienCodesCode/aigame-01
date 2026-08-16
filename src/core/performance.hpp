#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace wide_eye::core {

struct DurationStatistics {
    std::uint64_t minimum_ns = 0;
    std::uint64_t median_ns = 0;
    std::uint64_t p95_ns = 0;
    std::uint64_t p99_ns = 0;
    std::uint64_t maximum_ns = 0;
};

[[nodiscard]] std::optional<DurationStatistics>
summarize_durations(std::span<const std::uint64_t> samples);

struct ProcessMemorySample {
    std::uint64_t current_rss_bytes = 0;
    std::uint64_t peak_rss_bytes = 0;
};

[[nodiscard]] std::optional<ProcessMemorySample> sample_process_memory() noexcept;

struct PerformanceBudget {
    std::string_view id;
    std::uint64_t synchronized_frame_p95_ns = 0;
    std::uint64_t synchronized_frame_p99_ns = 0;
    std::uint64_t peak_rss_bytes = 0;
};

inline constexpr PerformanceBudget kLowProfilePerformanceBudget{
    .id = "low-profile-v1",
    .synchronized_frame_p95_ns = 16'670'000,
    .synchronized_frame_p99_ns = 25'000'000,
    .peak_rss_bytes = 1'073'741'824,
};

inline constexpr PerformanceBudget kTracer2LowProfilePerformanceBudget{
    .id = "tracer2-low-profile-v1",
    .synchronized_frame_p95_ns = kLowProfilePerformanceBudget.synchronized_frame_p95_ns,
    .synchronized_frame_p99_ns = kLowProfilePerformanceBudget.synchronized_frame_p99_ns,
    .peak_rss_bytes = 536'870'912,
};

static_assert(kTracer2LowProfilePerformanceBudget.peak_rss_bytes <
              kLowProfilePerformanceBudget.peak_rss_bytes);

[[nodiscard]] bool within_performance_budget(const DurationStatistics& synchronized_frame,
                                             const ProcessMemorySample& memory,
                                             const PerformanceBudget& budget) noexcept;

} // namespace wide_eye::core
