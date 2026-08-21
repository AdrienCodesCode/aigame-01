#pragma once

#include <algorithm>
#include <cmath>

namespace wide_eye::game {

// Minimal engine-independent math shared by authoritative gameplay systems.
// Keep domain types out of controller-specific headers so camera, flock, and
// spatial-query code do not acquire an artificial dependency on the dog motor.
struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    bool operator==(const Vec3&) const = default;
};

// Rotates `current` toward `target` along the shorter arc by at most
// `maximum_change`, and normalizes the result to `(-pi, pi]`. The dog motor and
// the sheep heading rule share this one implementation: two turn limits that
// disagreed about which way is shorter would be a silent behavior difference
// between the animals rather than a designed one.
[[nodiscard]] inline double approach_angle(double current, double target,
                                           double maximum_change) noexcept {
    constexpr double kTwoPi = 6.28318530717958647692;
    const double shortest_delta = std::remainder(target - current, kTwoPi);
    return std::remainder(current + std::clamp(shortest_delta, -maximum_change, maximum_change),
                          kTwoPi);
}

} // namespace wide_eye::game
