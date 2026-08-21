---
id: QA-002
title: wide_eye.gameplay_simulation is within 800 KiB of the default 8 MiB stack and segfaults when a fixture is added
status: fixed
severity: S2
confidence: confirmed
area: tests
reporter: agent
reported: 2026-08-21
closed: 2026-08-22
fix: 674c078
phase: 3
platform: wsl-ubuntu-24.04
rule: tests/README.md
verify:
  - wide_eye.gameplay_simulation
  - wide_eye.gameplay_simulation_stack_budget
  - label:unit
  - label:sanitizer
---

## Symptom

`tests/gameplay_simulation_tests.cpp` builds one very large `main` that holds
roughly seventy `GameplaySimulation` objects alive by value at the same time.
Each one is about 114 KiB, so `main`'s frame alone is over 7 MiB. On a host with
the usual 8 MiB `ulimit -s` the test binary passes with under 800 KiB of
headroom, and adding one ordinary paired fixture — eight more simulations, about
940 KiB — makes it segfault before printing anything.

Observed result (2026-08-21, WSL Ubuntu 24.04.4, Clang 18.1.3, `dev` preset,
`ulimit -s` default 8192). Minimum stack at which the test exits 0, found by
running it under decreasing `ulimit -s` in 200 KiB steps:

| build | minimum stack | headroom under the 8192 KiB default |
| --- | --- | --- |
| `HEAD` (`eaa9cda`) | `7200 KiB` | `992 KiB` |
| the same tree plus one new paired fixture holding 8 simulations by value | `> 8400 KiB` | none — segfault, exit `139` |

The failure mode is the bad one: the process dies with SIGSEGV and exit `139`
having produced **no output at all**, so nothing names the fixture that broke it
and nothing says "stack". It looked like flakiness for two runs before the cause
was found.

Reproduced directly:

```
$ sizeof(GameplaySnapshot)=1912  sizeof(GameplaySimulation)=116976
$ ulimit -s 6144; ./build/Linux/dev/wide_eye_gameplay_simulation_tests
Segmentation fault (core dumped)
$ ulimit -s 65536; ./build/Linux/dev/wide_eye_gameplay_simulation_tests
(passes)
```

## Investigation

Observed result, same build and date. `GameplaySimulation` is 116,976 bytes.
Almost all of it is `SheepSpatialGrid sheep_grid_` in
[`gameplay_simulation.hpp`](../../../src/game/gameplay_simulation.hpp): the grid
carries the fixed 1,000-member capacity-experiment ceiling recorded in
[`src/README.md`](../../../src/README.md) while the game has five sheep, and the
simulation stores it by value. The two snapshots add only about 3.8 KiB.

Every named fixture in the test declares its simulations as locals of `main`,
and `main` never leaves a scope, so all of them are live simultaneously. The
`dev` preset is `CMAKE_BUILD_TYPE=Debug`, so Clang emits no stack colouring and
disjoint `{}` scopes would not reclaim anything either. Each paired-fixture
outcome in Phase 3 has added roughly six to eight simulations, so consumption has
grown by about 0.7–0.9 MiB per outcome.

Inference: this is a pre-existing latent defect, not something the current work
introduced. `HEAD` already needed 7200 of 8192 KiB. The combined-influence
outcome is simply the first one whose additions did not fit.

Unverified claim, stated as the next thing to check rather than as fact: an
ASan build should need materially more stack than the plain `dev` build because
of redzones and `detect_stack_use_after_return`, so `dev-sanitized` may be even
closer to the limit than the numbers above suggest. The sanitized suite passes
today on this host, and the minimum-stack sweep was not repeated for it.

Test coverage: nothing checks stack consumption, and no CTest property raises
the limit. The suite passes on this host today and would fail on any host or CI
runner with a smaller default.

## Root cause

`sizeof(GameplaySimulation)` is dominated by a 1,000-member spatial grid the
five-sheep game never fills, and `tests/gameplay_simulation_tests.cpp` keeps one
such object per fixture alive by value for the whole of a single enormous `main`.
Frame size therefore grows linearly with the number of fixtures, and the growth
rate is set by a capacity constant that has nothing to do with the test.

## Expected behavior

[`tests/README.md`](../../../tests/README.md) makes this executable the accepted
project-owned harness for gameplay simulation under
[ADR 0003](../../decisions/0003-project-owned-test-harness.md). Adding a fixture
to an accepted harness must not be able to crash it, and a crash must not be
silent: a suite that dies with no output cannot tell the owner which check
failed, which is the one thing the harness exists to do.

## Fix notes

Scope: `tests/gameplay_simulation_tests.cpp`, and possibly the storage of
`SheepSpatialGrid` in `GameplaySimulation`. No gameplay rule, contract version,
scenario, or published evidence needs to change; this is entirely about where
memory lives.

Interim mitigation already applied (2026-08-21, in the combined-influence
outcome, so the suite could pass at all): the new combined-influence oracle was
moved out of `main` into its own function and its eight fixtures are held by
`std::unique_ptr` rather than by value. That reduced the added cost from about
940 KiB to about 200 KiB, leaving the working tree at a 7400 KiB minimum. It
mitigates; it does not fix. The next paired fixture has roughly 800 KiB left.

Candidate directions, each needing an owner decision:

1. Hold every fixture in the test by pointer, not just the new ones. Smallest
   change, mechanical, and it removes the growth rate entirely. It leaves the
   single enormous `main` in place.
2. Split `main` into one function per fixture family. Better structure and the
   frames stop overlapping, but it is a large diff across a 2,900-line file and
   the reported values would have to be returned rather than read from scope.
3. Give `SheepSpatialGrid` heap-backed storage, or lower the 1,000-member
   ceiling to what the game actually uses. This is the root cause and it would
   shrink `GameplaySimulation` by roughly 100×, but it touches an accepted
   capacity experiment and a zero-allocation guarantee on the fixed-tick path.
   Any change here must re-prove that rebuild and query still allocate nothing.

Whichever is chosen, the fix should add the missing signal as well as the
missing headroom: there is currently no check that the harness runs within a
stated stack budget, and a silent SIGSEGV with no output is what made this cost
two debugging cycles to identify.

## Update — 2026-08-21, bounded speed and turning

Observed result (same host, build, and method): the requirement grew again, and
not through fixture count. Adding the motion-limit evidence buffer to the
published snapshot moved `sizeof(GameplaySnapshot)` from 1912 to 2152 bytes and
`sizeof(GameplaySimulation)` from 116,976 to 117,480, which is multiplied by the
roughly seventy simulations `main` still holds by value. The new oracle itself
contributes nothing: it lives in its own function and holds every fixture by
`std::unique_ptr`.

Measured on the 100 KiB grid: `ulimit -s 7400` exits 0, `7300` segfaults —
previously `7400` passed and `7200` segfaulted.

Inference, and the reason this update is worth recording: the growth rate is now
dominated by snapshot size rather than by how many fixtures a test declares. Any
future outcome that publishes a new per-sheep evidence array pays roughly 240
bytes × 70 ≈ 17 KiB of stack whether or not it adds a single fixture, so holding
new fixtures on the heap is no longer sufficient to keep the headroom from
shrinking. That strengthens the case for candidate direction 1 or 3 above over
continuing to mitigate per outcome.

## Update — 2026-08-21, obstacle and drop avoidance

Observed result (same host, build, and method): **the requirement grew again, by
the same mechanism and the same amount.** Publishing the per-sheep avoidance
record moved `sizeof(GameplaySnapshot)` from 2,152 to 2,352 bytes and
`sizeof(GameplaySimulation)` from 117,480 to 117,904, multiplied by the roughly
seventy simulations `main` still holds by value.

| grid | before (`cb7ff1e`) | after |
| --- | --- | --- |
| 200 KiB reporting grid, `dev` binary | `7400` exits 0, `7300` segfaults | unchanged |
| 20 KiB sweep, two comparable standalone builds | `7320 KiB` | `7360 KiB` |

The mitigation held on its own terms and did not help: the new oracle lives in
its own function and holds all thirteen of its fixtures by `std::unique_ptr`, so
it adds no `main` frame at all, and the requirement still moved 40 KiB. That is
the second consecutive outcome in which the growth came entirely from snapshot
size.

The record shape was also measured rather than assumed, because the obvious
mitigation is to fold new evidence into an existing record instead of adding an
array. It saves nothing. Measured with Clang 18 (`clang++-18 -std=c++23 -I src`):

```
separate: collision=8 avoidance=40 total_per_sheep=48
folded:   48
```

Padding absorbs the duplicated `subject_id` either way, so the choice between a
new per-sheep record and extending an existing one is an ownership decision with
no stack consequence.

Inference: candidate direction 3 — shrinking `SheepSpatialGrid`, whose
1,000-member ceiling is what multiplies every snapshot byte by seventy — is the
only listed direction that addresses the actual growth rate. Directions 1 and 2
move where fixtures live, and the last two outcomes have shown that fixture
placement is no longer what consumes the headroom. Roughly `830 KiB` remains.

## Update — 2026-08-21, behavior transitions and arousal

Observed result (same host, build, and method): **the requirement grew again,
but by one grid step rather than two.** Driving the four behavior states needed
no new per-sheep array — `arousal` and `behavior` already exist on `SheepState` —
so the only published addition was one `double` on the existing dog-stimulus
record. That moved `sizeof(GameplaySnapshot)` from 2,352 to 2,392 bytes and
`sizeof(GameplaySimulation)` from 117,904 to 118,040, the extra 56 bytes being
the new scenario configuration the simulation also stores by value.

| grid | before (`d5e3276`) | after |
| --- | --- | --- |
| 200 KiB reporting grid, `dev` binary | `7400` exits 0, `7300` segfaults | unchanged |
| 20 KiB sweep, two comparable standalone builds | `7360 KiB` | `7380 KiB` |

The record shape was measured rather than assumed again, and this time the
answer is the opposite of the avoidance one. Measured with Clang 18
(`clang++-18 -std=c++23 -I src`):

```
separate: head_dog_record=128 arousal_record=16 total_per_sheep=144
folded:   136
```

Padding absorbed the duplicated `subject_id` for the 40-byte avoidance record;
it does not absorb it for a 16-byte one. Extending the existing record therefore
saved 8 bytes per sheep — about 5.6 KiB of `main` frame — over publishing a
parallel array, on top of being the better ownership answer.

Inference, unchanged: the growth rate is still set by
`sizeof(GameplaySnapshot)` multiplied by the roughly seventy simulations `main`
holds by value, and candidate direction 3 — shrinking `SheepSpatialGrid`, whose
1,000-member ceiling is what does the multiplying — is still the only listed
direction that addresses it. What this outcome adds is a second mitigation worth
naming: **publish new per-sheep state on `SheepState` or on an existing evidence
record rather than as a new array**, which cost 20 KiB here against the previous
two outcomes' 40 KiB each. Roughly `810 KiB` remains.

## Resolution

Fixed on 2026-08-22 by taking candidate direction 1 and holding **every**
`GameplaySimulation` fixture in
[`gameplay_simulation_tests.cpp`](../../../tests/gameplay_simulation_tests.cpp)
on the heap — the sixty-one that `main` declared by value, plus the two transient
ones in `run_cadence` and `run_paddock_collision` — using the `SimulationHandle`
/ `make_simulation` helpers the newer oracles already used. Every handle is
constructed before the window that counts allocations, so every zero-allocation
oracle still covers exactly the 600 fixed updates it claims to.
Direction 3 was deliberately **not** taken: `SheepSpatialGrid`'s 1,000-member
ceiling and its zero-allocation guarantee are an accepted capacity experiment and
need their own outcome and ADR.

The missing signal the closing note asked for is now a CTest.
`wide_eye.gameplay_simulation_stack_budget` (labels `unit`, `headless`, plus
`sanitizer` in the sanitized preset) runs the harness through
[`tests/assert-stack-budget.cmake`](../../../tests/assert-stack-budget.cmake)
under `ulimit -s`, against the named budget
`WIDE_EYE_GAMEPLAY_SIMULATION_STACK_BUDGET_KIB` in `CMakeLists.txt`. It is
registered on Unix hosts only and probes `ulimit -s` before trusting it, so a
host that cannot set the limit reports the check skipped instead of failing.

### Evidence

Observed result, 2026-08-22, WSL Ubuntu 24.04.4, Clang 18.1.3.

Minimum stack on the issue's reporting grid,
`for kb in 8192 7600 7400 7300 7200 7000 6000 4000 2000 1000; do (ulimit -s $kb; ./build/Linux/dev/wide_eye_gameplay_simulation_tests >/dev/null 2>&1); echo "$kb -> $?"; done`:

| grid step | before | after |
| --- | --- | --- |
| `8192` … `7400` | `0` | `0` |
| `7300` … `1000` | `139` (SIGSEGV, no output) | `0` |

Finer sweeps of the same trees put the actual requirement at:

| build | before | after |
| --- | --- | --- |
| `dev` | `> 7300 KiB` (`7400` was the smallest passing grid step) | `185 KiB` (`180` segfaults) |
| `release` | not swept before | `160 KiB` (`155` segfaults) |
| `dev-sanitized` | not swept before | `220 KiB` (`215` fails with an ASan stack-overflow report, exit `1`) |

The harness's reported observations are unchanged. Its 182 lines of stdout are
byte-identical before and after — `sha256 3d8a88cbd23cfbfb6392b405d4e878623d67ff13214a462ab5c783133f871f75`
for the `HEAD` (`42c3fec`) `dev` binary and for the fixed `dev`, `release`, and
`dev-sanitized` binaries alike — and stderr is empty in both. A separate
whitespace-insensitive normalization of the whole file showed that the only
textual differences are the relocated helper comment, the declaration form, and
`.` becoming `->` at the use sites: no assertion, name, or printed line changed.

The budget is `512 KiB`, more than twice the largest measured requirement and a
sixteenth of the usual 8,192 KiB default. It was proved to bite: tightening it to
`128` made `wide_eye.gameplay_simulation_stack_budget` fail with
`failure_stage=stack_budget ... did not complete within 128 KiB of stack (result
'Segmentation fault')`, and restoring `512` made it pass again. The skip path was
exercised with `prlimit --stack=4194304:4194304` and a `8192` KiB budget, which
printed `'ulimit -s 8192' is unavailable on this host (reported ''); stack-budget
check skipped` and exited `0`.

Suites, all on the same host and date:

| check | result |
| --- | --- |
| `cmake --build --preset dev && ctest --preset dev` | 25/25 passed |
| `cmake --build --preset release && ctest --preset release` | 25/25 passed |
| `cmake --build --preset dev-sanitized && ctest --preset dev-sanitized` | 25/25 passed |
| `cmake --build --preset dev --target format-check` | passed |
| `cmake --build --preset dev --target clang-tidy-check` | passed, no diagnostics in the changed files |
| `cmake -DMODE=check -P tools/qa/qa-tracker.cmake` | passed |
| `git diff --check` | no output |

The suite count moves from 24 to 25 because of the new test. Raw output is kept
under the ignored
`artifacts/phase3/2026-08-21/qa-002-stack-headroom/`.

Not covered: this is a WSL run, not native Windows or native Linux OpenGL 4.6
evidence; none is needed, because nothing in this fix touches a render path. The
root cause named above — `sizeof(GameplaySimulation)` being dominated by the
spatial grid — is unchanged and remains a candidate for its own outcome.
