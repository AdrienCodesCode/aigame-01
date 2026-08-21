---
id: QA-004
title: Build presets do not pin the compiler, so Release is built by GCC while the docs and roadmap claim Clang 18
status: open
severity: S2
confidence: confirmed
area: build
reporter: agent
reported: 2026-08-22
phase: 3
platform: wsl-ubuntu-24.04
rule: CLAUDE.md
verify:
  - label:unit
  - label:scenario
  - label:headless
  - label:sanitizer
---

## Symptom

`CMakePresets.json` never sets `CMAKE_CXX_COMPILER`, so which compiler a preset
uses depends entirely on the environment at configure time. On this development
host the three configured build trees disagree:

```
dev            CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++-18
dev-sanitized  CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++-18
release        CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++      -> GCC 13.3.0
```

Observed result (2026-08-22, WSL Ubuntu 24.04.4), read from each
`build/Linux/<preset>/CMakeCache.txt`, with `/usr/bin/c++ --version` reporting
`c++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`.

The consequence is a claim-accuracy problem, not a build failure. Every recorded
verification paragraph of the form "on WSL Ubuntu 24.04.4 with Clang 18.1.3,
development, Release, and ASan/UBSan configurations each built and passed N
CTests" reads as though all three used Clang 18.1.3. On this host the Release
configuration used GCC 13.3.0.

## Investigation

`CMakePresets.json` defines `base` (generator, `BUILD_TESTING`,
`CMAKE_EXPORT_COMPILE_COMMANDS`, `WIDE_EYE_WARNINGS_AS_ERRORS`) and the three
presets that inherit it. None of them names a compiler, and `CMakeLists.txt`
contains no `CMAKE_CXX_COMPILER` assignment or compiler-identity requirement —
`grep -n "CMAKE_CXX_COMPILER\|clang" CMakeLists.txt` returns nothing.

Where Clang comes from, when it comes at all:

- [`.github/workflows/linux.yml:19-20`](../../../.github/workflows/linux.yml#L19-L20)
  sets `CC: clang-18` and `CXX: clang++-18` as workflow environment, then
  configures the `dev` preset. So CI is pinned by the workflow, not by the
  preset, and CI configures **only** `dev` — neither `release` nor
  `dev-sanitized` is built there.
- [`docs/setup/UBUNTU_24_04.md`](../../setup/UBUNTU_24_04.md) installs
  `clang-18` but never instructs exporting `CC`/`CXX`, and records at line 88
  that "GCC 13.3.0 was preinstalled".

Inference: a developer following the documented setup exactly gets GCC for every
preset. The local `dev` and `dev-sanitized` trees carry Clang because they were
configured in a shell where `CXX` happened to be set; `release` was configured
without it. Nothing in the repository would have made that divergence visible.

`CLAUDE.md` states "Toolchain: CMake ≥ 3.28, Ninja, Clang 18 (`clang-format-18` /
`clang-tidy-18` for the developer targets)", which is what the presets do not
enforce.

Falsification attempted: the theory requires that the presets really are silent
about the compiler and that nothing else supplies it. Both were checked directly
rather than inferred — the preset file was read in full and `CMakeLists.txt` was
grepped. The remaining uncertainty is historical, not present-tense: this issue
cannot establish which compiler built the Release trees behind older recorded
measurements, only that the current tree used GCC and that nothing pinned it.

Not a defect, and worth stating so it is not "fixed" by accident: building
Release under a second compiler has actual value. It has repeatedly proved the
code is not Clang-specific, and it caught at least one real portability problem
(GCC's `-Wmismatched-new-delete` rejecting an array-`new` arena in the test
harness that Clang accepted). The defect is that this is accidental and
undocumented, not that it happens.

## Root cause

`CMakePresets.json` omits `CMAKE_CXX_COMPILER`, so preset identity does not
determine toolchain identity. The documented toolchain is therefore enforced
only by CI's environment variables, and only for the one preset CI configures.

## Expected behavior

`CLAUDE.md` names a single toolchain for the project, and
`docs/DEVELOPMENT_WORKFLOW.md` requires that "verification actually run is
reported" and that documentation state observed truth with build, date,
platform, and method. A preset is the repository's "one canonical command path"
under the same document's context-discipline section. Either the presets pin the
compiler they claim, or the recorded evidence names the compiler each
configuration actually used — the current state satisfies neither.

## Fix notes

Scope is `CMakePresets.json`, the setup docs, and the wording of recorded
evidence. Two directions, and the choice belongs to the owner because it decides
what "Release" means for this project:

1. **Pin each preset** to `clang++-18`/`clang-18`. Makes the docs true and the
   configure reproducible, but it silently changes what the Release
   configuration *is* on this host, so every Release-based measurement should be
   re-run rather than assumed to carry over, and the accidental GCC coverage is
   lost unless a second preset restores it deliberately.
2. **Pin `dev`/`dev-sanitized` to Clang and add an explicit second preset** for
   the GCC build, turning today's accident into stated cross-compiler coverage.
   Larger change; documents reality instead of narrowing it.

Either way the fix should make a mismatch visible rather than silent — a
configure-time report of the compiler identity, or a check that fails when a
preset is configured with an unexpected one — because the failure mode here was
that nothing ever said which compiler ran.

Blast radius: no gameplay code. Every suite must still pass under whichever
compilers survive the decision, and `wide_eye.gameplay_simulation_stack_budget`
is compiler-sensitive by nature (the measured minimum stack differs between the
Clang and GCC builds), so its budget should be re-checked against both.
