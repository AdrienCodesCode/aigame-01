#include "game/gameplay_simulation.hpp"
#include "render/sheep_proxy.hpp"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
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

// Heap, not stack: `GameplaySimulation` is 362 KiB and a published snapshot is
// 116 KiB at the authoritative capacity, and this `main` holds several of each.
// Holding them by value is the shape QA-002 recorded.
[[nodiscard]] std::unique_ptr<wide_eye::game::GameplaySnapshot>
interpolated_on_heap(const wide_eye::game::GameplaySimulation& simulation, double alpha) {
    return std::make_unique<wide_eye::game::GameplaySnapshot>(
        simulation.interpolated_snapshot(alpha));
}

bool check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "sheep_proxy_failure=" << name << '\n';
    }
    return condition;
}

} // namespace

int main() {
    const auto scenario = wide_eye::game::find_gameplay_scenario("paddock-start");
    if (!check(scenario.has_value(), "scenario_available")) {
        return EXIT_FAILURE;
    }

    const auto simulation = std::make_unique<wide_eye::game::GameplaySimulation>(*scenario);
    const auto snapshot_holder = interpolated_on_heap(*simulation, 0.5);
    const wide_eye::game::GameplaySnapshot& snapshot = *snapshot_holder;
    const wide_eye::render::SheepProxyPoseBuffer poses =
        wide_eye::render::make_sheep_proxy_poses(snapshot);

    // The pose buffer is sized at the authoritative capacity; the published
    // active count is what maps one to one.
    bool one_to_one = poses.size() == snapshot.sheep.size() && snapshot.sheep_count == 5;
    for (std::size_t index = 0; index < snapshot.sheep_count; ++index) {
        const wide_eye::render::SheepProxyPose& pose = poses[index];
        const wide_eye::game::SheepState& sheep = snapshot.sheep[index];
        one_to_one = one_to_one && pose.id == sheep.id &&
                     pose.ground_position[0] == static_cast<float>(sheep.position.x) &&
                     pose.ground_position[1] == static_cast<float>(sheep.position.y) &&
                     pose.ground_position[2] == static_cast<float>(sheep.position.z) &&
                     pose.heading_radians == static_cast<float>(sheep.heading_radians);
    }

    if (!check(one_to_one, "published_snapshot_maps_one_to_one") ||
        !check(poses.front().id == 1 && poses[snapshot.sheep_count - 1].id == 5,
               "stable_ids_preserved")) {
        return EXIT_FAILURE;
    }

    const auto motion_scenario = wide_eye::game::find_gameplay_scenario("presentation-motion");
    if (!check(motion_scenario.has_value() &&
                   motion_scenario->sheep_fixture ==
                       wide_eye::game::SheepFixture::scripted_presentation_motion,
               "presentation_motion_fixture_available")) {
        return EXIT_FAILURE;
    }
    const auto motion =
        std::make_unique<wide_eye::game::GameplaySimulation>(*motion_scenario);
    for (std::uint64_t tick = 0; tick < 61; ++tick) {
        motion->fixed_update({});
    }
    const auto motion_snapshot_holder = interpolated_on_heap(*motion, 0.5);
    const wide_eye::game::GameplaySnapshot& motion_snapshot = *motion_snapshot_holder;
    const auto motion_poses = wide_eye::render::make_sheep_proxy_poses(motion_snapshot);
    bool motion_maps_one_to_one = motion_snapshot.sheep_count == snapshot.sheep_count;
    for (std::size_t index = 0; index < motion_snapshot.sheep_count; ++index) {
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

    // The snapshot storage is acquired before the counter is read, so the loop
    // measures interpolation and pose preparation rather than one allocation of
    // their output.
    const auto prepared_snapshot = std::make_unique<wide_eye::game::GameplaySnapshot>();
    const std::size_t allocations_before_preparation = g_sheep_proxy_allocation_count;
    for (int frame = 0; frame < 600; ++frame) {
        *prepared_snapshot = motion->interpolated_snapshot(0.5);
        const auto prepared_poses =
            wide_eye::render::make_sheep_proxy_poses(*prepared_snapshot);
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

    std::cout << "sheep_proxy_count=" << snapshot.sheep_count << '\n'
              << "sheep_proxy_capacity=" << poses.size() << '\n'
              << "sheep_proxy_mapping=published_snapshot_one_to_one\n"
              << "presentation_motion_fixture=scripted_non_behavior\n"
              << "snapshot_presentation_preparation_frames=600\n"
              << "snapshot_presentation_preparation_allocations="
              << presentation_preparation_allocations << '\n'
              << "sheep_proxy_result=pass\n";
    return EXIT_SUCCESS;
}
