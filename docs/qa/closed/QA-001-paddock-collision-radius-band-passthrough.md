---
id: QA-001
title: Analytic paddock collision passes a body that starts within its own radius of an obstacle face
status: fixed
severity: S2
confidence: confirmed
area: game
reporter: agent
reported: 2026-08-21
phase: 3
platform: wsl-ubuntu-24.04
rule: src/README.md
charter: play-session
closed: 2026-08-22
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

## Resolution

Fixed on 2026-08-22 by restoring the precondition `move_axis` was written
against rather than by changing its refusal arithmetic:
`PaddockCollisionField::resolve_cylinder_move` now pushes a body that starts
overlapping an obstacle out along the shallowest single-axis move that clears
every shape at once and stays inside the paddock, then resolves the requested
displacement exactly as before. The decision, the rejected end-state
alternative, the tie break, and the no-escape case are recorded as a dated
correction in
[ADR 0005](../../decisions/0005-paddock-collision-ownership.md#correction--a-body-that-starts-inside-a-shape-2026-08-22-qa-001).

Direction 1 (block on the end state) was rejected on measurement, not on taste.
An exhaustive sweep of the field showed the start-clearance test is not wrong —
its precondition was simply unmet: over 5,960,704 (start, displacement) pairs on
a `641 × 641` grid, **no start clear of every shape was ever left overlapping
one**, so the passes never produce the state they cannot answer for. Testing the
end state instead would also have made an overlapping body's correction depend
on which way it asked to move — the same body at the same place thrown to
opposite faces by its velocity, and a body requesting no motion at all thrown a
full body diameter.

The tie break is the enumeration order: the field's own fixed obstacle order,
then a fixed `-x`, `+x`, `-z`, `+z` face order, with only a strictly smaller
depth replacing the standing candidate. Two equally shallow ways out therefore
resolve to the earlier shape when they belong to different shapes and to the X
face when one shape offers both — the same X-before-Z priority the X-first
resolve pass and `approaching_obstacle`'s corner tie already use. It depends on
nothing but the geometry and that fixed order: not storage order, not iteration
order, not an address, not which body is asking. A body no candidate can free is
left where it is; the geometry names no escape, so none is invented.

Rejecting any candidate that lands inside another shape is what makes the
correction a single step. The walls and the closed gate touch, so the shallowest
way out of one rectangle is routinely a way into its neighbour; an iteration
taking them in turn could hand a body back and forth forever.

No published field, contract version, scenario name, seed, or replay JSON
changed. Being pushed out of a shape is reported through the existing
`clipped_x` / `clipped_z` and `obstacle` rather than through a new signal,
because it is the event those already describe — the requested coordinate was
inside an obstacle and the field refused it — and because the caller's response
to a clipped axis (clearing that axis' velocity) is the right response to being
pushed out of a wall.

### Evidence

Observed result, 2026-08-22, WSL Ubuntu 24.04.4 x86-64. `dev` and
`dev-sanitized` are Clang 18.1.3; `release` is GNU 13.3.0 (see QA-004). Before
values come from a `git worktree` at `5e0a6fa`; standalone builds are
`clang++-18 -std=c++23 -I src`. This host exposes only OpenGL 4.5, so no capture
or native-4.6 evidence was produced and none is claimed.

**The reported probe table** — radius `0.5`, closed gate `x[14, 18]`,
`z[15, 16]`, displacement `-0.05` in `z`, start `x = 15`:

| start `z` | before | after |
| --- | --- | --- |
| `16.0` (on the face) | `15.95`, `clipped_z` false, `none` | `16.5`, `clipped_z` true, `gate` |
| `16.4` (inside the band) | `16.35`, `clipped_z` false, `none` | `16.5`, `clipped_z` true, `gate` |
| `16.5` (exactly one radius) | `16.5`, `clipped_z` true, `gate` | `16.5`, `clipped_z` true, `gate` |
| `17.0` (a radius clear) | `16.95`, `clipped_z` false, `none` | `16.95`, `clipped_z` false, `none` |

The two already-correct rows are byte-identical before and after.

**The named-scenario reproduction** — `sheep-dog-facing-off` and
`sheep-dog-facing-on`, 400 ticks with no dog input, sheep 4:

| | before | after |
| --- | --- | --- |
| `sheep-dog-facing-off` final | `(17.4895, 12.6807)` | `(20.3843, 16.5)` |
| `sheep-dog-facing-on` final | `(18.0084, 11.9888)` | `(21.4561, 16.5)` |
| minimum published `z` | `12.6807` / `11.9888` | `16.5` / `16.5` |
| ticks with the body inside the gate | `207` / `174` | `0` / `0` |

Sheep 4 is pushed off the gate's north face on tick `0` to exactly `16.5` — the
face at `16` plus its `0.5` body radius, the same rest position the clip already
produced for a sheep arriving from clear space — loses its `z` velocity under
the accepted contact rule, slides along the face under dog pressure, and is
carried east and north. It never reaches the south side of the wall line.

**Exhaustive analytic sweep**, `641 × 641` starts from `0.5` to `31.5` and 16
per-tick displacements up to the dog's sprint step, at radius `0.5` and `0.42`,
with the gate open and closed. After the fix, in all four configurations: no
clear start ends overlapping, **no overlapping start ends overlapping** (before:
`603086` of `613392` in the closed-gate sheep configuration), every clipped axis
is still explained by a named obstacle or the paddock's outer bounds, no position
is wedged with no way out (before: `38337` positions the field could not free),
and every corrected position is a fixed point, so a corrected body cannot jitter
on the next tick.

**Per-scenario comparison** against the `5e0a6fa` worktree: all 30 named
scenarios, 240 scripted ticks each under the shared moving-dog route, canonical
state dump every tick. **28 of 30 are byte-identical.** The two that differ are
`sheep-dog-facing-off` and `sheep-dog-facing-on`, both first at tick `0`, and in
both every differing field belongs to sheep 4 — the one standing on the gate. Its
accepted first-tick facing evidence is unchanged, including the exact `0.8`
cosine and the `(0.12, -0.16)` facing vector, because that evidence is read from
immutable prior state before the collision stage runs. Every dog scenario is
byte-identical, so accepted dog collision behavior is unchanged in fact.

**Coverage added.** No CTest exercised a body starting inside the band, which is
why the suite passed with the defect present. `wide_eye.gameplay_simulation` now
carries a QA-001 oracle covering a body on a face, inside the band, at exactly
one radius, fully inside a shape, at a corner where two ways out are equally
shallow within one shape, at a corner where two shapes tie, and wedged between
two shapes — at both body radii — plus the fixed-point requirement and a 400-tick
run of both witness fixtures asserting that no published sheep ever occupies a
paddock shape. Its published numbers are
`paddock_band_passthrough_on_face_z=16.5`,
`paddock_band_passthrough_at_one_radius_z=16.5`,
`paddock_band_passthrough_a_radius_clear_z=16.95`,
`paddock_band_passthrough_fully_inside=16,14.5`,
`paddock_band_passthrough_axis_tie=0.5,14.5`,
`paddock_band_passthrough_shape_tie=18.25,16.5`,
`paddock_band_passthrough_two_shape_wedge=18,13.5`,
`paddock_band_passthrough_dog_at_one_radius_z=16.42`, and
`paddock_band_passthrough_witness_overlap_ticks=0`.

One existing check was adapted rather than weakened:
`published_facing_term_matches_bounded_applied_acceleration` compared the first
tick's velocity change against the published terms for every sheep, which
silently assumed no sheep contacts anything on tick 1. Sheep 4 now does. The
check now requires an axis the paddock refused to publish exactly the accepted
contact rule's zero, and still requires every unrefused axis to match the
published terms exactly.

**The witness fixture is kept.** Sheep 4 stays at `(15, 16)`, on the gate face.
It is the only named fixture that starts a body inside an obstacle, so it is the
regression witness by name; and it cannot be moved without moving the accepted
first-tick `0.8` facing cosine it carries, because every 3-4-5 offset from that
dog which produces that cosine lies on the paddock's own wall line. A comment on
`kDogFacingSheepStates` records this so it is not "tidied up" later.

**Commands run**, all on WSL Ubuntu 24.04.4 x86-64 on 2026-08-22:

| Command | Result |
| --- | --- |
| `cmake --build --preset dev && ctest --preset dev` | 28/28 passed |
| `cmake --build --preset release && ctest --preset release` | 28/28 passed |
| `cmake --build --preset dev-sanitized && ctest --preset dev-sanitized` | 28/28 passed |
| `cmake --build --preset dev --target format-check` | passed |
| `cmake --build --preset dev --target clang-tidy-check` | passed, exit 0 |
| `cmake -DMODE=check -P tools/qa/qa-tracker.cmake` | `QA tracker check passed` |
| `git diff --check` | no output, exit 0 |
| `ctest --preset dev -R wide_eye.gameplay_simulation_stack_budget` | 1/1 passed |

The issue's own `verify` entries — `wide_eye.gameplay_simulation`,
`wide_eye.dog_controller`, `wide_eye.dog_closed_gate_scenario`, and
`label:unit` — are all inside those runs and all passed.

Artifacts (gitignored):
`artifacts/phase3/2026-08-22/qa-001-collision-passthrough/probe-table.txt`,
`analytic-sweep.txt`, `sheep4-reproduction.txt`, `scenario-state-diff.txt`,
`gameplay-simulation-oracle.txt`, and `verification.txt`.

### The Investigation's open question, answered

The Investigation left one unverified claim: that a corner move might place a
body inside the second axis' radius band within a single tick, making the defect
reachable without a fixture starting on a face. **It does not.** The exhaustive
sweep above measured exactly this: over 5,960,704 (start, displacement) pairs at
radius `0.5` on the closed-gate paddock — and the same again at the dog's `0.42`
and with the gate open — a start clear of every shape was left overlapping one
`0` times, before the fix as well as after it. The X-then-Z pass ordering is
sound; the only way into the band was a caller putting a body there. That
measurement is what made the depenetration direction the right one rather than a
rewrite of the refusal test.

The `paddock_collision.cpp:56` / `:63` line references in the Investigation point
at the pre-fix file as it stood at `5e0a6fa`; the refusal branches they name are
unchanged in content but have moved down the file.

### What is deliberately not covered

- **How the correction feels.** A badly placed body is moved up to one body
  radius in a single tick. No player or reviewer has seen that, and this host
  cannot capture it.
- **Paddocks other than the handcrafted one.** The single-step guarantee and the
  absence of a wedged position are measured against these three rectangles. The
  code answers a body it cannot free by leaving it where it is, which is
  predictable rather than proven unnecessary for a shape set that does not exist
  yet.
- **QA-005** is untouched: the avoidance response shape is still bang-bang near
  a face, and this fix does not change any steering term.
