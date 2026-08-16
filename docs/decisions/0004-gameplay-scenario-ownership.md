# ADR 0004: Gameplay-owned scenarios and shared game math

**Status:** Accepted
**Date:** 2026-08-16
**Decision owner:** Project owner

## Context

The first dog/camera tracer introduced named deterministic scenarios inside the
dog-controller module. When the five-sheep presentation fixture arrived, that
dog-specific definition gained a sheep-motion flag, and the gameplay replay seed
stored a dog-scenario ID. The generic `Vec3` value type also lived in the dog
header. Those shortcuts were small initially, but the next roadmap outcomes add
sheep-only separation, temperaments, gate objectives, and recovery scenarios.
Continuing the shortcut would either put whole-game state into dog-owned types or
force a versioned replay migration after behavior work had already depended on
them.

## Decision

- `GameplayScenarioDefinition` and `GameplayScenarioId` are owned by `game` and
  identify the complete deterministic starting contract: version, seed, dog
  configuration, sheep fixture, and future objective/fixture configuration.
- `DogController` accepts only `DogControllerConfiguration`, currently the
  initial dog state and analytic gate state. It does not own scenario names,
  replay identity, sheep fixtures, or objective state.
- The scripted presentation motion is represented by an explicit
  `SheepFixture` value rather than a dog-scenario boolean.
- `Vec3` is a minimal shared game-math value in `game/math.hpp`; camera, dog,
  flock, replay, and spatial-query code may depend on it without depending on a
  particular controller.
- Existing scenario names, versions, seeds, and canonical replay JSON remain
  unchanged. This internal ownership correction does not reinterpret the
  version 1 seed/replay or version 2 state-dump formats.

## Consequences

- Named sheep and objective scenarios can grow without expanding the dog motor's
  responsibility.
- Replay validation continues to identify complete gameplay scenarios while
  preserving existing serialized values.
- The scenario layer is intentionally concrete, not a generic entity or plugin
  framework. Add fields only when an accepted gameplay outcome needs them.
- Moving or renaming serialized scenario values later still requires an explicit
  format/version decision and compatibility tests.
