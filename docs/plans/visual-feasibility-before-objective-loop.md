# Plan: Visual feasibility before the objective loop

**Status:** Approved implementation plan; not implemented

**Date:** 2026-08-22

**Owner approval:** 2026-08-22

**Owner decision, 2026-08-22 — reference machine superseded:** the owner
confirmed that the machine this repository is checked out on *is* the reference
desktop, and that the earlier RTX 4070 Ti report is superseded by the observed
RTX 5070 Ti host recorded under
[Verified current state](#verified-current-state). This amendment retargets the
reference-GPU identity and the two hard gate constants
([`run-visual-feasibility-baseline.ps1`](../../tools/phase3/run-visual-feasibility-baseline.ps1)
and
[`assert-visual-feasibility-baseline-manifest.cmake`](../../tests/assert-visual-feasibility-baseline-manifest.cmake)).
It changes no budget, threshold, tolerance, scene, profile, camera, viewport, or
tick. **Inference:** the RTX 5070 Ti is a different and newer GPU than the one
this plan originally assumed, so every provisional budget and headroom
expectation that was reasoned about a 4070 Ti is now reasoned about untested
hardware until the Phase 0 baseline actually runs.

**Source research:**
[Visual feasibility before the objective loop](../research/visual-feasibility-before-objective-loop.md)

**Architecture readiness:** Localized prerequisite approved — the accepted
OpenGL renderer, immutable presentation snapshot, deterministic scenario,
capture, and timing boundaries are suitable. Before visual work, the fixed
paddock camera/light/shadow assumptions and hard-coded review configuration need
a bounded visual-scene and evidence seam. No renderer-backend migration or new
third-party dependency is required.

## Objective and success criteria

Determine whether one representative Wide Eye scene can reach the
owner-selected visual direction before more work is invested in the objective,
HUD, or farmer cue. The experiment uses the current OpenGL 4.6 backend, five
authoritative sheep, one bounded code-generated landscape composition, one
repeatable gameplay-camera route, and one holdout view.

The experiment succeeds only when all of the following are true:

- the owner judges the candidate credible enough to continue against the two
  primary generated sheep-game images and the documented secondary cues;
- voxel-informed geometry reads as sufficiently detailed rather than
  conspicuously chunky or aggressively low-poly;
- landscape layering, vegetation density, warm directional light, stable
  shadows, atmospheric depth, controlled softness, and animal silhouettes are
  visible in both the representative and holdout evidence;
- the active dog, five sheep, route, gates, and important terrain edges remain
  readable in motion, including when depth of field or another softening
  technique is enabled;
- the same scenario, seed, tick, camera route, viewport, exposure, and profile
  reproduce comparable captures and motion evidence;
- the reference Windows PC reports the observed RTX 5070 Ti as the active
  OpenGL adapter and meets the selected 60 Hz frame, memory, and startup budgets
  at the Phase 0 reference resolution;
- no high-severity OpenGL diagnostics, unexplained temporal instability, new
  steady-state allocation, or gameplay-state/replay change is introduced; and
- the five-sheep verdict is labeled as visual-direction evidence, not proof of
  the flock density shown in the references. A positive verdict unlocks a new
  25-sheep scale/behavior plan; it does not silently approve 25 or 100 sheep.

Failure is also a valid result. If the bounded scene cannot approach the target
without violating readability, the procedural-first rule, or the selected
reference-machine budget, stop and decide whether to simplify the visual target,
change the engine/asset strategy, or shelve the game. Do not hide that decision
by starting the objective loop or by indefinitely adding effects.

## Scope and non-goals

### In scope

- A deterministic five-sheep visual-tracer scene built from project-owned code.
- A bounded expansion of the visible landscape composition, including
  unreachable distant scenery where its non-authoritative role is explicit.
- Camera projection and render settings appropriate to a larger vista.
- Directional-light and shadow coverage/stability improvements.
- A deliberate linear/sRGB, exposure, palette, and material-response contract.
- Dense but bounded code-generated vegetation and environmental detail.
- Atmospheric depth, beginning with the simplest technique that can answer the
  visual question.
- More detailed procedural dog/sheep presentation while gameplay collision and
  authoritative state remain unchanged.
- Stable anti-aliasing/filtering and selective focal softness/depth of field,
  evaluated as separate effects.
- Feature-owned debug views, same-state captures, motion evidence, per-pass
  timings where useful, and an explicit owner visual verdict.
- RTX 5070 Ti reference evidence and optional GTX 1050 Ti Max-Q very-low-spec
  degradation evidence under clearly different graphics profiles.

### Non-goals

- The farmer, whistle, objective, success/failure, HUD, onboarding, scoring, or
  campaign.
- Vulkan, a permanent dual backend, ray tracing, mesh shaders, DLSS, FSR, or a
  vendor-specific rendering SDK.
- An infinite procedural world, general chunk streaming, production LOD,
  background generation, weather, day/night, water, or multiple biomes.
- Final shipping animal art, a general asset importer, Blender integration, or
  third-party art. The reference files are not runtime assets.
- Claiming that five sheep establish the reference flock density.
- Authoritative 25/100-sheep player state, replay/format expansion, or hard
  sheep-to-sheep collision in this plan.
- Selecting final minimum/recommended PC specifications before representative
  measurements exist.
- Replacing accepted Phase 0–3 baselines or promoting a candidate visual golden
  without the owner's explicit acceptance.

## Verified current state

- The project is pinned to C++23, SDL 3.4.10, glad 2.0.8, OpenGL 4.6 Core, and
  GLSL 4.60. OpenGL calls remain concentrated in the platform/runtime and
  `OpenGlRenderer` paths.
- `HandcraftedPaddockFrame` carries camera, optional dog, and exactly five
  renderer-facing sheep poses. It does not own gameplay truth.
- `HandcraftedPaddock` is a fixed two-by-two-chunk, 32-by-32 world with seven
  palette entries and a tested merged opaque mesh. Cutout and translucent passes
  exist as empty mesh categories, not implemented visual systems.
- The renderer draws directly to the default framebuffer with embedded shaders.
  Camera near/far/focal values, directional light, sky, fog distances, and the
  light projection are repeated or hard-coded in the scene shaders.
- The single 1024-by-1024 depth shadow map is centred on `(16, 4, 16)` with
  fixed extents. It is not suitable evidence for an expanded vista without a
  new coverage/stability decision.
- Current distance fog is a simple colour blend between 36 and 70 world units.
  It is not volumetric lighting, height fog, or a temporal participating-media
  solution.
- Sheep are submitted individually from a fixed five-pose buffer. The current
  procedural proxy and snapshot mapping have focused identity, interpolation,
  and zero-allocation evidence.
- Existing native Windows tooling captures deterministic normal/debug frames,
  state, source hashes, OpenGL diagnostics, memory, GPU time, CPU submission,
  preparation cost, and synchronized frame percentiles. The Phase 3 runner is
  hard-coded around 1920×1080 and the Tracer 2 Low budget, so it cannot simply
  be relabeled as the new reference packet.
- The current WSL host exposes only OpenGL 4.5 and can run headless/build suites,
  but cannot provide accepted visual evidence for this OpenGL 4.6 tracer.
- **Reference desktop — observed result, 2026-08-22, this repository's
  checked-out native Windows host.** Method: Windows CIM inventory queries plus
  the Phase 0 context smoke
  ([`tools/phase0/run-context-smoke.ps1`](../../tools/phase0/run-context-smoke.ps1)),
  ignored evidence log
  `artifacts/phase0/2026-08-22/windows-context-smoke-155925531.log`.
  - CPU: AMD Ryzen 9 9950X 16-Core Processor, 16 cores / 32 threads.
  - RAM: 61.6 GiB reported by `Win32_ComputerSystem.TotalPhysicalMemory`.
  - Board: ASRock X870 Pro RS WiFi.
  - OS: Microsoft Windows 11 Home, version `10.0.26200`, build `26200`, 64-bit.
  - Display adapters reported by Windows: NVIDIA GeForce RTX 5070 Ti, driver
    `32.0.15.9186`; AMD Radeon(TM) Graphics (Ryzen integrated), driver
    `32.0.21036.18`.
  - Primary display: 2560×1440; reported supported modes include
    2560×1440@144 Hz, 1920×1080@60 Hz, and 1024×768@60 Hz.
  - Active OpenGL: `gl_vendor=NVIDIA Corporation`,
    `gl_renderer=NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2`,
    `gl_version=4.6.0 NVIDIA 591.86`, `glsl_version=4.60 NVIDIA`,
    `core_profile=yes`, `debug_context=yes`, SDL 3.4.10,
    `video_driver=windows`, `result=pass`.
  - Toolchain: MSVC `19.44.35228.0` from Visual Studio 2022 Build Tools (MSVC
    toolset `14.44.35207`), bundled CMake `3.31.6-msvc6`, bundled Ninja
    `1.12.1`.
  - The native Windows `dev` preset configured, built, and passed 45/45 CTests,
    including every display-backed OpenGL test from `opengl_context_smoke`
    through `opengl_debug_high_severity` and `opengl_influence_debug_overlay`.
- This reference desktop is a **hybrid two-adapter machine** (discrete NVIDIA
  plus Ryzen-integrated AMD), so the runner's active-renderer pin still does
  real work: Windows can hand the process the integrated adapter, and inventory
  alone never proves which GPU rendered.
- **Inference:** the observed RTX 5070 Ti is a different and newer GPU than the
  RTX 4070 Ti this plan originally assumed. Every provisional budget and
  headroom expectation below was reasoned about a 4070 Ti and is therefore now
  reasoned about untested hardware until the Phase 0 baseline actually runs on
  this host.
- The laptop's GTX 1050 Ti Max-Q is a very-low-spec proxy, not the target visual
  machine or an accepted shipping minimum.
- ADR 0001 still contains provisional Iris Xe 1080p/60 Low and RX 6600
  1440p/60 High profiles. This plan does not silently supersede that decision.
- The reference hierarchy and rights boundary are recorded in
  [`HERDING_GAMEPLAY.md`](../game-design/HERDING_GAMEPLAY.md#camera-and-visual-reference-boundary).
  `real-photo-sheep3.jpg` informs spatial distribution only and is not a runtime
  asset or an animal-appearance reference.

## Adversarial review result

| Finding | Classification | Planning consequence |
| --- | --- | --- |
| A five-sheep scene can prove the complete flock-density target | **Rejected** | Gate 1 judges art language, composition, individual readability, and atmosphere. Density remains a conditional 25-sheep question. |
| The reference look requires Vulkan | **Rejected** | Keep OpenGL and measure actual passes. Revisit the backend only for a demonstrated capability or bottleneck. |
| A larger mesh alone creates landscape scale | **Rejected** | Projection range, shadow coverage, fog, camera composition, detail distribution, collision boundaries, and capture state must agree. |
| The traversable world must expand with the visible vista | **Qualified** | Near-field traversable geometry must agree with authoritative collision. Distant scenery may be non-authoritative only when unreachable and explicitly labeled. Stop if the tracer requires a larger playable world. |
| “Volumetric” should be implemented before simpler atmosphere | **Rejected** | Try a bounded height/distance/directional-scattering solution first. Add volumetric integration only if named reference gaps remain. |
| Depth of field is equivalent to the desired softness | **Rejected** | Audit anti-aliasing, filtering, atmosphere, exposure, and focal separation independently. DOF is optional and fails if gameplay cues blur. |
| The RTX 5070 Ti justifies ray tracing or vendor features | **Rejected** | The experiment asks whether the visual target works, not whether the newest GPU path can be exercised. Preserve cross-vendor OpenGL. |
| The current capture runner can serve unchanged | **Rejected** | Add only the configuration needed for a named scene, reference/holdout views, viewport/profile, and current budgets; do not build a generic automation platform. |
| A broad render graph is a prerequisite | **Rejected** | Add explicit pass/resource ownership only as each accepted effect requires it. |
| Finer geometry requires imported art | **Rejected as a prerequisite** | First test code-generated, moderately detailed forms. Stop for an asset-policy decision only if that route cannot approach the target. |
| The existing five-sheep presentation contract blocks Gate 1 | **Rejected** | It is exactly the correct fixed scope for the first visual gate. It blocks 25/100 and therefore protects the gate boundary. |
| Hard sheep collision must precede the visual gate | **Qualified but deferred** | Five-sheep captures must avoid misleading overlaps. A later density gate must explicitly decide acceptable spacing/contact behavior. |

The adversarial pass narrows rather than overturns the research recommendation.
The architecture is ready after localized scene/evidence configuration. The
desktop CPU, RAM, display modes, driver, and active OpenGL renderer are now
observed (2026-08-22, above), so they are no longer an open planning unknown.
Observing them still does not accept a reference-machine baseline: that requires
the Phase 0 packet produced by the runner on this host, not an inventory
listing.

## Decisions and assumptions

### Accepted decisions

- OpenGL remains the only renderer backend for this tracer.
- The two generated sheep-game images are the primary idealized target.
- Numbered visual references are secondary quality cues only.
- The visual language is voxel-informed, not deliberately coarse. Geometry
  density and surface detail are owner-reviewed iteration variables.
- Controlled softness includes atmosphere, filtering, and selective focal
  separation; it does not mean globally blurred gameplay.
- The first visual gate contains five authoritative sheep. Only an explicit
  positive owner verdict may unlock planning for 25 and then 100.
- The RTX 5070 Ti desktop is the intended reference visual GPU. Its complete
  machine inventory and active OpenGL adapter were observed on 2026-08-22; the
  reference role still requires the Phase 0 packet, not the inventory alone.
- The GTX 1050 Ti Max-Q laptop is optional very-low-spec compatibility evidence.
- Runtime visual content remains code-generated or provenance-approved under the
  existing procedural-first policy.

### Provisional assumptions to resolve in Phase 0

- Use ADR 0001's existing High display target, 2560×1440 at 60 Hz, as the
  provisional reference capture configuration only if the desktop/display
  supports it. **Observed result, 2026-08-22:** the reference desktop's primary
  display runs 2560×1440 and reports 2560×1440@144 Hz among its supported modes,
  so the provisional target is reachable on this host. The Phase 0 packet must
  still record the actual selected viewport; this observation does not itself
  select one, and no viewport default changes here.
- Use the existing High p95/p99 frame, memory, startup, and package budgets until
  an explicit decision retains or supersedes them. Those budgets were reasoned
  about ADR 0001's RX 6600 High class and, in this plan, about a 4070 Ti that is
  not the observed host; nothing about them is measured on the RTX 5070 Ti yet.
  Passing on an RTX 5070 Ti does not establish a minimum specification.
- The closer third-person hillside composition is the representative view and a
  short route around it supplies motion evidence. The elevated countryside view
  is the first holdout composition and must not be tuned frame-by-frame alongside
  the representative view.
- Distant landscape geometry is presentation-only and unreachable. The existing
  near-field gameplay/collision boundary stays authoritative and visually
  consistent.
- No temporal technique is assumed. A stable non-temporal result is preferable
  to softer output with ghosting, shimmer, or history artifacts.

### Stop-and-ask decisions

Stop before implementation scope expands if any outcome would require:

- changing OpenGL 4.6, SDL3, the native platform matrix, or the procedural-first
  asset rule;
- making the distant visual landscape traversable or expanding authoritative
  collision/world generation;
- importing or redistributing reference imagery, models, textures, audio, or
  another third-party asset/dependency;
- changing authoritative sheep count, snapshot/replay/state-format ownership,
  fixed 60 Hz simulation, or accepted behavior tuning;
- accepting a vendor-specific upscaler, ray-tracing path, or permanent graphics
  profile promise;
- promoting/replacing an accepted visual golden; or
- revising shipping minimum/recommended specifications before measured evidence
  and an explicit owner decision.

## Prerequisites

1. **Reference-machine observation:** on the desktop, record Windows version,
   CPU, RAM, RTX 5070 Ti identity, driver, active OpenGL renderer/version, display
   modes, and chosen viewport. Do not infer the active GPU from inventory
   alone — this host also exposes a Ryzen-integrated AMD adapter. The
   2026-08-22 observation above satisfies the inventory and active-renderer
   half; the Phase 0 packet must still carry the same fields with the chosen
   viewport.
2. **Baseline packet:** build the unchanged Release tree on that machine and
   capture the current five-sheep scene, motion, diagnostics, timings, memory,
   and source hashes at the selected viewport. This is comparison evidence, not
   a candidate visual baseline.
3. **Visual rubric:** turn the accepted reference roles into a compact owner
   review sheet covering composition/scale, geometry detail, vegetation density,
   palette/material response, lighting, shadow stability, atmosphere, animal
   silhouette, focal readability, motion, and reference/holdout agreement.
4. **Localized scene/evidence seam:** support one named visual-tracer scene,
   representative and holdout camera records, explicit viewport/profile, and
   selected render settings without moving gameplay truth into `render` or
   creating a generic scene framework.

No dependency installation, Vulkan SDK, model importer, texture pipeline,
render graph, streaming system, or external asset is a prerequisite.

## Implementation phases

### Phase 0 — Lock hardware, rubric, and unchanged baseline

**Outcome:** The reference machine, viewport, budget, deterministic scene state,
representative route, holdout camera, and visual rubric are recorded before any
candidate rendering change.

**Likely files/components:** native Windows review tooling under `tools/phase3`
or a narrowly named sibling, artifact manifests/review packet, scenario/camera
configuration, and documentation owned by the resulting decision.

**Dependency direction:** tooling invokes the shipping executable and records
observations; it does not become a second renderer or gameplay owner.

**Checkable tasks:**

- Record the desktop CPU, RAM, GPU/driver, active OpenGL renderer/version, and
  display modes.
- Select and record the reference viewport; prefer the provisional 2560×1440/60
  target only when the display supports it.
- Choose one deterministic existing tick/state from the five-sheep scenario and
  define a short repeatable camera/gameplay path without changing simulation.
- Freeze one representative camera/route and one holdout camera derived from the
  two primary reference roles.
- Capture unchanged normal, matching debug, and short motion evidence plus total
  timing/memory/OpenGL diagnostics.
- Write the owner-facing rubric without scoring unimplemented features as
  failures in the unchanged baseline.

**Validation:** native Release build and complete CTest suite, OpenGL 4.6 context
and high-severity diagnostic checks, repeat capture identity where currently
supported, artifact-manifest checks, and human confirmation that the chosen
views ask the intended visual question.

**Evidence artifact:**
`artifacts/phase3/<date>/visual-feasibility-baseline/` containing inventory,
configuration, commands, hashes, captures, motion, metrics, diagnostics, rubric,
and review metadata.

**Stopping condition:** stop if the RTX 5070 Ti is not the active OpenGL renderer,
the selected viewport cannot be reproduced, or the baseline state/cameras do not
expose both close readability and landscape-depth questions.

### Phase 1 — Establish the bounded visual scene and render settings

**Outcome:** One named code-generated visual-tracer scene shows a larger layered
vista while preserving the current near-field authoritative paddock, dog, and
five sheep. Camera, projection, light, fog, and shadow settings become explicit
scene inputs instead of shader-local paddock constants.

**Likely files/components:** `src/voxel` scene construction and tests,
`src/render/opengl_renderer.*`, renderer-facing frame/configuration values,
`src/platform/scenario_runner.*`, bounded capture CLI/tooling, CMake test
registration, and focused scene/render tests.

**Dependency direction:** `voxel` or a scene-construction owner produces checked
mesh/material inputs; `game` owns authoritative near-field state; `platform`
selects the named scenario; `render` consumes immutable scene/frame settings.
No OpenGL type crosses into scene or game code.

**Checkable tasks:**

- Add one bounded visual scene with deterministic geometry/material counts and
  explicit near-field versus unreachable distant-scenery bounds.
- Preserve exact current collision and sheep behavior; prevent the camera/dog
  route from presenting distant scenery as traversable.
- Replace repeated shader-local camera range, light direction/colour, sky/fog,
  and light-projection constants with validated renderer-facing settings only as
  required by this scene.
- Extend capture/performance selection for the named scene, two camera roles,
  chosen viewport, and a reference profile without weakening existing runners.
- Retain current paddock and accepted-golden scenarios unchanged.

**Validation:** deterministic scene counts/bounds, invalid-setting rejection,
existing mesh/collision/scenario tests, same-state representative/holdout
captures, current accepted-baseline checks, and no new high-severity OpenGL
messages.

**Evidence artifact:** a scene-contract packet containing geometry/material
counts, near/distant bounds, camera/render settings, normal/debug captures, and
baseline timing deltas.

**Stopping condition:** stop if visual scale requires expanding authoritative
collision/world generation, if distant geometry creates misleading traversable
surfaces, or if a generic scene/renderer abstraction is proposed without a
second demonstrated use.

### Phase 2 — Prove shadow coverage and stability

**Outcome:** The representative route and holdout vista retain readable,
spatially stable directional shadows without the current 32-by-32 hard-coded
projection.

**Likely files/components:** OpenGL shadow resources/pass, light-space settings,
feature-owned shadow debug output, capture configuration, and focused render
tests.

**Dependency direction:** renderer owns shadow resources and light-space math;
scene configuration supplies semantic coverage inputs, not framebuffer objects.

**Checkable tasks:**

- Establish an objective shadow question: near dog/sheep contact quality,
  mid-ground structure, distant coverage, and stability along the route.
- Try the smallest camera/scene-fitted directional solution first with a named
  resolution and texel-stability policy.
- Add a shadow-mask or light-space coverage debug output and GPU pass label/time.
- Compare stationary frames and a motion/contact sheet for acne, detachment,
  peter-panning, shimmer, abrupt coverage loss, and off-frustum behavior.
- If one fitted map cannot satisfy both near and holdout evidence, stop and
  approve a separate bounded cascade outcome rather than silently adding it.

**Validation:** shadow framebuffer completeness, settings/bounds tests, current
no-shadow/debug oracles where applicable, representative and holdout
normal/debug/motion evidence, GPU timing, and OpenGL diagnostics.

**Evidence artifact:** matched shadow-on/debug/motion packet with light/camera
settings, map extent, pass timing, and named observed defects.

**Stopping condition:** do not proceed while shadows disappear outside the old
paddock, visibly swim during the route, or consume an unexamined share of the
frame budget.

### Phase 3 — Establish colour, exposure, lighting, and material response

**Outcome:** The scene has a deliberate colour-space and exposure contract,
warm natural directional light, readable shaded values, and a bounded material
palette that approaches the references without relying on accidental framebuffer
gamma or copied textures.

**Likely files/components:** OpenGL colour target/framebuffer setup if required,
scene/material settings, paddock palette/mesh inputs, renderer shaders,
feature-owned normal/albedo/lighting diagnostics, and capture metadata.

**Dependency direction:** scene/material code owns semantic palette values;
renderer owns colour-space conversion, exposure, and lighting calculations.

**Checkable tasks:**

- Observe and document the current default-framebuffer sRGB behavior before
  changing colour or grading.
- Define linear versus display-space values and apply one explicit output
  conversion path.
- Tune bounded sun/sky/fill/material response against the representative view,
  then inspect the untouched holdout.
- Add only the colour/depth targets required by an accepted effect; do not build
  an unused deferred renderer.
- Preserve debug colours as diagnostic values or document their display-space
  conversion so causal overlays remain legible.
- Attempt a global-illumination approximation only if the accepted direct
  lighting leaves a **named** reference gap that only indirect light closes, and
  a separate coherent outcome is approved. GI is accepted as a later direction on
  this backend and is scheduled at Phase 6 renderer depth, not here; see
  [ADR 0011](../decisions/0011-global-illumination-on-the-existing-opengl-backend.md).
  This clause is the same conditional the atmosphere phase applies to
  volumetrics, and it adds nothing to this phase by default.

**Validation:** colour-space/configuration tests where deterministic, normal and
albedo/lighting debug captures, representative/holdout comparisons, motion for
banding/flicker, GPU timing, and current semantic framebuffer oracles updated
only for intentional output changes.

**Evidence artifact:** colour/lighting packet with settings, diagnostics,
same-state before/after/holdout frames, motion, and owner notes.

**Stopping condition:** stop if the intended warmth comes only from crushing
values, if sheep merge into terrain, if debug evidence becomes ambiguous, or if
the output differs because colour space is unknown.

### Phase 4 — Add bounded fine environment detail and vegetation

**Outcome:** The scene gains deterministic near/mid/far environmental density
and finer voxel-scale detail without importing assets, requiring streaming, or
obscuring the flock.

**Likely files/components:** code-generated visual-scene data/meshes, material
passes already justified by the scene, renderer upload/draw paths, density or
overdraw diagnostics, and deterministic scene tests.

**Dependency direction:** scene construction owns placement seeds/grammar and
validity; renderer owns resources and draw submission; gameplay reads none of
the decorative state.

**Checkable tasks:**

- Add one detail family at a time: terrain variation, ground cover, then bounded
  shrubs/trees or distant silhouettes as the rubric requires.
- Record exact generation parameters, counts, bounds, and rejection rules.
- Use finer geometry where silhouette or parallax matters; do not raise density
  uniformly or use “voxel resolution” as a quality metric by itself.
- Keep route, gates, dog, and sheep readable from both views; add a density,
  overdraw, or layer debug output only where it answers a measured problem.
- Measure upload, draw/dispatch, GPU, memory, and package effects before adding
  the next family.

**Validation:** fixed-seed/count/bounds tests, no invalid near-field placement,
normal/debug/holdout captures, camera-route popping/shimmer review, allocations,
GPU/CPU timing, memory, package delta, and OpenGL diagnostics.

**Evidence artifact:** per-detail-family before/after packets and a final
environment-density packet with generation inputs and costs.

**Stopping condition:** reject a detail family that produces unreadable sheep,
excessive overdraw, unstable motion, an unexplained budget regression, or a need
for general streaming/LOD before the visual question is answered.

### Phase 5 — Add atmospheric depth with the simplest accepted technique

**Outcome:** Terrain layers separate through stable atmospheric depth and
directional softness while foreground gameplay remains readable.

**Likely files/components:** renderer atmosphere/fog calculation and resources,
scene settings, feature-owned contribution/debug output, capture/performance
tooling, and focused tests.

**Dependency direction:** renderer owns optical approximation and GPU resources;
scene settings provide bounded artistic inputs; gameplay never reads fog state.

**Checkable tasks:**

- Replace or extend the fixed 36–70 distance blend with a camera/scene-aware
  height/distance/directional-scattering candidate.
- Add a contribution/transmittance debug view sufficient to explain where the
  effect comes from.
- Compare no-atmosphere and candidate output in both views and along the route.
- Measure banding, temporal stability, focal readability, GPU time, and memory.
- Attempt a volumetric integration only if the simpler candidate leaves a named
  reference gap and a separate coherent outcome is approved.

**Validation:** parameter validation, no-effect equivalence where applicable,
normal/contribution/holdout frames, motion evidence, pass timing, memory, and
OpenGL diagnostics.

**Evidence artifact:** paired atmosphere-off/on/debug packet with settings,
reference-gap statement, route motion, and costs.

**Stopping condition:** reject the effect if it hides relevant sheep/terrain,
flickers, bands objectionably, or consumes budget without an owner-visible gain.

### Phase 6 — Improve procedural dog and sheep presentation

**Outcome:** Five animals have moderately detailed, readable procedural forms
and motion cues appropriate to the visual direction while authoritative
identity, transforms, collision, and behavior remain unchanged.

**Likely files/components:** procedural animal mesh/presentation code under
`render` or a presentation-only asset boundary, renderer inputs/draw path,
existing sheep-proxy tests, a focused animal-presentation test, and motion
capture tooling.

**Dependency direction:** presentation consumes immutable dog/sheep poses and
behavior evidence; no visual joint, mesh, or animation value becomes gameplay
authority.

**Checkable tasks:**

- Establish silhouette and articulation criteria from the primary game images;
  do not use `real-photo-sheep3.jpg` for anatomy or appearance.
- Increase geometry only where it improves facing, body/wool mass, legs, head,
  ears/tail, gait, or shadow readability.
- Add controlled per-ID visual variation without changing gameplay temperament
  or introducing unrecorded randomness.
- Exercise idle/motion/turn/settle cues available from current published state;
  label any scripted presentation fixture honestly.
- Preserve simple authoritative collision and one-to-one stable-ID pose mapping.

**Validation:** mesh/joint/pose bounds, stable-ID mapping, restart/interpolation,
no steady-state presentation allocation, same-state animal views, route motion,
shadow contribution, GPU/CPU/memory cost, and owner silhouette review.

**Evidence artifact:** animal normal/silhouette/debug views plus a short
five-animal motion/contact sheet and cost comparison.

**Stopping condition:** stop if readable motion requires new authoritative state
or gameplay semantics, if variation becomes nondeterministic, or if detail does
not survive the representative camera distance.

### Phase 7 — Establish stable edge quality and optional focal softness

**Outcome:** The scene has stable edge treatment and, only if useful, selective
depth of field that contributes softness/depth without compromising gameplay.

**Likely files/components:** renderer colour/depth targets already justified by
prior phases, one anti-aliasing/filtering candidate, optional DOF pass and focus
settings, feature-owned debug output, and capture/performance tooling.

**Dependency direction:** renderer owns sampling/filtering; the camera or scene
supplies an explicit presentation focus target. Gameplay rules do not depend on
post-processing.

**Checkable tasks:**

- Compare the simplest stable anti-aliasing candidates under camera motion and
  fine vegetation before selecting one; avoid temporal history unless a static
  solution demonstrably fails.
- Judge edge stability separately from atmosphere and depth of field.
- If DOF remains a named reference gap, implement a bounded candidate with an
  explicit focal region and debug visualization.
- Keep dog, relevant sheep, route, gates, and important terrain inside the
  readable gameplay focus; allow stronger defocus only in a named
  close/presentation profile or view.
- Capture effect-off/on/debug, representative/holdout, and motion evidence with
  GPU/memory cost.

**Validation:** focus/settings validation, no-effect equivalence where useful,
motion review for shimmer/ghosting/halos/focus pumping, readability checks,
pass timing, memory, and OpenGL diagnostics.

**Evidence artifact:** AA/filter and optional DOF comparison packet with focus
metadata, debug view, route motion, and owner notes.

**Stopping condition:** reject any candidate that creates ghosting, unstable
focus, blanket blur, vegetation crawl, or a cost disproportionate to its visible
gain.

### Phase 8 — Produce the five-sheep visual continuation verdict

**Outcome:** One reproducible reference/holdout packet supports an explicit
owner decision to continue, revise, pivot, or stop.

**Likely files/components:** native review runner, artifact manifest,
performance/profile configuration, human review packet, and documentation only
after the verdict.

**Dependency direction:** the runner observes the shipping executable; it does
not supply runtime content or approve its own output.

**Checkable tasks:**

- Run clean native Release configure/build and the complete Windows CTest suite.
- Run WSL development and sanitizer suites for cross-configuration regression
  evidence; label native Linux graphics as unrun unless actually available.
- Capture repeat normal/debug frames, representative/holdout motion, feature
  diagnostics, state, source hashes, OpenGL messages, and all required timings.
- Compare against the unchanged Phase 0 packet and document intentional versus
  incidental changes.
- Review every rubric item without inferring objective quality, flock-density
  proof, final art, or shipping support.
- Obtain and record the owner's explicit continue/revise/pivot/stop verdict.

**Validation:** artifact manifest/hash checks, comparable-state metadata,
required automated suites, current performance/memory/startup/package budgets,
no high-severity OpenGL diagnostics, and manual owner review.

**Evidence artifact:**
`artifacts/phase3/<date>/visual-feasibility-candidate/` with the complete human
review packet and verdict.

**Stopping condition:** do not plan or implement 25 sheep, resume the objective
loop, change the backend, or promote a visual baseline until the verdict and its
limitations are explicit.

### Conditional successor — Plan 25, then 100 authoritative sheep

This is not an implementation phase in the current plan. After a positive
five-sheep visual verdict, perform a new readiness/ownership plan for 25 sheep.
It must address the currently fixed authoritative arrays, renderer pose buffer,
replay/state-format compatibility, draw submission, spatial distribution,
visible overlap/contact policy, simulation and presentation costs, and density
readability. Only a successful 25-sheep verdict may unlock a separate 100-sheep
gate. The existing non-player 25/100 diagnostic proves shared-rule cost
observability, not a shipping presentation path.

## Verification matrix

| Concern | Focused evidence per outcome | Five-sheep gate evidence |
| --- | --- | --- |
| Scene determinism and bounds | Fixed inputs, geometry/material counts, near/distant bounds, invalid-setting tests | Repeated manifests and same-state captures |
| Gameplay invariants | Existing dog/sheep/collision/replay scenarios unchanged | Complete registered CTests plus state comparison |
| Renderer ownership | No GL types outside platform/render; immutable frame settings | Diff review and current architecture tests |
| Shadows | Coverage/debug mask, framebuffer status, motion stability, GPU time | Representative and holdout route evidence |
| Colour/materials | Explicit colour-space metadata, albedo/lighting debug, before/after | Reference/holdout review without crushed cues |
| Environment detail | Fixed generation counts/bounds, placement validity, overdraw where needed | Density/readability and route-motion review |
| Atmosphere | Off/on/contribution evidence, parameter bounds, temporal stability | Flock/route readability and pass cost |
| Animals | Stable ID/pose mapping, mesh/pose bounds, interpolation, allocations | Five-animal silhouette and motion review |
| Edge/focal softness | Candidate off/on, focus/debug output, motion artifacts | Active-subject readability in both views |
| Graphics correctness | Focused framebuffer/resource tests and GL diagnostics | Zero high-severity OpenGL messages |
| Performance | Per-pass time when decision-relevant, total GPU/frame, CPU submission/preparation, memory | p50/p95/p99 and budget verdict on recorded RTX 5070 Ti configuration |
| Reproducibility | Scenario/seed/tick/route/profile/viewport/exposure/source hashes | Repeat packet and manifest verification |
| Human acceptance | Narrow owner question after each meaningful visual outcome | Explicit continue/revise/pivot/stop verdict |

Every coherent code outcome runs the touched build target, focused unit/scenario
tests, the named capture/reproduction, diff inspection, and relevant diagnostic.
The gate runs the proportional broader suites defined in
[`DEVELOPMENT_WORKFLOW.md`](../DEVELOPMENT_WORKFLOW.md).

## Performance and platform matrix

| Platform/profile | Role in this plan | Required evidence | Claim boundary |
| --- | --- | --- | --- |
| Windows desktop, observed RTX 5070 Ti (Ryzen 9 9950X, 61.6 GiB, Windows 11 Home 10.0.26200, 2560×1440 display; hybrid with a Ryzen-integrated AMD adapter) | Reference visual-development machine | Active OpenGL adapter/version, selected viewport, full captures/motion, GL diagnostics, total/per-pass timing, memory, startup, package | Establishes only this recorded machine/configuration; no budget is measured on it yet |
| Windows laptop, GTX 1050 Ti Max-Q | Optional very-low-spec degradation/compatibility proxy | Named reduced profile, active adapter, visible degradation, stability and timing if run | Not the visual target or shipping minimum |
| Windows laptop, Intel UHD 630 | Legacy observation only unless owner retains it | No new visual gate required by this plan | Existing evidence does not create future support |
| WSL Ubuntu 24.04 | Build/headless/sanitizer host | Development, Release, sanitizer and relevant headless tests | OpenGL 4.5 host cannot provide accepted visual evidence |
| Native Linux target | Deferred platform gate | Build/context/capture/performance when representative hardware exists | Unsupported/unverified until run |
| ADR 0001 provisional Low/High classes | Existing design targets under review | Retain or supersede only after measured tracer evidence and owner decision | This draft plan changes no release promise |

Provisional reference budgets remain the accepted ADR 0001 High targets when
the Phase 0 viewport is 2560×1440/60: synchronized frame p95 at or below 16.67
ms, p99 at or below 20.84 ms, RSS at or below 1.5 GiB, startup at or below 3 s,
and compressed package at or below 64 MiB. These are whole-program gates, not
preallocated pass budgets. Record each new pass cost before deciding whether a
separate pass limit would change an implementation decision.

Passing these budgets on an RTX 5070 Ti proves headroom on that machine only. It
does not validate ADR 0001's RX 6600 High class, select a shipping minimum, or
justify unbounded visual cost. The budget values themselves are unchanged by the
reference-machine supersession and remain unmeasured on this GPU.

## Risks, rollback, and deferred work

- **Target unattainable under procedural-first constraints:** preserve every
  accepted baseline and stop for an asset/engine decision; do not quietly import
  reference-derived content.
- **Renderer monolith growth:** keep each pass and its minimum diagnostics in one
  coherent outcome. Extract only ownership demonstrated by the new pass; do not
  prebuild a universal renderer.
- **Visual/collision mismatch:** keep detailed near-field surfaces consistent
  with analytic authority and distant scenery unreachable. Roll back scenery
  that reads as playable.
- **Shadow instability:** retain the last accepted fitted solution and reject or
  separately plan cascades rather than accumulating tuning constants.
- **Colour-space ambiguity:** preserve the Phase 0 frame and disable the candidate
  path until the conversion contract is observed and tested.
- **Vegetation overdraw or crawl:** remove the failing detail family or reduce its
  bounded density; do not start speculative streaming/LOD.
- **Atmosphere hides gameplay:** keep the feature contribution/off comparison and
  reject the candidate rather than tuning around one beauty camera.
- **Animal detail without readable benefit:** retain the existing proxy path as
  a regression reference until the owner accepts the new silhouette/motion.
- **DOF or temporal softness fails in motion:** disable/remove the pass and keep
  the accepted stable anti-aliasing/atmosphere result.
- **Reference-machine overfitting:** require the holdout camera and optional
  laptop degradation pass; neither replaces future target-class testing.
- **Scope pressure toward 25/100:** the fixed five-pose buffer is an intentional
  boundary. Scale begins only through the conditional successor plan.
- **Objective-loop drift:** no visual phase may add farmer/HUD/objective behavior.
- **Vulkan drift:** no OpenGL inconvenience inside this tracer is itself approval
  for a backend migration; record the measured limitation first.
- **Global-illumination drift:** on 2026-08-22 the owner accepted GI as a later
  direction **on this same OpenGL backend**, scheduled at Phase 6 renderer depth
  ([ADR 0011](../decisions/0011-global-illumination-on-the-existing-opengl-backend.md)).
  Inside this tracer the only route is the Phase 3 named-gap clause, under its
  own separately approved outcome. Accepting a direction is not a schedule, no
  lighting dissatisfaction is itself a named gap, and the ray-tracing and
  volumetrics rulings above are unchanged by it.

Unaccepted effects remain easy to disable or remove until their owner verdict.
Do not retain a permanent matrix of experimental toggles after the gate; keep
only accepted profile differences and feature-owned diagnostics with continuing
value.

Deferred work includes final minimum/recommended specifications, native Linux
graphics, 25/100 authoritative sheep, general world generation/streaming/LOD,
hard sheep contact, the objective loop, final animation breadth, weather/water,
global illumination, Vulkan, and vendor-specific advanced rendering.

## Definition of done

This plan is implemented only when:

- the reference desktop's full hardware/display/driver/OpenGL configuration and
  selected viewport are recorded;
- the unchanged baseline and candidate packets are reproducible and linked;
- one bounded code-generated visual scene supports the representative route and
  untouched holdout view without expanding authoritative gameplay scope;
- accepted shadow, colour/material, detail, atmosphere, animal, and edge/focal
  outcomes each have their own debug/motion/performance evidence;
- existing deterministic gameplay, collision, snapshot, replay/state, accepted
  baseline, build, test, sanitizer, memory, startup, and package invariants pass
  or every unavailable gate is named;
- no high-severity OpenGL diagnostic or unresolved visible temporal defect
  remains;
- the owner records a continue/revise/pivot/stop verdict against the selected
  visual rubric;
- documentation records observed results without calling five sheep a density
  proof, candidate art final, or the RTX 5070 Ti a shipping minimum; and
- no objective-loop, 25/100-sheep, Vulkan, streaming, imported-asset, or release
  promise was silently implemented.

## Recommended first step

Run Phase 0 on the desktop before visual implementation: record the complete
machine and active OpenGL renderer, select the reproducible viewport (provisional
2560×1440/60 when supported), build the unchanged Release tree, and generate the
reference/holdout baseline packet. As of 2026-08-22 the owner has access to that
desktop and it is this repository's checked-out host, so the run is unblocked;
the preparatory seam is already in place and, as of 2026-08-22, the packet **has
been produced** with `result=pass` at
`artifacts/phase3/2026-08-22/visual-feasibility-baseline-183850545/`. What
remains of this step is the owner's camera/rubric verdict, which is still blank.
Should desktop access lapse again, the only safe preparatory code
outcome remains the narrowly configurable named-scene, camera, viewport, and
manifest seam with byte/semantic-equivalent current captures; do not tune
visuals against the laptop and later call them the RTX 5070 Ti baseline.
