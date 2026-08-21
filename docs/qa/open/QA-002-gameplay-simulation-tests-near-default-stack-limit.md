---
id: QA-002
title: wide_eye.gameplay_simulation is within 800 KiB of the default 8 MiB stack and segfaults when a fixture is added
status: open
severity: S2
confidence: confirmed
area: tests
reporter: agent
reported: 2026-08-21
phase: 3
platform: wsl-ubuntu-24.04
rule: tests/README.md
verify:
  - wide_eye.gameplay_simulation
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
