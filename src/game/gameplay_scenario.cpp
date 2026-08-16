#include "game/gameplay_scenario.hpp"

#include <array>

namespace wide_eye::game {
namespace {

struct NamedGameplayScenario {
    std::string_view name;
    GameplayScenarioDefinition definition;
};

constexpr std::array<NamedGameplayScenario, 5> kGameplayScenarios{{
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
