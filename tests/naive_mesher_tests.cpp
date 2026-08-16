#include "voxel/naive_mesher.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

using wide_eye::voxel::Chunk;
using wide_eye::voxel::ChunkMesh;
using wide_eye::voxel::ChunkMeshBuildError;
using wide_eye::voxel::ChunkMeshBuildLimits;
using wide_eye::voxel::ChunkMeshBuildResult;
using wide_eye::voxel::ChunkMeshes;
using wide_eye::voxel::ChunkMeshVertex;
using wide_eye::voxel::ChunkNeighborhood;
using wide_eye::voxel::FaceDirection;
using wide_eye::voxel::FaceDisposition;
using wide_eye::voxel::FaceNeighborKind;
using wide_eye::voxel::LocalVoxelCoord;
using wide_eye::voxel::MaterialId;
using wide_eye::voxel::MaterialPassTable;
using wide_eye::voxel::MeshRenderPass;
using wide_eye::voxel::SetBlockResult;

constexpr MaterialId kGrass{.value = 1};
constexpr MaterialId kStone{.value = 2};
constexpr MaterialId kLeaves{.value = 3};
constexpr MaterialId kWater{.value = 4};

constexpr std::array<FaceDirection, wide_eye::voxel::kFaceDirectionCount> kDirections{
    FaceDirection::negative_x, FaceDirection::positive_x, FaceDirection::negative_y,
    FaceDirection::positive_y, FaceDirection::negative_z, FaceDirection::positive_z,
};

constexpr std::array<std::array<std::int8_t, 3>, wide_eye::voxel::kFaceDirectionCount>
    kExpectedNormals{{{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}}};

constexpr std::array<LocalVoxelCoord, wide_eye::voxel::kFaceDirectionCount> kBoundaryCells{
    LocalVoxelCoord{.x = 0, .y = 7, .z = 8}, LocalVoxelCoord{.x = 15, .y = 7, .z = 8},
    LocalVoxelCoord{.x = 7, .y = 0, .z = 8}, LocalVoxelCoord{.x = 7, .y = 15, .z = 8},
    LocalVoxelCoord{.x = 7, .y = 8, .z = 0}, LocalVoxelCoord{.x = 7, .y = 8, .z = 15},
};

constexpr std::array<LocalVoxelCoord, wide_eye::voxel::kFaceDirectionCount> kWrappedCells{
    LocalVoxelCoord{.x = 15, .y = 7, .z = 8}, LocalVoxelCoord{.x = 0, .y = 7, .z = 8},
    LocalVoxelCoord{.x = 7, .y = 15, .z = 8}, LocalVoxelCoord{.x = 7, .y = 0, .z = 8},
    LocalVoxelCoord{.x = 7, .y = 8, .z = 15}, LocalVoxelCoord{.x = 7, .y = 8, .z = 0},
};

bool check(bool condition, std::string_view stage) {
    if (condition) {
        return true;
    }
    std::cerr << "naive_mesher_result=fail\n"
              << "failure_stage=" << stage << '\n';
    return false;
}

[[nodiscard]] std::array<float, 3> subtract(const std::array<float, 3>& left,
                                            const std::array<float, 3>& right) noexcept {
    return {left[0] - right[0], left[1] - right[1], left[2] - right[2]};
}

[[nodiscard]] std::array<float, 3> cross(const std::array<float, 3>& left,
                                         const std::array<float, 3>& right) noexcept {
    return {
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    };
}

bool check_mesh_shape(const ChunkMesh& mesh, std::size_t expected_faces, std::string_view stage) {
    return check(mesh.face_count() == expected_faces &&
                     mesh.vertices.size() == expected_faces * 4U &&
                     mesh.indices.size() == expected_faces * 6U,
                 stage);
}

bool check_mesh_shapes(const ChunkMeshes& meshes, std::size_t expected_opaque,
                       std::size_t expected_cutout, std::size_t expected_translucent,
                       std::string_view stage) {
    return check_mesh_shape(meshes.opaque, expected_opaque, stage) &&
           check_mesh_shape(meshes.cutout, expected_cutout, stage) &&
           check_mesh_shape(meshes.translucent, expected_translucent, stage);
}

bool check_mesh_shapes(const ChunkMeshBuildResult& result, std::size_t expected_opaque,
                       std::size_t expected_cutout, std::size_t expected_translucent,
                       std::string_view stage) {
    return check(result.has_value(), stage) &&
           check_mesh_shapes(*result, expected_opaque, expected_cutout, expected_translucent,
                             stage);
}

bool check_face_geometry(const ChunkMesh& mesh, std::size_t face, MaterialId expected_material) {
    const std::size_t first_vertex = face * 4U;
    const std::size_t first_index = face * 6U;
    constexpr std::array<std::uint32_t, 6> kExpectedLocalIndices{0, 1, 2, 0, 2, 3};
    for (std::size_t offset = 0; offset < kExpectedLocalIndices.size(); ++offset) {
        const auto expected =
            static_cast<std::uint32_t>(first_vertex) + kExpectedLocalIndices[offset];
        if (!check(mesh.indices[first_index + offset] == expected, "quad_index_topology")) {
            return false;
        }
    }

    const ChunkMeshVertex& first = mesh.vertices[first_vertex];
    for (std::size_t offset = 0; offset < 4U; ++offset) {
        const ChunkMeshVertex& vertex = mesh.vertices[first_vertex + offset];
        if (!check(vertex.normal == first.normal && vertex.material == expected_material,
                   "face_normal_and_material") ||
            !check(vertex.position[0] >= 0.0F && vertex.position[0] <= 16.0F &&
                       vertex.position[1] >= 0.0F && vertex.position[1] <= 16.0F &&
                       vertex.position[2] >= 0.0F && vertex.position[2] <= 16.0F,
                   "vertex_inside_chunk_extent")) {
            return false;
        }
    }

    const auto edge_one = subtract(mesh.vertices[first_vertex + 1U].position, first.position);
    const auto edge_two = subtract(mesh.vertices[first_vertex + 2U].position, first.position);
    const auto triangle_normal = cross(edge_one, edge_two);
    return check(triangle_normal[0] == static_cast<float>(first.normal[0]) &&
                     triangle_normal[1] == static_cast<float>(first.normal[1]) &&
                     triangle_normal[2] == static_cast<float>(first.normal[2]),
                 "counter_clockwise_outward_winding");
}

bool test_empty_and_single_voxel() {
    const ChunkNeighborhood no_neighbors;
    const Chunk empty;
    if (!check_mesh_shapes(wide_eye::voxel::build_naive_chunk_mesh(empty, no_neighbors), 0, 0, 0,
                           "empty_chunk_has_no_geometry")) {
        return false;
    }
    if (!check(wide_eye::voxel::describe_naive_chunk_faces(empty, no_neighbors).empty(),
               "empty_chunk_has_no_face_diagnostics")) {
        return false;
    }

    Chunk single;
    if (!check(single.set({.x = 7, .y = 8, .z = 9}, kStone) == SetBlockResult::changed,
               "single_voxel_setup")) {
        return false;
    }
    const ChunkMeshBuildResult result =
        wide_eye::voxel::build_naive_chunk_mesh(single, no_neighbors);
    if (!check(result.has_value(), "single_voxel_build_succeeds")) {
        return false;
    }
    const ChunkMeshes& meshes = *result;
    const ChunkMesh& mesh = meshes.opaque;
    const auto diagnostics = wide_eye::voxel::describe_naive_chunk_faces(single, no_neighbors);
    if (!check_mesh_shapes(meshes, 6, 0, 0, "single_voxel_has_six_opaque_faces") ||
        !check(single.dirty_region().has_value(), "meshing_preserves_dirty_state") ||
        !check(diagnostics.size() == kDirections.size(),
               "single_voxel_has_one_diagnostic_per_side")) {
        return false;
    }
    for (std::size_t face = 0; face < mesh.face_count(); ++face) {
        if (!check_face_geometry(mesh, face, kStone) ||
            !check(mesh.vertices[face * 4U].normal == kExpectedNormals[face],
                   "single_voxel_direction_order") ||
            !check(diagnostics[face].source_local == LocalVoxelCoord{.x = 7, .y = 8, .z = 9} &&
                       diagnostics[face].source_material == kStone &&
                       diagnostics[face].direction == kDirections[face] &&
                       diagnostics[face].neighbor_material == wide_eye::voxel::kEmptyMaterialId &&
                       diagnostics[face].neighbor_kind == FaceNeighborKind::same_chunk &&
                       diagnostics[face].disposition == FaceDisposition::emitted,
                   "single_voxel_diagnostic_matches_emitted_face")) {
            return false;
        }
    }
    return true;
}

bool test_internal_face_culling() {
    const ChunkNeighborhood no_neighbors;
    Chunk adjacent;
    if (!check(adjacent.set({.x = 7, .y = 8, .z = 9}, kGrass) == SetBlockResult::changed,
               "adjacent_first_setup") ||
        !check(adjacent.set({.x = 8, .y = 8, .z = 9}, kStone) == SetBlockResult::changed,
               "adjacent_second_setup")) {
        return false;
    }
    const auto diagnostics = wide_eye::voxel::describe_naive_chunk_faces(adjacent, no_neighbors);
    std::size_t emitted = 0;
    std::size_t culled = 0;
    bool grass_positive_x_cull = false;
    bool stone_negative_x_cull = false;
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.disposition == FaceDisposition::emitted) {
            ++emitted;
        } else {
            ++culled;
        }
        grass_positive_x_cull |=
            diagnostic.source_local == LocalVoxelCoord{.x = 7, .y = 8, .z = 9} &&
            diagnostic.source_material == kGrass &&
            diagnostic.direction == FaceDirection::positive_x &&
            diagnostic.neighbor_local == LocalVoxelCoord{.x = 8, .y = 8, .z = 9} &&
            diagnostic.neighbor_material == kStone &&
            diagnostic.neighbor_kind == FaceNeighborKind::same_chunk &&
            diagnostic.disposition == FaceDisposition::culled;
        stone_negative_x_cull |=
            diagnostic.source_local == LocalVoxelCoord{.x = 8, .y = 8, .z = 9} &&
            diagnostic.source_material == kStone &&
            diagnostic.direction == FaceDirection::negative_x &&
            diagnostic.neighbor_local == LocalVoxelCoord{.x = 7, .y = 8, .z = 9} &&
            diagnostic.neighbor_material == kGrass &&
            diagnostic.neighbor_kind == FaceNeighborKind::same_chunk &&
            diagnostic.disposition == FaceDisposition::culled;
    }
    return check_mesh_shapes(wide_eye::voxel::build_naive_chunk_mesh(adjacent, no_neighbors), 10, 0,
                             0, "non_empty_materials_cull_shared_face") &&
           check(diagnostics.size() == 12 && emitted == 10 && culled == 2,
                 "adjacent_diagnostics_partition_every_side") &&
           check(grass_positive_x_cull && stone_negative_x_cull,
                 "adjacent_diagnostics_explain_both_missing_faces");
}

bool test_full_chunk_surface() {
    const ChunkNeighborhood no_neighbors;
    const Chunk full{kGrass};
    const ChunkMeshBuildResult exposed =
        wide_eye::voxel::build_naive_chunk_mesh(full, no_neighbors);
    if (!check_mesh_shapes(exposed, 6U * 16U * 16U, 0, 0, "full_chunk_surface_only")) {
        return false;
    }

    ChunkNeighborhood surrounded;
    std::array<Chunk, wide_eye::voxel::kFaceDirectionCount> full_neighbors{
        Chunk{kStone}, Chunk{kStone}, Chunk{kStone}, Chunk{kStone}, Chunk{kStone}, Chunk{kStone},
    };
    for (std::size_t direction = 0; direction < full_neighbors.size(); ++direction) {
        surrounded.adjacent[direction] = &full_neighbors[direction];
    }
    return check_mesh_shapes(wide_eye::voxel::build_naive_chunk_mesh(full, surrounded), 0, 0, 0,
                             "surrounded_full_chunk_has_no_exposed_faces");
}

bool test_all_cross_chunk_boundaries() {
    for (std::size_t direction = 0; direction < kDirections.size(); ++direction) {
        Chunk target;
        Chunk neighbor;
        if (!check(target.set(kBoundaryCells[direction], kGrass) == SetBlockResult::changed,
                   "boundary_target_setup") ||
            !check(neighbor.set(kWrappedCells[direction], kStone) == SetBlockResult::changed,
                   "boundary_neighbor_setup")) {
            return false;
        }

        ChunkNeighborhood neighborhood;
        neighborhood.adjacent[wide_eye::voxel::face_direction_index(kDirections[direction])] =
            &neighbor;
        if (!check(neighborhood.get(kDirections[direction]) == &neighbor,
                   "explicit_neighbor_lookup") ||
            !check_mesh_shapes(wide_eye::voxel::build_naive_chunk_mesh(target, neighborhood), 5, 0,
                               0, "cross_chunk_face_culled")) {
            return false;
        }
        const auto diagnostics = wide_eye::voxel::describe_naive_chunk_faces(target, neighborhood);
        const auto& boundary_diagnostic = diagnostics[direction];
        if (!check(diagnostics.size() == kDirections.size() &&
                       boundary_diagnostic.source_local == kBoundaryCells[direction] &&
                       boundary_diagnostic.direction == kDirections[direction] &&
                       boundary_diagnostic.neighbor_local == kWrappedCells[direction] &&
                       boundary_diagnostic.neighbor_material == kStone &&
                       boundary_diagnostic.neighbor_kind == FaceNeighborKind::adjacent_chunk &&
                       boundary_diagnostic.disposition == FaceDisposition::culled,
                   "cross_chunk_diagnostic_identifies_wrapped_occluder")) {
            return false;
        }
    }
    return true;
}

bool test_missing_and_empty_neighbors_are_exposed() {
    Chunk boundary;
    if (!check(boundary.set({.x = 15, .y = 7, .z = 8}, kGrass) == SetBlockResult::changed,
               "exposed_boundary_setup")) {
        return false;
    }

    const Chunk empty_neighbor;
    ChunkNeighborhood explicit_empty;
    explicit_empty.adjacent[wide_eye::voxel::face_direction_index(FaceDirection::positive_x)] =
        &empty_neighbor;
    const auto missing_diagnostics = wide_eye::voxel::describe_naive_chunk_faces(boundary, {});
    const auto empty_diagnostics =
        wide_eye::voxel::describe_naive_chunk_faces(boundary, explicit_empty);
    const std::size_t positive_x = wide_eye::voxel::face_direction_index(FaceDirection::positive_x);
    return check_mesh_shapes(wide_eye::voxel::build_naive_chunk_mesh(boundary, {}), 6, 0, 0,
                             "missing_neighbor_samples_empty") &&
           check_mesh_shapes(wide_eye::voxel::build_naive_chunk_mesh(boundary, explicit_empty), 6,
                             0, 0, "empty_neighbor_exposes_face") &&
           check(missing_diagnostics[positive_x].neighbor_kind == FaceNeighborKind::missing_chunk &&
                     missing_diagnostics[positive_x].neighbor_material ==
                         wide_eye::voxel::kEmptyMaterialId &&
                     missing_diagnostics[positive_x].disposition == FaceDisposition::emitted,
                 "missing_neighbor_diagnostic_explains_exposed_face") &&
           check(empty_diagnostics[positive_x].neighbor_kind == FaceNeighborKind::adjacent_chunk &&
                     empty_diagnostics[positive_x].neighbor_material ==
                         wide_eye::voxel::kEmptyMaterialId &&
                     empty_diagnostics[positive_x].disposition == FaceDisposition::emitted,
                 "stored_empty_neighbor_diagnostic_explains_exposed_face");
}

bool test_render_pass_separation_and_cross_pass_occlusion() {
    MaterialPassTable material_passes;
    material_passes.set(kLeaves, MeshRenderPass::cutout);
    material_passes.set(kWater, MeshRenderPass::translucent);
    if (!check(material_passes.get(kGrass) == MeshRenderPass::opaque,
               "default_material_pass_is_opaque") ||
        !check(material_passes.get(kLeaves) == MeshRenderPass::cutout,
               "cutout_material_pass_lookup") ||
        !check(material_passes.get(kWater) == MeshRenderPass::translucent,
               "translucent_material_pass_lookup")) {
        return false;
    }

    Chunk mixed;
    if (!check(mixed.set({.x = 5, .y = 5, .z = 5}, kStone) == SetBlockResult::changed,
               "opaque_pass_setup") ||
        !check(mixed.set({.x = 6, .y = 5, .z = 5}, kLeaves) == SetBlockResult::changed,
               "cutout_pass_setup") ||
        !check(mixed.set({.x = 10, .y = 10, .z = 10}, kWater) == SetBlockResult::changed,
               "translucent_pass_setup")) {
        return false;
    }

    constexpr std::size_t kExpectedFaces = 16;
    constexpr std::size_t kExpectedVertices = kExpectedFaces * 4U;
    constexpr std::size_t kExpectedIndices = kExpectedFaces * 6U;
    const ChunkMeshBuildResult result = wide_eye::voxel::build_naive_chunk_mesh(
        mixed, {}, material_passes,
        {.max_vertices = kExpectedVertices, .max_indices = kExpectedIndices});
    if (!check(result.has_value(), "render_pass_build_succeeds")) {
        return false;
    }
    const ChunkMeshes& meshes = *result;
    if (!check_mesh_shapes(meshes, 5, 5, 6, "render_pass_face_counts")) {
        return false;
    }

    const ChunkMeshBuildResult aggregate_limit_rejected = wide_eye::voxel::build_naive_chunk_mesh(
        mixed, {}, material_passes,
        {.max_vertices = kExpectedVertices - 1U, .max_indices = kExpectedIndices});
    if (!check(!aggregate_limit_rejected.has_value() &&
                   aggregate_limit_rejected.error() == ChunkMeshBuildError::vertex_limit_exceeded,
               "render_pass_aggregate_vertex_limit")) {
        return false;
    }

    for (std::size_t face = 0; face < meshes.opaque.face_count(); ++face) {
        if (!check_face_geometry(meshes.opaque, face, kStone)) {
            return false;
        }
    }
    for (std::size_t face = 0; face < meshes.cutout.face_count(); ++face) {
        if (!check_face_geometry(meshes.cutout, face, kLeaves)) {
            return false;
        }
    }
    for (std::size_t face = 0; face < meshes.translucent.face_count(); ++face) {
        if (!check_face_geometry(meshes.translucent, face, kWater)) {
            return false;
        }
    }
    return true;
}

bool test_count_bounds_and_limit_rejection() {
    constexpr std::size_t kConservativeFaceBound = Chunk::kCellCount * 6U;
    constexpr std::size_t kConservativeVertexBound = kConservativeFaceBound * 4U;
    constexpr std::size_t kConservativeIndexBound = kConservativeFaceBound * 6U;
    static_assert(kConservativeVertexBound <= std::numeric_limits<std::uint32_t>::max());
    if (!check(wide_eye::voxel::kMaxNaiveChunkFaceCount == kConservativeFaceBound &&
                   wide_eye::voxel::kMaxNaiveChunkVertexCount == kConservativeVertexBound &&
                   wide_eye::voxel::kMaxNaiveChunkIndexCount == kConservativeIndexBound,
               "fixed_chunk_count_bounds")) {
        return false;
    }

    Chunk checkerboard;
    std::size_t occupied_cells = 0;
    for (wide_eye::voxel::GridCoordinate z = 0; z < Chunk::kEdgeLength; ++z) {
        for (wide_eye::voxel::GridCoordinate y = 0; y < Chunk::kEdgeLength; ++y) {
            for (wide_eye::voxel::GridCoordinate x = 0; x < Chunk::kEdgeLength; ++x) {
                if ((x + y + z) % 2 == 0) {
                    if (!check(checkerboard.set({.x = x, .y = y, .z = z}, kGrass) ==
                                   SetBlockResult::changed,
                               "checkerboard_setup")) {
                        return false;
                    }
                    ++occupied_cells;
                }
            }
        }
    }

    const std::size_t expected_faces = occupied_cells * 6U;
    const std::size_t expected_vertices = expected_faces * 4U;
    const std::size_t expected_indices = expected_faces * 6U;
    const ChunkMeshBuildLimits exact_limits{
        .max_vertices = expected_vertices,
        .max_indices = expected_indices,
    };
    const ChunkMeshBuildResult exact =
        wide_eye::voxel::build_naive_chunk_mesh(checkerboard, {}, {}, exact_limits);
    if (!check_mesh_shapes(exact, expected_faces, 0, 0, "exact_limits_accept_output")) {
        return false;
    }

    const ChunkMeshBuildResult vertex_rejected = wide_eye::voxel::build_naive_chunk_mesh(
        checkerboard, {}, {},
        {.max_vertices = expected_vertices - 1U, .max_indices = expected_indices});
    if (!check(!vertex_rejected.has_value() &&
                   vertex_rejected.error() == ChunkMeshBuildError::vertex_limit_exceeded,
               "vertex_limit_rejected")) {
        return false;
    }

    const ChunkMeshBuildResult index_rejected = wide_eye::voxel::build_naive_chunk_mesh(
        checkerboard, {}, {},
        {.max_vertices = expected_vertices, .max_indices = expected_indices - 1U});
    if (!check(!index_rejected.has_value() &&
                   index_rejected.error() == ChunkMeshBuildError::index_limit_exceeded,
               "index_limit_rejected")) {
        return false;
    }

    const ChunkMeshBuildResult empty_with_zero_limits = wide_eye::voxel::build_naive_chunk_mesh(
        Chunk{}, {}, {}, {.max_vertices = 0, .max_indices = 0});
    return check_mesh_shapes(empty_with_zero_limits, 0, 0, 0, "zero_limits_accept_empty_chunk");
}

} // namespace

int main() {
    if (!test_empty_and_single_voxel() || !test_internal_face_culling() ||
        !test_full_chunk_surface() || !test_all_cross_chunk_boundaries() ||
        !test_missing_and_empty_neighbors_are_exposed() ||
        !test_render_pass_separation_and_cross_pass_occlusion() ||
        !test_count_bounds_and_limit_rejection()) {
        return EXIT_FAILURE;
    }

    std::cout << "naive_mesher_empty_single=yes\n"
              << "naive_mesher_internal_culling=yes\n"
              << "naive_mesher_full_surface=yes\n"
              << "naive_mesher_cross_chunk=yes\n"
              << "naive_mesher_face_diagnostics=yes\n"
              << "naive_mesher_render_pass_separation=yes\n"
              << "naive_mesher_count_bounds=yes\n"
              << "naive_mesher_limit_rejection=yes\n"
              << "naive_mesher_result=pass\n";
    return EXIT_SUCCESS;
}
