#include "voxel/coordinates.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using wide_eye::voxel::ChunkEdgeLength;

constexpr int kWorldEdge = 32;
constexpr std::size_t kWorldCellCount = 32U * 32U * 32U;
constexpr std::size_t kTimingSamples = 21;
constexpr std::uint64_t kTimingCellsPerSample = 1U << 20U;

struct CellCoord {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct ChunkFixture {
    CellCoord origin;
    int edge = 0;
    std::vector<std::uint8_t> occupancy;
};

struct RebuildResult {
    std::uint64_t scanned_cells = 0;
    std::uint64_t occupied_cells = 0;
    std::uint64_t visible_faces = 0;
};

struct ChunkSelection {
    std::array<std::size_t, 8> indices{};
    std::size_t count = 0;

    [[nodiscard]] std::span<const std::size_t> view() const noexcept {
        return {indices.data(), count};
    }
};

struct FixtureMemory {
    std::size_t logical_cell_bytes = 0;
    std::size_t reserved_cell_bytes = 0;
    std::size_t control_bytes = 0;

    [[nodiscard]] std::size_t modeled_bytes() const noexcept {
        return reserved_cell_bytes + control_bytes;
    }
};

struct TimingSummary {
    std::uint64_t minimum_ns = 0;
    std::uint64_t median_ns = 0;
    std::uint64_t maximum_ns = 0;
    std::uint64_t repetitions_per_sample = 0;
    std::uint64_t signature = 0;
};

[[nodiscard]] bool is_inside(CellCoord cell) noexcept {
    return cell.x >= 0 && cell.x < kWorldEdge && cell.y >= 0 && cell.y < kWorldEdge &&
           cell.z >= 0 && cell.z < kWorldEdge;
}

[[nodiscard]] std::size_t linear_index(CellCoord cell, int edge) noexcept {
    const auto size_edge = static_cast<std::size_t>(edge);
    const auto x = static_cast<std::size_t>(cell.x);
    const auto y = static_cast<std::size_t>(cell.y);
    const auto z = static_cast<std::size_t>(cell.z);
    return (z * size_edge + y) * size_edge + x;
}

[[nodiscard]] std::uint8_t occupancy_at(CellCoord cell) noexcept {
    const auto x = static_cast<std::uint32_t>(cell.x);
    const auto y = static_cast<std::uint32_t>(cell.y);
    const auto z = static_cast<std::uint32_t>(cell.z);
    const std::uint32_t mixed = (x * 73'856'093U) ^ (y * 19'349'663U) ^ (z * 83'492'791U);
    return static_cast<std::uint8_t>((mixed % 5U) != 0U);
}

class WorldFixture {
  public:
    explicit WorldFixture(int chunk_edge) : chunk_edge_(chunk_edge) {
        const int chunks_per_axis = kWorldEdge / chunk_edge_;
        const auto chunk_count =
            static_cast<std::size_t>(chunks_per_axis * chunks_per_axis * chunks_per_axis);
        chunks_.reserve(chunk_count);
        for (int chunk_z = 0; chunk_z < chunks_per_axis; ++chunk_z) {
            for (int chunk_y = 0; chunk_y < chunks_per_axis; ++chunk_y) {
                for (int chunk_x = 0; chunk_x < chunks_per_axis; ++chunk_x) {
                    chunks_.push_back(make_chunk({.x = chunk_x * chunk_edge_,
                                                  .y = chunk_y * chunk_edge_,
                                                  .z = chunk_z * chunk_edge_}));
                }
            }
        }
    }

    [[nodiscard]] int chunk_edge() const noexcept {
        return chunk_edge_;
    }
    [[nodiscard]] std::size_t chunk_count() const noexcept {
        return chunks_.size();
    }
    [[nodiscard]] const ChunkFixture& chunk(std::size_t index) const noexcept {
        return chunks_[index];
    }

    [[nodiscard]] std::size_t chunk_index(CellCoord cell) const noexcept {
        const int chunks_per_axis = kWorldEdge / chunk_edge_;
        const int chunk_x = cell.x / chunk_edge_;
        const int chunk_y = cell.y / chunk_edge_;
        const int chunk_z = cell.z / chunk_edge_;
        return static_cast<std::size_t>((chunk_z * chunks_per_axis + chunk_y) * chunks_per_axis +
                                        chunk_x);
    }

    [[nodiscard]] bool occupied(CellCoord cell) const noexcept {
        if (!is_inside(cell)) {
            return false;
        }
        const ChunkFixture& owning_chunk = chunks_[chunk_index(cell)];
        const CellCoord local{
            .x = cell.x - owning_chunk.origin.x,
            .y = cell.y - owning_chunk.origin.y,
            .z = cell.z - owning_chunk.origin.z,
        };
        return owning_chunk.occupancy[linear_index(local, owning_chunk.edge)] != 0U;
    }

    [[nodiscard]] FixtureMemory memory() const noexcept {
        FixtureMemory result{
            .control_bytes = sizeof(WorldFixture) + chunks_.capacity() * sizeof(ChunkFixture),
        };
        for (const ChunkFixture& chunk_value : chunks_) {
            result.logical_cell_bytes += chunk_value.occupancy.size() * sizeof(std::uint8_t);
            result.reserved_cell_bytes += chunk_value.occupancy.capacity() * sizeof(std::uint8_t);
        }
        return result;
    }

  private:
    [[nodiscard]] ChunkFixture make_chunk(CellCoord origin) const {
        const auto chunk_cell_count =
            static_cast<std::size_t>(chunk_edge_ * chunk_edge_ * chunk_edge_);
        ChunkFixture result{
            .origin = origin,
            .edge = chunk_edge_,
            .occupancy = std::vector<std::uint8_t>(chunk_cell_count),
        };
        for (int z = 0; z < chunk_edge_; ++z) {
            for (int y = 0; y < chunk_edge_; ++y) {
                for (int x = 0; x < chunk_edge_; ++x) {
                    const CellCoord local{.x = x, .y = y, .z = z};
                    const CellCoord world{
                        .x = origin.x + x,
                        .y = origin.y + y,
                        .z = origin.z + z,
                    };
                    result.occupancy[linear_index(local, chunk_edge_)] = occupancy_at(world);
                }
            }
        }
        return result;
    }

    int chunk_edge_ = 0;
    std::vector<ChunkFixture> chunks_;
};

[[nodiscard]] RebuildResult rebuild_chunk(const WorldFixture& world, std::size_t chunk_index) {
    constexpr std::array<CellCoord, 6> kNeighbors{
        CellCoord{.x = -1}, CellCoord{.x = 1},  CellCoord{.y = -1},
        CellCoord{.y = 1},  CellCoord{.z = -1}, CellCoord{.z = 1},
    };

    const ChunkFixture& chunk = world.chunk(chunk_index);
    RebuildResult result;
    for (int z = 0; z < chunk.edge; ++z) {
        for (int y = 0; y < chunk.edge; ++y) {
            for (int x = 0; x < chunk.edge; ++x) {
                ++result.scanned_cells;
                const CellCoord cell{
                    .x = chunk.origin.x + x,
                    .y = chunk.origin.y + y,
                    .z = chunk.origin.z + z,
                };
                if (!world.occupied(cell)) {
                    continue;
                }

                ++result.occupied_cells;
                for (const CellCoord neighbor : kNeighbors) {
                    if (!world.occupied({.x = cell.x + neighbor.x,
                                         .y = cell.y + neighbor.y,
                                         .z = cell.z + neighbor.z})) {
                        ++result.visible_faces;
                    }
                }
            }
        }
    }
    return result;
}

[[nodiscard]] RebuildResult rebuild_chunks(const WorldFixture& world,
                                           std::span<const std::size_t> indices) {
    RebuildResult total;
    for (const std::size_t index : indices) {
        const RebuildResult chunk_result = rebuild_chunk(world, index);
        total.scanned_cells += chunk_result.scanned_cells;
        total.occupied_cells += chunk_result.occupied_cells;
        total.visible_faces += chunk_result.visible_faces;
    }
    return total;
}

[[nodiscard]] ChunkSelection all_chunks(const WorldFixture& world) noexcept {
    ChunkSelection result;
    result.count = world.chunk_count();
    for (std::size_t index = 0; index < result.count; ++index) {
        result.indices[index] = index;
    }
    return result;
}

[[nodiscard]] ChunkSelection chunks_affected_by_edit(const WorldFixture& world,
                                                     CellCoord edited_cell) noexcept {
    constexpr std::array<CellCoord, 7> kAffectedCells{
        CellCoord{},       CellCoord{.x = -1}, CellCoord{.x = 1}, CellCoord{.y = -1},
        CellCoord{.y = 1}, CellCoord{.z = -1}, CellCoord{.z = 1},
    };

    ChunkSelection result;
    for (const CellCoord offset : kAffectedCells) {
        const CellCoord cell{.x = edited_cell.x + offset.x,
                             .y = edited_cell.y + offset.y,
                             .z = edited_cell.z + offset.z};
        if (!is_inside(cell)) {
            continue;
        }
        const std::size_t candidate = world.chunk_index(cell);
        const auto end = result.indices.begin() + static_cast<std::ptrdiff_t>(result.count);
        if (std::find(result.indices.begin(), end, candidate) == end) {
            result.indices[result.count] = candidate;
            ++result.count;
        }
    }
    return result;
}

[[nodiscard]] std::uint64_t combine_signature(std::uint64_t signature,
                                              RebuildResult result) noexcept {
    signature ^=
        result.scanned_cells + 0x9e3779b97f4a7c15ULL + (signature << 6U) + (signature >> 2U);
    signature ^=
        result.occupied_cells + 0x9e3779b97f4a7c15ULL + (signature << 6U) + (signature >> 2U);
    signature ^=
        result.visible_faces + 0x9e3779b97f4a7c15ULL + (signature << 6U) + (signature >> 2U);
    return signature;
}

[[nodiscard]] TimingSummary measure(const WorldFixture& world, ChunkSelection selection) {
    const RebuildResult reference = rebuild_chunks(world, selection.view());
    const std::uint64_t repetitions =
        std::max<std::uint64_t>(1U, kTimingCellsPerSample / reference.scanned_cells);
    std::uint64_t signature = 0;
    for (int warmup = 0; warmup < 3; ++warmup) {
        signature = combine_signature(signature, rebuild_chunks(world, selection.view()));
    }

    std::vector<std::uint64_t> durations;
    durations.reserve(kTimingSamples);
    for (std::size_t sample = 0; sample < kTimingSamples; ++sample) {
        const auto start = std::chrono::steady_clock::now();
        for (std::uint64_t repetition = 0; repetition < repetitions; ++repetition) {
            signature = combine_signature(signature, rebuild_chunks(world, selection.view()));
        }
        const auto end = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        durations.push_back(static_cast<std::uint64_t>(elapsed.count()) / repetitions);
    }
    std::ranges::sort(durations);
    return {
        .minimum_ns = durations.front(),
        .median_ns = durations[durations.size() / 2U],
        .maximum_ns = durations.back(),
        .repetitions_per_sample = repetitions,
        .signature = signature,
    };
}

bool check(bool condition, std::string_view stage) {
    if (condition) {
        return true;
    }
    std::cerr << "chunk_size_comparison_result=fail\n"
              << "failure_stage=" << stage << '\n';
    return false;
}

void print_candidate(const WorldFixture& world, ChunkSelection full, ChunkSelection interior,
                     ChunkSelection boundary) {
    const FixtureMemory memory = world.memory();
    const RebuildResult full_result = rebuild_chunks(world, full.view());
    const RebuildResult interior_result = rebuild_chunks(world, interior.view());
    const RebuildResult boundary_result = rebuild_chunks(world, boundary.view());
    std::cout << "chunk_size_candidate edge=" << world.chunk_edge()
              << " chunks=" << world.chunk_count()
              << " cells_per_chunk=" << full_result.scanned_cells / world.chunk_count()
              << " logical_cell_bytes=" << memory.logical_cell_bytes
              << " reserved_cell_bytes=" << memory.reserved_cell_bytes
              << " control_bytes=" << memory.control_bytes
              << " modeled_bytes=" << memory.modeled_bytes() << '\n'
              << "chunk_size_workload edge=" << world.chunk_edge()
              << " name=full chunks=" << full.count
              << " scanned_cells=" << full_result.scanned_cells
              << " occupied_cells=" << full_result.occupied_cells
              << " visible_faces=" << full_result.visible_faces << '\n'
              << "chunk_size_workload edge=" << world.chunk_edge()
              << " name=interior_edit chunks=" << interior.count
              << " scanned_cells=" << interior_result.scanned_cells << '\n'
              << "chunk_size_workload edge=" << world.chunk_edge()
              << " name=boundary_edit chunks=" << boundary.count
              << " scanned_cells=" << boundary_result.scanned_cells << '\n';
}

void print_timing(int edge, std::string_view workload, TimingSummary timing) {
    std::cout << "chunk_size_timing edge=" << edge << " name=" << workload
              << " samples=" << kTimingSamples
              << " repetitions_per_sample=" << timing.repetitions_per_sample
              << " minimum_ns=" << timing.minimum_ns << " median_ns=" << timing.median_ns
              << " maximum_ns=" << timing.maximum_ns << " signature=" << timing.signature << '\n';
}

} // namespace

int main(int argc, char** argv) {
    const bool run_benchmark = argc == 2 && std::string_view(argv[1]) == "--benchmark";
    const bool validate_only = argc == 2 && std::string_view(argv[1]) == "--validate-only";
    if (!run_benchmark && !validate_only) {
        std::cerr << "usage: wide_eye_chunk_size_comparison --validate-only|--benchmark\n"
                  << "chunk_size_comparison_result=fail\n"
                  << "failure_stage=arguments\n";
        return EXIT_FAILURE;
    }

    constexpr ChunkEdgeLength kEdge16{.cells = 16};
    constexpr ChunkEdgeLength kEdge32{.cells = 32};
    if (!check(wide_eye::voxel::is_valid_chunk_edge_length(kEdge16) &&
                   wide_eye::voxel::is_valid_chunk_edge_length(kEdge32),
               "coordinate_edges")) {
        return EXIT_FAILURE;
    }
    const auto boundary_split16 =
        wide_eye::voxel::world_to_chunk_local({.x = 16, .y = 8, .z = 8}, kEdge16);
    const auto boundary_split32 =
        wide_eye::voxel::world_to_chunk_local({.x = 16, .y = 8, .z = 8}, kEdge32);
    if (!check(boundary_split16 && boundary_split16->chunk.x == 1 &&
                   boundary_split16->local.x == 0 && boundary_split32 &&
                   boundary_split32->chunk.x == 0 && boundary_split32->local.x == 16,
               "coordinate_boundary_mapping")) {
        return EXIT_FAILURE;
    }

    const WorldFixture world16{static_cast<int>(kEdge16.cells)};
    const WorldFixture world32{static_cast<int>(kEdge32.cells)};
    const ChunkSelection full16 = all_chunks(world16);
    const ChunkSelection full32 = all_chunks(world32);
    const ChunkSelection interior16 = chunks_affected_by_edit(world16, {.x = 8, .y = 8, .z = 8});
    const ChunkSelection interior32 = chunks_affected_by_edit(world32, {.x = 8, .y = 8, .z = 8});
    const ChunkSelection boundary16 = chunks_affected_by_edit(world16, {.x = 16, .y = 8, .z = 8});
    const ChunkSelection boundary32 = chunks_affected_by_edit(world32, {.x = 16, .y = 8, .z = 8});

    const RebuildResult result16 = rebuild_chunks(world16, full16.view());
    const RebuildResult result32 = rebuild_chunks(world32, full32.view());
    if (!check(world16.chunk_count() == 8U && world32.chunk_count() == 1U, "chunk_counts") ||
        !check(world16.memory().logical_cell_bytes == kWorldCellCount &&
                   world32.memory().logical_cell_bytes == kWorldCellCount,
               "logical_memory") ||
        !check(world16.memory().reserved_cell_bytes >= world16.memory().logical_cell_bytes &&
                   world32.memory().reserved_cell_bytes >= world32.memory().logical_cell_bytes,
               "reserved_memory") ||
        !check(result16.scanned_cells == kWorldCellCount &&
                   result32.scanned_cells == kWorldCellCount,
               "full_scan_counts") ||
        !check(result16.occupied_cells == 26'211U && result16.visible_faces == 35'462U,
               "fixture_reference_counts") ||
        !check(result16.occupied_cells == result32.occupied_cells &&
                   result16.visible_faces == result32.visible_faces,
               "full_rebuild_equivalence") ||
        !check(interior16.count == 1U && interior32.count == 1U, "interior_dirty_chunks") ||
        !check(boundary16.count == 2U && boundary32.count == 1U, "boundary_dirty_chunks") ||
        !check(rebuild_chunks(world16, interior16.view()).scanned_cells == 4'096U &&
                   rebuild_chunks(world32, interior32.view()).scanned_cells == 32'768U,
               "interior_scan_counts") ||
        !check(rebuild_chunks(world16, boundary16.view()).scanned_cells == 8'192U &&
                   rebuild_chunks(world32, boundary32.view()).scanned_cells == 32'768U,
               "boundary_scan_counts")) {
        return EXIT_FAILURE;
    }

    std::cout << "chunk_size_fixture_schema=1\n"
              << "chunk_size_fixture_world_edge=32\n"
              << "chunk_size_fixture_storage_model=one_byte_occupancy_plus_fixture_control\n"
              << "chunk_size_fixture_reserved_capacity=included\n"
              << "chunk_size_fixture_allocator_metadata=excluded\n";
    print_candidate(world16, full16, interior16, boundary16);
    print_candidate(world32, full32, interior32, boundary32);

    if (run_benchmark) {
        const TimingSummary full_timing16 = measure(world16, full16);
        const TimingSummary full_timing32 = measure(world32, full32);
        const TimingSummary interior_timing16 = measure(world16, interior16);
        const TimingSummary interior_timing32 = measure(world32, interior32);
        const TimingSummary boundary_timing16 = measure(world16, boundary16);
        const TimingSummary boundary_timing32 = measure(world32, boundary32);
        print_timing(16, "full", full_timing16);
        print_timing(32, "full", full_timing32);
        print_timing(16, "interior_edit", interior_timing16);
        print_timing(32, "interior_edit", interior_timing32);
        print_timing(16, "boundary_edit", boundary_timing16);
        print_timing(32, "boundary_edit", boundary_timing32);

        const double memory_ratio = static_cast<double>(world16.memory().modeled_bytes()) /
                                    static_cast<double>(world32.memory().modeled_bytes());
        const double full_time_ratio = static_cast<double>(full_timing16.median_ns) /
                                       static_cast<double>(full_timing32.median_ns);
        std::cout << std::fixed << std::setprecision(4)
                  << "chunk_size_summary modeled_memory_ratio_16_over_32=" << memory_ratio
                  << " full_median_ratio_16_over_32=" << full_time_ratio
                  << " interior_scanned_ratio_32_over_16=8.0000"
                  << " boundary_scanned_ratio_32_over_16=4.0000\n";
    }

    std::cout << "chunk_size_comparison_result=pass\n";
    return EXIT_SUCCESS;
}
