#include "game/sheep_spatial_grid.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <span>

namespace {

std::size_t g_allocation_count = 0;

} // namespace

void* operator new(std::size_t size) {
    ++g_allocation_count;
    if (void* allocation = std::malloc(size)) {
        return allocation;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
}

void operator delete(void* allocation) noexcept {
    std::free(allocation);
}

void operator delete(void* allocation, std::size_t) noexcept {
    std::free(allocation);
}

void operator delete[](void* allocation) noexcept {
    std::free(allocation);
}

void operator delete[](void* allocation, std::size_t) noexcept {
    std::free(allocation);
}

namespace {

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "sheep_spatial_grid_failure=" << name << '\n';
    }
    return condition;
}

bool near(double actual, double expected, double tolerance = 1.0e-12) {
    return std::abs(actual - expected) <= tolerance;
}

std::size_t index_with_id(std::span<const wide_eye::game::SheepState> sheep, std::uint32_t id) {
    for (std::size_t index = 0; index < sheep.size(); ++index) {
        if (sheep[index].id == id) {
            return index;
        }
    }
    return sheep.size();
}

} // namespace

int main() {
    using wide_eye::game::SheepSpatialGrid;
    using wide_eye::game::SheepState;
    using wide_eye::game::SpatialGridBuildError;
    using wide_eye::game::SpatialGridQueryError;
    using wide_eye::game::SpatialNeighbor;

    const std::array<SheepState, 7> fixture{{
        {.id = 50, .position = {.x = 0.0, .z = 0.0}},
        {.id = 40, .position = {.x = 0.5, .z = 0.0}},
        {.id = 20, .position = {.x = -0.5, .z = 0.0}},
        {.id = 30, .position = {.x = 1.0, .z = 0.0}},
        {.id = 10, .position = {.x = 1.0001, .z = 0.0}},
        {.id = 60, .position = {.x = 0.9, .z = 0.9}},
        {.id = 70, .position = {.x = 100.0, .z = 100.0}},
    }};

    SheepSpatialGrid grid;
    std::array<SpatialNeighbor, 2> bounded_neighbors{};
    if (!check(grid.query_neighbors(0, 1.0, bounded_neighbors).error ==
                   SpatialGridQueryError::grid_not_built,
               "query_before_build_rejected") ||
        !check(grid.rebuild(fixture, 1.0) == SpatialGridBuildError::none, "fixture_builds") ||
        !check(grid.built() && grid.member_count() == fixture.size() && near(grid.cell_size(), 1.0),
               "build_metadata_published")) {
        return EXIT_FAILURE;
    }

    const auto bounded = grid.query_neighbors(0, 1.0, bounded_neighbors);
    if (!check(bounded.error == SpatialGridQueryError::none, "bounded_query_succeeds") ||
        !check(bounded.neighbor_count == 2 && bounded.within_radius_count == 3 &&
                   bounded.truncated(),
               "caller_span_bounds_nearest_selection") ||
        !check(bounded_neighbors[0].id == 20 && bounded_neighbors[1].id == 40 &&
                   near(bounded_neighbors[0].distance, 0.5) &&
                   near(bounded_neighbors[1].distance, 0.5),
               "distance_ties_use_stable_id_order") ||
        !check(bounded.inspected_candidate_count == 5, "only_occupied_query_cells_are_inspected")) {
        return EXIT_FAILURE;
    }

    std::array<SpatialNeighbor, 7> all_neighbors{};
    const auto all = grid.query_neighbors(0, 1.0, all_neighbors);
    if (!check(all.neighbor_count == 3 && !all.truncated(),
               "exact_radius_returns_all_matching_neighbors") ||
        !check(all_neighbors[0].id == 20 && all_neighbors[1].id == 40 &&
                   all_neighbors[2].id == 30 && near(all_neighbors[2].distance, 1.0),
               "exact_boundary_is_included") ||
        !check(std::none_of(all_neighbors.begin(), all_neighbors.begin() + all.neighbor_count,
                            [](const SpatialNeighbor& neighbor) {
                                return neighbor.id == 10 || neighbor.id == 60 || neighbor.id == 70;
                            }),
               "exact_planar_radius_rejects_cell_box_false_positives")) {
        return EXIT_FAILURE;
    }

    std::array<SheepState, 7> reversed = fixture;
    std::reverse(reversed.begin(), reversed.end());
    SheepSpatialGrid reversed_grid;
    std::array<SpatialNeighbor, 7> reversed_neighbors{};
    const std::size_t reversed_subject = index_with_id(reversed, 50);
    if (!check(reversed_grid.rebuild(reversed, 1.0) == SpatialGridBuildError::none,
               "reversed_fixture_builds")) {
        return EXIT_FAILURE;
    }
    const auto reversed_result =
        reversed_grid.query_neighbors(reversed_subject, 1.0, reversed_neighbors);
    for (std::size_t index = 0; index < all.neighbor_count; ++index) {
        if (!check(reversed_neighbors[index].id == all_neighbors[index].id &&
                       near(reversed_neighbors[index].distance, all_neighbors[index].distance),
                   "storage_order_does_not_change_id_distance_order")) {
            return EXIT_FAILURE;
        }
    }
    if (!check(reversed_result.neighbor_count == all.neighbor_count,
               "reversed_query_has_same_neighbor_count")) {
        return EXIT_FAILURE;
    }

    const std::array<SheepState, 3> copied_source{{
        {.id = 1, .position = {.x = -1.0, .z = -1.0}},
        {.id = 2, .position = {.x = -2.0, .z = -1.0}},
        {.id = 3, .position = {.x = 20.0, .z = 20.0}},
    }};
    std::array<SheepState, 3> mutable_source = copied_source;
    SheepSpatialGrid copied_grid;
    if (!check(copied_grid.rebuild(mutable_source, 1.0) == SpatialGridBuildError::none,
               "negative_boundary_fixture_builds")) {
        return EXIT_FAILURE;
    }
    mutable_source[1].position = {.x = 100.0, .z = 100.0};
    std::array<SpatialNeighbor, 2> copied_neighbors{};
    const auto copied_result = copied_grid.query_neighbors(0, 1.0, copied_neighbors);
    if (!check(copied_result.neighbor_count == 1 && copied_neighbors[0].id == 2 &&
                   near(copied_neighbors[0].distance, 1.0),
               "grid_is_immutable_copy_of_build_snapshot")) {
        return EXIT_FAILURE;
    }

    SheepSpatialGrid invalid_grid;
    std::array<SheepState, 2> invalid_members{{
        {.id = 1, .position = {}},
        {.id = 1, .position = {.x = 1.0}},
    }};
    if (!check(invalid_grid.rebuild(fixture, 1.0) == SpatialGridBuildError::none,
               "valid_state_precedes_failed_rebuild") ||
        !check(invalid_grid.rebuild(invalid_members, 1.0) == SpatialGridBuildError::duplicate_id,
               "duplicate_ids_rejected") ||
        !check(!invalid_grid.built(), "failed_build_invalidates_prior_state") ||
        !check(invalid_grid.rebuild(fixture, 0.0) == SpatialGridBuildError::invalid_cell_size,
               "zero_cell_size_rejected") ||
        !check(invalid_grid.rebuild(fixture, std::numeric_limits<double>::quiet_NaN()) ==
                   SpatialGridBuildError::invalid_cell_size,
               "non_finite_cell_size_rejected")) {
        return EXIT_FAILURE;
    }
    invalid_members[1].id = 2;
    invalid_members[1].position.x = std::numeric_limits<double>::infinity();
    if (!check(invalid_grid.rebuild(invalid_members, 1.0) == SpatialGridBuildError::invalid_member,
               "non_finite_position_rejected")) {
        return EXIT_FAILURE;
    }

    if (!check(grid.query_neighbors(fixture.size(), 1.0, all_neighbors).error ==
                   SpatialGridQueryError::invalid_subject,
               "out_of_range_subject_rejected") ||
        !check(grid.query_neighbors(0, -1.0, all_neighbors).error ==
                   SpatialGridQueryError::invalid_radius,
               "negative_radius_rejected") ||
        !check(
            grid.query_neighbors(0, std::numeric_limits<double>::infinity(), all_neighbors).error ==
                SpatialGridQueryError::invalid_radius,
            "non_finite_radius_rejected")) {
        return EXIT_FAILURE;
    }

    std::array<SheepState, SheepSpatialGrid::kMaximumMemberCount + 1> over_capacity{};
    if (!check(invalid_grid.rebuild(over_capacity, 1.0) == SpatialGridBuildError::too_many_members,
               "capacity_overflow_rejected_before_member_access")) {
        return EXIT_FAILURE;
    }

    std::array<SheepState, SheepSpatialGrid::kMaximumMemberCount> capacity_fixture{};
    for (std::size_t index = 0; index < capacity_fixture.size(); ++index) {
        capacity_fixture[index].id = static_cast<std::uint32_t>(index + 1);
        capacity_fixture[index].position.x = static_cast<double>(index * 2);
    }
    const std::size_t capacity_allocations_before = g_allocation_count;
    if (!check(invalid_grid.rebuild(capacity_fixture, 1.0) == SpatialGridBuildError::none,
               "capacity_experiment_ceiling_builds") ||
        !check(invalid_grid.member_count() == SheepSpatialGrid::kMaximumMemberCount,
               "capacity_experiment_ceiling_is_published") ||
        !check(invalid_grid.query_neighbors(500, 1.0, bounded_neighbors).neighbor_count == 0,
               "capacity_fixture_query_remains_spatially_bounded")) {
        return EXIT_FAILURE;
    }
    const std::size_t capacity_allocations = g_allocation_count - capacity_allocations_before;
    if (!check(capacity_allocations == 0, "capacity_rebuild_and_query_do_not_allocate")) {
        return EXIT_FAILURE;
    }

    const std::size_t allocations_before = g_allocation_count;
    for (std::size_t iteration = 0; iteration < 32; ++iteration) {
        if (!check(grid.rebuild(fixture, 1.0) == SpatialGridBuildError::none,
                   "steady_state_rebuild_succeeds") ||
            !check(grid.query_neighbors(0, 1.0, bounded_neighbors).error ==
                       SpatialGridQueryError::none,
                   "steady_state_query_succeeds")) {
            return EXIT_FAILURE;
        }
    }
    const std::size_t steady_state_allocations = g_allocation_count - allocations_before;
    if (!check(steady_state_allocations == 0, "rebuild_and_query_do_not_allocate")) {
        return EXIT_FAILURE;
    }

    std::cout << "sheep_spatial_grid_result=pass\n"
              << "steady_state_allocations=" << steady_state_allocations << '\n'
              << "capacity_allocations=" << capacity_allocations << '\n'
              << "bounded_neighbor_count=" << bounded.neighbor_count << '\n'
              << "within_radius_count=" << bounded.within_radius_count << '\n'
              << "inspected_candidate_count=" << bounded.inspected_candidate_count << '\n';
    return EXIT_SUCCESS;
}
