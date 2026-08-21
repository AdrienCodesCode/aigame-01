# Agent harness, MCP, skills, and technical references

## Purpose

This is the durable tool-selection record for developing **Wide Eye** as a
clean-room C++ voxel engine and game. It tells future context windows what Codex
needs in order to edit, build, observe, debug, and verify the native executable,
and which external integrations have actually been investigated.

Research and local tool survey last performed: **2026-08-16**.

The accepted closed feedback loop, verification cadence, failure protocol, and
human/context boundaries are defined in
[`DEVELOPMENT_WORKFLOW.md`](DEVELOPMENT_WORKFLOW.md). This document owns harness
capabilities and tool trust; it does not define a second development loop.

## Terms

| Layer | Purpose in this project |
| --- | --- |
| `AGENTS.md` | Durable repository rules, architecture boundaries, evidence standards, and commands every agent must follow |
| Prompt | One bounded assignment with explicit inputs and deliverables |
| Skill | A reusable workflow in `SKILL.md`, optionally backed by references and deterministic scripts |
| MCP server | A local or remote process that exposes structured tools and live context to Codex |
| Plugin | A distributable bundle that can include skills, MCP servers, connectors, and metadata |
| Harness | The closed repository-native build, run, observe, compare, review, and preserve loop that makes the game verifiable to humans and agents |

Official Codex behavior is documented in [OpenAI's MCP
guide](https://learn.chatgpt.com/docs/extend/mcp), [skills
guide](https://learn.chatgpt.com/docs/build-skills), and [`AGENTS.md`
guide](https://learn.chatgpt.com/docs/agent-configuration/agents-md).

MCP is optional. The repository harness is mandatory.

## Finding

No official Khronos OpenGL MCP server, official ISO C++ MCP server, official LLVM
clangd MCP server, or OpenAI-curated C++/OpenGL/game-engine skill was found in
the sources reviewed on 2026-08-14.

That is not a blocker. CMake, compilers, tests, sanitizers, clangd, GDB,
RenderDoc, engine CLI commands, deterministic replays, and captured PNG/JSON
artifacts already provide the fundamental development interfaces. MCP can wrap a
proven interface later; it should not replace one.

## Current Codex skill and plugin availability

The official `openai/skills` curated catalog was queried with the installed skill
installer. It contained general engineering skills such as Playwright,
screenshot, security, GitHub CI, deployment, and OpenAI documentation, but no
native C++, CMake, OpenGL, SDL, voxel-engine, RenderDoc, or game-simulation
skill. The former experimental catalog path was not present when queried.

Relevant capabilities already available in this environment:

- `skill-creator`: can create a repository-scoped workflow after we have a
  workflow worth preserving.
- `imagegen`: can develop concept art, palettes, silhouettes, and mood references;
  it is not an authoritative runtime asset pipeline.
- `playwright`: useful for browser builds or project web pages, not for inspecting
  a native OpenGL executable.
- Local image inspection: can review PNG captures emitted by the engine.
- Shell and patch tools: sufficient to build and test a well-designed CLI
  harness.

Repository-scoped workflow skills added on 2026-08-15:

- `end-engine-session`: proportional verification followed by an evidence-backed
  `ROADMAP.md` checkpoint.
- `update-project-docs`: scoped synchronization of roadmap, architecture,
  design, research, plan, harness, and prompt responsibilities.
- `deep-research`: primary-source research for consequential C++/OpenGL/voxel
  decisions without making an MCP connection a universal precondition.
- `plan-from-research`: adversarial review, architecture gate, and checkable
  implementation planning without silently implementing the result.

These reuse the sound workflow ideas from the Unity project in `testing-01`, but
were rewritten rather than copied: Unity Editor/MCP preflights, DeepSeek/model
prefix routing, mandatory model profiles, PowerShell line-ending scripts,
`SESSION_HANDOFF.md`, and Unity-specific multiplayer rules were removed. The
four skills validate with the official local `skill-creator` validator.

Potential catalog plugins:

- **GitHub** could help with issues, pull requests, and CI later. Plain `git` and
  the GitHub CLI are sufficient for local implementation.
- **Sentry** could help after native crash reporting becomes a product
  requirement. It should not enter the first playable tracer.
- **Figma** can help with HUD/UI design if we create an authored interface. It
  does not help core voxel rendering or herding simulation.

None of these plugins is required before engine work begins.

## Investigated MCP candidates

These verdicts are time-bounded. Before installing a community server, inspect
its current source, license, releases, dependencies, open security reports, tool
surface, and network behavior; then pin an exact release or commit.

| Candidate | Provenance and capability | Verdict |
| --- | --- | --- |
| [mcp-cpp](https://github.com/mpsm/mcp-cpp) | Community MIT project wrapping clangd for CMake/Meson project discovery, symbol search, call/inheritance context, and multiple build configurations | **Installed by owner request; use remains gated.** Version 0.2.2 is checksum-verified and registered as `cpp`. Its startup test detects local clangd 18.1.3. That test predates the standalone Phase 0 diagnostic, which has not been inspected through the MCP and is not an engine component. |
| [Microsoft DebugMCP](https://github.com/microsoft/DebugMCP) | Microsoft-maintained MIT VS Code extension exposing breakpoint, stepping, variable, expression, and test-debug controls; includes a reusable debugging skill and advertises C/C++ support | **Pending explicit risk approval.** The VS Code extension was not installed after the host safety gate rejected an outside-workspace executable install. If approved, keep its unauthenticated HTTP endpoint loopback-only and never blanket-auto-approve expression evaluation. |
| [renderdoc-mcp](https://github.com/JiaboLi-GitHub/renderdoc-mcp) | Community MIT wrapper around RenderDoc replay with frame, pipeline, shader, resource, pixel, capture-diff, and export tools; ships a Codex skill | **Installed for the Windows target; use remains gated.** Version 0.3.0 is the newest release with a packaged artifact, checksum-verified and registered as `renderdoc-mcp`. The Windows x64 CLI starts through WSL interoperability. Native Linux support and real capture/replay remain unverified. It is not maintained by RenderDoc. |
| [GDB MCP](https://github.com/Ipiano/gdb-mcp) | Community MIT Python bridge to GDB/MI with execution, breakpoint, thread, core-dump, variable, register, and expression tools | **Installed by owner request; use remains gated.** Commit `605220a4bbbbbe2e87629f29dc1136fb970f6525` is installed with MCP SDK `<2` and registered as `gdb`; a local GDB 15.1 runtime starts. Command, function-call, and expression tools keep prompt approval. |
| [Blender Lab MCP](https://www.blender.org/lab/mcp-server/) | Official Blender Lab project providing natural-language access to Blender's Python API and scene analysis; source at [Blender Projects](https://projects.blender.org/lab/blender_mcp) | **Official but experimental and optional.** It requires Blender 5.1 or newer. Blender explicitly warns that it executes model-generated code without guards and recommends isolation from sensitive data. Use only if an authored Blender pipeline earns its cost; the assetless procedural path does not need it. |
| Generic documentation MCP such as Context7 | Community documentation lookup rather than compiler, renderer, or game control | **Optional convenience.** Prefer the official sources below and record versions. It does not replace builds, tests, runtime captures, or profiling. |

## MCP installation record

Installation date: **2026-08-15**. Binaries and isolated environments live in
the ignored `.tools/` directory. The versioned project configuration is
`.codex/config.toml`; it uses relative paths and conservative approval defaults.
The same three servers were also registered in the current Codex user
configuration so they can be discovered after restart.

| MCP | Pin and integrity | Verified locally | Important limit |
| --- | --- | --- | --- |
| `cpp` | `mcp-cpp-server` 0.2.2 Linux x86-64; SHA-256 `555535c69a5087420ea283e323ff07c587d37a0a38c92b72e3765a898e83cfb4` | Server starts and finds clangd 18.1.3; its zero-component scan predates the standalone Phase 0 diagnostic | Needs the project config's local library path and a future engine `compile_commands.json`; the diagnostic has not been inspected through the MCP and current-session tools do not hot-load |
| `gdb` | GDB MCP commit `605220a4bbbbbe2e87629f29dc1136fb970f6525`; upstream dependency constrained to `mcp>=0.9,<2` | Server starts with the extracted Ubuntu GDB 15.1 runtime; direct GDB stopped `/bin/true` at its first instruction and read `rip` | No engine executable or MCP debug session exists; process-mutating and expression operations require approval |
| `renderdoc-mcp` | Windows x64 0.3.0 package; SHA-256 `08cc5ffe4a9eeb34308096e72830461b36d81102fd9f1477cebcb6179e8f6e02` | MCP server and CLI start through WSL interoperability; CLI help enumerates capture, frame, shader, resource, pixel, diff, and assertion commands | Windows package only; no `.rdc` was opened and native Linux capture/replay is unverified. The newer 0.3.1 release had no downloadable artifact when checked |

The first user-level `cpp` registration was created before clangd's extracted
library path was discovered and lacks `LD_LIBRARY_PATH`. The project-scoped
`.codex/config.toml` contains the corrected environment and is authoritative for
this repository. Replacing the redundant user-level entry was rejected by the
host's outside-workspace configuration guard pending fresh informed approval;
do not rely on that global entry from another project.

The GDB project currently declares `mcp>=0.9.0`; that admitted incompatible MCP
SDK 2.0 and failed at import because `Server.list_tools` was absent. The local
install therefore constrains the SDK to the latest compatible 1.x line, resolved
as 1.29.0 on installation. Re-test before updating either pin.

The owner reported installing the C/C++ extension pack offered by VS Code on
2026-08-15. Its exact extension IDs, versions, and operation against this
project have not been verified from the repository. DebugMCP 2.3.0 remains
uninstalled; it runs an unauthenticated local HTTP server and exposes evaluation
and launch controls. If approved later, retain its default loopback bind,
register `http://localhost:3001/mcp`, and require prompts for debugger actions.

Configuration does not make tools appear in an already running Codex session.
Restart Codex from this trusted repository, then inspect `/mcp` before relying on
the tools. A successful startup is not evidence that semantic navigation,
debugging, or GPU capture works on engine code; each still needs its first real
test at the relevant roadmap gate.

### What was not found

- A Khronos-provided OpenGL MCP server.
- An ISO C++ or Standard C++ Foundation MCP server.
- An LLVM-maintained clangd MCP bridge.
- A Kitware-maintained CMake MCP server.
- A RenderDoc-maintained MCP server.
- An OpenAI-curated native game-engine development skill.

Community projects may use those product names without being maintained or
endorsed by the underlying standards body or tool author.

## Official and primary technical references

Use primary documentation before secondary tutorials for API behavior. Pin
versions in architecture decisions and bug reports.

### C++ and compiler tooling

- [clangd](https://clangd.llvm.org/) and its [project setup
  guide](https://clangd.llvm.org/installation.html), including compilation
  databases.
- [Clang documentation](https://clang.llvm.org/docs/) and [AddressSanitizer
  guide](https://clang.llvm.org/docs/AddressSanitizer.html).
- [GCC manuals](https://gcc.gnu.org/onlinedocs/).
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
  for design guidance; these are guidelines, not the language standard.
- [cppreference](https://en.cppreference.com/w/) as a practical community
  language/library reference; verify subtle normative questions against the
  applicable standard/toolchain.

### Build and platform layer

- [CMake documentation](https://cmake.org/cmake/help/latest/) and [CMake
  presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html).
- [SDL3 API by category](https://wiki.libsdl.org/SDL3/APIByCategory).

### OpenGL and GLSL

- [Khronos OpenGL Registry](https://registry.khronos.org/OpenGL/index_gl.php)
  for formal API, GLSL, extensions, headers, and XML registry.
- [OpenGL 4.6 reference pages](https://registry.khronos.org/OpenGL-Refpages/gl4/html/start.html)
  for command and shader-function lookup.
- [Khronos OpenGL reference hub](https://registry.khronos.org/OpenGL-Refpages/)
  for version information, quick cards, and related tools.
- [RenderDoc source and documentation](https://github.com/baldurk/renderdoc) for
  GPU capture and frame analysis.

### Vulkan — prospective, not an approved backend

- [Khronos Vulkan documentation](https://docs.vulkan.org/) for the specification,
  guide, samples, tutorial, GLSL, and API reference pages.
- [Vulkan versions and porting
  guide](https://docs.vulkan.org/guide/latest/versions.html) for core-version,
  feature, extension, and SPIR-V compatibility rules.
- [Vulkan Profiles](https://docs.vulkan.org/guide/latest/vulkan_profiles.html) for
  expressing a versioned capability baseline after the named target devices are
  inventoried.
- SDL's [`SDL_Vulkan_CreateSurface`](https://wiki.libsdl.org/SDL3/SDL_Vulkan_CreateSurface)
  and [`SDL_Vulkan_GetInstanceExtensions`](https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions)
  for the existing platform layer's prospective surface integration.

These references and the local capability probe below do not supersede the
accepted OpenGL foundation or authorize Vulkan implementation. The sourced
decision analysis is in the
[OpenGL-to-Vulkan feasibility research](research/opengl-to-vulkan-feasibility.md).

The formal Khronos specifications are authoritative but written largely for
implementers. A tutorial may help explain an idea, but it does not override API
requirements or driver evidence.

## Local toolchain observation

System PATH checks plus project-local runtime checks began on 2026-08-15. Rows
with later observations state their date explicitly.

| Tool | State |
| --- | --- |
| GNU `g++` | System install present at `/usr/bin/g++`, version 13.3.0 |
| `xvfb-run` | System install present at `/usr/bin/xvfb-run`; explicit 4.5 Core context probe passed |
| CMake | System install present at `/usr/bin/cmake`, version 3.28.3; verified local fallback also present |
| Ninja | System install present at `/usr/bin/ninja`, version 1.11.1; verified local fallback also present |
| Clang/`clang++` | System Clang 18 install present, version 18.1.3; verified local fallback also present |
| clangd | System install present at `/usr/bin/clangd-18`, version 18.1.3; verified local fallback also present |
| clang-format/clang-tidy | System Ubuntu LLVM 18.1.3 executables are present at `/usr/bin`; verified local fallback also present; project-only CMake targets passed against the SDL lifecycle source |
| GDB | System install present at `/usr/bin/gdb`, version 15.1; verified local fallback also present |
| `pkg-config` | System install present at `/usr/bin/pkg-config`, version 1.8.1; verified local fallback also present |
| SDL3 development files | SDL 3.4.10 source archive checksum-verified, built, and consumed by CMake |
| OpenGL/Mesa development files | System install and verified local fallback present; Mesa 25.2.8 context observed |
| Windows MSVC | Visual Studio Build Tools 2022 17.14.37; toolset 14.44.35207, compiler 19.44.35228.0 |
| Windows CMake/Ninja | Build Tools bundles CMake 3.31.6-msvc6 and Ninja 1.12.1 |
| Native Windows OpenGL | Intel UHD Graphics 630 driver 27.20.100.9664 passed explicit OpenGL 4.6 Core/GLSL 4.60 debug contexts three times |
| Windows Vulkan capability diagnostic | Observed 2026-08-16 through WSL interoperability: `vulkaninfo.exe --summary` reported loader 1.4.309, Intel UHD 630 Vulkan 1.2.177, and GTX 1050 Ti Max-Q Vulkan 1.4.312. The full device report exposed neither mesh-shader nor KHR ray-query/ray-tracing extensions on either GPU; the Khronos validation layer and `glslc` were not found. This is capability/tooling evidence only, not an approved or working project backend. |
| RenderDoc CLI | Windows x64 package 0.3.0 starts through WSL interoperability; capture/replay unverified |
| Blender | Missing |
| ccache | System install present at `/usr/bin/ccache`, version 4.9.1; verified local fallback also present |

The WSL2 host reports an Intel Core i9-8950HK with 8 logical CPUs and 11 GiB of
visible memory. The observed X11 renderer is unaccelerated Mesa 25.2.8
`llvmpipe`: explicit OpenGL 4.5 Core/GLSL 4.50 debug contexts passed in the
normal WSLg session and under `xvfb-run`, while explicit 4.6 context creation
failed in both paths with `GLXBadFBConfig`. This is not a named native Linux or
Windows graphics test machine and does not satisfy the approved 4.6 baseline.
Reproduction commands and exact evidence are in the
[Ubuntu 24.04 setup](setup/UBUNTU_24_04.md).

The owner retained OpenGL 4.6 and selected native Windows as the first
hardware-rendering path. On 2026-08-15, the repository runner imported the
installed Build Tools environment, built the pinned SDL 3.4.10 diagnostic with
MSVC/C++23 and Ninja, and passed the explicit OpenGL 4.6 Core debug-context gate
three times. Windows selected the Intel UHD Graphics 630 rather than the
inventoried NVIDIA GTX 1050 Ti Max-Q. Exact commands, versions, source hashes,
and the retained final log are recorded in the
[Windows setup](setup/WINDOWS.md).

The Vulkan diagnostic also reported a stale Epic Online Services overlay-layer
JSON path before successfully enumerating both devices. A future Vulkan setup
gate must provision and verify the Khronos validation layer and a pinned
GLSL-to-SPIR-V compiler, resolve or explicitly quarantine loader warnings, and
rerun capability checks on the named Low/High target classes and native Linux.
Until then, `vulkaninfo` availability proves enumeration only.

## Required repository-native harness

The target interface is one-command configuration, build, test, deterministic
scenario execution, and artifact capture.

**Observed result (2026-08-15):** the scaffold implements and verifies the first
three commands below plus bounded tracer-specific window, context, triangle,
cube, and capture commands. WSL Ubuntu Clang 18.1.3 passed development and
ASan/UBSan presets, WSL Ubuntu GCC 13.3.0 passed release, and native Windows MSVC
19.44.35228.0 passed the 15-test development tracer suite. Build directories are
separated by host system, and clangd reads the WSL/Linux development compilation
database. The native Windows suite validates OpenGL 4.6 Core/GLSL 4.60 on Intel
UHD Graphics 630, rejects high-severity debug messages, checks triangle, cube,
and wireframe framebuffer oracles, and requires repeated normal and wireframe
captures to be byte-identical. Its runner emits a versioned, hashed
artifact/failure manifest with exact commands, platform/configuration, parsed
state, logs, source hashes, and captures; passing runs also emit the candidate
owner-review record with no preselected verdict. The owner accepted the first
packet on 2026-08-15 after checking the interactive window and both captures.
The resulting checked-in
[reference-machine baseline](../tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md)
has a platform-independent CTest guard for its manifest-linked hashes and single
recorded Accept verdict.
Every registered CTest also rejects project failure markers and ASan, LSan, or
UBSan diagnostics; a nested fixture verifies that common guard independently of
the engine executable.
The same 4.6 request fails with `GLXBadFBConfig` on the 4.5-limited WSL host; no
fallback is implemented. The `game` boundary now has implemented version 1
seed/action/replay types, a versioned dog, five-sheep, social-evidence, and
dog-stimulus-evidence state dump whose current version number is owned by the
format contract, pre-mutation compatibility validation, an in-memory replay
executor, and canonical JSON writers. A general
JSON decoder, executable replay/seed/state-dump flags, persistent replay
fixtures, remaining sheep behavior/objective state, native Linux graphics,
broader gameplay debug views, metrics, and later performance budgets remain
unverified or unimplemented. See the
[format contract](formats/GAMEPLAY_REPLAY_AND_STATE.md).

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev

./build/Linux/dev/wide_eye \
  --scenario gate-01 \
  --seed 42 \
  --replay tests/replays/basic-gather.json \
  --frames 900 \
  --capture artifacts/gate-01.png \
  --dump-state artifacts/gate-01.json \
  --headless
```

The harness must eventually provide:

- Checked-in CMake configure, build, and test presets.
- `compile_commands.json` for clangd and static tools.
- Fixed-tick simulation independent of render cadence.
- Named, tiny deterministic scenarios.
- Versioned seed and input replay format. The dog-only version 1 typed contract
  and canonical JSON writer exist; general file decoding and CLI execution are
  pending.
- Headless or virtual-display execution.
- Deterministic PNG capture and optional short frame sequences.
- Structured JSON state and metrics output. Canonical dog, sheep,
  chosen-neighbor, social-influence, and dog-stimulus JSON exists as a
  game-owned writer at the version recorded in the
  [format contract](formats/GAMEPLAY_REPLAY_AND_STATE.md); broader executable
  output paths and metrics are pending.
- Debug views for voxel chunks/meshes, collision, dog pressure, sheep neighbors,
  steering influences, arousal/recovery, group observables, objective state, and
  timings.
- Unit tests for math, coordinates, chunk boundaries, meshing, simulation,
  replay, and serialization.
- Warnings-as-errors for project code plus sanitizer presets.
- CPU/GPU frame-time, memory, allocation, draw, mesh, upload, and chunk budgets.
- An immutable last-known-good capture/replay set for material changes.
- CTest labels and named commands for `unit`, `scenario`, `headless`,
  `sanitizer`, and `performance`, plus separately recorded `manual` gates.
- Failure retention containing the command, configuration, platform, seed,
  replay, relevant logs, state/metrics, capture, and manifest.
- A versioned artifact manifest containing reproducibility and comparison
  metadata defined by the development workflow.

For visual agent work, the executable must save images to the repository's
ignored `artifacts/` directory. A native window visible only to a human is not an
adequate verification interface.

Generated captures are candidates, not accepted goldens. Material visual gates
must use the [human visual-review packet](review/HUMAN_VISUAL_REVIEW.md), and
only an explicit owner `accept` verdict can promote or replace a baseline.
The accepted Tracer 0 packet under [`tests/goldens/`](../tests/goldens/README.md)
is the first application of this rule; it remains a reference-machine
engineering baseline rather than a cross-GPU image-identity claim.

## MCP use gates

The owner requested early installation of the four non-Blender candidates so
they are ready later. Installation does not waive the original use gates:

- Use the clangd MCP after a correct compilation database exists and semantic
  navigation answers a concrete code question better than repository search.
- Install and use DebugMCP only after the owner approves its local code-execution
  surface and a reproducible bug needs interactive stepping or variable
  inspection beyond logs, tests, sanitizers, and replay.
- Use RenderDoc MCP after a real `.rdc` capture exists and structured frame
  inspection answers a GPU diagnosis or evidence-extraction need.
- Use GDB MCP only on a named debug executable or core dump; keep process
  mutation, expression evaluation, and inferior function calls approval-gated.
- Add Blender MCP only after the project approves Blender-authored assets and an
  isolated Blender environment. It remains intentionally uninstalled.
- Build a custom game MCP only after stable CLI/debug APIs exist. The MCP should
  wrap narrow read/capture/replay controls, not arbitrary shell or C++ evaluation.

Any MCP configuration must:

- Bind locally unless remote access is explicitly designed and authenticated.
- Expose the smallest necessary tool allowlist.
- Prompt for writes, process mutation, arbitrary expression evaluation, and
  generated-code execution.
- Avoid inheriting secrets unnecessarily.
- Be pinned and reproducible.
- Have a documented removal/fallback path.

## Future engine-specific skills

The four installed repository skills govern research, planning, documentation,
and handoff. Do not add a large implementation/diagnostic skill before its
engine workflow has succeeded at least twice. Once proven, use `skill-creator`
to add focused skills under `.agents/skills/`:

1. `voxel-engine-tracer`: advance one roadmap tracer and enforce build/test/
   capture/profile gates.
2. `herding-simulation-diagnostics`: replay and explain pressure, neighbor,
   arousal/recovery, collision, group observables, and state transitions.
3. `visual-regression-review`: capture comparable frames/motion and review
   readability, style, image stability, and performance.

Each skill should reference normal project commands. It must not introduce a
second private build or testing pathway.

## Procedural asset implication

A code-generated voxel project does not need Blender or an asset MCP to begin.
Terrain, material palettes, stone walls, gates, vegetation, barns, dog/sheep
parts, markings, and animation transforms can all be represented as code and
data generated from deterministic rules.

“No asset files” still involves art direction: palettes, proportions, shape
grammars, placement rules, animation curves, shader logic, and seeds are assets
in the creative sense. Preserve the option to introduce a small authored format
later if strict assetlessness prevents readable dog posture or sheep behavior.

## Continuation rule

Future context windows must read, in order:

1. [`AGENTS.md`](../AGENTS.md).
2. [`ROADMAP.md`](../ROADMAP.md), especially **Current checkpoint**.
3. [`DEVELOPMENT_WORKFLOW.md`](DEVELOPMENT_WORKFLOW.md) before implementation,
   diagnosis, verification, visual review, or a context handoff.
4. [`VOXEL_ENGINE_OPTION.md`](VOXEL_ENGINE_OPTION.md).
5. [`WIDE_EYE.md`](game-design/WIDE_EYE.md) before changing the first playable.
6. [`HERDING_GAMEPLAY.md`](game-design/HERDING_GAMEPLAY.md), the
   [herding research](research/herding-simulation-and-scale.md), and the
   [implementation plan](plans/herding-simulation-and-scale.md) before changing
   sheep behavior, scale, rewards, progression, or animals.
7. This document when installing, changing, or diagnosing the harness or an MCP.

Proceed from the first unchecked item in the current roadmap milestone, not from
the most visually exciting subsystem.
