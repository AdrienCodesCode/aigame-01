#include "render/sheep_proxy.hpp"

#include <cstddef>

namespace wide_eye::render {

SheepProxyPoseBuffer make_sheep_proxy_poses(const game::GameplaySnapshot& snapshot) noexcept {
    SheepProxyPoseBuffer poses{};
    for (std::size_t index = 0; index < poses.size(); ++index) {
        const game::SheepState& sheep = snapshot.sheep[index];
        poses[index] = {
            .id = sheep.id,
            .ground_position = {static_cast<float>(sheep.position.x),
                                static_cast<float>(sheep.position.y),
                                static_cast<float>(sheep.position.z)},
            .heading_radians = static_cast<float>(sheep.heading_radians),
        };
    }
    return poses;
}

} // namespace wide_eye::render
