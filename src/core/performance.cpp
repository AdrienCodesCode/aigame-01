#include "core/performance.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <vector>

#if defined(_WIN32)
// clang-format off: Psapi.h requires the Windows base types declared first.
#include <Windows.h>
#include <Psapi.h>
// clang-format on
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace wide_eye::core {
namespace {

[[nodiscard]] std::size_t nearest_rank_index(std::size_t sample_count,
                                             std::size_t percentile) noexcept {
    const std::size_t rank = (sample_count * percentile + 99U) / 100U;
    return std::max<std::size_t>(rank, 1U) - 1U;
}

} // namespace

std::optional<DurationStatistics> summarize_durations(std::span<const std::uint64_t> samples) {
    if (samples.empty()) {
        return std::nullopt;
    }
    std::vector<std::uint64_t> ordered{samples.begin(), samples.end()};
    std::sort(ordered.begin(), ordered.end());
    return DurationStatistics{
        .minimum_ns = ordered.front(),
        .median_ns = ordered[nearest_rank_index(ordered.size(), 50U)],
        .p95_ns = ordered[nearest_rank_index(ordered.size(), 95U)],
        .p99_ns = ordered[nearest_rank_index(ordered.size(), 99U)],
        .maximum_ns = ordered.back(),
    };
}

std::optional<ProcessMemorySample> sample_process_memory() noexcept {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)) == FALSE) {
        return std::nullopt;
    }
    return ProcessMemorySample{
        .current_rss_bytes = static_cast<std::uint64_t>(counters.WorkingSetSize),
        .peak_rss_bytes = static_cast<std::uint64_t>(counters.PeakWorkingSetSize),
    };
#elif defined(__linux__)
    std::ifstream statm{"/proc/self/statm"};
    std::uint64_t total_pages = 0;
    std::uint64_t resident_pages = 0;
    if (!(statm >> total_pages >> resident_pages)) {
        return std::nullopt;
    }
    static_cast<void>(total_pages);
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0 || resident_pages > std::numeric_limits<std::uint64_t>::max() /
                                               static_cast<std::uint64_t>(page_size)) {
        return std::nullopt;
    }

    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
        return std::nullopt;
    }
    constexpr std::uint64_t kBytesPerKibibyte = 1024;
    return ProcessMemorySample{
        .current_rss_bytes = resident_pages * static_cast<std::uint64_t>(page_size),
        .peak_rss_bytes = static_cast<std::uint64_t>(usage.ru_maxrss) * kBytesPerKibibyte,
    };
#else
    return std::nullopt;
#endif
}

} // namespace wide_eye::core
