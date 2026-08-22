# Research input: render-distance techniques for a C++/OpenGL voxel engine

**Status:** Research note. Records an agent-produced, owner-supplied reading
list and maps its recommendations onto this project's measured position and
existing phases. **It adopts nothing, changes no phase, ticks no box, and adds
no dependency.** One recommendation in it is time-sensitive; that recommendation
is flagged below as a recommendation, not as a decision taken.

## Source and provenance

A long annotated technical reading list titled **"Improving Render Distance in
a C++/OpenGL Voxel Engine"**, produced by **Codex** and supplied to this
repository by the owner on **2026-08-22** as pasted text. The document
self-dates its survey as "Checked: 22 August 2026".

The honest limits of that provenance, stated before anything is drawn from it:

- **It is agent output, not a measured study.** No build, machine, scene, or
  method accompanies it as a whole. It is a synthesis of other people's
  published work, and it inherits whatever those sources got right or wrong.
- **Its "Checked: 22 August 2026" date is its own claim**, not an observation
  made by this repository.
- **The pasted text is not in the repository and was not fetchable from this
  environment.** It exists in the session that received it. This note was
  written from that session's summary of the document — its thesis, its
  budget framing, its taxonomy, its named recommendations, and the one sentence
  reproduced verbatim below — rather than from the document itself. So this note
  records *structure and claims*, not wording; only the two-sentence thesis in
  [Central thesis](#central-thesis) is presented as the document's own words,
  and the closing rule is a paraphrase. The techniques listed here are the
  ones the summary reproduced. The document is longer than this list; absence
  from this note is not evidence of absence from the document.
- **Its roughly sixty linked sources were not independently verified here.
  None of the URLs was fetched.** Not one claim below has been reproduced,
  timed, or even read at its origin by this repository.

Per [`AGENTS.md`](../../AGENTS.md), a link alone is not a reproducible input,
and neither is this note: it is a second-hand record of a synthesis of
third-hand claims. Everything attributed to the document is an **unverified
claim**. Everything attributed to this project is labelled with its evidence.

This note is a sibling to
[`external-voxel-render-distance-input.md`](external-voxel-render-distance-input.md),
which records a different external input on the same topic. Where the two
overlap, this note cross-references rather than restates — see
[Relationship to the sibling note](#relationship-to-the-sibling-note).

## Central thesis

The document's organising argument, quoted as the session reproduced it:

> Greedy meshing makes the current mesh cheaper. LOD makes distant terrain
> fundamentally less detailed.

The wider thesis around that sentence is that **long render distance is not one
optimization but several layers**. A technique that halves the cost of the mesh
you already build does not change how much terrain you must build; a technique
that stops building distant terrain at full detail does. Confusing the two is
how a project spends months on the wrong layer.

## The five budgets

The document frames every technique against five separate budgets, and makes
the point that **a technique that fixes one budget may do nothing for the
others**:

1. **CPU meshing and generation** — the cost of producing chunk data and turning
   it into triangles.
2. **Draw-call and driver overhead** — the cost of submitting the work,
   independent of how much work it is.
3. **GPU geometry and fragment cost** — vertices transformed, triangles
   rasterized, fragments shaded, overdraw.
4. **RAM, VRAM, and bandwidth** — what stays resident and what moves across the
   bus each frame.
5. **Numeric precision** — depth-buffer and world-coordinate precision, which
   degrades with distance regardless of how fast everything else is.

This framing is the most useful thing in the document after the closing rule,
because it explains why a "voxel performance" recommendation from one project
can be irrelevant to another: the two projects were not short of the same thing.

## Taxonomy

The techniques the summary reproduced, grouped by the budget each one attacks:

| Budget | Techniques the document covers |
| --- | --- |
| CPU meshing and generation | Greedy meshing; two-pass and single-pass meshers; background/threaded meshing with per-frame budgets; generation parameterized by resolution |
| Draw-call and driver overhead | Multi-draw indirect (MDI); shared vertex pooling / persistent buffers; GPU-driven indirect draws; batching chunk draws |
| GPU geometry and fragment cost | Frustum culling; occlusion culling (hierarchical-Z and query-based); level of detail; 2.5D height/column far proxies; distance fog as a horizon hider |
| RAM, VRAM, and bandwidth | Streaming distance and eviction; residency budgets; mesh caching; budgeted upload queues; compressed voxel representations |
| Numeric precision | Reversed-Z depth with zero-to-one clip control; camera-relative (origin-rebased) rendering; large-world coordinate handling |
| Alternative representations | Transvoxel; sparse voxel octrees; GigaVoxels-style out-of-core raymarching; sparse voxel DAGs |
| Method | "Stage 0 — establish a reproducible benchmark" before optimizing anything |

## The closing rule of thumb

This is the best thing in the document, and the part most worth keeping. It is
**paraphrased**, not quoted — the wording below is this note's, the rule is the
document's:

> Any proposed render-distance optimization must reduce at least one of
> generated data, resident data, mesh complexity, visible geometry, OpenGL
> submissions, GPU fragments, upload bandwidth, or precision error — otherwise
> it will not materially extend render distance.

Eight named levers. A proposal that reduces none of them is not a
render-distance optimization, whatever else it is. That rule is directly usable
as a review question, and it does not depend on any of the document's unverified
numbers.

## Where this project actually stands

The mapping below is only meaningful against this project's real position, so
the position is stated first, with its evidence.

**Observed result (2026-08-22, reference desktop — AMD Ryzen 9 9950X, NVIDIA
GeForce RTX 5070 Ti, 2560×1440, OpenGL 4.6; Phase 0 baseline packet
`artifacts/phase3/2026-08-22/visual-feasibility-baseline-183850545/`,
`result=pass`):**

| Measurement | Value | Budget |
| --- | --- | --- |
| GPU render p95 | `114304` ns | `16670000` ns frame budget |
| CPU submission p95 | `237800` ns | same frame budget |
| Process peak RSS | `76713984` bytes | `1610612736` bytes |
| Default-framebuffer depth | `24` bits, fixed point | — |

GPU render p95 is about **0.7%** of the frame budget; CPU submission p95 is
about **1.4%**; peak RSS is under **5%** of the memory budget.

**Observed result (same packet).** The world drawn is the handcrafted paddock:
`4` chunks, `1746` occupied blocks, `2754` emitted faces (of `10476` face
decisions, `7722` culled by the neighbour test).

**Observed state of the code**, from
[`src/README.md`](../../src/README.md) and the sources it describes:

- The mesher is the deliberately naive two-pass exposed-face mesher with a fixed
  16³ conservative ceiling and checked caller-supplied aggregate limits
  ([`naive_mesher.hpp`](../../src/voxel/naive_mesher.hpp)). Its own header
  records that greedy merging remains a later outcome.
- The paddock's four chunks are offset into **one** world-space upload, so
  terrain is not a per-chunk submission problem today.
- **There is no procedural generation, no streaming, no eviction, no LOD, and
  no culling of any kind beyond the mesher's neighbour test.**
  [`handcrafted_paddock.hpp`](../../src/voxel/handcrafted_paddock.hpp) says so
  in its own comment; `src/README.md` records that the one-time opaque upload
  implements no rebuild queues or streaming.
- `far_plane` is `100.0`, now one validated field in
  [`scene_render_settings.hpp`](../../src/render/scene_render_settings.hpp)
  rather than a literal repeated across five vertex shaders.
- World voxel coordinates are already **signed 64-bit**
  (`GridCoordinate = std::int64_t` in
  [`coordinates.hpp`](../../src/voxel/coordinates.hpp)) with floor division.

**Owner decision, 2026-08-22 — the world shape**, recorded in
[`HERDING_GAMEPLAY.md` § World extent](../game-design/HERDING_GAMEPLAY.md#world-extent):
Wide Eye is **explicitly not an open world**. The playable area stays bounded,
the owner wants those bounds far apart, and the world stays *visible* beyond
them as scenery that is **unreachable and non-authoritative**. Visible extents
named in conversation are 1, 10, and 20 km² — owner-named magnitudes, not
accepted budgets.

**Inference.** Four of the document's five budgets are, right now, empty. This
project is not short of CPU meshing time, submission overhead, GPU cost, or
memory. It is short of *world*. The one budget where distance will bite
regardless of how small the scene is — because it is a property of the
projection and not of the workload — is **precision**. That asymmetry drives
the entire mapping below.

## Mapping onto this project

### (a) Applies now

| Recommendation | Why it applies now |
| --- | --- |
| **Reversed-Z depth plus camera-relative rendering** | The only precision item, and the only one whose cost grows with delay. Detailed in [the next section](#the-time-sensitive-item-reversed-z-and-camera-relative-rendering). **Recommendation, not a decision taken.** |
| **Fog end derived from effective draw distance rather than set independently** | Already routed: the sibling note placed this in visual-plan [Phase 1](../plans/visual-feasibility-before-objective-loop.md#phase-1--establish-the-bounded-visual-scene-and-render-settings), and the two consolidated fields (`fog_start_distance` `36.0`, `fog_end_distance` `70.0`, against `far_plane` `100.0`) now sit in one validated record. This document agrees with the sibling note; it adds no new claim. |
| **The eight-lever review question** | Usable immediately as a review test on any future render-distance proposal, at zero cost, without adopting a single technique. |
| **"Stage 0 — establish a reproducible benchmark" before optimizing** | Applies, and is largely already satisfied — see [where this project is ahead](#where-this-project-is-already-ahead-of-the-document). |

### (b) Belongs to an existing phase

| Recommendation | Phase | Argument |
| --- | --- | --- |
| Greedy meshing | [Roadmap Phase 6](../../ROADMAP.md#phase-6--tracer-5-measured-scale-and-renderer-depth) — "Add greedy meshing only if it improves the measured bottleneck without breaking boundary tests" | Attacks mesh complexity. There are `2754` faces in the whole world; halving them saves nothing measurable against a GPU p95 of `114304` ns. The document's own thesis is the reason to wait: greedy meshing makes the *current* mesh cheaper, and the current mesh is not the problem. It is also the technique most likely to break the mesher's per-face oracles and the voxel diagnostic ledger, so it should be bought with a measurement, not with enthusiasm. |
| Coarse LOD hierarchy / chunk LOD bands with seam handling | Roadmap Phase 6 — "Add LOD only if draw distance remains a measured constraint"; generation-side prerequisite already accepted at [Phase 5](../../ROADMAP.md#phase-5--tracer-4-procedural-voxel-world) | Attacks visible geometry and generated data — a real lever by the document's own rule. The generation half is already an accepted constraint (the resolution divisor); the render half is gated on a constraint that does not exist. |
| 2.5D height/column far proxy | Phase 5 (generation) plus Phase 6 (render); its *design* argument should inform visual-plan Phase 1 now | Suits this project unusually well — see [the note below](#the-far-proxy-suits-this-project-unusually-well). |
| Streaming distance, eviction, residency budgets | Roadmap Phase 6 — "Add streaming distance and eviction with explicit memory budgets" | Attacks resident data. Peak RSS is `76713984` bytes against a `1610612736` byte budget, and the world-shape decision notes that a *finite* low-resolution surround may be able to stay resident rather than stream at all. Streaming may turn out to be unnecessary here, which is a reason to measure before building it. |
| Mesh caching and budgeted GPU upload queues | Roadmap Phase 6 — already an item | Attacks upload bandwidth. The current world is one upload performed once. |
| Frustum culling | Roadmap Phase 6 — "Add frustum culling and measure it" | Attacks visible geometry. With one terrain draw and a `100.0` far plane there is nothing to cull; it becomes real the moment the vista does. |
| Background/threaded meshing with cancellation and per-frame budgets | Roadmap Phase 5 — already an item, with ownership, cancellation, stale-result rejection, and per-frame budgets named | Attacks CPU meshing. Needs generation to exist first. |
| Per-chunk edit records, serialization, versioning | Roadmap Phase 5 — already an item | Only relevant once chunks are generated and editable. |
| Per-pass GPU timing, draw/dispatch/upload/memory counters | Roadmap Phase 6 — already an item | The one genuine measurement gap; see below. |
| Distance fog used deliberately to hide the LOD horizon | Visual-plan [Phase 5](../plans/visual-feasibility-before-objective-loop.md#phase-5--add-atmospheric-depth-with-the-simplest-accepted-technique) | That phase already owns replacing the fixed 36–70 blend with a camera/scene-aware candidate. Hiding an LOD horizon is a second reason for work already planned, not a new item. |

### (c) Not applicable to this project

| Recommendation | Why not |
| --- | --- |
| **Multi-draw indirect, shared vertex pooling, GPU-driven indirect draws** | These target **driver and submission overhead this project does not have**. CPU submission p95 is `237800` ns — about 1.4% of the frame budget — with the entire paddock in a single upload and a single draw. There is no submission cost to amortize. The document itself makes the disqualifying point: **MDI does not make geometry free.** It changes who issues the draws, not how much geometry there is; by the eight-lever rule it reduces "OpenGL submissions" and nothing else. Revisit only if a measured submission bottleneck appears, at which point it is Phase 6 work like everything else. |
| **Occlusion culling (hierarchical-Z or query-based)** | Same family, plus a content prerequisite this project fails: occlusion culling pays when large occluders hide large amounts of geometry. A 32×16×32 blockout under a `100.0` far plane has no meaningful occluders and `2754` faces total. It also adds a latency/readback hazard for a saving that cannot currently exist. |
| **Transvoxel** | Targets **smooth density fields** — it exists to stitch LOD transitions in a marching-cubes-class isosurface without cracks. This project is **deliberately blocky**: one-byte material IDs per cell, axis-aligned quads, a 16³ chunk edge chosen by [ADR 0002](../decisions/0002-chunk-edge-length.md), and analytic box collision that matches the visible faces. Adopting Transvoxel would change the art direction and the collision model, which is not an optimization. If blocky LOD needs seam handling later, the applicable literature is chunked-LOD seam stitching, not Transvoxel. |
| **Sparse voxel octrees, GigaVoxels, sparse voxel DAGs** | These are a **renderer redesign, not an optimization**. Each replaces mesh-and-rasterize with ray casting a hierarchical volume, which discards the mesher, the one-upload path, the framebuffer oracles, the per-face voxel diagnostic ledger, and every accepted capture baseline, and couples the renderer to the voxel representation far more tightly than the current façade does. That is a superseding-ADR decision against the [native foundation](../decisions/0001-native-foundation.md), not a Phase 6 candidate. Note also that the sibling note's source explicitly reports success in a *meshing-based* engine rather than a raymarching one. |
| **Infinite generation, per-chunk edit persistence across detail bands, prop identity across bands** | Ruled out in that form by the world-shape decision: Wide Eye is explicitly not an open world, and a bounded surround that is never edited, collided against, simulated, or serialized avoids exactly this class of problem. The parts that survive as *bounded* problems are already Phase 5 items. |
| **A kilometre-scale traversable render distance as a target in itself** | Not a goal. The visual plan's [non-goals](../plans/visual-feasibility-before-objective-loop.md#non-goals) exclude an infinite procedural world, general chunk streaming, and production LOD, and its [stop conditions](../plans/visual-feasibility-before-objective-loop.md#stop-and-ask-decisions) halt the tracer if visual scale requires expanding authoritative collision or world generation. Visible extent is a *scenery* question here, not a traversal question. |

## The time-sensitive item: reversed-Z and camera-relative rendering

**This is a recommendation. It is not a decision taken, and nothing in this note
authorizes the change.** It is singled out because it is the one item in the
document whose cost is a function of *when* it is done.

**The problem.** With a conventional depth setup, precision is concentrated near
the camera twice over: the perspective divide already packs depth values toward
the near plane, and the storage format packs its own resolution toward zero as
well. At kilometre scale — the owner's named 1 to 20 km² of visible surround —
distant surfaces end up separated by less than one representable depth step and
**z-fighting at distance is the expected outcome**, not a bug to be found later.
This is a precision failure, so unlike every other budget it is not relieved by
the scene being small: it arrives with the far plane, not with the workload.

**The fix, as the document gives it.** Clip control set to zero-to-one, a
reversed projection (far maps to 0, near maps to 1), depth cleared to zero, and
a greater-than depth comparison. `glClipControl` is core in OpenGL 4.5 and this
project is pinned to 4.6, so no extension is involved.

**One correction this repository should record before adopting the item.** The
four state changes above are necessary but **not sufficient here**. The default
framebuffer currently requests and receives a **24-bit fixed-point** depth
buffer (`SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` in
[`window_runtime.cpp`](../../src/platform/window_runtime.cpp); the Phase 0
packet records `depth_bits: 24`). Reversed-Z earns its precision from the
interaction between the reversed mapping and a *floating-point* depth format;
on a uniform fixed-point buffer, reversing the mapping buys close to nothing.
The full change therefore also needs a float depth attachment, which for the
default framebuffer means either a different requested format or rendering the
scene into an offscreen target. **Inference**, from the code and the packet —
not measured, and not a reason to skip the item. It is a reason to size it
honestly.

**Why it is cheap now.** Near plane, far plane, and focal length were just
consolidated into one validated `SceneRenderSettings` record with a named
rejection reason per invalid field, and every scene program is composed from a
preamble generated out of it. That is the single seam a depth-convention change
has to pass through, and it exists today. Nothing yet depends on a long draw
distance: `far_plane` is `100.0`, there is no LOD horizon, no streaming band,
and no distant scenery whose fade was tuned against the current depth curve.

**Why it is expensive later.** Retrofitting invalidates captures and oracles
tuned against the old depth behaviour, and this project has already started
accumulating them:

- Three framebuffer oracles in
  [`scenario_runner.cpp`](../../src/platform/scenario_runner.cpp) assert
  `depth_function_less` as an accepted pass condition. Reversed-Z sets
  `GL_GREATER` and **falsifies all three by construction**.
- The Phase 0 packet records `paddock_center_depth: 0.992984` and
  `depth_function_less: yes` in its accepted measurement stream. Under reversed-Z
  the first becomes a near-zero value and the second becomes `no`.
- The shadow pass writes a `GL_DEPTH_COMPONENT24` texture and two programs
  sample it, so the convention change has to cross the shadow seam as well.

Every one of those is small **today**. Each visual-tracer phase that lands
before the change adds more accepted captures, more thresholds, and more
same-state comparisons that would all have to be regenerated and re-accepted by
the owner. This is the classic case where the cheapest moment is the earliest
one.

**On the camera-relative half.** It is the weaker of the two at this project's
named scale, and this note says so rather than bundling them. Float32 spacing at
4.5 km is about half a millimetre, so absolute-coordinate
vertex jitter is not the visible failure at 1 to 20 km²; depth is. This project
is also already partly protected: world voxel coordinates are signed 64-bit with
floor division, so the *data* is precision-safe and only the render-time
conversion convention is open. The two are named together because they are
decided together — the render-time convention (rebase positions on the camera
before converting to float) is nearly free while there is one scene-settings
seam and one camera path, and it is the same retrofit hazard afterwards.

**What is being recommended, exactly:** that the owner decide the depth
convention **before** visual-plan Phase 1 lands distant scenery, rather than
after. Not that it be adopted here, and not that it be adopted without its own
coherent outcome, measurement, and re-accepted captures.

## The far proxy suits this project unusually well

The document presents the 2.5D height/column far proxy — representing distant
terrain as a heightfield or column set rather than as full voxel volume — as a
general LOD technique with a general cost: it **cannot represent caves,
overhangs, or any concavity**, so distant terrain loses those features.

For this project that cost is **zero**, and the reason is a design decision
rather than a graphics argument. The owner's world shape makes the surround
**unreachable and non-authoritative**: it is never walked on, never collided
against, never edited, never simulated, and never serialized. A cave the player
can never enter and never see the inside of is not a feature that was lost. The
usual objection to the technique is an objection about *traversable* distant
terrain, and this project has decided it will not have any.

**Inference, not a measurement, and not an adoption.** It is recorded because it
changes how the technique should be *evaluated* when Phase 6 reaches LOD — the
standard trade-off table does not apply unchanged here — and because it should
inform how visual-plan Phase 1 authors its bounded distant scenery, which is the
first place a far-proxy shape could be chosen or accidentally ruled out.

## Where this project is already ahead of the document

The document's "Stage 0" asks the reader to establish a reproducible benchmark
before optimizing anything, and names four things to record: **per-pass GPU
time, CPU submission, resident memory, and worst frame time.**

The Phase 0 baseline packet, produced on 2026-08-22, already records three of
those four, and more than the document asks for around them:

| Stage 0 asks for | Packet records |
| --- | --- |
| CPU submission | min / median / p95 / p99 / max — `108600` / `174300` / `237800` / `299200` / `410700` ns |
| Resident memory | `process_peak_rss_bytes` `76713984`, against a named `1610612736` byte budget |
| Worst frame time | `gpu_render_maximum_ns` `221472`, `cpu_submission_maximum_ns` `410700`, `synchronized_frame_maximum_ns` `7147700` |
| Per-pass GPU time | **Partly.** A single whole-frame GPU timer (`gpu_render_*`), not a per-pass breakdown. |

Beyond the checklist, the packet also fixes the deterministic scene, seed, tick,
route, representative *and* holdout cameras, viewport, profile, GL
vendor/renderer/driver, high-severity message count, and same-state capture
repeat identity — and pins them with source hashes, a manifest, and an
independent manifest validator. Most published "benchmark first" advice does not
reach holdout cameras or repeat identity.

**The one real gap is per-pass GPU timing**, and it is already an unchecked
Phase 6 item ("Record per-pass GPU timing and relevant draw, dispatch, upload,
and memory values when tied to a named budget or optimization decision"), gated
behind the rule that a measurement-only dashboard with no decision attached is
rejected. So the gap is known, owned, and deliberately not yet filled. Nothing
in this document changes that ordering.

## Relationship to the sibling note

[`external-voxel-render-distance-input.md`](external-voxel-render-distance-input.md)
records a Reddit account of raising render distance in an author-described
"Micro Voxel Engine". The two inputs overlap in one important place, and the
overlap is worth stating so a later reader does not treat them as two
independent confirmations.

- **The sibling note's "sieve" (resolution-divisor generation) and this
  document's coarse-LOD hierarchy are the same family.** Both say: distant
  terrain must be *produced* at lower detail rather than produced at full detail
  and then reduced. The sibling note gives the generation-side form; this
  document gives the render-side form and the surrounding taxonomy.
- **The generation-side requirement is already an accepted Phase 5 constraint.**
  The owner decided on 2026-08-22 that every Phase 5 generation function takes a
  resolution divisor from the outset, so any 1/N resolution is generated
  natively rather than downsampled — recorded at the head of
  [Roadmap Phase 5](../../ROADMAP.md#phase-5--tracer-4-procedural-voxel-world).
  This document does not reopen that; it supplies an independent-sounding but
  **not independent** argument for the same idea, and should not be counted as
  corroboration.
- **The render-side half remains gated.** LOD bands, transitions, streaming, and
  eviction stay in Phase 6 behind a measured constraint, exactly as the sibling
  note left them.
- **Neither source is verified.** The sibling note's figures come from a
  non-retrievable post; this document's come from unfetched links. Two unverified
  sources agreeing is not evidence.

## Source index — as supplied

The document's condensed index, transcribed verbatim from the owner-pasted text
on **2026-08-22**. **No URL below was fetched or checked from this repository**,
so each is an unverified pointer, not evidence, and any of them may have moved.
An earlier draft of this note carried a *reconstructed* index written from
memory instead; it was replaced because several reconstructed entries were
plausible and wrong — a wrong article slug, the wrong author for the depth
article, and one reference the document never cited. Reconstruction from memory
is not a substitute for transcription.

### Must-read

- 0 FPS, meshing: <https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/>
- 0 FPS, meshing part 2: <https://0fps.net/2012/07/07/meshing-minecraft-part-2/>
- 0 FPS, blocky voxel LOD: <https://0fps.net/2018/03/03/a-level-of-detail-method-for-blocky-voxels/>
- POP buffer project: <https://x3dom.org/pop/> — paper <https://x3dom.org/pop/files/popbuffer2013.pdf>
- Binary greedy meshing: <https://github.com/cgerikj/binary-greedy-meshing>
- Geometry clipmaps: <https://hhoppe.com/proj/geomclipmap/> — paper <https://hhoppe.com/geomclipmap.pdf>
- GPU-based geometry clipmaps: <https://hhoppe.com/proj/gpugcm/> — paper <https://hhoppe.com/gpugcm.pdf>
- CDLOD: <https://github.com/fstrugar/CDLOD>
- Distant Horizons: <https://gitlab.com/distant-horizons-team/distant-horizons>
- Transvoxel: <https://transvoxel.org/>
- OpenGL multi-draw indirect: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_multi_draw_indirect.txt>
- OpenGL buffer object streaming: <https://wikis.khronos.org/opengl/Buffer_Object_Streaming>
- Visualizing depth precision (NVIDIA): <https://developer.nvidia.com/blog/visualizing-depth-precision/>
- OpenGL clip control: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_clip_control.txt>

### Practical implementation and case studies

- Vercidium, voxel world optimisations: <https://vercidium.com/blog/voxel-world-optimisations/>
- Vercidium, further optimisations: <https://vercidium.com/blog/further-voxel-world-optimisations/>
- Vercidium source: <https://github.com/Vercidium/voxel-mesh-generation>
- Vertex pooling: <https://nickmcd.me/2021/04/04/high-performance-voxel-engine/>
- Voxel Tools, smooth terrain: <https://voxel-tools.readthedocs.io/en/latest/smooth_terrain/>
- Voxel Tools, performance: <https://voxel-tools.readthedocs.io/en/latest/performance/>
- PolyVox overview and paging: <https://www.volumesoffun.com/polyvox-about/index.html>

### OpenGL submission and profiling

- Shader draw parameters: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_shader_draw_parameters.txt>
- Buffer storage: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_buffer_storage.txt>
- Indirect parameters: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_indirect_parameters.txt>
- Timer query: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_timer_query.txt>
- Pipeline statistics query: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_pipeline_statistics_query.txt>
- Occlusion queries made useful: <https://developer.nvidia.com/gpugems/gpugems2/part-i-geometric-complexity/chapter-6-hardware-occlusion-queries-made-useful>
- Efficient occlusion culling: <https://developer.nvidia.com/gpugems/gpugems/part-v-performance-and-practicalities/chapter-29-efficient-occlusion-culling>

### Large worlds and advanced voxel rendering

- Reversed-Z in OpenGL: <https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/>
- Rendering in camera space: <https://pharr.org/matt/blog/2018/03/02/rendering-in-camera-space.html>
- Unity camera-relative rendering: <https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition%408.2/manual/Camera-Relative-Rendering.html>
- Godot large world coordinates: <https://docs.godotengine.org/en/stable/tutorials/physics/large_world_coordinates.html>
- Efficient sparse voxel octrees: <https://research.nvidia.com/publication/2010-02_efficient-sparse-voxel-octrees>
- SVO analysis and implementation report: <https://research.nvidia.com/publication/2010-02_efficient-sparse-voxel-octrees-analysis-extensions-and-implementation>
- GigaVoxels: <https://www-sop.inria.fr/reves/Basilic/2009/CNLE09/> — paper <https://inria.hal.science/inria-00345899/file/CNLE09.pdf>
- High resolution sparse voxel DAGs: <https://research.chalmers.se/publication/240766>

## What this note does not do

It adopts nothing, approves nothing, schedules nothing, ticks no box, adds no
roadmap item, and adds no dependency. It restates none of the document's
performance figures as a measurement of this project — the document's numbers
describe other people's engines on other people's hardware, and none of them was
reproduced here.

The one item that wants an owner decision on a clock is the depth convention
(reversed-Z, with the float-depth-attachment correction above, and the
camera-relative render convention decided alongside it), **before** visual-plan
Phase 1 lands distant scenery and starts multiplying the captures a later
retrofit would invalidate. Recording that recommendation here is deliberately
not the same as approving it.
