#pragma once

#include "game/dog_controller.hpp"

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
};

enum class SheepFixture : std::uint8_t {
    stationary,
    scripted_presentation_motion,
};

// Owns the complete deterministic starting contract for one game scenario.
// Controller-specific configuration stays nested under its subsystem, while
// sheep, objective, and future fixture state remain owned at the game level.
struct GameplayScenarioDefinition {
    GameplayScenarioId id = GameplayScenarioId::paddock_start;
    std::uint32_t version = 1;
    std::uint64_t seed = 0;
    DogControllerConfiguration dog{};
    SheepFixture sheep_fixture = SheepFixture::stationary;
};

[[nodiscard]] std::optional<GameplayScenarioDefinition>
find_gameplay_scenario(std::string_view name) noexcept;
[[nodiscard]] std::string_view gameplay_scenario_name(GameplayScenarioId scenario) noexcept;

} // namespace wide_eye::game
