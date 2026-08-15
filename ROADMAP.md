# Wide Eye C++ voxel-engine roadmap

## Current checkpoint

- **Status:** Phase 1 is underway. The versioned `core`, `platform`, `render`,
  `voxel`, `game`, `tools`, and `tests` boundaries and a minimal C++23
  `wide_eye` executable now exist. The executable owns SDL 3.4.10 startup, a
  resizable window, resize/minimize/focus/close state, clean shutdown, an
  explicit OpenGL 4.6 Core debug-context request, context/driver reporting, and
  high-severity rejection. The `render` boundary owns GLSL 4.60 triangle and
  perspective voxel-cube pipelines, a same-camera triangle-wireframe diagnostic,
  color/depth/framebuffer oracles, top-left RGBA8 readback, and a
  dependency-free deterministic PNG writer. The `core` boundary owns a
  steady-clock frame timer, render-cadence-independent 60 Hz fixed-step
  accumulator with a 250 ms clamp, structured logging, and automation-safe
  fatal assertions. On 2026-08-15, the initial source-hashed native Windows
  candidate build with MSVC 19.44.35228.0 passed all 14 then-current development
  CTests on Intel UHD Graphics 630. Separate hidden capture CTests required two
  independent normal runs and two independent wireframe-debug runs to produce
  byte-identical PNGs. The normal capture is a validated 64x64 RGBA8 PNG with
  SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`;
  the matching wireframe capture has SHA-256
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`
  and passed a sparse-frame oracle with 253 visible pixels. Both paths preserved
  the intended depth state and zero high-severity messages. The owner then
  launched the interactive native Windows cube, confirmed that its resizable
  window worked, reviewed both captures, reported that they looked correct, and
  explicitly accepted the packet. The complete source packet is now the first
  checked-in [Tracer 0 visual baseline](tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md).
  Its manifest retains the source/worktree state, source hashes, platform/GPU
  data, commands, configuration, state, log, and exact normal/debug hashes. A
  registered platform-independent CTest requires every retained hash plus
  exactly one recorded Accept verdict. The root [`.gitattributes`](.gitattributes)
  prevents cross-platform text conversion from changing any hash-addressed
  packet byte. After promotion, a fresh source-hashed native Windows copy-build
  passed all 15 development CTests and reproduced both accepted hashes with zero
  high-severity messages; its ignored verification packet is
  [`windows-cube-smoke-223401700`](artifacts/phase1/2026-08-15/windows-cube-smoke-223401700/manifest.json).
  The WSL development, ASan/UBSan, and release builds and their eight default
  fast tests pass. Direct 4.6 context requests still fail as expected with
  `GLXBadFBConfig` because this WSL host exposes only 4.5. System `clang-format`
  and `clang-tidy` 18.1.3 pass the project-only checks. Native Linux graphics
  remains unverified. A controlled repeat-capture mismatch still supplies the
  retained failure-path evidence; it is not a baseline. A minimal
  [GitHub Actions Linux fast gate](.github/workflows/linux.yml) now targets
  Ubuntu 24.04 with Clang 18 and runs the exact checked-in `dev`
  configure/build/test sequence. In a clean export of the staged index on WSL,
  that sequence passed all 8 tests, including 4 `headless` tests and the
  accepted-packet integrity guard. A
  controlled copied-baseline failure then exited nonzero and populated each
  CTest diagnostic selected for CI upload; checksum-verified `actionlint`
  1.7.12 also accepted the workflow. Commit
  `fcb7d70f64ca3b1c37ffc1caf64670072a5070ca` was then pushed to `main`; its
  [hosted Linux fast gate](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894480357)
  completed in 1 minute 10 seconds on GitHub's Ubuntu 24.04 image and passed all
  8 tests, including 4 labeled `headless`. A separate, temporary revision
  changed only the expected accepted-manifest hash. Its
  [controlled hosted run](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894689894)
  failed the named baseline-integrity test, passed the other 7 tests, and
  uploaded the expected dependency, environment, configure, build, and CTest
  diagnostics. GitHub reported the artifact archive as 35,894 bytes; inspection
  of its downloaded contents confirmed the actual-versus-expected hash error and
  found no credential-pattern matches in a targeted scan. The remote probe
  branch was deleted after verification.
- **Current milestone:** Phase 1 — Tracer 0 native foundation; the repository
  build scaffold, SDL window lifecycle, context/debug gate, triangle and
  depth-tested cube, core timing/logging/assertion primitives, deterministic PNG
  capture, versioned artifact/failure packet, and a documented hidden-window
  reproduction, normal/debug review packet, explicit owner acceptance, and
  protected first visual baseline are complete. The minimal Linux CI definition,
  local validation, hosted known-good run, and controlled failure-artifact
  verification are complete.
- **Next action:** From a clean committed source export, configure, build, and
  test the `dev-sanitized` preset to finish the clean-tree preset gate; the `dev`
  path is already proven by the hosted presubmit. Then reconcile the remaining
  Tracer 0 executable/reproduction exit evidence. Do not expand into Phase 2
  while the remaining Phase 1 exit gates are unresolved.
- **Next-context files:** [`AGENTS.md`](AGENTS.md), this checkpoint, the
  [development workflow](docs/DEVELOPMENT_WORKFLOW.md),
  [workflow implementation plan](docs/plans/agentic-development-workflow.md),
  the [Linux workflow](.github/workflows/linux.yml),
  [`CMakePresets.json`](CMakePresets.json), and
  [`CMakeLists.txt`](CMakeLists.txt).
- **Last reviewed:** 2026-08-15.
- **Primary playtest question:** Can a first-time player intentionally steer five
  mixed-temperament sheep through one gate using only the dog's movement,
  facing, pressure, and release?

## How future context windows use this file

1. Read [`AGENTS.md`](AGENTS.md), this checkpoint, the current phase, and the
   accepted [`development workflow`](docs/DEVELOPMENT_WORKFLOW.md).
2. Inspect `git status` and preserve unrelated user work.
3. Verify the evidence behind checked items; a checkbox is not proof by itself.
4. Work on the first unblocked unchecked item in the current phase unless the
   user explicitly reprioritizes.
5. Do not start a later phase while the current exit gate is failing.
6. After work, update checkboxes, evidence links, decisions, measurements, and
   the current checkpoint in the same change.
7. Never check an item for a planned, mocked, or unrun result.

Supporting references:

- [Game design](docs/game-design/WIDE_EYE.md)
- [Broader herding gameplay direction](docs/game-design/HERDING_GAMEPLAY.md)
- [Herding simulation and scale research](docs/research/herding-simulation-and-scale.md)
- [Herding simulation and scale implementation plan](docs/plans/herding-simulation-and-scale.md)
- [Accepted native foundation decision](docs/decisions/0001-native-foundation.md)
- [C++ voxel-engine decision](docs/VOXEL_ENGINE_OPTION.md)
- [Agent harness and tooling](docs/AGENT_HARNESS_AND_TOOLS.md)
- [Development workflow and standardized feedback loop](docs/DEVELOPMENT_WORKFLOW.md)
- [Workflow implementation plan](docs/plans/agentic-development-workflow.md)
- [Human visual-review packet](docs/review/HUMAN_VISUAL_REVIEW.md)
- [C++ engine implementation prompt](prompts/cpp-voxel-game-engine.md)

## Phase 0 — Product constraints and toolchain

### Completed research

- [x] Define the border-collie player fantasy, core loop, and first playtest
  question. Evidence: [`WIDE_EYE.md`](docs/game-design/WIDE_EYE.md).
- [x] Compare the quick Three.js track with a custom native engine track.
  Evidence: [`TECH_STACK.md`](docs/TECH_STACK.md) and
  [`VOXEL_ENGINE_OPTION.md`](docs/VOXEL_ENGINE_OPTION.md).
- [x] Investigate LumenFall's public claims and establish a clean-room boundary.
  Evidence: [`VOXEL_ENGINE_OPTION.md`](docs/VOXEL_ENGINE_OPTION.md).
- [x] Investigate available skills, MCP candidates, official documentation, and
  the current local toolchain. Evidence:
  [`AGENT_HARNESS_AND_TOOLS.md`](docs/AGENT_HARNESS_AND_TOOLS.md).
- [x] Review the GPT transcript and generated reference images, then research
  measured sheep collective behavior, sheep-dog drives, and a scale-safe
  simulation path. Evidence:
  [`herding-simulation-and-scale.md`](docs/research/herding-simulation-and-scale.md).
- [x] Research and adversarially review an agentic development loop, verification
  cadence, visual owner gate, and context/token discipline. Evidence:
  [`agentic-development-workflow.md`](docs/research/agentic-development-workflow.md)
  and its [implementation plan](docs/plans/agentic-development-workflow.md).

### Approved decisions

- [x] Use the custom C++ engine—not the Three.js prototype—as the primary
  implementation track for the next milestone.
- [x] Develop and release for native x86-64 Linux and Windows. WSL2 development
  does not substitute for either native release test.
- [x] Use C++23, CMake presets, Ninja, SDL3, OpenGL 4.6 Core/GLSL 4.60, and
  doctest as the provisional foundation.
- [x] Use procedural-first media: no imported runtime media through Tracer 2,
  with a provenance-approved authored fallback from Tracer 3 when readability,
  accessibility, audio, or animal animation evidence justifies it.
- [x] Use the provisional low/high hardware classes and budgets for resolution,
  frame-time percentiles, memory, startup, and package size recorded in
  [`ADR 0001`](docs/decisions/0001-native-foundation.md).
- [x] Use host package managers for developer tools and immutable,
  checksum-verified CMake inputs for source dependencies; apply the license
  allowlist and review gates in [`ADR 0001`](docs/decisions/0001-native-foundation.md).
- [x] Use goal, relevant context, invariants, non-goals, and done-when evidence
  as the default coherent-outcome contract. Prescribe steps only when the
  process itself is required.
- [x] End each completed coherent outcome with an explicit fresh-chat,
  continue-chat, or compact-then-continue recommendation according to
  [`DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md).

### Workflow foundation

- [x] Define the standardized inspect/change/build/observe/review/preserve loop,
  proportional verification tiers, bug stop-loss, and review checklist.
  Evidence: [`DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md).
- [x] Define candidate artifact metadata, failure evidence, human visual review,
  and the rule that only owner acceptance promotes a golden. Evidence:
  [`HUMAN_VISUAL_REVIEW.md`](docs/review/HUMAN_VISUAL_REVIEW.md).
- [x] Add provisional checked-in `.clang-format` and bounded `.clang-tidy`
  configurations. Their execution remains unverified against engine code until
  the Phase 1 target and compilation database exist.
- [x] Ignore local build trees, tool/cache state, generated captures/profiles,
  and crash evidence while leaving future intentional golden locations
  trackable. Evidence: [`.gitignore`](.gitignore).

### Environment setup

- [x] Install or provide CMake, Ninja, Clang/clangd, GDB, `pkg-config`, SDL3
  development files, OpenGL/Mesa development files, and ccache if approved.
  Evidence: [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- [x] Record exact compiler, CMake, SDL, OpenGL driver/context, and OS versions.
  Evidence: [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- [x] Prove a hardware or software OpenGL context can run under the normal local
  session and the available virtual display (`xvfb-run`) where applicable.
  Observed result: explicit 4.5 Core debug contexts passed on both paths; the
  approved 4.6 request failed on both. Evidence:
  [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- [x] Retain OpenGL 4.6 as the first graphics baseline. WSL's observed 4.5
  ceiling is development-host evidence, not a supported-target failure or
  permission to fall back. Native Windows is the first hardware context gate;
  native Linux follows on an actual Linux installation or machine. Evidence:
  [`ADR 0001`](docs/decisions/0001-native-foundation.md),
  [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md), and
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Record reproducible setup commands without embedding machine secrets.
  Evidence: [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md) and the
  [`tools/phase0`](tools/phase0/) bootstrap/diagnostic sources.

### Phase 0 exit gate

- [x] All material product/platform/dependency decisions above are approved.
- [x] The native toolchain versions are recorded and a minimal compiler/context
  smoke test passes with the approved OpenGL 4.6 Core baseline. Evidence:
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] The project can proceed without installing an MCP server. The registered
  MCPs are optional inspection/debugging conveniences with CLI fallbacks.

## Phase 1 — Tracer 0: native foundation

### Repository and build

- [x] Create a clear source tree for `core`, `platform`, `render`, `voxel`,
  `game`, `tools`, and `tests` without speculative subsystem internals. Evidence:
  [`src/README.md`](src/README.md), [`src/`](src/), [`tools/`](tools/), and
  [`tests/`](tests/).
- [x] Add `CMakeLists.txt` and checked-in configure/build/test presets for
  development, sanitized development, and release. Evidence:
  [`CMakeLists.txt`](CMakeLists.txt) and [`CMakePresets.json`](CMakePresets.json).
- [x] Generate `compile_commands.json` for clangd. Observed result: the Linux
  development preset generated `build/Linux/dev/compile_commands.json` with the
  Clang 18 C++23 command, and [`.clangd`](.clangd) selects it.
- [x] Add `.gitignore` rules for build trees, captures, profiles, caches, and
  local user presets while preserving intentional golden artifacts. Evidence:
  [`.gitignore`](.gitignore).
- [x] Add a single command or documented preset sequence for configure, build,
  test, and run. Evidence: [README scaffold commands](README.md#native-scaffold-commands).
- [x] Enable strict warnings for project code without imposing them blindly on
  external dependencies. Observed result: target-scoped warnings-as-errors
  passed with Clang 18.1.3, GCC 13.3.0, and MSVC 19.44.35228.0. Evidence:
  [`WideEyeProjectOptions.cmake`](cmake/WideEyeProjectOptions.cmake).
- [x] Add AddressSanitizer and UndefinedBehaviorSanitizer coverage where the
  platform supports them. Observed result: the labeled scaffold test passed
  under Clang 18 ASan/UBSan on WSL and MSVC 19.44 ASan on native Windows on
  2026-08-15. Evidence: [`CMakePresets.json`](CMakePresets.json) and
  [`WideEyeProjectOptions.cmake`](cmake/WideEyeProjectOptions.cmake).
- [x] Validate `.clang-format` and the bounded `.clang-tidy` checks against the
  installed Clang version and real project code; expose one documented command
  without applying either policy to third-party dependencies. Observed result:
  locally provided Ubuntu LLVM 18.1.3 passed the `format-check` and
  `clang-tidy-check` targets against `src/platform/main.cpp` on WSL on
  2026-08-15. Evidence: [README scaffold commands](README.md#native-scaffold-commands)
  and [`WideEyeDeveloperTools.cmake`](cmake/WideEyeDeveloperTools.cmake).
- [x] Label automated CTest coverage as `unit`, `scenario`, `headless`,
  `sanitizer`, or `performance`; record `manual` evidence separately and keep
  the default affected-change suite fast. Observed result: the current process
  test reports `headless`, plus `sanitizer` in the sanitized preset.
- [x] Prevent CTest pass markers from masking project or sanitizer failure
  diagnostics. Observed result: every registered test rejects `failure_stage=`,
  ASan, LSan, and UBSan diagnostics, and a nested fixture proved all four
  rejection paths before the development, ASan/UBSan, release, and 12-test
  native Windows suites passed on 2026-08-15. Evidence:
  [`CMakeLists.txt`](CMakeLists.txt) and
  [`assert-ctest-failure-regex.cmake`](tests/assert-ctest-failure-regex.cmake).

### Executable smoke tracer

- [x] Open and close an SDL3 window cleanly. Observed result: SDL 3.4.10 created,
  pumped, destroyed, and shut down a bounded dummy-driver CTest in development,
  ASan/UBSan, and release presets on WSL Ubuntu; a normal WSLg smoke also passed
  through the X11 video driver. A source-hashed native Windows copy-build with
  MSVC 19.44.35228.0 passed both CTests and the normal Windows-driver smoke on
  2026-08-15. Evidence: [`main.cpp`](src/platform/main.cpp),
  [`CMakeLists.txt`](CMakeLists.txt),
  [`WideEyeDependencies.cmake`](cmake/WideEyeDependencies.cmake), and the
  [Windows smoke runner](tools/phase1/run-window-smoke.ps1).
- [x] Create an explicit OpenGL core context and report vendor, renderer,
  version, and GLSL version. Observed result: on 2026-08-15 the project requested
  OpenGL 4.6 Core without fallback, validated the returned version/profile, and
  reported Intel UHD Graphics 630, OpenGL 4.6, and GLSL 4.60 from a source-hashed
  native Windows MSVC 19.44.35228.0 copy-build. All three Windows development
  CTests and the direct `--context-smoke` passed. The same request failed with
  `GLXBadFBConfig` on the 4.5-limited WSL host, preserving the accepted baseline.
  Evidence: [`main.cpp`](src/platform/main.cpp),
  [`CMakeLists.txt`](CMakeLists.txt),
  [`WINDOWS.md`](docs/setup/WINDOWS.md), and the
  [Windows project runner](tools/phase1/run-window-smoke.ps1).
- [x] Install an OpenGL debug callback and fail tests on high-severity messages.
  Observed result: on 2026-08-15 a source-hashed native Windows MSVC
  19.44.35228.0 copy-build requested and validated a Core debug context,
  installed a synchronous callback, passed a driver-backed regression that
  injected a high-severity message and required smoke failure, then passed the
  normal Intel UHD Graphics 630 context smoke with zero high-severity messages.
  Evidence: [`main.cpp`](src/platform/main.cpp), [`CMakeLists.txt`](CMakeLists.txt),
  [`assert-opengl-high-severity.cmake`](tests/assert-opengl-high-severity.cmake),
  and [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Render a triangle, then one voxel cube with correct depth testing.
  Observed result: on 2026-08-15 a source-hashed native Windows development
  build passed all nine CTests and both direct render smokes on Intel UHD
  Graphics 630. It reported a 24-bit depth/8-bit stencil framebuffer, triangle
  center RGBA `99,127,155,255`, cube center RGBA `229,56,31,255` at depth
  `0.959411`, enabled `LESS` depth testing and depth writes, matching oracles,
  and zero high-severity messages. Evidence:
  [`triangle_renderer.cpp`](src/render/triangle_renderer.cpp),
  [`CMakeLists.txt`](CMakeLists.txt), and [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Handle resize, minimized window, focus, and clean shutdown. Observed
  result: the state-transition unit CTest and SDL dummy-driver event-mapping
  smoke cover pixel-size change, minimize/restore, focus loss/gain, close, and
  shutdown. They passed in WSL development, ASan/UBSan, and release presets and
  in the nine-test native Windows development run. Evidence:
  [`window_state.cpp`](src/platform/window_state.cpp),
  [`main.cpp`](src/platform/main.cpp), and [`CMakeLists.txt`](CMakeLists.txt).
- [x] Add monotonic frame timing, fixed simulation timing, logging, and
  assertions. Observed result: the core runtime CTest verified a steady clock,
  exactly 60 ticks for one second partitioned as either 100 10 ms frames or 10
  100 ms frames, and a 250 ms clamp producing 15 ticks. A wrapped death test
  verified the structured fatal-assertion diagnostic and nonzero exit on WSL
  and Windows; runtime and lifecycle paths emit structured logs. Evidence:
  [`runtime.cpp`](src/core/runtime.cpp),
  [`assert-core-assertion.cmake`](tests/assert-core-assertion.cmake), and
  [`CMakeLists.txt`](CMakeLists.txt).
- [x] Save a deterministic PNG frame from a named smoke scenario. Observed
  result: on 2026-08-15 the `voxel_cube_smoke` scenario read back a top-left
  RGBA8 frame before swap and wrote a validated 64x64 non-interlaced PNG on
  native Windows. The retained capture has SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`;
  it was inspected but not promoted to a golden. A platform-independent unit
  CTest also matched the exact known bytes for a 1x1 RGBA8 PNG. Evidence:
  [`png_writer.cpp`](src/render/png_writer.cpp),
  [`triangle_renderer.cpp`](src/render/triangle_renderer.cpp),
  [`main.cpp`](src/platform/main.cpp), and [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Run the smoke scenario under `xvfb-run` or another documented headless
  path and retain evidence. Observed result: the source-hashed native Windows
  runner passed the hidden-window capture CTest, which invoked the scenario
  twice and rejected unequal PNG hashes, then retained a direct capture, hash,
  and log under the ignored `artifacts/phase1/2026-08-15/` tree. The Windows
  development suite passed all 12 tests. Evidence:
  [`assert-deterministic-png.cmake`](tests/assert-deterministic-png.cmake),
  [`run-window-smoke.ps1`](tools/phase1/run-window-smoke.ps1), and
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Emit a versioned artifact manifest and preserve command, platform, logs,
  state, capture, and configuration when the smoke scenario fails. Observed
  result: on 2026-08-15, the source-hashed native Windows runner passed all 11
  development CTests and two direct cube captures, emitted a schema-version 1
  packet whose normal PNG retained SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`,
  and passed independent field/file/hash validation. A controlled repeat-capture
  mismatch then exited with code 2 at `capture-repeat-compare`, retained both
  divergent captures, log, parsed state, configuration, source hashes, commands,
  platform metadata, Git/worktree state, and a failure manifest; the independent
  validator accepted that packet. Evidence:
  [`run-window-smoke.ps1`](tools/phase1/run-window-smoke.ps1),
  [`assert-artifact-manifest.cmake`](tests/assert-artifact-manifest.cmake), and
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Produce a candidate cube visual-review packet with matching normal/debug
  evidence; do not promote a golden without the owner's explicit verdict.
  Observed result: the source-hashed native Windows runner passed 14 development
  CTests, including independent two-run deterministic checks for the normal and
  wireframe-debug PNGs, then emitted a same-camera packet whose normal/debug
  hashes are
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`
  and `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  The candidate review record and its manifest passed independent validation.
  The owner subsequently launched the interactive cube, confirmed that its
  resizable window worked, reviewed both captures, reported that they looked
  correct, and explicitly accepted them. The resulting checked-in
  [baseline record](tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md)
  retains the full evidence packet, and `wide_eye.accepted_tracer0_baseline`
  verifies its manifest, files, hashes, and single Accept verdict. A fresh
  native Windows run passed all 15 CTests and reproduced both accepted hashes.
  Evidence: [`main.cpp`](src/platform/main.cpp),
  [`triangle_renderer.cpp`](src/render/triangle_renderer.cpp),
  [`assert-deterministic-png.cmake`](tests/assert-deterministic-png.cmake),
  [`assert-artifact-manifest.cmake`](tests/assert-artifact-manifest.cmake), and
  [`run-window-smoke.ps1`](tools/phase1/run-window-smoke.ps1).
- [x] After the local loop is reliable, add minimal Linux CI using the same
  configure/build/test/headless commands and retain useful failure artifacts.
  Observed result: [`.github/workflows/linux.yml`](.github/workflows/linux.yml)
  configures, builds, and tests the `dev` preset with Clang 18 on Ubuntu 24.04,
  uses read-only repository permission and immutable action commits, and selects
  bounded environment, dependency, configure, build, and CTest diagnostics for
  14-day upload on failure. On 2026-08-15, the exact three commands passed 8/8
  tests, including 4 labeled `headless`, in a fresh WSL copy. Removing the
  copied accepted-review file made the same CTest command exit 8 and populated
  the selected test log, `LastTest.log`, and `LastTestsFailed.log` with the
  named failure. Checksum-verified `actionlint` 1.7.12 passed. This validates
  the source, staged checkout bytes, and local artifact paths. Commit
  `fcb7d70f64ca3b1c37ffc1caf64670072a5070ca` then passed the
  [hosted gate](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894480357)
  in 1 minute 10 seconds with 8/8 tests and 4 `headless` tests. A temporary
  one-character expected-hash probe produced a
  [controlled hosted failure](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894689894):
  7/8 tests passed, the named accepted-baseline guard failed with the intended
  actual-versus-expected hash diagnostic, and the workflow uploaded all selected
  failure files. This does not verify native Linux graphics.

### Phase 1 exit gate

- [ ] Development and sanitized presets build and test from a clean tree.
- [ ] The executable opens, renders, captures, and shuts down without sanitizer
  failures or high-severity GL debug messages.
- [ ] A future agent can reproduce the smoke capture using only repository
  documentation.
- [x] The owner accepts, revises, or rejects the Tracer 0 visual packet; only an
  accepted packet becomes the first visual baseline. Observed result: the owner
  explicitly accepted after checking the interactive resizable window and both
  captures; the [accepted packet](tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md)
  is protected by a passing manifest/hash/verdict CTest.
- [x] The minimal Linux presubmit reproduces the already-working local fast gate,
  or its remaining runner/context limitation is explicitly recorded. Observed
  result: the hosted known-good run passed 8/8 tests on Ubuntu 24.04, and a
  controlled failing revision uploaded the expected useful diagnostics. The
  headless gate does not claim native Linux OpenGL 4.6 graphics coverage.

## Phase 2 — Tracer 1: bounded voxel paddock

### Foundation boundaries before world growth

- [ ] Split the tracer-sized `run_window` lifecycle from named scenario runners
  before dog, camera, or world behavior expands it; preserve the current event,
  shutdown, and smoke-test evidence.
- [ ] Rename or split `TriangleRenderer` so its public ownership matches the
  triangle, cube, capture, and future mesh responsibilities before chunk
  rendering is added.
- [ ] Replace the hand-loaded OpenGL entry-point table with the approved pinned,
  checksum-verified generated loader before the renderer consumes a broader GL
  API. Evidence for the dependency policy:
  [`ADR 0001`](docs/decisions/0001-native-foundation.md).

### Voxel representation

- [ ] Define integer world, chunk, and local coordinates with negative-coordinate
  tests.
- [ ] Compare 16³ and 32³ chunk memory/rebuild behavior before selecting a size.
- [ ] Implement palette/material IDs and an explicit empty block.
- [ ] Implement safe get/set and dirty-region tracking.
- [ ] Test empty, full, boundary, adjacent, and edited chunks.

### Meshing and rendering

- [ ] Implement a correct naive exposed-face mesher first.
- [ ] Keep opaque, cutout, and translucent output separate even if only opaque is
  drawn initially.
- [ ] Bound vertex/index counts and reject overflow.
- [ ] Render one handcrafted paddock with ground, stone wall, red gate, and a
  distant barn landmark.
- [ ] Add a small palette, directional light, sky color, fog, and stable basic
  shadows only after geometry is correct.
- [ ] Add chunk bounds, face-normal, wireframe, and mesh-stat debug views.
- [ ] Capture identical-camera normal/debug frames and frame-time/memory data.

### Dog and camera placeholder

- [ ] Add a kinematic placeholder dog with predictable ground contact.
- [ ] Use simple analytic collision separate from voxel render geometry.
- [ ] Add gameplay and free-debug cameras.
- [ ] Support keyboard/controller input through named actions.
- [ ] Add restart and deterministic scenario selection.

### Phase 2 exit gate

- [ ] The bounded paddock is visually readable and reproducibly captured.
- [ ] Dog movement cannot tunnel through the representative wall/gate tests.
- [ ] Chunk/mesh debug data explains every visible face and missing face.
- [ ] A same-state normal/debug review packet receives an explicit owner verdict.
- [ ] Procedural terrain, streaming, LOD, and advanced post-processing remain out
  of scope.

## Phase 3 — Tracer 2: five sheep and one gate

### Deterministic simulation harness

- [ ] Run authoritative gameplay at a fixed 60 Hz tick independent of rendering.
- [ ] Define versioned seed, action-input, replay, and state-dump formats.
- [ ] Add named scenarios for calm gather, nervous sheep, stubborn sheep, split,
  collision, gate success, restart, and recovery.
- [ ] Verify the same replay produces the same outcome across repeated local
  runs; record any cross-platform determinism limit honestly.

### Sheep behavior

- [ ] Store hot sheep state contiguously with stable IDs, explicit behavior
  state, synchronous updates from an immutable prior snapshot, and no
  steady-state per-agent allocation.
- [ ] Implement a uniform spatial grid for bounded neighbor queries.
- [ ] Implement named, independently inspectable close-range repulsion,
  selected-neighbor attraction, and optional selected-neighbor alignment.
- [ ] Implement dog pressure from distance, approach velocity, facing, line of
  sight, terrain, and temperament.
- [ ] Implement obstacle/drop avoidance and bounded acceleration/turning.
- [ ] Implement ordinary, nervous, and stubborn temperaments.
- [ ] Implement settled, alert, driven, and recovering transitions plus an
  explicitly non-physiological arousal/recovery proxy.
- [ ] Add debug arrows/labels for every influence, chosen neighbor, arousal,
  target, state, and balance point.
- [ ] Test that randomness never masks unstable or unexplained steering.

### Behavior observability and early scale hygiene

- [ ] Emit centroid, mean radius, polarization, elongation, group speed,
  nearest-neighbor spacing, connected components, dog bearing/distance,
  response latency, split/rejoin time, and settle time from named scenarios.
- [ ] Unit-test observable definitions on hand-authored positions and velocities.
- [ ] Add non-player diagnostic fixtures for 5, 14, 25, and 100 sheep without
  making large flocks a Tracer 2 content requirement.
- [ ] Record spatial-grid build, neighbor selection, behavior, terrain query,
  snapshot, allocation, and total simulation costs separately.
- [ ] Compare alignment-on and alignment-off fixtures; retain explicit alignment
  only when measured behavior and legibility justify it.

### Objective loop

- [ ] Spawn one dog, farmer placeholder, five sheep, one gate, and destination
  pen.
- [ ] Implement one gather-and-drive farmer whistle.
- [ ] Implement explicit success, recoverable failure, restart, and concise
  coaching after repeated failure.
- [ ] Add minimal HUD showing only farmer signal, flock status, objective, and
  debug mode.

### Phase 3 exit gate

- [ ] A recorded input sequence deliberately moves all five sheep through the
  gate.
- [ ] The replay reaches the same objective result on repeated runs.
- [ ] Debug views explain surprising flock responses without guessing.
- [ ] Headless tests cover pressure direction, neighbor bounds, temperament,
  arousal/recovery, obstacles, split/rejoin, gate counting, and restart.
- [ ] The executable meets provisional frame-time and memory budgets with ample
  headroom.
- [ ] A short motion/contact-sheet packet plus matching debug/state evidence
  receives an explicit owner verdict on flock readability and causality.

## Phase 4 — Tracer 3: readable procedural art and feedback

### Art bible in code

- [ ] Approve grid scale, silhouette rules, palette/value hierarchy, material
  response, biome rules, lighting intent, and controlled variation limits.
- [ ] Define when a generated variation is invalid rather than accepting every
  seed.
- [ ] Produce identical-camera reference captures for the approved look.
- [ ] Record the approved look and animal-motion evidence through a visual-review
  packet; an agent cannot approve or overwrite the baseline.

### Code-generated animals

- [ ] Generate an articulated border collie from named body parts and joints.
- [ ] Generate readable procedural black/white markings.
- [ ] Generate articulated sheep with controlled body/wool/face variation.
- [ ] Implement dog idle, walk, sprint, crouch, turn, stop, head/gaze, ears, and
  tail cues.
- [ ] Implement sheep idle, locomotion, hesitation, alarm, bunching, and settling
  cues.
- [ ] Add slope-aware placement or simple foot correction only if visible
  evidence justifies it.
- [ ] Preserve simple authoritative collision regardless of visual complexity.

### Feedback and atmosphere

- [ ] Add farmer whistle and essential dog/sheep/environment audio, procedural or
  provenance-approved according to the Phase 0 decision.
- [ ] Add restrained dust/grass/contact feedback where it improves pressure and
  movement readability.
- [ ] Add accessible controls for camera sensitivity, inversion, shake, contrast,
  and reduced motion as applicable.

### Fresh-player gate

- [ ] Run the first-playable test with at least five fresh players and record
  build, platform, method, behavior, comments, and failures.
- [ ] Confirm at least four of five complete within ten minutes after a controls-
  only introduction, or revise the design instead of declaring success.
- [ ] Confirm players can explain why the flock turns/splits and at least three
  intentionally release pressure.
- [ ] Decide keep/change/simplify/pivot from evidence.
- [ ] Only after a positive core-loop decision, develop and approve the detailed
  reward, progression, session, and additional-animal design; do not infer it
  from the generated reference-image UI.

## Phase 5 — Tracer 4: procedural voxel world

- [ ] Define deterministic terrain inputs and versioned generation parameters.
- [ ] Add bounded hills and valleys while preserving navigable herding surfaces.
- [ ] Add biome palette rules and deliberate transitions.
- [ ] Generate stone walls, paths, gates, barns, trees, hedges, rocks, and
  landmarks from readable placement grammars.
- [ ] Reject invalid placements and unreachable objectives.
- [ ] Add caves, water, weather, or additional biomes only when the approved game
  design needs them.
- [ ] Add chunk serialization and migration/version behavior.
- [ ] Add background generation/meshing with explicit ownership, cancellation,
  stale-result rejection, and per-frame budgets.
- [ ] Preserve all handcrafted herding replays as regression scenarios.
- [ ] Capture comparable normal/debug evidence for the fixed seed set, including
  camera, generation parameters, validity results, and performance metadata.

### Phase 5 exit gate

- [ ] Ten fixed seeds produce traversable, coherent worlds without intersections,
  impossible gates, or unreadable flock routes.
- [ ] Generation and meshing latency stay within named budgets.
- [ ] The world supports the herding loop rather than distracting from it.
- [ ] The bounded fixed-seed visual packet receives an explicit owner verdict.

## Phase 6 — Tracer 5: measured scale and renderer depth

- [ ] Establish low/high hardware capture baselines before optimizing.
- [ ] Benchmark authoritative flock simulation at 5, 14, 25, 100, 250, 500,
  and 1,000 sheep with fixed scenarios and separate CPU-stage, GPU, allocation,
  and memory measurements.
- [ ] Compare 100-sheep group observables with smaller full-rate fixtures before
  treating higher counts as behaviorally valid.
- [ ] Treat 250-, 500-, and 1,000-sheep results as capacity evidence until camera
  readability and playtests show that scale adds decisions or fun.
- [ ] If simulation LOD, jobs, or GPU compute is proposed, prove a measured
  bottleneck first and compare behavior against the full-rate baseline.
- [ ] Add frustum culling and measure it.
- [ ] Add mesh caching and budgeted GPU upload queues and measure them.
- [ ] Add greedy meshing only if it improves the measured bottleneck without
  breaking boundary tests.
- [ ] Add streaming distance and eviction with explicit memory budgets.
- [ ] Add LOD only if draw distance remains a measured constraint.
- [ ] Evaluate RenderDoc and optionally RenderDoc MCP at this gate, not earlier.
- [ ] Consider SSAO, improved anti-aliasing, stylized water, volumetric atmosphere,
  PCSS, or reflections one at a time with identical-state evidence.
- [ ] Reject effects that reduce flock readability, temporal stability, or low-
  target performance.

### Phase 6 exit gate

- [ ] Frame-time percentiles, memory, startup, and chunk latency pass on named low
  and high targets.
- [ ] Captures show stable motion and no high-severity GL/debugger findings.
- [ ] Low/high profiles have intentional differences and tested defaults.
- [ ] Same-state normal/debug/motion evidence receives an explicit owner verdict
  before a renderer-depth candidate replaces an accepted baseline.

## Phase 7 — Tracer 6: product hardening

- [ ] Add settings persistence, safe save/load, schema/version handling, and
  recovery from interrupted writes.
- [ ] Add complete input remapping and supported controller coverage.
- [ ] Complete UI states, accessibility, localization architecture, and claimed
  locale tests.
- [ ] Add crash diagnostics and evaluate Sentry only if product/privacy
  requirements approve it.
- [ ] Add privacy-minimized analytics only after a specific playtest/product
  question requires it.
- [ ] Create reproducible release packaging, dependency licenses, symbols, smoke
  tests, and rollback instructions.
- [ ] Verify supported Windows/Linux hardware and driver matrix.
- [ ] Retain comparable native Linux/Windows startup, render, and smoke-capture
  manifests; WSL evidence is labeled separately.
- [ ] Run the bounded ultra production pass only after core gameplay and release
  readiness gates pass.

## Deferred ideas—not current scope

- [ ] Multiple farms or open world.
- [ ] Weather affecting scent, footing, or urgency.
- [ ] Lamb/adult attachment.
- [ ] Ducks, geese, goats, or cattle.
- [ ] A researched animal-learning progression in which each species introduces
  a distinct group behavior and player judgment.
- [ ] Up to 1,000 authoritative sheep as an actual gameplay requirement rather
  than a capacity benchmark.
- [ ] Multi-dog or cooperative play.
- [ ] Accounts, cloud saves, leaderboards, backend, or online competition.
- [ ] Strict one-megabyte executable or zero-dependency challenge.
- [ ] Advanced volumetric renderer feature parity with LumenFall's claims.

Checking a deferred item does not authorize implementation; move it into an
approved milestone first.

## Decision and evidence log

Add short dated entries here or link a dedicated decision record once
implementation begins.

- **2026-08-14 — Clean-room boundary:** LumenFall is inspiration only; its public
  repository did not provide source or a reuse license.
- **2026-08-14 — Tooling:** No MCP is required for Phase 0/1. Prefer normal CLI,
  official documentation, deterministic captures, and tests. External candidates
  and their adoption gates are recorded in
  [`AGENT_HARNESS_AND_TOOLS.md`](docs/AGENT_HARNESS_AND_TOOLS.md).
- **2026-08-14 — Scope:** The current game experiment remains five sheep, one
  dog, one farmer signal, one gate, success/failure/restart, and debug evidence.
- **2026-08-15 — Foundation:** The project owner selected the custom C++ engine
  as the primary track and approved native Linux/Windows releases plus the
  provisional C++23/CMake/Ninja/SDL3/OpenGL 4.6/doctest foundation. Evidence:
  [`ADR 0001`](docs/decisions/0001-native-foundation.md).
- **2026-08-15 — Assets:** Use procedural-first media. Tracer 0–2 remains free
  of imported runtime media; later authored exceptions require readability or
  feedback evidence plus complete provenance and license review.
- **2026-08-15 — Budgets and dependencies:** Low/high targets, frame-time,
  memory, startup, package-size limits, dependency pinning, and the license
  allowlist are accepted provisionally in
  [`ADR 0001`](docs/decisions/0001-native-foundation.md).
- **2026-08-15 — Optional MCPs:** `mcp-cpp` 0.2.2, GDB MCP at commit
  `605220a4bbbbbe2e87629f29dc1136fb970f6525`, and `renderdoc-mcp` 0.3.0 are
  installed and registered. They require a new Codex session and do not replace
  builds, tests, replays, or captures. DebugMCP remains pending explicit approval
  of its local debugger code-execution surface. Evidence:
  [`AGENT_HARNESS_AND_TOOLS.md`](docs/AGENT_HARNESS_AND_TOOLS.md).
- **2026-08-15 — Herding research and scale:** Keep five sheep as the correctness
  and first-fun gate, but make Tracer 2 data-oriented and observable from the
  start. Research-comparison fixtures use 14 and 100 sheep; later capacity
  benchmarks use 250, 500, and 1,000. No large count is a product promise until
  performance, behavior, camera readability, and playtests support it. Evidence:
  [`herding-simulation-and-scale.md`](docs/research/herding-simulation-and-scale.md)
  and its [implementation plan](docs/plans/herding-simulation-and-scale.md).
- **2026-08-15 — Reference boundary:** The two generated images supply
  mood/composition cues only. Their HUD, commands, score, inventory, minimap,
  camera, and task structure are not requirements. Broader gameplay hypotheses
  live in [`HERDING_GAMEPLAY.md`](docs/game-design/HERDING_GAMEPLAY.md).
- **2026-08-15 — Standardized feedback loop:** The owner approved the
  goal/context/invariants/evidence task contract, proportional verification
  cadence, reproducible failure artifacts, explicit human visual-baseline gate,
  and fresh/continue/compact context handoff. The `.clang-format` and
  `.clang-tidy` files were initially provisional; their subsequent validation
  is recorded below. Evidence:
  [`DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md), the
  [visual-review packet](docs/review/HUMAN_VISUAL_REVIEW.md), and the
  [implementation plan](docs/plans/agentic-development-workflow.md).
- **2026-08-15 — Phase 0 toolchain and context observation:** Ubuntu 24.04.4
  tooling was first provided through a verified ignored local fallback, then
  installed system-wide by the owner. Clang 18.1.3 built the pinned SDL 3.4.10
  diagnostic as C++23. The normal WSLg X11 session and `xvfb-run` both passed
  explicit OpenGL 4.5 Core debug-context creation and clean shutdown using Mesa
  25.2.8 `llvmpipe`; both rejected the approved 4.6 request with
  `GLXBadFBConfig`. This is development-host capability evidence, not native
  Linux/Windows release support. Evidence:
  [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- **2026-08-15 — Platform workflow and Windows inventory:** The owner retained
  OpenGL 4.6, selected WSL Ubuntu for everyday non-hardware work, selected native
  Windows for the first real-GPU build/render gate, and deferred native Linux
  proof to an actual Linux installation or machine. Windows 11 build 26200
  reports Intel UHD Graphics 630 and NVIDIA GTX 1050 Ti Max-Q adapters, but no
  native CMake, Ninja, MSVC/clang-cl, Visual Studio, or `vswhere` was present in
  the initial inventory, so the Windows compiler/context gate was unrun at that
  point. The following entry records its subsequent completion. Evidence:
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- **2026-08-15 — Native Windows Phase 0 gate:** Visual Studio Build Tools
  17.14.37 provided CMake 3.31.6-msvc6, Ninja 1.12.1, and MSVC 19.44.35228.0.
  The pinned SDL 3.4.10/C++23 diagnostic built natively and passed the explicit
  OpenGL 4.6 Core debug-context request three times, including an initial build
  and cached reruns. Windows selected the Intel UHD Graphics 630 hardware
  renderer (driver 27.20.100.9664), which reported OpenGL 4.6 and GLSL 4.60.
  This completes Phase 0 but is not a full performance or release-support
  claim. Evidence: [`WINDOWS.md`](docs/setup/WINDOWS.md).
- **2026-08-15 — Project developer checks:** Ubuntu LLVM 18.1.3
  `clang-format` and `clang-tidy` were first provided through the ignored local
  fallback, then installed system-wide by the owner. The documented
  `format-check` and `clang-tidy-check` CMake targets prefer the system tools,
  fall back locally, and passed against the SDL lifecycle source while the
  development and ASan/UBSan tests passed. The targets enumerate only Wide
  Eye-owned source files and do not apply either policy to fetched dependency
  sources. Evidence: [README scaffold commands](README.md#native-scaffold-commands),
  [`WideEyeDeveloperTools.cmake`](cmake/WideEyeDeveloperTools.cmake), and
  [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- **2026-08-15 — SDL window lifecycle:** The main build now fetches the pinned,
  checksum-verified SDL 3.4.10 source, disables unowned audio/controller/rendering
  subsystems. Its bounded `--window-smoke` regression opens and closes a native
  window without creating a GL context. The dummy-driver scenario passed in
  development, ASan/UBSan, and release presets; a normal WSLg run reported
  `video_driver=x11`. The native Windows development preset passed both then-
  current CTests with MSVC 19.44.35228.0, and its normal smoke reported
  `video_driver=windows`. Native Linux remains unverified. Evidence:
  [`main.cpp`](src/platform/main.cpp),
  [`CMakeLists.txt`](CMakeLists.txt),
  [`WideEyeDependencies.cmake`](cmake/WideEyeDependencies.cmake), and the
  [Windows smoke runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — Project OpenGL context reporting:** The interactive executable
  and bounded `--context-smoke` now request OpenGL 4.6 Core, validate the actual
  version and Core-profile bit, report vendor/renderer/GL/GLSL strings, and shut
  down cleanly without rendering. A source-hashed native Windows MSVC
  19.44.35228.0 copy-build passed three CTests and the direct context smoke on
  Intel UHD Graphics 630, reporting OpenGL 4.6 and GLSL 4.60. Direct development
  and sanitized requests on WSL failed at context creation with the previously
  observed `GLXBadFBConfig`; no lower or compatibility fallback was added.
  Native Linux remains unverified. Evidence: [`main.cpp`](src/platform/main.cpp),
  [`CMakeLists.txt`](CMakeLists.txt), [`WINDOWS.md`](docs/setup/WINDOWS.md), and
  the [Windows project runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — First triangle:** The `render` boundary now loads its required
  OpenGL entry points, compiles and links GLSL 4.60 shaders, owns a VAO/VBO, and
  draws one colored triangle. A dedicated hidden smoke reads the center pixel
  before presentation and rejects the clear color. A source-hashed native
  Windows MSVC 19.44.35228.0 copy-build passed five development CTests and the
  direct triangle smoke on Intel UHD Graphics 630; the sampled center was
  `99,127,155,255` with zero high-severity messages. Development, ASan/UBSan,
  and release builds plus their default fast tests passed on WSL, but its 4.5
  ceiling prevents executing the accepted 4.6 triangle path there. The cube,
  depth testing, PNG capture, visual packet, native Linux proof, and broad GPU
  support remain unverified. Evidence:
  [`triangle_renderer.cpp`](src/render/triangle_renderer.cpp),
  [`main.cpp`](src/platform/main.cpp), [`CMakeLists.txt`](CMakeLists.txt),
  [`WINDOWS.md`](docs/setup/WINDOWS.md), and the
  [Windows project runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — Cube, lifecycle, and runtime foundation:** The interactive
  executable now renders a perspective voxel cube with explicit depth state,
  tracks drawable resize, minimize/restore, focus, and close transitions, and
  advances a steady-clock 60 Hz fixed-step accumulator independently of render
  cadence. Core logging and fatal assertions have automated checks. A
  source-hashed native Windows MSVC 19.44.35228.0 copy-build passed all nine
  development CTests and both direct render smokes on Intel UHD Graphics 630.
  The cube oracle observed a D24S8 framebuffer, center RGBA `229,56,31,255` at
  depth `0.959411`, `LESS`, depth writes, and zero high-severity messages. WSL
  development, ASan/UBSan, and release presets passed their five fast tests,
  plus project formatting and bounded static analysis. At that checkpoint, PNG
  capture, headless graphics reproduction, the artifact manifest, visual review,
  and native Linux proof remained open; the following entry resolves only the
  first two. Evidence: [`runtime.cpp`](src/core/runtime.cpp),
  [`window_state.cpp`](src/platform/window_state.cpp),
  [`triangle_renderer.cpp`](src/render/triangle_renderer.cpp),
  [`WINDOWS.md`](docs/setup/WINDOWS.md), and the
  [Windows project runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — Deterministic cube capture:** The named `voxel_cube_smoke`
  scenario now supports `--capture <png-path>`. The renderer reads the RGBA8
  framebuffer before swap and normalizes OpenGL row order; a project-owned,
  dependency-free writer emits fixed-filter, uncompressed PNG bytes. A known-
  byte unit test passed in WSL development, ASan/UBSan, and release builds. A
  source-hashed native Windows MSVC 19.44.35228.0 copy-build passed all 11
  development CTests, including a hidden two-run hash comparison, and retained
  a valid 64x64 capture with SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`.
  Agent inspection found the expected colored cube against the dark clear
  color, but at that checkpoint no golden or owner visual verdict existed. The
  artifact manifest,
  failure packet, visual-review packet, and native Linux graphics proof remain
  open. Evidence: [`png_writer.cpp`](src/render/png_writer.cpp),
  [`assert-deterministic-png.cmake`](tests/assert-deterministic-png.cmake),
  [`WINDOWS.md`](docs/setup/WINDOWS.md), and the
  [Windows project runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — Candidate cube visual packet:** The renderer gained a bounded
  `--voxel-cube-debug-smoke` wireframe view that reuses the normal cube geometry,
  camera, viewport, shader, and depth state. A source-hashed native Windows MSVC
  19.44.35228.0 copy-build passed 14 development CTests on Intel UHD Graphics
  630, including two-run deterministic PNG checks for both normal and debug
  views. The runner emitted the ignored
  [`windows-cube-smoke-220642406`](artifacts/phase1/2026-08-15/windows-cube-smoke-220642406/review.md)
  candidate packet; its normal PNG retained SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`,
  its debug PNG has SHA-256
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`,
  and all direct render runs reported zero high-severity GL messages. Agent
  inspection found a matching normal cube and explanatory wireframe; this is
  not an owner verdict. The manifest and controlled mismatch packet passed
  independent file/hash validation. No golden was created or promoted.
- **2026-08-15 — Accepted Tracer 0 visual baseline:** The owner launched the
  native Windows interactive cube, verified the resizable window, reviewed the
  normal and wireframe captures together, reported that both looked correct,
  and explicitly selected Accept. The complete packet was promoted to the
  checked-in
  [`voxel_cube_smoke-v1` baseline](tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md).
  Its CTest guard validates all manifest-linked file hashes and requires exactly
  one Accept verdict with an owner observation/date. WSL development,
  ASan/UBSan, and release presets each passed 8/8 tests after promotion. A fresh
  native Windows MSVC 19.44.35228.0 copy-build passed 15/15 tests and reproduced
  the accepted normal/debug hashes with zero high-severity GL messages. This is
  a reference-machine engineering-tracer baseline, not a cross-GPU identity,
  gameplay, motion, performance, or production-art claim.
- **2026-08-15 — Minimal Linux CI:** The new
  [Linux fast gate](.github/workflows/linux.yml) targets GitHub's Ubuntu 24.04
  image, selects Clang 18, installs only the current X11/OpenGL build
  dependencies, and runs the documented `cmake --preset dev`,
  `cmake --build --preset dev`, and `ctest --preset dev` sequence. Checkout and
  failure-upload actions are pinned to their v7.0.1 commit SHAs; repository
  permission is read-only and checkout credentials are not persisted. A fresh
  WSL source copy passed 8/8 tests, and a copied-baseline failure confirmed the
  selected CTest diagnostics. The workflow passed checksum-verified
  `actionlint` 1.7.12. A clean staged-index export preserved every accepted
  packet byte and passed all 8 tests after [`.gitattributes`](.gitattributes)
  disabled text conversion under `tests/goldens/`. The resulting commit
  `fcb7d70f64ca3b1c37ffc1caf64670072a5070ca` passed its
  [GitHub-hosted run](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894480357)
  on Ubuntu 24.04 with Clang 18: configure, build, and all 8 tests succeeded in
  1 minute 10 seconds, including 4 `headless` tests. A temporary revision then
  changed only the expected accepted-manifest hash; its
  [hosted run](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894689894)
  failed only `wide_eye.accepted_tracer0_baseline` and successfully uploaded the
  selected failure bundle. Downloaded inspection found all five workflow logs,
  `CMakeConfigureLog.yaml`, `LastTest.log`, and `LastTestsFailed.log`; the named
  hash mismatch was actionable, and a targeted credential-pattern scan returned
  no matches. Native Linux OpenGL 4.6 remains unverified.
