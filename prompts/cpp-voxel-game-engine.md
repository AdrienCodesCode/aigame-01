# Task: build a custom C++ voxel engine through playable tracers

## Use only when

Building a native engine is an explicit project goal. If the primary goal is to
validate a game loop quickly, use an existing engine or the Three.js architecture
prompt instead.

Before using this prompt, read the current checkpoint in
[`ROADMAP.md`](../ROADMAP.md), the accepted
[`DEVELOPMENT_WORKFLOW.md`](../docs/DEVELOPMENT_WORKFLOW.md), and the
tool/adoption gates in
[`AGENT_HARNESS_AND_TOOLS.md`](../docs/AGENT_HARNESS_AND_TOOLS.md). Implement only
the first approved incomplete tracer; update the roadmap with evidence afterward.
When the tracer changes sheep behavior or population scale, also read the
[research synthesis](../docs/research/herding-simulation-and-scale.md) and
[implementation plan](../docs/plans/herding-simulation-and-scale.md).

## Inputs

- Game design and primary playtest question: [PATH]
- Target operating systems, CPU/GPU classes, and input devices: [TARGETS]
- C++ standard, compiler set, and build tools: [TOOLCHAIN]
- Approved platform, test, debug, and asset dependencies: [DEPENDENCIES]
- Frame-time, memory, startup, executable-size, and load budgets: [BUDGETS]
- Current tracer and explicit out-of-scope list: [SCOPE]
- Time, token, and human-review budget for this run: [RUN BUDGET]

## Mission

Build the smallest clean-room native engine increment that makes the approved
game tracer playable and measurable. The engine owns its game simulation, voxel
representation, meshing, rendering, resource lifetime, and debug evidence. It
may use the approved narrow platform dependencies; “no libraries” is not itself
a quality goal.

Do not reproduce a showcase engine's feature list. Every subsystem must retire a
named risk or enable a player-visible part of the current tracer.

## Inspect first

Read repository instructions and design records. Inventory the build, platform,
renderer, simulation, voxel, asset, input, audio, test, and tooling code. Build
and run the current executable. Record compiler, configuration, hardware,
warnings, tests, frame-time percentiles, memory, and known failures.

State whether this is a new tracer or a modification. Do not replace working
systems merely to match this prompt.

## Required boundaries

- Run authoritative game rules at a fixed tick independent of render cadence.
- Pass immutable render snapshots to presentation; the renderer does not own
  gameplay state.
- Keep voxel storage/generation, meshing, graphics, game rules, platform code,
  and tools in explicit modules.
- Use integer chunk/world coordinates and document coordinate conversions.
- Use deterministic seeds and recordable input for reproducible simulation.
- Keep detailed visual meshes separate from simple gameplay collision.
- Make GPU and native resource ownership explicit and automatically released.
- Put every worker task behind cancellation and lifetime rules.
- Keep dependencies small, licensed, pinned, and recorded.
- Avoid a general ECS, custom allocator, frame graph, job system, asset database,
  or scripting language until observed pressure justifies it.

## Tracer sequence

Implement only the next incomplete tracer:

1. **Foundation:** reproducible build, window, shutdown, triangle, voxel cube,
   logs, tests, sanitizers, and frame timing.
2. **Paddock:** bounded voxel field, camera, simple light, dog movement, analytic
   collision, debug overlays, and one gate.
3. **Herding:** five placeholder sheep, dog-pressure field, three temperaments,
   fixed-tick neighbors, restart, success/failure, seed and input replay.
4. **Readable style:** one representative articulated dog and sheep, locomotion
   and state cues, whistle, minimal HUD/audio, and fresh-player test.
5. **World:** deterministic terrain, chunk persistence, budgeted worker generation
   and meshing, then a bounded streaming horizon.
6. **Measured scale:** culling, caching, LOD, and profiles added only in response
   to captured bottlenecks; authoritative population benchmarks at 5, 14, 25,
   100, 250, 500, and 1,000 remain capacity evidence until separately approved
   as gameplay.
7. **Hardening:** packaging, licenses, settings, input remapping, accessibility,
   save recovery, crash handling, and regression suite.

Do not implement a later tracer while an earlier acceptance gate is failing.

## Voxel implementation order

Begin with a correct naive face mesher and a small fixed chunk representation.
Test boundaries, negative coordinates, edits, empty/full chunks, and deterministic
generation. Add greedy meshing, batching, worker threads, streaming, and LOD one
at a time, retaining a known-good fallback and measuring each change.

Keep opaque, cutout, and translucent geometry distinct. Budget mesh generation,
GPU uploads, and chunk activation per frame. Never let an obsolete background
job publish into a destroyed or recycled chunk.

## Herding implementation order

Use stable IDs, contiguous hot state, and synchronous updates from an immutable
prior snapshot. Implement named, bounded influences in order: close-range
repulsion, attraction to selected local neighbors, optional alignment with a
smaller selected subset, dog stimulus, obstacle/drop avoidance, and explicit
settled/alert/driven/recovering transitions. Treat arousal/recovery as a designed
proxy, not a physiological claim. Use a deterministic uniform spatial grid and
avoid steady-state per-agent allocation.

Expose forces, selected neighbors, arousal, target, speed, state, and group
observables through state dumps and debug views. Compare alignment enabled and
disabled rather than assuming generic boids are realistic. Avoid random fixes
for unstable behavior.

The current acceptance question is behavioral, so basic lighting and primitive
animals are sufficient until the flock is explainable and steerable.

## Evidence loop

Follow the authoritative development workflow, including its coherent-outcome
contract, proportional verification cadence, failure bundle, baseline-approval
rule, and context recommendation. For the owned tracer increment:

1. Declare files, dependencies, hypothesis, budgets, and rollback point.
2. Capture the current reproducible state.
3. Make the smallest coherent change.
4. Compile with warnings enabled and run relevant unit/integration tests.
5. Run the executable and capture the actual state in motion.
6. Measure frame time, memory, startup, and subsystem-specific cost.
7. Replay at least one recorded behavior case when simulation changes.
8. Review for lifetime, bounds, overflow, thread, GL-state, and shutdown faults.
9. Accept, revise, or revert; record why.

## Gate

The increment is complete only when its declared behavior works in the running
build, tests and sanitizers relevant to the change pass, budgets pass on named
hardware, debug evidence explains the result, and no later-tracer system was
smuggled into scope. Stop at the run budget and report a narrow blocker instead
of generating speculative architecture.

## Deliverables

- Buildable implementation for one tracer.
- Updated architecture and decision record.
- Commands and environments actually verified.
- Runtime captures, measurements, and replay evidence.
- Dependency/license changes.
- Accepted, reverted, deferred, and untested work.
- The single next highest-risk tracer step.
- The explicit fresh-chat, continue-chat, or compact-then-continue recommendation
  required at a coherent-outcome boundary.
