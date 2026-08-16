#pragma once

#include "game/gameplay_simulation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace wide_eye::game {

enum class SpatialGridBuildError : std::uint8_t {
    none,
    invalid_cell_size,
    too_many_members,
    invalid_member,
    duplicate_id,
};

enum class SpatialGridQueryError : std::uint8_t {
    none,
    grid_not_built,
    invalid_subject,
    invalid_radius,
};

struct SpatialNeighbor {
    std::size_t member_index = 0;
    std::uint32_t id = 0;
    double distance = 0.0;

    bool operator==(const SpatialNeighbor&) const = default;
};

struct SpatialGridQueryResult {
    SpatialGridQueryError error = SpatialGridQueryError::none;
    std::size_t neighbor_count = 0;
    std::size_t within_radius_count = 0;
    std::size_t inspected_candidate_count = 0;

    [[nodiscard]] bool truncated() const noexcept {
        return neighbor_count < within_radius_count;
    }

    bool operator==(const SpatialGridQueryResult&) const = default;
};

// Fixed-capacity, ground-plane grid derived from a published sheep snapshot.
// Rebuild copies the query fields, so later snapshot mutation cannot change an
// existing grid. Query output is caller-owned: its span size is the explicit
// neighbor bound, and selected neighbors are ordered by distance, stable sheep
// ID, then source index. Neither rebuild nor query performs heap allocation.
class SheepSpatialGrid {
  public:
    // This is the approved capacity-experiment ceiling, not a gameplay target.
    static constexpr std::size_t kMaximumMemberCount = 1'000;

    [[nodiscard]] SpatialGridBuildError rebuild(std::span<const SheepState> sheep,
                                                double cell_size) noexcept;

    [[nodiscard]] SpatialGridQueryResult
    query_neighbors(std::size_t subject_index, double radius,
                    std::span<SpatialNeighbor> output) const noexcept;

    [[nodiscard]] bool built() const noexcept;
    [[nodiscard]] std::size_t member_count() const noexcept;
    [[nodiscard]] double cell_size() const noexcept;

  private:
    struct Entry {
        std::int64_t cell_x = 0;
        std::int64_t cell_z = 0;
        std::size_t source_index = 0;
        std::uint32_t id = 0;
        double x = 0.0;
        double z = 0.0;
    };

    struct CellRange {
        std::int64_t x = 0;
        std::int64_t z = 0;
        std::size_t first_entry = 0;
        std::size_t entry_count = 0;
    };

    struct RowRange {
        std::int64_t x = 0;
        std::size_t first_cell = 0;
        std::size_t cell_count = 0;
    };

    std::array<Entry, kMaximumMemberCount> entries_{};
    std::array<std::size_t, kMaximumMemberCount> source_to_entry_{};
    std::array<CellRange, kMaximumMemberCount> cells_{};
    std::array<RowRange, kMaximumMemberCount> rows_{};
    std::size_t member_count_ = 0;
    std::size_t cell_count_ = 0;
    std::size_t row_count_ = 0;
    double cell_size_ = 0.0;
    bool built_ = false;
};

} // namespace wide_eye::game
