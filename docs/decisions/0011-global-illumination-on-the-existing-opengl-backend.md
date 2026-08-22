# ADR 0011: Global illumination on OpenGL, hardware ray tracing held

**Status:** Accepted
**Date:** 2026-08-22
**Decision owner:** Project owner

## Context

On 2026-08-22 the owner named global illumination and ray-traced volumetric
clouds as long-term rendering ambitions. That statement is recorded, with the
rulings it runs into, in
[`HERDING_GAMEPLAY.md`](../game-design/HERDING_GAMEPLAY.md#rendering-ambitions).

Four existing records already constrain the ask, and they agreed with each other
before this decision:

- [ADR 0001](0001-native-foundation.md) accepted C++23/SDL3/**OpenGL 4.6 Core**
  as the foundation, with provisional Low/High budgets and a dependency gate.
- The approved
  [visual-feasibility plan](../plans/visual-feasibility-before-objective-loop.md)
  lists Vulkan, a permanent dual backend, ray tracing, mesh shaders, DLSS, and
  FSR among its **non-goals**, and its adversarial review **rejected** the claim
  that "the RTX 5070 Ti justifies ray tracing or vendor features", on the grounds
  that the experiment asks whether the visual target works rather than whether
  the newest GPU path can be exercised, and that cross-vendor OpenGL should be
  preserved.
- [`opengl-to-vulkan-feasibility.md`](../research/opengl-to-vulkan-feasibility.md#recommendation)
  recommends continuing on OpenGL, deciding the backend question no earlier than
  the Phase 3 exit gate, and deferring ray queries, ray tracing, Brixelizer GI,
  mesh shaders, and asynchronous compute until conventional Vulkan parity exists
  and a named experiment or bottleneck justifies each one.
- The roadmap's
  [graphics-backend decision](../../ROADMAP.md#graphics-backend-decision)
  records the owner's verdict of 2026-08-22: retain OpenGL for the tracer;
  Vulkan remains unapproved.

**A hard fact, independent of every ruling above.** OpenGL 4.6 has no ray
tracing. There is no core or ARB ray-tracing path in the API this project is
pinned to; hardware ray tracing is reached through Vulkan
(`VK_KHR_ray_tracing_pipeline`, `VK_KHR_ray_query`) or D3D12. "Add hardware ray
tracing" is therefore not a rendering-feature decision at all. It is a decision
to add a Vulkan backend, and it inherits every cost, gate, and parity
requirement the feasibility study attaches to that migration.

What was missing was an owner decision separating two asks that had been
travelling together as one: **ray tracing**, which is a backend question, and
**global illumination**, which is a lighting question with routes that do not
need a backend change. This ADR separates them and rules on both.

## Decision

### Global illumination is accepted as a direction, on OpenGL 4.6

Indirect light is an accepted long-term rendering direction for Wide Eye, to be
achieved on the OpenGL 4.6 Core backend accepted in
[ADR 0001](0001-native-foundation.md) rather than by way of hardware ray
tracing.

Two candidate routes are named without choosing between them:

- **Probe / irradiance-volume GI** — bounded irradiance samples placed through
  the world and interpolated at shading time.
- **Voxel cone tracing** — indirect light gathered by cone-marching a voxel
  representation of the scene.

Voxel cone tracing is noted as a natural fit for a world that is already a voxel
grid. That is an **inference** from the existing data structure, not a measured
result and not a selection: no GI route has been prototyped, captured, timed, or
costed in this repository, and this ADR creates no expectation that either route
will be reached. Route selection belongs to whoever plans the work, from
measurements that do not exist yet.

### Accepted as a direction is not scheduled work

This distinction is the load-bearing half of the decision, so it is stated
exactly:

1. **The approved visual-tracer sequence continues unchanged.** The next outcome
   remains the bounded distant vista in Phase 1 of the
   [visual-feasibility plan](../plans/visual-feasibility-before-objective-loop.md).
   Nothing in this ADR reorders, replaces, or adds to that sequence.
2. **Implementation belongs at Phase 6, renderer depth.** GI joins the existing
   renderer-depth candidate list in
   [Phase 6](../../ROADMAP.md#phase-6--tracer-5-measured-scale-and-renderer-depth),
   beside SSAO, improved anti-aliasing, stylized water, volumetric atmosphere,
   PCSS, and reflections.
3. **It inherits every Phase 6 gate and is granted no exception.** Low/high
   capture baselines are established before optimizing; each candidate defines a
   deterministic representative scene/route plus at least one owner-controlled
   holdout camera or seed that was not the tuning target; each carries
   feature-owned debug output and stable human-readable pass/resource labels;
   candidates are considered one at a time with identical-state evidence; and an
   effect that reduces flock readability, temporal stability, or low-target
   performance is rejected.

### Hardware ray tracing is held

Hardware ray tracing is **not pursued now**. This confirms the rulings listed in
the context rather than adding a new prohibition: the plan already rejected it,
the feasibility study already deferred it, and the roadmap's backend verdict
already retained OpenGL.

Two consequences of the hard fact above are recorded so a later reader does not
have to rediscover them:

- A hardware-RT proposal is a **Vulkan proposal**, and must go through the
  roadmap's graphics-backend gate: a native Windows/Linux capability inventory
  on the actual target classes, adversarial planning from the feasibility study,
  an accepted superseding ADR, OpenGL preserved as a temporary known-good
  reference, and explicit parity/cancellation/retirement gates.
- **A Vulkan migration is not authorized by this ADR.** Accepting GI as a
  direction is specifically *not* a step toward a backend change; it is the
  reason a backend change is not needed in order to pursue the owner's lighting
  ambition.

"Held" means held, not forbidden permanently. The reopening conditions are
below.

### The one route by which GI could reach the visual tracer earlier

Mirroring the way the approved plan already handles volumetrics — permitted only
if the simpler atmosphere leaves a named reference gap — there is exactly one
earlier route, and it is narrow.

If the visual tracer's own
[Phase 3](../plans/visual-feasibility-before-objective-loop.md#phase-3--establish-colour-exposure-lighting-and-material-response)
(colour, exposure, lighting, and material response) identifies a **named
reference gap** against the approved reference images that only indirect light
closes, then a global-illumination approximation may be attempted as its **own
separate approved coherent outcome**, with its own evidence.

All three conditions bind together. A named gap without an approved outcome does
not start the work; dissatisfaction with direct lighting is not a named gap; and
every stop condition in that plan — including the ones on authoritative
collision, world expansion, and vendor-specific paths — is unchanged by this
route.

### Alternatives considered and rejected for now

- **Authorize a Vulkan backend now so hardware RT becomes available.** Rejected.
  It reverses an owner verdict recorded the same day, contradicts the
  feasibility study's timing recommendation, and spends the parity work before
  the project has a playable objective loop or a first-playable signal. The
  ambition it would serve is reachable without it.
- **Schedule GI into the visual tracer immediately.** Rejected. The tracer's
  question is whether the visual target is achievable through bounded, measurable
  steps; inserting the most expensive lighting technique into that sequence would
  answer a different question and would breach the plan's own
  one-effect-at-a-time discipline.
- **Select the GI route now — commit to voxel cone tracing.** Rejected as
  premature. The voxel-grid affinity is a real argument, but the project has no
  measured lighting bottleneck, no GI prototype, no world large enough to make
  the trade visible, and no low-target measurement. Choosing a route now would
  record a preference as a decision.
- **Treat SSAO as sufficient and drop GI.** Rejected. Ambient occlusion is a
  contact-shadow approximation, not indirect light, and cannot deliver the bounce
  and colour bleed the owner named. SSAO remains its own independent Phase 6
  candidate; it is neither a prerequisite for GI nor a substitute for it.
- **Leave GI in "Deferred ideas — not current scope" and record nothing.**
  Rejected. The ask keeps recurring together with ray tracing, and leaving it
  unrecorded is what allows the two to be conflated again — which is the
  confusion this ADR exists to end.

## Consequences

- The owner's lighting ambition and the accepted backend are no longer in
  tension. Pursuing GI now requires no backend decision, and a backend decision
  is no longer implied by pursuing GI.
- Phase 6's renderer-depth candidate list grows by one entry under gates that
  already existed. No budget, threshold, stop condition, or exit gate moved, and
  no roadmap checkbox was ticked, added, or removed by this decision.
- GI is a **Goal** with no supporting measurement in this repository. Nothing
  here claims it is affordable on the reference desktop, on the optional
  very-low-spec laptop, or at any flock size. The Phase 6 low/high baselines are
  what would establish that, and they do not exist yet.
- Voxel cone tracing, if it is ever chosen, would couple the renderer to the
  voxel representation more tightly than the current
  [renderer façade](../../src/README.md) does. That coupling is a cost to weigh
  at route-selection time, not a reason to pre-build an abstraction for it now.
- Ray-traced volumetric clouds, named by the owner in the same conversation,
  remain **undecided**. The plan's rejection of volumetrics before simpler
  atmosphere stands untouched. The raymarched fragment-shader form of volumetric
  cloud rendering needs no RT hardware path, but nothing about it is accepted
  here.
- A future reader who finds "ray tracing" in a wishlist has one place to learn
  that it is a backend question rather than a shader question.

## What would reopen this decision

Any one of the following, and none of them is satisfied today:

- **A named reference gap that only indirect light closes**, identified by the
  visual tracer's Phase 3 as described above. This reopens *scheduling*; it does
  not reopen the backend ruling.
- **A measured GI result that fails on the OpenGL backend for a reason specific
  to the API** — not to the technique, the content, or the hardware — recorded
  with build, date, platform, scene, and method. This is the only evidence that
  would make hardware RT a rendering argument rather than a preference.
- **An accepted superseding ADR authorizing Vulkan**, produced through the
  roadmap's graphics-backend gate. Hardware ray tracing cannot arrive before
  that ADR does.
- **An owner decision that the reference look is unreachable without indirect
  light**, taken against same-state captures rather than against the reference
  images alone.

Route selection between probe/irradiance-volume GI and voxel cone tracing does
not reopen this ADR; it is a Phase 6 planning decision made from Phase 6
measurements.
