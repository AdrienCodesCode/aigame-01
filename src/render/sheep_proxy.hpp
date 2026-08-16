#pragma once

#include "game/gameplay_simulation.hpp"

#include <array>
#include <cstdint>

namespace wide_eye::render {

struct SheepProxyPose {
    std::uint32_t id = 0;
    std::array<float, 3> ground_position{};
    float heading_radians = 0.0F;

    bool operator==(const SheepProxyPose&) const = default;
};

using SheepProxyPoseBuffer = std::array<SheepProxyPose, game::kGameplaySheepCount>;

// Converts one published gameplay snapshot into renderer-facing values without
// retaining pointers or introducing a second owner for sheep identity/state.
[[nodiscard]] SheepProxyPoseBuffer
make_sheep_proxy_poses(const game::GameplaySnapshot& snapshot) noexcept;

} // namespace wide_eye::render
