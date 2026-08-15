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

The script imports the x64 MSVC environment, copies the project-owned CMake,
source, and test inputs into a new timestamped `%LOCALAPPDATA%\WideEye` directory,
records their SHA-256 hashes, and uses the checked-in `dev` preset. It runs the
version, PNG-writer, core runtime, window state, fatal assertion, dummy-driver
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

## Observed Windows host — 2026-08-15

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
