#pragma once

#include "game/math.hpp"
#include "game/paddock_collision.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace wide_eye::game {

enum class SheepBehaviorState : std::uint8_t {
    settled,
    alert,
    driven,
    recovering,
};

[[nodiscard]] constexpr bool is_known_sheep_behavior(SheepBehaviorState behavior) noexcept {
    switch (behavior) {
    case SheepBehaviorState::settled:
    case SheepBehaviorState::alert:
    case SheepBehaviorState::driven:
    case SheepBehaviorState::recovering:
        return true;
    }
    return false;
}

struct SheepState {
    std::uint32_t id = 0;
    Vec3 position{};
    Vec3 velocity{};
    double heading_radians = 0.0;
    double arousal = 0.0;
    SheepBehaviorState behavior = SheepBehaviorState::settled;
    bool grounded = false;

    bool operator==(const SheepState&) const = default;
};

inline constexpr std::size_t kGameplaySheepCount = 5;
using SheepStateBuffer = std::array<SheepState, kGameplaySheepCount>;

inline constexpr std::size_t kMaximumSelectedAttractionNeighbors = 2;
inline constexpr std::size_t kMaximumSelectedAlignmentNeighbors = 1;

// Read-only causal evidence published with each authoritative sheep snapshot.
// IDs, rather than buffer indexes, keep the evidence meaningful when storage
// order changes. Candidate count records the density seen inside the attraction
// radius even when the selected set is truncated by its fixed bound.
struct SheepSocialEvidence {
    std::uint32_t subject_id = 0;
    std::array<std::uint32_t, kMaximumSelectedAttractionNeighbors> attraction_neighbor_ids{};
    std::uint32_t attraction_neighbor_count = 0;
    std::uint32_t attraction_candidate_count = 0;
    std::array<std::uint32_t, kMaximumSelectedAlignmentNeighbors> alignment_neighbor_ids{};
    std::uint32_t alignment_neighbor_count = 0;
    std::uint32_t alignment_candidate_count = 0;
    Vec3 separation_acceleration{};
    Vec3 attraction_acceleration{};
    Vec3 alignment_acceleration{};

    bool operator==(const SheepSocialEvidence&) const = default;
};

using SheepSocialEvidenceBuffer = std::array<SheepSocialEvidence, kGameplaySheepCount>;

// Dog stimulus remains separate from social evidence so paired controls can
// publish identical geometry while independently switching the applied term.
// Distance, bearing, approach speed, facing alignment, and line-of-sight
// blocking describe the immutable prior state that caused the next authoritative
// sheep state. Bearing is signed relative to sheep heading. Approach speed is
// the prior-state component of dog velocity along the dog-to-sheep direction:
// positive when the dog closes, negative when it leaves. Facing alignment is the
// cosine between the prior dog forward direction and the dog-to-sheep direction:
// `1` when the dog looks straight at the sheep, `0` abeam, `-1` when it looks
// directly away. Line of sight names the analytic paddock obstacle that hides
// the dog, so an occlusion result can be attributed to a wall or a closed gate
// rather than to an anonymous flag. Each applied term keeps its own acceleration
// vector so one switch can be isolated without hiding it inside a combined
// total; an occluded dog releases all three vectors rather than adding a fourth.
struct SheepDogPressureEvidence {
    std::uint32_t subject_id = 0;
    bool stimulus_evaluated = false;
    double dog_distance = 0.0;
    double dog_relative_bearing_radians = 0.0;
    double dog_approach_speed = 0.0;
    double dog_facing_alignment = 0.0;
    bool dog_line_of_sight_blocked = false;
    PaddockObstacle dog_line_of_sight_occluder = PaddockObstacle::none;
    Vec3 pressure_acceleration{};
    Vec3 approach_acceleration{};
    Vec3 facing_acceleration{};

    bool operator==(const SheepDogPressureEvidence&) const = default;
};

using SheepDogPressureEvidenceBuffer = std::array<SheepDogPressureEvidence, kGameplaySheepCount>;

inline constexpr SheepStateBuffer kDefaultGameplaySheepStates{{
    {.id = 1,
     .position = {.x = 14.5, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 16.0, .y = 1.0, .z = 19.5},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 17.5, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 15.25, .y = 1.0, .z = 21.5},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 16.75, .y = 1.0, .z = 21.5},
     .heading_radians = 0.0,
     .grounded = true},
}};

} // namespace wide_eye::game
