---
id: QA-011
title: The influence debug view's `applied` lane is near-black, so the one arrow that carries the result disappears against the paddock wall's shadow, the gate, and the sheep's own legs
status: open
severity: S2
confidence: confirmed
area: render
reporter: owner
reported: 2026-08-22
phase: 3
platform: windows
rule: docs/review/HUMAN_VISUAL_REVIEW.md
depends: QA-012
verify:
  - wide_eye.influence_debug_view
  - wide_eye.influence_debug_frame_dump
  - wide_eye.opengl_influence_debug_overlay
  - target:qa-check
  - manual:a native capture of sheep-all-influences-diagnostic at tick 120 in which the applied arrows separate from the wall shadow, the red gate, and the sheep legs
  - manual:the same capture at tick 30 shows the applied lane still separating from grass, and no other lane became harder to read
  - manual:owner readability verdict recorded in the review packet for this view
---

## Symptom

Owner report, 2026-08-22, after looking at the native captures of the influence
debug view: the `applied` arrows — the one line that says what the sheep
actually did — cannot be told apart from the background when the flock is up
against the paddock wall. Against open grass the same channel reads fine.

The frames looked at are the retained candidate packet
`artifacts/phase3/2026-08-22/debug-influence-views-native/`
(`influence-tick-{30,60,120}.png` with matching `.txt` frame dumps and a
`review.md`). They were produced on the reference desktop — native Windows 11
Home `10.0.26200`, MSVC `19.44.35228.0` Release, NVIDIA GeForce RTX 5070 Ti
driver `32.0.15.9186`, OpenGL `4.6.0 NVIDIA 591.86`, SDL 3.4.10, 1920x1080 — by
three runs of:

```text
build\Windows\release\wide_eye.exe --influence-debug-smoke \
    sheep-all-influences-diagnostic --tick <T> \
    --capture <dir>\influence-tick-<T>.png --frame-dump <dir>\influence-tick-<T>.txt
```

with `<T>` in `30`, `60`, `120`. All three exited `0` with
`influence_debug_result=pass` and `influence_debug_frame_matches=yes`.

The owner accepted the view in principle and required this and
[QA-012](QA-012-influence-debug-view-is-unjudgeable-at-its-fixed-review-camera.md)
be corrected before the readability verdict is recorded. **Note the repository's
own records predate that statement and have not been updated for it:** the
packet's `review.md` verdict boxes are blank, and
[`ROADMAP.md`](../../../ROADMAP.md#L662-L667) still reads "no owner has looked at
the images". Whoever next edits the checkpoint should reconcile that; this issue
does not.

## Investigation

**Observed result — the colour, and the reason recorded for it.**
[`influence_debug_view.cpp:24`](../../../src/render/influence_debug_view.cpp#L24)
sets the eighth lane to `{{0.04F, 0.04F, 0.06F}}, // applied: near-black`. The
comment above the palette
([lines 10-15](../../../src/render/influence_debug_view.cpp#L10-L15)) states the
choice deliberately:

> One color per lane. Hue alone is never the only channel — the lane index is
> countable off the mast — but the palette still avoids putting the two terms a
> reviewer most often compares, attraction and separation, on the red/green axis
> that the common forms of color blindness confuse. `applied` is deliberately
> near-black: it is the only line that has to read as a *result* against both
> the grass and the sky.

That reasoning names two backgrounds. The failure case is a third the comment
does not consider.

**Observed result — the second named background never occurs at this camera.**
Measured here on 2026-08-22 by decoding all three retained PNGs and marking
every pixel within ±4 of the clear colour `kSkyColor`
([`opengl_renderer.cpp:571`](../../../src/render/opengl_renderer.cpp#L571),
`0.47, 0.66, 0.82`, 8-bit `120,168,209`, cleared at
[line 1336](../../../src/render/opengl_renderer.cpp#L1336)): **the number of
overlay pixels with any sky pixel inside a 5x5 neighbourhood is `0` in all three
frames.** The overlay's vertical extent is rows 455-605, 451-595, and 425-557 of
1080; the sky occupies 1,649,260 of 2,073,600 pixels but none of it is behind an
arrow. The comment's grass case is exercised; its sky case is not, at the only
camera this view has (see QA-012). A third case — paddock wall, red gate,
wall shadow, and the sheep proxies' own dark legs — is exercised and is not
considered.

**Observed result — measured contrast, and it is the numbers that make this
actionable.** Method: for every pixel matching the `applied` lane colour within
the same ±6 tolerance the engine's own oracle uses
([`opengl_renderer.cpp:1714`](../../../src/render/opengl_renderer.cpp#L1714)),
take the mean WCAG relative luminance of the non-overlay pixels in its 5x5
neighbourhood and compute the contrast ratio against the lane colour. Decoded
from the three retained PNGs on this host, 2026-08-22:

| tick | background | `applied` px | median contrast | below 3:1 | below 2:1 | below 1.5:1 |
| --- | --- | --- | --- | --- | --- | --- |
| 30 | open grass | 646 | 3.40:1 | 16.0% | 2.2% | 0% |
| 60 | mixed | 594 | 3.23:1 | 36.5% | 21.3% | 7.8% |
| 120 | wall, gate, legs | 644 | 2.04:1 | 58.1% | 49.8% | 28.2% |

The maximum contrast anywhere in the tick-120 frame is 3.48:1; at tick 30 it is
10.80:1. There is nothing bright behind the arrows in the frame where the
interesting thing happens.

**Falsification attempted, and it narrowed the claim rather than confirming the
first framing.** The obvious counter-theory is that tick 120 is simply a dark
frame and every lane suffers. Measuring all eight lanes the same way says
otherwise, and says something more useful:

| lane | tick 30 median | tick 120 median |
| --- | --- | --- |
| separation (orange) | 1.57:1 | 1.55:1 |
| attraction (green) | 1.24:1 | 3.11:1 |
| alignment (cyan) | 1.07:1 | 6.80:1 |
| avoidance (blue) | 1.04:1 | 2.65:1 |
| `applied` (near-black) | 3.40:1 | 2.04:1 |

The seven cause lanes have *worse* luminance contrast than `applied` in places —
and that is fine, because they are saturated hues and luminance is not their
only separation channel. `applied` is the only steering lane with no hue at all:
luminance is the only thing it has. **A low luminance-contrast number condemns
`applied` specifically, and does not condemn the chromatic lanes.** Do not read
the table above as seven more defects.

**Observed result — the specific dark backgrounds, from source.** The sheep
proxy's legs are `kLeg{0.12F, 0.105F, 0.09F}` and its face blocks are
`kFace{0.18F, 0.16F, 0.14F}`
([`opengl_renderer.cpp:485-486`](../../../src/render/opengl_renderer.cpp#L485-L486)),
before shading darkens the unlit faces further. The overlay is drawn unlit, so
the arrow keeps its exact vertex colour
([`opengl_renderer.cpp:1710-1712`](../../../src/render/opengl_renderer.cpp#L1710-L1712))
while the geometry behind it does not.

**Observed result — the mast is the nearest neighbour and is nearly the same
value.** `kMastColor{0.14F, 0.14F, 0.17F}`
([`influence_debug_view.cpp:36`](../../../src/render/influence_debug_view.cpp#L36))
runs from `kInfluenceLaneBaseHeight - kInfluenceLaneSpacing` to
`kInfluenceLaneBaseHeight + 7 * kInfluenceLaneSpacing` above the sheep
([lines 171-176](../../../src/render/influence_debug_view.cpp#L171-L176)), and
`applied` is lane index 7 — the topmost lane, whose arrow origin sits on the
mast's top end
([lines 202-211](../../../src/render/influence_debug_view.cpp#L202-L211)).
Computed contrast between the applied colour and the mast colour: **1.27:1**.
The result arrow and the scaffolding it hangs off are, for practical purposes,
the same value.

**Observed result — this is the largest lane, not a corner case.** Read back
from the retained frame dumps and independently recounted from the PNGs here:
`applied` is 646 of 1113 drawn overlay pixels at tick 30, 594 of 1029 at tick
60, and 644 of 1143 at tick 120. Two reasons, both by design: it is the summed
vector, so it is usually the longest arrow; and it is the only channel drawn
with a doubled shaft, because core-profile line width is not portable
([`influence_debug_view.cpp:110-122`](../../../src/render/influence_debug_view.cpp#L110-L122)).

**Observed result — the grey ramp is oversubscribed, which is why this is not a
one-line colour swap.** Eight of the palette's achromatic entries, in file
order:

| constant | value | 8-bit |
| --- | --- | --- |
| `applied` lane | `0.04, 0.04, 0.06` | `10,10,15` |
| `kMastColor` | `0.14, 0.14, 0.17` | `36,36,43` |
| `kHeadingPreviousColor` | `0.42, 0.42, 0.46` | `107,107,117` |
| `kNotEvaluatedColor` | `0.46, 0.46, 0.50` | `117,117,128` |
| `kArousalScaleColor` | `0.58, 0.58, 0.62` | `148,148,158` |
| `kPushAxisColor` | `0.60, 0.60, 0.66` | `153,153,168` |
| `kHeadingCurrentColor` | `0.98, 0.98, 0.98` | `250,250,250` |
| `kCentroidColor` | `0.98, 0.98, 0.98` | `250,250,250` |

Eight roles across seven distinct grey levels — the near-white slot is taken
twice, by two different things
([`influence_debug_view.cpp:36-45`](../../../src/render/influence_debug_view.cpp#L36-L45)).
Moving `applied` up the ramp collides with `kMastColor` and
`kHeadingPreviousColor`; moving it to the top collides with the two 0.98
entries. The seven cause channels occupy saturated hues, so `applied` cannot
take one of those without losing the "this is the result, not a cause"
distinction the near-black was chosen for. A brightened grey is not a fix; it
just moves the collision.

**Blast radius, checked rather than assumed — no test asserts a lane colour.**

- `wide_eye.opengl_influence_debug_overlay` runs
  `--influence-debug-smoke sheep-all-influences-diagnostic` and passes on the
  literal `influence_debug_frame_matches=yes`
  ([`CMakeLists.txt:1056-1061`](../../../CMakeLists.txt#L1056-L1061)). Its
  oracle,
  [`is_expected_influence_debug_frame`](../../../src/render/opengl_renderer.cpp#L1741-L1768),
  requires at least 3 lanes with at least 8 pixels each, at least 64 overlay
  pixels in total, and fewer than half the frame — nothing about which colours.
  Note the coupling: that oracle finds lanes *by* their colours, so a colour
  change silently changes what it counts.
- `wide_eye.influence_debug_view` passes on `influence_debug_view_result=pass`
  ([`CMakeLists.txt:946-949`](../../../CMakeLists.txt#L946-L949)).
- `wide_eye.influence_debug_frame_dump` passes on
  `influence_debug_dump_result=pass`
  ([`CMakeLists.txt:958-961`](../../../CMakeLists.txt#L958-L961)). The dump does
  print every lane colour to 17 digits, so its output text changes.
- No golden depends on this view: nothing under `tests/goldens/` references it.

**Correction to the report, and it matters for the fixer.** The report expected
`influence_debug_sweep_digest` to be compared against a warm-run digest for
repeatability, and to be recorded in `ROADMAP.md`. Neither is true.
[`influence_debug_view_tests.cpp:142-156`](../../../tests/influence_debug_view_tests.cpp#L142-L156)
does mix `segment.color[axis]` into the digest, so a colour change *will* move
the number — but the digest is only **printed**
([line 576](../../../tests/influence_debug_view_tests.cpp#L576)) and never
compared to anything. `warm_digest`
([lines 544-549](../../../tests/influence_debug_view_tests.cpp#L544-L549)) is
accumulated and then never read; the only assertion in that block is
`build_allocations == 0`. And a `grep` for `digest` across `ROADMAP.md`,
`ROADMAP_ARCHIVE.md`, and `docs/` finds no recorded value — only the prose claim
"a stable sweep digest" at
[`ROADMAP.md:2077`](../../../ROADMAP.md#L2077), which no assertion backs.

So the practical consequence is the opposite of the one expected: **a colour
change will not fail any test, will not require updating any recorded number,
and will leave no trace.** That is a gap worth its own issue; it was not filed
here because this session was scoped to three, and it is recorded in this
paragraph so it is not lost.

### What is not wrong

The following are sound and a fix must preserve them:

- the stacked per-channel lane design, one arrow per steering term off a shared
  mast, so terms read independently instead of as one summed blob;
- the countable lane index off the mast, which is the colour-independent half of
  the statement and is what makes the view usable by a colour-blind reviewer;
- the doubled `applied` shaft, which is the right way to weight a line in a core
  profile without portable line width;
- the deliberate avoidance of a red/green pairing for attraction and separation;
- the arrow-length scale as a stated `0.5 s²` with a `2.5` world-unit clamp
  rather than a raw acceleration.

## Root cause

The `applied` lane was given the only value on the palette's luminance ramp that
has no headroom downward, and it was chosen against a two-background model —
grass and sky — that does not match the frames this view actually produces. At
the shipped camera the sky is never behind an arrow at all, while the paddock
wall, the wall's shadow, the red gate, the sheep proxies' `0.12`-albedo legs,
and the `0.14` mast the arrow hangs off are all within about one stop of it.
Because `applied` is the only steering lane with no hue, luminance is its only
separation channel, so when the background darkens the channel has nothing left.

## Expected behavior

The Phase 3 exit gate requires that "Debug views explain surprising flock
responses without guessing"
([`ROADMAP.md:2577`](../../../ROADMAP.md#L2577)), and the review template
requires that "Debug views explain surprising output rather than merely adding
noise"
([`docs/review/HUMAN_VISUAL_REVIEW.md:85`](../../review/HUMAN_VISUAL_REVIEW.md#L85)).
The applied acceleration is the answer to the question this view exists to ask;
a channel that is legible on grass and not against structure explains the flock
exactly when the flock is uninteresting.

[AGENTS.md](../../../AGENTS.md) additionally asks that interaction design treat
"readable contrast" and "non-color-only meaning" as part of the design rather
than an afterthought. The lane index off the mast already satisfies the second
half for this view; the first half is what fails here.

[`src/README.md`](../../../src/README.md#L141-L147) fixes what must not change
while fixing this: the view reads published snapshots only, holds no
authoritative state, writes into a caller-owned bounded segment buffer, and is
reachable only through its own strict argv shapes.

## Fix notes

Scope is `src/render/influence_debug_view.cpp` and, if the direction chosen
needs new segments, `influence_debug_view.hpp`'s ceilings. No gameplay rule, no
scenario, no published state, no format version, no budget, and no golden.
Ownership boundary: entirely inside `src/render`, which
[`src/README.md`](../../../src/README.md) already defines as holding no
authoritative state.

**Reproduce first**, and it costs one GPU run rather than a suite: re-capture
`--influence-debug-smoke sheep-all-influences-diagnostic --tick 120` and compare
the applied arrows against the wall shadow. The three existing PNGs in
`artifacts/phase3/2026-08-22/debug-influence-views-native/` are the before-state
and need no run at all.

**Candidate directions, none of them a decision.** The constraint that rules out
the obvious fix is above: the grey ramp has no free slot, and the seven
saturated hues are spoken for.

- A contrasting casing or outline — draw the applied shaft twice, once in a
  light value slightly offset and once in the near-black — so the pair reads
  against any background without claiming a new palette slot. This costs extra
  segments and must be checked against `kMaximumInfluenceDebugSegments` (`271`;
  worst observed `240` at tick 60, so headroom is 31 segments per frame with
  five sheep, and the ceiling may need raising).
- Give `applied` a distinct *form* rather than a distinct value — a dashed
  shaft, a different head, a wider doubled-shaft offset — so the identification
  no longer depends on luminance at all.
- Re-derive the whole palette against the actual backgrounds this view produces,
  which is the thorough option and probably wants a short plan doc rather than a
  QA fix.

Constraints the fixer must respect:

1. **Do not simply lighten the grey.** It collides with `kMastColor` at 0.14 or
   `kHeadingPreviousColor` at 0.42, and at the top of the ramp with the two 0.98
   entries. Any change to a grey must be checked against all eight achromatic
   entries listed above, not just the neighbours.
2. **Do not give `applied` a saturated hue.** The seven cause channels are hues;
   the result must stay visually distinct in kind, not merely in value.
3. **Keep the lane index countable off the mast.** That is the colour-blind
   fallback and it is the reason hue is allowed to carry meaning at all here.
4. **Verify against a dark background *and* grass.** A fix measured only at tick
   120 can easily make tick 30 worse; the `verify:` entries name both.
5. **No test will catch a mistake here.** See the digest paragraph above — the
   suites pass on `*_result=pass` markers, no golden covers this view, and the
   printed digest is never compared. The evidence has to be a capture and the
   owner's eye.

This issue and QA-012 are the two the owner named as preconditions for the
readability verdict; fixing either alone will not produce it. QA-012 changes the
framing the fix is judged at, so re-capturing for this issue after QA-012 lands
avoids doing the visual work twice.

Suites that must pass: `wide_eye.influence_debug_view` and
`wide_eye.influence_debug_frame_dump` headless, plus
`wide_eye.opengl_influence_debug_overlay` on a native display to show the frame
oracle still recognises the overlay after the colours move — that one is the
real risk, because the oracle identifies lanes by their colours.
`target:qa-check` for the tracker.
