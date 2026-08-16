# Tests

CTest registrations live with the target they verify. Add test sources here
when a tracer introduces a real invariant; do not add placeholder assertions.

The current fast suite covers exact known-byte PNG encoding; signed integer
world/chunk/local coordinate boundaries and round trips; empty, full, boundary,
adjacent, and edited 16³ chunks; safe get/set and dirty-region invariants; a
deterministic 16³/32³ equal-world memory/rebuild comparison; naive exposed-face
meshing for empty, single-cell, adjacent-cell, full, surrounded, and all six
cross-chunk boundary cases; exact quad topology and outward winding; opaque-
default plus independent opaque/cutout/translucent routing and cross-pass
culling; fixed 16³ conservative face/vertex/index ceilings; a 12,288-face
checkerboard; exact aggregate-limit acceptance; vertex/index rejection; exact
per-side emitted/culled diagnostics with same-, adjacent-, and missing-chunk
neighbor provenance; four-chunk handcrafted-paddock block/material counts;
merged world-space topology; internal border removal; bounded palette values;
exact per-material face totals; required visible material faces; and a complete
10,476-record paddock ledger that proves six unique decisions per occupied block,
none for empty blocks, live source/neighbor material agreement, and a one-to-one
mapping from all 2,754 emitted decisions to rendered quads; the
accepted Tracer 0 and Tracer 1 paddock packets' files, hashes, and verdicts; the
60 Hz fixed-step accumulator; exact authoritative gameplay state after the same
tick-indexed controls under 100×10 ms and 10×100 ms render partitions;
previous/current snapshot publication, read-only interpolation, suspended-dog
ticks, and restart coherence; one-to-one preservation of five published sheep
IDs, positions, and headings in the renderer-facing proxy buffer; repeated
hand-authored five-sheep centroid, ground-plane mean radius, polarization,
bounded elongation, group-speed, nearest-neighbor spacing, threshold-connected
component, and chosen-neighbor-count calculations, including invalid-input
rejection; deterministic ground-plane uniform-grid queries with exact-radius
filtering, caller-bounded nearest selection, stable distance/ID tie ordering,
negative and boundary cells, reversed storage, invalid-input rejection,
snapshot-copy ownership, and zero allocations at the fixed 1,000-member
capacity-experiment ceiling; repeated
scripted `presentation-motion` state, all-five translation, midpoint turn
interpolation, immutable-prior publication, exact restart, and one-to-one moving
proxy poses without claiming flock behavior; independent
version 1 seed, action-input, replay,
and state-dump contracts; canonical compact JSON; pre-mutation rejection of
unsupported versions, tick rates, scenario/seed mismatches, gaps, and invalid
values; repeated local replay/state equality; window-state transitions; fatal project
assertions; the dummy-driver SDL lifecycle; nearest-rank duration summaries and
nonzero process-memory sampling; keyboard/mouse/gamepad named-input translation;
mouse-delta accumulation, signs, retention, one-tick consumption, focus clearing,
and stick-rate coexistence; camera-relative cardinal and diagonal movement;
mouse orbit/body independence; planar acceleration, facing, reversal slowdown,
restart, angle interpolation, deterministic repeated control sequences, and
named scenarios; giant and fixed-tick wall/gate non-tunneling; open-gate
passage; and isolated gameplay/free-camera state. The
chunk comparison is labeled `performance` but asserts only exact work
and equivalence invariants; release timing is an explicit `--benchmark`
observation, not a machine-dependent CTest threshold. On a
capable OpenGL 4.6 target it also covers context validation, triangle and depth-
tested cube framebuffer oracles, byte-identical repeated normal and wireframe
cube captures, the paddock center/depth oracle, byte-identical repeated 960×540
paddock captures, broad framebuffer oracles for same-camera chunk bounds,
face-normal vectors, indexed wireframe, and mesh-statistics views, and injected
high-severity GL rejection. The dog render smoke checks the same paddock pipeline
with an optional grounded dog and five procedural sheep proxies derived from one
published gameplay snapshot. A separate bounded motion smoke pre-rolls the
presentation-only fixture to tick 61 and draws alpha 0.5 through the same proxy
material, static-shadow receiver, and five-draw path. The proxy-pose oracle also
counts allocations across 600 snapshot/interpolation preparations. Release
additionally runs the named 1920×1080 static-paddock and five-proxy motion
measurement paths; their CTests check completion and diagnostics, while review
packets record preparation/submission/GPU/frame percentiles, RSS, allocations,
and the explicit provisional-budget comparison instead of enforcing a
machine-agnostic timing threshold.
The CMake wrappers distinguish required process behavior from a crash or a
missing diagnostic; the capture wrapper preserves failed outputs but removes
its temporary PNGs after a passing repeat comparison. The artifact-manifest
validators independently check versioned Windows evidence packets' required
fields, retained files, and SHA-256 hashes. Accepted-review validation requires
exactly one Accept verdict plus an owner observation and date. A nested CTest
regression proves that the common failure regex rejects
project failure markers and ASan, LSan, and UBSan diagnostics even when output
also contains a configured pass marker.
