#pragma once

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

} // namespace wide_eye::game
