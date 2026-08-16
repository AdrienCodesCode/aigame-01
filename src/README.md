# Source boundaries

Phase 1 begins with only the ownership boundaries required by the accepted
architecture. Add APIs inside a boundary when a tracer needs them, not to fill
out a speculative engine framework.

- `core`: platform-independent monotonic time, fixed-step scheduling, logging,
  assertions, duration statistics, and process-memory observations.
- `platform`: process entry, SDL lifecycle, native windowing, and translation of
  physical input events into named actions.
- `render`: OpenGL resources and presentation of immutable render state.
- `voxel`: voxel storage, coordinate conversion, generation, and meshing.
- `game`: authoritative fixed-tick game rules and render snapshots.

The executable entry point starts in `platform` and maps CLI arguments to named
scenario runners. `window_runtime` owns SDL initialization, window and OpenGL
context lifetime, event polling, presentation, and shutdown. Its window-state
reducer owns drawable resize, minimize/restore, focus, and close transitions.
`scenario_runner` owns scenario configuration, render-resource lifetime,
framebuffer-oracle decisions, and the caller-supplied capture path without
receiving the raw SDL window. After making a context current, `window_runtime`
initializes the checksum-verified generated OpenGL 4.6 Core loader through SDL.
The platform-owned `NamedInputState` combines keyboard and gamepad sources,
accumulates relative mouse look separately from held look rates, preserves
rising presses and mouse deltas until one fixed tick consumes them, applies a
tested stick dead zone, and clears the appropriate state on focus loss or
disconnect. `window_runtime` owns relative-mouse capture and passes the fixed
accumulator's interpolation alpha into scenario presentation; game rules never
receive SDL scancodes, buttons, axes, windows, or event structures. The `render`
boundary's `OpenGlRenderer` façade owns the Phase 1 triangle and
perspective voxel-cube resources and draw paths, the Tracer 1 paddock's checked
indexed upload and opaque draw path, camera-parameterized paddock/debug draws, an
optional placeholder-dog draw, five static-mesh procedural sheep-proxy draws,
and top-left RGBA8 readback; it does not own
another entry-point table or authoritative camera/dog state. The boundary also
supplies the color/depth oracle helpers and deterministic PNG encoding used by
the scenario runner. The
`voxel` boundary owns distinct signed 64-bit world-voxel, chunk, and local-voxel
coordinate types. Conversion uses floor division so negative world cells
produce non-negative local offsets, accepts a caller-supplied positive cubic
edge length, and rejects invalid locals or recomposition overflow. No storage
constant is embedded in the coordinate module. The initial production storage
edge is 16 cells per axis, selected by
[`ADR 0002`](../docs/decisions/0002-chunk-edge-length.md) after an isolated
16³/32³ memory and rebuild-proxy comparison. Each production `Chunk` stores
4,096 one-byte material IDs, reserves ID zero for empty space, rejects invalid
local reads and writes, and conservatively tracks an inclusive local-space dirty
region across actual edits. The naive CPU mesher emits one duplicated four-
vertex/two-triangle quad for each non-empty cell face whose neighbor is empty,
with outward winding, a cardinal normal, and the material ID. It samples a
caller-owned read-only snapshot of the six axial chunks, treating a missing
neighbor as empty. A caller-owned material table defaults every ID to opaque and
can classify IDs as cutout or translucent; emitted faces are routed into three
independent CPU vertex/index buffers. Classification does not change the
baseline rule that every non-empty material occludes every other non-empty
material. A two-pass build counts and classifies faces before allocation,
enforces the fixed 16³ conservative ceiling and caller-supplied aggregate
vertex/index limits, reserves exact per-pass storage, and returns a checked
error without partial output when a count cannot be represented or a limit
would be exceeded. `handcrafted_paddock` stores its 32×16×32 bounded blockout in
four production chunks, supplies live axial neighbors to each checked mesh, and
offsets complete per-chunk outputs into one world-space upload. Its six opaque
material IDs distinguish grass, stone, the red gate, barn walls, roof, and door;
a bounded voxel-owned palette maps those IDs to render colors and records
independent per-material face counts. The renderer preserves the fixed camera
while owning the fixed directional light, deliberate sky and distance fog, and
a static filtered shadow map. It also owns same-camera GPU diagnostics for all
four chunk bounds, one normal segment per emitted face, the actual indexed
wireframe, and a mesh-stat chart. `scenario_runner` selects those views, applies
their broad framebuffer oracles, and logs the exact chunk, occupied-block, face,
vertex, index, and per-material counts. A caller-requested voxel diagnostic
ledger uses the mesher's traversal and neighbor sampling to describe every side
of every non-empty cell with source local/material/direction, wrapped neighbor
local/material, same/adjacent/missing-chunk provenance, and emitted/culled
disposition. The paddock associates each record with its source chunk, and its
unit oracle proves unique occupied-side coverage plus a one-to-one mapping from
every emitted decision to the actual world-space quad. `scenario_runner` rejects
inconsistent aggregate counts before rendering and logs the emitted, culled,
and neighbor-provenance totals. Ordinary per-chunk mesh builds do not retain the
full ledger. A future world/rebuild boundary owns
neighbor lifetime and must remesh the changed chunk plus each affected axial
neighbor after a border edit, chunk load, or chunk unload; `Chunk` and the
mesher do not mutate invalidation state. The current one-time opaque upload does
not implement rebuild queues, streaming, cutout/translucent submission, or
dynamic shadow updates.
The `game` boundary owns the authoritative `GameplaySimulation`, Tracer 1 dog,
and camera behavior. The core `FixedStepAccumulator` is the only
render-to-simulation scheduler. `GameplaySimulation` consumes exactly one
domain input per fixed tick without receiving render-frame timing, owns the
existing dog controller, and publishes read-only previous/current snapshots of
the dog plus five authoritative sheep. Sheep IDs 1–5 and hot kinematic,
arousal, behavior, and grounded fields live in one fixed contiguous buffer.
Every tick derives the next sheep buffer from the immutable prior buffer. The
default no-behavior baseline preserves those records unchanged. The named
`presentation-motion` fixture moves the same five records synchronously around
a scripted square while retaining settled behavior and zero arousal; it is
presentation evidence, not accepted flock behavior. Presentation may
interpolate published copies but cannot mutate game truth. The render
boundary copies each published sheep ID, position, and heading into a fixed
renderer-facing pose buffer, then submits one shared procedural proxy mesh for
each entry; it retains no authoritative identity or transform state. Independent
version 1 seed, action-input, and replay contracts bind the named
scenario/version/seed, 60 Hz rate, and one contiguous domain action per tick.
The version 2 state dump includes both dog and sheep prior/current snapshots.
Compatibility validation completes before replay mutation, and canonical
compact JSON writers expose replay plus published state. The bounded
presentation capture path can write the latter beside a frame; file decoding
and general replay/seed CLI integration remain tooling work. A pure fixed-size
observable pass reads the published five-sheep buffer and explicit connectivity/
chosen-neighbor inputs to compute centroid, ground-plane radius, polarization,
elongation, group speed, nearest-neighbor spacing, connected components, and
neighbor-count summaries without mutating simulation state or selecting social
neighbors. A fixed-capacity ground-plane spatial grid copies published sheep ID
and position into deterministic sorted cell/row ranges. Caller-owned output
spans bound nearest-neighbor selection, with exact-distance filtering and stable
distance/ID/source-index ordering; rebuild and query use no heap allocation.
The 1,000-member fixed ceiling supports the approved capacity experiment but is
not evidence that the tier meets a performance budget or belongs in the game.
The grid is not yet connected to social forces. Dog-relative and response-timing
observables remain deferred until their required behavior scenarios exist. The
dog is a kinematic upright cylinder whose world-space planar motor
bounds vector acceleration/deceleration, rotates facing toward movement by the
shortest path, and slows during large heading changes. It retains predictable
analytic ground contact and collision shapes for the paddock edges,
representative wall, and gate that are independent of voxel faces and render
meshes. Version 1,
seed-zero `paddock-start`, `presentation-motion`, `wall-contact`, `closed-gate`, and `open-gate`
definitions provide deterministic initialization and exact restart. The
gameplay camera owns orbit yaw/pitch independently of dog facing; orchestration
applies look first, resolves camera-yaw-relative movement on the ground plane,
then advances the dog. The free-debug camera retains independent position/look
state. Previous/current dog snapshots and camera states produce one coherent
interpolated render view without feeding presentation back into simulation. The
owner accepted this keyboard/mouse control baseline on 2026-08-16 and deferred
refinement. It still has no animation, camera obstruction, sheep behavior or
final sheep art, final tuning, or physically verified controller feel.
