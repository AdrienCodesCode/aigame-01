# Ubuntu 24.04 development setup

## Scope

This setup supports the Phase 0 compiler and SDL3/OpenGL context diagnostic on
Ubuntu 24.04. It is development-host evidence, not a native Linux or Windows
release-support claim.

SDL is a source dependency, not a required system package. The diagnostic CMake
project pins the upstream
[SDL 3.4.10 release](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.10)
archive with SHA-256
`12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785`.

## Preferred system setup

```bash
sudo apt-get update
sudo apt-get install --yes \
  cmake ninja-build clang-18 clangd-18 clang-format-18 clang-tidy-18 \
  gdb pkg-config ccache \
  libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev mesa-common-dev \
  mesa-utils xvfb xauth \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev \
  libxi-dev libxss-dev libxtst-dev libxkbcommon-dev
```

The [official SDL Linux build document](https://wiki.libsdl.org/SDL3/README-linux)
lists additional optional audio, input, Wayland, and device backends. Add those
only when the corresponding runtime subsystem enters scope; the Phase 0
diagnostic deliberately requires only an X11 OpenGL context.

When `sudo` is unavailable, extract the same Ubuntu packages into the ignored
local tool root. This fallback assumes the host already has Ubuntu's base G++
runtime and development headers:

```bash
./tools/phase0/bootstrap-ubuntu-local.sh
```

The fallback records hashes of the resolved Ubuntu packages under
`.tools/packages/phase0/`. Ubuntu packages are host-tool inputs and may advance
with Noble updates; the observed versions must therefore be recorded with each
milestone result. SDL remains pinned and checksum-verified by CMake.

## Context diagnostic

The default command requests the approved OpenGL 4.6 Core profile. It never
falls back to a compatibility profile or a lower version:

```bash
./tools/phase0/run-context-smoke.sh
```

Probe the available virtual-display ceiling separately without changing the
approved target:

```bash
xvfb-run -a ./tools/phase0/run-context-smoke.sh 4 5
```

A successful run reports the SDL version and revision, video driver, OpenGL
vendor, renderer, GL/GLSL versions, core-profile bit, and debug-context bit,
then destroys the context and window before exiting. A lower-version probe is
capability evidence only; it is not permission to revise the OpenGL baseline.

## Phase 1 project context smoke

After configuring and building the project, exercise the executable's bounded
context-reporting path with:

```bash
./build/Linux/dev/wide_eye --context-smoke
```

The checked-in Linux presets leave the display-backed context CTest disabled so
the known WSL 4.5 ceiling does not break the default compiler and SDL lifecycle
suite. On a native Linux machine that provides the approved baseline, configure
with `-DWIDE_EYE_ENABLE_OPENGL_CONTEXT_TEST=ON` to register
`wide_eye.opengl_context_smoke`, then build and run `ctest --preset dev`.

## Observed development host — 2026-08-15

**Observed result:**

- OS: Ubuntu 24.04.4 LTS under WSL2 kernel
  `6.6.87.2-microsoft-standard-WSL2`.
- CPU/compiler host: x86-64; GCC 13.3.0 was preinstalled.
- The owner subsequently completed the preferred `apt-get` installation with
  the interactive `sudo` password. System tools are now present at `/usr/bin`:
  CMake 3.28.3, Ninja 1.11.1, Clang/clangd/clang-format/clang-tidy 18.1.3,
  GDB 15.1, pkg-config 1.8.1, and ccache 4.9.1. The listed X11 and Mesa
  development packages are installed.
- Before that system installation, the ignored local fallback was built and
  verified end to end with the same tool versions. It remains an optional
  recovery path rather than the preferred active setup.
- LLVM 18.1.3 `clang-format` and `clang-tidy` were initially added to that
  ignored local fallback because an automated session could not answer the
  host's interactive `sudo` password prompt. The owner later installed both
  system packages. The documented project checks now prefer `/usr/bin` and
  passed against all six current Wide Eye source/test translation units; the
  fallback remains available.
- SDL source: 3.4.10, revision `SDL-release-3.4.10-0-g8e37db5e7`, zlib license,
  checksum as recorded above.
- Normal WSLg X11 probe: Mesa 25.2.8 software `llvmpipe` renderer; maximum
  reported OpenGL 4.5 Core and GLSL 4.50; not accelerated.
- Normal WSLg X11 SDL probe: explicit 4.5 Core debug context passed; explicit
  4.6 context creation failed with `GLXBadFBConfig`.
- Xvfb SDL probe: explicit 4.5 Core debug context passed; explicit 4.6 context
  creation failed with `GLXBadFBConfig`.
- The Phase 1 project executable's direct development and ASan/UBSan
  `--context-smoke` paths requested OpenGL 4.6 Core and failed at context
  creation with the same `GLXBadFBConfig`. The development, ASan/UBSan, and
  release builds and their eight default CTests passed. Those tests cover exact
  known-byte PNG encoding, accepted Tracer 0 packet integrity, core timing,
  window state, fatal assertions, SDL lifecycle/event mapping, and rejection of
  project or sanitizer failure diagnostics even when a pass marker is present;
  this confirms the platform-independent capture codec, baseline guard, and
  no-fallback failure path, not native Linux graphics support.
- Local logs and source hashes are under the ignored
  `artifacts/phase0/2026-08-15/` evidence directory.

**Owner decision and inference:** Retain the approved OpenGL 4.6 target. WSL is
the everyday development host, not a supported native graphics target; its 4.5
ceiling therefore does not revise the baseline. The required hardware context
evidence moves to native Windows as documented in
[`WINDOWS.md`](WINDOWS.md). Native Linux support remains unverified until the
same gate runs on an actual Linux installation or machine.
