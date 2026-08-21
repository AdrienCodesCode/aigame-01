#pragma once

#include "game/sheep_state.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace wide_eye::game {

struct ChosenNeighborCountSummary {
    std::uint32_t total = 0;
    std::uint32_t minimum = 0;
    std::uint32_t maximum = 0;
    double mean = 0.0;

    bool operator==(const ChosenNeighborCountSummary&) const = default;
};

// Ground-plane observables for the fixed five-sheep Tracer 2 snapshot. The
// connectivity distance and chosen-neighbor counts are explicit inputs so a
// caller can supply counts from published social evidence without running a
// second neighbor-selection path. This function only reads published state; it
// neither chooses neighbors nor mutates simulation state.
struct FiveSheepObservables {
    Vec3 centroid{};
    double mean_radius = 0.0;
    double polarization = 0.0;
    // Bounded planar covariance anisotropy: 0 is isotropic and 1 is collinear.
    double elongation = 0.0;
    double group_speed = 0.0;
    std::array<double, kGameplaySheepCount> nearest_neighbor_spacing{};
    double mean_nearest_neighbor_spacing = 0.0;
    std::uint32_t connected_component_count = 0;
    ChosenNeighborCountSummary chosen_neighbors{};

    bool operator==(const FiveSheepObservables&) const = default;
};

[[nodiscard]] std::optional<FiveSheepObservables> compute_five_sheep_observables(
    const SheepStateBuffer& sheep,
    const std::array<std::uint32_t, kGameplaySheepCount>& chosen_neighbor_counts,
    double connectivity_distance) noexcept;

} // namespace wide_eye::game
