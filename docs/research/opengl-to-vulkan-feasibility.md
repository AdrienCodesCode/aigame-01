# Research: OpenGL-to-Vulkan timing and AI-readable rendering workflow

**Status:** Draft research; not implemented

**Workflow integration:** Owner-authorized documentation and future-roadmap
integration on 2026-08-16; no Vulkan migration or advanced renderer feature was
approved or implemented

**Produced by:** Codex

**Date:** 2026-08-16

**Project revision:** `d50d7fce1b6724952c55baed412469a3dc2b6754`;
clean worktree before this research change

**Adversarial review:** Not yet reviewed

## Problem and decision

The project owner supplied
[`ref/AI_Rendering_Engine_Feasibility.md`](../../ref/AI_Rendering_Engine_Feasibility.md),
an external GPT conversation proposing an AI-readable graphics laboratory and
raising Vulkan as a better long-term foundation for mesh shaders, ray tracing,
GPU-driven rendering, explicit memory management, and modern vendor SDKs.

This research answers four decisions:

1. Which advice from that conversation should improve the accepted development
   workflow?
2. Would Vulkan make the current game look better?
3. Is the renderer early enough that migration is sensible, and how much of the
   engine would actually change?
4. When, if ever, should Vulkan work interrupt the current first-playable
   roadmap?

Success means preserving the fastest route to evidence about the herding loop,
while avoiding an OpenGL architecture that would make a later, justified Vulkan
migration needlessly expensive. The answer must preserve deterministic game
simulation, procedural-world ownership, accepted visual evidence, native
Windows/Linux intent, and the provisional low/high hardware budgets.

This is not an implementation plan, an approval to change the graphics API, a
claim that either API can deliver “AAA” quality, or a decision to adopt mesh
shaders, ray tracing, Brixelizer GI, a render graph, or a permanent multi-backend
engine.

**Recommendation:** do **not** ask Codex to “rewrite the engine in Vulkan” now.
Keep OpenGL as the accepted backend through the Phase 3 first-playable evidence
gate. At that gate, make an explicit product-versus-engine-learning decision. If
Vulkan still serves an approved destination, insert a bounded Vulkan parity
tracer before the renderer expands materially in Phases 4–6. Preserve the
current high-level render inputs and game/simulation systems; run OpenGL and
Vulkan side by side only long enough to establish parity, then choose one
primary backend.

## Verified project constraints

### Current milestone and product evidence

- **Confirmed fact:** [`ROADMAP.md`](../../ROADMAP.md) places the project in
  Phase 3. The five sheep and their render proxies exist, but sheep behavior,
  dog pressure, the gate objective, success/failure, HUD, and fresh-player
  evidence do not. The next accepted work is close-range sheep/sheep repulsion,
  not renderer depth.
- **Confirmed fact:** the primary playtest question is whether a first-time
  player can intentionally steer five mixed-temperament sheep through one gate.
  A graphics-API migration cannot answer that question by itself.
- **Confirmed fact:** [ADR 0001](../decisions/0001-native-foundation.md) accepts
  C++23, SDL3, OpenGL 4.6 Core, and GLSL 4.60 as the current foundation. Changing
  that contract requires a deliberate, owner-approved decision; a new
  conversation cannot silently supersede it.
- **Confirmed fact:** the latest accepted Windows presentation packet measured
  synchronized frame p95/p99 at 3.4986/4.8862 ms and GPU p95/p99 at
  2.367409/2.656996 ms at 1920×1080 on the available Intel UHD 630 proxy. No
  named budget failed, so there is no measured performance bottleneck currently
  requiring Vulkan.
- **Confirmed fact:** the provisional Low target is an Iris Xe 80 EU at
  1920×1080/60, and the High target is a Radeon RX 6600 at 2560×1440/60. Neither
  named target has been measured. The accepted UHD 630 result is only a proxy.

### Existing renderer boundary and migration surface

- **Confirmed fact:** authoritative gameplay, fixed-step simulation, flock
  state, procedural chunk mesh data, and immutable render snapshots do not call
  OpenGL. [`opengl_renderer.hpp`](../../src/render/opengl_renderer.hpp) already
  accepts renderer-facing values such as `CameraPose`, `DogRenderPose`,
  `SheepProxyPoseBuffer`, `HandcraftedPaddockFrame`, `ChunkMesh`, and
  `PaddockPalette`.
- **Confirmed fact:** direct OpenGL usage is concentrated in two source files:
  [`opengl_renderer.cpp`](../../src/render/opengl_renderer.cpp) and
  [`window_runtime.cpp`](../../src/platform/window_runtime.cpp). The former is
  1,631 lines with 306 direct `gl*` call sites; the latter is 681 lines with 21
  direct `gl*` call sites. These counts measure call sites, not migration effort.
- **Confirmed fact:** the renderer currently owns seven small program pipelines,
  eleven embedded GLSL source strings, vertex/index buffers and vertex arrays,
  one static shadow framebuffer/texture, debug-line resources, framebuffer
  readback, and GPU timestamp queries. It is a compact forward raster renderer,
  not yet a streaming, bindless, deferred, GPU-driven, or ray-traced renderer.
- **Confirmed fact:** the platform runtime owns the SDL OpenGL context, loader,
  debug callback, swap, and query-result path. CMake, setup docs, CTest labels,
  source-hashed Windows runners, and accepted visual goldens also encode OpenGL.
  Migration is therefore broader than replacing draw calls, even though it does
  not require rewriting the game.
- **Qualified finding:** the present high-level separation is sufficient to
  preserve gameplay and world systems during a backend migration. The platform
  and renderer internals need new ownership, but the repository does not need a
  speculative universal resource API before a Vulkan proof exists.

### Observed Vulkan capability on the available Windows machine

The following is a local diagnostic observation, not release-support evidence.
On 2026-08-16, `vulkaninfo.exe --summary` and the full text capability report
were run from WSL against the installed Windows Vulkan loader and drivers.

| Item | Observed result | Consequence |
| --- | --- | --- |
| Instance loader | Vulkan 1.4.309 | The loader can enumerate newer APIs; this does not raise a physical device's supported API version. |
| Intel UHD Graphics 630, driver 100.9664 | Vulkan 1.2.177; `VK_KHR_synchronization2`, timeline semaphores, descriptor indexing, buffer device address, and indirect draw count reported; no dynamic-rendering, mesh-shader, ray-query, acceleration-structure, or ray-tracing-pipeline extension reported | A Vulkan 1.3-only conventional backend would exclude the currently accepted Windows proxy. The cited advanced features are unavailable on this device. |
| GeForce GTX 1050 Ti Max-Q, driver 581.57 | Vulkan 1.4.312 and dynamic rendering reported; no mesh-shader, ray-query, acceleration-structure, or ray-tracing-pipeline extension reported | A high core version still does not imply mesh shading or hardware ray tracing. |
| Instance layers | NVIDIA Optimus/present layers only; `VK_LAYER_KHRONOS_validation` was not enumerated | A serious Vulkan implementation must first provision and verify validation tooling. |
| Shader compiler | No `glslc` command found on the current WSL path | The build has no verified GLSL-to-SPIR-V toolchain yet. |
| Diagnostic warning | A stale Epic Online Services overlay-layer JSON path was reported | This did not prevent device enumeration, but the eventual native setup gate should resolve or explicitly quarantine loader warnings. |

- **Confirmed fact:** mesh shading and ray tracing are separate Vulkan device
  extensions with feature/property queries, not visual-quality switches implied
  merely by selecting Vulkan. Khronos defines `VK_EXT_mesh_shader` as a device
  extension and `VK_KHR_ray_tracing_pipeline` as a device extension that also
  depends on acceleration structures.
- **Unresolved:** Vulkan support, extensions, limits, formats, driver quality,
  and performance on the named Iris Xe and RX 6600 targets and on native Linux.
  Those machines must be queried directly before choosing a baseline.

## Findings

### 1. The conversation's strongest idea is the measurement laboratory, not Vulkan

**Qualified finding:** the external conversation correctly emphasizes that AI
becomes more useful when the renderer exposes deterministic, machine-readable
evidence. That principle is already central to this repository. Fixed scenarios,
seeds, state dumps, same-state normal/debug captures, motion contact sheets,
CPU/GPU percentiles, allocation checks, source hashes, candidate goldens, and
owner verdicts are implemented earlier than most custom engines provide them.

The useful change is to extend that system as new rendering risks appear, not to
create a second “AI graphics lab” workflow.

| Conversation proposal | Current fit | Proportional project action |
| --- | --- | --- |
| Deterministic cameras, seeds, exposure, and scene state | Largely present; exposure is not yet a material system | Keep one versioned reference route per visible experiment and record all view/exposure state in the existing artifact manifest. |
| Normal/debug frames and short motion capture | Present for current tracers | Add a debug output only when it explains a named pass or regression; retain motion evidence for temporal effects. |
| Depth, normals, motion vectors, shadow masks, history rejection, LOD, and overdraw | Most passes do not exist yet | Add outputs with the pass that owns them. Do not build an empty G-buffer/debug framework in Phase 3. |
| Per-pass GPU time, VRAM, bandwidth, draw/dispatch, and streaming statistics | Current packet has broad CPU/GPU/frame/memory evidence | Add stable debug names and per-pass timestamps when multiple expensive passes or upload queues exist; do not collect metrics with no budget or decision. |
| FLIP image comparison | Not adopted | Evaluate it at the renderer-depth gate as one perceptual signal, with calibrated thresholds, semantic assertions, and human review. FLIP is not a correctness or art-quality oracle. |
| Unseen holdout scenes | Not formalized | Add a small holdout set when visual tuning begins, so improvements must generalize beyond the accepted showcase camera. Keep holdout contents versioned and owner-controlled. |
| Slow offline reference renderer | Not present and not currently justified | Defer until a specific lighting/material calculation needs a higher-quality reference. Small analytic or CPU reference tests may be cheaper and more diagnostic first. |
| RenderDoc/vendor captures | Already placed in Phase 6 | Retain that timing. A Vulkan parity tracer should nevertheless include validation, object names, labels, and a debuggable capture before advanced effects. |

**Inference:** this staged instrumentation will reduce trial-and-error more than
an immediate graphics-API change because it improves the error signal for every
future renderer experiment, including OpenGL experiments.

### 2. Vulkan does not produce better pixels by itself

**Inference, high confidence:** given equivalent geometry, material inputs,
lighting equations, sampling, formats, precision, and post-processing, OpenGL
and Vulkan should produce substantially the same intended image. Differences in
clip/depth conventions, shader compilation, formats, and driver behavior may
create parity bugs, but those are migration variables, not automatic visual
improvements.

Vulkan can enable better visuals indirectly when all of the following are true:

- a desired technique actually requires or is maintained primarily for Vulkan;
- the supported hardware exposes the required features and enough performance;
- the engine implements correct resource lifetime, synchronization, streaming,
  profiling, and fallback behavior; and
- the freed CPU/GPU budget is spent on a visible improvement that survives the
  game's readability and frame-time gates.

**Confirmed fact:** Khronos describes Vulkan as giving the application primary
responsibility for synchronization, cache visibility, and much of execution
ordering. That explicit control is an opportunity and an engineering burden.
A Vulkan renderer with unnecessary waits, broad barriers, descriptor churn, or
poor allocation can be slower and less stable than the current OpenGL renderer.

**Confirmed fact:** the currently available GPUs do not expose the mesh-shader
or hardware-ray-tracing extensions named as the principal advanced benefit.
Vulkan therefore would not unlock those paths on the machine that currently
produces accepted captures.

### 3. Vulkan is a conditional long-term fit, not an automatic upgrade

Vulkan is a credible long-term fit if the approved engine direction later needs
one or more of these concrete capabilities:

- explicit multi-threaded command recording and submission after CPU submission
  becomes a measured bottleneck;
- carefully budgeted transfer/streaming queues and resource residency;
- modern GPU-driven submission using indirect draws, descriptor indexing, and
  buffer device address on a measured target profile;
- a maintained Vulkan integration for a selected vendor-neutral SDK; or
- optional mesh-shader or ray-query/ray-tracing paths on hardware that has been
  inventoried, with a conventional fallback.

**Qualified finding:** Vulkan 1.2 already standardizes useful building blocks
such as descriptor indexing, timeline semaphores, buffer device address, and
draw-indirect count. Vulkan 1.3 adds conveniences including dynamic rendering
and Synchronization2 in core. Khronos recommends checking exact features and
extensions rather than assuming availability from a version number. A project
capability contract or Vulkan Profile is therefore more meaningful than saying
only “we use Vulkan 1.x.”

**Qualified finding:** AMD's current Brixelizer GI sample lists Windows with
DirectX 12 or Vulkan as its requirements. That proves an official Vulkan sample
path exists; it does not prove the SDK fits this renderer, license/dependency
policy, package budget, Linux target, low GPU, art direction, or frame budget.

**Unresolved:** none of mesh shaders, hardware ray tracing, Brixelizer GI,
FidelityFX integration, sparse resources, or asynchronous compute has an
approved current requirement or a measured benefit for the first playable.

### 4. Preserve semantic render inputs, not a lowest-common-denominator GPU API

The advice to make “buffers, textures, pipelines, commands and render passes
API-neutral” is too broad if taken literally.

**Qualified finding:** the durable cross-backend boundary should express what
the game asks the renderer to present:

- a versioned frame/view description;
- immutable camera, dog, sheep, environment, and debug presentation data;
- renderer-owned uploaded mesh/material handles or upload requests;
- capture, diagnostic, and timing requests; and
- explicit resize, device-failure, and shutdown outcomes.

OpenGL and Vulkan internals should remain backend-native. Vulkan descriptors,
pipeline layouts, command buffers, queue ownership, image layouts, frame-flight
resources, and synchronization should not be disguised as generic OpenGL-like
objects. Conversely, OpenGL global state should not dictate the Vulkan model.

**Inference:** a low-level abstraction introduced before a second backend exists
is likely either leaky or over-general. A modest render graph becomes useful
when real passes share transient resources and synchronization becomes hard to
reason about; it should be earned by that dependency graph, not added as a
prerequisite for drawing the current paddock.

### 5. The migration is contained, but it is not a one-prompt rewrite

**Qualified finding:** the CPU/game side can remain intact, and the renderer is
still small enough that migration is materially cheaper now than after terrain
streaming, temporal post-processing, vegetation, and multiple quality profiles.
However, Vulkan parity still requires several coherent outcomes:

| Area | Work needed before parity is credible |
| --- | --- |
| Capability/toolchain contract | Choose actual required versions, extensions, features, limits, formats, shader compiler, loader, validation layers, and supported-device behavior. |
| SDL/platform lifecycle | Create a Vulkan window/surface, instance, physical/logical device, queues, swapchain, resize/minimize handling, presentation, teardown, and actionable diagnostics. SDL3 supplies surface helpers, not a renderer. |
| Frame lifetime and synchronization | Define frames in flight, command pools/buffers, fences/semaphores, image transitions, upload completion, deferred destruction, and device-idle boundaries. |
| Resource allocation | Own buffers/images/views/samplers and staging. Prefer a reviewed allocator such as Vulkan Memory Allocator over inventing production suballocation during parity. |
| Shaders and pipelines | Convert the eleven GLSL sources to a pinned, validated SPIR-V build; define descriptors, push/uniform data, vertex layouts, render state, pipeline caching, and shader failure evidence. |
| Scene passes | Port triangle/cube probes, paddock upload/draw, static shadow pass, dog, sheep, and debug lines without changing authoritative data. |
| Observation | Recreate debug diagnostics, framebuffer capture, timestamps, object names/labels, validation failure policy, and artifact manifests. |
| Parity and cutover | Compare fixed states and motion on supported Windows/Linux devices, explain accepted differences, pass budgets, obtain owner review, then retire one backend or explicitly fund both. |

This is likely multiple reviewable engine outcomes and thousands of lines of
backend/tooling/test code, not a mechanical translation of 327 GL call sites.
That is an engineering-size inference, not a schedule estimate.

### 6. The best timing is after the first-playable signal and before renderer expansion

**Inference, medium-high confidence:** completing Phase 3 before migration is the
best default sequencing.

- Phase 3's remaining work is primarily deterministic gameplay behavior and
  objectives. OpenGL has ample measured headroom for it.
- Finishing that loop reveals whether the game earns deeper renderer investment.
- The renderer is still compact at the Phase 3 exit, so the project has not lost
  the “migrate early” advantage.
- A migration before large procedural terrain, vegetation, temporal effects,
  streaming, or renderer-depth work avoids porting those systems later.

If the owner's primary goal changes from “learn quickly whether Wide Eye works”
to “learn and own a modern Vulkan engine even if the game is delayed,” then an
earlier Vulkan tracer can be rational. That would be an explicit product/learning
reprioritization and ADR change, not evidence that Vulkan improves the current
image.

## Options and tradeoffs

### Option A — Keep OpenGL indefinitely

**Benefits:** no migration delay, simplest current build, retained accepted
evidence, and enough capability for a strong conventional voxel rasterizer.

**Costs:** later Vulkan-only SDKs or cross-vendor mesh/ray paths would require a
larger migration after more renderer code exists. The engine would continue to
depend on OpenGL driver behavior and its less explicit execution model.

**Verdict:** viable if the project remains a conventional stylized raster game;
premature as a permanent decision.

### Option B — Stop Phase 3 and rewrite the renderer in Vulkan now

**Benefits:** the smallest current rendering surface to port and immediate
Vulkan learning.

**Costs:** delays the first complete gameplay loop, invalidates the current
graphics foundation before a replacement is proven, adds tooling that is not
installed, and unlocks none of the named mesh/ray features on the available
GPUs. A “rewrite” framing also invites avoidable changes to working game code.

**Verdict:** rejected under the accepted product-first workflow. Consider only
if the owner explicitly changes the primary objective to Vulkan engine learning.

### Option C — Phase 3 exit decision, then a temporary parallel parity tracer

**Benefits:** obtains the gameplay signal first, migrates while the renderer is
still modest, preserves OpenGL as a known-good oracle, and makes cutover
evidence-based. It also lets the project choose a capability profile from actual
target hardware rather than aspiration.

**Costs:** temporarily maintains two backends and requires careful parity
criteria. It still delays the next content/presentation tracer if approved.

**Verdict:** recommended.

### Option D — Maintain OpenGL and Vulkan permanently

**Benefits:** wider fallback and an ongoing cross-API oracle.

**Costs:** every renderer feature, shader, diagnostic, performance change,
platform runner, and visual acceptance path gains a second implementation and
test matrix. For one developer and agents, this can consume the time intended
for the game.

**Verdict:** reject by default. Parallel backends should be a migration harness,
not a permanent promise, unless measured platform reach justifies the ownership
cost.

## Recommendation

### Decision

1. Continue the first unchecked Phase 3 gameplay item on OpenGL.
2. Preserve the current high-level render-state boundary; do not introduce a
   generic `Buffer`/`Texture`/`Command` abstraction or render graph solely in
   anticipation of Vulkan.
3. Add AI-readable graphics evidence only with the feature that needs it. The
   highest-value near-term additions are named debug outputs for behavior and
   stable motion evidence, not advanced GPU infrastructure.
4. At the Phase 3 exit gate, decide whether Vulkan serves an approved engine
   destination and whether that value exceeds the delay to Phase 4.
5. If approved, plan a bounded parity tracer before material renderer growth.
   Keep OpenGL frozen as the reference until Vulkan passes functional, visual,
   diagnostic, performance, and platform gates; then choose one primary backend.
6. Defer GPU-driven submission, async compute, mesh shaders, ray queries, ray
   tracing, Brixelizer GI, and other advanced effects until conventional Vulkan
   parity exists and a named experiment or bottleneck justifies each one.

### Baseline rules for an eventual Vulkan tracer

- **Do not declare Vulkan 1.3 or 1.4 as the baseline yet.** The accepted UHD 630
  proxy exposes only Vulkan 1.2, while the named Low/High targets are unmeasured.
- Define an engine capability contract from exact features, extensions, limits,
  formats, queues, and presentation support. Consider a versioned Vulkan Profile
  only after the target inventory exists.
- Use SDL3's Vulkan surface and required-instance-extension APIs, but keep device,
  swapchain, resource, synchronization, and rendering ownership in project code.
- Enable `VK_LAYER_KHRONOS_validation`, synchronization validation, and useful
  best-practice/GPU-assisted modes in diagnostic builds where supported. Treat
  validation availability and clean output as gates, not optional polish.
- Compile shaders to SPIR-V with a pinned, reproducible compiler and validate the
  binaries. Preserve shared lighting/material mathematics where practical, but
  permit small backend entry-point/layout differences rather than hiding them.
- Use debug object names and command labels from the first meaningful frame so
  captures remain agent- and human-readable.
- Start with one graphics/present path and conservative synchronization. Add
  transfer queues, asynchronous compute, bindless layouts, or complex scheduling
  only after a measurement proves their value.
- Review a maintained allocator such as AMD's Vulkan Memory Allocator instead of
  spending the parity phase on custom suballocation. Pinning, license,
  provenance, package impact, and failure behavior still require the normal
  dependency gate.
- Compare semantic state, geometry counts, depth/shadow behavior, and human-visible
  output. Do not require unexplained byte-identical OpenGL/Vulkan pixels.

## Failure modes and gotchas

- **Capability by name:** “Vulkan 1.4” does not imply mesh shaders or ray
  tracing. Query and enable every required feature; test the fallback.
- **Visual-upgrade fallacy:** a successful Vulkan triangle or paddock can look
  identical while consuming substantial time. Call parity a foundation result,
  not a quality improvement.
- **Leaky abstraction:** generic API-neutral buffers/pipelines often expose
  Vulkan complexity poorly while making OpenGL awkward. Keep the common layer
  semantic and high-level.
- **Permanent dual-backend tax:** a temporary oracle can quietly become two
  renderers that every future feature must maintain. Set an explicit cutover or
  cancellation gate.
- **Validation blind spot:** the current Windows enumeration did not find the
  Khronos validation layer. Development without validation would make an
  already explicit API much harder to debug.
- **Synchronization bugs:** missing availability/visibility dependencies,
  incorrect image layouts, premature destruction, and frame-flight reuse can
  render correctly on one driver and fail on another.
- **Over-synchronization:** `vkDeviceWaitIdle`, broad barriers, and serialized
  uploads can make Vulkan slower than OpenGL while appearing correct.
- **Swapchain lifecycle:** resize, minimize, surface loss, out-of-date/suboptimal
  presentation, device loss, and shutdown must be deterministic and tested, not
  left behind the happy-path reference frame.
- **Coordinate and format mismatches:** Vulkan's default clip-depth range is
  `[0,1]` rather than OpenGL's `[-1,1]`; Y orientation, front-face winding,
  framebuffer readback orientation, sRGB conversion, shadow comparison, depth
  precision, and clear/load/store behavior can all create false “shader parity”
  failures.
- **Shader-interface drift:** Vulkan consumes SPIR-V and requires explicit
  descriptor and memory-layout contracts. Current runtime-compiled OpenGL GLSL
  cannot simply be passed unchanged to `vkCreateShaderModule`.
- **Golden misuse:** driver/API differences make raw cross-backend image identity
  brittle. FLIP or thresholded differences still need semantic masks and human
  review; they do not certify art direction or temporal stability.
- **Benchmark overfit:** one accepted camera can hide pop, shimmer, upload
  stalls, or poor culling. Use fixed representative routes plus small holdout
  scenes before accepting an optimization.
- **Advanced-feature inversion:** building meshlets, ray tracing, GI, or async
  compute before conventional parity makes debugging ownership and performance
  much harder. Every advanced path needs a fallback, debug view, benchmark
  scene, GPU-time/memory budget, and known limitations.
- **Target mismatch:** the currently available GPUs do not exercise the desired
  advanced extensions, while the named Low/High targets remain unmeasured.
  Emulator, loader, or extension-database claims cannot replace target runs.

## Evidence and confidence

| Claim | Evidence class | Confidence |
| --- | --- | --- |
| Gameplay/simulation can survive a renderer migration without rewrite | Direct source-boundary audit | High |
| OpenGL coupling is concentrated but includes platform, build, tests, artifacts, and docs | Direct repository audit and call-site inventory | High |
| No current frame/GPU budget requires Vulkan | Accepted native measurement packet recorded in `ROADMAP.md` | High for the measured UHD 630 proxy; not evidence for target devices |
| The available Intel device is Vulkan 1.2 and neither available GPU reports mesh/ray extensions | Local `vulkaninfo.exe` capability observation on 2026-08-16 | High for the observed drivers; not general hardware-family support |
| Vulkan does not automatically improve image quality | Rendering-semantics inference | High |
| Vulkan can provide better control and access to modern extension/SDK paths | Khronos specifications/guides and AMD SDK documentation | High for capability; benefit remains workload-dependent |
| Phase 3 exit is the best default migration decision point | Project-priority and change-cost inference | Medium-high |
| A Vulkan parity backend will require multiple outcomes and substantial new code | Current renderer/platform audit plus explicit Vulkan lifecycle requirements | Medium-high; intentionally no hour estimate |
| FLIP and holdout scenes will reduce visual overfitting | Tool capability plus project-workflow inference | Medium; thresholds and scene selection need experiments |
| Brixelizer GI is suitable for Wide Eye | Only an official Vulkan sample path is confirmed | Low until target, license, integration, quality, and performance trials exist |
| AI can deliver advanced rendering with little human review | External conversation claim, not reproduced here | Low; the accepted workflow requires measurement and owner judgment |

## Planning handoff

This research is architecture-ready for planning only after the owner answers or
explicitly defers these choices:

1. Is Vulkan a means to ship Wide Eye, or is learning/building a Vulkan engine
   itself a primary goal worth delaying game evidence?
2. Must the currently accepted Intel UHD 630 proxy continue to run the primary
   backend? A Vulkan 1.3-only baseline would exclude its observed driver.
3. Are mesh shaders and hardware ray tracing optional High-profile experiments
   with conventional fallbacks, or future minimum requirements? The current
   available GPUs support neither path.
4. Is OpenGL allowed to be retired after parity, or does a supported-platform
   requirement justify permanent dual-backend maintenance?

If planning is approved, the plan should:

- start with a native Windows/Linux target capability inventory and an ADR-ready
  baseline decision;
- preserve `HandcraftedPaddockFrame`, camera/animal presentation data,
  `ChunkMesh`, replay/state/scenario versions, and authoritative simulation;
- extract only the platform/render seams needed by the first Vulkan reference
  scene;
- port triangle/cube probes and one fixed paddock before shadows or animals;
- add validation, debug names, capture, and timestamps as acceptance
  requirements rather than cleanup;
- port paddock, shadow, dog, sheep, debug, capture, and performance paths in
  bounded increments;
- compare fixed state, semantic outputs, images, motion, and budgets after every
  increment;
- require native Windows and native Linux evidence on named devices before
  cutover;
- define cancellation and OpenGL-retirement gates; and
- adjust ADR 0001, setup/build documentation, backend-neutral validation
  language, artifact manifests, and `ROADMAP.md` only after owner approval and
  observed implementation results.

A suitable next-chat instruction is:

> Use `$plan-from-research` on
> `docs/research/opengl-to-vulkan-feasibility.md`. Challenge the recommendation
> and architecture readiness. If the owner has approved the timing and platform
> choices, produce an incremental Vulkan parity plan that preserves OpenGL as a
> temporary reference and does not rewrite gameplay, simulation, procedural
> world, replay, or authoritative state. Do not implement the plan.

## References

All web sources were accessed 2026-08-16.

### Project and ideation inputs

- [`ROADMAP.md`](../../ROADMAP.md), current checkpoint and Phase 6 renderer-depth
  gate.
- [ADR 0001: Native engine foundation](../decisions/0001-native-foundation.md).
- [Accepted development workflow](../DEVELOPMENT_WORKFLOW.md).
- [Engine boundary](../VOXEL_ENGINE_OPTION.md#architecture-boundary).
- [`OpenGlRenderer` interface](../../src/render/opengl_renderer.hpp) and
  [implementation](../../src/render/opengl_renderer.cpp).
- [SDL/window runtime](../../src/platform/window_runtime.cpp).
- [External GPT conversation: AI rendering-engine
  feasibility](../../ref/AI_Rendering_Engine_Feasibility.md), treated as
  non-authoritative ideation.

### Primary external sources

- Khronos, [Vulkan Versions and Porting
  Guide](https://docs.vulkan.org/guide/latest/versions.html).
- Khronos, [Vulkan
  Profiles](https://docs.vulkan.org/guide/latest/vulkan_profiles.html).
- Khronos, [Synchronization and Cache
  Control](https://docs.vulkan.org/spec/latest/chapters/synchronization.html).
- Khronos, [`VK_EXT_mesh_shader`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_mesh_shader.html).
- Khronos, [`VK_KHR_ray_tracing_pipeline`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_ray_tracing_pipeline.html).
- Khronos, [What is
  SPIR-V](https://docs.vulkan.org/guide/latest/what_is_spirv.html).
- Khronos, [Depth and OpenGL porting
  conventions](https://docs.vulkan.org/guide/latest/depth.html).
- Khronos, [Vulkan profiling
  guidance](https://docs.vulkan.org/guide/latest/profiling.html).
- Khronos, [validation feature definitions](https://docs.vulkan.org/spec/latest/chapters/initialization.html).
- SDL, [`SDL_Vulkan_CreateSurface`](https://wiki.libsdl.org/SDL3/SDL_Vulkan_CreateSurface)
  and [`SDL_Vulkan_GetInstanceExtensions`](https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions).
- AMD GPUOpen, [FidelityFX Brixelizer GI
  sample](https://gpuopen.com/manuals/fidelityfx_sdk/samples/brixelizer-gi/).
- AMD GPUOpen, [Vulkan Memory
  Allocator](https://gpuopen.com/vulkan-memory-allocator/).
- NVIDIA Research, [FLIP image-difference
  tool](https://github.com/NVlabs/flip).

## Recommended next step

Continue the current `ROADMAP.md` item—close-range sheep/sheep repulsion—on the
accepted OpenGL backend. Do not start a Vulkan rewrite or change ADR 0001 from
this draft research. At the Phase 3 exit gate, decide the four planning questions
above. If Vulkan remains an approved destination, run `$plan-from-research` on
this file before any backend implementation or roadmap expansion.
