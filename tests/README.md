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
capacity-experiment ceiling; a named sheep-only separation scenario with exact
overlap recovery, immutable-prior publication, capped acceleration, stable
per-ID results under reversed storage, exact restart, initially out-of-range
rejection, and zero allocations across 600 fixed updates; bounded
selected-neighbor attraction with a four-candidate/two-selected density
oracle, exact distance/ID tie ordering, published influence equality, state-dump
evidence, reversed-storage identity, restart, and zero allocations across 600
fixed updates; paired alignment-off/on fixtures with identical initial state,
one-neighbor bounded selection, exact published/applied alignment acceleration,
60-tick polarization comparison, reversed-storage identity, state-dump
evidence, restart, and zero allocations across 600 fixed updates; paired
distance-only dog-pressure off/on fixtures with identical prior-state distance/
bearing observations, exact falloff/direction and published/applied vectors,
radius rejection, zero-direction exact-overlap handling, immutable-prior dog
stimulus, reversed-storage identity,
state-dump evidence, restart, and zero allocations across 600 fixed updates;
paired dog-approach off/on fixtures that preserve the accepted distance-only
pressure exactly, with identical prior-state approach speeds, saturated head-on
response, zero abeam response, no response to a leaving dog, an exact diagonal
projection, published/applied sum equality, radius rejection, zero-direction
exact-overlap handling, immutable-prior approach evidence, reversed-storage
identity, state-dump evidence, restart, and zero allocations across 600 fixed
updates; paired dog-facing off/on fixtures that preserve the accepted
distance-only pressure exactly, with identical prior-state facing alignments, a
full head-on response, zero abeam response, no response from a dog looking away,
an exact diagonal cosine, a half-turn heading control that swaps the front and
back results without moving a position, published/applied sum equality, radius
rejection, zero-direction exact-overlap handling, immutable-prior facing
evidence, reversed-storage identity, state-dump evidence, restart, and zero
allocations across 600 fixed updates; paired dog-line-of-sight off/on fixtures
that preserve the accepted distance-only pressure exactly, with identical
prior-state blocked flags and named occluders, an unchanged clear-line response,
released pressure behind each wall, a preserved response through the open gate
gap, a gate-state control that hides only the sheep watching through the
opening, a combined derived fixture in which the pressure, approach, and facing
vectors are released together while their prior-state evidence is unchanged,
published/applied equality in both members, zero-direction exact-overlap
handling, immutable-prior sight evidence, reversed-storage identity, state-dump
evidence, restart, and zero allocations across 600 fixed updates;
repeated scripted `presentation-motion` state, all-five
translation, midpoint turn interpolation, immutable-prior publication, exact
restart, and one-to-one moving proxy poses without claiming flock behavior;
independent version 1 seed, action-input, and replay contracts plus the
versioned state-dump contract; canonical compact JSON; pre-mutation rejection of
unsupported versions, tick rates, scenario/seed mismatches, gaps, and invalid
values; repeated local replay/state equality; window-state transitions; fatal
project assertions; the dummy-driver SDL lifecycle; nearest-rank duration
summaries and
nonzero process-memory sampling; keyboard/mouse/gamepad named-input translation;
mouse-delta accumulation, signs, retention, one-tick consumption, focus clearing,
and stick-rate coexistence; the Escape pointer-capture binding, its single-read
press consumption, and the window-state rule that an unfocused window releases
the pointer while regaining focus never recaptures a deliberately released one; camera-relative cardinal and diagonal movement;
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
measurement paths. Their CTests and executables require the exact selected
budget to pass: the static Tracer 1 scene uses the general Low 1 GiB RSS cap,
while the five-proxy Tracer 2 scene uses its tighter 512 MiB cap; both retain the
Low frame p95/p99 limits. Review packets record the budget ID, preparation,
submission, GPU/frame percentiles, RSS, and allocations. These provisional proxy
comparisons do not establish support for the named Iris Xe target.
The CMake wrappers distinguish required process behavior from a crash or a
missing diagnostic; the capture wrapper preserves failed outputs but removes
its temporary PNGs after a passing repeat comparison. The artifact-manifest
validators independently check versioned Windows evidence packets' required
fields, retained files, and SHA-256 hashes. Accepted-review validation requires
exactly one Accept verdict plus an owner observation and date. A nested CTest
regression proves that the common failure regex rejects
project failure markers and ASan, LSan, and UBSan diagnostics even when output
also contains a configured pass marker. The same nested fixture proves that the
exact performance pass expression rejects `within_provisional_low_budget=no`
and accepts only `within_provisional_low_budget=yes`.
