#include "game/gameplay_replay.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <system_error>

namespace wide_eye::game {
namespace {

bool is_known_scenario(GameplayScenarioId scenario) noexcept {
    return gameplay_scenario_name(scenario) != "unknown";
}

bool valid_action(const GameplayTickInput& input) noexcept {
    if (!input.dog_move.has_value()) {
        return true;
    }

    const DogMoveInput& dog_move = *input.dog_move;
    return std::isfinite(dog_move.world_x) && std::isfinite(dog_move.world_z) &&
           dog_move.world_x >= -1.0 && dog_move.world_x <= 1.0 && dog_move.world_z >= -1.0 &&
           dog_move.world_z <= 1.0;
}

bool finite(const Vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(const DogState& state) noexcept {
    return finite(state.position) && finite(state.velocity) && std::isfinite(state.heading_radians);
}

bool valid_state(const SheepState& state) noexcept {
    return finite(state.position) && finite(state.velocity) &&
           std::isfinite(state.heading_radians) && std::isfinite(state.arousal) &&
           is_known_sheep_behavior(state.behavior);
}

std::string_view sheep_behavior_name(SheepBehaviorState behavior) noexcept {
    switch (behavior) {
    case SheepBehaviorState::settled:
        return "settled";
    case SheepBehaviorState::alert:
        return "alert";
    case SheepBehaviorState::driven:
        return "driven";
    case SheepBehaviorState::recovering:
        return "recovering";
    }
    return "unknown";
}

template <typename Integer> bool append_integer(std::string& output, Integer value) {
    std::array<char, std::numeric_limits<Integer>::digits10 + 3> buffer{};
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (error != std::errc{}) {
        return false;
    }
    output.append(buffer.data(), end);
    return true;
}

bool append_double(std::string& output, double value) {
    std::array<char, 64> buffer{};
    const auto [end, error] =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value,
                      std::chars_format::general, std::numeric_limits<double>::max_digits10);
    if (error != std::errc{}) {
        return false;
    }
    output.append(buffer.data(), end);
    return true;
}

bool append_scenario(std::string& output, const GameplayScenarioSeed& scenario) {
    output += "{\"seed_format_version\":";
    if (!append_integer(output, scenario.format_version)) {
        return false;
    }
    output += ",\"id\":\"";
    output += gameplay_scenario_name(scenario.scenario);
    output += "\",\"version\":";
    if (!append_integer(output, scenario.scenario_version)) {
        return false;
    }
    output += ",\"seed\":";
    if (!append_integer(output, scenario.seed)) {
        return false;
    }
    output += '}';
    return true;
}

bool append_vec3(std::string& output, const Vec3& value) {
    output += "{\"x\":";
    if (!append_double(output, value.x)) {
        return false;
    }
    output += ",\"y\":";
    if (!append_double(output, value.y)) {
        return false;
    }
    output += ",\"z\":";
    if (!append_double(output, value.z)) {
        return false;
    }
    output += '}';
    return true;
}

bool append_dog_state(std::string& output, const DogState& dog) {
    output += "{\"position\":";
    if (!append_vec3(output, dog.position)) {
        return false;
    }
    output += ",\"velocity\":";
    if (!append_vec3(output, dog.velocity)) {
        return false;
    }
    output += ",\"heading_radians\":";
    if (!append_double(output, dog.heading_radians)) {
        return false;
    }
    output += ",\"grounded\":";
    output += dog.grounded ? "true" : "false";
    output += '}';
    return true;
}

bool append_sheep_state(std::string& output, const SheepState& sheep) {
    output += "{\"id\":";
    if (!append_integer(output, sheep.id)) {
        return false;
    }
    output += ",\"position\":";
    if (!append_vec3(output, sheep.position)) {
        return false;
    }
    output += ",\"velocity\":";
    if (!append_vec3(output, sheep.velocity)) {
        return false;
    }
    output += ",\"heading_radians\":";
    if (!append_double(output, sheep.heading_radians)) {
        return false;
    }
    output += ",\"arousal\":";
    if (!append_double(output, sheep.arousal)) {
        return false;
    }
    output += ",\"behavior\":\"";
    output += sheep_behavior_name(sheep.behavior);
    output += "\",\"grounded\":";
    output += sheep.grounded ? "true" : "false";
    output += '}';
    return true;
}

bool append_snapshot(std::string& output, const GameplaySnapshot& snapshot) {
    output += "{\"tick\":";
    if (!append_integer(output, snapshot.tick)) {
        return false;
    }
    output += ",\"dog\":";
    if (!append_dog_state(output, snapshot.dog)) {
        return false;
    }
    output += ",\"sheep\":[";
    bool first_sheep = true;
    for (const SheepState& sheep : snapshot.sheep) {
        if (!first_sheep) {
            output += ',';
        }
        first_sheep = false;
        if (!append_sheep_state(output, sheep)) {
            return false;
        }
    }
    output += ']';
    output += '}';
    return true;
}

} // namespace

GameplayScenarioSeed gameplay_scenario_seed(const GameplaySimulation& simulation) noexcept {
    const GameplayScenarioDefinition& scenario = simulation.scenario();
    return {
        .format_version = kGameplaySeedFormatVersion,
        .scenario = scenario.id,
        .scenario_version = scenario.version,
        .seed = scenario.seed,
    };
}

GameplayContractError validate_gameplay_replay(const GameplayReplay& replay) noexcept {
    if (replay.format_version != kGameplayReplayFormatVersion) {
        return GameplayContractError::unsupported_replay_version;
    }
    if (replay.action_input_format_version != kGameplayActionInputFormatVersion) {
        return GameplayContractError::unsupported_action_input_version;
    }
    if (replay.scenario.format_version != kGameplaySeedFormatVersion) {
        return GameplayContractError::unsupported_seed_version;
    }
    if (replay.ticks_per_second != GameplaySimulation::kTicksPerSecond) {
        return GameplayContractError::tick_rate_mismatch;
    }
    if (!is_known_scenario(replay.scenario.scenario)) {
        return GameplayContractError::unknown_scenario;
    }
    if (replay.scenario.scenario_version == 0) {
        return GameplayContractError::invalid_scenario_version;
    }

    std::uint64_t expected_tick = 0;
    for (const TickIndexedGameplayAction& action : replay.actions) {
        if (action.tick != expected_tick) {
            return GameplayContractError::non_contiguous_action_tick;
        }
        if (!valid_action(action.input)) {
            return GameplayContractError::invalid_action_value;
        }
        if (expected_tick == std::numeric_limits<std::uint64_t>::max()) {
            return GameplayContractError::non_contiguous_action_tick;
        }
        ++expected_tick;
    }

    return GameplayContractError::none;
}

GameplayContractError validate_gameplay_replay(const GameplayReplay& replay,
                                               const GameplaySimulation& simulation) noexcept {
    const GameplayContractError structural_error = validate_gameplay_replay(replay);
    if (structural_error != GameplayContractError::none) {
        return structural_error;
    }
    if (simulation.current_snapshot().tick != 0) {
        return GameplayContractError::simulation_not_at_replay_start;
    }
    if (replay.scenario != gameplay_scenario_seed(simulation)) {
        return GameplayContractError::scenario_mismatch;
    }
    return GameplayContractError::none;
}

GameplayContractError apply_gameplay_replay(GameplaySimulation& simulation,
                                            const GameplayReplay& replay) noexcept {
    const GameplayContractError validation_error = validate_gameplay_replay(replay, simulation);
    if (validation_error != GameplayContractError::none) {
        return validation_error;
    }

    for (const TickIndexedGameplayAction& action : replay.actions) {
        simulation.fixed_update(action.input);
    }
    return GameplayContractError::none;
}

GameplayTextResult gameplay_replay_json(const GameplayReplay& replay) {
    const GameplayContractError validation_error = validate_gameplay_replay(replay);
    if (validation_error != GameplayContractError::none) {
        return {.error = validation_error, .text = {}};
    }

    GameplayTextResult result;
    std::string& output = result.text;
    output.reserve(256 + replay.actions.size() * 96);
    output += "{\"schema\":\"wide-eye.gameplay-replay\",\"version\":";
    if (!append_integer(output, replay.format_version)) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"tick_rate\":";
    if (!append_integer(output, replay.ticks_per_second)) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"action_input_version\":";
    if (!append_integer(output, replay.action_input_format_version)) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"scenario\":";
    if (!append_scenario(output, replay.scenario)) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"actions\":[";

    bool first_action = true;
    for (const TickIndexedGameplayAction& action : replay.actions) {
        if (!first_action) {
            output += ',';
        }
        first_action = false;
        output += "{\"tick\":";
        if (!append_integer(output, action.tick)) {
            return {.error = GameplayContractError::text_encoding_failed, .text = {}};
        }
        output += ",\"dog_move\":";
        if (!action.input.dog_move.has_value()) {
            output += "null}";
            continue;
        }

        const DogMoveInput& dog_move = *action.input.dog_move;
        output += "{\"world_x\":";
        if (!append_double(output, dog_move.world_x)) {
            return {.error = GameplayContractError::text_encoding_failed, .text = {}};
        }
        output += ",\"world_z\":";
        if (!append_double(output, dog_move.world_z)) {
            return {.error = GameplayContractError::text_encoding_failed, .text = {}};
        }
        output += ",\"sprint\":";
        output += dog_move.sprint ? "true" : "false";
        output += "}}";
    }
    output += "]}";
    return result;
}

GameplayTextResult gameplay_state_dump_json(const GameplaySimulation& simulation) {
    const GameplayScenarioSeed scenario = gameplay_scenario_seed(simulation);
    if (!is_known_scenario(scenario.scenario)) {
        return {.error = GameplayContractError::unknown_scenario, .text = {}};
    }
    if (scenario.scenario_version == 0) {
        return {.error = GameplayContractError::invalid_scenario_version, .text = {}};
    }
    if (!finite(simulation.previous_snapshot().dog) || !finite(simulation.current_snapshot().dog)) {
        return {.error = GameplayContractError::non_finite_state, .text = {}};
    }
    for (const SheepState& sheep : simulation.previous_snapshot().sheep) {
        if (!valid_state(sheep)) {
            return {.error = GameplayContractError::non_finite_state, .text = {}};
        }
    }
    for (const SheepState& sheep : simulation.current_snapshot().sheep) {
        if (!valid_state(sheep)) {
            return {.error = GameplayContractError::non_finite_state, .text = {}};
        }
    }

    GameplayTextResult result;
    std::string& output = result.text;
    output.reserve(2'048);
    output += "{\"schema\":\"wide-eye.gameplay-state\",\"version\":";
    if (!append_integer(output, kGameplayStateDumpFormatVersion)) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"tick_rate\":";
    if (!append_integer(output, GameplaySimulation::kTicksPerSecond)) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"scenario\":";
    if (!append_scenario(output, scenario)) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"restart_count\":";
    if (!append_integer(output, simulation.restart_count())) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"previous\":";
    if (!append_snapshot(output, simulation.previous_snapshot())) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += ",\"current\":";
    if (!append_snapshot(output, simulation.current_snapshot())) {
        return {.error = GameplayContractError::text_encoding_failed, .text = {}};
    }
    output += '}';
    return result;
}

std::string_view gameplay_contract_error_name(GameplayContractError error) noexcept {
    switch (error) {
    case GameplayContractError::none:
        return "none";
    case GameplayContractError::unsupported_replay_version:
        return "unsupported_replay_version";
    case GameplayContractError::unsupported_action_input_version:
        return "unsupported_action_input_version";
    case GameplayContractError::unsupported_seed_version:
        return "unsupported_seed_version";
    case GameplayContractError::tick_rate_mismatch:
        return "tick_rate_mismatch";
    case GameplayContractError::unknown_scenario:
        return "unknown_scenario";
    case GameplayContractError::invalid_scenario_version:
        return "invalid_scenario_version";
    case GameplayContractError::non_contiguous_action_tick:
        return "non_contiguous_action_tick";
    case GameplayContractError::invalid_action_value:
        return "invalid_action_value";
    case GameplayContractError::simulation_not_at_replay_start:
        return "simulation_not_at_replay_start";
    case GameplayContractError::scenario_mismatch:
        return "scenario_mismatch";
    case GameplayContractError::non_finite_state:
        return "non_finite_state";
    case GameplayContractError::text_encoding_failed:
        return "text_encoding_failed";
    }
    return "unknown";
}

} // namespace wide_eye::game
