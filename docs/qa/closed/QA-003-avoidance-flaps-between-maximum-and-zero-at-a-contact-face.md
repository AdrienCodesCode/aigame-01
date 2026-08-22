---
id: QA-003
title: Obstacle avoidance alternates between its maximum and zero for a sheep in contact with a wall face
status: fixed
severity: S2
confidence: confirmed
area: game
reporter: agent
reported: 2026-08-22
phase: 3
platform: wsl-ubuntu-24.04
closed: 2026-08-22
fix: edc2548
rule: docs/decisions/0008-obstacle-and-drop-avoidance.md
verify:
  - wide_eye.gameplay_simulation
  - wide_eye.gameplay_simulation_stack_budget
  - label:unit
  - label:sanitizer
---

## Symptom

A sheep that is held against a paddock wall face and sliding along it publishes
an obstacle-avoidance acceleration that alternates between the term's full
maximum and exactly zero on consecutive ticks, and the applied acceleration
reverses direction with it. The published vector on the maximum ticks points
*backwards along the sheep's own direction of travel*, away from a face the
sheep passed some ticks earlier.

Observed result (2026-08-22, WSL Ubuntu 24.04.4, Clang 18.1.3, `dev` preset).
Found by the randomness-and-steering-stability oracle in
`wide_eye.gameplay_simulation`, in the new `sheep-all-influences-diagnostic`
scenario driven by the dog for 600 ticks. Sheep 2 is pressed north onto the
right wall by dog pressure, comes to rest at `z = 16.5` — the wall's `+z` face
at `16.0` plus the `0.5` sheep body radius — and then slides east along it:

| tick | prior `z` | prior velocity | avoidance vector | named obstacle | applied acceleration | clipped `z` |
| --- | --- | --- | --- | --- | --- | --- |
| 130 | `16.5` | `(2.0231, 0)` | `(-2.8284, +2.8284)`, magnitude `4.0` | `right_wall` at `0.0` | `(0.013, 0.284)` | no |
| 131 | `16.500079` | `(2.0234, +0.0047)` | `(0, 0)` | none | `(2.764, -2.438)` | yes |
| 132 | `16.5` | `(2.0694, 0)` | `(-2.8284, +2.8284)`, magnitude `4.0` | `right_wall` at `0.0` | `(-0.170, 0.504)` | no |
| 133 | `16.500140` | `(2.0666, +0.0084)` | `(0, 0)` | none | `(2.570, -2.223)` | yes |

The sheep is travelling `+x` at about `2.1` world units/s throughout, so the
`-x` component of the maximum-magnitude ticks is a full-strength brake pointing
backwards, and the applied acceleration reverses on every one of these ticks.
The two states alternate because they cause each other: on a tick when
avoidance fires, its `+z` half pushes the sheep off the boundary, so the next
tick sees nothing ahead, dog pressure pushes the sheep back north onto the wall,
the paddock refuses the `z` axis and clears that velocity component, and the
sheep is back at exactly `16.5` with `v_z` exactly `0`. The alternation runs for
23 consecutive ticks.

Measured over the whole 600-tick run, worst sheep of five, as the harness now
reports it:

```
sheep_steering_stability_avoidance_worst_sheep_flaps=90
sheep_steering_stability_avoidance_worst_sheep_flap_run=23
sheep_steering_stability_applied_worst_sheep_flaps=77
sheep_steering_stability_applied_worst_sheep_flap_run=17
```

A flap is one tick on which a term switched itself on or off, or reversed
direction while both ticks were live. `90` flaps in `600` ticks is `15.0` per
hundred; the six continuous terms in the same run peak at `2.67` per hundred
with a longest run of `3`.

Reproduce:

```bash
cmake --build --preset dev
ctest --preset dev -R wide_eye.gameplay_simulation
./build/Linux/dev/wide_eye_gameplay_simulation_tests | grep sheep_steering_stability_avoidance
```

## Investigation

Observed result, same build, date, and platform. The geometry query is exact and
reproduces outside the simulation. Compiling `paddock_collision.cpp` standalone
(`clang++-18 -std=c++23 -I src`) and calling
`PaddockCollisionField::approaching_obstacle` with the closed-gate field, the
sheep's own `6.25` look-ahead and `0.5` radius, from `x = 19.355` travelling
`+x`:

| start `z` | direction | obstacle | contact distance | face normal | lateral escape |
| --- | --- | --- | --- | --- | --- |
| `16.5` | `(1, 0)` | `right_wall` | `0.0` | `(-1, 0)` | `(0, +1)` |
| `16.500000001` | `(1, 0)` | `none` | `0.0` | `(0, 0)` | `(0, 0)` |
| `16.5` | `(1, +0.0025)` | `right_wall` | `0.0` | `(-1, 0)` | `(0, +1)` |
| `16.51` | `(1, 0)` | `none` | `0.0` | `(0, 0)` | `(0, 0)` |

A billionth of a world unit of separation from the face flips the answer between
"touching this obstacle at zero distance" and "nothing ahead at all", and the
`4.0` maximum the caller derives from it flips with it.

The mechanism is in two accepted pieces that meet badly:

- [`paddock_collision.cpp:263`](../../../src/game/paddock_collision.cpp#L263)
  clamps a negative entry fraction to zero, deliberately: "a body whose swept
  rectangle already overlaps this obstacle reaches it at zero distance rather
  than at a negative one. Pushing that body out is a steering caller's decision;
  this query only reports the geometry." A sheep resting at face-plus-radius is
  exactly on the closed boundary of the radius-expanded rectangle, so
  `slab_span` reports the `z` axis as always overlapping and the `x` axis as
  entered at `-1.855` — behind the sheep — which the clamp turns into a contact
  at distance `0`.
- [`gameplay_simulation.cpp:637`](../../../src/game/gameplay_simulation.cpp#L637)
  is the steering caller, and its decision is the linear falloff
  `urgency = 1 - contact_distance / look_ahead`. At a contact distance of `0`
  that is `1.0`, the term's full maximum.

The published direction follows from the same overlap: with `unit_z == 0` the
`z` slab span is infinite, so the `x` slab is "entered last" and
[`paddock_collision.cpp:275`](../../../src/game/paddock_collision.cpp#L275)
names the `-x` face — the one at `x = 17.5`, which the sheep passed `1.9` units
ago — as the face it meets. The `+z` free edge is at clearance `0`, well inside
the look-ahead, so the lateral escape is added and the result is the exact
`(-1, +1)/√2 × 4.0` seen above.

Any influence that presses a sheep onto a face sustains the cycle; the dog
pressure term happens to be the one that does it here.

Second mechanism, same class, also observed in the same run: the drop half is
binary by design — "the ground under the look-ahead point either exists or does
not" — so a sheep sitting near the look-ahead's reach of the paddock edge
toggles `drop_ahead` and with it a `4.0` push. Sheep 4 toggled the flag 20 times
and sheep 5 16 times in the same 600 ticks; at tick 370 sheep 4's published
vector went from `(-3.890, -0.931)` to `(2.733, 2.733)` and back on the
following tick.

Inference: this is inherited from the accepted rule rather than introduced by
the fixture. Nothing about the diagnostic scenario is special except that it is
the first fixture in which a sheep is pressed against a face by a live influence
while still moving along it.

Test coverage, and a finding in its own right: the accepted paired
`sheep-avoidance-off` / `sheep-avoidance-on` fixtures cannot see this. Their
sheep approach faces head-on and the term brakes them to a stop `0.758357` short
of the wall, so no sheep in them ever reaches contact. The suite passed with the
defect present until this outcome measured it. `wide_eye.gameplay_simulation`
now measures the flap rate and holds avoidance and the applied sum to a
**recorded allowance of 20 per hundred ticks with a run of 25**, deliberately
looser than the `5` and `4` the six continuous terms are held to, and names this
issue as the reason. Closing this issue should tighten both to the continuous
allowance.

## Root cause

`approaching_obstacle` reports a body that is merely touching the boundary of a
radius-expanded obstacle rectangle as contacting that obstacle at distance zero,
and names as "the face it meets" the perpendicular face whose slab was entered
behind the body. `apply_avoidance` then converts a contact distance of zero into
the term's full maximum through its linear falloff. Together they make the term
discontinuous at the exact position the collision authority parks a sheep at, so
a sheep held on a face alternates between the maximum and nothing.

## Expected behavior

[ADR 0008](../../decisions/0008-obstacle-and-drop-avoidance.md) and
[`src/README.md`](../../../src/README.md) both state the accepted rule as
direction-aware rather than proximity-aware: "a sheep standing beside a wall or
running parallel to one feels nothing at all", and the accepted fixture pins that
with a sheep running parallel `3.5` from the wall line whose published vector is
exactly zero. A sheep running parallel to a face while touching it is the same
case geometrically and must not receive the term's maximum, and must certainly
not receive it directed backwards along its own travel.

ADR 0008 also makes continuity an explicit property of the far boundary: "a
shape exactly at the look-ahead distance therefore publishes a named obstacle and
a zero vector, so the boundary is continuous rather than a step." The near
boundary currently has the opposite property, and nothing recorded that.

## Fix notes

Scope: `apply_avoidance` in `src/game/gameplay_simulation.cpp`, and possibly
`approaching_obstacle` in `src/game/paddock_collision.cpp`. No published field,
contract version, or scenario has to change; the evidence record already carries
`obstacle`, `obstacle_distance`, and the vector.

Blast radius is why this is filed rather than fixed inline. `approaching_obstacle`
has exactly one caller — the sheep avoidance term — so the dog motor is not
affected, but the accepted `sheep-avoidance-off`/`-on` numbers in
[ADR 0008](../../decisions/0008-obstacle-and-drop-avoidance.md) and in the Phase 3
checkpoint are exact and must be re-proved unchanged, or the change recorded as a
deliberate owner-accepted correction.

Candidate directions, each needing an owner decision:

1. Make the query answer the question the caller asks: report the face the body
   is *moving toward*, not the last slab entered, and report no obstacle when the
   travel direction has no component into the named face. This is the smallest
   change that matches the documented "running parallel feels nothing".
2. Keep the query and give the caller the decision the query's comment says is
   the caller's: treat an overlap or grazing contact as a depenetration case with
   its own bounded response rather than as maximum urgency along a look-ahead.
3. Bound how fast the term may change between ticks — a rate limit or a hold —
   which suppresses the alternation without addressing either cause. Cheapest,
   and the one most likely to hide the next instance; listed for completeness.

Whichever is chosen, the fix must add the missing coverage: a sheep in contact
with a face while travelling parallel to it, a sheep a fraction of a unit clear
of the same face, and a sheep on the drop boundary. It must also tighten the
avoidance and applied-sum flap allowances in the stability oracle to the
continuous-term values
and re-run `wide_eye.gameplay_simulation`, which is where the regression would
show.

## Resolution

Fixed on 2026-08-22 in `src/game/paddock_collision.cpp` and its header — the
query, not the caller. Two corrections, neither touching the hard collision
authority and neither changing a published field, contract version, scenario, or
magnitude:

- `approaching_obstacle` now requires the contact to lie **at or ahead of** the
  body. The `std::max(entry, 0.0)` clamp is gone, so a body already inside an
  expanded rectangle reports no obstacle instead of a contact at distance zero
  through whichever slab it entered last — which, for a body travelling along a
  face, was a face it had already passed.
- `slab_span` now treats a zero-direction axis strictly: touching a rectangle's
  boundary while travelling along it is not being inside it. That is the same
  convention `overlaps` already applies in `resolve_cylinder_move` for the axis a
  body is not moving along, and it is what lets the clip park a body at face plus
  radius and then let it slide.

A contact distance of zero now means the body is touching the face it is moving
into, and the falloff's maximum is the right answer to it: it points out of that
face and it is the *same* answer on every tick the sheep keeps pressing, so the
response is stable rather than alternating. A sheep travelling along a face it
touches gets nothing, which is ADR 0008's accepted "running parallel feels
nothing" applied at zero clearance. Any nonzero answer for that case was rejected
on measurement: it moves the body off the contact, the term then switches off,
and the alternation returns. ADR 0008 carries the reasoning as a dated correction
section.

Evidence — all on WSL Ubuntu 24.04.4, `dev` and `dev-sanitized` Clang 18.1.3,
`release` GNU 13.3.0 (QA-004):

- **Reproduction, before.** A new fixture puts three sheep at the wall line —
  two exactly on it, one a hundredth of a unit clear — with a stationary dog
  pressing them onto it. Built against `HEAD` (`12700d0`) with
  `clang++-18 -std=c++23 -I src`, the sheep held on the line flapped **81 times
  in 240 ticks**, including the reported symptom exactly: prior `z = 16.5`,
  prior velocity `(1.038291, 0)`, `right_wall` at distance `0`, published vector
  `(-2.828427, +2.828427)` — the full maximum pointing backwards along its own
  travel. Same fixture with the fix: **0 flaps**.
- **Geometry, before and after.** Standalone probes of `approaching_obstacle`
  from the accepted clip line: on the line travelling `+x`, `right_wall` at
  `-0.000000` with normal `(-1, 0)` before, `none` after; on the line turning
  into the face, `right_wall` at `0` with normal `(0, +1)` both before and after;
  `0.5` clear reports `0.5` and `6.25` clear reports `6.25` in both.
- **Stability oracle**, 600-tick `sheep-all-influences-diagnostic` run, worst
  sheep: avoidance `90` flaps with a run of `23` → **`27` with a run of `10`**;
  applied sum `77`/`17` → **`44`/`17`**.
- **Attribution.** Switching the drop half off in a standalone build (experiment
  only): `HEAD` measured `71`/`23` and the fixed tree `11`/`2`, inside the `30`
  flaps and run of `4` every continuous term meets. The obstacle half this issue
  named is therefore fixed to the continuous-term bound.
- **Blast radius.** Standalone 240-tick canonical state dumps of all 30 named
  scenarios, `HEAD` versus the working tree: **29 byte-identical**;
  `sheep-all-influences-diagnostic` differs, first at tick 113, which is the
  first tick in any fixture where a sheep is held on a contact face. At that tick
  the term stops publishing `(2.828427, 2.828427)` against `right_wall`, the
  sheep is pressed onto the line, and `resolve_sheep_against_paddock` refuses the
  `z` axis and names `right_wall` — the accepted division of labour.
- **Accepted measurements re-proved unchanged**, by diffing the whole
  `wide_eye.gameplay_simulation` key report against `HEAD`: zero clips with
  avoidance on against four with it off, first off-contact ticks `63` and `71`,
  closest approach `0.758357`, alignment polarization `0.824621` off versus
  `0.924042` on, and every first-tick dog-term observation. The only keys that
  moved are the diagnostic run's own stability counts.
- **New regression**, `wide_eye.gameplay_simulation`: an oracle in its own
  function holding every simulation on the heap. It pins the query at the clip
  line (along the face, into the face, already inside the band, a hundredth
  clear, half clear, exactly at the look-ahead), then runs the dog-pressed
  fixture for 240 ticks and requires zero flaps for the sheep held on the line
  and at most one for the sheep that stops turning into it. It fails at `HEAD`
  (`a_body_travelling_along_a_face_it_touches_reaches_no_obstacle`) and passes
  with the fix.
- **Suites.** `ctest --preset dev` 25/25, `ctest --preset release` 25/25,
  `ctest --preset dev-sanitized` 25/25, `format-check`, `clang-tidy-check`,
  `cmake -DMODE=check -P tools/qa/qa-tracker.cmake`, `git diff --check`, and
  `ctest --preset dev -R wide_eye.gameplay_simulation_stack_budget` all pass.
- Evidence files:
  `artifacts/phase3/2026-08-22/qa-003-avoidance-flap/` (gitignored).

**What this fix does not close.** The stability oracle's widened flap allowance
for avoidance and the applied sum is tightened from `20` per hundred ticks with a
run of `25` to `8` with a run of `20`, but it does not reach the continuous-term
`5`/`4`. The remainder is a different mechanism, filed as
[QA-005](QA-005-avoidance-response-is-bang-bang-near-a-face-and-at-the-drop-boundary.md):
the response is decided by distance alone rather than by the closing it corrects,
so a sheep a hundredth clear of a face with a near-parallel heading, and the
binary drop half, still alternate. The attribution experiment above is what
separates the two. Correcting that means changing the *shape* of an accepted
response, including a section of ADR 0008 that considered and rejected grading
the drop, so it is an owner decision rather than part of this fix.
