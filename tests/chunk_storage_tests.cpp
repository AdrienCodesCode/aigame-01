#include "voxel/chunk.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

using wide_eye::voxel::Chunk;
using wide_eye::voxel::ChunkCoord;
using wide_eye::voxel::ChunkEdgeLength;
using wide_eye::voxel::DirtyRegion;
using wide_eye::voxel::kEmptyMaterialId;
using wide_eye::voxel::LocalVoxelCoord;
using wide_eye::voxel::MaterialId;
using wide_eye::voxel::SetBlockResult;
using wide_eye::voxel::WorldVoxelCoord;

constexpr MaterialId kGrass{.value = 1};
constexpr MaterialId kStone{.value = 2};

bool check(bool condition, std::string_view stage) {
    if (condition) {
        return true;
    }
    std::cerr << "chunk_storage_result=fail\n"
              << "failure_stage=" << stage << '\n';
    return false;
}

bool check_uniform(const Chunk& chunk, MaterialId expected, std::string_view stage) {
    for (wide_eye::voxel::GridCoordinate z = 0; z < Chunk::kEdgeLength; ++z) {
        for (wide_eye::voxel::GridCoordinate y = 0; y < Chunk::kEdgeLength; ++y) {
            for (wide_eye::voxel::GridCoordinate x = 0; x < Chunk::kEdgeLength; ++x) {
                if (chunk.get({.x = x, .y = y, .z = z}) != expected) {
                    return check(false, stage);
                }
            }
        }
    }
    return true;
}

bool test_empty_and_full_chunks() {
    const Chunk empty;
    const Chunk full{kGrass};
    return check(wide_eye::voxel::is_empty(kEmptyMaterialId) &&
                     !wide_eye::voxel::is_empty(kGrass) && kEmptyMaterialId.value == 0,
                 "explicit_empty_material") &&
           check(Chunk::kEdgeLength == 16 && Chunk::kCellCount == 4096,
                 "fixed_storage_dimensions") &&
           check_uniform(empty, kEmptyMaterialId, "empty_chunk") &&
           check_uniform(full, kGrass, "full_chunk") &&
           check(!empty.dirty_region() && !full.dirty_region(), "initial_chunks_are_clean");
}

bool test_safe_boundaries() {
    Chunk chunk;
    constexpr std::array<LocalVoxelCoord, 8> kCorners{
        LocalVoxelCoord{.x = 0, .y = 0, .z = 0},   LocalVoxelCoord{.x = 15, .y = 0, .z = 0},
        LocalVoxelCoord{.x = 0, .y = 15, .z = 0},  LocalVoxelCoord{.x = 15, .y = 15, .z = 0},
        LocalVoxelCoord{.x = 0, .y = 0, .z = 15},  LocalVoxelCoord{.x = 15, .y = 0, .z = 15},
        LocalVoxelCoord{.x = 0, .y = 15, .z = 15}, LocalVoxelCoord{.x = 15, .y = 15, .z = 15},
    };
    for (const LocalVoxelCoord corner : kCorners) {
        if (!check(chunk.get(corner) == kEmptyMaterialId, "boundary_corner_get") ||
            !check(chunk.set(corner, kStone) == SetBlockResult::changed, "boundary_corner_set") ||
            !check(chunk.get(corner) == kStone, "boundary_corner_round_trip")) {
            return false;
        }
    }

    constexpr std::array<LocalVoxelCoord, 6> kOutside{
        LocalVoxelCoord{.x = -1, .y = 0, .z = 0}, LocalVoxelCoord{.x = 16, .y = 0, .z = 0},
        LocalVoxelCoord{.x = 0, .y = -1, .z = 0}, LocalVoxelCoord{.x = 0, .y = 16, .z = 0},
        LocalVoxelCoord{.x = 0, .y = 0, .z = -1}, LocalVoxelCoord{.x = 0, .y = 0, .z = 16},
    };
    for (const LocalVoxelCoord outside : kOutside) {
        if (!check(!chunk.get(outside), "out_of_bounds_get") ||
            !check(chunk.set(outside, kGrass) == SetBlockResult::out_of_bounds,
                   "out_of_bounds_set")) {
            return false;
        }
    }

    return check(chunk.dirty_region() == DirtyRegion{.minimum = {.x = 0, .y = 0, .z = 0},
                                                     .maximum = {.x = 15, .y = 15, .z = 15}},
                 "boundary_dirty_region");
}

bool test_adjacent_chunks() {
    constexpr ChunkEdgeLength kEdge{.cells = Chunk::kEdgeLength};
    constexpr std::array<WorldVoxelCoord, 4> kWorldCells{
        WorldVoxelCoord{.x = -1, .y = 4, .z = 5},
        WorldVoxelCoord{.x = 0, .y = 4, .z = 5},
        WorldVoxelCoord{.x = 15, .y = 4, .z = 5},
        WorldVoxelCoord{.x = 16, .y = 4, .z = 5},
    };
    constexpr std::array<ChunkCoord, 4> kExpectedChunks{
        ChunkCoord{.x = -1, .y = 0, .z = 0},
        ChunkCoord{.x = 0, .y = 0, .z = 0},
        ChunkCoord{.x = 0, .y = 0, .z = 0},
        ChunkCoord{.x = 1, .y = 0, .z = 0},
    };
    constexpr std::array<LocalVoxelCoord, 4> kExpectedLocals{
        LocalVoxelCoord{.x = 15, .y = 4, .z = 5},
        LocalVoxelCoord{.x = 0, .y = 4, .z = 5},
        LocalVoxelCoord{.x = 15, .y = 4, .z = 5},
        LocalVoxelCoord{.x = 0, .y = 4, .z = 5},
    };

    for (std::size_t index = 0; index < kWorldCells.size(); ++index) {
        const auto split = wide_eye::voxel::world_to_chunk_local(kWorldCells[index], kEdge);
        if (!check(split && split->chunk == kExpectedChunks[index] &&
                       split->local == kExpectedLocals[index],
                   "adjacent_coordinate_split")) {
            return false;
        }
    }

    Chunk left;
    Chunk right;
    if (!check(left.set(kExpectedLocals[2], kGrass) == SetBlockResult::changed,
               "adjacent_left_set") ||
        !check(right.set(kExpectedLocals[3], kStone) == SetBlockResult::changed,
               "adjacent_right_set")) {
        return false;
    }
    return check(left.get(kExpectedLocals[2]) == kGrass && right.get(kExpectedLocals[3]) == kStone,
                 "adjacent_values_independent") &&
           check(left.get(kExpectedLocals[3]) == kEmptyMaterialId &&
                     right.get(kExpectedLocals[2]) == kEmptyMaterialId,
                 "adjacent_storage_isolated") &&
           check(left.dirty_region() == DirtyRegion{.minimum = kExpectedLocals[2],
                                                    .maximum = kExpectedLocals[2]} &&
                     right.dirty_region() ==
                         DirtyRegion{.minimum = kExpectedLocals[3], .maximum = kExpectedLocals[3]},
                 "adjacent_dirty_regions_independent");
}

bool test_edits_and_dirty_region() {
    Chunk chunk;
    constexpr LocalVoxelCoord kFirst{.x = 8, .y = 7, .z = 6};
    constexpr LocalVoxelCoord kLow{.x = 2, .y = 10, .z = 3};
    constexpr LocalVoxelCoord kHigh{.x = 12, .y = 1, .z = 14};

    if (!check(chunk.set(kFirst, kGrass) == SetBlockResult::changed, "first_edit_changed") ||
        !check(chunk.dirty_region() == DirtyRegion{.minimum = kFirst, .maximum = kFirst},
               "first_edit_dirty_region") ||
        !check(chunk.set(kFirst, kGrass) == SetBlockResult::unchanged, "same_material_unchanged") ||
        !check(chunk.set(kLow, kStone) == SetBlockResult::changed, "low_edit_changed") ||
        !check(chunk.set(kHigh, kGrass) == SetBlockResult::changed, "high_edit_changed") ||
        !check(chunk.dirty_region() == DirtyRegion{.minimum = {.x = 2, .y = 1, .z = 3},
                                                   .maximum = {.x = 12, .y = 10, .z = 14}},
               "expanded_dirty_region")) {
        return false;
    }

    chunk.clear_dirty_region();
    return check(!chunk.dirty_region(), "dirty_region_cleared") &&
           check(chunk.get(kFirst) == kGrass && chunk.get(kLow) == kStone &&
                     chunk.get(kHigh) == kGrass,
                 "clear_preserves_cells") &&
           check(chunk.set(kFirst, kEmptyMaterialId) == SetBlockResult::changed,
                 "edit_back_to_empty") &&
           check(chunk.get(kFirst) == kEmptyMaterialId, "empty_edit_persisted") &&
           check(chunk.dirty_region() == DirtyRegion{.minimum = kFirst, .maximum = kFirst},
                 "dirty_after_clear");
}

} // namespace

int main() {
    if (!test_empty_and_full_chunks() || !test_safe_boundaries() || !test_adjacent_chunks() ||
        !test_edits_and_dirty_region()) {
        return EXIT_FAILURE;
    }

    std::cout << "chunk_storage_empty_full=yes\n"
              << "chunk_storage_boundaries=yes\n"
              << "chunk_storage_adjacent=yes\n"
              << "chunk_storage_edits=yes\n"
              << "chunk_storage_result=pass\n";
    return EXIT_SUCCESS;
}
