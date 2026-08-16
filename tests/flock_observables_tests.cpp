#include "game/flock_observables.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "flock_observables_failure=" << name << '\n';
    }
    return condition;
}

bool near(double actual, double expected, double tolerance = 1.0e-12) {
    return std::abs(actual - expected) <= tolerance;
}

} // namespace

int main() {
    using wide_eye::game::SheepState;
    using wide_eye::game::SheepStateBuffer;

    const SheepStateBuffer cross{{
        {.id = 1, .position = {.x = 0.0, .y = 2.0, .z = 0.0}, .velocity = {.x = 2.0}},
        {.id = 2, .position = {.x = 1.0, .y = 2.0, .z = 0.0}, .velocity = {.x = 4.0}},
        {.id = 3, .position = {.x = -1.0, .y = 2.0, .z = 0.0}, .velocity = {.x = 1.0}},
        {.id = 4, .position = {.x = 0.0, .y = 2.0, .z = 1.0}, .velocity = {.x = 3.0}},
        {.id = 5, .position = {.x = 0.0, .y = 2.0, .z = -1.0}, .velocity = {.x = 5.0}},
    }};
    constexpr std::array<std::uint32_t, 5> chosen{{0, 1, 2, 3, 4}};
    const auto cross_metrics = wide_eye::game::compute_five_sheep_observables(cross, chosen, 1.0);
    if (!check(cross_metrics.has_value(), "cross_fixture_is_valid") ||
        !check(cross_metrics->centroid == wide_eye::game::Vec3{.y = 2.0},
               "centroid_is_arithmetic_mean") ||
        !check(near(cross_metrics->mean_radius, 0.8), "mean_planar_radius") ||
        !check(near(cross_metrics->polarization, 1.0), "polarization_uses_unit_headings") ||
        !check(near(cross_metrics->elongation, 0.0), "symmetric_cross_is_not_elongated") ||
        !check(near(cross_metrics->group_speed, 3.0), "group_speed_is_mean_member_speed") ||
        !check(near(cross_metrics->mean_nearest_neighbor_spacing, 1.0), "nearest_neighbor_mean") ||
        !check(cross_metrics->connected_component_count == 1,
               "threshold_edges_form_one_transitive_component") ||
        !check(cross_metrics->chosen_neighbors.total == 10 &&
                   cross_metrics->chosen_neighbors.minimum == 0 &&
                   cross_metrics->chosen_neighbors.maximum == 4 &&
                   near(cross_metrics->chosen_neighbors.mean, 2.0),
               "chosen_neighbor_count_summary")) {
        return EXIT_FAILURE;
    }
    for (const double spacing : cross_metrics->nearest_neighbor_spacing) {
        if (!check(near(spacing, 1.0), "per_sheep_nearest_spacing")) {
            return EXIT_FAILURE;
        }
    }

    const SheepStateBuffer line{{
        {.id = 10, .position = {.x = 0.0}, .velocity = {.x = 1.0}},
        {.id = 11, .position = {.x = 1.0}, .velocity = {.x = -1.0}},
        {.id = 12, .position = {.x = 2.0}},
        {.id = 13, .position = {.x = 10.0}, .velocity = {.z = 1.0}},
        {.id = 14, .position = {.x = 11.0}, .velocity = {.z = -1.0}},
    }};
    const auto line_metrics =
        wide_eye::game::compute_five_sheep_observables(line, std::array<std::uint32_t, 5>{}, 1.0);
    if (!check(line_metrics.has_value(), "line_fixture_is_valid") ||
        !check(near(line_metrics->centroid.x, 4.8), "offset_centroid") ||
        !check(near(line_metrics->mean_radius, 4.56), "offset_mean_radius") ||
        !check(near(line_metrics->elongation, 1.0), "collinear_group_is_maximally_elongated") ||
        !check(near(line_metrics->polarization, 0.0), "opposed_headings_have_zero_polarization") ||
        !check(near(line_metrics->group_speed, 0.8), "stationary_member_contributes_zero_speed") ||
        !check(line_metrics->connected_component_count == 2,
               "separated_chains_form_two_components")) {
        return EXIT_FAILURE;
    }

    SheepStateBuffer invalid = cross;
    invalid[1].id = invalid[0].id;
    if (!check(!wide_eye::game::compute_five_sheep_observables(invalid, chosen, 1.0),
               "duplicate_id_rejected") ||
        !check(!wide_eye::game::compute_five_sheep_observables(
                   cross, chosen, std::numeric_limits<double>::quiet_NaN()),
               "non_finite_threshold_rejected") ||
        !check(!wide_eye::game::compute_five_sheep_observables(
                   cross, std::array<std::uint32_t, 5>{0, 0, 0, 0, 5}, 1.0),
               "out_of_range_neighbor_count_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].velocity.x = std::numeric_limits<double>::infinity();
    if (!check(!wide_eye::game::compute_five_sheep_observables(invalid, chosen, 1.0),
               "non_finite_state_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].heading_radians = std::numeric_limits<double>::quiet_NaN();
    if (!check(!wide_eye::game::compute_five_sheep_observables(invalid, chosen, 1.0),
               "non_finite_heading_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].arousal = std::numeric_limits<double>::infinity();
    if (!check(!wide_eye::game::compute_five_sheep_observables(invalid, chosen, 1.0),
               "non_finite_arousal_rejected")) {
        return EXIT_FAILURE;
    }
    invalid = cross;
    invalid[0].behavior = static_cast<wide_eye::game::SheepBehaviorState>(255);
    if (!check(!wide_eye::game::compute_five_sheep_observables(invalid, chosen, 1.0),
               "unknown_behavior_rejected")) {
        return EXIT_FAILURE;
    }

    std::cout << "flock_observables_result=pass\n";
    return EXIT_SUCCESS;
}
