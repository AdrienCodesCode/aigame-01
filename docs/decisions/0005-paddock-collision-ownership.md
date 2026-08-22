# ADR 0005: Game-owned paddock collision field and gate state

**Status:** Accepted; behavior for a body that starts inside a shape corrected
2026-08-22 (see
[Correction — a body that starts inside a shape](#correction--a-body-that-starts-inside-a-shape-2026-08-22-qa-001))
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

## Correction — a body that starts inside a shape (2026-08-22, QA-001)

This section corrects the decision above rather than restating it. It was written
after [QA-001](../qa/closed/QA-001-paddock-collision-radius-band-passthrough.md)
measured the field as accepted and found it letting a body walk through the
closed gate.

**What was wrong.** `resolve_cylinder_move` refuses a displacement by asking
whether the body was clear of the face *before* the move — `start ± radius`
against the obstacle's own limit. That test only answers correctly for a body
that starts clear. A body that starts inside an obstacle's radius band matched
neither the `+` nor the `-` branch, so it was returned unclipped: it moved into
the shape, through it, and out the far side, and nothing anywhere in the field
pushed an overlapping body back out. With the sheep body radius of `0.5` against
the closed gate, a body starting exactly on the gate's north face at `z = 16.0`
moved to `15.95` with `clipped_z` false and no named obstacle, while the same
body one radius clear at `16.5` was correctly held. `sheep-dog-facing-off` and
`sheep-dog-facing-on` place sheep 4 at exactly `(15, 16)`, and driven with no dog
input for 400 ticks that sheep ended at `(17.4895, 12.6807)` and
`(18.0084, 11.9888)` — through the closed gate and out the south side.

The defect predates this decision. It was accepted dog-motor behavior, where
`DogController::kRadius` is `0.42` and no fixture starts inside the band; it
became reachable when sheep gained the same authority at
`kSheepCollisionRadius = 0.5` and one fixture did.

**What changed, and why it is a precondition rather than a new rule.** The
refusal arithmetic is untouched. What the field now does first is restore the
precondition that arithmetic was written against: a body that starts overlapping
an obstacle is pushed out before its displacement is resolved. Measured over
5,960,704 start-and-displacement pairs on a `641 × 641` grid across the paddock,
at both body radii and both gate states, **no start that is clear of every shape
is ever left overlapping one** — the passes never feed themselves a body they
cannot answer for, so the only source of an overlap is a caller placing one
there. That measurement is why the correction is a step in front of the rule
rather than a different rule.

The alternative the issue named — testing the *end* state instead of the start —
was rejected on that same evidence. The start test is not wrong; its precondition
was unmet. Blocking on the end state would additionally have made an already
overlapping body's correction depend on which way it asked to move: the same body
at the same place inside a shape would be thrown to opposite faces depending on
its velocity, and a body that requested no motion at all would be thrown a full
body diameter by an axis test that was never meant to answer for it.

**The push, and the tie break.** The correction is the *shallowest single-axis
move that puts the body outside every shape at once and leaves it inside the
paddock*. Answering for the union rather than for one rectangle at a time is
what makes it a single step: the walls and the closed gate touch, so the
shallowest way out of one shape is routinely a way into its neighbour, and an
iteration that took those in turn could hand a body back and forth between two
neighbours forever. Candidates are enumerated in the field's own fixed obstacle
order, then in a fixed `-x`, `+x`, `-z`, `+z` face order, and only a *strictly*
smaller depth replaces the standing candidate. Two equally shallow ways out
therefore always resolve to the earlier one — the earlier shape when they belong
to different shapes, and the X face when one shape offers both, which is the same
X-before-Z priority the X-first resolve pass and
[ADR 0008](0008-obstacle-and-drop-avoidance.md)'s corner tie already use. Nothing
in that order depends on storage order, iteration order, an address, or which
body is asking, so an exact tie answers the same way in every run. It is the same
kind of tie break as the antisymmetric stable-ID direction the close-range
separation rule uses at exact overlap: a fixed rule where the geometry names
none, rather than a hidden one.

**A body with no way out is left where it is.** If no candidate clears every
shape and stays inside the paddock, the geometry names no escape and none is
invented; the ordinary passes then resolve the displacement exactly as they did
before. That case does not arise in this paddock — the same exhaustive sweep
found zero positions at either radius, with the gate open or closed, from which
the field could not free a body in one step, and zero positions where applying
the resolve twice moved the body a second time. A body the field has corrected is
a fixed point, so a resting body cannot jitter.

**"Overlapping" is the strict test the rest of the field already uses.** A body
parked at face plus radius — exactly where the clip leaves it — does not overlap
and is not pushed. That is the same strictness ADR 0008's correction pinned for
the look-ahead sweep, and it is what keeps the clip's own output from being
corrected again on the next tick.

**No new published signal, and no contract version change.** Being pushed out of
a shape is reported through the existing `clipped_x` / `clipped_z` and
`obstacle`, because it is the event those already describe: the requested
coordinate was inside an obstacle and the field refused it. The caller's response
to a clipped axis — clearing that axis' velocity — is the right response here
too, since a body being pushed out of a wall should not keep the speed it had
into the wall. A separate flag would be a contract member carried forever for an
event that only a badly placed fixture can produce. Measured on the same sweep,
every clipped axis is still explained by either a named obstacle or the paddock's
outer bounds, so the documented meaning of a clipped axis with `none` is intact.

**What did not change.** No published field, contract version, scenario name,
seed, magnitude, or replay JSON. Measured on WSL Ubuntu 24.04.4 with Clang 18.1.3
against a `git worktree` at `5e0a6fa`, the canonical state dumps of 28 of the 30
named scenarios are byte-identical over 240 scripted ticks. The two that differ
are `sheep-dog-facing-off` and `sheep-dog-facing-on`, both from tick 0, and in
both the only fields that ever differ belong to sheep 4 — the one standing on the
gate. Its accepted first-tick evidence is unchanged, including the exact `0.8`
facing cosine and the `(0.12, -0.16)` facing vector, because that evidence is
computed from immutable prior state before the collision stage runs. Every dog
scenario is byte-identical, so accepted dog collision behavior is unchanged in
fact rather than by assertion.

**The fixture stays where it is.** Sheep 4's placement on the gate face is kept
deliberately, as the named witness for this defect: with the fault present it
walked through the closed gate, so any return of the fault changes those two
scenarios by name. It also cannot be moved without moving the accepted facing
measurement it carries, because every 3-4-5 offset from that dog which produces a
`0.8` cosine lies on the paddock's own wall line.
