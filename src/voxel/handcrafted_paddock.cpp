#include "voxel/handcrafted_paddock.hpp"

#include <cstdint>
#include <limits>
#include <utility>

namespace wide_eye::voxel {
namespace {

constexpr ChunkEdgeLength kProductionChunkEdge{Chunk::kEdgeLength};

[[nodiscard]] std::optional<std::size_t> chunk_index(ChunkCoord coordinate) noexcept {
    if (coordinate.y != 0 || coordinate.x < 0 || coordinate.x >= HandcraftedPaddock::kChunkCountX ||
        coordinate.z < 0 || coordinate.z >= HandcraftedPaddock::kChunkCountZ) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(coordinate.z * HandcraftedPaddock::kChunkCountX + coordinate.x);
}

[[nodiscard]] ChunkCoord neighbor_coordinate(ChunkCoord coordinate,
                                             FaceDirection direction) noexcept {
    switch (direction) {
    case FaceDirection::negative_x:
        --coordinate.x;
        break;
    case FaceDirection::positive_x:
        ++coordinate.x;
        break;
    case FaceDirection::negative_y:
        --coordinate.y;
        break;
    case FaceDirection::positive_y:
        ++coordinate.y;
        break;
    case FaceDirection::negative_z:
        --coordinate.z;
        break;
    case FaceDirection::positive_z:
        ++coordinate.z;
        break;
    }
    return coordinate;
}

[[nodiscard]] bool append_mesh(ChunkMesh& destination, const ChunkMesh& source,
                               ChunkCoord source_coordinate) {
    if (source.vertices.empty() && source.indices.empty()) {
        return true;
    }
    if (destination.vertices.size() >
        std::numeric_limits<std::uint32_t>::max() - source.vertices.size()) {
        return false;
    }

    const auto first_vertex = static_cast<std::uint32_t>(destination.vertices.size());
    const std::array<float, 3> offset{
        static_cast<float>(source_coordinate.x * Chunk::kEdgeLength),
        static_cast<float>(source_coordinate.y * Chunk::kEdgeLength),
        static_cast<float>(source_coordinate.z * Chunk::kEdgeLength),
    };

    destination.vertices.reserve(destination.vertices.size() + source.vertices.size());
    for (ChunkMeshVertex vertex : source.vertices) {
        vertex.position[0] += offset[0];
        vertex.position[1] += offset[1];
        vertex.position[2] += offset[2];
        destination.vertices.push_back(vertex);
    }

    destination.indices.reserve(destination.indices.size() + source.indices.size());
    for (const std::uint32_t index : source.indices) {
        if (index > std::numeric_limits<std::uint32_t>::max() - first_vertex) {
            return false;
        }
        destination.indices.push_back(first_vertex + index);
    }
    return true;
}

[[nodiscard]] bool append_meshes(ChunkMeshes& destination, const ChunkMeshes& source,
                                 ChunkCoord source_coordinate) {
    return append_mesh(destination.opaque, source.opaque, source_coordinate) &&
           append_mesh(destination.cutout, source.cutout, source_coordinate) &&
           append_mesh(destination.translucent, source.translucent, source_coordinate);
}

} // namespace

std::optional<PaddockPaletteColor> PaddockPalette::get(MaterialId material) const noexcept {
    const auto index = static_cast<std::size_t>(material.value);
    return index < entries_.size() ? std::optional{entries_[index]} : std::nullopt;
}

const std::array<PaddockPaletteColor, PaddockPalette::kEntryCount>&
PaddockPalette::entries() const noexcept {
    return entries_;
}

PaddockPalette make_handcrafted_paddock_palette() noexcept {
    PaddockPalette palette;
    palette.entries_ = {{
        {.rgb = {0.0F, 0.0F, 0.0F}},
        {.rgb = {0.31F, 0.50F, 0.20F}},
        {.rgb = {0.55F, 0.55F, 0.49F}},
        {.rgb = {0.78F, 0.12F, 0.065F}},
        {.rgb = {0.67F, 0.36F, 0.15F}},
        {.rgb = {0.30F, 0.095F, 0.055F}},
        {.rgb = {0.18F, 0.065F, 0.025F}},
    }};
    return palette;
}

Chunk* HandcraftedPaddock::chunk_at(ChunkCoord coordinate) noexcept {
    const auto index = chunk_index(coordinate);
    return index.has_value() ? &chunks_[*index] : nullptr;
}

const Chunk* HandcraftedPaddock::chunk_at(ChunkCoord coordinate) const noexcept {
    const auto index = chunk_index(coordinate);
    return index.has_value() ? &chunks_[*index] : nullptr;
}

std::optional<MaterialId> HandcraftedPaddock::get(WorldVoxelCoord world) const noexcept {
    const auto coordinate = world_to_chunk_local(world, kProductionChunkEdge);
    if (!coordinate.has_value()) {
        return std::nullopt;
    }
    const Chunk* chunk = chunk_at(coordinate->chunk);
    return chunk == nullptr ? std::nullopt : chunk->get(coordinate->local);
}

bool HandcraftedPaddock::set(WorldVoxelCoord world, MaterialId material) noexcept {
    const auto coordinate = world_to_chunk_local(world, kProductionChunkEdge);
    if (!coordinate.has_value()) {
        return false;
    }
    Chunk* chunk = chunk_at(coordinate->chunk);
    return chunk != nullptr &&
           chunk->set(coordinate->local, material) != SetBlockResult::out_of_bounds;
}

std::size_t HandcraftedPaddock::count_blocks(MaterialId material) const noexcept {
    std::size_t count = 0;
    for (GridCoordinate z = 0; z < kWorldDepth; ++z) {
        for (GridCoordinate y = 0; y < Chunk::kEdgeLength; ++y) {
            for (GridCoordinate x = 0; x < kWorldWidth; ++x) {
                if (get({.x = x, .y = y, .z = z}).value_or(kEmptyMaterialId) == material) {
                    ++count;
                }
            }
        }
    }
    return count;
}

std::size_t HandcraftedPaddock::occupied_block_count() const noexcept {
    return Chunk::kCellCount * kChunkCount - count_blocks(kEmptyMaterialId);
}

std::optional<HandcraftedPaddock> make_handcrafted_paddock() noexcept {
    HandcraftedPaddock paddock;

    const auto set_box = [&](WorldVoxelCoord minimum, WorldVoxelCoord maximum,
                             MaterialId material) {
        for (GridCoordinate z = minimum.z; z <= maximum.z; ++z) {
            for (GridCoordinate y = minimum.y; y <= maximum.y; ++y) {
                for (GridCoordinate x = minimum.x; x <= maximum.x; ++x) {
                    if (!paddock.set({.x = x, .y = y, .z = z}, material)) {
                        return false;
                    }
                }
            }
        }
        return true;
    };

    const bool ground =
        set_box({.x = 0, .y = 0, .z = 0}, {.x = 31, .y = 0, .z = 31}, kPaddockGrassMaterial);
    const bool left_wall =
        set_box({.x = 1, .y = 1, .z = 14}, {.x = 13, .y = 3, .z = 15}, kPaddockStoneMaterial);
    const bool right_wall =
        set_box({.x = 18, .y = 1, .z = 14}, {.x = 30, .y = 3, .z = 15}, kPaddockStoneMaterial);
    const bool left_post =
        set_box({.x = 13, .y = 4, .z = 14}, {.x = 13, .y = 4, .z = 15}, kPaddockStoneMaterial);
    const bool right_post =
        set_box({.x = 18, .y = 4, .z = 14}, {.x = 18, .y = 4, .z = 15}, kPaddockStoneMaterial);
    const bool gate =
        set_box({.x = 14, .y = 1, .z = 15}, {.x = 17, .y = 3, .z = 15}, kPaddockGateMaterial);

    const bool barn_body =
        set_box({.x = 3, .y = 1, .z = 2}, {.x = 10, .y = 5, .z = 8}, kPaddockBarnWallMaterial);
    const bool barn_door =
        set_box({.x = 5, .y = 1, .z = 8}, {.x = 8, .y = 4, .z = 8}, kPaddockBarnDoorMaterial);
    const bool roof_0 =
        set_box({.x = 2, .y = 6, .z = 1}, {.x = 11, .y = 6, .z = 9}, kPaddockBarnRoofMaterial);
    const bool roof_1 =
        set_box({.x = 3, .y = 7, .z = 1}, {.x = 10, .y = 7, .z = 9}, kPaddockBarnRoofMaterial);
    const bool roof_2 =
        set_box({.x = 4, .y = 8, .z = 1}, {.x = 9, .y = 8, .z = 9}, kPaddockBarnRoofMaterial);
    const bool roof_3 =
        set_box({.x = 5, .y = 9, .z = 1}, {.x = 8, .y = 9, .z = 9}, kPaddockBarnRoofMaterial);
    const bool roof_4 =
        set_box({.x = 6, .y = 10, .z = 1}, {.x = 7, .y = 10, .z = 9}, kPaddockBarnRoofMaterial);

    if (!ground || !left_wall || !right_wall || !left_post || !right_post || !gate || !barn_body ||
        !barn_door || !roof_0 || !roof_1 || !roof_2 || !roof_3 || !roof_4) {
        return std::nullopt;
    }
    return paddock;
}

std::optional<HandcraftedPaddockMesh>
build_handcrafted_paddock_mesh(const HandcraftedPaddock& paddock) {
    HandcraftedPaddockMesh result{
        .passes = {},
        .source_chunk_count = HandcraftedPaddock::kChunkCount,
        .occupied_block_count = paddock.occupied_block_count(),
        .opaque_faces_by_material = {},
        .face_diagnostics = {},
    };

    for (GridCoordinate z = 0; z < HandcraftedPaddock::kChunkCountZ; ++z) {
        for (GridCoordinate x = 0; x < HandcraftedPaddock::kChunkCountX; ++x) {
            const ChunkCoord coordinate{.x = x, .z = z};
            const Chunk* chunk = paddock.chunk_at(coordinate);
            if (chunk == nullptr) {
                return std::nullopt;
            }

            ChunkNeighborhood neighborhood;
            for (std::size_t direction_index = 0; direction_index < kFaceDirectionCount;
                 ++direction_index) {
                const auto direction = static_cast<FaceDirection>(direction_index);
                neighborhood.adjacent[face_direction_index(direction)] =
                    paddock.chunk_at(neighbor_coordinate(coordinate, direction));
            }

            const ChunkMeshBuildResult chunk_mesh = build_naive_chunk_mesh(*chunk, neighborhood);
            if (!chunk_mesh.has_value() || !append_meshes(result.passes, *chunk_mesh, coordinate)) {
                return std::nullopt;
            }
            const std::vector<ChunkFaceDiagnostic> chunk_diagnostics =
                describe_naive_chunk_faces(*chunk, neighborhood);
            result.face_diagnostics.reserve(result.face_diagnostics.size() +
                                            chunk_diagnostics.size());
            for (const ChunkFaceDiagnostic& face : chunk_diagnostics) {
                result.face_diagnostics.push_back({
                    .source_chunk = coordinate,
                    .face = face,
                });
            }
        }
    }

    for (std::size_t face = 0; face < result.passes.opaque.face_count(); ++face) {
        const std::size_t first_vertex = face * kNaiveMeshVerticesPerFace;
        const auto material =
            static_cast<std::size_t>(result.passes.opaque.vertices[first_vertex].material.value);
        if (material >= result.opaque_faces_by_material.size()) {
            return std::nullopt;
        }
        ++result.opaque_faces_by_material[material];
    }
    return result;
}

} // namespace wide_eye::voxel
