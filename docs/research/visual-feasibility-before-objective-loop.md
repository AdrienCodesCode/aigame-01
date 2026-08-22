# Research: Visual feasibility before the objective loop

**Status:** Draft research; not implemented

**Produced by:** Codex

**Date:** 2026-08-22

**Project revision:** `9cc5c7d8a727c2958c01cedf19b7744836675994`;
worktree already contained uncommitted QA, herding-plan, and local influence-capture
changes before this research file

**Adversarial review:** Not yet reviewed

## Problem and decision

The current [`ROADMAP.md`](../../ROADMAP.md) next action is a three-to-five-minute
objective loop: one dog, farmer placeholder, five sheep, a gate and destination,
one farmer signal, success/failure/restart, and a minimal HUD. The owner proposes
temporarily shelving that loop because two more fundamental questions are not yet
settled:

1. Can the custom engine produce an environment, animals, lighting, shadows,
   atmosphere, scale, and motion that meet an owner-approved visual standard?
2. Is the direct-control herding premise clear enough to justify implementing
   the farmer cue and objective around it?

The decision is whether to keep building the objective loop, insert a bounded
visual-and-scale feasibility gate on OpenGL, or migrate to Vulkan before that
gate.

Success for this research means recommending the smallest reversible experiment
that can honestly kill or justify the project on visual grounds, while keeping
graphics-API learning separate from player value. This document does not approve
a renderer migration, implement art or behavior, redefine the game, or update the
roadmap.

### Owner follow-up on the decision inputs

On 2026-08-22 the owner accepted the OpenGL-first, visuals-before-objective
recommendation and selected these reference roles:

- The [overhead countryside](<../../ref/ChatGPT Image Aug 15, 2026, 03_22_55 PM.png>)
  and [closer hillside](<../../ref/ChatGPT Image Aug 15, 2026, 03_23_03 PM.png>)
  generated images are the primary idealized Wide Eye visual target.
- `visual-ref-1`, `visual-ref-2`, `visual-ref-3`, `visual-ref-4`, and
  `visual-ref-6` are secondary graphics-quality references for landscape scale,
  atmosphere, shadows, environmental density, and related presentation cues;
  they are not literal environment specifications. No `visual-ref-5` file was
  present in the worktree when inspected. After `visual-ref-2` and
  `visual-ref-3` were replaced with readable PNG files, owner and agent review
  identified controlled softness, layered depth of field, dense fine
  vegetation, and finer voxel-scale/geometry resolution as useful additional
  cues.
- [`real-photo-sheep3.jpg`](../../ref/real-photo-sheep3.jpg) is a reference only
  for sheep spatial distribution: local spacing, clustering, density variation,
  gaps, and outliers. It does not define sheep morphology, proportions, surface
  treatment, or appearance. It is visibly watermarked Bigstock material and is
  not approved as a project asset or for redistribution without documented
  rights.

The owner also accepted keeping OpenGL for the bounded visual tracer and the
recommended sequencing of a five-authoritative-sheep visual gate followed, only
after a positive visual verdict, by 25- and 100-sheep scale/behavior gates. The
owner subsequently clarified that the laptop's GTX 1050 Ti Max-Q should be
treated as a very-low-spec development/compatibility proxy, not the machine that
defines the target visual quality or an accepted shipping minimum. A better
desktop PC is available for later testing, but its exact CPU, GPU, RAM, display,
and driver have not yet been recorded. The shipping minimum/recommended profiles
and objective/farmer-cue contract remain unresolved.

## Verified project constraints

### The player and the farmer signal

- **Confirmed fact:** [`WIDE_EYE.md`](../game-design/WIDE_EYE.md) says the player
  directly controls the border collie using position, movement, facing,
  pressure, and release. The player does not select or order sheep.
- **Confirmed fact:** the planned whistle is an instruction *from the NPC farmer
  to the dog/player* that communicates the intended flock and destination. It is
  not currently a player-controlled dog whistle or a command sent to sheep.
- **Qualified finding:** “farmer whistle” is too compressed a label for an
  unimplemented interaction. The source design also says the player should *see*
  the intended flock and destination, but it does not yet specify whether this is
  conveyed by the farmer's position, gesture, UI, a gate highlight, audio, or a
  combination. The owner's confusion is evidence that this interaction should
  be renamed and resolved before implementation.

### Current visual and world baseline

- **Confirmed fact:** the renderer is a compact OpenGL 4.6 forward raster path.
  It already has fixed directional lighting, a filtered 1024-by-1024 shadow map,
  sky colour, and shader-based distance fog. The current fog is a depth cue, not
  volumetric lighting or a participating-media simulation.
- **Confirmed fact:** the current authoritative paddock collision bounds are
  32 by 32 world units. Shadow projection is hard-coded around the paddock centre
  at `(16, 4, 16)` with fixed extents. A materially larger landscape therefore
  needs a new shadow-coverage strategy and new scene ownership; merely enlarging
  the terrain mesh would make the existing shadow assumptions wrong.
- **Confirmed fact:** Phase 4 currently owns the art bible and readable animal
  presentation. Phase 6 currently owns measured scale work, culling/LOD/streaming,
  and one-at-a-time evaluation of effects such as volumetric atmosphere. Moving
  selected work earlier is an owner-authorized reprioritization, not evidence that
  all of Phases 4 through 6 should be pulled forward together.

### Sheep behavior and scale

- **Confirmed fact:** five authoritative sheep are the current correctness
  envelope. Larger flocks are hypotheses with an eventual capacity ladder, not
  demonstrated gameplay or renderer support.
- **Confirmed fact:** the current model includes soft neighbor separation but no
  hard sheep-to-sheep collision solver. Visible interpenetration is therefore
  expected in some configurations, not a renderer defect.
- **Qualified finding:** environment scale, rendered sheep count, authoritative
  simulation count, and convincing flock behavior are separate measurements. A
  beauty shot containing duplicated render-only sheep would not demonstrate that
  a large flock can behave or play correctly. Conversely, a 100-sheep diagnostic
  does not prove that the scene meets the visual bar.

### Observed Vulkan support on the available laptop

On 2026-08-22, Windows `vulkaninfo.exe --summary` was run from WSL against the
installed Windows drivers:

| Item | Observed result | Consequence |
| --- | --- | --- |
| Vulkan loader | Instance version 1.4.309 | The Windows Vulkan loader is installed and working. |
| Intel UHD Graphics 630 | Vulkan 1.2.177, driver 100.9664 | The integrated GPU can run Vulkan, but a Vulkan 1.3-only baseline would exclude it. |
| NVIDIA GeForce GTX 1050 Ti Max-Q | Vulkan 1.4.312, driver 581.57 | The discrete GPU can run a conventional Vulkan renderer. |
| Instance layers | NVIDIA Optimus/present layers only | `VK_LAYER_KHRONOS_validation` is not currently installed or visible, so the machine is not yet provisioned for serious Vulkan development. |
| Loader diagnostic | Stale Epic Online Services overlay manifest warning | This did not prevent enumeration, but it is environment noise to remove or account for before treating clean diagnostics as evidence. |

The full capability observation recorded on 2026-08-16 in
[`opengl-to-vulkan-feasibility.md`](opengl-to-vulkan-feasibility.md) found no mesh
shader or hardware ray-tracing extensions on either GPU. The GTX 1050 Ti's higher
reported core version does not add hardware that the device does not expose.

Khronos distinguishes platform/loader support from device support and recommends
querying each physical device's properties, features, extensions, limits, and
formats. A shipping backend therefore needs a capability contract and device
selection; a successful `vulkaninfo` run proves feasibility on this laptop, not
performance or support across the target hardware classes.

## Findings

### 1. A visual-first gate is rational if it is truly a stop criterion

**Qualified finding:** the owner has identified presentation as a prerequisite,
not polish to add after validating mechanics. Continuing to build objectives
before testing that prerequisite would answer the wrong project question. It is
therefore reasonable to pause the objective loop.

The useful experiment is not “make final art.” It is: can one representative
Wide Eye vista, rendered on named hardware from fixed views and in motion, reach
an agreed visual bar without invalidating the engine's size, performance, and
procedural-first constraints?

The bar is now externalized through the owner-selected reference roles above.
The two primary images set a cohesive target for voxel-informed stylization,
layered terrain depth, dense vegetation, warm low-angle light, long readable
shadows, misty atmosphere, animal silhouettes, and flock density. The geometry
and surface treatment must not become conspicuously chunky or aggressively
low-poly; the acceptable balance should be iterated through owner review rather
than fixed as a polygon-count slogan. Those properties—not the images'
unapproved HUD, commands, task structure, or exact scene objects—must become the
review rubric. The secondary references can clarify rendering quality but cannot
silently replace the primary direction. In particular, `visual-ref-2.png` and
`visual-ref-3.png` distinguish softness from coarseness: their atmosphere,
focal-plane separation, and filtered edges soften a scene that still contains
dense fine geometry. The tracer should therefore evaluate atmospheric depth,
anti-aliasing/filtering, and optional depth of field as separate contributors.
Depth of field fails the gameplay rubric if it obscures active sheep, the dog,
the intended route, gates, or important terrain edges; stronger defocus may be
reserved for close or presentation views.

### 2. Do not combine the art proof and the scale proof into one first step

**Recommendation:** use two gates in order:

1. **Visual-direction gate:** use the existing five authoritative sheep and
   dog in one expanded representative landscape. Prove composition, material and
   palette direction, animal readability, shadow coverage/stability, atmospheric
   depth, and motion at the owner-selected camera and hardware target. Keep the
   scene bounded; infinite terrain and production streaming are not necessary to
   answer this question.
2. **Scale-and-behavior gate:** only if the visual direction passes, generalize
   the authoritative snapshot/render path and test 25 sheep, then 100. Capture
   CPU simulation, render preparation, GPU passes, frame pacing, memory, and
   visible behavior. Add spatial indexing, instancing, culling, LOD, or collision
   work only in response to the measured limit or an explicit visual defect.

This ordering prevents poor proxy silhouettes from being mistaken for a scale
failure, and prevents a large but visually weak flock from being mistaken for an
art success. Counts above 100 should remain deferred until 25 and 100 provide a
reason to continue.

The five-sheep art gate does not claim that five sheep are the desired final
fantasy. It keeps one major variable fixed while the project asks whether its
visual language is viable.

### 3. OpenGL can answer the proposed visual question

**Confirmed fact:** OpenGL 4.6 exposes programmable graphics and compute stages,
shader storage, textures/images, framebuffer passes, indirect drawing, and the
other conventional facilities needed to prototype improved shadowing,
atmospheric/height fog, volumetric-light integration, instanced animals, culling,
and post-processing. The Khronos OpenGL 4.6 specification defines compute-shader
limits and the GLSL 4.60 specification describes compute shaders operating on
textures, images, storage buffers, and atomic counters.

**Inference, high confidence:** none of the visual features named in the owner's
proposal inherently requires Vulkan. A correctly implemented raster effect has
substantially the same intended pixels whether commands were submitted through
OpenGL or Vulkan. Vulkan may later improve CPU scaling, resource lifetime
control, synchronization control, or access to a Vulkan-only SDK, but it does not
automatically improve lighting, art direction, atmosphere, or animal animation.

The current OpenGL renderer is also the only backend with working paddock,
animal, shadow, capture, diagnostic, and timing paths. Using it for the visual
gate isolates the visual question. Migrating first would require reconstructing
known output before learning whether the intended new output is achievable.

### 4. Vulkan is usable on this laptop, with important limits

**Confirmed fact:** yes, Vulkan can be used on this laptop. Both installed GPUs
enumerate as Vulkan physical devices, and SDL3 provides the Windows-surface bridge
needed by this project's platform layer.

**Qualified finding:** “can run Vulkan” is not the same as “is a good advanced
renderer target.” If Intel UHD 630 remains in the supported development envelope,
a new backend must use a Vulkan 1.2-compatible baseline or explicitly exclude
that device. The GTX 1050 Ti Max-Q is useful for very-low-spec compatibility and
degradation evidence, but the owner has explicitly rejected it as the visual
quality reference or an accepted shipping minimum. Its performance with a larger
scene, many animated sheep, better shadows, and volumetrics is nevertheless
unmeasured. Neither observed GPU offers the mesh/ray features that motivated
some earlier Vulkan discussion.

A Vulkan implementation would also need explicit physical-device selection on
this hybrid-GPU machine, validation tooling, swapchain and resize handling,
resource allocation, synchronization, shader compilation, capture support, and
parity tests. SDL3 creates a Vulkan surface; it does not supply those renderer
systems.

### 5. The farmer cue can be deferred without changing dog control

**Qualified finding:** the confusion is about job communication, not the control
scheme. The approved mechanic remains direct dog control. Before an objective
loop is resumed, choose one of these explicit contracts:

- **Farmer intent cue:** the NPC farmer indicates “these sheep, through that
  gate,” using a visible signal plus optional whistle audio. This is the closest
  reading of the current design and avoids audio-only communication.
- **World objective cue:** the destination is legible from gate/pen staging and
  minimal UI, with the farmer absent from the first experiment.
- **Player whistle ability:** the dog/player emits a command that affects sheep
  or another dog. This is a different mechanic and would need its own behavioral
  rule and playtest question; it should not be smuggled in under the existing
  roadmap label.

The visual feasibility gate does not require deciding among them immediately.

## Options and tradeoffs

| Option | What it answers | Advantages | Costs and risks | Verdict |
| --- | --- | --- | --- | --- |
| A. Continue the objective loop now | Whether five-sheep direct herding forms a comprehensible task | Fastest route to gameplay evidence; follows the current roadmap | Invests in a loop the owner may reject if the engine cannot reach the required look; implements an ambiguous farmer cue | Not recommended under the owner's stated stop criterion |
| B. Bounded OpenGL visual gate, then scale gate | Whether the look is viable, then whether 25/100 authoritative sheep remain viable | Uses the working backend; isolates variables; gives an early honest kill/continue result | Delays gameplay evidence; can drift into polish unless references and stop rules are fixed | **Recommended** |
| C. Vulkan parity first, then the same visual gate | Whether the owner wants to learn/own Vulkan and whether the look works afterward | Establishes the long-term API before renderer growth | Delays visual learning; adds tooling and parity work; does not improve pixels by itself; available laptop lacks the advanced mesh/ray features discussed | Use only if Vulkan-engine learning is itself the primary objective |
| D. Start art, large landscapes, 100+ sheep, behavior overhaul, volumetrics, and Vulkan together | No single question reliably | Maximum visible activity | Confounded failures, unbounded scope, difficult regression diagnosis, and no clean cancellation point | Reject |

## Recommendation

Pause the objective/HUD/farmer-cue work and authorize a **bounded visual
feasibility tracer on the existing OpenGL backend**. This is a temporary reorder,
not a claim that gameplay is unimportant and not approval to pull all later
renderer work into the current milestone.

Before implementation, lock these gates:

1. **References and rubric:** use the two primary idealized game images, the
   secondary quality references, and the licensed-use boundary around the real
   sheep photo to define named judgments for landscape depth, atmosphere,
   shadow quality/stability, palette/material cohesion, animal silhouette,
   flock readability, controlled softness, focal readability, geometry detail,
   and motion.
2. **Test state:** one deterministic scene, fixed camera views plus a short
   repeatable camera/gameplay path, matching baseline/candidate captures, and no
   debug overlay in the beauty view.
3. **Hardware and output:** record the better desktop's CPU, GPU, RAM, display,
   driver, resolution, and frame target before declaring it the reference visual
   machine. Treat the GTX 1050 Ti Max-Q laptop as optional very-low-spec
   compatibility/degradation evidence, not a requirement to reproduce the full
   target look. Propose shipping minimum and recommended profiles only after the
   tracer is measured; do not guess them from GPU age or marketing tiers.
4. **Bounded first implementation:** expand one landscape composition; replace
   the fixed paddock-centred shadow assumptions; improve palette/material and
   animal silhouettes; then evaluate atmospheric depth. Add effects one at a
   time. “Volumetric” should not be a checkbox if a cheaper height-fog or
   directional-scattering treatment produces the accepted image.
5. **Kill/continue decision:** if the fixed scene cannot reach the agreed look
   within the accepted GPU/frame/memory envelope, stop or reconsider the engine
   direction. If it passes, proceed to 25 and then 100 authoritative sheep before
   resuming the objective-loop decision.

Keep OpenGL during this experiment. Make the Vulkan decision afterward from
measured OpenGL bottlenecks and the owner's product priority. If the true priority
is instead “learn and build a Vulkan engine even if Wide Eye is delayed,” state
that explicitly and run an adversarial plan from
[`opengl-to-vulkan-feasibility.md`](opengl-to-vulkan-feasibility.md) before code.

## Failure modes and gotchas

- **Undefined visual target:** repeated polishing cannot produce a decisive
  result without references, rubric, target GPU, resolution, and stop criteria.
- **Beauty-shot bias:** a fixed screenshot can hide shadow shimmer, temporal fog
  noise, bad animal animation, popping, and frame pacing. Review motion as well.
- **Atmosphere as camouflage:** fog can create depth but also conceal flock
  facing, pressure cues, gates, and terrain hazards. Gameplay readability remains
  an invariant even before the objective exists.
- **Fake scale:** duplicated render-only animals do not prove authoritative flock
  simulation. Label any crowd stand-in honestly or avoid it.
- **Behavior confounded with collision:** visible overlap may look like poor art
  even when caused by the current soft-separation rule. Define the acceptable
  minimum spacing before judging a 25/100-sheep scene. If hard contact becomes a
  requirement, use a deterministic local spatial broad phase rather than naive
  all-pairs collision, then profile it.
- **Shadow coverage collapse:** the current fixed projection is tied to the
  32-by-32 paddock. Larger vistas need camera/scene-relative coverage, potentially
  cascades or another measured solution, and stability testing in motion.
- **Vulkan feature-name optimism:** a Vulkan version number does not imply mesh
  shaders, ray tracing, good performance, or a complete development toolchain.
- **Hybrid-GPU ambiguity:** renderer evidence must name the active GPU. Merely
  having the GTX 1050 Ti installed does not prove an app selected it.
- **Permanent dual backend:** keeping OpenGL and Vulkan indefinitely would double
  renderer verification and obscure the visual experiment. A future parity
  period should have an explicit retirement decision.

## Evidence and confidence

| Claim | Basis | Confidence and limitation |
| --- | --- | --- |
| The farmer whistle is an NPC intent cue, not the player's control verb | Approved `WIDE_EYE.md` | High for the current design; the presentation of the cue remains unresolved |
| The current renderer has simple fog and shadows but is tied to the bounded paddock | Source inspection at revision `9cc5c7d` | High |
| OpenGL 4.6 can prototype the named conventional raster/compute effects | Khronos OpenGL 4.6 and GLSL 4.60 specifications plus current renderer | High for API capability; quality and performance remain implementation- and hardware-dependent |
| Vulkan works on the available laptop | Local `vulkaninfo.exe --summary`, 2026-08-22 | High for enumeration on installed Windows drivers; no Vulkan app or performance workload was run |
| Intel must constrain a Vulkan baseline if it remains supported | Local Intel Vulkan 1.2.177 observation | High for the observed driver; updating/excluding the driver is an owner/platform decision |
| Vulkan will not by itself improve image quality | Rendering-semantics inference and existing feasibility research | High |
| Five-sheep art proof followed by 25/100 scale proof is the lowest-confounding order | Architecture/test-design inference | Medium-high; the owner must choose what flock count is visually essential |
| The GTX 1050 Ti is the target visual-quality or shipping-minimum device | Owner clarification on 2026-08-22 rejects both roles | High; it remains useful as a very-low-spec proxy |
| The better desktop can meet the proposed advanced visual bar | Its hardware and no representative workload have been recorded | **Unresolved** |

## Planning handoff

A later plan can treat these as resolved owner inputs:

1. The immediate priority is testing Wide Eye's visual feasibility, not learning
   Vulkan at the cost of delaying that evidence.
2. The two generated sheep-game images are the primary idealized visual target;
   the numbered visual references are secondary quality cues, and
   `real-photo-sheep3.jpg` informs only spatial flock distribution. The
   voxel/detail balance remains deliberately iterative and must avoid an overly
   chunky or aggressively low-poly result.
3. The first art gate uses five authoritative sheep. A positive human visual
   verdict unlocks 25 and then 100 authoritative sheep as separate
   scale/behavior gates.
4. The GTX 1050 Ti Max-Q laptop is a very-low-spec
   development/compatibility/degradation proxy, not the full-quality visual
   reference and not an accepted shipping minimum.

The following material choices remain unresolved:

1. What are the better desktop's exact CPU, GPU, RAM, display, and driver, and
   what resolution/frame target must the reference visual gate pass?
2. Which measured hardware becomes the eventual shipping minimum and recommended
   profile? ADR 0001's Iris Xe/RX 6600 profiles remain provisional until an
   explicit decision retains or supersedes them.
3. When the objective returns, should intent come from a visible farmer cue, from
   the world/gate, or from a new player-controlled whistle mechanic?

After those decisions, an incremental plan should name one effect or ownership
change per outcome, preserve deterministic simulation and accepted captures,
and place a human visual verdict between the five-sheep art gate and scale work.

## References

### Local project evidence

- [`ROADMAP.md`](../../ROADMAP.md), current checkpoint, Phase 3 objective loop,
  post-Phase-3 graphics decision, Phase 4 art, and Phase 6 renderer depth.
- [Approved first-playable design](../game-design/WIDE_EYE.md).
- [Broader herding and scale hypotheses](../game-design/HERDING_GAMEPLAY.md).
- [Herding simulation and scale plan](../plans/herding-simulation-and-scale.md).
- [OpenGL-to-Vulkan feasibility research](opengl-to-vulkan-feasibility.md).
- [`OpenGlRenderer` implementation](../../src/render/opengl_renderer.cpp).
- [Paddock collision bounds](../../src/game/paddock_collision.hpp).
- [Primary overhead visual target](<../../ref/ChatGPT Image Aug 15, 2026, 03_22_55 PM.png>)
  and [primary closer visual target](<../../ref/ChatGPT Image Aug 15, 2026, 03_23_03 PM.png>).
- [Secondary softness/depth reference 2](../../ref/visual-ref-2.png) and
  [secondary softness/depth reference 3](../../ref/visual-ref-3.png).
- [Real-sheep spatial-distribution reference](../../ref/real-photo-sheep3.jpg),
  spatial distribution only; visible Bigstock watermark and no documented
  project usage rights.

### Primary external sources

- Khronos, [Checking for Vulkan support](https://docs.vulkan.org/guide/latest/checking_for_support.html),
  accessed 2026-08-22.
- Khronos, [Querying Vulkan properties, extensions, features, limits, and
  formats](https://docs.vulkan.org/guide/latest/querying_extensions_features.html),
  accessed 2026-08-22.
- Khronos, [Enabling Vulkan features](https://docs.vulkan.org/guide/latest/enabling_features.html),
  accessed 2026-08-22.
- Khronos, [OpenGL 4.6 Core Profile
  Specification](https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf),
  May 5, 2022.
- Khronos, [OpenGL Shading Language 4.60.8
  Specification](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html),
  accessed 2026-08-22.
- SDL, [`SDL_Vulkan_CreateSurface`](https://wiki.libsdl.org/SDL3/SDL_Vulkan_CreateSurface)
  and [SDL3 Vulkan category](https://wiki.libsdl.org/SDL3/CategoryVulkan),
  accessed 2026-08-22.

## Recommended next step

Do not implement the current objective loop or begin a Vulkan migration yet.
Use `$plan-from-research` on this file to challenge and sequence the bounded
OpenGL visual tracer against the selected reference roles. Resolve the target
GPU/resolution/frame budget during that planning gate, and revisit the farmer
intent contract before objective-loop work resumes.
