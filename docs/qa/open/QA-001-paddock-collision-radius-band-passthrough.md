---
id: QA-001
title: Analytic paddock collision passes a body that starts within its own radius of an obstacle face
status: open
severity: S2
confidence: confirmed
area: game
reporter: agent
reported: 2026-08-21
phase: 3
platform: wsl-ubuntu-24.04
rule: src/README.md
charter: play-session
verify:
  - wide_eye.gameplay_simulation
  - wide_eye.dog_controller
  - wide_eye.dog_closed_gate_scenario
  - label:unit
---

## Symptom

The analytic paddock stops a body only when the body starts at least one full
body radius clear of the obstacle face it is moving toward. A body that starts
inside that radius band is not stopped: it moves into the obstacle, through it,
and out the other side.

Observed result (2026-08-21, WSL Ubuntu 24.04.4, Clang 18.1.3, `dev` preset
sources compiled standalone with `clang++-18 -std=c++23 -I src`). Calling
`PaddockCollisionField::resolve_cylinder_move` with `radius = 0.5` against the
closed gate rectangle `x[14, 18], z[15, 16]`, displacement `-0.05` in `z`:

| start `z` | clearance to the `+z` face | resolved `z` | `clipped_z` | `obstacle` |
| --- | --- | --- | --- | --- |
| `16.0` | `0.0` (on the face) | `15.95` | `false` | `none` |
| `16.4` | `0.4` (less than the radius) | `16.35` | `false` | `none` |
| `16.5` | `0.5` (exactly the radius) | `16.5` | `true` | `gate` |
| `17.0` | `1.0` | `16.95` | `false` | `none` |

Reproduced in a named scenario, same build and date: `sheep-dog-facing-off` and
`sheep-dog-facing-on` place sheep 4 at exactly `(15, 16)`, on the closed gate's
`+z` face. Driving those fixtures with no dog input for 400 ticks, sheep 4 is
inside the closed gate rectangle on tick 0 and ends at `(17.4895, 12.6807)` and
`(18.0084, 11.9888)` respectively — clean through the closed gate and out the
far side.

## Investigation

The blocking test is in `move_axis` in
[`paddock_collision.cpp:56`](../../../src/game/paddock_collision.cpp#L56) and
[`paddock_collision.cpp:63`](../../../src/game/paddock_collision.cpp#L63).
Each branch requires the body to have started fully clear of the face:

- moving `+`: `desired > start && start + radius <= obstacle_minimum && desired + radius > obstacle_minimum`
- moving `-`: `desired < start && start - radius >= obstacle_maximum && desired - radius < obstacle_maximum`

Observed result: the `start ± radius` term is the defect. It asks whether the
body was already clear, so a body whose cylinder already overlaps the face's
radius band satisfies neither branch and is returned unclipped. There is no
depenetration path anywhere in the field: nothing pushes an overlapping body
back out, so once inside, the body keeps moving until it leaves on its own.

Inference: this behavior is inherited, not new. The rule predates the ownership
move in [ADR 0005](../../decisions/0005-paddock-collision-ownership.md) and was
accepted as dog motor behavior, where `DogController::kRadius` is `0.42`. Every
current dog fixture starts more than `0.42` from every obstacle face, which is
why it was never observed. It became reachable when sheep gained collision
authority at `kSheepCollisionRadius = 0.5`, because a sheep fixture does start
on a face.

Both call sites are affected — there are exactly two:
[`dog_controller.cpp:123`](../../../src/game/dog_controller.cpp#L123) and
[`gameplay_simulation.cpp:48`](../../../src/game/gameplay_simulation.cpp#L48).

The paddock's outer bounds are not affected: they are applied as an
unconditional `std::clamp` in `resolve_cylinder_move`, so a body outside them is
pulled back in rather than let through.

Unverified claim, stated as the next thing to check rather than as fact: because
the `x` axis resolves first and the `z` pass then uses the already-resolved `x`,
a corner move might place a body inside the radius band of the second axis
within a single tick, which would make the defect reachable without a fixture
starting on a face. This was not reproduced and no test covers it.

Test coverage: no CTest exercises a body starting inside the radius band. The
suite passes with the defect present. The fix must add that case rather than
rely on the existing wall/gate scenarios, all of which start clear.

## Root cause

`move_axis` decides whether to block using the body's *starting* clearance
(`start ± radius`) rather than whether the requested move would end with the
body's cylinder overlapping the obstacle. A body that begins inside the radius
band therefore matches no blocking branch, and the field has no depenetration
step to resolve the overlap it is in.

## Expected behavior

`src/README.md` and [ADR 0005](../../decisions/0005-paddock-collision-ownership.md)
make `PaddockCollisionField` the analytic collision truth for the paddock,
independent of voxel faces and renderer meshes. The Phase 3 item that granted
sheep this authority requires that "a wall or a closed gate physically stops a
driven sheep and the open gate is the only way through", and the Phase 3 exit
gate requires a recorded input sequence that moves all five sheep through the
gate. A closed gate that a body can occupy and traverse violates that
invariant regardless of where the body started.

## Fix notes

Scope: `move_axis` and `resolve_cylinder_move` in `src/game/paddock_collision.cpp`
only. The public API does not need to change; `CylinderMoveResult` already has
somewhere to report the outcome.

Blast radius is the reason this is filed rather than fixed inline. The rule is
shared by the dog motor and the sheep resolver, so any change to it changes
accepted dog collision behavior, which ADR 0005 and the Phase 3 checkpoint both
record as unchanged. Two candidate directions, both needing an owner decision:

1. Block on the *end* state (would the resolved cylinder overlap the obstacle)
   rather than the start state. Simple, but it traps a body that is already
   overlapping, which is worse than passing through for a spawn placed badly.
2. Keep the current rule and add an explicit depenetration step that pushes an
   overlapping body out along its shallowest axis before resolving the move.
   Better behavior, larger change, and it needs its own determinism argument for
   the tie between two equally shallow axes.

Whichever is chosen, the fix must add the missing coverage: a body starting on a
face, a body starting inside the band, and a body starting at exactly one radius
(which currently works and must keep working). It must also re-prove that
accepted dog behavior is unchanged, or record the change as a deliberate,
owner-accepted correction — `wide_eye.dog_controller`,
`wide_eye.dog_wall_scenario`, `wide_eye.dog_closed_gate_scenario`, and
`wide_eye.dog_open_gate_scenario` are the relevant suites, plus
`wide_eye.gameplay_simulation` for the sheep side. The `sheep-dog-facing-off/on`
fixtures place sheep 4 on the gate face; whether to move that sheep or keep it
as the regression witness is part of the fix decision.
