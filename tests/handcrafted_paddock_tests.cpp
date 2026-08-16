#include "voxel/handcrafted_paddock.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

using wide_eye::voxel::Chunk;
using wide_eye::voxel::ChunkCoord;
using wide_eye::voxel::FaceDirection;
using wide_eye::voxel::GridCoordinate;
using wide_eye::voxel::HandcraftedPaddockMesh;
using wide_eye::voxel::LocalVoxelCoord;
using wide_eye::voxel::MaterialId;

constexpr std::array<std::array<std::int8_t, 3>, wide_eye::voxel::kFaceDirectionCount> kFaceNormals{
    {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}}};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "handcrafted_paddock_test_failure=" << message << '\n';
        return false;
    }
    return true;
}

std::size_t count_faces(const wide_eye::voxel::ChunkMesh& mesh,
                        wide_eye::voxel::MaterialId material) {
    std::size_t vertices = 0;
    for (const wide_eye::voxel::ChunkMeshVertex& vertex : mesh.vertices) {
        if (vertex.material == material) {
            ++vertices;
        }
    }
    return vertices / wide_eye::voxel::kNaiveMeshVerticesPerFace;
}

bool every_index_is_valid(const wide_eye::voxel::ChunkMesh& mesh) {
    for (const std::uint32_t index : mesh.indices) {
        if (index >= mesh.vertices.size()) {
            return false;
        }
    }
    return true;
}

bool mesh_face_matches_diagnostic(const wide_eye::voxel::ChunkMesh& mesh, std::size_t face_index,
                                  const HandcraftedPaddockMesh::FaceDiagnostic& diagnostic) {
    using namespace wide_eye::voxel;

    const auto source_world = chunk_local_to_world(
        {.chunk = diagnostic.source_chunk, .local = diagnostic.face.source_local},
        ChunkEdgeLength{Chunk::kEdgeLength});
    if (!source_world.has_value()) {
        return false;
    }

    const std::size_t first_vertex = face_index * kNaiveMeshVerticesPerFace;
    const auto expected_normal = kFaceNormals[face_direction_index(diagnostic.face.direction)];
    for (std::size_t corner = 0; corner < kNaiveMeshVerticesPerFace; ++corner) {
        const ChunkMeshVertex& vertex = mesh.vertices[first_vertex + corner];
        if (vertex.material != diagnostic.face.source_material ||
            vertex.normal != expected_normal ||
            vertex.position[0] < static_cast<float>(source_world->x) ||
            vertex.position[0] > static_cast<float>(source_world->x + 1) ||
            vertex.position[1] < static_cast<float>(source_world->y) ||
            vertex.position[1] > static_cast<float>(source_world->y + 1) ||
            vertex.position[2] < static_cast<float>(source_world->z) ||
            vertex.position[2] > static_cast<float>(source_world->z + 1)) {
            return false;
        }
    }

    const auto& position = mesh.vertices[first_vertex].position;
    switch (diagnostic.face.direction) {
    case FaceDirection::negative_x:
        return position[0] == static_cast<float>(source_world->x);
    case FaceDirection::positive_x:
        return position[0] == static_cast<float>(source_world->x + 1);
    case FaceDirection::negative_y:
        return position[1] == static_cast<float>(source_world->y);
    case FaceDirection::positive_y:
        return position[1] == static_cast<float>(source_world->y + 1);
    case FaceDirection::negative_z:
        return position[2] == static_cast<float>(source_world->z);
    case FaceDirection::positive_z:
        return position[2] == static_cast<float>(source_world->z + 1);
    }
    return false;
}

ChunkCoord adjacent_chunk(ChunkCoord chunk, FaceDirection direction) {
    switch (direction) {
    case FaceDirection::negative_x:
        --chunk.x;
        break;
    case FaceDirection::positive_x:
        ++chunk.x;
        break;
    case FaceDirection::negative_y:
        --chunk.y;
        break;
    case FaceDirection::positive_y:
        ++chunk.y;
        break;
    case FaceDirection::negative_z:
        --chunk.z;
        break;
    case FaceDirection::positive_z:
        ++chunk.z;
        break;
    }
    return chunk;
}

bool contains_face_diagnostic(
    const std::vector<HandcraftedPaddockMesh::FaceDiagnostic>& diagnostics, ChunkCoord source_chunk,
    LocalVoxelCoord source_local, FaceDirection direction, MaterialId neighbor_material,
    wide_eye::voxel::FaceNeighborKind neighbor_kind, wide_eye::voxel::FaceDisposition disposition) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.source_chunk == source_chunk &&
            diagnostic.face.source_local == source_local &&
            diagnostic.face.direction == direction &&
            diagnostic.face.neighbor_material == neighbor_material &&
            diagnostic.face.neighbor_kind == neighbor_kind &&
            diagnostic.face.disposition == disposition) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    using namespace wide_eye::voxel;

    bool passed = true;
    const PaddockPalette palette = make_handcrafted_paddock_palette();
    passed &= expect(palette.entries().size() == PaddockPalette::kEntryCount,
                     "palette must stay bounded to empty plus six visible materials");
    passed &= expect(palette.get(kEmptyMaterialId) == PaddockPaletteColor{{0.0F, 0.0F, 0.0F}},
                     "empty material must retain a non-rendered black palette entry");
    const std::array<MaterialId, 6> required_materials{
        kPaddockGrassMaterial,    kPaddockStoneMaterial,    kPaddockGateMaterial,
        kPaddockBarnWallMaterial, kPaddockBarnRoofMaterial, kPaddockBarnDoorMaterial,
    };
    for (const MaterialId material : required_materials) {
        const auto color = palette.get(material);
        passed &= expect(color.has_value(), "every handcrafted material must have a palette entry");
        if (color.has_value()) {
            passed &=
                expect(color->rgb[0] >= 0.0F && color->rgb[0] <= 1.0F && color->rgb[1] >= 0.0F &&
                           color->rgb[1] <= 1.0F && color->rgb[2] >= 0.0F && color->rgb[2] <= 1.0F,
                       "every palette channel must be normalized");
        }
    }
    passed &= expect(!palette.get(static_cast<MaterialId>(PaddockPalette::kEntryCount)).has_value(),
                     "palette lookup must reject unowned material IDs");

    const auto paddock = make_handcrafted_paddock();
    passed &= expect(paddock.has_value(), "scene construction must succeed");
    if (!paddock.has_value()) {
        return EXIT_FAILURE;
    }

    passed &= expect(paddock->get({.x = 0, .y = 0, .z = 0}) == kPaddockGrassMaterial,
                     "ground corner must be grass");
    passed &= expect(paddock->get({.x = 1, .y = 2, .z = 14}) == kPaddockStoneMaterial,
                     "wall sample must be stone");
    passed &= expect(paddock->get({.x = 16, .y = 2, .z = 15}) == kPaddockGateMaterial,
                     "center gate sample must be red-gate material");
    passed &= expect(paddock->get({.x = 4, .y = 4, .z = 4}) == kPaddockBarnWallMaterial,
                     "barn body sample must use wall material");
    passed &= expect(paddock->get({.x = 6, .y = 9, .z = 4}) == kPaddockBarnRoofMaterial,
                     "barn ridge sample must use roof material");
    passed &= expect(paddock->get({.x = 6, .y = 3, .z = 8}) == kPaddockBarnDoorMaterial,
                     "barn front sample must use door material");
    passed &= expect(!paddock->get({.x = -1, .y = 0, .z = 0}).has_value() &&
                         !paddock->get({.x = 32, .y = 0, .z = 0}).has_value(),
                     "outside-world samples must be rejected");

    passed &= expect(paddock->count_blocks(kPaddockGrassMaterial) == 1'024,
                     "ground must fill exactly one 32 by 32 layer");
    passed &= expect(paddock->count_blocks(kPaddockStoneMaterial) == 160,
                     "wall and gateposts must preserve their exact stone count");
    passed &= expect(paddock->count_blocks(kPaddockGateMaterial) == 12,
                     "gate must preserve its exact red block count");
    passed &= expect(paddock->count_blocks(kPaddockBarnWallMaterial) == 264,
                     "barn body count must account for the material-replaced door");
    passed &= expect(paddock->count_blocks(kPaddockBarnRoofMaterial) == 270,
                     "stepped barn roof must preserve its exact block count");
    passed &= expect(paddock->count_blocks(kPaddockBarnDoorMaterial) == 16,
                     "barn door must preserve its exact block count");
    passed &= expect(paddock->occupied_block_count() == 1'746,
                     "scene occupied count must equal all handcrafted materials");

    const auto mesh = build_handcrafted_paddock_mesh(*paddock);
    passed &= expect(mesh.has_value(), "four checked chunk meshes must merge successfully");
    if (!mesh.has_value()) {
        return EXIT_FAILURE;
    }

    passed &= expect(mesh->source_chunk_count == HandcraftedPaddock::kChunkCount,
                     "mesh must report all four source chunks");
    passed &= expect(mesh->occupied_block_count == paddock->occupied_block_count(),
                     "mesh metadata must retain the scene occupied count");
    passed &= expect(mesh->passes.cutout.vertices.empty() && mesh->passes.cutout.indices.empty() &&
                         mesh->passes.translucent.vertices.empty() &&
                         mesh->passes.translucent.indices.empty(),
                     "handcrafted scene must retain empty non-opaque passes");
    passed &= expect(!mesh->passes.opaque.vertices.empty() && !mesh->passes.opaque.indices.empty(),
                     "handcrafted scene must emit opaque geometry");
    passed &= expect(mesh->passes.opaque.vertices.size() % kNaiveMeshVerticesPerFace == 0 &&
                         mesh->passes.opaque.indices.size() % kNaiveMeshIndicesPerFace == 0,
                     "merged mesh must retain whole quad topology");
    passed &= expect(every_index_is_valid(mesh->passes.opaque),
                     "every merged index must address a merged vertex");
    passed &= expect(count_faces(mesh->passes.opaque, kPaddockGrassMaterial) == 2'064,
                     "ground faces must remove internal chunk borders and covered tops");
    passed &= expect(count_faces(mesh->passes.opaque, kPaddockGateMaterial) == 28,
                     "gate faces must be occluded by ground and stone posts");
    passed &= expect(count_faces(mesh->passes.opaque, kPaddockBarnDoorMaterial) == 16,
                     "each exposed barn-door block must contribute its front face only");

    for (const MaterialId material : required_materials) {
        const auto material_index = static_cast<std::size_t>(material.value);
        const std::size_t counted_faces = count_faces(mesh->passes.opaque, material);
        passed &= expect(counted_faces > 0,
                         "every handcrafted material must reach visible mesh geometry");
        passed &= expect(mesh->opaque_faces_by_material[material_index] == counted_faces,
                         "mesh statistics must match independently counted material faces");
    }
    passed &= expect(mesh->opaque_faces_by_material[kEmptyMaterialId.value] == 0,
                     "empty material must never contribute mesh statistics");

    constexpr std::size_t kWorldCellCount = static_cast<std::size_t>(
        HandcraftedPaddock::kWorldWidth * Chunk::kEdgeLength * HandcraftedPaddock::kWorldDepth);
    std::array<bool, kWorldCellCount * kFaceDirectionCount> described_sides{};
    std::array<std::size_t, kFaceDirectionCount> emitted_by_direction{};
    std::array<std::size_t, kFaceDirectionCount> culled_by_direction{};
    std::array<std::size_t, kFaceNeighborKindCount> decisions_by_neighbor_kind{};
    std::size_t emitted_diagnostics = 0;
    std::size_t culled_diagnostics = 0;
    std::size_t mesh_face = 0;
    for (const HandcraftedPaddockMesh::FaceDiagnostic& diagnostic : mesh->face_diagnostics) {
        const auto source_world = chunk_local_to_world(
            {.chunk = diagnostic.source_chunk, .local = diagnostic.face.source_local},
            ChunkEdgeLength{Chunk::kEdgeLength});
        passed &= expect(source_world.has_value(),
                         "every face diagnostic source must recompose to world space");
        if (!source_world.has_value()) {
            continue;
        }
        passed &=
            expect(source_world->x >= 0 && source_world->x < HandcraftedPaddock::kWorldWidth &&
                       source_world->y >= 0 && source_world->y < Chunk::kEdgeLength &&
                       source_world->z >= 0 && source_world->z < HandcraftedPaddock::kWorldDepth,
                   "every diagnostic source must lie inside the bounded paddock");
        if (source_world->x < 0 || source_world->x >= HandcraftedPaddock::kWorldWidth ||
            source_world->y < 0 || source_world->y >= Chunk::kEdgeLength || source_world->z < 0 ||
            source_world->z >= HandcraftedPaddock::kWorldDepth) {
            continue;
        }

        const std::size_t direction_index = face_direction_index(diagnostic.face.direction);
        const std::size_t cell_index =
            static_cast<std::size_t>((source_world->z * Chunk::kEdgeLength + source_world->y) *
                                         HandcraftedPaddock::kWorldWidth +
                                     source_world->x);
        const std::size_t side_index = cell_index * kFaceDirectionCount + direction_index;
        passed &= expect(!described_sides[side_index],
                         "each occupied voxel side must have exactly one diagnostic");
        described_sides[side_index] = true;
        passed &= expect(paddock->get(*source_world) == diagnostic.face.source_material &&
                             !is_empty(diagnostic.face.source_material),
                         "diagnostic source material must match live non-empty storage");

        ChunkCoord neighbor_chunk = diagnostic.source_chunk;
        if (diagnostic.face.neighbor_kind != FaceNeighborKind::same_chunk) {
            neighbor_chunk = adjacent_chunk(neighbor_chunk, diagnostic.face.direction);
        }
        const auto neighbor_world =
            chunk_local_to_world({.chunk = neighbor_chunk, .local = diagnostic.face.neighbor_local},
                                 ChunkEdgeLength{Chunk::kEdgeLength});
        passed &= expect(neighbor_world.has_value(),
                         "every diagnostic neighbor must recompose to world space");
        if (!neighbor_world.has_value()) {
            continue;
        }
        const auto stored_neighbor = paddock->get(*neighbor_world);
        if (diagnostic.face.neighbor_kind == FaceNeighborKind::missing_chunk) {
            passed &=
                expect(!stored_neighbor.has_value() && is_empty(diagnostic.face.neighbor_material),
                       "missing-chunk diagnostics must identify absent empty neighbors");
        } else {
            passed &= expect(stored_neighbor == diagnostic.face.neighbor_material,
                             "stored-neighbor diagnostics must match live material storage");
        }

        const bool should_emit = is_empty(diagnostic.face.neighbor_material);
        passed &= expect((diagnostic.face.disposition == FaceDisposition::emitted) == should_emit,
                         "face disposition must follow the sampled neighbor material");
        ++decisions_by_neighbor_kind[face_neighbor_kind_index(diagnostic.face.neighbor_kind)];
        if (diagnostic.face.disposition == FaceDisposition::emitted) {
            ++emitted_diagnostics;
            ++emitted_by_direction[direction_index];
            passed &=
                expect(mesh_face < mesh->passes.opaque.face_count() &&
                           mesh_face_matches_diagnostic(mesh->passes.opaque, mesh_face, diagnostic),
                       "each emitted diagnostic must explain the next rendered quad");
            ++mesh_face;
        } else {
            ++culled_diagnostics;
            ++culled_by_direction[direction_index];
        }
    }

    passed &=
        expect(mesh->face_diagnostics.size() == mesh->occupied_block_count * kFaceDirectionCount,
               "every occupied block must have six face explanations");
    passed &= expect(mesh->face_diagnostics.size() == 10'476 && emitted_diagnostics == 2'754 &&
                         culled_diagnostics == 7'722,
                     "paddock face ledger must retain exact emitted and culled totals");
    passed &= expect(mesh_face == mesh->passes.opaque.face_count() &&
                         emitted_diagnostics == mesh->passes.opaque.face_count(),
                     "emitted diagnostics must reconcile one-to-one with rendered quads");
    passed &= expect(
        decisions_by_neighbor_kind[face_neighbor_kind_index(FaceNeighborKind::same_chunk)] ==
                9'098 &&
            decisions_by_neighbor_kind[face_neighbor_kind_index(
                FaceNeighborKind::adjacent_chunk)] == 226 &&
            decisions_by_neighbor_kind[face_neighbor_kind_index(FaceNeighborKind::missing_chunk)] ==
                1'152,
        "paddock face ledger must retain exact same, adjacent, and missing chunk totals");
    for (GridCoordinate z = 0; z < HandcraftedPaddock::kWorldDepth; ++z) {
        for (GridCoordinate y = 0; y < Chunk::kEdgeLength; ++y) {
            for (GridCoordinate x = 0; x < HandcraftedPaddock::kWorldWidth; ++x) {
                const bool occupied =
                    !is_empty(paddock->get({.x = x, .y = y, .z = z}).value_or(kEmptyMaterialId));
                const std::size_t cell_index = static_cast<std::size_t>(
                    (z * Chunk::kEdgeLength + y) * HandcraftedPaddock::kWorldWidth + x);
                for (std::size_t direction = 0; direction < kFaceDirectionCount; ++direction) {
                    passed &= expect(
                        described_sides[cell_index * kFaceDirectionCount + direction] == occupied,
                        "only occupied blocks may own six described sides");
                }
            }
        }
    }
    for (std::size_t direction = 0; direction < kFaceDirectionCount; ++direction) {
        passed &= expect(emitted_by_direction[direction] + culled_by_direction[direction] ==
                             mesh->occupied_block_count,
                         "each direction must classify one side per occupied block");
    }
    passed &=
        expect(contains_face_diagnostic(mesh->face_diagnostics, {.x = 0, .y = 0, .z = 0},
                                        {.x = 15, .y = 0, .z = 0}, FaceDirection::positive_x,
                                        kPaddockGrassMaterial, FaceNeighborKind::adjacent_chunk,
                                        FaceDisposition::culled),
               "ledger must explain a culled cross-chunk ground face");
    passed &= expect(contains_face_diagnostic(mesh->face_diagnostics, {.x = 0, .y = 0, .z = 0},
                                              {.x = 0, .y = 0, .z = 0}, FaceDirection::negative_x,
                                              kEmptyMaterialId, FaceNeighborKind::missing_chunk,
                                              FaceDisposition::emitted),
                     "ledger must explain an emitted outer-world ground face");
    passed &= expect(contains_face_diagnostic(
                         mesh->face_diagnostics, {.x = 0, .y = 0, .z = 0}, {.x = 4, .y = 0, .z = 4},
                         FaceDirection::positive_y, kPaddockBarnWallMaterial,
                         FaceNeighborKind::same_chunk, FaceDisposition::culled),
                     "ledger must explain a ground face hidden by the barn");

    if (!passed) {
        return EXIT_FAILURE;
    }

    std::cout
        << "paddock_chunks=" << mesh->source_chunk_count << '\n'
        << "paddock_occupied_blocks=" << mesh->occupied_block_count << '\n'
        << "paddock_faces=" << mesh->passes.opaque.face_count() << '\n'
        << "paddock_vertices=" << mesh->passes.opaque.vertices.size() << '\n'
        << "paddock_indices=" << mesh->passes.opaque.indices.size() << '\n'
        << "paddock_face_decisions=" << mesh->face_diagnostics.size() << '\n'
        << "paddock_emitted_face_decisions=" << emitted_diagnostics << '\n'
        << "paddock_culled_face_decisions=" << culled_diagnostics << '\n'
        << "paddock_same_chunk_neighbor_decisions="
        << decisions_by_neighbor_kind[face_neighbor_kind_index(FaceNeighborKind::same_chunk)]
        << '\n'
        << "paddock_adjacent_chunk_neighbor_decisions="
        << decisions_by_neighbor_kind[face_neighbor_kind_index(FaceNeighborKind::adjacent_chunk)]
        << '\n'
        << "paddock_missing_chunk_neighbor_decisions="
        << decisions_by_neighbor_kind[face_neighbor_kind_index(FaceNeighborKind::missing_chunk)]
        << '\n'
        << "handcrafted_paddock_result=pass\n";
    return EXIT_SUCCESS;
}
