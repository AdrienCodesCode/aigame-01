#pragma once

#include "core/runtime.hpp"
#include "game/dog_controller.hpp"
#include "game/gameplay_scenario.hpp"
#include "game/paddock_collision.hpp"
#include "game/sheep_spatial_grid.hpp"
#include "game/sheep_state.hpp"

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

[[nodiscard]] SheepState interpolate_sheep_state(const SheepState& previous,
                                                 const SheepState& current, double alpha) noexcept;

struct GameplaySnapshot {
    std::uint64_t tick = 0;
    DogState dog{};
    // How many entries of the seven fixed-capacity buffers below this snapshot
    // actually publishes. **Every reader iterates this, never `sheep.size()`**:
    // the buffers are sized at `kMaximumGameplaySheepCount` and the entries past
    // this count are default-constructed filler that no rule wrote and no
    // contract describes.
    //
    // It is published rather than looked up because it is the simulation's own
    // data: the scenario supplies the initial value and the tick carries it
    // forward, so a snapshot is self-describing and a later outcome that grows
    // or shrinks a flock during play changes what the next tick writes here
    // without changing the shape of anything.
    std::size_t sheep_count = 0;
    SheepStateBuffer sheep{};
    SheepSocialEvidenceBuffer sheep_social_evidence{};
    SheepDogPressureEvidenceBuffer sheep_dog_pressure_evidence{};
    SheepCollisionEvidenceBuffer sheep_collision_evidence{};
    SheepAvoidanceEvidenceBuffer sheep_avoidance_evidence{};
    SheepCombinedInfluenceEvidenceBuffer sheep_combined_influence_evidence{};
    SheepMotionLimitEvidenceBuffer sheep_motion_limit_evidence{};

    bool operator==(const GameplaySnapshot&) const = default;
};

// Owns the authoritative gameplay state advanced by the platform scheduler.
// Callers provide one input per fixed tick; render cadence and frame delta are
// deliberately absent from this API.
class GameplaySimulation {
  public:
    static constexpr std::uint32_t kTicksPerSecond = core::FixedStepAccumulator::ticks_per_second;
    static constexpr double kFixedDeltaSeconds = 1.0 / static_cast<double>(kTicksPerSecond);

    explicit GameplaySimulation(GameplayScenarioDefinition scenario) noexcept;

    void fixed_update(const GameplayTickInput& input) noexcept;
    void restart() noexcept;

    [[nodiscard]] const GameplaySnapshot& previous_snapshot() const noexcept;
    [[nodiscard]] const GameplaySnapshot& current_snapshot() const noexcept;
    [[nodiscard]] GameplaySnapshot interpolated_snapshot(double alpha) const noexcept;
    [[nodiscard]] const GameplayScenarioDefinition& scenario() const noexcept;
    [[nodiscard]] std::uint32_t restart_count() const noexcept;

  private:
    GameplayScenarioDefinition scenario_;
    DogController dog_;
    // Sheep rules read the same analytic paddock the dog motor collides with, so
    // occlusion, sheep collision, and dog collision can never disagree about
    // where a wall is.
    PaddockCollisionField paddock_;
    SheepSpatialGrid sheep_grid_;
    GameplaySnapshot previous_{};
    GameplaySnapshot current_{};
};

} // namespace wide_eye::game
