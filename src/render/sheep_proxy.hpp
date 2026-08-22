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

// Sized at the authoritative capacity. Only the snapshot's published
// `sheep_count` entries are written; the rest stay default and are not drawn.
using SheepProxyPoseBuffer = std::array<SheepProxyPose, game::kMaximumGameplaySheepCount>;

// Converts one published gameplay snapshot into renderer-facing values without
// retaining pointers or introducing a second owner for sheep identity/state.
// The caller reads how many entries are meaningful from the same snapshot's
// `sheep_count`, which is also what the draw path submits.
[[nodiscard]] SheepProxyPoseBuffer
make_sheep_proxy_poses(const game::GameplaySnapshot& snapshot) noexcept;

} // namespace wide_eye::render
