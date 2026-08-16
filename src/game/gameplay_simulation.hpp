#pragma once

#include "core/runtime.hpp"
#include "game/dog_controller.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace wide_eye::game {

struct GameplayTickInput {
    // An absent dog input advances authoritative time while preserving the
    // accepted free-debug behavior that suspends the dog motor.
    std::optional<DogMoveInput> dog_move;
};

enum class SheepBehaviorState : std::uint8_t {
    settled,
    alert,
    driven,
    recovering,
};

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

[[nodiscard]] SheepState interpolate_sheep_state(const SheepState& previous,
                                                 const SheepState& current, double alpha) noexcept;

struct GameplaySnapshot {
    std::uint64_t tick = 0;
    DogState dog{};
    SheepStateBuffer sheep{};

    bool operator==(const GameplaySnapshot&) const = default;
};

// Owns the authoritative gameplay state advanced by the platform scheduler.
// Callers provide one input per fixed tick; render cadence and frame delta are
// deliberately absent from this API.
class GameplaySimulation {
  public:
    static constexpr std::uint32_t kTicksPerSecond = core::FixedStepAccumulator::ticks_per_second;
    static constexpr double kFixedDeltaSeconds = 1.0 / static_cast<double>(kTicksPerSecond);

    explicit GameplaySimulation(DogScenarioDefinition scenario) noexcept;

    void fixed_update(const GameplayTickInput& input) noexcept;
    void restart() noexcept;

    [[nodiscard]] const GameplaySnapshot& previous_snapshot() const noexcept;
    [[nodiscard]] const GameplaySnapshot& current_snapshot() const noexcept;
    [[nodiscard]] GameplaySnapshot interpolated_snapshot(double alpha) const noexcept;
    [[nodiscard]] const DogScenarioDefinition& scenario() const noexcept;
    [[nodiscard]] std::uint32_t restart_count() const noexcept;

  private:
    DogController dog_;
    GameplaySnapshot previous_{};
    GameplaySnapshot current_{};
};

} // namespace wide_eye::game
