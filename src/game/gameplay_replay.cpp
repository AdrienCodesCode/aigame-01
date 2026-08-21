#include "game/gameplay_replay.hpp"

#include <algorithm>
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
           is_known_sheep_behavior(state.behavior) && is_known_sheep_temperament(state.temperament);
}

bool valid_social_evidence(const GameplaySnapshot& snapshot) noexcept {
    const auto valid_neighbor_set = [&snapshot](std::uint32_t subject_id, const auto& neighbor_ids,
                                                std::uint32_t neighbor_count,
                                                std::uint32_t candidate_count) {
        if (neighbor_count > neighbor_ids.size() || candidate_count < neighbor_count ||
            candidate_count >= snapshot.sheep.size()) {
            return false;
        }
        for (std::size_t neighbor_index = 0; neighbor_index < neighbor_ids.size();
             ++neighbor_index) {
            const std::uint32_t neighbor_id = neighbor_ids[neighbor_index];
            if (neighbor_index >= neighbor_count) {
                if (neighbor_id != 0) {
                    return false;
                }
                continue;
            }
            if (neighbor_id == 0 || neighbor_id == subject_id) {
                return false;
            }
            const bool neighbor_exists = std::any_of(
                snapshot.sheep.begin(), snapshot.sheep.end(),
                [neighbor_id](const SheepState& sheep) { return sheep.id == neighbor_id; });
            if (!neighbor_exists) {
                return false;
            }
            for (std::size_t prior = 0; prior < neighbor_index; ++prior) {
                if (neighbor_ids[prior] == neighbor_id) {
                    return false;
                }
            }
        }
        return true;
    };

    for (std::size_t index = 0; index < snapshot.sheep.size(); ++index) {
        const SheepSocialEvidence& evidence = snapshot.sheep_social_evidence[index];
        if (evidence.subject_id != snapshot.sheep[index].id ||
            !valid_neighbor_set(evidence.subject_id, evidence.attraction_neighbor_ids,
                                evidence.attraction_neighbor_count,
                                evidence.attraction_candidate_count) ||
            !valid_neighbor_set(evidence.subject_id, evidence.alignment_neighbor_ids,
                                evidence.alignment_neighbor_count,
                                evidence.alignment_candidate_count) ||
            !finite(evidence.separation_acceleration) ||
            !finite(evidence.attraction_acceleration) || !finite(evidence.alignment_acceleration)) {
            return false;
        }
    }
    return true;
}

bool valid_dog_pressure_evidence(const GameplaySnapshot& snapshot) noexcept {
    constexpr double kPi = 3.14159265358979323846;
    for (std::size_t index = 0; index < snapshot.sheep.size(); ++index) {
        const SheepDogPressureEvidence& evidence = snapshot.sheep_dog_pressure_evidence[index];
        if (evidence.subject_id != snapshot.sheep[index].id ||
            !std::isfinite(evidence.dog_distance) || evidence.dog_distance < 0.0 ||
            !std::isfinite(evidence.dog_relative_bearing_radians) ||
            evidence.dog_relative_bearing_radians < -kPi ||
            evidence.dog_relative_bearing_radians > kPi ||
            !std::isfinite(evidence.dog_approach_speed) ||
            !std::isfinite(evidence.dog_facing_alignment) || evidence.dog_facing_alignment < -1.0 ||
            evidence.dog_facing_alignment > 1.0 ||
            !is_known_paddock_obstacle(evidence.dog_line_of_sight_occluder) ||
            !std::isfinite(evidence.temperament_response_scale) ||
            !finite(evidence.pressure_acceleration) || !finite(evidence.approach_acceleration) ||
            !finite(evidence.facing_acceleration)) {
            return false;
        }
        // An evaluated stimulus always applied some fraction of the ordinary
        // response, so a zero or negative published scale would describe a
        // response that cannot exist rather than a stubborn one.
        if (evidence.stimulus_evaluated && evidence.temperament_response_scale <= 0.0) {
            return false;
        }
        // A blocked sight line must name its occluder, and a clear one must name
        // none, so the flag and the identified shape cannot disagree.
        if (evidence.dog_line_of_sight_blocked !=
            (evidence.dog_line_of_sight_occluder != PaddockObstacle::none)) {
            return false;
        }
        if (!evidence.stimulus_evaluated &&
            (evidence.dog_distance != 0.0 || evidence.dog_relative_bearing_radians != 0.0 ||
             evidence.dog_approach_speed != 0.0 || evidence.dog_facing_alignment != 0.0 ||
             evidence.dog_line_of_sight_blocked ||
             evidence.dog_line_of_sight_occluder != PaddockObstacle::none ||
             evidence.temperament_response_scale != 0.0 ||
             evidence.pressure_acceleration != Vec3{} || evidence.approach_acceleration != Vec3{} ||
             evidence.facing_acceleration != Vec3{})) {
            return false;
        }
    }
    return true;
}

bool valid_collision_evidence(const GameplaySnapshot& snapshot) noexcept {
    for (std::size_t index = 0; index < snapshot.sheep.size(); ++index) {
        const SheepCollisionEvidence& evidence = snapshot.sheep_collision_evidence[index];
        if (evidence.subject_id != snapshot.sheep[index].id ||
            !is_known_paddock_obstacle(evidence.obstacle)) {
            return false;
        }
        // A named obstacle must have actually stopped an axis. The reverse is
        // not required: the paddock's outer bounds clip without being one of
        // the analytic obstacle shapes.
        if (evidence.obstacle != PaddockObstacle::none && !evidence.clipped_x &&
            !evidence.clipped_z) {
            return false;
        }
    }
    return true;
}

bool valid_avoidance_evidence(const GameplaySnapshot& snapshot) noexcept {
    for (std::size_t index = 0; index < snapshot.sheep.size(); ++index) {
        const SheepAvoidanceEvidence& evidence = snapshot.sheep_avoidance_evidence[index];
        if (evidence.subject_id != snapshot.sheep[index].id ||
            !is_known_paddock_obstacle(evidence.obstacle) ||
            !std::isfinite(evidence.obstacle_distance) || evidence.obstacle_distance < 0.0 ||
            !finite(evidence.avoidance_acceleration)) {
            return false;
        }
        // The term probes along the sheep's own travel, so a sheep it never ran
        // for cannot have seen anything and cannot have pushed anywhere.
        if (!evidence.avoidance_evaluated &&
            (evidence.drop_ahead || evidence.obstacle != PaddockObstacle::none ||
             evidence.obstacle_distance != 0.0 || evidence.avoidance_acceleration != Vec3{})) {
            return false;
        }
        // A distance without a shape is an anonymous number, and a push with
        // neither a shape nor a drop behind it is a direction from nowhere.
        if (evidence.obstacle == PaddockObstacle::none &&
            (evidence.obstacle_distance != 0.0 ||
             (!evidence.drop_ahead && evidence.avoidance_acceleration != Vec3{}))) {
            return false;
        }
    }
    return true;
}

bool valid_combined_influence_evidence(const GameplaySnapshot& snapshot) noexcept {
    for (std::size_t index = 0; index < snapshot.sheep.size(); ++index) {
        const SheepCombinedInfluenceEvidence& evidence =
            snapshot.sheep_combined_influence_evidence[index];
        if (evidence.subject_id != snapshot.sheep[index].id ||
            !std::isfinite(evidence.summed_acceleration_magnitude) ||
            evidence.summed_acceleration_magnitude < 0.0 ||
            !std::isfinite(evidence.applied_scale) || !finite(evidence.applied_acceleration)) {
            return false;
        }
        // The rule is a bound, not a gain: an evaluated tick either left the
        // summed terms alone or scaled them down, so a scale outside `(0, 1]`
        // would describe something the bound cannot do.
        if (evidence.bound_evaluated &&
            (evidence.applied_scale <= 0.0 || evidence.applied_scale > 1.0)) {
            return false;
        }
        if (!evidence.bound_evaluated &&
            (evidence.summed_acceleration_magnitude != 0.0 || evidence.applied_scale != 0.0 ||
             evidence.applied_acceleration != Vec3{})) {
            return false;
        }
    }
    return true;
}

bool valid_motion_limit_evidence(const GameplaySnapshot& snapshot) noexcept {
    constexpr double kPi = 3.14159265358979323846;
    for (std::size_t index = 0; index < snapshot.sheep.size(); ++index) {
        const SheepMotionLimitEvidence& evidence = snapshot.sheep_motion_limit_evidence[index];
        if (evidence.subject_id != snapshot.sheep[index].id ||
            !std::isfinite(evidence.integrated_speed) || evidence.integrated_speed < 0.0 ||
            !std::isfinite(evidence.applied_speed_scale) ||
            !std::isfinite(evidence.applied_speed) || evidence.applied_speed < 0.0 ||
            !std::isfinite(evidence.motion_heading_radians) ||
            evidence.motion_heading_radians < -kPi || evidence.motion_heading_radians > kPi ||
            !std::isfinite(evidence.heading_change_radians) ||
            evidence.heading_change_radians < -kPi || evidence.heading_change_radians > kPi) {
            return false;
        }
        // The rule is a clamp, not a throttle: an evaluated tick either left the
        // integrated speed alone or scaled it down, so a scale outside `(0, 1]`
        // or a result faster than the integration that produced it would
        // describe something the clamp cannot do.
        if (evidence.limit_evaluated &&
            (evidence.applied_speed_scale <= 0.0 || evidence.applied_speed_scale > 1.0 ||
             evidence.applied_speed > evidence.integrated_speed)) {
            return false;
        }
        // A sheep too slow to have a motion direction must not publish one.
        if (!evidence.motion_heading_followed &&
            (evidence.motion_heading_radians != 0.0 || evidence.heading_change_radians != 0.0)) {
            return false;
        }
        if (!evidence.limit_evaluated &&
            (evidence.motion_heading_followed || evidence.integrated_speed != 0.0 ||
             evidence.applied_speed_scale != 0.0 || evidence.applied_speed != 0.0)) {
            return false;
        }
    }
    return true;
}

std::string_view paddock_obstacle_name(PaddockObstacle obstacle) noexcept {
    switch (obstacle) {
    case PaddockObstacle::none:
        return "none";
    case PaddockObstacle::left_wall:
        return "left_wall";
    case PaddockObstacle::right_wall:
        return "right_wall";
    case PaddockObstacle::gate:
        return "gate";
    }
    return "unknown";
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

std::string_view sheep_temperament_name(SheepTemperament temperament) noexcept {
    switch (temperament) {
    case SheepTemperament::ordinary:
        return "ordinary";
    case SheepTemperament::nervous:
        return "nervous";
    case SheepTemperament::stubborn:
        return "stubborn";
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
    output += "\",\"temperament\":\"";
    output += sheep_temperament_name(sheep.temperament);
    output += "\",\"grounded\":";
    output += sheep.grounded ? "true" : "false";
    output += '}';
    return true;
}

bool append_social_evidence(std::string& output, const SheepSocialEvidence& evidence) {
    output += "{\"subject_id\":";
    if (!append_integer(output, evidence.subject_id)) {
        return false;
    }
    output += ",\"attraction_neighbor_ids\":[";
    for (std::size_t index = 0; index < evidence.attraction_neighbor_count; ++index) {
        if (index != 0) {
            output += ',';
        }
        if (!append_integer(output, evidence.attraction_neighbor_ids[index])) {
            return false;
        }
    }
    output += "],\"attraction_neighbor_count\":";
    if (!append_integer(output, evidence.attraction_neighbor_count)) {
        return false;
    }
    output += ",\"attraction_candidate_count\":";
    if (!append_integer(output, evidence.attraction_candidate_count)) {
        return false;
    }
    output += ",\"alignment_neighbor_ids\":[";
    for (std::size_t index = 0; index < evidence.alignment_neighbor_count; ++index) {
        if (index != 0) {
            output += ',';
        }
        if (!append_integer(output, evidence.alignment_neighbor_ids[index])) {
            return false;
        }
    }
    output += "],\"alignment_neighbor_count\":";
    if (!append_integer(output, evidence.alignment_neighbor_count)) {
        return false;
    }
    output += ",\"alignment_candidate_count\":";
    if (!append_integer(output, evidence.alignment_candidate_count)) {
        return false;
    }
    output += ",\"separation_acceleration\":";
    if (!append_vec3(output, evidence.separation_acceleration)) {
        return false;
    }
    output += ",\"attraction_acceleration\":";
    if (!append_vec3(output, evidence.attraction_acceleration)) {
        return false;
    }
    output += ",\"alignment_acceleration\":";
    if (!append_vec3(output, evidence.alignment_acceleration)) {
        return false;
    }
    output += '}';
    return true;
}

bool append_dog_pressure_evidence(std::string& output, const SheepDogPressureEvidence& evidence) {
    output += "{\"subject_id\":";
    if (!append_integer(output, evidence.subject_id)) {
        return false;
    }
    output += ",\"stimulus_evaluated\":";
    output += evidence.stimulus_evaluated ? "true" : "false";
    output += ",\"dog_distance\":";
    if (!append_double(output, evidence.dog_distance)) {
        return false;
    }
    output += ",\"dog_relative_bearing_radians\":";
    if (!append_double(output, evidence.dog_relative_bearing_radians)) {
        return false;
    }
    output += ",\"dog_approach_speed\":";
    if (!append_double(output, evidence.dog_approach_speed)) {
        return false;
    }
    output += ",\"dog_facing_alignment\":";
    if (!append_double(output, evidence.dog_facing_alignment)) {
        return false;
    }
    output += ",\"dog_line_of_sight_blocked\":";
    output += evidence.dog_line_of_sight_blocked ? "true" : "false";
    output += ",\"dog_line_of_sight_occluder\":\"";
    output += paddock_obstacle_name(evidence.dog_line_of_sight_occluder);
    output += "\",\"temperament_response_scale\":";
    if (!append_double(output, evidence.temperament_response_scale)) {
        return false;
    }
    output += ",\"pressure_acceleration\":";
    if (!append_vec3(output, evidence.pressure_acceleration)) {
        return false;
    }
    output += ",\"approach_acceleration\":";
    if (!append_vec3(output, evidence.approach_acceleration)) {
        return false;
    }
    output += ",\"facing_acceleration\":";
    if (!append_vec3(output, evidence.facing_acceleration)) {
        return false;
    }
    output += '}';
    return true;
}

bool append_collision_evidence(std::string& output, const SheepCollisionEvidence& evidence) {
    output += "{\"subject_id\":";
    if (!append_integer(output, evidence.subject_id)) {
        return false;
    }
    output += ",\"clipped_x\":";
    output += evidence.clipped_x ? "true" : "false";
    output += ",\"clipped_z\":";
    output += evidence.clipped_z ? "true" : "false";
    output += ",\"contact_obstacle\":\"";
    output += paddock_obstacle_name(evidence.obstacle);
    output += "\"}";
    return true;
}

bool append_avoidance_evidence(std::string& output, const SheepAvoidanceEvidence& evidence) {
    output += "{\"subject_id\":";
    if (!append_integer(output, evidence.subject_id)) {
        return false;
    }
    output += ",\"avoidance_evaluated\":";
    output += evidence.avoidance_evaluated ? "true" : "false";
    output += ",\"obstacle_ahead\":\"";
    output += paddock_obstacle_name(evidence.obstacle);
    output += "\",\"obstacle_distance\":";
    if (!append_double(output, evidence.obstacle_distance)) {
        return false;
    }
    output += ",\"drop_ahead\":";
    output += evidence.drop_ahead ? "true" : "false";
    output += ",\"avoidance_acceleration\":";
    if (!append_vec3(output, evidence.avoidance_acceleration)) {
        return false;
    }
    output += '}';
    return true;
}

bool append_combined_influence_evidence(std::string& output,
                                        const SheepCombinedInfluenceEvidence& evidence) {
    output += "{\"subject_id\":";
    if (!append_integer(output, evidence.subject_id)) {
        return false;
    }
    output += ",\"bound_evaluated\":";
    output += evidence.bound_evaluated ? "true" : "false";
    output += ",\"summed_acceleration_magnitude\":";
    if (!append_double(output, evidence.summed_acceleration_magnitude)) {
        return false;
    }
    output += ",\"applied_scale\":";
    if (!append_double(output, evidence.applied_scale)) {
        return false;
    }
    output += ",\"applied_acceleration\":";
    if (!append_vec3(output, evidence.applied_acceleration)) {
        return false;
    }
    output += '}';
    return true;
}

bool append_motion_limit_evidence(std::string& output, const SheepMotionLimitEvidence& evidence) {
    output += "{\"subject_id\":";
    if (!append_integer(output, evidence.subject_id)) {
        return false;
    }
    output += ",\"limit_evaluated\":";
    output += evidence.limit_evaluated ? "true" : "false";
    output += ",\"integrated_speed\":";
    if (!append_double(output, evidence.integrated_speed)) {
        return false;
    }
    output += ",\"applied_speed_scale\":";
    if (!append_double(output, evidence.applied_speed_scale)) {
        return false;
    }
    output += ",\"applied_speed\":";
    if (!append_double(output, evidence.applied_speed)) {
        return false;
    }
    output += ",\"motion_heading_followed\":";
    output += evidence.motion_heading_followed ? "true" : "false";
    output += ",\"motion_heading_radians\":";
    if (!append_double(output, evidence.motion_heading_radians)) {
        return false;
    }
    output += ",\"heading_change_radians\":";
    if (!append_double(output, evidence.heading_change_radians)) {
        return false;
    }
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
    output += ",\"sheep_social_evidence\":[";
    bool first_evidence = true;
    for (const SheepSocialEvidence& evidence : snapshot.sheep_social_evidence) {
        if (!first_evidence) {
            output += ',';
        }
        first_evidence = false;
        if (!append_social_evidence(output, evidence)) {
            return false;
        }
    }
    output += ']';
    output += ",\"sheep_dog_pressure_evidence\":[";
    bool first_dog_evidence = true;
    for (const SheepDogPressureEvidence& evidence : snapshot.sheep_dog_pressure_evidence) {
        if (!first_dog_evidence) {
            output += ',';
        }
        first_dog_evidence = false;
        if (!append_dog_pressure_evidence(output, evidence)) {
            return false;
        }
    }
    output += ']';
    output += ",\"sheep_collision_evidence\":[";
    bool first_collision_evidence = true;
    for (const SheepCollisionEvidence& evidence : snapshot.sheep_collision_evidence) {
        if (!first_collision_evidence) {
            output += ',';
        }
        first_collision_evidence = false;
        if (!append_collision_evidence(output, evidence)) {
            return false;
        }
    }
    output += ']';
    output += ",\"sheep_avoidance_evidence\":[";
    bool first_avoidance_evidence = true;
    for (const SheepAvoidanceEvidence& evidence : snapshot.sheep_avoidance_evidence) {
        if (!first_avoidance_evidence) {
            output += ',';
        }
        first_avoidance_evidence = false;
        if (!append_avoidance_evidence(output, evidence)) {
            return false;
        }
    }
    output += ']';
    output += ",\"sheep_combined_influence_evidence\":[";
    bool first_combined_evidence = true;
    for (const SheepCombinedInfluenceEvidence& evidence :
         snapshot.sheep_combined_influence_evidence) {
        if (!first_combined_evidence) {
            output += ',';
        }
        first_combined_evidence = false;
        if (!append_combined_influence_evidence(output, evidence)) {
            return false;
        }
    }
    output += ']';
    output += ",\"sheep_motion_limit_evidence\":[";
    bool first_motion_limit_evidence = true;
    for (const SheepMotionLimitEvidence& evidence : snapshot.sheep_motion_limit_evidence) {
        if (!first_motion_limit_evidence) {
            output += ',';
        }
        first_motion_limit_evidence = false;
        if (!append_motion_limit_evidence(output, evidence)) {
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
    if (!valid_social_evidence(simulation.previous_snapshot()) ||
        !valid_social_evidence(simulation.current_snapshot()) ||
        !valid_dog_pressure_evidence(simulation.previous_snapshot()) ||
        !valid_dog_pressure_evidence(simulation.current_snapshot()) ||
        !valid_collision_evidence(simulation.previous_snapshot()) ||
        !valid_collision_evidence(simulation.current_snapshot()) ||
        !valid_avoidance_evidence(simulation.previous_snapshot()) ||
        !valid_avoidance_evidence(simulation.current_snapshot()) ||
        !valid_combined_influence_evidence(simulation.previous_snapshot()) ||
        !valid_combined_influence_evidence(simulation.current_snapshot()) ||
        !valid_motion_limit_evidence(simulation.previous_snapshot()) ||
        !valid_motion_limit_evidence(simulation.current_snapshot())) {
        return {.error = GameplayContractError::non_finite_state, .text = {}};
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
