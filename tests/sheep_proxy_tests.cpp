#include "game/gameplay_simulation.hpp"
#include "render/sheep_proxy.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>

std::size_t g_sheep_proxy_allocation_count = 0;

void* operator new(std::size_t size) {
    ++g_sheep_proxy_allocation_count;
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
        std::cerr << "sheep_proxy_failure=" << name << '\n';
    }
    return condition;
}

} // namespace

int main() {
    const auto scenario = wide_eye::game::find_dog_scenario("paddock-start");
    if (!check(scenario.has_value(), "scenario_available")) {
        return EXIT_FAILURE;
    }

    wide_eye::game::GameplaySimulation simulation{*scenario};
    const wide_eye::game::GameplaySnapshot snapshot = simulation.interpolated_snapshot(0.5);
    const wide_eye::render::SheepProxyPoseBuffer poses =
        wide_eye::render::make_sheep_proxy_poses(snapshot);

    bool one_to_one = poses.size() == snapshot.sheep.size();
    for (std::size_t index = 0; index < poses.size(); ++index) {
        const wide_eye::render::SheepProxyPose& pose = poses[index];
        const wide_eye::game::SheepState& sheep = snapshot.sheep[index];
        one_to_one = one_to_one && pose.id == sheep.id &&
                     pose.ground_position[0] == static_cast<float>(sheep.position.x) &&
                     pose.ground_position[1] == static_cast<float>(sheep.position.y) &&
                     pose.ground_position[2] == static_cast<float>(sheep.position.z) &&
                     pose.heading_radians == static_cast<float>(sheep.heading_radians);
    }

    if (!check(one_to_one, "published_snapshot_maps_one_to_one") ||
        !check(poses.front().id == 1 && poses.back().id == 5, "stable_ids_preserved")) {
        return EXIT_FAILURE;
    }

    const auto motion_scenario = wide_eye::game::find_dog_scenario("presentation-motion");
    if (!check(motion_scenario.has_value() && motion_scenario->presentation_only_sheep_motion,
               "presentation_motion_fixture_available")) {
        return EXIT_FAILURE;
    }
    wide_eye::game::GameplaySimulation motion{*motion_scenario};
    for (std::uint64_t tick = 0; tick < 61; ++tick) {
        motion.fixed_update({});
    }
    const auto motion_snapshot = motion.interpolated_snapshot(0.5);
    const auto motion_poses = wide_eye::render::make_sheep_proxy_poses(motion_snapshot);
    bool motion_maps_one_to_one = true;
    for (std::size_t index = 0; index < motion_poses.size(); ++index) {
        motion_maps_one_to_one =
            motion_maps_one_to_one && motion_poses[index].id == motion_snapshot.sheep[index].id &&
            motion_poses[index].ground_position[0] ==
                static_cast<float>(motion_snapshot.sheep[index].position.x) &&
            motion_poses[index].ground_position[2] ==
                static_cast<float>(motion_snapshot.sheep[index].position.z) &&
            motion_poses[index].heading_radians ==
                static_cast<float>(motion_snapshot.sheep[index].heading_radians);
    }
    if (!check(motion_maps_one_to_one, "motion_snapshot_maps_one_to_one") ||
        !check(motion_poses.front().ground_position[2] < poses.front().ground_position[2],
               "motion_fixture_translates_proxy") ||
        !check(std::abs(motion_poses.front().heading_radians -
                        static_cast<float>(0.25 * 3.14159265358979323846)) < 1.0e-6F,
               "motion_fixture_interpolates_facing")) {
        return EXIT_FAILURE;
    }

    const std::size_t allocations_before_preparation = g_sheep_proxy_allocation_count;
    for (int frame = 0; frame < 600; ++frame) {
        const auto prepared_snapshot = motion.interpolated_snapshot(0.5);
        const auto prepared_poses = wide_eye::render::make_sheep_proxy_poses(prepared_snapshot);
        if (prepared_poses.front().id != 1) {
            return EXIT_FAILURE;
        }
    }
    const std::size_t presentation_preparation_allocations =
        g_sheep_proxy_allocation_count - allocations_before_preparation;
    if (!check(presentation_preparation_allocations == 0,
               "snapshot_presentation_preparation_does_not_allocate")) {
        return EXIT_FAILURE;
    }

    std::cout << "sheep_proxy_count=" << poses.size() << '\n'
              << "sheep_proxy_mapping=published_snapshot_one_to_one\n"
              << "presentation_motion_fixture=scripted_non_behavior\n"
              << "snapshot_presentation_preparation_frames=600\n"
              << "snapshot_presentation_preparation_allocations="
              << presentation_preparation_allocations << '\n'
              << "sheep_proxy_result=pass\n";
    return EXIT_SUCCESS;
}
