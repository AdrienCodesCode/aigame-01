#include "game/gameplay_scenario.hpp"

#include <array>

namespace wide_eye::game {
namespace {

struct NamedGameplayScenario {
    std::string_view name;
    GameplayScenarioDefinition definition;
};

constexpr SheepStateBuffer kSeparationSheepStates{{
    {.id = 1,
     .position = {.x = 15.25, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.25, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 18.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 20.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 22.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr SheepStateBuffer kAttractionSheepStates{{
    {.id = 1,
     .position = {.x = 15.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.5, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 15.0, .y = 1.0, .z = 20.5},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 14.25, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 15.0, .y = 1.0, .z = 19.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr SheepStateBuffer kAlignmentSheepStates{{
    {.id = 1,
     .position = {.x = 15.0, .y = 1.0, .z = 20.0},
     .velocity = {.x = 1.0},
     .heading_radians = 1.57079632679489661923,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.5, .y = 1.0, .z = 20.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 18.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 20.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 22.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -1.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr SheepStateBuffer kDogPressureSheepStates{{
    {.id = 1,
     .position = {.x = 14.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 18.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 19.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 12.0, .y = 1.0, .z = 24.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

// One moving dog at (12, 20) travelling +x at 4.0 world units/s isolates approach
// velocity: sheep 1 closes head-on above the saturation speed, sheep 2 sits
// exactly abeam, sheep 3 is behind the dog and opening, sheep 4 closes on an
// exact 3-4-5 diagonal, and sheep 5 closes from outside the pressure radius.
constexpr SheepStateBuffer kDogApproachSheepStates{{
    {.id = 1,
     .position = {.x = 14.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 12.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 9.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 15.0, .y = 1.0, .z = 24.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 20.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kApproachDogState{.position = {.x = 12.0, .y = 1.0, .z = 20.0},
                                     .velocity = {.x = 4.0},
                                     .heading_radians = 1.57079632679489661923,
                                     .grounded = true};

// One stationary dog at (12, 20) isolates facing from approach velocity: heading
// zero is exactly the -z forward direction, so sheep 1 is straight ahead, sheep 2
// is exactly abeam, sheep 3 is directly behind, sheep 4 sits on an exact 3-4-5
// diagonal ahead of the dog, and sheep 5 is straight ahead but outside the
// pressure radius.
constexpr SheepStateBuffer kDogFacingSheepStates{{
    {.id = 1,
     .position = {.x = 12.0, .y = 1.0, .z = 18.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 15.0, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 12.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 15.0, .y = 1.0, .z = 16.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 12.0, .y = 1.0, .z = 12.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kFacingDogState{
    .position = {.x = 12.0, .y = 1.0, .z = 20.0}, .heading_radians = 0.0, .grounded = true};

constexpr std::array<NamedGameplayScenario, 15> kGameplayScenarios{{
    {
        .name = "paddock-start",
        .definition = {.id = GameplayScenarioId::paddock_start,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}}},
    },
    {
        .name = "presentation-motion",
        .definition = {.id = GameplayScenarioId::presentation_motion,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::scripted_presentation_motion},
    },
    {
        .name = "wall-contact",
        .definition = {.id = GameplayScenarioId::wall_contact,
                       .dog = {.initial_state = {.position = {.x = 8.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}}},
    },
    {
        .name = "closed-gate",
        .definition = {.id = GameplayScenarioId::closed_gate,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}}},
    },
    {
        .name = "open-gate",
        .definition = {.id = GameplayScenarioId::open_gate,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true},
                               .gate_open = true}},
    },
    {
        .name = "sheep-only-separation",
        .definition = {.id = GameplayScenarioId::sheep_only_separation,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kSeparationSheepStates,
                       .sheep_separation = {.enabled = true}},
    },
    {
        .name = "sheep-only-attraction",
        .definition = {.id = GameplayScenarioId::sheep_only_attraction,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAttractionSheepStates,
                       .sheep_attraction = {.enabled = true}},
    },
    {
        .name = "sheep-alignment-off",
        .definition = {.id = GameplayScenarioId::sheep_alignment_off,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAlignmentSheepStates},
    },
    {
        .name = "sheep-alignment-on",
        .definition = {.id = GameplayScenarioId::sheep_alignment_on,
                       .dog = {.initial_state = {.position = {.x = 16.0, .y = 1.0, .z = 24.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kAlignmentSheepStates,
                       .sheep_alignment = {.enabled = true}},
    },
    {
        .name = "sheep-dog-pressure-off",
        .definition = {.id = GameplayScenarioId::sheep_dog_pressure_off,
                       .dog = {.initial_state = {.position = {.x = 12.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogPressureSheepStates},
    },
    {
        .name = "sheep-dog-pressure-on",
        .definition = {.id = GameplayScenarioId::sheep_dog_pressure_on,
                       .dog = {.initial_state = {.position = {.x = 12.0, .y = 1.0, .z = 20.0},
                                                 .heading_radians = 0.0,
                                                 .grounded = true}},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogPressureSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-dog-approach-off",
        .definition = {.id = GameplayScenarioId::sheep_dog_approach_off,
                       .dog = {.initial_state = kApproachDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogApproachSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-dog-approach-on",
        .definition = {.id = GameplayScenarioId::sheep_dog_approach_on,
                       .dog = {.initial_state = kApproachDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogApproachSheepStates,
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_approach = {.enabled = true}},
    },
    {
        .name = "sheep-dog-facing-off",
        .definition = {.id = GameplayScenarioId::sheep_dog_facing_off,
                       .dog = {.initial_state = kFacingDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogFacingSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-dog-facing-on",
        .definition = {.id = GameplayScenarioId::sheep_dog_facing_on,
                       .dog = {.initial_state = kFacingDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogFacingSheepStates,
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_facing = {.enabled = true}},
    },
}};

} // namespace

std::optional<GameplayScenarioDefinition> find_gameplay_scenario(std::string_view name) noexcept {
    for (const NamedGameplayScenario& scenario : kGameplayScenarios) {
        if (scenario.name == name) {
            return scenario.definition;
        }
    }
    return std::nullopt;
}

std::string_view gameplay_scenario_name(GameplayScenarioId scenario) noexcept {
    for (const NamedGameplayScenario& definition : kGameplayScenarios) {
        if (definition.definition.id == scenario) {
            return definition.name;
        }
    }
    return "unknown";
}

} // namespace wide_eye::game
