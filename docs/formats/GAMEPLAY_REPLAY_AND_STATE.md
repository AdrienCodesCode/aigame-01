# Gameplay replay and state-dump contracts

**Status:** Version 1 seed/action/replay and version 8 dog, five-sheep, social-
evidence, and dog-stimulus-evidence state dump implemented; the presentation
capture CLI can write a state dump, while JSON decoding and general replay/seed
CLI integration remain pending

**Last revised:** 2026-08-21

The `game` boundary owns these contracts. They make a fixed-tick input sequence
and its observed state inspectable without giving file or renderer code control
of `GameplaySimulation`.

The seed identifies a complete gameplay scenario, not a dog-controller
scenario. [`ADR 0004`](../decisions/0004-gameplay-scenario-ownership.md) records
that ownership correction, and
[`ADR 0005`](../decisions/0005-paddock-collision-ownership.md) records the later
move of the analytic paddock shapes and the gate flag out of the dog-owned
header. Both corrections left the then-current scenario names, seeds, scenario
versions, and serialized replay JSON unchanged because the serialized contract
already represented the whole simulation; only the internal type ownership
changed.

The authoritative implementation is
[`gameplay_replay.hpp`](../../src/game/gameplay_replay.hpp). The JSON writers and
validation rules are in
[`gameplay_replay.cpp`](../../src/game/gameplay_replay.cpp), and the focused
oracles are in
[`gameplay_simulation_tests.cpp`](../../tests/gameplay_simulation_tests.cpp).

## Compatibility rules

The contracts use four independently named versions:

- seed format `1` identifies a named scenario, that scenario's version, and a
  64-bit seed;
- action-input format `1` records one optional dog movement action per
  authoritative tick;
- replay format `1` binds the tick rate, seed contract, action-input version,
  and complete action sequence;
- state-dump format `8` records the seed contract, tick rate, restart count, and
  published previous/current dog, five-sheep, social-evidence, and dog-stimulus-
  evidence snapshots. Version 1 was dog-only, version 2 added sheep state,
  version 3 added attraction evidence, version 4 added alignment evidence,
  version 5 added dog distance, relative bearing, and pressure acceleration,
  version 6 added dog approach speed and a separate approach acceleration,
  version 7 added dog facing alignment and a separate facing acceleration, and
  version 8 added the dog line-of-sight blocked flag and the named analytic
  paddock obstacle that blocks it. No older version is silently reinterpreted.

A replay consumer must reject an unsupported version, a tick rate other than
60 Hz, an unknown scenario, a zero scenario version, a scenario/version/seed
mismatch, a non-contiguous tick sequence, or a non-finite or out-of-range action.
Validation completes before simulation mutation. Changing the meaning or
required fields of one contract requires incrementing its corresponding
version; a reader must not silently reinterpret a version it does not support.

The current encoder writes canonical compact JSON with a fixed key order and
round-trip-safe finite `double` values. This is useful for local comparisons and
future artifact hashing. It is not a claim that floating-point state is
byte-identical across compilers or platforms.

## Replay version 1

The schema name is `wide-eye.gameplay-replay`. Actions start at tick zero and
cover every consumed tick in order. `dog_move: null` advances authoritative
time while preserving the existing free-debug behavior that suspends the dog
motor. Otherwise `world_x` and `world_z` are finite normalized domain values in
`[-1, 1]`; `sprint` is boolean.

```json
{"schema":"wide-eye.gameplay-replay","version":1,"tick_rate":60,"action_input_version":1,"scenario":{"seed_format_version":1,"id":"paddock-start","version":1,"seed":0},"actions":[{"tick":0,"dog_move":{"world_x":0,"world_z":-1,"sprint":false}},{"tick":1,"dog_move":null}]}
```

`GameplayReplay` currently owns a contiguous action vector and
`apply_gameplay_replay` consumes that typed contract in memory. A general JSON
decoder, replay file path, checked-in replay fixture, and executable
`--replay`/`--seed` flags are intentionally deferred. No untrusted or external
JSON is accepted by the engine yet.

## State dump version 8

The schema name is `wide-eye.gameplay-state`. The dump contains the scenario
seed contract, fixed rate, restart count, and complete published previous and
current snapshots. Each snapshot contains the dog position, velocity, heading,
and grounded state plus exactly five sheep. Every sheep record contains ID,
position, velocity, heading, arousal, explicit behavior state, and grounded
state. A parallel fixed-size evidence record maps to each sheep by subject ID
and contains the exact attraction and alignment selected-neighbor IDs, selected
and in-radius candidate counts, and separate separation, attraction, and
alignment acceleration vectors. The writer validates subject mapping, bounds,
unique known neighbor IDs, zeroed unused IDs, and finite vectors before
encoding. A second fixed-size record per sheep states whether dog stimulus was
evaluated, then publishes the prior-state planar dog distance, signed bearing
relative to sheep heading, dog approach speed, dog facing alignment, whether the
sight line to the dog is blocked, the named paddock obstacle that blocks it, and
separate pressure-, approach-, and facing-acceleration vectors. Approach speed
is the component of prior dog velocity along the dog-to-sheep direction:
positive when the dog closes, negative when it leaves. Facing alignment is the
cosine between the prior dog forward direction and the dog-to-sheep direction:
`1` looking straight at the sheep, `0` abeam, `-1` looking directly away. The
occluder is `none`, `left_wall`, `right_wall`, or `gate`; the gate shape exists
only while the scenario's gate is closed, and the sight line is the zero-width
planar segment between the prior sheep and dog positions. Line of sight adds no
acceleration vector of its own: when its term is enabled, an occluded dog
releases the pressure, approach, and facing vectors instead. The writer requires
unevaluated stimulus fields to remain zero, requires a blocked flag to name an
obstacle and a clear flag to name `none`, and validates finite distance/approach
data, a bearing within `[-π, π]`, and an alignment within `[-1, 1]` before
encoding.

IDs 1–5 initialize in a fixed contiguous buffer with `settled` behavior and zero
arousal. The default scenarios keep them stationary. The version 1, seed-zero
`presentation-motion` fixture moves them through a deterministic scripted path
while leaving behavior settled and arousal zero; this is presentation evidence,
not a flock-response model. The version 1, seed-zero
`sheep-only-separation` fixture starts two sheep at the same position and
records bounded close-range repulsion. The independent version 1, seed-zero
`sheep-only-attraction` fixture places four candidates inside sheep 1's
attraction radius, selects the nearest two by distance then stable ID, and pulls
toward their prior-snapshot centroid. The paired `sheep-alignment-off` and
`sheep-alignment-on` fixtures share the same moving five-sheep start and differ
only by the alignment switch and scenario ID. The on case selects at most one
nearest prior-snapshot neighbor and accelerates toward its velocity under the
scenario response time and cap. The paired `sheep-dog-pressure-off` and
`sheep-dog-pressure-on` fixtures share one stationary dog/five-sheep start and
differ only by the pressure switch and required scenario ID. Both publish the
same prior-state distance and relative-bearing geometry; the on case applies a
linear distance falloff directed away from the dog inside a scenario-owned
radius. The paired `sheep-dog-approach-off` and `sheep-dog-approach-on` fixtures
add one dog moving at 4.0 world units/s, keep that accepted distance-only
pressure enabled and identical in both cases, and differ only by the approach
switch and required scenario ID. Both publish the same prior-state approach
speed; the on case adds a separate away-from-dog vector that responds only to a
closing dog, shares the pressure radius and linear falloff, and saturates at the
scenario reference speed. The paired `sheep-dog-facing-off` and
`sheep-dog-facing-on` fixtures use one stationary dog whose heading is the
isolated variable, keep the accepted distance-only pressure enabled and
identical in both cases, and differ only by the facing switch and required
scenario ID. Both publish the same prior-state facing alignment; the on case
adds a separate away-from-dog vector scaled by the positive part of that
alignment, sharing the pressure radius and linear falloff, so a dog looking away
releases rather than pulls. The paired `sheep-dog-line-of-sight-off` and
`sheep-dog-line-of-sight-on` fixtures share one stationary dog placed north of
the paddock wall line with the gate open, keep the accepted distance-only
pressure enabled and identical in both cases, and differ only by the
line-of-sight switch and required scenario ID. Both publish the same prior-state
blocked flag and named occluder; only the on case releases the dog terms for an
occluded sheep. Terrain, temperament, and behavior-state transitions remain
absent, and the dog terms are still summed without a combined-influence bound. A non-finite state is rejected because JSON has no
portable representation for NaN or infinity.

The state dump is an observation, not a save format. It does not restore a
simulation, promise save compatibility, include renderer state, or establish
cross-platform determinism. Changing required state fields or their meaning
requires another explicit state-dump version decision with focused tests.

The bounded presentation fixture can write this canonical observation beside a
capture with `--state-dump <json-path>`. That output-only platform path does not
decode JSON, restore state, or add replay/seed ingestion.

## Observed evidence and limits

**Observed result (2026-08-16):** on WSL Ubuntu 24.04.4 with Clang 18.1.3, two
fresh simulations consumed the same three-tick typed dog-input replay and
produced equal dog-and-sheep snapshots plus byte-identical canonical state
dumps. At that checkpoint the dump was version 2. Focused oracles verified
contiguous IDs 1–5, immutable-prior publication, restart, interpolation, and
zero allocations across 600 fixed
updates. They also rejected incompatible replay/action/seed versions, a 59 Hz
replay, non-contiguous ticks, a mismatched seed, non-finite input, application
after tick zero, and non-finite dumped dog state. A later focused extension
verified repeated exact `presentation-motion` state, immutable prior/current
publication, midpoint facing interpolation, exact restart, and settled
zero-arousal state for all five sheep. Development and ASan/UBSan suites each
passed 22/22 CTests; project formatting and bounded static analysis passed.

**Observed result (2026-08-16):** the state dump advanced to version 3 when
required per-sheep social evidence was added. The focused attraction oracle
observed four in-radius candidates for sheep 1, exact selected IDs `[2, 3]`
under the two-neighbor bound, and an attraction vector equal to the applied
acceleration. Reversed fixture storage produced exact per-ID sheep state and
evidence; restart was exact; 600 attraction ticks allocated no heap memory.
WSL development, Release, and ASan/UBSan configurations each passed 24/24
CTests; formatting and bounded clang-tidy passed.

**Observed result (2026-08-16):** required alignment selection and influence
fields advanced the state dump to version 4. The paired 60-tick oracle measured
polarization `0.824621` with alignment off and `0.924042` with alignment on.
For sheep 1, the first alignment-on tick selected ID 2 from two in-radius
candidates, published the exact `(-1, 0, -1)` acceleration applied to velocity,
and left separation and attraction vectors zero. Reversed fixture storage
produced exact per-ID state and evidence; restart was exact; 600 alignment ticks
allocated no heap memory. WSL development, Release, and ASan/UBSan
configurations each passed 24/24 CTests; formatting and bounded clang-tidy
passed. This synthetic directional-agreement result retains alignment
provisionally; it is not biological validation or player-facing motion review.

**Observed result (2026-08-16):** required prior-state dog distance, signed
relative bearing, stimulus-evaluated flag, and separate pressure vector advanced
the state dump to version 5. The paired distance-only oracle observed the same
geometry with pressure off/on, exact linear falloff at distances 2 and 3,
zero pressure at and beyond the 6-unit radius, and away-from-dog vectors matching
the applied acceleration. Exact dog/sheep overlap published zero bearing and
pressure instead of inventing a direction. The oracle also proved that dog
movement in the tested tick did not alter prior-state stimulus, reversed sheep
storage preserved exact per-ID state/evidence, restart was exact, and 600
enabled ticks allocated no heap memory. WSL development, Release, and ASan/UBSan
configurations each passed 24/24 CTests; formatting and bounded clang-tidy
passed. This is synthetic causal evidence, not calibrated sheep behavior or
player-facing motion acceptance.

**Observed result (2026-08-17):** required prior-state dog approach speed and a
separate approach vector advanced the state dump to version 6. In the paired
approach oracle's first tick, sheep 1 closed head-on at `4.0` world units/s
above the `3.0` reference speed and received the saturated `1.333333` away
acceleration; the abeam sheep measured exactly `0` approach speed and no
approach vector; the sheep behind the dog measured `-4.0` and received no
approach vector, so a leaving dog releases rather than pulls; the 3-4-5 diagonal
sheep measured the exact `2.4` projection and its `(0.16, 0.213333)` vector; and
the sheep outside the 6-unit radius published `4.0` approach speed with no
influence. The off case reproduced identical distance, bearing, approach speed,
and distance-only pressure vectors with a zero approach vector, so the accepted
distance-only control is preserved exactly. The oracle also proved that the same
tick's dog-motor velocity change did not alter prior-state approach evidence,
that exact dog/sheep overlap invented no approach direction, that reversed sheep
storage preserved exact per-ID state and evidence, that restart was exact, and
that 600 enabled ticks allocated no heap memory. On WSL Ubuntu 24.04.4 with
Clang 18.1.3, development, Release, and ASan/UBSan configurations each passed
24/24 CTests; formatting and bounded clang-tidy passed. This is synthetic causal
evidence, not calibrated sheep behavior or player-facing motion acceptance.

**Observed result (2026-08-17):** required prior-state dog facing alignment and a
separate facing vector advanced the state dump to version 7. In the paired
facing oracle's first tick, the stationary dog's heading-zero forward direction
gave the sheep straight ahead an exact `1.0` alignment and the full `(0, -1)`
facing acceleration at distance 2; the abeam sheep measured exactly `0`
alignment and received distance pressure only; the sheep directly behind
measured `-1.0` and received no facing vector; the 3-4-5 diagonal sheep measured
the exact `0.8` cosine and its `(0.12, -0.16)` vector; and the sheep straight
ahead but outside the 6-unit radius published `1.0` alignment with no influence.
Turning the same fixture's dog through half a turn without moving any position
swapped those front and back results exactly, so the term reads dog heading
rather than fixture layout. The off case reproduced identical distance, bearing,
approach speed, alignment, and distance-only pressure vectors with a zero facing
vector, so the accepted distance-only control is preserved exactly. The oracle
also proved that the same tick's dog-motor turn did not alter prior-state facing
evidence, that exact dog/sheep overlap invented no facing direction, that
reversed sheep storage preserved exact per-ID state and evidence, that restart
was exact, and that 600 enabled ticks allocated no heap memory. On WSL Ubuntu
24.04.4 with Clang 18.1.3, development, Release, and ASan/UBSan configurations
each passed 24/24 CTests; formatting and bounded clang-tidy passed. This is
synthetic causal evidence, not calibrated sheep behavior or player-facing motion
acceptance.

**Observed result (2026-08-21):** required prior-state dog line-of-sight blocking
and its named occluder advanced the state dump to version 8. In the paired
line-of-sight oracle's first tick, the sheep with a clear line at distance `4`
reproduced the accepted `(0, -1)` distance pressure exactly; the sheep at an
exact distance of `5` behind the left wall published `left_wall` and received
zero applied dog acceleration where the control published `(-0.3, 0.4)`; the
mirrored sheep behind the right wall published `right_wall` against the
control's `(0.3, 0.4)`; the sheep looking through the open gate at distance `5`
kept its full `(0, 0.5)` pressure; and the sheep occluded from distance `10`
published its blocked flag with no influence in either case. Closing the same
fixture's gate without moving anything flipped only the gate-gap sheep to a
`gate` occluder with zero pressure and left the other sight lines identical. A
derived fixture with the approach and facing terms also enabled observed the
occluded sheep publish `2.4` approach speed and `0.8` facing alignment while all
three applied vectors went to zero together, and the visible sheep keep all
three accepted vectors unchanged. The off case reproduced identical distance,
bearing, approach, alignment, blocked, and occluder evidence with the accepted
distance-only pressure applied through the wall, and published vectors matched
applied acceleration in both cases. A direct comparison against a pre-change
build found the canonical dumps of all fifteen earlier scenarios byte-identical
over 240 scripted ticks each once the two new keys and the version number were
removed, so version 8 adds fields without reinterpreting version 7 values. The oracle also proved that the same tick's
dog-motor move did not alter prior-state sight evidence, that exact dog/sheep
overlap invented no occluder, that reversed sheep storage preserved exact per-ID
state and evidence, that restart was exact, and that 600 enabled ticks allocated
no heap memory. On WSL Ubuntu 24.04.4 with Clang 18.1.3, development, Release,
and ASan/UBSan configurations each passed 24/24 CTests; formatting and bounded
clang-tidy passed. Visibility is binary, so pressure changes discontinuously as
a sight line crosses an obstacle edge. This is synthetic causal evidence, not
calibrated sheep behavior or player-facing motion acceptance.

**Observed result (2026-08-16):** the earlier native Windows Release capture
commands wrote canonical version 2 state at presentation ticks 1, 61, and 121.
The independent normal, repeat-normal, and face-normal debug commands at tick 61
produced
byte-identical state files with SHA-256
`8b2921e4a87bc2e8b8b86e08f4f17d8b3a7bf7c9413293b90b03b728ec27a905`.
This is local same-platform evidence, not a cross-platform identity claim.

Native Linux graphics, a native Windows version 8 capture, JSON decoding, CLI
replay/seed ingestion, persistent replay files, terrain and temperament pressure
factors, combined-influence acceleration bounds, behavior transitions, objective
outcomes, and cross-platform state or text identity remain untested or
unimplemented.
