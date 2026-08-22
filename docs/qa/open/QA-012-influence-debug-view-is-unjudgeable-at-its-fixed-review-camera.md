---
id: QA-012
title: The influence debug view ships with one camera, and it is the visual tracer's distant holdout pose, so the overlay is 0.055% of the frame and cannot be read at the framing it captures
status: open
severity: S2
confidence: confirmed
area: platform
reporter: owner
reported: 2026-08-22
phase: 3
platform: windows
rule: docs/review/HUMAN_VISUAL_REVIEW.md
blocks: QA-011
verify:
  - wide_eye.opengl_influence_debug_overlay
  - wide_eye.influence_debug_view
  - wide_eye.influence_debug_frame_dump
  - target:qa-check
  - manual:a native capture of sheep-all-influences-diagnostic in which the per-sheep lanes, arrow directions, and chosen-neighbour links are readable at 1:1 without cropping or magnification
  - manual:the same run still shows enough flock context to tell which sheep is which and where the gate and wall are
  - manual:owner readability verdict recorded in the review packet for this view
---

## Symptom

Owner report, 2026-08-22: the influence debug view cannot be judged from the
frames it produces. The arrows are a few pixels each and could only be assessed
after cropping and magnifying about 4x, which is not a review of the view as it
ships.

The frames are the retained candidate packet
`artifacts/phase3/2026-08-22/debug-influence-views-native/`
(`influence-tick-{30,60,120}.png` plus matching `.txt` frame dumps and a
`review.md`), captured on the reference desktop — native Windows 11 Home
`10.0.26200`, MSVC `19.44.35228.0` Release, NVIDIA GeForce RTX 5070 Ti driver
`32.0.15.9186`, OpenGL `4.6.0 NVIDIA 591.86`, SDL 3.4.10, 1920x1080 — by three
runs of:

```text
build\Windows\release\wide_eye.exe --influence-debug-smoke \
    sheep-all-influences-diagnostic --tick <T> \
    --capture <dir>\influence-tick-<T>.png --frame-dump <dir>\influence-tick-<T>.txt
```

with `<T>` in `30`, `60`, `120`. All three exited `0` with
`influence_debug_result=pass` and `influence_debug_frame_matches=yes`.

The owner accepted the view in principle and required this and
[QA-011](QA-011-influence-applied-lane-is-unreadable-against-dark-geometry.md)
be corrected before the readability verdict is recorded. As noted in QA-011, the
repository's own records predate that statement: the packet's `review.md`
verdict boxes are blank and
[`ROADMAP.md`](../../../ROADMAP.md#L662-L667) still reads "no owner has looked at
the images". The roadmap item "Add debug arrows/labels for every influence,
chosen neighbor, arousal, target, state, and balance point"
([`ROADMAP.md:2054`](../../../ROADMAP.md#L2054)) stays unticked until the verdict
exists.

## Investigation

**Observed result — the view has exactly one camera, and it is a constant.**
`kInfluenceReviewCamera` is defined at
[`scenario_runner.cpp:136`](../../../src/platform/scenario_runner.cpp#L136) as
`.eye = {38.0F, 24.0F, 42.0F}, .target = {16.0F, 1.5F, 21.0F}`. A `grep` for the
symbol across `src/` and `tests/` returns exactly two hits: the definition and
its single use at
[`scenario_runner.cpp:504`](../../../src/platform/scenario_runner.cpp#L504),
inside `make_influence_debug_scene_frame`. There is no override, no argv shape
that selects a camera, and no per-tick derivation. The comment above it
([lines 131-135](../../../src/platform/scenario_runner.cpp#L131-L135)) records
two intentions — that two captures of different ticks must be comparable, which
requires a fixed pose, and that the pose "hold the whole 32x32 paddock floor in
a 16:9 frame". Neither intention mentions the legibility of what is drawn on top
of that floor.

**Observed result — that pose is the visual tracer's deliberately distant
holdout.** `kVisualFeasibilityFiveSheep.holdout_camera` is
`{.eye = {38.0F, 24.0F, 42.0F}, .target = {16.0F, 1.5F, 21.0F}}`
([`visual_tracer_configuration.cpp:29`](../../../src/platform/visual_tracer_configuration.cpp#L29)),
byte-identical, and its own comment calls it "the existing fixed influence-review
composition, retained as the elevated holdout rather than tuned alongside the
representative view". The plan that introduced it treats the holdout as the view
that must **not** be tuned for
([`docs/plans/visual-feasibility-before-objective-loop.md:244-246`](../../plans/visual-feasibility-before-objective-loop.md#L244-L246)).
So the only camera the debug view has is the one the surrounding plan chose to
be far away and left alone on purpose.

**Observed result — how little of the frame the overlay occupies.** Measured
here on 2026-08-22 by decoding the three retained PNGs and marking every pixel
within the engine's own ±6 lane tolerance
([`opengl_renderer.cpp:1714`](../../../src/render/opengl_renderer.cpp#L1714)):

| tick | drawn overlay px | share of 2,073,600 | bounding box | box as share of frame |
| --- | --- | --- | --- | --- |
| 30 | 1,113 | 0.054% | 120 x 151 | 0.87% |
| 60 | 1,029 | 0.050% | 138 x 145 | 0.97% |
| 120 | 1,143 | 0.055% | 143 x 133 | 0.92% |

The three drawn-pixel totals reproduce the counts the packet's `review.md`
transcribed from the original run logs, which were not retained as a file — so
those numbers are now independently recounted from the durable artifacts rather
than trusted from a transcription.

**Observed result — 79.5% of the frame is empty clear colour.** In all three
frames, 1,649,260 of 2,073,600 pixels are within ±4 of `kSkyColor`
([`opengl_renderer.cpp:571`](../../../src/render/opengl_renderer.cpp#L571)),
the colour the paddock scene clears to
([line 1336](../../../src/render/opengl_renderer.cpp#L1336)). All four corners
are sky. The count is identical across the three ticks, which is expected: only
the dog, the sheep, and the overlay move, and all of them stay inside the
paddock.

Inference: the pose does hold the whole floor, exactly as its comment says — but
the 32x32 floor is seen as a rotated diamond, so fitting it fits its diagonal,
and the four corners of the frame are spent on nothing. The subject of the
capture, the flock and its overlay, lands inside about 1% of the image.

**Observed result — what it takes to read it.** Cropping the tick-120 frame to
the overlay's bounding box plus 24 px of padding (191 x 181) and magnifying 4x
with nearest-neighbour makes the lanes, arrow directions, and chosen-neighbour
links legible. At 1:1 they are not. The packet's own agent inspection reached
the same conclusion independently and called scale "the single biggest obstacle
to a verdict", recommending "crop to the flock first" — that recommendation is
the workaround, and its existence is the reason this is S2 rather than S1.

**Falsification attempted.** The narrowest alternative explanation is that the
arrows are small because the *scale* is small, not because the camera is far —
in which case the fix would be the scale knobs and not the pose. Checked:
`kInfluenceArrowScaleSecondsSquared = 0.5` and
`kInfluenceArrowMaximumLength = 2.5`
([`influence_debug_view.hpp:117`](../../../src/render/influence_debug_view.hpp#L117)
and [line 124](../../../src/render/influence_debug_view.hpp#L124)) are the two
existing knobs, and `influence_debug_clamped_arrows` is **`0` at all three
ticks** (read back from the three frame dumps), so no arrow is currently hitting
the 2.5-unit ceiling. Lengthening arrows is therefore available as a lever.
But the arrows are already up to 2.5 world units in a 32x32 paddock that renders
into roughly 1% of the frame; a longer arrow at this pose still lands in single-
digit pixels, and lengthening them enough to read would make them overlap each
other and stop meaning "this much acceleration". **The alternative explanation
does not survive, but it does establish that the scale knobs are part of the
solution space, not outside it.**

**Blast radius, checked rather than assumed — nothing pins this camera.**

- `wide_eye.opengl_influence_debug_overlay` runs
  `--influence-debug-smoke sheep-all-influences-diagnostic` with no capture and
  passes on the literal `influence_debug_frame_matches=yes`
  ([`CMakeLists.txt:1056-1061`](../../../CMakeLists.txt#L1056-L1061)).
- Its oracle is
  [`is_expected_influence_debug_frame`](../../../src/render/opengl_renderer.cpp#L1741-L1768):
  at least `3` lanes with at least `8` pixels each, at least `64` overlay pixels
  in total, and strictly fewer than half the frame's pixels. **A camera change
  moves every one of those counts**, and the lower bounds are the real
  constraint — a pose that pushed part of the flock out of frame could drop a
  lane below 8 pixels. The upper bound (half the frame) is far away: the current
  total is about 1,100.
- `wide_eye.influence_debug_view` and `wide_eye.influence_debug_frame_dump` are
  headless and build segments in world space from a published snapshot; neither
  is aware of a camera.
- No golden depends on this view: nothing under `tests/goldens/` references it,
  and no accepted baseline for it exists — the packet's `review.md` states
  plainly that an Accept here would create the first one.

Changing the camera changes captured pixels. That is the point of the change,
not a risk of it.

### What is not wrong

- Having a **fixed** pose is right, and the reason recorded at
  [`scenario_runner.cpp:131-135`](../../../src/platform/scenario_runner.cpp#L131-L135)
  is sound: two captures at different ticks must be comparable, which a
  dog-following camera would break. This issue is about *which* fixed pose, not
  about fixing one.
- The depth-tested overlay, which lets scene geometry occlude an arrow so the
  diagram reads as part of the world, is a deliberate choice and is out of scope
  here.
- The lane design, the arrow-length scale contract, and the frame dump are all
  sound and unaffected.

## Root cause

The influence debug view was given one camera and it was borrowed from the
visual tracer's holdout — a pose selected to be distant and elevated, and
explicitly not tuned for what it shows. Fitting the whole rotated 32x32 paddock
into 16:9 spends 79.5% of the frame on clear colour and leaves the flock, which
is the entire subject of the overlay, inside a box under 1% of the image. The
result is a view that renders correctly, passes its oracle, and cannot answer
its own question — "why did that sheep do that" — at the framing it ships with.

## Expected behavior

The Phase 3 exit gate requires that "Debug views explain surprising flock
responses without guessing"
([`ROADMAP.md:2577`](../../../ROADMAP.md#L2577)), and the review template
requires that "Debug views explain surprising output rather than merely adding
noise" and that "Geometry, collision cues, and objectives are readable"
([`docs/review/HUMAN_VISUAL_REVIEW.md:80,85`](../../review/HUMAN_VISUAL_REVIEW.md#L80-L85)).
A diagnostic whose contents require external cropping and 4x magnification
before any of that can be assessed does not meet those, however correct its
geometry.

The same template also states that a candidate "is not accepted from one
showcase camera alone"
([line 87](../../review/HUMAN_VISUAL_REVIEW.md#L87)); this view currently has
one camera, which the packet's own metadata records as satisfying neither the
representative nor the holdout role formally.

[`src/README.md`](../../../src/README.md#L141-L147) fixes what must survive any
change: the view reads published snapshots only, holds no authoritative state,
writes into a caller-owned bounded segment buffer, and is reachable only through
its own strict argv shapes — and per
[`CLAUDE.md`](../../../CLAUDE.md), a new capability gets a new argv shape rather
than a loosened existing one.

## Fix notes

Scope is the camera and framing of one debug scenario. Likely files:
`src/platform/scenario_runner.cpp` (the pose and, if a selectable view is
chosen, the argv shape in `src/platform/main.cpp`), and possibly
`src/render/influence_debug_view.hpp` if the arrow scale moves. No gameplay
rule, no published state, no format version, no budget, and no golden. Ownership
boundaries: `src/platform` owns the pose; `src/render` owns the scale. Nothing
in [`src/README.md`](../../../src/README.md) moves.

**This is the framing question, not a decision.** Whether the answer is a nearer
fixed pose, a pose derived once from the flock's extent, a larger arrow scale, a
smaller capture of a cropped region, or a selectable camera on a new argv shape
belongs to the fixer and the owner together. Some considerations, offered so the
choice is informed rather than to make it:

- A **nearer fixed pose** is the smallest change and keeps tick-to-tick
  comparability intact, but it has to be chosen once and it will not hold a
  flock that later spreads out.
- A **flock-framing pose** solves that and breaks comparability across ticks
  unless the framing is computed from the scenario rather than the tick — which
  is a real design question, not a parameter.
- **Arrow scale** alone will not carry this on its own (see the falsification
  above), but it is a legitimate part of a combined answer, and
  `clamped_arrows` being `0` at all three ticks means there is currently room
  before arrows start losing their length meaning. Any change to the scale must
  keep the "stated scale, not a raw acceleration" contract recorded in
  [`src/README.md`](../../../src/README.md#L146-L147) and re-record the printed
  `influence_debug_arrow_scale_seconds_squared` /
  `influence_debug_arrow_maximum_length` values.
- A **selectable camera** is the most flexible and the most expensive; it needs
  a new strict argv shape and it multiplies what a reviewer has to look at.

Constraints the fixer must respect:

1. **Keep the pose deterministic and tick-independent**, for the reason at
   `scenario_runner.cpp:131-135`. Comparability across ticks is the one property
   the current pose gets right.
2. **Do not silently reuse the holdout again.** If a new pose happens to match
   the visual tracer's `representative_camera_state` or `holdout_camera`, say so
   and say why; the current defect exists precisely because a pose was reused
   without asking what it was for.
3. **Re-check the frame oracle's lower bounds.** `kMinimumVisibleLanes = 3`,
   `kMinimumLanePixels = 8`, `kMinimumOverlayPixels = 64`. A tighter framing
   raises the counts and is safe; a framing that clips a sheep out of view could
   drop a lane below 8 pixels and fail `wide_eye.opengl_influence_debug_overlay`
   for a reason unrelated to the change's intent.
4. **Do not loosen that oracle to make a new framing pass.** If the new pose
   trips it, the pose is wrong, not the bound.
5. **Re-capture all three ticks, not one.** Tick 120 is the hardest frame
   (flock against the wall and gate) and tick 30 the easiest; a framing tuned on
   either alone will be wrong for the other.

Suites that must pass: `wide_eye.opengl_influence_debug_overlay` on a native
display, which is the only test the change can break, plus
`wide_eye.influence_debug_view` and `wide_eye.influence_debug_frame_dump`
headless to show the world-space geometry is untouched. `target:qa-check` for
the tracker. The closure evidence is manual and named in `verify:`.

This issue and QA-011 are the two the owner named as preconditions for the
readability verdict; fixing either alone will not produce it. Land this one
first — QA-011's fix has to be judged against whatever framing this issue
settles on, and doing it in the other order means capturing and judging the
colours twice.
