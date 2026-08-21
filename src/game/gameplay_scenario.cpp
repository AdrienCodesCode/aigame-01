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

// One stationary dog at (16, 13) just north of the paddock wall line isolates
// line of sight against the analytic obstacles the dog itself collides with:
// sheep 1 shares the open ground north of the walls on a clear line, sheep 2 and
// sheep 4 stand south of the left and right walls at an exact 3-4-5 distance of
// 5, sheep 3 stands south of the gate opening and sees the dog only while that
// gate is open, and sheep 5 is hidden by the left wall from outside the pressure
// radius. The dog heading looks south down the gate line so a derived fixture
// can observe the approach and facing terms releasing together with pressure.
constexpr SheepStateBuffer kDogLineOfSightSheepStates{{
    {.id = 1,
     .position = {.x = 16.0, .y = 1.0, .z = 9.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 13.0, .y = 1.0, .z = 17.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 16.0, .y = 1.0, .z = 18.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 19.0, .y = 1.0, .z = 17.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 10.0, .y = 1.0, .z = 21.0},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kLineOfSightDogState{.position = {.x = 16.0, .y = 1.0, .z = 13.0},
                                        .heading_radians = 3.14159265358979323846,
                                        .grounded = true};

// One stationary dog parked well clear of the wall line and five sheep given an
// exact initial velocity isolate paddock collision from every steering term: no
// social or dog term is enabled, so each sheep travels in a straight line at a
// constant speed until the analytic paddock stops it, which makes every resting
// coordinate exact arithmetic. Every sheep that contacts something starts an exact
// 3.5 units of travel along its blocked axis from the limit that stops it.
// Sheep 1 runs head-on into the left wall, sheep 2 runs
// at the gate line and is the paired variable, sheep 3 arrives diagonally at the
// right wall so one axis is blocked while the other keeps running, sheep 4 never
// touches anything and is the untouched control, and sheep 5 runs at the
// paddock's own outer bound, which stops a sheep without being one of the named
// obstacle shapes.
constexpr double kPaddockCollisionSheepSpeed = 3.0;

constexpr SheepStateBuffer kPaddockCollisionSheepStates{{
    {.id = 1,
     .position = {.x = 8.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 16.0, .y = 1.0, .z = 20.0},
     .velocity = {.z = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 24.0, .y = 1.0, .z = 20.0},
     .velocity = {.x = -kPaddockCollisionSheepSpeed, .z = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 28.0, .y = 1.0, .z = 26.0},
     .velocity = {.x = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 4.0, .y = 1.0, .z = 20.0},
     .velocity = {.x = -kPaddockCollisionSheepSpeed},
     .heading_radians = 0.0,
     .grounded = true},
}};

constexpr DogState kPaddockCollisionDogState{
    .position = {.x = 16.0, .y = 1.0, .z = 28.0}, .heading_radians = 0.0, .grounded = true};

// One stationary dog at (16, 26) with five sheep on an exact 5-unit ring around
// it isolates temperament: every sheep sees the same prior-state distance, the
// same falloff, and the same dog, so the only thing that can make two of them
// move differently is the temperament each one carries. Each offset is an exact
// 3-4-5 or 0-5-5 triangle, and the dog heading of zero looks straight down the
// -z axis at the ring so a derived fixture can enable the approach and facing
// terms without moving anything. The ring sits five units north of the wall
// line, so no sheep starts within its own body radius of an obstacle face. The
// nervous and stubborn sheep are placed in mirrored pairs across the gate line,
// so a per-temperament result cannot be an artifact of one bearing.
constexpr SheepStateBuffer kTemperamentSheepStates{{
    {.id = 1,
     .position = {.x = 16.0, .y = 1.0, .z = 21.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::ordinary,
     .grounded = true},
    {.id = 2,
     .position = {.x = 13.0, .y = 1.0, .z = 22.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
    {.id = 3,
     .position = {.x = 19.0, .y = 1.0, .z = 22.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::stubborn,
     .grounded = true},
    {.id = 4,
     .position = {.x = 12.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::stubborn,
     .grounded = true},
    {.id = 5,
     .position = {.x = 20.0, .y = 1.0, .z = 23.0},
     .heading_radians = 0.0,
     .temperament = SheepTemperament::nervous,
     .grounded = true},
}};

constexpr DogState kTemperamentDogState{
    .position = {.x = 16.0, .y = 1.0, .z = 26.0}, .heading_radians = 0.0, .grounded = true};

constexpr std::array<NamedGameplayScenario, 21> kGameplayScenarios{{
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
                                                 .grounded = true}},
                       .gate_open = true},
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
    {
        .name = "sheep-dog-line-of-sight-off",
        .definition = {.id = GameplayScenarioId::sheep_dog_line_of_sight_off,
                       .dog = {.initial_state = kLineOfSightDogState},
                       .gate_open = true,
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogLineOfSightSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-dog-line-of-sight-on",
        .definition = {.id = GameplayScenarioId::sheep_dog_line_of_sight_on,
                       .dog = {.initial_state = kLineOfSightDogState},
                       .gate_open = true,
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kDogLineOfSightSheepStates,
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_dog_line_of_sight = {.enabled = true}},
    },
    {
        .name = "sheep-paddock-collision-closed-gate",
        .definition = {.id = GameplayScenarioId::sheep_paddock_collision_closed_gate,
                       .dog = {.initial_state = kPaddockCollisionDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kPaddockCollisionSheepStates},
    },
    {
        .name = "sheep-paddock-collision-open-gate",
        .definition = {.id = GameplayScenarioId::sheep_paddock_collision_open_gate,
                       .dog = {.initial_state = kPaddockCollisionDogState},
                       .gate_open = true,
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kPaddockCollisionSheepStates},
    },
    {
        .name = "sheep-temperament-neutral",
        .definition = {.id = GameplayScenarioId::sheep_temperament_neutral,
                       .dog = {.initial_state = kTemperamentDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kTemperamentSheepStates,
                       .sheep_dog_pressure = {.enabled = true}},
    },
    {
        .name = "sheep-temperament-varied",
        .definition = {.id = GameplayScenarioId::sheep_temperament_varied,
                       .dog = {.initial_state = kTemperamentDogState},
                       .sheep_fixture = SheepFixture::local_social_response,
                       .initial_sheep = kTemperamentSheepStates,
                       .sheep_dog_pressure = {.enabled = true},
                       .sheep_temperament = {.enabled = true}},
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
