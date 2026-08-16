#include "voxel/naive_mesher.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace wide_eye::voxel {
namespace {

struct DirectionDefinition {
    FaceDirection direction;
    LocalVoxelCoord neighbor_offset;
    std::array<std::int8_t, 3> normal;
    std::array<std::array<GridCoordinate, 3>, 4> corners;
};

constexpr std::array<DirectionDefinition, kFaceDirectionCount> kDirections{{
    {
        .direction = FaceDirection::negative_x,
        .neighbor_offset = {.x = -1},
        .normal = {-1, 0, 0},
        .corners = {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}},
    },
    {
        .direction = FaceDirection::positive_x,
        .neighbor_offset = {.x = 1},
        .normal = {1, 0, 0},
        .corners = {{{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}},
    },
    {
        .direction = FaceDirection::negative_y,
        .neighbor_offset = {.y = -1},
        .normal = {0, -1, 0},
        .corners = {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}},
    },
    {
        .direction = FaceDirection::positive_y,
        .neighbor_offset = {.y = 1},
        .normal = {0, 1, 0},
        .corners = {{{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}}},
    },
    {
        .direction = FaceDirection::negative_z,
        .neighbor_offset = {.z = -1},
        .normal = {0, 0, -1},
        .corners = {{{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}},
    },
    {
        .direction = FaceDirection::positive_z,
        .neighbor_offset = {.z = 1},
        .normal = {0, 0, 1},
        .corners = {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},
    },
}};

constexpr std::array<std::uint32_t, 6> kQuadIndices{0, 1, 2, 0, 2, 3};

static_assert(kQuadIndices.size() == kNaiveMeshIndicesPerFace);
static_assert(kMaxNaiveChunkVertexCount <= std::numeric_limits<std::uint32_t>::max());

[[nodiscard]] LocalVoxelCoord offset(LocalVoxelCoord local, LocalVoxelCoord delta) noexcept {
    return {
        .x = local.x + delta.x,
        .y = local.y + delta.y,
        .z = local.z + delta.z,
    };
}

[[nodiscard]] LocalVoxelCoord wrap_neighbor_local(LocalVoxelCoord local,
                                                  FaceDirection direction) noexcept {
    switch (direction) {
    case FaceDirection::negative_x:
        local.x = Chunk::kEdgeLength - 1;
        break;
    case FaceDirection::positive_x:
        local.x = 0;
        break;
    case FaceDirection::negative_y:
        local.y = Chunk::kEdgeLength - 1;
        break;
    case FaceDirection::positive_y:
        local.y = 0;
        break;
    case FaceDirection::negative_z:
        local.z = Chunk::kEdgeLength - 1;
        break;
    case FaceDirection::positive_z:
        local.z = 0;
        break;
    }
    return local;
}

struct NeighborSample {
    LocalVoxelCoord local;
    MaterialId material;
    FaceNeighborKind kind;
};

[[nodiscard]] NeighborSample sample_neighbor(const Chunk& chunk,
                                             const ChunkNeighborhood& neighborhood,
                                             LocalVoxelCoord neighbor_local,
                                             FaceDirection direction) noexcept {
    if (const auto material = chunk.get(neighbor_local)) {
        return {
            .local = neighbor_local,
            .material = *material,
            .kind = FaceNeighborKind::same_chunk,
        };
    }

    const LocalVoxelCoord wrapped_local = wrap_neighbor_local(neighbor_local, direction);
    const Chunk* adjacent = neighborhood.get(direction);
    if (adjacent == nullptr) {
        return {
            .local = wrapped_local,
            .material = kEmptyMaterialId,
            .kind = FaceNeighborKind::missing_chunk,
        };
    }

    return {
        .local = wrapped_local,
        .material = adjacent->get(wrapped_local).value_or(kEmptyMaterialId),
        .kind = FaceNeighborKind::adjacent_chunk,
    };
}

void append_face(ChunkMesh& mesh, LocalVoxelCoord local, MaterialId material,
                 const DirectionDefinition& definition) {
    const auto first_vertex = static_cast<std::uint32_t>(mesh.vertices.size());
    for (const auto& corner : definition.corners) {
        mesh.vertices.push_back({
            .position =
                {
                    static_cast<float>(local.x + corner[0]),
                    static_cast<float>(local.y + corner[1]),
                    static_cast<float>(local.z + corner[2]),
                },
            .normal = definition.normal,
            .material = material,
        });
    }
    for (const std::uint32_t index : kQuadIndices) {
        mesh.indices.push_back(first_vertex + index);
    }
}

[[nodiscard]] std::size_t render_pass_index(MeshRenderPass pass) noexcept {
    switch (pass) {
    case MeshRenderPass::opaque:
        return 0;
    case MeshRenderPass::cutout:
        return 1;
    case MeshRenderPass::translucent:
        return 2;
    }
    std::unreachable();
}

[[nodiscard]] ChunkMesh& mesh_for_pass(ChunkMeshes& meshes, MeshRenderPass pass) noexcept {
    switch (pass) {
    case MeshRenderPass::opaque:
        return meshes.opaque;
    case MeshRenderPass::cutout:
        return meshes.cutout;
    case MeshRenderPass::translucent:
        return meshes.translucent;
    }
    std::unreachable();
}

template <typename VisitFace>
bool for_each_face(const Chunk& chunk, const ChunkNeighborhood& neighborhood, VisitFace&& visit) {
    for (GridCoordinate z = 0; z < Chunk::kEdgeLength; ++z) {
        for (GridCoordinate y = 0; y < Chunk::kEdgeLength; ++y) {
            for (GridCoordinate x = 0; x < Chunk::kEdgeLength; ++x) {
                const LocalVoxelCoord local{.x = x, .y = y, .z = z};
                const MaterialId material = chunk.get(local).value_or(kEmptyMaterialId);
                if (is_empty(material)) {
                    continue;
                }

                for (const DirectionDefinition& definition : kDirections) {
                    const LocalVoxelCoord neighbor_local =
                        offset(local, definition.neighbor_offset);
                    const NeighborSample neighbor =
                        sample_neighbor(chunk, neighborhood, neighbor_local, definition.direction);
                    const FaceDisposition disposition = is_empty(neighbor.material)
                                                            ? FaceDisposition::emitted
                                                            : FaceDisposition::culled;
                    if (!visit(local, material, definition, neighbor, disposition)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

template <typename VisitExposedFace>
bool for_each_exposed_face(const Chunk& chunk, const ChunkNeighborhood& neighborhood,
                           VisitExposedFace&& visit) {
    return for_each_face(
        chunk, neighborhood,
        [&](LocalVoxelCoord local, MaterialId material, const DirectionDefinition& definition,
            const NeighborSample&, FaceDisposition disposition) {
            return disposition == FaceDisposition::culled || visit(local, material, definition);
        });
}

[[nodiscard]] std::optional<std::size_t> checked_multiply(std::size_t value,
                                                          std::size_t multiplier) noexcept {
    if (multiplier != 0 && value > std::numeric_limits<std::size_t>::max() / multiplier) {
        return std::nullopt;
    }
    return value * multiplier;
}

struct FaceCounts {
    std::array<std::size_t, 3> by_pass{};
    std::size_t total = 0;
};

[[nodiscard]] std::optional<FaceCounts>
count_exposed_faces(const Chunk& chunk, const ChunkNeighborhood& neighborhood,
                    const MaterialPassTable& material_passes) {
    FaceCounts counts;
    const bool completed = for_each_exposed_face(
        chunk, neighborhood, [&](LocalVoxelCoord, MaterialId material, const DirectionDefinition&) {
            if (counts.total == kMaxNaiveChunkFaceCount) {
                return false;
            }
            ++counts.by_pass[render_pass_index(material_passes.get(material))];
            ++counts.total;
            return true;
        });
    if (!completed) {
        return std::nullopt;
    }
    return counts;
}

[[nodiscard]] bool can_store_counts(const ChunkMeshes& meshes, const FaceCounts& face_counts) {
    const std::array<const ChunkMesh*, 3> pass_meshes{
        &meshes.opaque,
        &meshes.cutout,
        &meshes.translucent,
    };
    for (std::size_t pass = 0; pass < pass_meshes.size(); ++pass) {
        const auto vertices =
            checked_multiply(face_counts.by_pass[pass], kNaiveMeshVerticesPerFace);
        const auto indices = checked_multiply(face_counts.by_pass[pass], kNaiveMeshIndicesPerFace);
        if (!vertices || !indices || *vertices > pass_meshes[pass]->vertices.max_size() ||
            *indices > pass_meshes[pass]->indices.max_size()) {
            return false;
        }
    }
    return true;
}

void reserve_meshes(ChunkMeshes& meshes, const FaceCounts& face_counts) {
    const std::array<ChunkMesh*, 3> pass_meshes{
        &meshes.opaque,
        &meshes.cutout,
        &meshes.translucent,
    };
    for (std::size_t pass = 0; pass < pass_meshes.size(); ++pass) {
        pass_meshes[pass]->vertices.reserve(face_counts.by_pass[pass] * kNaiveMeshVerticesPerFace);
        pass_meshes[pass]->indices.reserve(face_counts.by_pass[pass] * kNaiveMeshIndicesPerFace);
    }
}

} // namespace

const Chunk* ChunkNeighborhood::get(FaceDirection direction) const noexcept {
    return adjacent[face_direction_index(direction)];
}

std::size_t ChunkMesh::face_count() const noexcept {
    return indices.size() / kQuadIndices.size();
}

MeshRenderPass MaterialPassTable::get(MaterialId material) const noexcept {
    return entries_[material.value];
}

void MaterialPassTable::set(MaterialId material, MeshRenderPass pass) noexcept {
    entries_[material.value] = pass;
}

ChunkMeshBuildResult::ChunkMeshBuildResult(ChunkMeshes meshes) noexcept
    : meshes_(std::move(meshes)) {}

ChunkMeshBuildResult::ChunkMeshBuildResult(ChunkMeshBuildError error) noexcept : error_(error) {}

bool ChunkMeshBuildResult::has_value() const noexcept {
    return !error_.has_value();
}

ChunkMeshBuildError ChunkMeshBuildResult::error() const noexcept {
    return *error_;
}

const ChunkMeshes& ChunkMeshBuildResult::operator*() const noexcept {
    return meshes_;
}

ChunkMeshes& ChunkMeshBuildResult::operator*() noexcept {
    return meshes_;
}

ChunkMeshBuildResult build_naive_chunk_mesh(const Chunk& chunk,
                                            const ChunkNeighborhood& neighborhood,
                                            const MaterialPassTable& material_passes,
                                            ChunkMeshBuildLimits limits) {
    const auto face_counts = count_exposed_faces(chunk, neighborhood, material_passes);
    if (!face_counts) {
        return ChunkMeshBuildResult{ChunkMeshBuildError::count_overflow};
    }

    const auto vertex_count = checked_multiply(face_counts->total, kNaiveMeshVerticesPerFace);
    const auto index_count = checked_multiply(face_counts->total, kNaiveMeshIndicesPerFace);
    if (!vertex_count || !index_count || *vertex_count > kMaxNaiveChunkVertexCount ||
        *index_count > kMaxNaiveChunkIndexCount) {
        return ChunkMeshBuildResult{ChunkMeshBuildError::count_overflow};
    }
    if (*vertex_count > limits.max_vertices) {
        return ChunkMeshBuildResult{ChunkMeshBuildError::vertex_limit_exceeded};
    }
    if (*index_count > limits.max_indices) {
        return ChunkMeshBuildResult{ChunkMeshBuildError::index_limit_exceeded};
    }

    ChunkMeshes meshes;
    if (!can_store_counts(meshes, *face_counts)) {
        return ChunkMeshBuildResult{ChunkMeshBuildError::count_overflow};
    }
    reserve_meshes(meshes, *face_counts);

    const bool completed = for_each_exposed_face(
        chunk, neighborhood,
        [&](LocalVoxelCoord local, MaterialId material, const DirectionDefinition& definition) {
            append_face(mesh_for_pass(meshes, material_passes.get(material)), local, material,
                        definition);
            return true;
        });
    if (!completed) {
        return ChunkMeshBuildResult{ChunkMeshBuildError::count_overflow};
    }

    return ChunkMeshBuildResult{std::move(meshes)};
}

std::vector<ChunkFaceDiagnostic> describe_naive_chunk_faces(const Chunk& chunk,
                                                            const ChunkNeighborhood& neighborhood) {
    std::vector<ChunkFaceDiagnostic> diagnostics;
    diagnostics.reserve(kMaxNaiveChunkFaceCount);
    static_cast<void>(for_each_face(
        chunk, neighborhood,
        [&](LocalVoxelCoord local, MaterialId material, const DirectionDefinition& definition,
            const NeighborSample& neighbor, FaceDisposition disposition) {
            diagnostics.push_back({
                .source_local = local,
                .source_material = material,
                .direction = definition.direction,
                .neighbor_local = neighbor.local,
                .neighbor_material = neighbor.material,
                .neighbor_kind = neighbor.kind,
                .disposition = disposition,
            });
            return true;
        }));
    return diagnostics;
}

} // namespace wide_eye::voxel
