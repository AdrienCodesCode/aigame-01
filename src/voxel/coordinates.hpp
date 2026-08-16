#pragma once

#include <cstdint>
#include <optional>

namespace wide_eye::voxel {

using GridCoordinate = std::int64_t;

// The three coordinate spaces are distinct types so call sites cannot silently
// substitute a chunk index or local offset for a world-space voxel cell.
struct WorldVoxelCoord {
    GridCoordinate x = 0;
    GridCoordinate y = 0;
    GridCoordinate z = 0;

    bool operator==(const WorldVoxelCoord&) const = default;
};

struct ChunkCoord {
    GridCoordinate x = 0;
    GridCoordinate y = 0;
    GridCoordinate z = 0;

    bool operator==(const ChunkCoord&) const = default;
};

struct LocalVoxelCoord {
    GridCoordinate x = 0;
    GridCoordinate y = 0;
    GridCoordinate z = 0;

    bool operator==(const LocalVoxelCoord&) const = default;
};

// A positive cubic edge length supplied by the caller. No project-wide chunk
// size is selected by this coordinate module.
struct ChunkEdgeLength {
    GridCoordinate cells = 0;
};

struct ChunkLocalCoord {
    ChunkCoord chunk;
    LocalVoxelCoord local;

    bool operator==(const ChunkLocalCoord&) const = default;
};

[[nodiscard]] bool is_valid_chunk_edge_length(ChunkEdgeLength edge_length) noexcept;
[[nodiscard]] bool is_valid_local(LocalVoxelCoord local, ChunkEdgeLength edge_length) noexcept;

// Uses floor division on every axis. For a valid edge length, local coordinates
// are always in [0, edge_length.cells), including for negative world positions.
[[nodiscard]] std::optional<ChunkLocalCoord>
world_to_chunk_local(WorldVoxelCoord world, ChunkEdgeLength edge_length) noexcept;

// Rejects invalid local coordinates and any result outside the signed 64-bit
// world range. Every successful world_to_chunk_local result recomposes exactly.
[[nodiscard]] std::optional<WorldVoxelCoord>
chunk_local_to_world(ChunkLocalCoord coordinate, ChunkEdgeLength edge_length) noexcept;

} // namespace wide_eye::voxel
