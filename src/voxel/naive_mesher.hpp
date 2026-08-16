#pragma once

#include "voxel/chunk.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace wide_eye::voxel {

enum class FaceDirection : std::uint8_t {
    negative_x,
    positive_x,
    negative_y,
    positive_y,
    negative_z,
    positive_z,
};

inline constexpr std::size_t kFaceDirectionCount = 6;

[[nodiscard]] constexpr std::size_t face_direction_index(FaceDirection direction) noexcept {
    return static_cast<std::size_t>(direction);
}

static_assert(face_direction_index(FaceDirection::positive_z) + 1U == kFaceDirectionCount);

// A read-only snapshot of the six chunks touching the chunk being meshed.
// Missing neighbors are sampled as empty space. The world/rebuild owner keeps
// every pointed-to chunk alive and invalidates both meshes when a shared border
// changes; neither Chunk nor the mesher owns cross-chunk invalidation.
struct ChunkNeighborhood {
    std::array<const Chunk*, kFaceDirectionCount> adjacent{};

    [[nodiscard]] const Chunk* get(FaceDirection direction) const noexcept;
};

enum class FaceNeighborKind : std::uint8_t {
    same_chunk,
    adjacent_chunk,
    missing_chunk,
};

inline constexpr std::size_t kFaceNeighborKindCount = 3;

[[nodiscard]] constexpr std::size_t face_neighbor_kind_index(FaceNeighborKind kind) noexcept {
    return static_cast<std::size_t>(kind);
}

static_assert(face_neighbor_kind_index(FaceNeighborKind::missing_chunk) + 1U ==
              kFaceNeighborKindCount);

enum class FaceDisposition : std::uint8_t {
    emitted,
    culled,
};

// One deterministic explanation for one side of one non-empty cell. The
// wrapped neighbor local is valid even when the adjacent chunk is missing;
// neighbor_kind distinguishes that case from stored empty space. A face is
// emitted exactly when its sampled neighbor material is empty.
struct ChunkFaceDiagnostic {
    LocalVoxelCoord source_local;
    MaterialId source_material;
    FaceDirection direction = FaceDirection::negative_x;
    LocalVoxelCoord neighbor_local;
    MaterialId neighbor_material;
    FaceNeighborKind neighbor_kind = FaceNeighborKind::same_chunk;
    FaceDisposition disposition = FaceDisposition::emitted;

    bool operator==(const ChunkFaceDiagnostic&) const = default;
};

// Integer-valued positions are stored as floats for direct use by a later
// renderer. Every face has four unique vertices so material and normal data
// remain face-local in this deliberately naive baseline.
struct ChunkMeshVertex {
    std::array<float, 3> position{};
    std::array<std::int8_t, 3> normal{};
    MaterialId material{};

    bool operator==(const ChunkMeshVertex&) const = default;
};

struct ChunkMesh {
    std::vector<ChunkMeshVertex> vertices;
    std::vector<std::uint32_t> indices;

    [[nodiscard]] std::size_t face_count() const noexcept;
};

enum class MeshRenderPass : std::uint8_t {
    opaque,
    cutout,
    translucent,
};

// A palette-owned classification snapshot for meshing. Value initialization
// classifies every material as opaque, preserving the original naive baseline;
// empty material never emits geometry regardless of its table entry.
class MaterialPassTable {
  public:
    [[nodiscard]] MeshRenderPass get(MaterialId material) const noexcept;
    void set(MaterialId material, MeshRenderPass pass) noexcept;

  private:
    static constexpr std::size_t kEntryCount = 256;
    std::array<MeshRenderPass, kEntryCount> entries_{};
};

// Independent CPU buffers let the renderer eventually submit each pass with
// its own depth/blending state without re-partitioning geometry.
struct ChunkMeshes {
    ChunkMesh opaque;
    ChunkMesh cutout;
    ChunkMesh translucent;
};

inline constexpr std::size_t kNaiveMeshVerticesPerFace = 4;
inline constexpr std::size_t kNaiveMeshIndicesPerFace = 6;

// This deliberately conservative ceiling treats every face of every cell as
// exposed. Real occupancy cannot exceed it, and the fixed 16^3 production
// chunk keeps both the vertex count and every uint32 index representable.
inline constexpr std::size_t kMaxNaiveChunkFaceCount = Chunk::kCellCount * kFaceDirectionCount;
inline constexpr std::size_t kMaxNaiveChunkVertexCount =
    kMaxNaiveChunkFaceCount * kNaiveMeshVerticesPerFace;
inline constexpr std::size_t kMaxNaiveChunkIndexCount =
    kMaxNaiveChunkFaceCount * kNaiveMeshIndicesPerFace;

// Limits apply to the aggregate output across opaque, cutout, and translucent
// buffers. A future rebuild owner can impose a tighter per-job budget without
// allowing a partially built mesh to escape.
struct ChunkMeshBuildLimits {
    std::size_t max_vertices = kMaxNaiveChunkVertexCount;
    std::size_t max_indices = kMaxNaiveChunkIndexCount;
};

enum class ChunkMeshBuildError : std::uint8_t {
    count_overflow,
    vertex_limit_exceeded,
    index_limit_exceeded,
};

class ChunkMeshBuildResult {
  public:
    explicit ChunkMeshBuildResult(ChunkMeshes meshes) noexcept;
    explicit ChunkMeshBuildResult(ChunkMeshBuildError error) noexcept;

    [[nodiscard]] bool has_value() const noexcept;
    [[nodiscard]] ChunkMeshBuildError error() const noexcept;
    [[nodiscard]] const ChunkMeshes& operator*() const noexcept;
    [[nodiscard]] ChunkMeshes& operator*() noexcept;

  private:
    ChunkMeshes meshes_;
    std::optional<ChunkMeshBuildError> error_;
};

// Every non-empty material still occludes every other non-empty material, so
// the verified opaque visibility and geometry order remain unchanged. This
// checked build counts and classifies all exposed faces before allocating or
// emitting. It rejects arithmetic/type overflow and caller limits atomically;
// greedy merging, upload, and drawing remain later outcomes.
[[nodiscard]] ChunkMeshBuildResult
build_naive_chunk_mesh(const Chunk& chunk, const ChunkNeighborhood& neighborhood,
                       const MaterialPassTable& material_passes = {},
                       ChunkMeshBuildLimits limits = {});

// Returns one record for every side of every non-empty cell in the exact
// z/y/x/direction traversal used by the naive mesh build. This diagnostic path
// is intentionally caller-requested so normal mesh builds do not retain the
// full face-decision ledger.
[[nodiscard]] std::vector<ChunkFaceDiagnostic>
describe_naive_chunk_faces(const Chunk& chunk, const ChunkNeighborhood& neighborhood);

} // namespace wide_eye::voxel
