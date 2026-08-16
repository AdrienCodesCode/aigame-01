#include "voxel/coordinates.hpp"

#include <limits>

namespace wide_eye::voxel {
namespace {

struct AxisChunkLocal {
    GridCoordinate chunk = 0;
    GridCoordinate local = 0;
};

AxisChunkLocal split_axis(GridCoordinate world, GridCoordinate edge_length) noexcept {
    GridCoordinate chunk = world / edge_length;
    GridCoordinate local = world % edge_length;
    if (local < 0) {
        --chunk;
        local += edge_length;
    }
    return {.chunk = chunk, .local = local};
}

bool axis_is_before(AxisChunkLocal lhs, AxisChunkLocal rhs) noexcept {
    return lhs.chunk < rhs.chunk || (lhs.chunk == rhs.chunk && lhs.local < rhs.local);
}

bool axis_is_after(AxisChunkLocal lhs, AxisChunkLocal rhs) noexcept {
    return lhs.chunk > rhs.chunk || (lhs.chunk == rhs.chunk && lhs.local > rhs.local);
}

std::optional<GridCoordinate> combine_axis(GridCoordinate chunk, GridCoordinate local,
                                           GridCoordinate edge_length) noexcept {
    if (local < 0 || local >= edge_length) {
        return std::nullopt;
    }

    const AxisChunkLocal coordinate{.chunk = chunk, .local = local};
    const AxisChunkLocal minimum =
        split_axis(std::numeric_limits<GridCoordinate>::min(), edge_length);
    const AxisChunkLocal maximum =
        split_axis(std::numeric_limits<GridCoordinate>::max(), edge_length);
    if (axis_is_before(coordinate, minimum) || axis_is_after(coordinate, maximum)) {
        return std::nullopt;
    }

    if (chunk >= 0) {
        return chunk * edge_length + local;
    }
    if (local == 0) {
        return chunk * edge_length;
    }

    // Reassociate the expression so the intermediate product stays within the
    // signed range even when the final value is INT64_MIN with a nonzero local.
    const GridCoordinate next_chunk_origin = (chunk + 1) * edge_length;
    return next_chunk_origin - (edge_length - local);
}

} // namespace

bool is_valid_chunk_edge_length(ChunkEdgeLength edge_length) noexcept {
    return edge_length.cells > 0;
}

bool is_valid_local(LocalVoxelCoord local, ChunkEdgeLength edge_length) noexcept {
    return is_valid_chunk_edge_length(edge_length) && local.x >= 0 && local.x < edge_length.cells &&
           local.y >= 0 && local.y < edge_length.cells && local.z >= 0 &&
           local.z < edge_length.cells;
}

std::optional<ChunkLocalCoord> world_to_chunk_local(WorldVoxelCoord world,
                                                    ChunkEdgeLength edge_length) noexcept {
    if (!is_valid_chunk_edge_length(edge_length)) {
        return std::nullopt;
    }

    const AxisChunkLocal x = split_axis(world.x, edge_length.cells);
    const AxisChunkLocal y = split_axis(world.y, edge_length.cells);
    const AxisChunkLocal z = split_axis(world.z, edge_length.cells);
    return ChunkLocalCoord{
        .chunk = {.x = x.chunk, .y = y.chunk, .z = z.chunk},
        .local = {.x = x.local, .y = y.local, .z = z.local},
    };
}

std::optional<WorldVoxelCoord> chunk_local_to_world(ChunkLocalCoord coordinate,
                                                    ChunkEdgeLength edge_length) noexcept {
    if (!is_valid_chunk_edge_length(edge_length) ||
        !is_valid_local(coordinate.local, edge_length)) {
        return std::nullopt;
    }

    const auto x = combine_axis(coordinate.chunk.x, coordinate.local.x, edge_length.cells);
    const auto y = combine_axis(coordinate.chunk.y, coordinate.local.y, edge_length.cells);
    const auto z = combine_axis(coordinate.chunk.z, coordinate.local.z, edge_length.cells);
    if (!x || !y || !z) {
        return std::nullopt;
    }
    return WorldVoxelCoord{.x = *x, .y = *y, .z = *z};
}

} // namespace wide_eye::voxel
