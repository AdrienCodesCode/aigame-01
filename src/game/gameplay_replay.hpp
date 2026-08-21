#pragma once

#include "game/gameplay_simulation.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wide_eye::game {

inline constexpr std::uint32_t kGameplaySeedFormatVersion = 1;
inline constexpr std::uint32_t kGameplayActionInputFormatVersion = 1;
inline constexpr std::uint32_t kGameplayReplayFormatVersion = 1;
inline constexpr std::uint32_t kGameplayStateDumpFormatVersion = 9;

// Identifies the complete deterministic starting contract independently of a
// render frame or wall clock. Scenario parameters remain owned by the named
// scenario definition; changing those parameters requires a scenario-version
// increment.
struct GameplayScenarioSeed {
    std::uint32_t format_version = kGameplaySeedFormatVersion;
    GameplayScenarioId scenario = GameplayScenarioId::paddock_start;
    std::uint32_t scenario_version = 1;
    std::uint64_t seed = 0;

    bool operator==(const GameplayScenarioSeed&) const = default;
};

// One domain action consumed by one authoritative tick. Physical input events,
// render cadence, and frame delta are deliberately absent from this format.
struct TickIndexedGameplayAction {
    std::uint64_t tick = 0;
    GameplayTickInput input{};
};

struct GameplayReplay {
    std::uint32_t format_version = kGameplayReplayFormatVersion;
    std::uint32_t action_input_format_version = kGameplayActionInputFormatVersion;
    std::uint32_t ticks_per_second = GameplaySimulation::kTicksPerSecond;
    GameplayScenarioSeed scenario{};
    std::vector<TickIndexedGameplayAction> actions;
};

enum class GameplayContractError : std::uint8_t {
    none,
    unsupported_replay_version,
    unsupported_action_input_version,
    unsupported_seed_version,
    tick_rate_mismatch,
    unknown_scenario,
    invalid_scenario_version,
    non_contiguous_action_tick,
    invalid_action_value,
    simulation_not_at_replay_start,
    scenario_mismatch,
    non_finite_state,
    text_encoding_failed,
};

struct GameplayTextResult {
    GameplayContractError error = GameplayContractError::none;
    std::string text;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == GameplayContractError::none;
    }
};

[[nodiscard]] GameplayScenarioSeed
gameplay_scenario_seed(const GameplaySimulation& simulation) noexcept;
[[nodiscard]] GameplayContractError validate_gameplay_replay(const GameplayReplay& replay) noexcept;
[[nodiscard]] GameplayContractError
validate_gameplay_replay(const GameplayReplay& replay,
                         const GameplaySimulation& simulation) noexcept;

// Validation completes before the first tick is applied, so a rejected replay
// cannot partially mutate the supplied simulation.
[[nodiscard]] GameplayContractError apply_gameplay_replay(GameplaySimulation& simulation,
                                                          const GameplayReplay& replay) noexcept;

// These writers define the canonical compact JSON representation of the
// independently versioned contracts above. Decoding files and exposing CLI
// paths are separate tooling work.
[[nodiscard]] GameplayTextResult gameplay_replay_json(const GameplayReplay& replay);
[[nodiscard]] GameplayTextResult gameplay_state_dump_json(const GameplaySimulation& simulation);
[[nodiscard]] std::string_view gameplay_contract_error_name(GameplayContractError error) noexcept;

} // namespace wide_eye::game
