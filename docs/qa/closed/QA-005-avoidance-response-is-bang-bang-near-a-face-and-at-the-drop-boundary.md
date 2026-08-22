---
id: QA-005
title: Obstacle avoidance answers a grazing approach and a drop with a near-maximum push, so a sheep alternates between most of the maximum and zero
status: fixed
severity: S2
confidence: confirmed
area: game
reporter: agent
reported: 2026-08-22
phase: 3
platform: wsl-ubuntu-24.04
closed: 2026-08-22
rule: docs/decisions/0008-obstacle-and-drop-avoidance.md
verify:
  - wide_eye.gameplay_simulation
  - wide_eye.gameplay_simulation_stack_budget
  - label:unit
  - label:sanitizer
---

## Symptom

Obstacle and drop avoidance publishes a push whose size is decided by *distance
alone*, so a sheep that is barely closing on something gets nearly the whole
term. The push then removes the closing it was answering, the next tick sees
nothing, whatever was pressing the sheep restores the approach, and the term
alternates. Two shapes of this were measured; both survive the QA-003 fix, which
corrected a different mechanism (a contact reported through a face behind the
body).

Observed result (2026-08-22, WSL Ubuntu 24.04.4, Clang 18.1.3, `dev` preset,
working tree with the QA-003 fix applied).

**A grazing approach.** In the new `sheep_avoidance_contact_*` fixture — three
sheep on the paddock's wall line with one stationary dog pressing them onto it —
the sheep that starts `0.01` above the line flaps **36 times in 240 ticks**, on a
two-to-four tick cycle, with no contact and no drop involved:

```
tick=  2 sheep=2 prior_z=16.509827 prior_v=(1.010414, -0.010380) obstacle=left_wall distance=0.956672 push=(2.395487, 2.395487)
tick=  3 sheep=2 prior_z=16.510148 prior_v=(1.060711, +0.019265) obstacle=none      distance=0.000000 push=(0.000000, 0.000000)
tick=  6 sheep=2 prior_z=16.510101 prior_v=(1.091559, -0.010951) obstacle=left_wall distance=1.006812 push=(2.372796, 2.372796)
tick=  7 sheep=2 prior_z=16.510413 prior_v=(1.141292, +0.018734) obstacle=none      distance=0.000000 push=(0.000000, 0.000000)
```

The sheep is `0.0098` above the face closing at `0.0104` world units/s. Its swept
body reaches the face `0.96` along its path, so the linear falloff answers with
`3.39` of the term's `4.0`. Stopping that closing inside that clearance needs
about `0.005`; the term applies roughly seven hundred times that, the sheep is
lifted off its approach in one tick, and the cycle repeats. The count is
identical at `HEAD` (`12700d0`) and after the QA-003 fix — this mechanism is
untouched by it.

**The drop boundary.** The drop half is binary by the accepted design, at the
full maximum whatever the distance, so a sheep whose look-ahead probe sits near
the paddock bound toggles a `4.0` brake on and off as its heading wanders. In the
600-tick `sheep-all-influences-diagnostic` run this is what the remaining
stability-oracle flaps are: worst sheep `27` avoidance flaps with a run of `10`,
and `44` applied-sum flaps with a run of `17`, against `5`/`4` for the six
continuous terms. Because the brake points straight back along the approach, it
reverses the sheep it slows, which reverses the probe direction, which switches
the answer — a bang-bang controller with no dead zone.

Reproduce:

```bash
cmake --build --preset dev
ctest --preset dev -R wide_eye.gameplay_simulation
./build/Linux/dev/wide_eye_gameplay_simulation_tests | grep -E "contact_face_grazing_flaps|stability_avoidance_worst|stability_applied_worst"
```

## Investigation

Observed result, same build, date, and platform.

- [`gameplay_simulation.cpp:637`](../../../src/game/gameplay_simulation.cpp#L637)
  computes `urgency = 1 - contact_distance / look_ahead` and multiplies the
  term's maximum by it. `contact_distance` is measured *along the sheep's path*,
  so for a grazing approach it is `perpendicular_clearance / sin(angle)` — a
  quantity that says nothing about how fast the sheep is closing. The same
  clearance approached at a hundredth of a radian and at a right angle produce
  wildly different distances and therefore wildly different pushes, and the
  shallow one is the case where the push is least warranted.
- [`gameplay_simulation.cpp:650`](../../../src/game/gameplay_simulation.cpp#L650)
  applies `-travel * maximum_acceleration` whenever the ground under the
  look-ahead point is not finite, with no distance term at all. This is the
  accepted design: [ADR 0008](../../decisions/0008-obstacle-and-drop-avoidance.md)
  records *why* the drop response is binary and rejects clipping the probe
  against the paddock's outer bounds.

Attribution measured by switching the drop half off in a standalone
`clang++-18 -std=c++23 -I src` build of the game sources (experiment only, not a
change to the tree), worst sheep over the same 600-tick diagnostic run:

| build | drop half | avoidance flaps / run | applied flaps / run |
| --- | --- | --- | --- |
| `HEAD` `12700d0` | on | `90` / `23` | `77` / `17` |
| QA-003 fixed | on | `27` / `10` | `44` / `17` |
| `HEAD` `12700d0` | off | `71` / `23` | `94` / `17` |
| QA-003 fixed | off | `11` / `2` | `10` / `2` |

With the drop half switched off, the QA-003 fix takes the term from `71` flaps
with a run of `23` to `11` with a run of `2` — inside the `30`/`4` allowance every
continuous term meets. The remaining flaps in the shipped run are therefore the
drop half and the grazing case above, not the contact face QA-003 named.

Inference: both are the same defect stated twice — the response is decided by
*where* the geometry is and not by *how fast the sheep is closing on it*, so it
overshoots by orders of magnitude whenever the closing is slow, and an
overshooting response that removes its own input oscillates.

## Root cause

The magnitude of both halves is a function of position and direction only.
Obstacle urgency uses the along-path contact distance, which is not a measure of
approach; the drop half uses no distance at all. A term whose output is
near-maximal for an arbitrarily small closing rate necessarily reverses that
closing in one tick, which removes the condition that produced the output, so the
term alternates rather than settling.

## Expected behavior

[ADR 0008](../../decisions/0008-obstacle-and-drop-avoidance.md) states continuity
as a property it wants — "the boundary is continuous rather than a step" — and
derives its magnitudes from an energy argument (`L = v² / A`) that is explicitly
about the *speed* a sheep is travelling at the face. A response consistent with
that derivation would scale with the closing the sheep actually has, so a sheep
grazing a face at a hundredth of a unit per second would feel a hundredth of the
push a sheep driving straight at it feels, and the drop half would fall off with
distance the way the obstacle half does.

## Fix notes

The rule now lives in `apply_sheep_avoidance` in `src/game/sheep_rules.cpp`.
Its obstacle query already reported the face normal; the final correction adds
an exact ground-boundary query to the geometry owner rather than sampling or
hard-coding paddock bounds in the sheep rule.

**This was a redesign of an accepted rule, not a silent defect patch.** The owner
approved changing all three affected parts together: the response magnitude, the
drop direction/distance, and lateral reachability at paddock corners. ADR 0008
now records that correction. The two initial candidate shapes were:

1. Scale the obstacle half by the component of travel into the named face. Head-on
   approaches are unchanged (the component is exactly `1`), grazing approaches
   fall to nearly nothing, and the term becomes continuous as the path turns
   parallel. Measured as an experiment on 2026-08-22 this moved the worst sheep
   from `27`/`10` to `6`/`3` for two sheep in the diagnostic run while *raising* a
   third to `46`/`9`, because the drop half then dominates — so it is not
   sufficient alone.
2. Give the drop half a distance by marching `ground_height` along the probe and
   applying the same linear falloff. Deterministic and allocation-free, but it
   overturns ADR 0008's "Why the drop response is binary", changes every
   avoidance-enabled scenario, and needs a stated sampling resolution.

Whichever is chosen, the accepted `sheep-avoidance-on` numbers — zero clips with
the term on against four with it off, closest approach `0.758357` — must be
re-proved or re-derived as a deliberate correction, and the flap allowances in
`wide_eye.gameplay_simulation` (`8` per hundred ticks with a run of `20` for
avoidance and the applied sum, against `5`/`4` for the continuous terms) and the
grazing allowance in the contact fixture (`40` flaps in 240 ticks) should then
come down to the continuous values.

## Work note — 2026-08-22

The confirmed batch reproduction passed `wide_eye.gameplay_simulation` and
again reported `36` grazing flaps, avoidance `27`/`10`, and applied-sum
`44`/`17` on WSL Ubuntu 24.04.4 with Clang 18.1.3. With owner approval, one
combined candidate scaled obstacle response by normalized inward approach and
located the drop boundary with a deterministic 12-step bisection before applying
the same linear falloff. It improved the focused grazing fixture from `36` to
`2` flaps, but worsened the 600-tick diagnostic to avoidance `100`/`6` and
applied-sum `95`/`6`. Temporary attribution showed repeated obstacle/drop
switches at paddock corners.

That candidate and its experimental oracle changes were reverted. The original
`wide_eye.gameplay_simulation` test passes again, and no gameplay source or test
change remains from the experiment. At that checkpoint work stayed open: the
evidence pointed to a direction/ownership problem between obstacle and drop
responses near corners,
so another magnitude-only patch would exceed the approved correction and needs
a fresh ADR-aware design decision.

## Resolution

The geometry owner now reports an exact ground-boundary distance and inward
normal, and it offers an obstacle's lateral edge only when a sheep body has legal
centre space beyond it. Obstacle and drop responses share one distance falloff
and scale with actual speed into the boundary; drop response points along the
inward normal, and simultaneous responses still sum under the existing term
maximum. No published evidence field or format version changed.

Observed result (2026-08-22, WSL Ubuntu 24.04.4, Clang 18.1.3): the grazing
fixture fell from `36` to `2` flaps in 240 ticks. The 600-tick diagnostic now
measures avoidance at `4` flaps with a longest run of `1` and the applied sum at
`9` with a run of `2`, so both meet the shared `5`-per-100-ticks / `4`-tick-run
allowance. The paired fixture retains zero avoidance-on hard clips against four
with avoidance off; corrected deterministic observations include a closest wall
gap of `0.172705` and drop rest at `x = 0.919383`.

Verification on the same platform:

- `cmake --build --preset dev` — passed.
- `ctest --preset dev --output-on-failure` — 30/30 passed, including
  `wide_eye.gameplay_simulation`, its `unit` label, and
  `wide_eye.gameplay_simulation_stack_budget`.
- `cmake --build --preset dev-sanitized` — passed.
- `ctest --preset dev-sanitized --output-on-failure` — 30/30 passed, including
  all 28 sanitizer-labeled tests.
- `cmake --build --preset dev --target format-check` — passed after applying the
  project formatter to the four changed C++ files.
- `cmake --build --preset dev --target clang-tidy-check` — passed.
- `cmake -DMODE=check -P tools/qa/qa-tracker.cmake` — passed with one open and
  six closed issues after regeneration.
- `git diff --check` — passed.
