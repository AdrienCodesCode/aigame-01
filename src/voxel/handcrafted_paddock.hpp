#pragma once

#include "voxel/naive_mesher.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace wide_eye::voxel {

inline constexpr MaterialId kPaddockGrassMaterial{1};
inline constexpr MaterialId kPaddockStoneMaterial{2};
inline constexpr MaterialId kPaddockGateMaterial{3};
inline constexpr MaterialId kPaddockBarnWallMaterial{4};
inline constexpr MaterialId kPaddockBarnRoofMaterial{5};
inline constexpr MaterialId kPaddockBarnDoorMaterial{6};

struct PaddockPaletteColor {
    std::array<float, 3> rgb{};

    bool operator==(const PaddockPaletteColor&) const = default;
};

// The bounded paddock owns a deliberately small code-generated material
// palette. Entry zero is the non-rendered empty material; the six visible
// entries correspond exactly to the material IDs above.
class PaddockPalette {
  public:
    static constexpr std::size_t kEntryCount = 7;

    [[nodiscard]] std::optional<PaddockPaletteColor> get(MaterialId material) const noexcept;
    [[nodiscard]] const std::array<PaddockPaletteColor, kEntryCount>& entries() const noexcept;

  private:
    std::array<PaddockPaletteColor, kEntryCount> entries_{};

    friend PaddockPalette make_handcrafted_paddock_palette() noexcept;
};

[[nodiscard]] PaddockPalette make_handcrafted_paddock_palette() noexcept;

// A fixed two-by-one-by-two chunk world used to prove the production storage,
// cross-chunk neighbor sampling, naive mesher, and renderer as one visible path.
// It is deliberately handcrafted; terrain generation and streaming remain
// later outcomes.
class HandcraftedPaddock {
  public:
    static constexpr GridCoordinate kChunkCountX = 2;
    static constexpr GridCoordinate kChunkCountZ = 2;
    static constexpr std::size_t kChunkCount = 4;
    static constexpr GridCoordinate kWorldWidth = kChunkCountX * Chunk::kEdgeLength;
    static constexpr GridCoordinate kWorldDepth = kChunkCountZ * Chunk::kEdgeLength;

    [[nodiscard]] std::optional<MaterialId> get(WorldVoxelCoord world) const noexcept;
    [[nodiscard]] std::size_t count_blocks(MaterialId material) const noexcept;
    [[nodiscard]] std::size_t occupied_block_count() const noexcept;

  private:
    HandcraftedPaddock() = default;

    [[nodiscard]] Chunk* chunk_at(ChunkCoord coordinate) noexcept;
    [[nodiscard]] const Chunk* chunk_at(ChunkCoord coordinate) const noexcept;
    [[nodiscard]] bool set(WorldVoxelCoord world, MaterialId material) noexcept;

    std::array<Chunk, kChunkCount> chunks_{Chunk{}, Chunk{}, Chunk{}, Chunk{}};

    friend std::optional<HandcraftedPaddock> make_handcrafted_paddock() noexcept;
    friend std::optional<struct HandcraftedPaddockMesh>
    build_handcrafted_paddock_mesh(const HandcraftedPaddock& paddock);
};

struct HandcraftedPaddockMesh {
    ChunkMeshes passes;
    std::size_t source_chunk_count = 0;
    std::size_t occupied_block_count = 0;
    std::array<std::size_t, PaddockPalette::kEntryCount> opaque_faces_by_material{};
    struct FaceDiagnostic {
        ChunkCoord source_chunk;
        ChunkFaceDiagnostic face;

        bool operator==(const FaceDiagnostic&) const = default;
    };
    std::vector<FaceDiagnostic> face_diagnostics;
};

[[nodiscard]] std::optional<HandcraftedPaddock> make_handcrafted_paddock() noexcept;

// Produces world-space vertices by meshing each chunk against its live axial
// neighbors and then offsetting that chunk's complete checked output. The
// current handcrafted materials are all opaque, but pass separation is kept.
[[nodiscard]] std::optional<HandcraftedPaddockMesh>
build_handcrafted_paddock_mesh(const HandcraftedPaddock& paddock);

} // namespace wide_eye::voxel
