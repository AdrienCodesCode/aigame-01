#include "core/performance.hpp"

#include <array>
#include <cstdlib>
#include <iostream>

int main() {
    constexpr std::array<std::uint64_t, 10> samples{10, 1, 9, 2, 8, 3, 7, 4, 6, 5};
    const auto statistics = wide_eye::core::summarize_durations(samples);
    if (!statistics.has_value() || statistics->minimum_ns != 1 || statistics->median_ns != 5 ||
        statistics->p95_ns != 10 || statistics->p99_ns != 10 || statistics->maximum_ns != 10 ||
        wide_eye::core::summarize_durations(std::span<const std::uint64_t>{}).has_value()) {
        std::cerr << "performance_failure=duration_statistics\n";
        return EXIT_FAILURE;
    }
    const auto memory = wide_eye::core::sample_process_memory();
    if (!memory.has_value() || memory->current_rss_bytes == 0 || memory->peak_rss_bytes == 0 ||
        memory->peak_rss_bytes < memory->current_rss_bytes) {
        std::cerr << "performance_failure=process_memory\n";
        return EXIT_FAILURE;
    }
    std::cout << "duration_statistics=nearest_rank\n"
              << "process_rss_bytes=" << memory->current_rss_bytes << '\n'
              << "process_peak_rss_bytes=" << memory->peak_rss_bytes << '\n'
              << "performance_result=pass\n";
    return EXIT_SUCCESS;
}
