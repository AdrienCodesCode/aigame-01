#include "voxel/coordinates.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

using wide_eye::voxel::ChunkCoord;
using wide_eye::voxel::ChunkEdgeLength;
using wide_eye::voxel::ChunkLocalCoord;
using wide_eye::voxel::GridCoordinate;
using wide_eye::voxel::LocalVoxelCoord;
using wide_eye::voxel::WorldVoxelCoord;

bool check(bool condition, std::string_view stage) {
    if (condition) {
        return true;
    }
    std::cerr << "voxel_coordinates_result=fail\n"
              << "failure_stage=" << stage << '\n';
    return false;
}

bool check_split(WorldVoxelCoord world, ChunkEdgeLength edge_length, ChunkCoord expected_chunk,
                 LocalVoxelCoord expected_local, std::string_view stage) {
    const auto actual = wide_eye::voxel::world_to_chunk_local(world, edge_length);
    return check(actual && actual->chunk == expected_chunk && actual->local == expected_local,
                 stage);
}

bool check_round_trip(GridCoordinate value, ChunkEdgeLength edge_length) {
    const WorldVoxelCoord world{.x = value, .y = value, .z = value};
    const auto split = wide_eye::voxel::world_to_chunk_local(world, edge_length);
    if (!check(split.has_value(), "round_trip_split")) {
        return false;
    }
    return check(wide_eye::voxel::chunk_local_to_world(*split, edge_length) == world,
                 "round_trip_join");
}

} // namespace

int main() {
    constexpr ChunkEdgeLength kEdge16{.cells = 16};
    if (!check(!wide_eye::voxel::is_valid_chunk_edge_length({.cells = -1}) &&
                   !wide_eye::voxel::is_valid_chunk_edge_length({.cells = 0}) &&
                   wide_eye::voxel::is_valid_chunk_edge_length(kEdge16),
               "edge_length_validation")) {
        return EXIT_FAILURE;
    }

    if (!check_split({.x = 0, .y = 15, .z = 16}, kEdge16, {.x = 0, .y = 0, .z = 1},
                     {.x = 0, .y = 15, .z = 0}, "positive_boundaries") ||
        !check_split({.x = -1, .y = -16, .z = -17}, kEdge16, {.x = -1, .y = -1, .z = -2},
                     {.x = 15, .y = 0, .z = 15}, "negative_boundaries")) {
        return EXIT_FAILURE;
    }

    if (!check(wide_eye::voxel::is_valid_local({.x = 0, .y = 15, .z = 7}, kEdge16) &&
                   !wide_eye::voxel::is_valid_local({.x = -1, .y = 0, .z = 0}, kEdge16) &&
                   !wide_eye::voxel::is_valid_local({.x = 16, .y = 0, .z = 0}, kEdge16),
               "local_validation") ||
        !check(!wide_eye::voxel::world_to_chunk_local({.x = 0, .y = 0, .z = 0}, {.cells = 0}),
               "split_invalid_edge") ||
        !check(!wide_eye::voxel::chunk_local_to_world(
                   {.chunk = {}, .local = {.x = -1, .y = 0, .z = 0}}, kEdge16),
               "join_negative_local") ||
        !check(!wide_eye::voxel::chunk_local_to_world(
                   {.chunk = {}, .local = {.x = 16, .y = 0, .z = 0}}, kEdge16),
               "join_local_at_edge")) {
        return EXIT_FAILURE;
    }

    constexpr auto kMinimum = std::numeric_limits<GridCoordinate>::min();
    constexpr auto kMaximum = std::numeric_limits<GridCoordinate>::max();
    constexpr std::array<GridCoordinate, 23> kWorldValues{
        kMinimum, kMinimum + 1, -65, -33, -32, -31, -17, -16, -15, -1,           0,        1,
        15,       16,           17,  31,  32,  33,  63,  64,  65,  kMaximum - 1, kMaximum,
    };
    constexpr std::array<ChunkEdgeLength, 4> kEdgeLengths{
        ChunkEdgeLength{.cells = 1},
        ChunkEdgeLength{.cells = 3},
        ChunkEdgeLength{.cells = 16},
        ChunkEdgeLength{.cells = 32},
    };
    for (const ChunkEdgeLength edge_length : kEdgeLengths) {
        for (const GridCoordinate world_value : kWorldValues) {
            if (!check_round_trip(world_value, edge_length)) {
                return EXIT_FAILURE;
            }
        }
    }

    constexpr ChunkEdgeLength kEdge3{.cells = 3};
    const auto minimum_split =
        wide_eye::voxel::world_to_chunk_local({.x = kMinimum, .y = 0, .z = 0}, kEdge3);
    const auto maximum_split =
        wide_eye::voxel::world_to_chunk_local({.x = kMaximum, .y = 0, .z = 0}, kEdge3);
    if (!check(minimum_split && minimum_split->local.x == 1, "minimum_split") ||
        !check(maximum_split && maximum_split->local.x == 1, "maximum_split") ||
        !check(!wide_eye::voxel::chunk_local_to_world(
                   {.chunk = minimum_split->chunk,
                    .local = {.x = 0, .y = minimum_split->local.y, .z = minimum_split->local.z}},
                   kEdge3),
               "join_underflow") ||
        !check(!wide_eye::voxel::chunk_local_to_world(
                   {.chunk = maximum_split->chunk,
                    .local = {.x = 2, .y = maximum_split->local.y, .z = maximum_split->local.z}},
                   kEdge3),
               "join_overflow")) {
        return EXIT_FAILURE;
    }

    std::cout << "voxel_coordinates_negative_boundaries=yes\n"
              << "voxel_coordinates_round_trip=yes\n"
              << "voxel_coordinates_overflow_rejected=yes\n"
              << "voxel_coordinates_result=pass\n";
    return EXIT_SUCCESS;
}
