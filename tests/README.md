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
evidence, restart, and zero allocations across 600 fixed updates; paired
sheep-paddock-collision closed-gate/open-gate fixtures in which the analytic
paddock stops a driven sheep at exactly the obstacle face plus the sheep body
radius, clears only the refused axis's velocity while the free axis keeps
moving, names the wall or gate that refused it, distinguishes the paddock's
outer bounds from a named obstacle, lets the same sheep through when only the
world gate state differs, leaves a non-contacting sheep bit-identical to
unclipped integration, keeps a live dog-pressure vector published while the
sheep is held, and preserves reversed-storage identity, restart, and zero
allocations across 600 fixed updates; paired temperament neutral/varied
fixtures on one exact five-unit ring in which ordinary reproduces the accepted
response bit for bit, nervous and stubborn scale every dog term by exactly the
configured factors under identical published geometry, the social terms are
unchanged in both members, and the published scale, published/applied equality,
unknown-temperament rejection, reversed-storage identity, restart, and zero
allocations across 600 fixed updates all hold; paired combined-influence off/on
fixtures in which every published per-term vector and pre-bound summed magnitude
is identical between the members, an over-bound sheep's summed terms are scaled
to exactly the bound with an unchanged direction on and off the axis, an
under-bound sheep and a sheep under no influence are untouched with a published
scale of exactly one, a sum exactly at the bound is left alone, the published
scale times the summed terms is the applied acceleration exactly, the bound holds
on every tick of a run while the unbounded control breaches it, and
reversed-storage identity, restart, unevaluated-bound evidence, and zero
allocations across 600 fixed updates all hold; paired motion-limit off/on
fixtures with no steering term enabled in which a sheep driven at two and a half
times the maximum keeps exactly the maximum speed with an unchanged direction on
and off the axis, an under-limit sheep and a stationary sheep are bit-identical
to the off member, a stationary sheep keeps its heading instead of facing
`atan2(0, 0)`, a reversed sheep turns by exactly one budget on each of fifty
consecutive ticks and then lands exactly on its motion direction, the same
sheep's published dog bearing stays relative to its prior heading while that
heading changes, a lowered maximum clamps a speed integration accumulated where
the control runs past it, the heading floor is observed from both sides, and
reversed-storage identity, restart, unevaluated-limit evidence, and zero
allocations across 600 fixed updates all hold; paired behavior-transition off/on
fixtures with no steering term enabled in which every published position and every
evidence record is identical between the two members and only arousal and
behavior differ, the off member stays settled at exactly zero arousal, one
deterministic run walks settled, alert, driven, recovering, and settled with each
ladder transition produced by a prior arousal exactly on its threshold and
`recovering` produced by release rather than by decay, arousal rises and decays at
exactly the configured per-tick budgets, a dog exactly at the stimulus radius
raises none at all, an exact overlap raises the maximum, temperament puts two
sheep at the same distance at opposite ends of the ladder, a stimulus above the
maximum is clamped, two runs holding the same arousal inside the driven
hysteresis band publish two different stable labels, a sheep resting exactly on
an entry threshold never flaps while an adversarial cause crosses that threshold
on nearly every tick, a scripted dog that approaches, holds, and leaves walks the
same four states, the same tick's dog move cannot alter a transition, an unknown
behavior state and an out-of-range arousal are both rejected by the writer, and
reversed-storage identity, restart of a non-zero starting arousal and label, and
zero allocations across 600 fixed updates all hold;
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
and accepts only `within_provisional_low_budget=yes`. A stack-budget wrapper
runs the gameplay-simulation harness under a reduced `ulimit -s` and fails by
name if the process does not finish inside it; the budget is
`WIDE_EYE_GAMEPLAY_SIMULATION_STACK_BUDGET_KIB` in `CMakeLists.txt`, the check
is registered on Unix hosts only, and it reports itself skipped rather than
failing where the limit cannot be set. It is why that harness holds every
`GameplaySimulation` fixture by `std::unique_ptr` instead of by value: QA-002
records the silent SIGSEGV the by-value version eventually produced.
