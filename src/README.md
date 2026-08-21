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
reducer owns drawable resize, minimize/restore, focus, close, and pointer-capture
transitions. It separates the player's capture intent from the capture the
window should actually hold, so an unfocused window always releases the pointer
while regaining focus restores only a capture the player still wants.
`scenario_runner` owns scenario configuration, render-resource lifetime,
framebuffer-oracle decisions, and the caller-supplied capture path without
receiving the raw SDL window. After making a context current, `window_runtime`
initializes the checksum-verified generated OpenGL 4.6 Core loader through SDL.
The platform-owned `NamedInputState` combines keyboard and gamepad sources,
accumulates relative mouse look separately from held look rates, preserves
rising presses and mouse deltas until one fixed tick consumes them, applies a
tested stick dead zone, and clears the appropriate state on focus loss or
disconnect. A window-lifetime press such as the Escape pointer-capture toggle is
read once through `consume_press` by its owner rather than waiting for a fixed
tick that may not run in a given frame. `window_runtime` owns relative-mouse
capture: one reconcile step per frame is the only caller of SDL's relative-mouse
mode, driven by the window-state decision above, and mouse motion is translated
into look only while the pointer is actually captured. It passes the fixed
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
The `game` boundary owns a minimal shared `Vec3` value, whole-game scenario
definitions, the authoritative `GameplaySimulation`, Tracer 1 dog, and camera
behavior. Generic math, sheep state, and scenario identity therefore do not
depend on a particular controller. A `GameplayScenarioDefinition` owns the
version, seed, dog configuration, initial sheep buffer, sheep behavior
configuration, and future objective fixture; the dog motor receives only its
initial state plus the scenario's gate flag. **No rule reads the seed.** Every
accepted rule is a function of the immutable prior state, so the simulation
contains no randomness at all and the seed is carried by the contract without
being consumed; `wide_eye.gameplay_simulation` pins that as an equality — the
same scenario under three different seeds publishes one byte-identical sequence
— so the first rule that starts consuming it fails an existing check instead of
quietly making a scenario irreproducible. The analytic paddock shapes, their
`PaddockObstacle` identities, and the collision/sight-line queries over them live
in a neutral `paddock_collision` boundary, so sheep rules never depend on a
dog-named header and dog collision, sheep collision, occlusion, and the
steering-level look-ahead cannot describe four different walls. These boundaries are recorded in
[`ADR 0004`](../docs/decisions/0004-gameplay-scenario-ownership.md) and
[`ADR 0005`](../docs/decisions/0005-paddock-collision-ownership.md).
The core `FixedStepAccumulator` is the only
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
each entry; it retains no authoritative identity or transform state. The
influence debug view in `render/influence_debug_view` builds its line segments
from the same published snapshot on the same terms: it reads the per-term
acceleration vectors, chosen-neighbor IDs, arousal, behavior, heading target,
and the combined-bound record, holds no authoritative state, and writes into a
caller-owned bounded segment buffer so a debug frame cannot allocate or grow
without bound. Its arrow length is a stated scale rather than a raw
acceleration, and it is reachable only through its own strict argv shapes. Independent
version 1 seed, action-input, and replay contracts bind the named
scenario/version/seed, 60 Hz rate, and one contiguous domain action per tick.
The versioned state dump — its current version number is owned by the
[format contract](../docs/formats/GAMEPLAY_REPLAY_AND_STATE.md) — includes dog
and sheep prior/current snapshots plus per-sheep attraction/alignment neighbor
selection, separated social influences, and prior-state dog distance, relative
bearing, approach speed, facing alignment, line-of-sight blocking with its named
paddock occluder, separated pressure/approach/facing evidence, the
combined-influence bound's pre-bound summed magnitude, applied scale, and applied
acceleration, the motion limits' integrated speed, applied speed scale,
applied speed, motion direction, and applied heading change, and avoidance's
named obstacle ahead, its distance, whether a drop lies ahead, the
acceleration the term produced, and the dimensionless arousal stimulus that same
dog geometry produces.
Compatibility validation completes before replay mutation, and canonical
compact JSON writers expose replay plus published state. The bounded
presentation capture path can write the latter beside a frame; file decoding
and general replay/seed CLI integration remain tooling work. A pure fixed-size
observable pass reads the published five-sheep buffer and explicit connectivity/
chosen-neighbor/dog-position inputs to compute centroid, ground-plane radius,
polarization, elongation, group speed, nearest-neighbor spacing, connected
components, neighbor-count summaries, and the flock-level dog geometry —
centroid distance, the world bearing of the dog from the centroid, and the
nearest and rear-most members with their distances — without mutating simulation
state or selecting social neighbors. The temporal measurements that pass cannot
make are a second, equally pure fold: `advance_flock_response_timing` takes the
previous timing record and returns the next one from one published snapshot, so
the caller owns the elapsed-tick state as a plain comparable value rather than a
tracker holding a clock. It measures response latency against the accepted
"is a cause acting" stimulus test and the `alert`/`driven` labels, split and
rejoin time against the connected-component count, and settle time from the last
release to a wholly `settled` flock. A fixed-capacity ground-plane spatial grid copies published sheep ID
and position into deterministic sorted cell/row ranges. Caller-owned output
spans bound nearest-neighbor selection, with exact-distance filtering and stable
distance/ID/source-index ordering; rebuild and query use no heap allocation.
The 1,000-member fixed ceiling supports the approved capacity experiment but is
not evidence that the tier meets a performance budget or belongs in the game.
The named `sheep-only-separation` fixture rebuilds this grid from the immutable
prior buffer and applies a linear close-range repulsion capped by its
scenario-owned maximum acceleration. Exact overlaps recover along an
antisymmetric stable-ID direction, and every next sheep state publishes
synchronously. The independent `sheep-only-attraction` fixture uses the same
prior/grid path to select at most two nearest sheep, pulls toward their prior
centroid, and publishes exact selected IDs, in-radius candidate count, and
separate attraction/separation acceleration vectors. Paired
`sheep-alignment-off` and `sheep-alignment-on` fixtures share the same moving
five-sheep start; only the on case selects one nearest prior-snapshot velocity
and applies a response-time-scaled, capped alignment vector. Paired
`sheep-dog-pressure-off` and `sheep-dog-pressure-on` fixtures publish identical
prior-state dog distance/bearing geometry; only the on case applies a linear,
radius-bounded vector directly away from the dog. Paired
`sheep-dog-approach-off` and `sheep-dog-approach-on` fixtures keep that accepted
distance-only pressure identical and publish the same prior-state dog approach
speed, the component of prior dog velocity along the dog-to-sheep direction;
only the on case adds a separate away-from-dog vector that responds to a closing
dog, shares the pressure radius and linear falloff, and saturates at a
scenario-owned reference speed. Paired `sheep-dog-facing-off` and
`sheep-dog-facing-on` fixtures keep that accepted distance-only pressure
identical, use one stationary dog whose heading is the isolated variable, and
publish the same prior-state facing alignment, the cosine between the prior dog
forward direction and the dog-to-sheep direction; only the on case adds a
separate away-from-dog vector scaled by the positive part of that alignment
under the same radius and falloff, so a dog looking away releases rather than
pulls. Paired `sheep-dog-line-of-sight-off` and `sheep-dog-line-of-sight-on`
fixtures keep that accepted distance-only pressure identical and publish the same
prior-state blocked flag and named occluder, tested as a zero-width planar
segment against the analytic obstacles the dog collides with; only the on case
releases the dog terms when a wall or a closed gate stands between the sheep and
the dog. Visibility is binary, so that release is discontinuous at an obstacle
edge. Every sheep displacement chosen by a fixture is then resolved through the
same game-owned analytic paddock the dog collides with, as an upright cylinder
of the sheep body radius owned by `sheep_state.hpp`: a wall, a closed gate, or
the paddock's own bounds physically stops the sheep, a clipped axis loses its
velocity on the contact tick exactly as the dog's does, and each sheep publishes
which axes were refused and which named obstacle refused them. Collision is a
later positional authority than steering and never rewrites a published
acceleration vector. Paired `sheep-paddock-collision-closed-gate` and
`sheep-paddock-collision-open-gate` fixtures disable every steering term and
differ only by the world gate state, so the analytic paddock is the only thing
that can change a sheep's straight-line motion. A sheep whose cylinder already
overlaps an obstacle is not pushed out, and sheep-versus-sheep and
sheep-versus-dog body collision are absent. Each sheep also carries an
`ordinary`, `nervous`, or `stubborn` temperament in the authoritative buffer,
because the accepted design makes the dog's effective pressure depend on it. Like
line of sight it adds no vector of its own: it is the last factor of every dog
magnitude, so an ordinary sheep multiplies by exactly `1.0` and reproduces the
accepted arithmetic bit for bit while a nervous or stubborn sheep publishes the
same stimulus and a proportionally larger or smaller response. It scales the dog
terms only; the social terms have no temperament factor, and each sheep publishes
the scale that was applied. Paired `sheep-temperament-neutral` and
`sheep-temperament-varied` fixtures stand five sheep on one exact 5-unit ring
around a stationary dog and differ only by the temperament switch. All of those
per-term vectors are then summed once, and that sum — not any individual term —
is limited by a single scenario-owned combined-influence bound set to the largest
accepted per-term maximum, so a sheep inside several overlapping influences
cannot be accelerated by their unbounded total. Each per-term vector keeps its
published value; each sheep publishes the pre-bound summed magnitude, the scale
the bound applied, and the acceleration integration used, and that scale is
exactly `1.0` wherever the bound did not bind. The bound's magnitude is a
provisional legibility choice rather than a measured one, and
[`ADR 0006`](../docs/decisions/0006-combined-influence-acceleration-bound.md)
records the combination rule and the rejected alternatives. Paired
`sheep-combined-influence-off` and `sheep-combined-influence-on` fixtures put two
sheep over the bound and two under it in the same tick and differ only by the
bound switch. Two further limits then act on the result of integration rather
than on any term: the planar speed is clamped to a scenario-owned maximum with
its direction preserved, and the heading follows the direction of that motion,
rotating toward it along the shorter arc by at most one scenario-owned turn
budget per tick. The shortest-arc rule is the dog motor's, shared through
`game/math.hpp` so the two animals cannot disagree about which way is shorter.
The heading is derived from the prior heading, so the published dog bearing —
which is relative to that prior heading — is never altered retroactively, and a
sheep moving slower than the named heading-motion floor keeps its previous facing
instead of snapping to the direction rounding invented. The maximum speed sits
between the dog's accepted walk and sprint and the turn rate below the dog's, but
both magnitudes are provisional legibility choices rather than measured ones. The
turn rate limits the heading only: motion is deliberately not slaved to it yet,
so a sheep pushed sideways still moves sideways while its facing catches up, and
[`ADR 0007`](../docs/decisions/0007-bounded-sheep-speed-and-turning.md) records
that choice and its consequence. Paired `sheep-motion-limit-off` and
`sheep-motion-limit-on` fixtures enable no steering term at all and instead give
five sheep exact initial velocities and headings, so the two limits are the only
thing that can change their motion. One further steering term looks at where a
sheep is going rather than at how hard it is pushed: obstacle and drop avoidance
probes along the sheep's own prior direction of travel, asks the same analytic
field which shape its swept body reaches first, and pushes away from the face it
would enter — turned exactly halfway toward the nearer free edge of that same
shape when such an edge lies within the look-ahead, so the sheep is steered aside
as well as slowed — while a look-ahead point whose `ground_height` is not finite
pushes it straight back from the drop. The response falls off linearly over the
look-ahead, both halves are held to one named maximum, and the whole thing is a
term like every other: it publishes its own vector, that vector joins the same
sum, the combined bound scales it identically, and it replaces nothing. The
paddock still resolves every displacement last, and avoidance is deliberately not
exempt from the bound, because the hard boundary rather than the steering term is
what guarantees a wall stops a sheep. Both magnitudes are derived from accepted
values — the maximum is the strongest single accepted influence and the look-ahead
is exactly the room that maximum needs to stop a sheep travelling at the accepted
maximum speed — and inherit their provisional status;
[`ADR 0008`](../docs/decisions/0008-obstacle-and-drop-avoidance.md) records the
rule, why the drop response is binary, the near-boundary correction that closed
QA-003, and the limits left standing. The term is direction-aware rather than
proximity-aware, so a sheep standing beside a wall or running parallel to one
feels nothing — including at zero clearance, because a reported contact always
lies at or ahead of the body and touching a shape's boundary while travelling
along it is not being inside it, exactly as the hard clip's own overlap test is
strict — and a sheep with no measurable motion publishes an unevaluated record
instead of a direction rounding invented. A contact distance of zero therefore
means the body is touching the face it is moving into, and the maximum it earns
is the same answer on every tick it keeps pressing. The
paddock's ground is finite everywhere inside its own bounds, so the only drop
that exists today is the paddock edge. Paired `sheep-avoidance-off` and
`sheep-avoidance-on` fixtures enable no other steering term, start every driven
sheep exactly half a look-ahead from the face it is heading at, and differ only by
the avoidance switch. Two fields that had existed unwritten in the authoritative
buffer since the first sheep are now driven: `arousal` and `behavior`. **Arousal
is a named game parameter, not a claim about animal physiology** — a bounded
`[0, 1]` design variable, not a heart rate or a stress hormone — and it follows
the published dog stimulus, expressed as a dimensionless fraction of the same
linear falloff over the same radius, released by the same sight line and scaled by
the same temperament factor, at a scenario-owned rise rate upward and a slower
recovery rate downward. It cannot overshoot, so it cannot oscillate on its own.
Arousal plus whether a cause is acting selects `settled`, `alert`, `driven`, or
`recovering` from the immutable prior label, prior arousal, and prior-state
stimulus, with each band entered at its named arousal and left only at a lower
one so a sheep resting on a threshold holds its label. `recovering` is the state
after release and before rest, and it is the one label a sheep can never carry
while a cause is acting on it. **No steering term reads either field**: the
transitions are deliberately observational for now, and the paired
`sheep-behavior-transitions-off` and `sheep-behavior-transitions-on` fixtures —
which enable no steering term and differ only by the transition switch — prove it
by publishing identical positions and identical evidence.
[`ADR 0009`](../docs/decisions/0009-behavior-transitions-and-arousal.md) records
the non-physiological framing, the rates and thresholds, why hysteresis is
separate thresholds rather than a dwell timer, and what is deliberately left out.
Damping, terrain, avoidance against an interior ledge, the
terrain pressure factor, arousal causes other than the dog, and any effect of
behavior on steering remain deferred. The dog is a kinematic upright
cylinder whose world-space
planar motor bounds vector acceleration/deceleration, rotates facing toward
movement by the shortest path, and slows during large heading changes. It
retains predictable analytic ground contact and collision shapes for the paddock edges,
representative walls, and gate that are independent of voxel faces and render
meshes; the same game-owned field answers the sheep sight-line query. Version 1, seed-zero `paddock-start`, `presentation-motion`,
`sheep-only-separation`, `sheep-only-attraction`, `sheep-alignment-off`,
`sheep-alignment-on`, `sheep-dog-pressure-off`, `sheep-dog-pressure-on`,
`sheep-dog-approach-off`, `sheep-dog-approach-on`, `sheep-dog-facing-off`,
`sheep-dog-facing-on`, `sheep-dog-line-of-sight-off`,
`sheep-dog-line-of-sight-on`, `sheep-paddock-collision-closed-gate`,
`sheep-paddock-collision-open-gate`, `sheep-temperament-neutral`,
`sheep-temperament-varied`, `sheep-combined-influence-off`,
`sheep-combined-influence-on`, `sheep-motion-limit-off`, `sheep-motion-limit-on`,
`sheep-avoidance-off`, `sheep-avoidance-on`,
`sheep-behavior-transitions-off`, `sheep-behavior-transitions-on`,
`sheep-all-influences-diagnostic`, `wall-contact`, `closed-gate`, and
`open-gate` gameplay definitions provide deterministic initialization and exact
restart. `sheep-all-influences-diagnostic` is the one deliberately maximal
fixture — every steering term, both motion limits, temperament, line of sight,
the combined bound, and the behavior transitions switched on together against a
dog that drives the flock into the closed wall line and releases it. It exists
for the properties that only appear when everything runs at once: that the
applied acceleration is always exactly the published terms, that no term
oscillates, and that the whole published sequence is reproducible. **It is a
diagnostic, not accepted tuned gameplay**, and no owner has reviewed the motion
it produces. Every one of those per-sheep rules — the paddock resolve, the dog
stimulus, the behavior transition, neighbour selection, the three social terms,
avoidance, the combined bound and its integration, and the two motion limits —
is declared in `game/sheep_rules.hpp` and defined in `sheep_rules.cpp` as a
function over immutable prior state and a `std::span<const SheepState>`.
`GameplaySimulation` remains the sole authoritative caller and still owns tick
order, the fixed five-member buffers, snapshot publication, scenario validation,
and the replay contract; what it no longer owns is a private monopoly on the
arithmetic, because a rule only one caller can reach can only be measured
through that caller. The member count reaches the rules only as an index, so a
non-player diagnostic can drive the same functions over a longer caller-owned
span **without any published contract growing a member**: `kGameplaySheepCount`
is still `5`, every evidence buffer is still a five-member array, and
`GameplaySnapshot` is still a fixed-size value.
[`ADR 0010`](../docs/decisions/0010-diagnostic-flock-scale-over-shared-sheep-rules.md)
records that decision, the three widenings it rejected, and why the diagnostic
lives in `tests/` and is never linked into `wide_eye`. The
gameplay camera owns orbit yaw/pitch independently of dog facing; orchestration
applies look first, resolves camera-yaw-relative movement on the ground plane,
then advances the dog. The free-debug camera retains independent position/look
state. Previous/current dog snapshots and camera states produce one coherent
interpolated render view without feeding presentation back into simulation. The
owner accepted this keyboard/mouse control baseline on 2026-08-16 and deferred
refinement. It still has no animation, camera obstruction, complete flock
behavior, final sheep art, final tuning, or physically verified controller
feel.
