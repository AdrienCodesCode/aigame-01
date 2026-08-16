#include "game/gameplay_simulation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace wide_eye::game {
namespace {

constexpr SheepStateBuffer kInitialSheepStates{{
    {.id = 1,
     .position = {.x = 14.5, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 2,
     .position = {.x = 16.0, .y = 1.0, .z = 19.5},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 3,
     .position = {.x = 17.5, .y = 1.0, .z = 20.0},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 4,
     .position = {.x = 15.25, .y = 1.0, .z = 21.5},
     .heading_radians = 0.0,
     .grounded = true},
    {.id = 5,
     .position = {.x = 16.75, .y = 1.0, .z = 21.5},
     .heading_radians = 0.0,
     .grounded = true},
}};

void advance_sheep_from_prior(const SheepStateBuffer& prior, SheepStateBuffer& next,
                              std::uint64_t tick, bool presentation_only_motion) noexcept {
    constexpr std::uint64_t kTicksPerLeg = 60;
    constexpr std::uint64_t kLegCount = 4;
    constexpr double kFixtureSpeed = 1.5;
    constexpr double kHalfPi = 1.57079632679489661923;
    constexpr double kPi = 3.14159265358979323846;
    constexpr std::array<Vec3, kLegCount> kVelocities{{
        {.z = -kFixtureSpeed},
        {.x = kFixtureSpeed},
        {.z = kFixtureSpeed},
        {.x = -kFixtureSpeed},
    }};
    constexpr std::array<double, kLegCount> kHeadings{{0.0, kHalfPi, kPi, -kHalfPi}};

    // Both modes keep the explicit prior-to-next pass so later behavior cannot
    // accidentally acquire update-order dependence. The named motion fixture
    // is scripted presentation evidence, not a social-response implementation.
    for (std::size_t index = 0; index < prior.size(); ++index) {
        next[index] = prior[index];
        if (!presentation_only_motion) {
            continue;
        }

        const std::size_t leg = static_cast<std::size_t>((tick / kTicksPerLeg) % kLegCount);
        next[index].velocity = kVelocities[leg];
        next[index].position.x += kVelocities[leg].x * GameplaySimulation::kFixedDeltaSeconds;
        next[index].position.z += kVelocities[leg].z * GameplaySimulation::kFixedDeltaSeconds;
        next[index].heading_radians = kHeadings[leg];
    }
}

} // namespace

SheepState interpolate_sheep_state(const SheepState& previous, const SheepState& current,
                                   double alpha) noexcept {
    constexpr double kTwoPi = 6.28318530717958647692;
    const double bounded_alpha = std::isfinite(alpha) ? std::clamp(alpha, 0.0, 1.0) : 1.0;
    const auto interpolate = [bounded_alpha](double start, double end) {
        return start + (end - start) * bounded_alpha;
    };
    return {
        .id = current.id,
        .position = {.x = interpolate(previous.position.x, current.position.x),
                     .y = interpolate(previous.position.y, current.position.y),
                     .z = interpolate(previous.position.z, current.position.z)},
        .velocity = {.x = interpolate(previous.velocity.x, current.velocity.x),
                     .y = interpolate(previous.velocity.y, current.velocity.y),
                     .z = interpolate(previous.velocity.z, current.velocity.z)},
        .heading_radians = std::remainder(
            previous.heading_radians +
                std::remainder(current.heading_radians - previous.heading_radians, kTwoPi) *
                    bounded_alpha,
            kTwoPi),
        .arousal = interpolate(previous.arousal, current.arousal),
        .behavior = current.behavior,
        .grounded = current.grounded,
    };
}

GameplaySimulation::GameplaySimulation(DogScenarioDefinition scenario) noexcept : dog_{scenario} {
    current_.dog = dog_.state();
    current_.sheep = kInitialSheepStates;
    previous_ = current_;
}

void GameplaySimulation::fixed_update(const GameplayTickInput& input) noexcept {
    WIDE_EYE_ASSERT(current_.tick < std::numeric_limits<std::uint64_t>::max(),
                    "authoritative gameplay tick overflow");

    previous_ = current_;
    if (input.dog_move.has_value()) {
        dog_.fixed_update(*input.dog_move, kFixedDeltaSeconds);
    }
    current_.tick += 1;
    current_.dog = dog_.state();
    advance_sheep_from_prior(previous_.sheep, current_.sheep, previous_.tick,
                             scenario().presentation_only_sheep_motion);
}

void GameplaySimulation::restart() noexcept {
    dog_.restart();
    current_ = {.tick = 0, .dog = dog_.state(), .sheep = kInitialSheepStates};
    previous_ = current_;
}

const GameplaySnapshot& GameplaySimulation::previous_snapshot() const noexcept {
    return previous_;
}

const GameplaySnapshot& GameplaySimulation::current_snapshot() const noexcept {
    return current_;
}

GameplaySnapshot GameplaySimulation::interpolated_snapshot(double alpha) const noexcept {
    GameplaySnapshot result{
        .tick = current_.tick,
        .dog = interpolate_dog_state(previous_.dog, current_.dog, alpha),
        .sheep = current_.sheep,
    };
    for (std::size_t index = 0; index < result.sheep.size(); ++index) {
        result.sheep[index] =
            interpolate_sheep_state(previous_.sheep[index], current_.sheep[index], alpha);
    }
    return result;
}

const DogScenarioDefinition& GameplaySimulation::scenario() const noexcept {
    return dog_.scenario();
}

std::uint32_t GameplaySimulation::restart_count() const noexcept {
    return dog_.restart_count();
}

} // namespace wide_eye::game
