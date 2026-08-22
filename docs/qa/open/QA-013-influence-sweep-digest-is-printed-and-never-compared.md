---
id: QA-013
title: The influence debug view's sweep digest asserts nothing: 7,200 frames are hashed into `influence_debug_sweep_digest`, printed, and never compared, so a geometry regression moves the number and the test still exits 0
status: open
severity: S3
confidence: confirmed
area: tests
reporter: agent
reported: 2026-08-22
phase: 3
platform: windows
rule: tests/README.md
verify:
  - wide_eye.influence_debug_view
  - wide_eye.influence_debug_frame_dump
  - target:qa-check
  - manual:a negative probe showing the new digest assertion fails by name when the drawn geometry changes, reported with its exact failure marker
  - manual:the digest assertion run on a second toolchain (Clang 18 on Linux or WSL) before any literal digest value is pinned, so a platform-specific float result is not baked into the suite
---

## Symptom

Deferred finding, carried out of [QA-011](../closed/QA-011-influence-applied-lane-is-unreadable-against-dark-geometry.md)
by the owner's instruction to file it separately rather than fold it into a
colour fix. QA-011's investigation recorded it twice and its resolution repeats
it verbatim:

> **The digest correction in the investigation still stands and was not fixed
> here.** `influence_debug_sweep_digest` is still only printed, and `warm_digest`
> is still accumulated and never read […] That is a separate finding, left for
> its own issue.

The behavior, in the terms the deferral used: the headless influence debug-view
oracle hashes every drawn segment of 30 scenarios × 240 ticks into a 64-bit
FNV-1a digest, prints it as `influence_debug_sweep_digest=<n>`, and never
compares it to anything — not to a literal, not to a second run, not to the
second digest the same file accumulates. An unintended change to the drawn
geometry moves the printed number and the test still exits `0`.

Reproduction is one command with no GPU, on the already-configured tree:

```text
build\Windows\dev\wide_eye_influence_debug_view_tests.exe
```

or through CTest as `ctest --preset dev -R wide_eye.influence_debug_view`.

## Investigation

**Observed result — the digest's entire life, on this host, 2026-08-22.**
`sweep_digest` is initialised at
[`influence_debug_view_tests.cpp:208`](../../../tests/influence_debug_view_tests.cpp#L208),
fed once per frame at
[line 251](../../../tests/influence_debug_view_tests.cpp#L251), and printed at
[line 635](../../../tests/influence_debug_view_tests.cpp#L635). A `grep` for
`sweep_digest` across the repository returns exactly those three lines plus one
comment at [line 341](../../../tests/influence_debug_view_tests.cpp#L341). There
is no `==`, no `check(`, no golden, and no manifest anywhere on the value. Its
observed value on this host, from the `dev` and `release` binaries on
2026-08-22, is `9179670838601995633` in both; a `grep` for that literal across
the working tree, excluding `build/` and `.git/`, returns nothing.

**Observed result — the digest is a complete hash of the drawn geometry, which
is what makes the omission matter.** `mix_frame`
([lines 144-159](../../../tests/influence_debug_view_tests.cpp#L144-L159)) mixes
`segment_count` and then, per segment, all three `start`, `end`, and `color`
components, the `role`, the `channel`, `subject_id`, and `object_id`. That is
every member `DebugSegment` declares
([`influence_debug_view.hpp:94-104`](../../../src/render/influence_debug_view.hpp#L94-L104)) —
field-by-field rather than over the object's bytes, deliberately, so padding
cannot make the value ABI-dependent
([lines 127-130](../../../tests/influence_debug_view_tests.cpp#L127-L130)). The
mechanism is sound and complete. Only the comparison is missing.

**Observed result — the file's own two comments disagree about what the digest
does, and the older one is the false one.** At
[lines 204-207](../../../tests/influence_debug_view_tests.cpp#L204-L207):

> Every named scenario, every tick: the invariants that must hold for any
> published state at all, **plus the aggregate digest that makes an unintended
> change to the drawn geometry visible.**

At [lines 339-342](../../../tests/influence_debug_view_tests.cpp#L339-L342), 130
lines later, added by `c261d6f`:

> Nothing else in the suite would notice if that came back out: the framebuffer
> oracle passes on lane pixel counts and **the sweep digest is printed rather
> than compared**. This is the assertion that would.

The second is accurate. The first is the live instance of "a doc that describes
code that does not exist" — it is in the test source itself, where a later
reader is most likely to trust it and stop looking.

**Correction to the lead this issue was filed from — the prose claim in
`ROADMAP.md` has already been fixed, and this issue must not re-report it as
outstanding.** The phrase "a stable sweep digest" no longer appears anywhere:
`c261d6f` corrected it in both places that carried it, before this issue was
filed. `ROADMAP.md` now reads, at
[lines 2151-2160](../../../ROADMAP.md#L2151-L2160):

> The headless oracle swept 30 scenarios × 240 ticks, building 25,480 arrows,
> 4,800 attraction links, 2,400 alignment links and 1,916 heading targets with
> `0` unresolved neighbour IDs, `0` allocations, a bounded worst-case segment
> count of `246` against a `271` capacity, and a sweep digest.
> **Correction (2026-08-22):** that digest was described here as "stable", which
> the code does not support — `influence_debug_sweep_digest` is printed and never
> compared to anything, and the accumulated `warm_digest` is never read, so no
> assertion pins it. It is a reported number, not a regression signal […] The gap
> is a separate finding, not fixed here.

[`tests/README.md:158-160`](../../../tests/README.md#L158-L160) carries the
matching correction: "the sweep digest is printed for a reader and is not
compared to anything, so it is not a regression signal". **Both documents are
now truthful.** What remains is the code gap they describe, plus the stale
comment at `influence_debug_view_tests.cpp:206` that neither correction reached.
This is a smaller claim than "the roadmap records unsupported evidence", and it
is the accurate one.

**Observed result — what the test does assert, so the scope of the gap is not
overstated.** The file makes **22** named `check()` assertions, not the handful a
first reading suggests. In file order:

| # | Assertion name | Line | Covers |
| --- | --- | --- | --- |
| 1 | `scenario_available` | [240](../../../tests/influence_debug_view_tests.cpp#L240) | fixture guard |
| 2 | `every_published_influence_is_exactly_planar` | [493](../../../tests/influence_debug_view_tests.cpp#L493) | sweep |
| 3 | `every_frame_within_declared_capacity` | [494](../../../tests/influence_debug_view_tests.cpp#L494) | sweep |
| 4 | `worst_frame_within_declared_capacity` | [495](../../../tests/influence_debug_view_tests.cpp#L495) | sweep |
| 5 | `every_lane_has_exactly_one_tick` | [497](../../../tests/influence_debug_view_tests.cpp#L497) | sweep |
| 6 | `every_arrow_matches_published_vector` | [498](../../../tests/influence_debug_view_tests.cpp#L498) | sweep |
| 7 | `every_applied_stroke_carries_a_bright_casing` | [499](../../../tests/influence_debug_view_tests.cpp#L499) | sweep (added by `c261d6f`) |
| 8 | `the_applied_lane_color_is_still_near_black` | [501](../../../tests/influence_debug_view_tests.cpp#L501) | palette (added by `c261d6f`) |
| 9 | `every_link_matches_published_neighbor_id` | [503](../../../tests/influence_debug_view_tests.cpp#L503) | sweep |
| 10 | `every_published_neighbor_id_resolves` | [504](../../../tests/influence_debug_view_tests.cpp#L504) | sweep |
| 11 | `every_arousal_bar_matches_published_arousal` | [505](../../../tests/influence_debug_view_tests.cpp#L505) | sweep |
| 12 | `every_behavior_rung_count_matches_label` | [507](../../../tests/influence_debug_view_tests.cpp#L507) | sweep |
| 13 | `every_target_matches_published_motion_heading` | [509](../../../tests/influence_debug_view_tests.cpp#L509) | sweep |
| 14 | `every_balance_point_matches_published_observables` | [511](../../../tests/influence_debug_view_tests.cpp#L511) | sweep |
| 15 | `bounded_scenario_draws_no_clamped_arrow` | [540](../../../tests/influence_debug_view_tests.cpp#L540) | paired clamp |
| 16 | `unbounded_control_draws_a_clamped_arrow` | [541](../../../tests/influence_debug_view_tests.cpp#L541) | paired clamp |
| 17 | `diagnostic_scenario_available` | [550](../../../tests/influence_debug_view_tests.cpp#L550) | fixture guard |
| 18 | `repeated_build_is_identical` | [570](../../../tests/influence_debug_view_tests.cpp#L570) | determinism, one frame |
| 19 | `restarted_run_rebuilds_an_identical_frame` | [571](../../../tests/influence_debug_view_tests.cpp#L571) | determinism, one frame |
| 20 | `diagnostic_frame_is_not_empty` | [572](../../../tests/influence_debug_view_tests.cpp#L572) | determinism, one frame |
| 21 | `building_a_debug_frame_changes_no_published_state` | [597](../../../tests/influence_debug_view_tests.cpp#L597) | inertness |
| 22 | `building_a_debug_frame_does_not_allocate` | [610](../../../tests/influence_debug_view_tests.cpp#L610) | allocation |

Twelve of those run over the full 7,200-frame sweep and are strong: they check
each drawn primitive against the published evidence it claims to depict. **This
issue is not "the test asserts nothing."** It is that the one output the
repository named as the sweep's aggregate regression signal is the one output
with no comparison behind it.

**Observed result — the digest is not the only inert number, and saying so keeps
the finding honest.** Cross-referencing the 19 printed lines against the 22
assertions, on the 2026-08-22 `dev` run:

| Printed line | Value today | Backed by |
| --- | --- | --- |
| `influence_debug_worst_segment_count` | `266` | assertion 4 (`<= 291`) |
| `influence_debug_unresolved_neighbor_ids` | `0` | assertion 10 (`== 0`) |
| `influence_debug_bounded_clamped_arrows` | `0` | assertion 15 (`== 0`) |
| `influence_debug_unbounded_clamped_arrows` | `11` | assertion 16 (`> 0`) |
| `influence_debug_diagnostic_segment_count` | `244` | assertion 20 (`> 0`) |
| `influence_debug_build_allocations` | `0` | assertion 22 (`== 0`) |
| `influence_debug_total_arrows` | `26870` | **nothing** |
| `influence_debug_total_attraction_links` | `4800` | **nothing** |
| `influence_debug_total_alignment_links` | `2400` | **nothing** |
| `influence_debug_total_heading_targets` | `1916` | **nothing** |
| `influence_debug_scenarios_with_clamped_arrows` | `1` | **nothing** |
| `influence_debug_sweep_digest` | `9179670838601995633` | **nothing** |

The remaining seven lines print compile-time constants (`kScenarioNames.size()`,
`kInfluenceChannelCount`, the segment capacity, the two arrow-scale constants),
a literal `240`, and the pass marker.

**Observed result — one of those unpinned aggregates has already drifted, which
is what an inert number looks like in practice.** `ROADMAP.md` records `25,480`
arrows for the build that introduced this oracle (`5e0a6fa`); the same line
prints `26,870` here today, `+1,390`. The other three link/target totals are
unchanged, and the worst segment count's `246 → 266` is fully accounted for by
QA-011's casing (`+4` segments × 5 sheep). **The cause of the arrow drift is not
investigated in this issue and is not claimed to be a defect** — intervening
gameplay work between `5e0a6fa` and today (`0e9aeb0`, the QA-001 depenetration
fix, is the obvious candidate) changes trajectories, and `arrow_count` is
incremented only when a term clears `kInfluenceArrowMinimumLength`
([`influence_debug_view.cpp:185`](../../../src/render/influence_debug_view.cpp#L185)).
The point is narrower and certain: the number moved, no suite noticed, and
nobody had to attribute it. `sweep_digest` sits in the same column of the table
above. If the fixer cannot attribute the arrow drift while pinning these
numbers, that attribution is its own issue, not this one.

**Observed result — the second digest, and the strongest defence of it, checked
rather than dismissed.** `warm_digest`
([lines 603-608](../../../tests/influence_debug_view_tests.cpp#L603-L608)) is
initialised, fed on each of 600 rebuilds of the *same* frame, and then never
read: the only assertion in that block is `build_allocations == 0` at
[line 610](../../../tests/influence_debug_view_tests.cpp#L610). The obvious
counter-theory is that it is not meant to be read — that it is an optimizer sink
keeping `build_influence_debug_frame` alive so the allocation count means
something. Checked, and it does not survive as written:
`build_influence_debug_frame` is defined out of line in
[`influence_debug_view.cpp:515`](../../../src/render/influence_debug_view.cpp#L515),
which is a separate translation unit compiled into the test target
([`CMakeLists.txt:517-519`](../../../CMakeLists.txt#L517-L519)), and no preset,
`CMakeLists.txt`, or cmake module sets `INTERPROCEDURAL_OPTIMIZATION`, `-flto`,
or `/GL` — a `grep` for all three across `CMakeLists.txt`, `cmake/*.cmake`, and
`CMakePresets.json` returns nothing. Without interprocedural optimization the
call cannot be elided, so no sink is needed; and if one were, it would not need
to be a *digest*. Either reading leaves the same fact: 600 rebuilds of a frame
that the file already knows how to compare exactly are performed and discarded.

**Falsification attempted, and it changed the framing.** The narrowest thing
that would have to be true for "the digest is inert" to be wrong is that some
consumer outside this file reads it — a golden, an artifact manifest, a CI step,
a runner. Checked, all four:

- nothing under `tests/goldens/` mentions `influence` at all (`grep -rl`);
- `wide_eye.influence_debug_view` is registered at
  [`CMakeLists.txt:607-610`](../../../CMakeLists.txt#L607-L610) with exactly
  three properties at
  [lines 945-950](../../../CMakeLists.txt#L945-L950): `LABELS`,
  `PASS_REGULAR_EXPRESSION "influence_debug_view_result=pass"`, and
  `TIMEOUT 60` — the pass marker is the whole contract, and the digest line
  cannot match it;
- the shared `FAIL_REGULAR_EXPRESSION` at
  [lines 1155-1158](../../../CMakeLists.txt#L1155-L1158) matches
  `failure_stage=`, the three sanitizer strings, and nothing numeric;
- [`.github/workflows/linux.yml`](../../../.github/workflows/linux.yml) runs
  `ctest --preset dev` and tees the log to `artifacts/ci/linux-fast/test.log`,
  which is uploaded only `if: failure()` and compared to nothing.

The theory survived. What it also produced is the empirical half below.

**Observed result — the CTest verdict is provably independent of the digest,
demonstrated rather than reasoned.** With `PASS_REGULAR_EXPRESSION` set, CTest
requires that pattern and only that pattern; every other byte of stdout is
unread unless a `FAIL_REGULAR_EXPRESSION` claims it. Confirmed on this host with
CTest `3.31.6-msvc6` on 2026-08-22 using a throwaway fixture built with the same
technique as
[`assert-ctest-failure-regex.cmake`](../../../tests/assert-ctest-failure-regex.cmake)
(scratch directory outside the repository; nothing in the working tree was
modified): a test whose output carried the configured pass marker plus arbitrary
extra text passed, and the same output with a `FAIL_REGULAR_EXPRESSION` covering
that text failed with `Error regular expression found in output`. A digest line
is exactly "arbitrary extra text" to this configuration.

**Corroboration from the history, since the change already happened once.**
`c261d6f` added four casing segments per sheep and a new colour, moving the worst
segment count from `246` to `266`. Both are inputs to `mix_frame`
(`segment_count` directly, `color` per segment), so the digest necessarily moved.
Recorded in QA-011's resolution: `ctest --preset dev` 45/45 and
`ctest --preset release` 47/47 passed *after* that change, and the commit message
records both suites passing before it as well. No file recorded a digest, so
nothing needed updating and nothing objected. **That is the defect's behavior
observed in the wild, not predicted.**

### What is not wrong

A fix must preserve these, and a fixer should not "improve" them:

- **the digest's construction.** Field-by-field FNV-1a over every `DebugSegment`
  member, hashing `segment_count` first and skipping the array tail beyond it, is
  correct and deliberately padding-free.
- **the twelve sweep assertions.** They check drawn primitives against published
  evidence, which is a stronger statement than any digest can make, and they name
  their failure. A digest that fails says only "something moved".
- **the three determinism assertions.** `*first == *second` and
  `*first == *after_restart` use `InfluenceDebugFrame`'s defaulted `operator==`
  ([`influence_debug_view.hpp:247`](../../../src/render/influence_debug_view.hpp#L247)),
  which is an exact whole-frame comparison and strictly better than comparing
  hashes.
- **printing the digest.** A reader-facing number in the evidence block is
  useful. The defect is that it is *only* that while being described as more.

## Root cause

The digest was written as the sweep's aggregate regression signal and the
comparison step was never written. Because the value is emitted into the same
`std::cout` evidence block as six numbers that *are* asserted, and because the
comment above its accumulator claims it "makes an unintended change to the drawn
geometry visible", it reads to a later author — and read to the roadmap and
`tests/README.md` until `c261d6f` — as a check rather than as a report. The
second accumulator, `warm_digest`, is the same omission at a smaller scale: the
value is computed and dropped, with no assertion and no comment explaining what
it is for.

## Expected behavior

[`tests/README.md`](../../../tests/README.md#L1-L4) opens with the rule this
breaks: "Add test sources here when a tracer introduces a real invariant; **do
not add placeholder assertions.**" A digest accumulated over 7,200 frames,
printed beside asserted numbers, and compared to nothing is the shape that rule
names.

[`AGENTS.md`](../../../AGENTS.md) requires that an observed result be
distinguishable from an unverified claim, and that documentation "not describe
intended support as verified support". The two roadmap-side instances were
corrected in `c261d6f`; the instance at
[`influence_debug_view_tests.cpp:206`](../../../tests/influence_debug_view_tests.cpp#L206)
was not, and it is the one a maintainer reads before touching this file.

The Phase 3 exit gate requires that "Debug views explain surprising flock
responses without guessing" ([`ROADMAP.md:2686`](../../../ROADMAP.md#L2686)). A
view whose geometry can shift with no test naming the shift is one where the
next surprising frame cannot be attributed to a change or to the flock — which
is the guessing the gate excludes.

Ownership is unchanged by any fix here:
[`src/README.md`](../../../src/README.md#L141-L147) keeps the view reading
published snapshots only and holding no authoritative state, and
[ADR 0003](../../decisions/0003-project-owned-test-harness.md) keeps the harness
project-owned — no framework is to be introduced to express an assertion this
file can already express.

## Fix notes

Scope is one file,
[`tests/influence_debug_view_tests.cpp`](../../../tests/influence_debug_view_tests.cpp).
No engine source, no `CMakeLists.txt` registration, no published contract, no
format version, no golden, no budget. Blast radius is `wide_eye.influence_debug_view`
alone; the boundary in `src/README.md` is untouched because nothing under `src/`
changes.

**Reproduce first, and it costs one process:** run
`build\Windows\dev\wide_eye_influence_debug_view_tests.exe`, note
`influence_debug_sweep_digest=`, and confirm the exit status is `0` and that the
value appears in no other file.

**The decision the fixer must make, stated rather than made.** There are two
different properties on the table and they are not interchangeable:

1. **Repeatability within a build** — the same binary produces the same sweep
   twice. Cheap, toolchain-independent, and expressible today: run the sweep
   twice and require equal digests, or reuse the exact whole-frame `operator==`
   the determinism block already uses. This catches nondeterminism. **It does not
   catch an unintended geometry change**, which is what the comment at line 206
   claims and what QA-011's fix notes wanted.
2. **A pinned baseline across commits** — a literal digest constant that a
   geometry change must break, forcing the author to acknowledge it. This is the
   property actually claimed. It is also the one with a real risk, below.

**The risk that makes (2) more than a one-line change, checked here.** The digest
is identical — `9179670838601995633` — between the `dev` (MSVC Debug) and
`release` (MSVC Release) binaries on this host, so it is at least
configuration-stable on one toolchain. Whether it is stable across MSVC, Clang
18, and GCC 13 is **unverified**: the segments are `float`s derived from `double`
world state through `std::sqrt`/`std::hypot` and normalization, and last-bit
libm differences would move the hash without moving anything a reader would call
geometry. `-ffast-math` is not in play, but that is not sufficient to conclude
bit-identity. **Do not commit a literal digest until it has been produced on at
least one non-MSVC toolchain and compared** — the `verify:` entries name that.
If it turns out not to be portable, the honest forms are a
tolerance-bearing structural assertion (per-role and per-channel segment counts,
which are integers) plus (1), or a digest over quantized coordinates.

**Do this too, and it is not optional:** correct or delete the comment at
[lines 204-207](../../../tests/influence_debug_view_tests.cpp#L204-L207) so it
matches whatever the code ends up doing. Leaving a fixed digest under a comment
that overstates it would recreate the defect in a new place.

**`warm_digest` is a separate decision inside the same fix.** Either give it the
assertion it is one line away from — the 600 rebuilds are all of the same
snapshot, and `*first` is already in scope, so `warm_digest` can be required to
equal 600 mixes of `*first`, which pins "a warm rebuild is identical" over the
whole loop rather than over one frame — or delete it and say in a comment why
the loop exists. Do not leave it accumulated and unread.

Constraints:

1. **Do not weaken the twelve sweep assertions to make room.** They are the
   substantive coverage; the digest is an aggregate on top of them.
2. **Do not add a dependency.** [ADR 0003](../../decisions/0003-project-owned-test-harness.md)
   keeps this harness project-owned.
3. **Keep the printed evidence block stable in shape.** `influence_debug_view_result=pass`
   is the CTest contract ([`CMakeLists.txt:948`](../../../CMakeLists.txt#L948));
   other lines are read by humans and by the roadmap's evidence paragraphs.
4. **Prove the new assertion can fail.** `c261d6f` set the precedent for this
   file by confirming its casing assertion with a negative probe that made the
   suite exit `1` by name. Do the same here and record the marker; an assertion
   nobody has seen fail is the defect this issue is about.
5. **Do not touch `ROADMAP.md`'s or `tests/README.md`'s corrections while
   fixing.** They are accurate today. They become updatable only once the code
   supports a stronger statement, and that is a checkpoint edit, not part of the
   test change.

Suites that must pass: `wide_eye.influence_debug_view` (the changed test) and
`wide_eye.influence_debug_frame_dump` (the sibling oracle over the same view, to
show the frame text is untouched). `target:qa-check` for the tracker. The
display-backed `wide_eye.opengl_influence_debug_overlay` is *not* required — no
`src/` file changes — but running it costs nothing on the reference desktop and
would confirm that.
