#include "game/sheep_spatial_grid.hpp"

#include <algorithm>
#include <cmath>

namespace wide_eye::game {
namespace {

[[nodiscard]] bool cell_coordinate(double position, double cell_size,
                                   std::int64_t& output) noexcept {
    constexpr long double minimum = -static_cast<long double>(std::int64_t{1} << 62);
    constexpr long double maximum = static_cast<long double>(std::int64_t{1} << 62);
    const long double coordinate =
        std::floor(static_cast<long double>(position) / static_cast<long double>(cell_size));
    if (coordinate < minimum || coordinate > maximum) {
        return false;
    }
    output = static_cast<std::int64_t>(coordinate);
    return true;
}

[[nodiscard]] std::int64_t saturated_cell_coordinate(long double position,
                                                     double cell_size) noexcept {
    constexpr std::int64_t minimum = -(std::int64_t{1} << 62);
    constexpr std::int64_t maximum = std::int64_t{1} << 62;
    const long double coordinate = std::floor(position / static_cast<long double>(cell_size));
    if (coordinate <= static_cast<long double>(minimum)) {
        return minimum;
    }
    if (coordinate >= static_cast<long double>(maximum)) {
        return maximum;
    }
    return static_cast<std::int64_t>(coordinate);
}

[[nodiscard]] bool neighbor_precedes(const SpatialNeighbor& left,
                                     const SpatialNeighbor& right) noexcept {
    if (left.distance != right.distance) {
        return left.distance < right.distance;
    }
    if (left.id != right.id) {
        return left.id < right.id;
    }
    return left.member_index < right.member_index;
}

} // namespace

SpatialGridBuildError SheepSpatialGrid::rebuild(std::span<const SheepState> sheep,
                                                double cell_size) noexcept {
    built_ = false;
    member_count_ = 0;
    cell_count_ = 0;
    row_count_ = 0;
    cell_size_ = 0.0;

    if (!std::isfinite(cell_size) || cell_size <= 0.0) {
        return SpatialGridBuildError::invalid_cell_size;
    }
    if (sheep.size() > kMaximumMemberCount) {
        return SpatialGridBuildError::too_many_members;
    }

    std::array<std::uint32_t, kMaximumMemberCount> sorted_ids{};
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        const SheepState& member = sheep[index];
        Entry& entry = entries_[index];
        if (member.id == 0 || !std::isfinite(member.position.x) ||
            !std::isfinite(member.position.z) ||
            !cell_coordinate(member.position.x, cell_size, entry.cell_x) ||
            !cell_coordinate(member.position.z, cell_size, entry.cell_z)) {
            return SpatialGridBuildError::invalid_member;
        }
        entry.source_index = index;
        entry.id = member.id;
        entry.x = member.position.x;
        entry.z = member.position.z;
        sorted_ids[index] = member.id;
    }

    std::sort(sorted_ids.begin(), sorted_ids.begin() + sheep.size());
    for (std::size_t index = 1; index < sheep.size(); ++index) {
        if (sorted_ids[index] == sorted_ids[index - 1]) {
            return SpatialGridBuildError::duplicate_id;
        }
    }

    std::sort(entries_.begin(), entries_.begin() + sheep.size(),
              [](const Entry& left, const Entry& right) {
                  if (left.cell_x != right.cell_x) {
                      return left.cell_x < right.cell_x;
                  }
                  if (left.cell_z != right.cell_z) {
                      return left.cell_z < right.cell_z;
                  }
                  if (left.id != right.id) {
                      return left.id < right.id;
                  }
                  return left.source_index < right.source_index;
              });

    member_count_ = sheep.size();
    for (std::size_t index = 0; index < member_count_; ++index) {
        source_to_entry_[entries_[index].source_index] = index;
        if (index == 0 || entries_[index].cell_x != entries_[index - 1].cell_x ||
            entries_[index].cell_z != entries_[index - 1].cell_z) {
            cells_[cell_count_] = {.x = entries_[index].cell_x,
                                   .z = entries_[index].cell_z,
                                   .first_entry = index,
                                   .entry_count = 1};
            ++cell_count_;
        } else {
            ++cells_[cell_count_ - 1].entry_count;
        }
    }
    for (std::size_t index = 0; index < cell_count_; ++index) {
        if (index == 0 || cells_[index].x != cells_[index - 1].x) {
            rows_[row_count_] = {.x = cells_[index].x, .first_cell = index, .cell_count = 1};
            ++row_count_;
        } else {
            ++rows_[row_count_ - 1].cell_count;
        }
    }

    cell_size_ = cell_size;
    built_ = true;
    return SpatialGridBuildError::none;
}

SpatialGridQueryResult
SheepSpatialGrid::query_neighbors(std::size_t subject_index, double radius,
                                  std::span<SpatialNeighbor> output) const noexcept {
    if (!built_) {
        return {.error = SpatialGridQueryError::grid_not_built};
    }
    if (subject_index >= member_count_) {
        return {.error = SpatialGridQueryError::invalid_subject};
    }
    if (!std::isfinite(radius) || radius < 0.0) {
        return {.error = SpatialGridQueryError::invalid_radius};
    }

    SpatialGridQueryResult result;
    const Entry& subject = entries_[source_to_entry_[subject_index]];
    const long double query_radius = static_cast<long double>(radius);
    const std::int64_t minimum_x =
        saturated_cell_coordinate(static_cast<long double>(subject.x) - query_radius, cell_size_);
    const std::int64_t maximum_x =
        saturated_cell_coordinate(static_cast<long double>(subject.x) + query_radius, cell_size_);
    const std::int64_t minimum_z =
        saturated_cell_coordinate(static_cast<long double>(subject.z) - query_radius, cell_size_);
    const std::int64_t maximum_z =
        saturated_cell_coordinate(static_cast<long double>(subject.z) + query_radius, cell_size_);

    const auto first_row =
        std::lower_bound(rows_.begin(), rows_.begin() + row_count_, minimum_x,
                         [](const RowRange& row, std::int64_t x) { return row.x < x; });
    for (auto row = first_row; row != rows_.begin() + row_count_ && row->x <= maximum_x; ++row) {
        const auto row_cells_begin = cells_.begin() + row->first_cell;
        const auto row_cells_end = row_cells_begin + row->cell_count;
        const auto first_cell =
            std::lower_bound(row_cells_begin, row_cells_end, minimum_z,
                             [](const CellRange& cell, std::int64_t z) { return cell.z < z; });
        for (auto cell = first_cell; cell != row_cells_end && cell->z <= maximum_z; ++cell) {
            const std::size_t entry_end = cell->first_entry + cell->entry_count;
            for (std::size_t entry_index = cell->first_entry; entry_index < entry_end;
                 ++entry_index) {
                const Entry& candidate_entry = entries_[entry_index];
                if (candidate_entry.source_index == subject_index) {
                    continue;
                }
                ++result.inspected_candidate_count;
                const double distance =
                    std::hypot(candidate_entry.x - subject.x, candidate_entry.z - subject.z);
                if (distance > radius) {
                    continue;
                }
                ++result.within_radius_count;
                const SpatialNeighbor candidate{.member_index = candidate_entry.source_index,
                                                .id = candidate_entry.id,
                                                .distance = distance};
                std::size_t insertion_index = 0;
                while (insertion_index < result.neighbor_count &&
                       !neighbor_precedes(candidate, output[insertion_index])) {
                    ++insertion_index;
                }
                if (insertion_index >= output.size()) {
                    continue;
                }
                const std::size_t prior_count = result.neighbor_count;
                result.neighbor_count = std::min(prior_count + 1, output.size());
                for (std::size_t shift = result.neighbor_count; shift > insertion_index + 1;
                     --shift) {
                    output[shift - 1] = output[shift - 2];
                }
                output[insertion_index] = candidate;
            }
        }
    }
    return result;
}

bool SheepSpatialGrid::built() const noexcept {
    return built_;
}

std::size_t SheepSpatialGrid::member_count() const noexcept {
    return member_count_;
}

double SheepSpatialGrid::cell_size() const noexcept {
    return cell_size_;
}

} // namespace wide_eye::game
