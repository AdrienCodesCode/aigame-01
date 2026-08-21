# ADR 0005: Game-owned paddock collision field and gate state

**Status:** Accepted
**Date:** 2026-08-21
**Decision owner:** Project owner

## Context

The analytic paddock shapes — the two representative walls and the gate — were
introduced with the Tracer 1 dog motor and therefore lived in
`src/game/dog_controller.hpp` as `AnalyticObstacle` and
`PaddockCollisionField`. The paddock gate flag lived in
`DogControllerConfiguration` for the same historical reason, even though
[`ADR 0004`](0004-gameplay-scenario-ownership.md) had already moved whole-game
scenario state up to `GameplayScenarioDefinition`.

The next roadmap outcomes make sheep depend on those same shapes: dog line of
sight needs the walls and the gate as occluders, and the following item gives
sheep the same analytic collision authority the dog already has. Keeping the
shapes in a dog-named header would have made sheep rules include the dog motor
and read world state out of a controller's configuration, and it would have
allowed occlusion and collision to drift apart into two descriptions of the same
wall.

## Decision

- `AnalyticObstacle`, `PaddockCollisionField`, and the new `PaddockObstacle`
  identity live in a neutral game-owned `src/game/paddock_collision.hpp/.cpp`.
  The dog motor, the sheep rules, and any later system depend on that boundary
  rather than on each other.
- The obstacle set is identified rather than anonymous: every analytic shape
  carries a `PaddockObstacle` id, so a collision or occlusion result can name the
  left wall, the right wall, or the gate.
- `PaddockCollisionField` also answers a planar segment query,
  `blocking_obstacle`, so a sight line is tested against exactly the shapes that
  stop movement. The paddock cannot have one geometry for collision and another
  for visibility.
- Paddock gate state is world state. `GameplayScenarioDefinition::gate_open`
  owns it; `DogController` receives it as an explicit constructor argument, and
  `DogControllerConfiguration` retains only the initial dog state.
- Dog collision behavior, scenario names, scenario versions, seeds, and the
  serialized replay JSON are unchanged by this correction. Only the state-dump
  contract advances, and only because the new line-of-sight evidence is
  published in the same outcome.

## Consequences

- Sheep rules can read the paddock without depending on a dog-owned boundary,
  and the later sheep-collision item can reuse the same field rather than
  duplicating the shapes.
- `GameplaySimulation` now owns a `PaddockCollisionField` beside the dog motor.
  Both are constructed from the same scenario gate flag, so they cannot disagree.
- Every `DogController` construction site must state the gate explicitly. That
  is intentional: a defaulted gate would silently pick closed-gate collision.
- The obstacle set stays a concrete fixed array for the handcrafted paddock. It
  is not a general collision-shape registry, and it does not model voxel faces or
  renderer meshes.
