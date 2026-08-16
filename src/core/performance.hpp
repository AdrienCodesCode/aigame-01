#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

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

} // namespace wide_eye::core
