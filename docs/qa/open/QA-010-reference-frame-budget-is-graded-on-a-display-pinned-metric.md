---
id: QA-010
title: The reference performance gate grades a swap-inclusive frame metric that the display pins, so `within_performance_budget=yes` reports the refresh rate rather than renderer cost
status: open
severity: S3
confidence: confirmed
area: platform
reporter: owner
reported: 2026-08-22
phase: 3
platform: windows
rule: docs/decisions/0001-native-foundation.md
verify:
  - target:qa-check
  - wide_eye.performance_utilities
  - wide_eye.opengl_handcrafted_paddock_performance
  - wide_eye.opengl_sheep_motion_performance
  - manual:a performance run on the reference desktop records the swap interval actually in force beside synchronized_frame_*, so a later reader can tell whether the frame metric was refresh-paced
  - manual:paired --visual-tracer-performance-smoke runs on the reference desktop with the driver swap interval on and off, reported together, showing whether synchronized_frame_median_ns leaves the 6947690 ns refresh period
  - manual:the plan and roadmap prose that presents the frame budget name what the metric contains, or the gate moves to a metric that excludes presentation wait
---

## Symptom

The Phase 0 reference baseline reports `within_performance_budget=yes`, and the
runner's `review.md` shows it as a headline row ("Reference budget: yes"). The
owner's report is that the number this verdict is computed from looks like the
display's refresh interval rather than anything the renderer did.

Evidence packet, produced by
[`tools/phase3/run-visual-feasibility-baseline.ps1`](../../../tools/phase3/run-visual-feasibility-baseline.ps1)
on 2026-08-22 on the reference desktop (native Windows 11 Home `10.0.26200`,
Ryzen 9 9950X, NVIDIA GeForce RTX 5070 Ti driver `32.0.15.9186`, OpenGL
`4.6.0 NVIDIA 591.86`, MSVC `19.44.35228.0` Release, 2560x1440, 600 sampled
frames after 120 warmup frames):
`artifacts/phase3/2026-08-22/visual-feasibility-baseline-183850545/`.

Reproduction command, from that packet's own manifest:

```text
powershell.exe -NoProfile -ExecutionPolicy Bypass \
    -File .\tools\phase3\run-visual-feasibility-baseline.ps1 \
    -Width 2560 -Height 1440 -RefreshHz 60
```

Nothing crashed, nothing failed, and no measurement in the packet is
arithmetically wrong. The complaint is about what the graded number means.

## Investigation

**Observed result — the graded metric contains the swap.**
[`window_runtime.cpp:455-500`](../../../src/platform/window_runtime.cpp#L455-L500)
times one frame as `frame_end - frame_begin`, where `frame_begin` precedes
`prepare_performance_frame` and `frame_end` is taken *after*
`SDL_GL_SwapWindow(window)`
([line 479](../../../src/platform/window_runtime.cpp#L479)). The engine names
this honestly: it prints
`performance_timing_mode=serialized_gpu_query_and_swap`
([line 526](../../../src/platform/window_runtime.cpp#L526)). **The engine is not
lying about what it measures.**

**Observed result — that metric is the only timing the budget grades.**
[`core::within_performance_budget`](../../../src/core/performance.cpp#L83-L89)
tests exactly three things: `synchronized_frame.p95_ns`,
`synchronized_frame.p99_ns`, and `memory.peak_rss_bytes`. The `gpu_render_*`,
`cpu_submission_*`, and `snapshot_presentation_preparation_*` percentiles are
printed and never compared to anything. The runner then throws unless
`within_performance_budget` is `yes`
([`run-visual-feasibility-baseline.ps1:267`](../../../tools/phase3/run-visual-feasibility-baseline.ps1#L267)),
and the two Release-only CTests gate on the same value under its second name,
`within_provisional_low_budget`
([`CMakeLists.txt:1087`](../../../CMakeLists.txt#L1087)).

**Observed result — the recorded distribution is one refresh interval wide.**
From `measurements.json` in the packet above:

| `synchronized_frame_*_ns` | value |
| --- | --- |
| minimum | 6,739,600 |
| median | 6,947,700 |
| p95 | 6,985,000 |
| p99 | 7,009,100 |
| maximum | 7,147,700 |

All 600 samples fall inside a 408,100 ns band. The same packet's
`inventory.json` records the primary display's 2560x1440 mode as
`refresh_numerator = 7453125`, `refresh_denominator = 51782`, i.e.
**143.93273 Hz**, a period of **6,947,690 ns**. The recorded median is
6,947,700 ns — **10 ns away**, about 1.5 parts per million.
`current_refresh_hz` in the same file is `143`.

**Observed result — the work inside that frame is two orders of magnitude
smaller.** Same packet: `gpu_render_median_ns=87904`,
`cpu_submission_median_ns=174300`,
`snapshot_presentation_preparation_median_ns=1500`. Subtracting the two CPU
stages from the median frame leaves 6,771,900 ns in the query readback plus the
swap; the GPU's own timer accounts for at most 87,904 ns of that, so roughly
**96.2% of the median graded frame is neither CPU work the engine did nor GPU
work the GPU did.** (Percentiles do not add, so this subtraction is done on
medians and is an approximation, not an identity.)

**The reported mechanism does not hold, and the real one is worse.** The report
attributed the pinning to
[`scenario_runner.cpp:865`](../../../src/platform/scenario_runner.cpp#L865),
`configuration.request_vsync = true`. That line is real, but it is inside
`interactive_configuration()`, which is reached only by
`run_interactive_scenario`
([line 890](../../../src/platform/scenario_runner.cpp#L890)). **Every
performance path uses `bounded_configuration` instead** —
`run_visual_tracer_performance_scenario`
([lines 1106-1107](../../../src/platform/scenario_runner.cpp#L1106-L1107)),
`run_sheep_motion_performance_scenario`
([line 980](../../../src/platform/scenario_runner.cpp#L980)), and the paddock
performance smoke — and `WindowRunConfiguration::request_vsync` defaults to
`false` ([`window_runtime.hpp:30`](../../../src/platform/window_runtime.hpp#L30)).
`SDL_GL_SetSwapInterval` is therefore **never called** on a measured run
([`window_runtime.cpp:393`](../../../src/platform/window_runtime.cpp#L393) is
guarded by `request_vsync`).

Inference, with its limit named: the pacing comes from the platform's default
swap interval, which the engine neither sets, queries, nor records. That is
worse than an explicit `request_vsync = true` would be, because an explicit
request would at least be greppable. **This inference is the one thing here that
was not measured** — no GPU run was performed for this issue, by instruction —
and the `verify:` entries name the paired run that would settle it.

**Falsification attempted, and it changed the claim.** The narrowest thing that
would have to be true for "the metric is intrinsically refresh-locked" is that
every host produces a refresh-shaped number. It does not. Recorded observed
results on the earlier Intel UHD 630 laptop, through the same
`synchronized_frame` code path, are `2,864,800` ns p95
([`docs/setup/WINDOWS.md:516`](../../setup/WINDOWS.md#L516)), `2,273,000` ns p95
([line 530](../../setup/WINDOWS.md#L530)), and 3.4986 ms p95
([`ROADMAP.md:3384`](../../../ROADMAP.md#L3384)) — all far below any plausible
refresh period, so that host was not swap-paced. Those are recorded results
from other sessions, not runs performed here.

The corrected claim is therefore narrower and more useful: **the same metric
means different things on different hosts, and the repository records nothing
that lets a reader tell which.** Comparing this reference baseline against those
laptop numbers, or against a future run on another machine, is not valid, and
nothing in the packet warns of that.

**Observed result — what passing actually requires on this host.** If the frame
is quantised to the 6,947,690 ns refresh interval, the graded p95 can only take
values near 6.95 ms, 13.90 ms, 20.84 ms, … The budget being graded is
`performance_p95_budget_ns=16670000` and `performance_p99_budget_ns=20840000`.
So one missed refresh on every single frame (13,895,380 ns, a sustained 71.97
FPS) still passes p95, and only a sustained third interval fails — and it fails
p99 by 3,070 ns. **The gate's real meaning on this host is "sustains about 72
FPS at 143.93 Hz", not "p95 frame time is at or below 16.67 ms".**

Inference: since the measured non-swap work is ~264 µs at the median, per-frame
work could grow by roughly an order of magnitude and change the graded number by
nothing at all.

**Observed result — the packet's own `@60` label is not a measurement.** The
runner accepts the requested refresh if *any* reported mode is `>= $RefreshHz`
([`run-visual-feasibility-baseline.ps1:214-218`](../../../tools/phase3/run-visual-feasibility-baseline.ps1#L214-L218)),
so `-RefreshHz 60` passed against a 143.93 Hz display. The engine only checks
`refresh_hz > 0`
([`visual_tracer_configuration.cpp:101-104`](../../../src/platform/visual_tracer_configuration.cpp#L101-L104))
and echoes it; a `grep` for `refresh` across `src/` shows it never reaches
window creation, display-mode selection, or the swap. The packet therefore
records `visual_tracer_provisional_viewport=2560x1440@60` and
`visual_tracer_refresh_hz=60` beside an inventory that says the display ran at
143.93 Hz, and a later reader has no way to know the first pair is a floor and
the second is the fact.

**Observed result — nothing in the repository mentions this.** A case-insensitive
`grep` for `vsync`, `v-sync`, `swap interval`, `swap_interval`, and
`SetSwapInterval` across every `*.md`, `*.ps1`, `*.cmake`, `*.json`, and `*.txt`
outside `build/`, `artifacts/`, and `.git/` returns **zero** hits. The only two
occurrences in the repository are the two C++ lines cited above. Meanwhile
[`docs/plans/visual-feasibility-before-objective-loop.md:687-692`](../../plans/visual-feasibility-before-objective-loop.md#L687-L692)
presents "synchronized frame p95 at or below 16.67 ms" as a whole-program gate
and instructs "Record each new pass cost before deciding whether a separate pass
limit would change an implementation decision", and
[lines 416-418](../../plans/visual-feasibility-before-objective-loop.md#L416-L418) makes a
Phase 2 stopping condition out of shadows consuming "an unexamined share of the
frame budget". Those later outcomes are the reason this matters now.

**Observed result — scope is three call sites, not one.** Every path that sets
`performance_sample_frames` uses `bounded_configuration` and inherits the same
unrecorded swap interval:

| path | source | budget | graded by |
| --- | --- | --- | --- |
| `--visual-tracer-performance-smoke` | [`scenario_runner.cpp:1083`](../../../src/platform/scenario_runner.cpp#L1083) | `visual-feasibility-reference-high-v1`, p95 16.67 ms | the Phase 0 runner |
| `--sheep-motion-performance-smoke` | [`scenario_runner.cpp:970`](../../../src/platform/scenario_runner.cpp#L970) | `kTracer2LowProfilePerformanceBudget`, p95 16.67 ms | `wide_eye.opengl_sheep_motion_performance` |
| `--paddock-performance-smoke` | `RenderScenario::handcrafted_paddock_performance` | provisional Low, p95 16.67 ms | `wide_eye.opengl_handcrafted_paddock_performance` |

The two CTests are registered only for `CMAKE_BUILD_TYPE=Release`
([`CMakeLists.txt:747-756`](../../../CMakeLists.txt#L747-L756)) and pass on the
literal `within_provisional_low_budget=yes`, so on this host they are graded by
the same display-pinned number. They passed inside the same packet
(`ctest-release` 47/47).

**This is not a "suite passed while the defect existed" finding.** The tests do
exactly what they are written to do. Nothing in the repository ever asserted
that the metric excludes presentation wait.

### What is not wrong

Stated explicitly, because a fixer could easily overcorrect:

- The `gpu_render_*` and `cpu_submission_*` percentiles **are** meaningful, and
  they do show large headroom: GPU p95 114,304 ns and CPU submission p95
  237,800 ns against a 6,947,690 ns refresh interval.
- The memory half of the budget is unaffected:
  `process_peak_rss_bytes=76713984` against `1610612736`.
- The same-state repeat gates, the high-severity OpenGL gate, the active-renderer
  gate, the OpenGL-4.6 gate, the startup gate, and the package-size gate
  ([`run-visual-feasibility-baseline.ps1:233-281`](../../../tools/phase3/run-visual-feasibility-baseline.ps1#L233-L281))
  are all unaffected by this issue and remain valid evidence.
- The engine's own reporting is honest; the metric's name says `synchronized`
  and its mode string says `and_swap`.

## Root cause

The one number the performance gate grades — `synchronized_frame` p95/p99 —
spans `SDL_GL_SwapWindow`, and no measured path ever sets, queries, or records
the swap interval in force. On the reference desktop the platform default paces
that swap to the display, so the graded value collapses onto the 6,947,690 ns
refresh period and stops responding to renderer cost. The budget it is compared
against, 16,670,000 ns, is more than two refresh intervals, so the verdict
`within_performance_budget=yes` is produced by the display rather than by the
renderer, and nothing written down anywhere in the repository tells a reader
that.

## Expected behavior

[ADR 0001](../../decisions/0001-native-foundation.md#L60-L61) sets the High
target as "p95 <= 16.67 ms; p99 <= 20.84 ms" for a 2560x1440 60 Hz
configuration. A frame-time budget is a statement about the cost of producing a
frame; a measurement that is pinned by the presentation interval cannot
falsify it, and `docs/plans/visual-feasibility-before-objective-loop.md` builds
four later stopping conditions on the assumption that it can.

[AGENTS.md](../../../AGENTS.md) requires an observed result to name its build,
date, platform, sample, and method, and requires inference to be separated from
measurement. A packet that records a swap-paced frame time as a budget verdict,
with no record of the swap interval and with a `@60` label the run never
honoured, does not meet that bar — not because a number is wrong, but because
the method it was produced by is not recoverable from the artifact.

Either the graded metric excludes presentation wait, or the swap interval and
the display's real refresh rate are recorded beside it and the prose says what
the verdict means. This issue does not choose between those; see below.

## Fix notes

Scope is measurement and its record, not rendering. No gameplay rule, scenario,
oracle, camera, viewport, golden, or accepted capture is involved. Ownership
boundaries touched: `src/platform` (the timing loop and the swap),
`src/core` (the budget comparison), `tools/phase3` (the runner's gate and its
`review.md` row), and `docs/` (the plan and roadmap prose). No ADR boundary in
[`src/README.md`](../../../src/README.md) moves.

**Reproduce before changing anything.** The cheap decisive experiment is two
runs of the existing Release `--visual-tracer-performance-smoke` on the
reference desktop, identical except for the driver's OpenGL vertical-sync
setting, reported side by side. If `synchronized_frame_median_ns` leaves
6,947,690 ns when the swap interval is forced off, the inference in
`## Investigation` is confirmed and the rest follows. If it does not, this issue
needs re-diagnosing before any code changes.

**Candidate directions, none of them a decision.** Ranked by blast radius:

1. **Record, do not change.** Print the swap interval actually in force
   (`SDL_GL_GetSwapInterval`) and the display's real refresh rate as
   `key=value` state, and let the runner record both in `measurements.json`.
   This makes every existing packet interpretable and every future one
   comparable, and it changes no verdict. It is the smallest change that
   removes the misleading part.
2. **Grade something falsifiable.** Add the GPU and CPU-submission percentiles
   to `PerformanceBudget` and to `within_performance_budget`, so the pass/fail
   is computed from numbers the display cannot pin. This changes an accepted
   budget contract, so it is an owner decision and probably an ADR amendment
   rather than a QA fix.
3. **Disable the swap interval on measured paths only.** Tempting and the most
   dangerous: it silently changes what every future `synchronized_frame` number
   means, invalidating comparison against the 2026-08-22 baseline and against
   the recorded Intel UHD 630 figures, and it would need a versioned marker in
   the packet so old and new numbers are never compared. Do not do this without
   the owner explicitly accepting the discontinuity.

Constraints a fixer must respect:

1. **Do not weaken any other gate.** The high-severity, active-renderer,
   OpenGL-4.6, same-state-repeat, startup, and package gates in the runner are
   sound and independent; QA-009 is the precedent for how much damage a careless
   change to that runner can do.
2. **Do not retroactively edit the accepted packet.**
   `artifacts/phase3/2026-08-22/visual-feasibility-baseline-183850545/` is
   evidence of what was measured. If its interpretation changes, that belongs in
   a new record, not in an edit to the old one.
3. **Fix all three performance paths or state why not.** Two of them are graded
   by CTest, so a change that only teaches the Phase 3 runner about the swap
   interval leaves `wide_eye.opengl_*_performance` reporting the same
   uninterpretable pass.
4. **The `@60` label is a separate, smaller wrong.** Either validate the
   requested refresh against a mode that actually matches, or rename the field
   so it reads as the floor it is.

Suites that must pass: `wide_eye.performance_utilities` for the statistics and
budget helpers, and — on a native Release build with a display — both
`wide_eye.opengl_*_performance` tests, to show that whatever changes, the
existing gates still fire. `target:qa-check` for the tracker. The real closure
evidence is the manual paired run named in `verify:`.

Roadmap note, for the owner rather than the fixer: this is S3, so
[`docs/qa/README.md`](../README.md) asks only for one checkpoint or
decision-log line covering this session's batch, not a named mention. The
argument for S3 over S2 is that no behavior or rendering path is wrong — the
engine measures what it says it measures — and the argument for S3 over S4 is
that a headline pass/fail verdict and the baseline for four later plan phases
are both computed from it. It becomes S2 the first time a shadow, colour,
environment-detail, or atmosphere outcome is accepted on the strength of this
metric not moving.
