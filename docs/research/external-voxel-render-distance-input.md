# External input: macro-chunk LOD for voxel render distance

**Status:** Research note. Records an external, unverified account and maps its
ideas to this project's phases. Adopts nothing by itself.

## Source and provenance

An `r/VoxelGameDev` post describing three weeks of work raising render distance
in an author-described "Micro Voxel Engine", supplied to this repository by the
owner on **2026-08-22** as pasted text.

The original URL (`https://www.reddit.com/r/VoxelGameDev/s/LvkPKX1J9z`) was
**not retrievable** from this environment — Reddit fetches are blocked here and
the `/s/` share id did not resolve through search. The post title, author,
date, and comment thread were therefore never observed. What is recorded below
is the owner-pasted body only. Per [`AGENTS.md`](../../AGENTS.md), a link alone
is not a reproducible input, and this note is not one either: it is a
second-hand record of a claim.

## What the author claims

**Unverified claim.** Render distance raised from 300 m to roughly 10–15 km at
45–50 FPS on an Apple M1 Pro, in a meshing-based engine rather than a
raymarching/DDA one.

**Unverified claim.** Transient LOD transitions — detail levels fading between
each other once, rather than a continuous gradual blend — are "around 30%
cheaper on the GPU" and look "nearly" as smooth in most scenarios.

No methodology, scene description, frame-time percentiles, resolution, chunk
counts, or memory figures accompany either number. Neither is reproducible from
this repository, and an Apple M1 Pro is not comparable to this project's
recorded reference hardware.

### The four described techniques

1. **Macro chunks with a sieve function.** Every chunk generation function
   takes a resolution divisor, so a 1/N chunk is *generated natively* at that
   resolution rather than downsampled from a full-resolution copy. Generated
   features and stamps are included. The author reports a 1/64 chunk costing
   about the same to generate as a full-resolution one while covering far more
   ground. Macro chunks record and resolve local edits independently and are
   saved and cached independently, so terrain edits survive without keeping the
   full-resolution copy resident.
2. **Transient LOD transitions** rather than continuous ones (see the claim
   above).
3. **Adaptive fog.** Effective draw distance scales dynamically with the bands
   actually loaded. Bands generate high resolution first, so during fast motion
   draw distance is temporarily pulled in until chunks arrive.
4. **Macro chunk cards and props.** Prop IDs are deterministic from position
   and type, so a destroyed or newly spawned entity reconciles against the
   macro-chunk set without instantiating millions of entities. Grass and
   foliage deliberately do not map one-to-one with loaded props; they follow the
   same generation pattern, which the author accepts as a known disparity in
   exchange for not tracking millions of items.

## Why most of it does not apply yet

**Observed result (2026-08-22, reference desktop, Phase 0 baseline packet
`artifacts/phase3/2026-08-22/visual-feasibility-baseline-183850545/`):** GPU
render p95 `114304` ns against a `16670000` ns frame budget, CPU submission p95
`237800` ns, peak RSS `76713984` bytes against a `1610612736` byte budget. The
world drawn is the handcrafted paddock: 4 chunks, 1,746 occupied blocks, 2,754
emitted faces.

**Inference.** Macro-chunk LOD bands, streaming, eviction, and transition
blending exist to relieve draw-call and memory pressure across a large editable
world. This project has neither the world nor the pressure. Adopting them now
would be optimizing an absent bottleneck, and [Phase 6](../../ROADMAP.md)
already gates each one — frustum culling, mesh caching, greedy meshing,
streaming distance and eviction, LOD — behind a *measured* constraint, with
low/high capture baselines required before optimizing at all.

**The goals also differ.** The post targets a traversable 10–15 km view. The
approved [visual-feasibility plan](../plans/visual-feasibility-before-objective-loop.md)
targets a bounded pastoral backdrop whose distant scenery must be explicitly
unreachable and non-authoritative, under a stop condition that halts the tracer
if visual scale requires expanding authoritative collision or world generation.
The post's headline number is therefore not a benchmark this project is failing.

## What is worth carrying, and where

| Idea | Where it lands | Why |
| --- | --- | --- |
| Fog end derived from effective draw distance rather than fixed independently | Plan Phase 1 | Directly relevant now. Today `smoothstep(36.0, 70.0, camera_distance)` is a fixed pair while `far_plane` is `100.0`, both repeated as shader literals. If Phase 1 extends the vista without moving fog with it, new distant scenery blends to sky before it is visible. |
| Resolution-parameterized generation (the "sieve") | Phase 5, as a design constraint adopted before the code exists | **The highest-value item.** Phase 5 writes terrain generation from scratch. If those functions take a resolution divisor from the first line, Phase 6 LOD becomes nearly free and no separate downsampling path can silently disagree with the full-resolution one. Costs nothing to adopt early; expensive to retrofit. |
| Prop identity deterministic from position and type | Phase 5 | Fits this project's existing grain: the simulation contains no randomness, the scenario seed is consumed by no rule, and Phase 5 must preserve every handcrafted herding replay as a regression scenario. Deterministic placement keeps that possible. |
| Independent per-chunk edit records, saved and cached separately | Phase 5 | Relevant to the existing "chunk serialization and migration/version behavior" item, not before it. |
| Transient LOD transitions; band streaming; eviction | Phase 6 only | Gated on a measured constraint that does not exist. The 30% figure is unverified and unmethodical. |
| A 10–15 km draw distance | Not a goal | Out of scope for a bounded paddock herding first playable. |

## What this note does not do

It adopts nothing, changes no phase, ticks no box, and adds no dependency. The
Phase 5 sieve recommendation is the one item worth an owner decision before
Phase 5 begins; recording it here is deliberately not the same as approving it.
