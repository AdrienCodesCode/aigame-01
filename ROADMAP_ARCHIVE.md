# Wide Eye roadmap archive — completed phases

Archived verbatim from `ROADMAP.md` on 2026-08-21 to keep the live
cross-context roadmap small. These are the completed-phase evidence records:
every checkbox and observed result below is unchanged from the pre-archive
roadmap, and this file sits beside `ROADMAP.md` so all relative links resolve
exactly as they did there. One mechanical repair was applied during archival:
two links to the renamed `README.md#native-scaffold-commands` anchor now point
to `README.md#native-engine-quick-start`. The live [`ROADMAP.md`](ROADMAP.md)
keeps each phase's summary and exit-gate status; the current phase and later
phases remain in the live file only.

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
  focused project-owned C++ test executables orchestrated by CTest as the
  provisional foundation. The initial doctest choice was superseded before
  adoption by [`ADR 0003`](docs/decisions/0003-project-owned-test-harness.md).
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
  test, and run. Evidence: [README native engine quick start](README.md#native-engine-quick-start).
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
  2026-08-15. Evidence: [README native engine quick start](README.md#native-engine-quick-start)
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
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
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
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
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
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
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

- [x] Development and sanitized presets build and test from a clean tree.
  Observed result: source commit
  `fcb7d70f64ca3b1c37ffc1caf64670072a5070ca` passed the hosted Ubuntu 24.04
  Clang 18 `dev` configure/build/test sequence with 8/8 tests. Commit
  `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f`, whose intervening changes are
  documentation only, was then exported with `git archive` on WSL Ubuntu
  24.04.4 and passed the Clang 18.1.3 `dev-sanitized` configure, 244-step build,
  and 8/8-test sequence with strict warnings plus ASan/UBSan. Seven tests carried
  the `sanitizer` label, four carried `headless`, and the retained logs contain
  no project failure marker or sanitizer diagnostic. Evidence: the
  [hosted `dev` run](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894480357),
  [`CMakePresets.json`](CMakePresets.json), and the ignored
  [`dev-sanitized` verification packet](artifacts/phase1/2026-08-15/linux-clean-dev-sanitized-b4d5d5c/manifest.json).
- [x] The executable opens, renders, captures, and shuts down without sanitizer
  failures or high-severity GL debug messages. Observed result: on native
  Windows 11 with Intel UHD Graphics 630, MSVC 19.44.35228.0 applied
  AddressSanitizer to all six project compile commands and `wide_eye.exe`, built
  the source-hashed `dev-sanitized` copy, and passed 15/15 CTests (14 labeled
  `sanitizer`, 11 `headless`). Five direct render/capture invocations reported
  clean shutdown and zero high-severity GL messages; the repeated normal and
  debug captures matched the accepted hashes. The final diagnostic scan and
  independent manifest/file/hash validation passed. Evidence: the ignored
  [`native Windows sanitizer packet`](artifacts/phase1/2026-08-15/windows-sanitized-cube-smoke-234237930/manifest.json)
  and the [Windows setup record](docs/setup/WINDOWS.md#observed-windows-host--2026-08-15).
- [x] A future agent can reproduce the smoke capture using only repository
  documentation. Observed result: in a fresh context on 2026-08-15, an agent
  followed `docs/setup/WINDOWS.md` and `tools/phase1/run-window-smoke.ps1`
  without relying on the authoring context. The native Windows 11/MSVC
  19.44.35228.0 `dev` copy-build passed 15/15 CTests (11 `headless`), all five
  direct render/capture paths reported clean shutdown and zero high-severity GL
  messages on Intel UHD Graphics 630, and the repeated normal plus debug PNGs
  matched the accepted hashes. The documented independent validator accepted
  the packet and no missing or ambiguous instruction was observed. Evidence:
  the ignored
  [`independent reproduction packet`](artifacts/phase1/2026-08-15/windows-cube-smoke-235312691/manifest.json)
  and the [Windows setup record](docs/setup/WINDOWS.md#observed-windows-host--2026-08-15).
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

- [x] Split the tracer-sized `run_window` lifecycle from named scenario runners
  before dog, camera, or world behavior expands it; preserve the current event,
  shutdown, and smoke-test evidence. Observed result: on 2026-08-16,
  [`window_runtime`](src/platform/window_runtime.cpp) became the sole owner of
  SDL/OpenGL initialization, event polling, presentation, diagnostics, and
  teardown, while [`scenario_runner`](src/platform/scenario_runner.cpp) took
  scenario configuration, render-resource lifetime, framebuffer oracles, and
  optional capture ownership. The CLI remains unchanged. WSL Clang 18.1.3
  `dev` and ASan/UBSan builds each passed 8/8 CTests, and the project-only format
  and bounded static-analysis gates passed. A native Windows 11/MSVC
  19.44.35228.0 source copy passed 15/15 CTests on Intel UHD Graphics 630,
  preserved zero high-severity GL messages and clean shutdown across the direct
  render/capture paths, and reproduced the accepted normal/debug hashes.
  Evidence: [`CMakeLists.txt`](CMakeLists.txt), [`src/README.md`](src/README.md),
  and the ignored
  [`native Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-001038930/manifest.json).
- [x] Rename or split `TriangleRenderer` so its public ownership matches the
  triangle, cube, capture, and future mesh responsibilities before chunk
  rendering is added. Observed result: on 2026-08-16, the public façade and
  source files became `OpenGlRenderer` and `opengl_renderer.*`; the ambiguous
  `render` and `sample_center` methods became `render_triangle` and
  `sample_triangle_center`. The façade explicitly owns current-context OpenGL
  rendering entry points, render resources, draw submission, and framebuffer
  readback.
  Scenario runners still own pass/fail oracles and PNG output, and no
  speculative chunk-mesh API was introduced. WSL Clang 18.1.3 `dev` and
  ASan/UBSan builds each passed 8/8 CTests; the project-only format and bounded
  static-analysis gates passed. A native Windows 11/MSVC 19.44.35228.0 source
  copy passed 15/15 CTests on Intel UHD Graphics 630. All five direct
  render/capture invocations shut down cleanly with zero high-severity GL
  messages; repeated normal captures retained SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`
  and the debug capture retained
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  The ignored packet passed independent manifest/file/hash validation. This was
  an invisible ownership rename, so no new visual baseline was promoted.
  Evidence: [`opengl_renderer.hpp`](src/render/opengl_renderer.hpp),
  [`scenario_runner.cpp`](src/platform/scenario_runner.cpp), and the
  [`native Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-001947114/manifest.json).
- [x] Replace the hand-loaded OpenGL entry-point table with the approved pinned,
  checksum-verified generated loader before the renderer consumes a broader GL
  API. Observed result: glad 2.0.8 generated the checked-in OpenGL 4.6 Core
  loader reproducibly with no extensions; CMake verifies all generated sources
  and the retained license before configuration proceeds. The platform runtime
  initializes it once through SDL after making the context current, and the
  renderer no longer owns per-symbol pointers. A controlled generated-source
  mutation was rejected at configure. WSL Clang 18.1.3 `dev` and ASan/UBSan
  builds each passed 8/8 CTests, formatting and bounded static analysis passed,
  and native Windows MSVC 19.44.35228.0 passed 15/15 CTests on Intel UHD
  Graphics 630 with `loaded_gl=4.6`, zero high-severity messages, and unchanged
  accepted capture hashes. Evidence: [`glad provenance`](third_party/glad/README.md),
  [`WideEyeDependencies.cmake`](cmake/WideEyeDependencies.cmake),
  [`window_runtime.cpp`](src/platform/window_runtime.cpp), and the ignored
  [`native Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-003632556/manifest.json).

### Voxel representation

- [x] Define integer world, chunk, and local coordinates with negative-coordinate
  tests. Observed result: distinct signed 64-bit triples convert through a
  caller-supplied positive cubic edge length using floor division. Explicit
  tests cover `-1`, exact negative edges, the cell before an edge, positive
  edges, invalid extents and locals, 1/3/16/32 edge lengths, exact round trips at
  both signed 64-bit endpoints, and checked recomposition underflow/overflow.
  On 2026-08-16, WSL Ubuntu 24.04.4 Clang 18.1.3 `dev` and ASan/UBSan suites
  each passed 9/9 CTests; format and bounded static analysis passed. Native
  Windows 11 MSVC 19.44.35228.0 passed 16/16 CTests and preserved the accepted
  capture hashes with zero high-severity GL messages. Evidence:
  [`coordinates.hpp`](src/voxel/coordinates.hpp),
  [`coordinates.cpp`](src/voxel/coordinates.cpp),
  [`voxel_coordinates_tests.cpp`](tests/voxel_coordinates_tests.cpp), and the
  ignored
  [`native Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-010409722/manifest.json).
- [x] Compare 16³ and 32³ chunk memory/rebuild behavior before selecting a size.
  Observed result: a deterministic 32³ field produced the same 26,211 occupied
  cells and 35,462 visible faces under both partitions. The one-byte occupancy
  plus fixture-control model used 33,120 bytes for eight 16³ chunks and 32,840
  bytes for one 32³ chunk. An interior edit scanned 4,096 versus 32,768 cells;
  the representative cross-boundary edit scanned 8,192 versus 32,768. On the
  WSL Ubuntu 24.04.4/GCC 13.3.0 release run, 21-sample medians were 1,740,255 ns
  versus 1,704,272 ns for the equivalent full rebuild, below the predefined 25%
  regression guard. The initial production edge is therefore 16, with real
  storage/meshing/upload/target-device measurements required before any future
  change. WSL development, ASan/UBSan, and release suites each passed 10/10
  CTests; native Windows MSVC 19.44.35228.0 passed 17/17 and preserved the
  accepted captures. Evidence: [`ADR 0002`](docs/decisions/0002-chunk-edge-length.md),
  [`chunk_size_comparison.cpp`](tests/chunk_size_comparison.cpp), the ignored
  [`measurement manifest`](artifacts/phase2/2026-08-16/chunk-size-comparison-wsl-gcc13-release-manifest.json),
  and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-013203819/manifest.json).
- [x] Implement palette/material IDs and an explicit empty block. Observed
  result: production chunks contain 4,096 one-byte `MaterialId` cells, with ID
  zero reserved for empty space and used by default construction. Evidence:
  [`chunk.hpp`](src/voxel/chunk.hpp).
- [x] Implement safe get/set and dirty-region tracking. Observed result: invalid
  local reads return no value, invalid writes are rejected, unchanged writes do
  not dirty storage, and actual edits expand a clearable inclusive local-space
  dirty region. Evidence: [`chunk.cpp`](src/voxel/chunk.cpp).
- [x] Test empty, full, boundary, adjacent, and edited chunks. Observed result:
  the focused unit executable covers all 4,096 cells of empty and full chunks,
  every corner, each invalid axis boundary, positive/negative adjacent chunk
  splits, independent neighboring storage, and dirty edit/clear behavior. WSL
  development and ASan/UBSan passed 11/11 CTests; native Windows passed 18/18.
  Evidence: [`chunk_storage_tests.cpp`](tests/chunk_storage_tests.cpp) and the
  ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-110914010/manifest.json).

### Meshing and rendering

- [x] Implement a correct naive exposed-face mesher first. Observed result: the
  CPU baseline emits one indexed quad per face bordering empty space, removes
  shared faces between non-empty cells, preserves material IDs, and supplies
  exact outward winding/cardinal normals. The unit oracle covers empty and
  single-cell chunks, different-material adjacency, the 1,536-face shell of a
  full isolated chunk, a fully surrounded chunk, missing and empty neighbors,
  and wrapped sampling across all six chunk borders. WSL development and
  ASan/UBSan passed 12/12 CTests; native Windows passed 19/19. Evidence:
  [`naive_mesher.cpp`](src/voxel/naive_mesher.cpp) and
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp).
- [x] Keep opaque, cutout, and translucent output separate even if only opaque is
  drawn initially. Observed result: a caller-owned `MaterialPassTable` defaults
  every ID to opaque and can classify IDs as cutout or translucent; the mesher
  routes emitted faces into three independent `ChunkMesh` buffers without
  changing non-empty-neighbor occlusion. The focused oracle verifies exact
  opaque/cutout/translucent counts of 5/5/6 for a mixed fixture, cross-pass
  shared-face removal, and topology/material preservation in every output. WSL
  development and ASan/UBSan passed 12/12 CTests; formatting and bounded static
  analysis passed. Native Windows MSVC 19.44.35228.0 passed 19/19 CTests and
  preserved the accepted captures with zero high-severity OpenGL messages.
  Evidence: [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-113523193/manifest.json).
- [x] Bound vertex/index counts and reject overflow. Observed result: the fixed
  16³ build declares conservative maxima of 24,576 faces, 98,304 vertices, and
  147,456 indices, with a compile-time proof that the vertex bound is
  representable by the `uint32` index format. The mesher counts and classifies
  first, checks arithmetic, vector, and caller-supplied aggregate limits before
  allocation, reserves exact per-pass storage, and exposes either complete
  buffers or a checked error. A 2,048-cell checkerboard emitted 12,288 faces,
  accepted exact 49,152-vertex/73,728-index limits, and rejected one-less limits;
  zero limits still accepted an empty chunk. WSL development and ASan/UBSan
  passed 12/12 CTests; formatting and bounded static analysis passed. Native
  Windows MSVC 19.44.35228.0 passed 19/19 CTests and preserved the accepted
  captures with zero high-severity OpenGL messages. Evidence:
  [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-120029373/manifest.json).
- [x] Render one handcrafted paddock with ground, stone wall, red gate, and a
  distant barn landmark. Observed result: on 2026-08-16, a deterministic
  32×16×32 blockout stored 1,746 occupied cells across four production 16³
  chunks. Live axial neighborhoods removed shared chunk-border faces; the
  checked aggregate opaque mesh contained 2,754 faces, 11,016 vertices, and
  16,524 indices, while cutout and translucent outputs remained empty. The
  renderer uploaded that mesh once, drew it with depth testing and back-face
  culling, and exposed it through the default interactive path plus a bounded
  `--paddock-smoke` capture. WSL development and ASan/UBSan suites each passed
  13/13 CTests; formatting and bounded static analysis passed. Native Windows
  11/MSVC 19.44.35228.0 passed 22/22 CTests on Intel UHD Graphics 630, including
  the paddock center/depth oracle and repeated capture, with zero high-severity
  OpenGL messages and unchanged accepted Tracer 0 hashes. Two retained 960×540
  frames were byte-identical at SHA-256
  `173238274346f39ce3a5fae87e2524e515cb65636302e0e2c3541cf0eaec92d2`;
  agent inspection found the required ground, wall, centered red gate, and barn
  distinct. The owner explicitly accepted the
  [candidate packet](artifacts/phase2/2026-08-16/handcrafted-paddock/windows-intel-uhd-630/review.md)
  on 2026-08-16, promoting the exact normal frame to the checked-in
  [Tracer 1 baseline](tests/goldens/tracer1/handcrafted_paddock-v1/windows-intel-uhd-630-development-blockout/review.md).
  A registered platform-independent CTest pins its manifest, review, capture,
  scenario/profile, metrics, and sole Accept verdict; post-promotion WSL
  development and ASan/UBSan suites each passed 14/14 CTests. Evidence:
  [`handcrafted_paddock.hpp`](src/voxel/handcrafted_paddock.hpp),
  [`handcrafted_paddock_tests.cpp`](tests/handcrafted_paddock_tests.cpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp), and the ignored
  [native build/test packet](artifacts/phase1/2026-08-16/windows-cube-smoke-123305811/manifest.json).
- [x] Add a small palette, directional light, sky color, fog, and stable basic
  shadows only after geometry is correct. Observed result: the six visible
  paddock materials now resolve through a voxel-owned bounded palette, while
  the unchanged fixed geometry and camera render under a fixed directional
  light, deliberate blue sky, distance fog, and a static 1024x1024 depth shadow
  map with 3x3 filtering. The static scene renders the shadow map once per
  upload. A native Windows 960x540 normal capture showed readable ground, wall,
  red gate, barn, atmospheric separation, and stable wall/barn shadows; it is
  ignored implementation evidence, not a promoted golden. WSL development and
  ASan/UBSan suites each passed 14/14 CTests. Native Windows development and
  AddressSanitizer copy-builds each passed 27/27 CTests on Intel UHD Graphics
  630, including byte-identical repeated normal captures, with zero
  high-severity OpenGL messages. Evidence:
  [`handcrafted_paddock.hpp`](src/voxel/handcrafted_paddock.hpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp), and the ignored
  [development packet](artifacts/phase1/2026-08-16/windows-cube-smoke-132243278/manifest.json).
- [x] Add chunk bounds, face-normal, wireframe, and mesh-stat debug views.
  Observed result: four named same-camera scenarios now draw complete cyan
  bounds for all four production chunks; color faces by cardinal normal and
  draw one normal segment for every emitted face; expose the actual indexed
  naive topology in wireframe; or overlay a five-bar mesh-stat chart while
  logging exact chunk, occupied-block, face, vertex, index, and per-material
  face counts. The checked mesh remains 4 chunks, 1,746 occupied blocks, 2,754
  faces, 11,016 vertices, and 16,524 indices. Broad framebuffer oracles passed
  for every debug mode in both native Windows matrices, and agent inspection of
  the ignored 960x540 captures found each intended diagnostic visible and
  aligned with the fixed paddock. Evidence:
  [`scenario_runner.cpp`](src/platform/scenario_runner.cpp),
  [`opengl_renderer.hpp`](src/render/opengl_renderer.hpp),
  [`handcrafted_paddock_tests.cpp`](tests/handcrafted_paddock_tests.cpp), and the
  ignored [sanitized packet](artifacts/phase1/2026-08-16/windows-sanitized-cube-smoke-132406509/manifest.json).
- [x] Capture identical-camera normal/debug frames and frame-time/memory data.
  Observed result: the native Windows Release
  [`tracer1-review-windows-142557466`](artifacts/phase2/2026-08-16/tracer1-review-windows-142557466/review.md)
  packet retained byte-identical 960×540 normal captures plus the four named
  same-camera debug views. Its 1920×1080 static scenario measured 600 frames
  after 120 warmup frames using serialized GPU queries and swap: synchronized
  p95/p99 were 2.865/5.449 ms, GPU p95/p99 were 1.625/1.775 ms, and current/peak
  RSS was 104,673,280 bytes. The provisional Low comparison passed on the Intel
  UHD 630 proxy; the Iris Xe reference target remains unmeasured.

### Dog and camera placeholder

- [x] Add a kinematic placeholder dog with predictable ground contact.
  Evidence: the fixed-tick controller keeps its cylinder base at analytic ground
  height; focused unit/scenario tests and the native rendered dog capture pass.
- [x] Use simple analytic collision separate from voxel render geometry.
  Evidence: [`PaddockCollisionField`](src/game/dog_controller.hpp) owns paddock,
  wall, and gate shapes without querying voxel faces or renderer meshes.
- [x] Add gameplay and free-debug cameras. Evidence: gameplay orbit yaw/pitch is
  independent from dog facing, free-debug state remains isolated, and fixed-tick
  tests cover mouse orbit, movement-basis signs, free-camera movement, toggling,
  restart, angle interpolation, and repeated-sequence determinism.
- [x] Support keyboard/controller input through named actions. Evidence:
  synthetic SDL keyboard/mouse/gamepad tests cover movement, held look rates,
  accumulated transient mouse deltas, sprint, restart, source coexistence,
  dead-zone behavior, one-tick consumption, focus clearing, and disconnect
  clearing. A physical controller remains unverified.
  Usability follow-up (observed result, 2026-08-17): the owner reported that a
  focused interactive window could only return the desktop cursor by switching
  applications. Escape is now a named `toggle_mouse_capture` action; the window
  reducer separates capture intent from actual capture, one reconcile step per
  frame owns every SDL relative-mouse call, and mouse motion becomes camera look
  only while the pointer is captured. Headless coverage covers the binding,
  single-read press consumption, and the focus/intent rules. WSL development,
  Release, and ASan/UBSan configurations each passed 24/24 CTests; formatting and
  bounded clang-tidy passed. The interactive toggle itself remains unverified on
  this WSL host, which cannot create the required OpenGL 4.6 context; it needs
  one native run. Evidence: [`input.hpp`](src/platform/input.hpp),
  [`window_state.hpp`](src/platform/window_state.hpp), and
  [`window_runtime.cpp`](src/platform/window_runtime.cpp).
- [x] Add restart and deterministic scenario selection. Evidence: version 1,
  seed 0 `paddock-start`, `wall-contact`, `closed-gate`, and `open-gate`
  scenarios reproduce local fixed-tick state and restart exactly.

### Phase 2 exit gate

- [x] The bounded paddock is visually readable and reproducibly captured.
- [x] Dog movement cannot tunnel through the representative wall/gate tests.
  Observed result: both giant analytic sweeps and 240 fixed-tick runs stop at the
  wall/closed gate, while the corresponding open-gate scenario passes through;
  all tests pass under WSL ASan/UBSan and native Windows Release.
- [x] Chunk/mesh debug data explains every visible face and missing face.
  Observed result: a caller-requested `ChunkFaceDiagnostic` ledger uses the same
  deterministic z/y/x/direction traversal and neighbor sampler as the naive
  mesher. Each non-empty voxel side records its source local coordinate,
  material, direction, wrapped neighbor local/material, same-, adjacent-, or
  missing-chunk provenance, and emitted/culled disposition. The four-chunk
  paddock retains the source chunk for every record and reports exactly 10,476
  decisions: 2,754 emitted quads, 7,722 occupied-neighbor culls, 9,098
  same-chunk samples, 226 adjacent-chunk samples, and 1,152 missing-chunk
  samples. The focused oracle proves exactly six unique records for every
  occupied block and none for empty blocks, matches stored source and neighbor
  materials, reconciles every emitted record in order to one actual world-space
  mesh quad, and pins representative same-chunk, cross-chunk, and missing-world
  cases. Existing same-camera chunk-bounds, face-normal, indexed-wireframe, and
  mesh-stat views remain the visual side of the diagnostic; scenario logs now
  expose the ledger totals and reject an inconsistent ledger at initialization.
  On 2026-08-16, WSL Clang 18.1.3 development and ASan/UBSan suites each passed
  20/20 CTests, and the project-only format and bounded static-analysis gates
  passed. Native OpenGL captures were not rerun for this CPU-only diagnostic
  change, and no accepted visual baseline changed. Evidence:
  [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`handcrafted_paddock.hpp`](src/voxel/handcrafted_paddock.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and
  [`handcrafted_paddock_tests.cpp`](tests/handcrafted_paddock_tests.cpp).
- [x] A same-state normal/debug review packet receives an explicit owner verdict.
  Observed result: on 2026-08-16, the owner explicitly selected Accept for the
  named native Windows Release
  [`tracer1-review-windows-142557466`](artifacts/phase2/2026-08-16/tracer1-review-windows-142557466/review.md)
  packet after receiving the normal frame, four same-camera debug views, dog
  placeholder, automated evidence, and known limitations. The verdict accepts
  the diagnostics, grounded dog/facing marker, gameplay-camera starting point,
  previously reviewed keyboard/mouse behavior, and Intel UHD 630 proxy result as
  sufficient for Tracer 1. It explicitly leaves a physical controller, native
  Linux graphics, and the named Iris Xe target unverified. The checked-in
  blockout golden remains the accepted visual baseline and was not replaced.
- [x] Procedural terrain, streaming, LOD, and advanced post-processing remain out
  of scope.
