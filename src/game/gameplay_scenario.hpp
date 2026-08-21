#pragma once

#include "game/dog_controller.hpp"
#include "game/sheep_state.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace wide_eye::game {

enum class GameplayScenarioId : std::uint8_t {
    paddock_start,
    wall_contact,
    closed_gate,
    open_gate,
    presentation_motion,
    sheep_only_separation,
    sheep_only_attraction,
    sheep_alignment_off,
    sheep_alignment_on,
    sheep_dog_pressure_off,
    sheep_dog_pressure_on,
    sheep_dog_approach_off,
    sheep_dog_approach_on,
    sheep_dog_facing_off,
    sheep_dog_facing_on,
    sheep_dog_line_of_sight_off,
    sheep_dog_line_of_sight_on,
    sheep_paddock_collision_closed_gate,
    sheep_paddock_collision_open_gate,
};

enum class SheepFixture : std::uint8_t {
    stationary,
    scripted_presentation_motion,
    local_social_response,
};

struct SheepSeparationConfiguration {
    bool enabled = false;
    double radius = 1.0;
    double maximum_acceleration = 4.0;

    bool operator==(const SheepSeparationConfiguration&) const = default;
};

struct SheepAttractionConfiguration {
    bool enabled = false;
    double radius = 4.0;
    double maximum_acceleration = 1.5;
    std::uint32_t neighbor_limit = 2;

    bool operator==(const SheepAttractionConfiguration&) const = default;
};

struct SheepAlignmentConfiguration {
    bool enabled = false;
    double radius = 3.0;
    double response_time_seconds = 1.0;
    double maximum_acceleration = 1.5;
    std::uint32_t neighbor_limit = 1;

    bool operator==(const SheepAlignmentConfiguration&) const = default;
};

struct SheepDogPressureConfiguration {
    bool enabled = false;
    double radius = 6.0;
    double maximum_acceleration = 3.0;

    bool operator==(const SheepDogPressureConfiguration&) const = default;
};

// Approach velocity is a separate dog-stimulus variable layered on the accepted
// distance-only pressure geometry. It shares the pressure radius and linear
// distance falloff so the radius boundary stays continuous, and it responds only
// to a closing dog. `reference_speed` is the closing speed at which the response
// saturates; faster approaches do not exceed `maximum_acceleration`.
struct SheepDogApproachConfiguration {
    bool enabled = false;
    double reference_speed = 3.0;
    double maximum_acceleration = 2.0;

    bool operator==(const SheepDogApproachConfiguration&) const = default;
};

// Facing is a third separate dog-stimulus variable layered on the same accepted
// distance-only pressure geometry. It shares the pressure radius and linear
// distance falloff so the radius boundary stays continuous, and it responds only
// to a dog whose prior forward direction points toward the sheep. The response
// is the positive part of the published facing alignment, so a dog looking away
// releases rather than pulling.
struct SheepDogFacingConfiguration {
    bool enabled = false;
    double maximum_acceleration = 1.5;

    bool operator==(const SheepDogFacingConfiguration&) const = default;
};

// Line of sight is a fourth separate dog-stimulus variable. It adds no vector of
// its own: an occluded dog releases the pressure, approach, and facing terms, so
// the published per-term vectors stay the applied vectors. Visibility is binary
// against the same analytic paddock obstacles the dog collides with, because a
// soft falloff would need a shape this project has not observed yet.
struct SheepDogLineOfSightConfiguration {
    bool enabled = false;

    bool operator==(const SheepDogLineOfSightConfiguration&) const = default;
};

// Owns the complete deterministic starting contract for one game scenario.
// Controller-specific configuration stays nested under its subsystem, while
// sheep, objective, and future fixture state remain owned at the game level.
struct GameplayScenarioDefinition {
    GameplayScenarioId id = GameplayScenarioId::paddock_start;
    std::uint32_t version = 1;
    std::uint64_t seed = 0;
    DogControllerConfiguration dog{};
    // Paddock gate state is world state: the dog motor collides with it and the
    // sheep line-of-sight query needs it, so neither owns it.
    bool gate_open = false;
    SheepFixture sheep_fixture = SheepFixture::stationary;
    SheepStateBuffer initial_sheep = kDefaultGameplaySheepStates;
    SheepSeparationConfiguration sheep_separation{};
    SheepAttractionConfiguration sheep_attraction{};
    SheepAlignmentConfiguration sheep_alignment{};
    SheepDogPressureConfiguration sheep_dog_pressure{};
    SheepDogApproachConfiguration sheep_dog_approach{};
    SheepDogFacingConfiguration sheep_dog_facing{};
    SheepDogLineOfSightConfiguration sheep_dog_line_of_sight{};

    bool operator==(const GameplayScenarioDefinition&) const = default;
};

[[nodiscard]] std::optional<GameplayScenarioDefinition>
find_gameplay_scenario(std::string_view name) noexcept;
[[nodiscard]] std::string_view gameplay_scenario_name(GameplayScenarioId scenario) noexcept;

} // namespace wide_eye::game
