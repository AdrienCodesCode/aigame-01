# Native Windows development setup

## Scope

Native Windows is the first hardware-rendering path for Wide Eye. WSL Ubuntu
remains the everyday editing, compilation, unit-test, simulation, and headless
development host, but a WSL graphics result is not native Windows evidence.
Native Linux remains a release target and must be tested later on an actual
Linux installation or machine before support is claimed.

The Phase 0 diagnostic builds the same pinned SDL 3.4.10/C++23 source used by
the Ubuntu probe, requests an explicit OpenGL 4.6 Core debug context, reports
the actual renderer and driver-visible GL/GLSL versions, and exits nonzero if
the requested version or Core profile is missing.

## One-time Windows build tools

Run the following in **Windows PowerShell**, not the Ubuntu shell, when
reproducing this setup on another machine. It installs
the standalone Visual Studio 2022 Build Tools with Microsoft's Desktop
development with C++ workload and its recommended MSVC, Windows SDK, CMake,
Ninja, testing, and sanitizer components; the Visual Studio IDE is not required.

```powershell
winget install --exact --id Microsoft.VisualStudio.2022.BuildTools `
  --source winget --accept-package-agreements --accept-source-agreements `
  --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

The workload and component IDs are documented in Microsoft's
[Build Tools component directory](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=visualstudio).
The installer may request administrator approval. After it completes, close and
reopen PowerShell before running the diagnostic.

## OpenGL context diagnostic

From the repository's WSL Ubuntu shell, run:

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$(wslpath -w ./tools/phase0/run-context-smoke.ps1)"
```

The script imports the x64 MSVC environment, uses the CMake and Ninja installed
with Build Tools, copies only the two diagnostic source files to
`%LOCALAPPDATA%\WideEye\phase0`, builds there as a native Windows executable,
and runs it on Windows. It records OS, GPU, driver, tool paths, source hashes,
configure/build output, and the context result in the ignored
`artifacts/phase0/<date>/windows-context-smoke-<time>.log` file in this
repository. Each invocation uses a new log so a failed reproduction is not
overwritten by a later run.

The default request is the accepted OpenGL 4.6 baseline. A lower-version probe
is allowed only as separately labeled capability evidence:

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$(wslpath -w ./tools/phase0/run-context-smoke.ps1)" \
  -Major 4 -Minor 5
```

On a hybrid-GPU laptop, read `gl_renderer` in the smoke output; do not infer
which adapter rendered from the inventory alone. If Windows selects the Intel
adapter and an NVIDIA-specific result is needed, add
`%LOCALAPPDATA%\WideEye\phase0\context-smoke-build\wide-eye-context-smoke.exe`
under **Settings > System > Display > Graphics**, select **Options**, choose
**High performance**, save, and rerun. Microsoft's
[Windows graphics settings](https://support.microsoft.com/en-us/windows/optimizations-for-windowed-games-in-windows-11-3f006843-2c7e-4ed0-9a5e-f9389e535952)
document this per-application preference.

## Phase 1 project graphics smoke

From the repository's WSL Ubuntu shell, run the native Windows project build and
OpenGL context/triangle/cube/capture smokes with:

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$(wslpath -w ./tools/phase1/run-window-smoke.ps1)"
```

The script imports the x64 MSVC environment, copies the CMake, project source,
test, and checksum-verified third-party inputs into a new timestamped
`%LOCALAPPDATA%\WideEye` directory, records their SHA-256 hashes, and uses the
checked-in `dev` preset. It runs the
version, PNG-writer, voxel-coordinate, chunk-storage, naive-mesher,
chunk-size-comparison, core runtime, window state, fatal assertion, dummy-driver
SDL lifecycle, accepted-baseline integrity, display-backed OpenGL context,
triangle framebuffer, cube-depth, normal and wireframe repeated-capture, and
injected high-severity debug-message CTests. It then runs
`wide_eye.exe --triangle-smoke`,
`wide_eye.exe --voxel-cube-smoke`, two normal cube captures, and a matching
`wide_eye.exe --voxel-cube-debug-smoke` wireframe capture directly through the
normal Windows video driver. Each invocation keeps one ignored
`windows-cube-smoke-<time>/` packet under `artifacts/phase1/<date>/`. Its
`wide-eye.artifact-manifest` schema version 1 records Git/worktree state, source
hashes, platform/GPU data, exact commands and results, relevant configuration,
parsed scenario state, and SHA-256 records for the retained log, JSON files, and
normal/debug PNGs. A passing run also writes `review.md` with matching metadata,
artifact-manifest hash, known limitations, and an intentionally blank owner
verdict. A passing comparison removes the temporary repeat PNG.

Use the same source-hashed copy-build under MSVC AddressSanitizer with:

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$(wslpath -w ./tools/phase1/run-window-smoke.ps1)" \
  -Preset dev-sanitized
```

This mode first verifies that every generated project compile command and the
`wide_eye.exe` link rule contain `/fsanitize=address`. It then runs the full
Windows CTest suite and the direct triangle, cube, repeated normal-capture, and
wireframe-capture paths. Before accepting the run, it scans the retained log for
project failure stages, AddressSanitizer/LeakSanitizer/UBSan diagnostics, and
nonzero high-severity GL counts. The resulting
`windows-sanitized-cube-smoke-<time>/` packet records the preset and sanitizer
in its manifest and does not create a new visual-review candidate; sanitizer
evidence does not change or supersede the accepted development visual baseline.

To verify the failure-preservation branch, run the same command with
`-InjectCaptureMismatch`. This test-only switch alters the second generated
artifact after two real successful captures; the wrapper must exit with code 2
at `capture-repeat-compare` and retain both images. Validate either emitted
manifest independently from WSL with:

```bash
cmake -DMANIFEST="artifacts/phase1/DATE/PACKET/manifest.json" \
  -DEXPECTED_RESULT=pass -DREQUIRE_CAPTURE=ON \
  -DREQUIRE_DEBUG_CAPTURE=ON -DREQUIRE_REVIEW=ON \
  -P tests/assert-artifact-manifest.cmake
```

For the controlled failure packet, use `-DEXPECTED_RESULT=fail` and add
`-DEXPECTED_FAILURE_STAGE=capture-repeat-compare`,
`-DREQUIRE_REPEAT_CAPTURE=ON`, and `-DREQUIRE_DEBUG_CAPTURE=ON`. A controlled
failure does not produce `review.md` and must not be treated as a candidate.

## Phase 2 Tracer 1 review and measurement

From the repository's WSL Ubuntu shell, build and test the current native
Windows Release target, capture the fixed-camera normal/debug set and dog
placeholder, and record the static-paddock frame-time/RSS baseline with:

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$(wslpath -w ./tools/phase2/run-tracer1-review.ps1)"
```

This runner keeps all writes in the repository's ignored `build/Windows/release`
and `artifacts/phase2/<date>/tracer1-review-windows-<time>/` trees. It imports the
x64 MSVC environment and uses `cmd.exe pushd` only to give CMake a temporary
drive mapping for the WSL UNC workspace. The current Release matrix contains 37
CTests. After it passes, the runner captures two normal frames, chunk bounds,
face normals, indexed wireframe, mesh statistics, and the `paddock-start` dog
gameplay frame, which now also includes five sheep proxies. It rejects unequal
normal hashes and any failed capture or high-severity
OpenGL diagnostic.

The final direct path measures `handcrafted_paddock_static_v1` at 1920×1080 for
600 frames after 120 warmup frames. It records CPU submission, GPU query, and
serialized query-plus-swap percentiles plus current/peak process RSS, then
compares the synchronized p95/p99 and memory values with the provisional Low
limits from ADR 0001. The serialized query intentionally blocks each frame, so
the method is reproducible evidence rather than a production frame-pacing
profiler. `manifest.json` retains commands, host/GPU information, hashes, and
the parsed `state.json`; `review.md` keeps the owner verdict blank. Do not
promote or replace a checked-in visual baseline until the owner inspects the
packet and explicitly records Accept.

## Phase 3 Tracer 2 presentation packet

From WSL, reproduce the five-proxy capture, canonical state, allocation, and
measurement packet with:

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$(wslpath -w ./tools/phase3/run-tracer2-presentation-review.ps1)"
```

The runner keeps writes under the ignored
`artifacts/phase3/<date>/tracer2-presentation-windows-<time>/` tree. It builds
and runs the native Release matrix, executes the fixed-update and
snapshot/presentation allocation oracles, and captures the deterministic
`presentation-motion` fixture at ticks 1, 61, and 121 with interpolation alpha
0.5. Tick 61 also receives an independent repeat-normal frame and a face-normal
debug frame; all three commands emit canonical state at the executable's
current dump version, which the runner records from the captured JSON, so it
can reject a same-state mismatch. The packet retains the original 1920×1080
frames plus a 1920×360 contact sheet.

The final path measures 600 five-proxy frames after 120 warmup frames at
1920×1080. It records snapshot/presentation preparation, CPU submission, GPU,
and serialized frame p50/p95/p99 values plus current/peak RSS and the
provisional Low comparison. `review.md` deliberately leaves the owner verdict
blank. This packet is presentation-envelope evidence, not acceptance of final
animal art or flock behavior.

## Phase 0 visual-feasibility unchanged baseline

On the reference desktop, run the approved visual-feasibility baseline at the
selected display mode before tuning presentation. The provisional invocation is:

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$(wslpath -w ./tools/phase3/run-visual-feasibility-baseline.ps1)" \
  -Width 2560 -Height 1440 -RefreshHz 60
```

The runner uses the native Windows Release build and complete CTest suite. It
records OS, CPU, RAM, GPU/driver inventory, reported monitor modes, the active
OpenGL renderer/version, the selected viewport, the named
`visual-feasibility-five-sheep-v1` configuration, source hashes, normal/repeat/
face-normal representative frames at tick 30, an untouched elevated holdout,
route samples at ticks 1/30/90, canonical gameplay state, synchronized timing,
RSS, startup-to-first-capture, compressed Release size, and zero-high-severity
OpenGL evidence. It writes the rubric and blank owner camera confirmation beside
the packet under
`artifacts/phase3/<date>/visual-feasibility-baseline-<time>/`, then runs the
independent manifest/file/hash validator.

The command exits nonzero rather than relabeling evidence when the requested
display mode is not reported, the active `gl_renderer` is not the NVIDIA
GeForce RTX 5070 Ti, OpenGL 4.6 is unavailable, repeat state/captures differ,
graphics diagnostics fail, or a provisional High budget is exceeded. Inventory
alone never proves which GPU rendered, and the reference desktop is a hybrid
two-adapter machine, so that pin does real work. A laptop or WSL smoke can
verify the configuration seam, but cannot satisfy this reference-baseline gate.
The packet is unchanged comparison evidence, not a promoted golden or an
accepted candidate look; Phase 0 remains open until the owner confirms that both
cameras ask the intended visual question.

For the interactive owner check, run `wide_eye.exe --play-scenario
paddock-start` from the build tree. In gameplay mode, the mouse orbits the
camera and WASD moves relative to camera yaw; Shift sprints, R restarts, and Tab
toggles the free-debug camera. Debug mode uses mouse or arrow look, WASD, and
Q/E. Gamepad mappings use both sticks, South, Start, Back, and the shoulder
buttons. Automated tests validate translation and control math. The owner
accepted the keyboard/mouse baseline on 2026-08-16; physical-controller behavior
still requires a manual run when hardware is available.

## Observed reference desktop — 2026-08-22

This is the machine the reference visual-development role points at. The owner
confirmed on 2026-08-22 that it is the reference desktop and that the earlier
RTX 4070 Ti report is superseded by the observed host below.

**Observed result** (method: Windows CIM inventory queries plus the Phase 0
context smoke `tools/phase0/run-context-smoke.ps1`; ignored evidence log
`artifacts/phase0/2026-08-22/windows-context-smoke-155925531.log`):

- CPU: AMD Ryzen 9 9950X 16-Core Processor, 16 cores / 32 threads.
- RAM: 61.6 GiB reported by `Win32_ComputerSystem.TotalPhysicalMemory`.
- Board: ASRock X870 Pro RS WiFi.
- OS: Microsoft Windows 11 Home, version `10.0.26200`, build `26200`, 64-bit.
- Display adapters reported by Windows: NVIDIA GeForce RTX 5070 Ti, driver
  `32.0.15.9186`; AMD Radeon(TM) Graphics (Ryzen integrated), driver
  `32.0.21036.18`. This is a hybrid two-adapter host, so the per-application
  graphics preference described under
  [OpenGL context diagnostic](#opengl-context-diagnostic) applies here too, and
  the baseline runner's active-renderer pin is a real gate rather than a
  formality.
- Primary display: 2560×1440; reported supported modes include
  2560×1440@144 Hz, 1920×1080@60 Hz, and 1024×768@60 Hz.
- Active OpenGL: `gl_vendor=NVIDIA Corporation`,
  `gl_renderer=NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2`,
  `gl_version=4.6.0 NVIDIA 591.86`, `glsl_version=4.60 NVIDIA`,
  `core_profile=yes`, `debug_context=yes`, SDL 3.4.10, `video_driver=windows`,
  `result=pass`.
- Toolchain: MSVC `19.44.35228.0` from Visual Studio 2022 Build Tools (MSVC
  toolset `14.44.35207`), bundled CMake `3.31.6-msvc6`, bundled Ninja `1.12.1`.
- The native Windows `dev` preset configured, built, and passed 45/45 CTests,
  including every display-backed OpenGL test from `opengl_context_smoke`
  through `opengl_debug_high_severity` and `opengl_influence_debug_overlay`.

**Observed result — the Phase 0 baseline packet, 2026-08-22.**
[`run-visual-feasibility-baseline.ps1`](../../tools/phase3/run-visual-feasibility-baseline.ps1)
has now run on this host at 2560×1440@60 and completed with `result=pass`.
Ignored evidence packet:
`artifacts/phase3/2026-08-22/visual-feasibility-baseline-183850545/`. It records
all 13 stages `pass` (including `ctest-release` 47/47),
`gl_debug_high_severity_messages=0`, `within_performance_budget=yes`,
`startup_to_first_capture_ms=392.116`, `compressed_package_bytes=956271`,
`process_peak_rss_bytes=76713984`, and `synchronized_frame_p95_ns=6985000`.
The first attempt, on the same day, aborted on a shell defect rather than a
graphics one — see
[QA-009](../qa/closed/QA-009-windows-evidence-runners-abort-on-benign-opengl-notification-stderr.md).

**Not claimed:** the packet is unchanged pre-visual-implementation comparison
evidence, so no visual quality, accepted look, or promoted baseline follows from
it, and its owner camera/rubric verdict is still blank. The RTX 5070 Ti is also a
newer GPU than the RTX 4070 Ti the approved plan assumed: the provisional High
budgets were met here, but they remain provisional numbers chosen before this
hardware was measured rather than budgets derived from it.

## Observed Windows host — 2026-08-15

This is the development **laptop**, not the reference desktop above.

**Observed result:**

- OS: Microsoft Windows 11 Pro, native x64, version `10.0.26200`, build `26200`.
- Display adapters reported by Windows: Intel UHD Graphics 630, driver
  `27.20.100.9664`; NVIDIA GeForce GTX 1050 Ti with Max-Q Design, driver
  `32.0.15.8157`.
- Windows Package Manager is present and resolves Visual Studio Build Tools 2022
  package `Microsoft.VisualStudio.2022.BuildTools` version `17.14.37`.
- Visual Studio Build Tools 2022 version 17.14.37 is installed without the IDE.
  The diagnostic imported MSVC toolset `14.44.35207`; CMake identified compiler
  version `19.44.35228.0`. Bundled CMake is `3.31.6-msvc6`, and bundled Ninja is
  `1.12.1`.
- The checksum-pinned SDL 3.4.10 source configured and built natively with its
  Windows video and OpenGL backends. The diagnostic executable reported
  `video_driver=windows`.
- The default OpenGL 4.6 diagnostic passed three times, including the initial
  full build and two cached reruns. Windows selected the Intel UHD Graphics 630:
  `gl_vendor=Intel`, `gl_version=4.6.0 - Build 27.20.100.9664`,
  `glsl_version=4.60 - Build 27.20.100.9664`, `actual_gl=4.6`,
  `core_profile=yes`, `debug_context=yes`, and `result=pass`.
- The final UTF-8 evidence log is the ignored
  `artifacts/phase0/2026-08-15/windows-context-smoke-174920033.log`. The source
  hashes and exact configure/build commands are recorded in that file.
- The NVIDIA adapter was inventoried but not selected by these runs. A separate
  NVIDIA or stronger-PC run is optional capability/performance evidence, not a
  prerequisite for completing Phase 0.
- The Phase 1 SDL lifecycle runner copy-built the project with MSVC
  19.44.35228.0, passed both development-preset CTests, and reported SDL 3.4.10,
  `video_driver=windows`, and `window_result=pass`. The source hashes, commands,
  and output are retained in the ignored
  `artifacts/phase1/2026-08-15/windows-window-smoke-184214910.log` evidence file.
- The Phase 1 project context runner source-hashed and copy-built the updated
  project with MSVC 19.44.35228.0, passed all three development-preset CTests,
  and passed the direct `--context-smoke`. The Intel UHD Graphics 630 reported
  `gl_version=4.6.0 - Build 27.20.100.9664`,
  `glsl_version=4.60 - Build 27.20.100.9664`, `actual_gl=4.6`,
  `core_profile=yes`, and `context_result=pass`. The exact commands, hashes, and
  output are retained in the ignored
  `artifacts/phase1/2026-08-15/windows-context-smoke-185245172.log` evidence file.
- The OpenGL debug-callback update was source-hashed and copy-built with MSVC
  19.44.35228.0 on the same native Windows host. All four development CTests
  passed, including a driver-backed regression that injected one application
  high-severity message and required a nonzero smoke result. The direct normal
  smoke reported `debug_context=yes`, `gl_debug_callback=installed`,
  `gl_debug_high_severity_messages=0`, and `context_result=pass` on the Intel
  UHD Graphics 630. The command, source hashes, CTest results, and direct report
  are retained in the ignored
  `artifacts/phase1/2026-08-15/windows-context-smoke-191751268.log` evidence file.
- The triangle update was source-hashed and copy-built with MSVC 19.44.35228.0
  on the same native Windows host. All five development CTests passed, including
  the independent context, triangle, and injected high-severity paths. The
  direct `--triangle-smoke` compiled GLSL 4.60 shaders, issued the draw, sampled
  center RGBA `99,127,155,255`, matched the triangle oracle, and reported zero
  high-severity messages on Intel UHD Graphics 630. The command, source hashes,
  CTest results, and direct report are retained in the ignored
  `artifacts/phase1/2026-08-15/windows-context-smoke-193100124.log` evidence file.
- The cube/lifecycle/runtime update was source-hashed and copy-built with MSVC
  19.44.35228.0 on the same native Windows host. All nine development CTests
  passed, covering the 60 Hz fixed-step accumulator, window-state reducer,
  automation-safe fatal assertion, SDL lifecycle/event mapping, context,
  triangle, cube depth, and high-severity GL paths. The direct cube smoke
  reported a 24-bit depth/8-bit stencil framebuffer, center RGBA
  `229,56,31,255`, center depth `0.959411`, enabled `LESS` depth testing and
  depth writes, a matching cube oracle, and zero high-severity messages on
  Intel UHD Graphics 630. The direct triangle smoke remained
  `99,127,155,255`. Commands, source hashes, CTest results, and direct reports
  are retained in the ignored
  `artifacts/phase1/2026-08-15/windows-context-smoke-200718490.log` evidence file.
- The deterministic-capture update was source-hashed and copy-built with MSVC
  19.44.35228.0 on the same native Windows host. All 11 development CTests
  passed, including an exact known-byte PNG unit check and a hidden-window test
  that required two independent cube captures to have the same SHA-256. The
  direct named scenario retained a valid 64x64, 8-bit RGBA, non-interlaced PNG
  at
  `artifacts/phase1/2026-08-15/windows-voxel-cube-205544198.png` with SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`.
  It preserved the cube color/depth oracle and reported zero high-severity
  messages on Intel UHD Graphics 630. Agent inspection found the expected
  colored cube against the dark clear color; this is not an owner verdict or an
  accepted golden. Source hashes, commands, CTest results, and the direct report
  are retained in
  `artifacts/phase1/2026-08-15/windows-context-smoke-205544198.log`.
- The artifact-manifest update was source-hashed and copy-built with MSVC
  19.44.35228.0 on the same native Windows host. The passing packet at
  `artifacts/phase1/2026-08-15/windows-cube-smoke-212707784/` recorded commit
  `8fe5a95b9d5726cbb35c22395c9422ea69ca4fb5` and a dirty worktree, passed all 11
  development CTests, and produced two direct PNGs with matching SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`.
  It retained the normal frame, log, parsed state, configuration, and source-hash
  inventory, and its manifest passed the independent CMake field/file/hash
  validator. The controlled mismatch packet at
  `artifacts/phase1/2026-08-15/windows-cube-smoke-212442229/` exited with code 2
  at `capture-repeat-compare`, retained the untouched normal frame and altered
  repeat frame plus the other required evidence, and passed independent failure-
  manifest validation. The altered repeat is a regression fixture, not a visual
  candidate or baseline.
- The common CTest failure-diagnostic guard was source-hashed and copy-built
  with MSVC 19.44.35228.0 on the same native Windows host. All 12 development
  CTests passed, including the nested fixture proving rejection of project
  failure markers and ASan, LSan, and UBSan diagnostics. The direct triangle,
  cube, and two cube-capture runs preserved the prior color/depth oracles, zero
  high-severity messages, and matching capture SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`.
  The ignored packet at
  `artifacts/phase1/2026-08-15/windows-cube-smoke-214449957/` passed independent
  manifest/file/hash validation; it records the then-current dirty worktree and
  exact source-hash inventory rather than claiming a clean revision.
- The candidate visual-packet update was source-hashed and copy-built with MSVC
  19.44.35228.0 on the same native Windows host. All 14 development CTests
  passed, including independent two-run deterministic PNG checks for the normal
  and same-camera wireframe-debug views. Direct captures reported zero
  high-severity GL messages; the normal hash remained
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`,
  while the debug hash was
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  Agent inspection found the expected colored cube and a matching diagnostic
  wireframe with visible triangle edges. The ignored candidate packet at
  `artifacts/phase1/2026-08-15/windows-cube-smoke-220642406/` passed independent
  manifest/file/hash/review validation; at that checkpoint its owner verdict was
  blank and no golden existed. A controlled mismatch packet at
  `artifacts/phase1/2026-08-15/windows-cube-smoke-220753900/` exited at
  `capture-repeat-compare`, retained normal, altered repeat, and debug PNGs, and
  passed independent failure-manifest validation.
- The owner subsequently launched the interactive native Windows cube, verified
  the resizable window, reviewed the normal and wireframe captures together,
  reported that both looked correct, and explicitly accepted the packet. The
  complete checked-in
  [Tracer 0 baseline](../../tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md)
  retains that verdict and every manifest-linked artifact. Its new CTest guard
  passed in WSL development, ASan/UBSan, and release presets. A post-promotion
  source-hashed native Windows run passed all 15 development CTests, including
  the accepted-baseline guard, reproduced both accepted image hashes, and
  reported zero high-severity GL messages. The ignored verification packet is
  `artifacts/phase1/2026-08-15/windows-cube-smoke-223401700/`; its manifest also
  passed independent field/file/hash/review validation.
- The first explicit native Windows sanitizer graphics packet used the
  source-hashed `dev-sanitized` copy-build with MSVC 19.44.35228.0 and verified
  `/fsanitize=address` on all six project compile commands plus the
  `wide_eye.exe` link rule. All 15 CTests passed; 14 carried the `sanitizer`
  label and 11 carried `headless`. Five direct triangle/cube/capture invocations
  each reported clean shutdown and zero high-severity GL messages on Intel UHD
  Graphics 630. The two normal captures matched one another and the accepted
  normal hash; the debug capture matched the accepted wireframe hash. The final
  log scan found no project, ASan, LSan, UBSan, or nonzero high-severity GL
  marker, and the manifest/files/hashes passed independent validation. Evidence:
  the ignored
  [`windows-sanitized-cube-smoke-234237930`](../../artifacts/phase1/2026-08-15/windows-sanitized-cube-smoke-234237930/manifest.json)
  packet. This is evidence for project-owned code under MSVC AddressSanitizer;
  the separately built SDL shared library was not sanitizer-instrumented.
- A fresh agent independently followed this document's default Phase 1 command
  without relying on the runner-authoring context. The source-hashed `dev`
  copy-build at commit `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f` with a
  dirty worktree passed all 15 CTests, including 11 labeled `headless`, on the
  same native Windows 11/Intel UHD Graphics 630 host. All five direct triangle,
  cube, repeated normal-capture, and wireframe-capture invocations shut down
  cleanly and reported zero high-severity GL messages. Both normal captures
  matched the accepted normal hash and the debug capture matched the accepted
  wireframe hash. The documented WSL validator passed for the ignored
  [`windows-cube-smoke-235312691`](../../artifacts/phase1/2026-08-15/windows-cube-smoke-235312691/manifest.json)
  packet, and no missing or ambiguous reproduction instruction was observed.
  This packet is independent reproduction evidence, not a replacement visual
  baseline; its blank candidate-review form does not supersede the accepted
  packet.
- The generated-loader replacement used the same source-hashed `dev` copy-build
  on 2026-08-16. CMake verified the retained glad 2.0.8 OpenGL 4.6 Core source
  and license hashes before MSVC 19.44.35228.0 built the static loader. All 15
  CTests passed on Intel UHD Graphics 630, and every direct graphics invocation
  reported `loaded_gl=4.6`, clean shutdown, and zero high-severity GL messages.
  The repeated normal captures retained SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`;
  the debug capture retained
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  The ignored
  [`windows-cube-smoke-003632556`](../../artifacts/phase1/2026-08-16/windows-cube-smoke-003632556/manifest.json)
  packet includes all five retained glad source/provenance inputs in its source
  inventory and passed independent manifest/file/hash/review validation. This
  invisible dependency change did not create or promote a visual baseline.
- The first production chunk-storage outcome used the same source-hashed `dev`
  copy-build on 2026-08-16. MSVC 19.44.35228.0 built the fixed 16³ one-byte
  material storage and its safe get/set plus dirty-region test. All 18 CTests
  passed, including empty, full, boundary, adjacent, and edited chunk cases.
  Every direct graphics invocation still reported `loaded_gl=4.6`, clean
  shutdown, and zero high-severity GL messages; the normal and debug captures
  remained byte-identical to the accepted Tracer 0 hashes. The ignored
  [`windows-cube-smoke-110914010`](../../artifacts/phase1/2026-08-16/windows-cube-smoke-110914010/manifest.json)
  packet retains the source inventory and run evidence. This storage result does
  not verify meshing or cross-chunk rebuild propagation.
- The naive exposed-face mesher used the same source-hashed `dev` copy-build on
  2026-08-16. MSVC 19.44.35228.0 built its CPU vertex/index output and passed all
  19 CTests, including exact quad/winding, internal culling, full/surrounded
  chunk, and all-six-border cases. Every direct graphics invocation reported
  clean shutdown and zero high-severity GL messages; the normal/debug captures
  retained the accepted Tracer 0 hashes. The ignored
  [`windows-cube-smoke-112151833`](../../artifacts/phase1/2026-08-16/windows-cube-smoke-112151833/manifest.json)
  packet passed independent manifest/file/hash/review-linkage validation. The
  mesher is not uploaded or drawn yet, and this run does not verify a rebuild
  queue or cross-chunk invalidation implementation.
- The mesher pass-separation outcome used the same source-hashed `dev`
  copy-build on 2026-08-16. MSVC 19.44.35228.0 built the palette-owned
  classification table and independent opaque/cutout/translucent CPU buffers.
  All 19 CTests passed, including exact 5/5/6 per-pass face counts,
  cross-pass shared-face culling, and topology/material checks for each output.
  Every direct graphics invocation reported clean shutdown and zero
  high-severity GL messages; normal and debug captures retained the accepted
  Tracer 0 hashes. The ignored
  [`windows-cube-smoke-113523193`](../../artifacts/phase1/2026-08-16/windows-cube-smoke-113523193/manifest.json)
  packet passed independent manifest/file/hash/review-linkage validation. No
  separated chunk mesh is uploaded or drawn yet.
- On 2026-08-16, the workspace-local Tracer 1 Release runner built the current
  project under MSVC and passed 35/35 CTests on the Intel UHD Graphics 630. The
  final
  [`tracer1-review-windows-142557466`](../../artifacts/phase2/2026-08-16/tracer1-review-windows-142557466/review.md)
  packet retained byte-identical 960×540 normal frames with SHA-256
  `ce60bddee7073c2a392bcf727473694770537a3da32f84e1ca1aa09b89b83a15`, all
  four same-camera diagnostics, and a grounded dog frame with a visible facing
  marker. Every direct path reported zero high-severity OpenGL messages. The
  600-frame serialized 1920×1080 static-paddock sample, after 120 warmup frames,
  measured synchronized frame time at 2,864,800 ns p95 and 5,449,000 ns p99,
  GPU render time at 1,625,223 ns p95 and 1,774,623 ns p99, and current/peak RSS
  at 104,673,280 bytes. It passed the provisional Low comparison on this proxy
  host. On 2026-08-16, the owner explicitly selected Accept for the named packet,
  closing the Tracer 1 same-state review gate without replacing the checked-in
  blockout baseline. The verdict does not verify the named Iris Xe target,
  physical controller, or native Linux graphics.
- On 2026-08-16, the camera-relative controller candidate's source-aligned
  Release run passed 35/35 CTests, including transient mouse input, movement
  basis, planar motor/reversal, interpolation, and repeated-sequence checks.
  Every direct OpenGL 4.6 path again reported zero high-severity messages on the
  Intel UHD Graphics 630. The ignored
  [`tracer1-review-windows-171645758`](../../artifacts/phase2/2026-08-16/tracer1-review-windows-171645758/review.md)
  packet retained byte-identical normal frames and the grounded dog frame. Its
  serialized 1920×1080 proxy measurement reported synchronized p95/p99 of
  2,273,000/2,828,100 ns, GPU p95/p99 of 1,039,658/1,061,487 ns, and peak RSS of
  103,682,048 bytes. The owner subsequently reported the keyboard/mouse behavior
  as good enough to continue and deferred refinement. A physical controller, the
  Iris Xe target, and native Linux graphics remain unverified.

**Inference:** This laptop satisfies the approved native Windows compiler and
OpenGL 4.6 context gate, and the project executable now reproduces that context,
triangle and depth-tested cube oracles, deterministic cube capture,
lifecycle/runtime checks, reporting, high-severity debug rejection, and a
reproducible accepted normal/wireframe reference packet on native Windows. The
SDL-only lifecycle and platform-independent PNG writer are portable across the
observed WSL and native Windows builds, and the controlled mismatch demonstrates
failure-packet retention for the current capture regression. The results do not
establish the engine's future frame-time budgets, broad hardware compatibility,
gameplay quality, or native Linux release support; those remain named gates.
