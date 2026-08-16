# ADR 0001: Native engine foundation

**Status:** Accepted; test-framework clauses amended by
[`ADR 0003`](0003-project-owned-test-harness.md)
**Date:** 2026-08-15
**Decision owner:** Project owner

## Context

Wide Eye can be prototyped quickly in Three.js or built as a clean-room native
voxel game and engine. The next milestone needs one primary track, a release
matrix, an asset policy, measurable budgets, and reproducible dependency rules.
Without those constraints, engine work can expand indefinitely without proving
the herding loop.

## Decision

### Primary track and platforms

- The custom C++ engine is the primary implementation track. The Three.js work
  remains reference material and a possible fallback experiment, not a parallel
  milestone.
- Development and release target native x86-64 **Linux and Windows**. WSL2 is a
  useful development host, but it is not evidence for either native release.
- The first implementation foundation is C++23, CMake presets, Ninja, SDL3,
  OpenGL 4.6 Core, GLSL 4.60, clangd, GDB, and sanitizers where supported. The
  initial doctest selection was superseded before adoption by
  [`ADR 0003`](0003-project-owned-test-harness.md).
- OpenGL 4.6 is an explicit target, not permission to create a compatibility
  context. If a supported machine cannot provide it, the project must record a
  deliberate baseline change before implementation proceeds.

### Asset rule

Use **procedural-first with a gated authored fallback**.

- Through Tracer 2, runtime presentation is code-generated: palettes, shaders,
  voxel geometry, animal placeholders, UI shapes, and diagnostic captures. No
  imported textures, models, animations, fonts, or audio are required to answer
  the first gameplay question.
- Beginning with Tracer 3, a small authored or permissively licensed font,
  essential audio set, or animal rig may be approved when a same-state test
  shows that generated output does not communicate posture, stress, controls, or
  feedback clearly enough.
- Every exception records source, creator/tool, license, allowed use, and why it
  is better than the procedural result.

This differs from strict assetlessness: strict mode would prohibit a tiny font
or readable animal rig even when it improves the game. Procedural-first keeps
the visual identity and small-package pressure without turning file count into
the product goal. Source-code dependencies are not media assets.

### Provisional hardware and budgets

These are design targets, not observed support claims. Native devices matching
both profiles must be named and measured before release support is claimed.

| Profile | Provisional reference class | Display target | Frame-time target | Memory | Startup | Compressed package |
| --- | --- | --- | --- | --- | --- | --- |
| Low | Core i5-1135G7, Intel Iris Xe 80 EU, 8 GiB RAM | 1920x1080, Low, 60 Hz | p95 <= 16.67 ms; p99 <= 25 ms | RSS <= 1 GiB | <= 3 s | <= 64 MiB |
| High | Ryzen 5 5600, Radeon RX 6600, 16 GiB RAM | 2560x1440, High, 60 Hz | p95 <= 16.67 ms; p99 <= 20.84 ms | RSS <= 1.5 GiB | <= 3 s | <= 64 MiB |

Additional gates:

- Fixed gameplay simulation runs at 60 Hz and uses no more than 2 ms p95 on the
  Low profile in the five-sheep gate scenario.
- Tracer 2 has tighter provisional caps of 512 MiB RSS and a 32 MiB compressed
  release archive.
- Measure release builds after a warm-up using a named scenario, seed, duration,
  graphics profile, driver, and capture method. Separate CPU and GPU timings
  where possible, and report stalls rather than averaging them away.
- Debug symbols, captures, and developer tools are distributed separately and
  do not count toward the player package.

### Dependencies and licenses

- Install developer tools through the host package manager or a documented,
  checksum-verified local tool bundle.
- Declare source dependencies in CMake and pin an immutable commit or an archive
  with `URL_HASH`. Never depend on an unpinned `latest` tag for a reproducible
  build.
- Start with SDL3, a generated OpenGL 4.6 Core loader, project-owned focused test
  executables orchestrated by CTest, and one small PNG writer for deterministic
  captures. Dear ImGui remains optional and must earn a current debugging
  requirement. A third-party test framework must meet the adoption gate in
  [`ADR 0003`](0003-project-owned-test-harness.md).
- Prefer MIT, BSD-2-Clause, BSD-3-Clause, Zlib, BSL-1.0, Apache-2.0, CC0, and
  equivalent permissive terms. Public-domain dual licensing is acceptable when
  provenance is recorded.
- LGPL, MPL, GPL, AGPL, source-available, noncommercial, no-derivatives, custom,
  or unlicensed code/assets require explicit review before adoption. GPL/AGPL,
  noncommercial, and no-derivatives material is not part of the default build.
- Keep third-party notices and license text with releases. Record generated
  loader/tool provenance. Do not vendor a dependency merely for convenience;
  vendor only when offline/reproducible builds or upstream stability justifies
  it.

## Consequences

- Linux and Windows parity is part of each meaningful gate, even though Linux is
  the current development environment.
- The first playable spends effort on an observable deterministic harness and a
  readable flock rather than an asset pipeline or advanced renderer.
- A small package remains a guardrail, not a claim that the game must match an
  anecdotal one-megabyte executable.
- The OpenGL 4.6 baseline excludes macOS and some older hardware unless a future
  decision deliberately changes the graphics contract.
- Dependencies remain small and auditable, while “from scratch” continues to
  mean ownership of engine/game architecture rather than refusal to use SDL or
  a test library.

## Evidence status and remaining requirements

- Native Windows compiler/context smoke passed on 2026-08-15 with MSVC
  19.44.35228.0, SDL 3.4.10, and an Intel UHD Graphics 630 OpenGL 4.6 Core debug
  context. Evidence: [`../setup/WINDOWS.md`](../setup/WINDOWS.md).
- The generated loader is glad 2.0.8 at commit
  `73db193f853e2ee079bf3ca8a64aa2eaf6459043`, generated reproducibly for
  OpenGL 4.6 Core with no extensions and SDL-supplied symbol lookup. Its
  generator archive SHA-256, command, generated-file hashes, MIT generator
  terms, Khronos terms, and generated
  `(WTFPL OR CC0-1.0) AND Apache-2.0` expression are retained in
  [`../../third_party/glad/README.md`](../../third_party/glad/README.md) and
  enforced by [`../../cmake/WideEyeDependencies.cmake`](../../cmake/WideEyeDependencies.cmake).
- A native Linux compiler/context smoke test is still required; WSL evidence
  does not satisfy it.
- Actual low/high devices and driver versions.
- First measured frame-time, memory, startup, and package baselines.
- License review of exact dependency revisions added after SDL 3.4.10 and glad
  2.0.8.
