# Gameplay replay and state-dump contracts

**Status:** Version 1 seed/action/replay and version 15 dog, scenario-owned-flock,
social-evidence, dog-stimulus-evidence, sheep-temperament, sheep-collision-evidence,
combined-influence-bound, sheep-motion-limit, sheep-avoidance, and
sheep-arousal-stimulus state dump implemented; the presentation capture CLI can
write a state dump, while JSON decoding and general replay/seed CLI integration
remain pending

**Last revised:** 2026-08-22

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
- state-dump format `15` records the seed contract, tick rate, restart count, and
  published previous/current dog, flock, social-evidence, dog-stimulus-
  evidence, sheep-collision-evidence, combined-influence-bound,
  sheep-motion-limit, and sheep-avoidance snapshots. Version 1 was dog-only,
  version 2 added sheep state, version 3 added attraction evidence, version 4
  added alignment evidence, version 5 added dog distance, relative bearing, and
  pressure acceleration, version 6 added dog approach speed and a separate
  approach acceleration, version 7 added dog facing alignment and a separate
  facing acceleration, version 8 added the dog line-of-sight blocked flag and the
  named analytic paddock obstacle that blocks it, version 9 added the
  per-sheep paddock-contact record, version 10 added the per-sheep
  temperament label and the temperament response scale it applied to the dog
  terms, and version 11 added the per-sheep combined-influence record: the
  pre-bound summed magnitude of every steering term, the scale the bound applied
  to that sum, and the acceleration integration actually used, and version 12
  added the per-sheep motion-limit record: the integrated planar speed, the scale
  the speed clamp applied to it, the speed the sheep kept, and the direction of
  motion the heading turned toward with the rotation the turn rate allowed, and
  version 13 added the per-sheep avoidance record: whether the term ran, the
  named analytic obstacle the sheep's look-ahead path reaches first and how far
  along it that shape lies, whether the ground under the look-ahead point is not
  finite, and the acceleration the term produced, version 14 added the per-sheep
  arousal stimulus the behavior transition follows, and version 15 made the flock
  size scenario-owned: each snapshot now states its own `sheep_count` and writes
  exactly that many records into each per-member array, where version 14 and every
  version before it wrote exactly five. A version 14 reader must not be pointed at
  a version 15 dump, because a dump of a scenario with more than five members has
  arrays it cannot describe. No
  older version is silently reinterpreted.

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

## State dump version 15

The schema name is `wide-eye.gameplay-state`. The dump contains the scenario
seed contract, fixed rate, restart count, and complete published previous and
current snapshots. Each snapshot contains the dog position, velocity, heading,
and grounded state, then its own `sheep_count`, then exactly that many sheep.

The flock size is **scenario-owned data**, not a constant of the format.
`GameplayScenarioDefinition::sheep_count` supplies the initial value, the
simulation carries it on every published snapshot, and every per-member array in
the dump — sheep, social evidence, dog-stimulus evidence, collision evidence,
avoidance evidence, combined-influence evidence, and motion-limit evidence — has
exactly `sheep_count` entries in the same order. The authoritative buffers behind
them are fixed-size arrays of `kMaximumGameplaySheepCount`, currently `256`; the
entries past the active count are default-constructed filler that no rule writes
and the dump never serializes. A `sheep_count` of zero, or one above that
capacity, is refused with `non_finite_state` before any text is produced. Every
named scenario written before this version still publishes five, and their dumps
are byte-identical to their version 14 dumps apart from the version number and
the added `sheep_count` key. Every sheep record contains ID,
position, velocity, heading, arousal, explicit behavior state, explicit
temperament, and grounded state. Behavior is `settled`, `alert`, `driven`, or
`recovering` and arousal is a bounded `[0, 1]` value; both are described under
"Behavior transitions and arousal" below, and an unknown behavior state or an
out-of-range arousal is rejected before encoding. Temperament is `ordinary`,
`nervous`, or `stubborn`; it is part of the scenario's starting contract, does
not change during a run, and an unknown value is rejected before encoding. A parallel fixed-size evidence record maps to each sheep by subject ID
and contains the exact attraction and alignment selected-neighbor IDs, selected
and in-radius candidate counts, and separate separation, attraction, and
alignment acceleration vectors. The writer validates subject mapping, bounds,
unique known neighbor IDs, zeroed unused IDs, and finite vectors before
encoding. A second fixed-size record per sheep states whether dog stimulus was
evaluated, then publishes the prior-state planar dog distance, signed bearing
relative to sheep heading, dog approach speed, dog facing alignment, whether the
sight line to the dog is blocked, the named paddock obstacle that blocks it, the
temperament response scale that sheep applied, the arousal stimulus that scale
produced, and separate pressure-, approach-, and facing-acceleration vectors. Approach speed
is the component of prior dog velocity along the dog-to-sheep direction:
positive when the dog closes, negative when it leaves. Facing alignment is the
cosine between the prior dog forward direction and the dog-to-sheep direction:
`1` looking straight at the sheep, `0` abeam, `-1` looking directly away. The
occluder is `none`, `left_wall`, `right_wall`, or `gate`; the gate shape exists
only while the scenario's gate is closed, and the sight line is the zero-width
planar segment between the prior sheep and dog positions. Line of sight adds no
acceleration vector of its own: when its term is enabled, an occluded dog
releases the pressure, approach, and facing vectors instead. Temperament also
adds no vector of its own: the response scale is the last factor of every dog
magnitude, so an `ordinary` sheep multiplies by exactly `1.0` and reproduces the
version 9 arithmetic bit for bit while a `nervous` or `stubborn` sheep publishes
the same stimulus and a proportionally larger or smaller response. The scale is a
property of the prior sheep rather than of the geometry, so it is published
whenever stimulus was evaluated, including at exact dog/sheep overlap where every
geometric term is zero. `arousal_stimulus` is the same dog stimulus expressed as a
dimensionless `[0, 1]` fraction rather than as an acceleration: the same linear
distance falloff over the same radius, released by the same line-of-sight rule,
scaled by the same temperament factor, and clamped to the arousal range because a
`nervous` scale can carry the product above one. It is the cause the behavior
transitions read, and it is published whenever stimulus was evaluated — including
at exact dog/sheep overlap, where the proximity is maximal and every geometric
term is zero — in a scenario with the transitions switched off as well as on, so a
paired control publishes an identical cause and only the applied arousal differs.
The writer requires
unevaluated stimulus fields to remain zero, requires a blocked flag to name an
obstacle and a clear flag to name `none`, requires an evaluated stimulus to
publish a finite, strictly positive response scale, requires the arousal stimulus
to be finite and within `[0, 1]`, and validates finite
distance/approach data, a bearing within `[-π, π]`, and an alignment within
`[-1, 1]` before encoding.

A third fixed-size record per sheep publishes what the paddock did to that
sheep's displacement this tick: `clipped_x` and `clipped_z` name the axes whose
requested movement the analytic field refused, and `contact_obstacle` names the
shape that refused them using the same `none`, `left_wall`, `right_wall`, `gate`
identity as the occluder. A clipped axis with `contact_obstacle` `none` was
stopped by the paddock's outer bounds, which are limits rather than obstacle
shapes; the writer therefore requires a named obstacle to have clipped an axis
but permits a clipped axis with no named obstacle. Collision is a separate,
later authority than steering: a clipped tick still publishes the acceleration
vectors that were applied, so a sheep held against a wall shows a live pressure
vector beside a refused displacement. A clipped axis also loses its velocity on
the contact tick, the same rule the dog motor uses, so contact is reported only
on ticks where the field actually refused movement. Sheep resolve their
displacement as an upright cylinder of the game-owned sheep body radius against
the same field the dog collides with; sheep-versus-sheep and sheep-versus-dog
body collision are not implemented.

A fourth fixed-size record per sheep publishes the one bound that limits what all
of those terms can do to that sheep together. `bound_evaluated` states whether
the tick summed any steering terms at all; a stationary or scripted fixture never
does and leaves every other field zero. `summed_acceleration_magnitude` is the
magnitude of the summed per-term vectors before the bound, `applied_scale` is the
factor the bound applied to that sum, and `applied_acceleration` is the vector
integration actually used. The rule is a clamp on the sum rather than on any
individual term: every per-term vector above keeps exactly the value its term
produced, and the relationship between them and the applied result is
`applied == sum * applied_scale`. `applied_scale` is exactly `1.0` whenever the
bound did not bind, including when the term is switched off and when the summed
magnitude is exactly equal to the bound, so an under-bound scenario is
arithmetically untouched rather than multiplied by one. The writer requires an
evaluated bound to publish a finite scale within `(0, 1]` — the rule is a bound,
not a gain — requires a finite, non-negative summed magnitude and a finite
applied vector, and requires an unevaluated bound to leave every field zero.
The bound is a per-sheep, per-tick pure function of that tick's terms and is
applied before integration and before collision;
[`ADR 0006`](../decisions/0006-combined-influence-acceleration-bound.md) records
the combination rule and the alternatives rejected for now.

A fifth fixed-size record per sheep publishes the two limits that act on the
result of integration rather than on any steering term. `limit_evaluated` states
whether the tick applied them at all; a fixture with the term switched off, and a
stationary or scripted fixture, leaves every other field zero.
`integrated_speed` is the planar speed the applied acceleration produced,
`applied_speed_scale` is the factor the speed clamp applied to it, and
`applied_speed` is the speed integration left the sheep with before collision may
refuse an axis. The clamp preserves direction by scaling both planar components,
and `applied_speed_scale` is exactly `1.0` whenever the clamp did not bind, so an
under-limit sheep is arithmetically untouched rather than multiplied by one.
`motion_heading_radians` is the direction of that applied velocity and
`heading_change_radians` is the signed rotation the turn rate allowed toward it
this tick, along the shorter arc and never larger than
`turn_rate * fixed_delta`. `motion_heading_followed` is `false` for a sheep whose
applied speed is at or below the named sheep heading-motion floor: it keeps its
previous heading instead of facing the direction rounding invented for a
cancelled-out velocity, and both heading fields then stay zero. The heading is
derived from the *prior* heading, so `dog_relative_bearing_radians` above — which
is relative to that prior heading — is never altered retroactively by the same
tick's rotation. The turn rate limits the sheep's heading only: motion is not
constrained to that heading, so a sheep pushed sideways still moves sideways
while its facing catches up. The writer requires an evaluated record to publish a
scale within `(0, 1]` and a result no faster than the integration that produced
it, requires a record that did not follow motion to publish no motion direction,
requires both angles to lie within `[-pi, pi]`, and requires an unevaluated
record to leave every field zero.
[`ADR 0007`](../decisions/0007-bounded-sheep-speed-and-turning.md) records both
magnitudes, the heading floor, and why motion is deliberately not slaved to
heading yet.

A sixth fixed-size record per sheep publishes the steering term whose job is to
make the hard collision authority unnecessary. `avoidance_evaluated` states
whether the term ran at all; a scenario with it switched off never runs it, and
neither does a sheep whose prior planar speed is at or below the named sheep
heading-motion floor, because the term probes along the sheep's own direction of
travel and a sheep that is not measurably moving has no direction to probe.
`obstacle_ahead` names the analytic paddock shape the look-ahead path reaches
first, using the same `none`, `left_wall`, `right_wall`, `gate` identity as the
occluder and the contact obstacle, and `obstacle_distance` is how far along that
path the shape lies. The body is swept against the same radius-expanded
rectangles the collision resolver stops it against, so the shape a sheep steers
around and the shape that refuses its displacement are the same shape.
`drop_ahead` is true when `ground_height` under the look-ahead point is not
finite. `avoidance_acceleration` is the vector the term produced: away from the
face the sheep would enter, turned exactly halfway toward the nearer free edge of
that same shape when such an edge exists within the look-ahead, and straight back
along the approach for a drop. The magnitude falls off linearly over the
look-ahead, so a shape exactly at the look-ahead distance is named with a vector
of exactly zero and the boundary is continuous. Avoidance is a steering term like
every other one: its vector joins the same sum, the combined bound scales it
identically, and it never rewrites another term or the collision stage. The
writer requires an unevaluated record to leave every field zero, requires a
published distance to name a shape, and requires a non-zero vector to name either
a shape or a drop.
[`ADR 0008`](../decisions/0008-obstacle-and-drop-avoidance.md) records the rule,
why both magnitudes are derived from already-accepted values, why the drop
response is binary, and the limits that are recorded rather than fixed — including
that the flat paddock has no interior drop, so the drop half is verified only
against the paddock edge until Phase 5 terrain exists.

### Behavior transitions and arousal

**Arousal is a named game parameter, not a claim about animal physiology.** It is
not a heart rate, a stress hormone, or any measured animal response; nothing in
this project has measured an animal. It is a bounded `[0, 1]` design variable
whose only job is to select one of the four behavior labels, and the writer
rejects a value outside that range.

Arousal is a rate-limited follower of `arousal_stimulus`: it moves toward that
value at up to a scenario-owned rise rate while the stimulus is higher and at up
to a scenario-owned recovery rate while it is lower, and is assigned exactly when
the stimulus is within one tick's budget. It therefore stays inside `[0, 1]`
without a clamp and cannot overshoot, so any oscillation visible in arousal is an
oscillation in its cause.

The behavior state is selected from *prior* state only — the prior label, the
arousal that label produced, and the prior-state stimulus — so the same tick's
arousal update cannot change it. Reading a transition out of the dump therefore
means reading `previous.sheep[i].arousal` together with
`current.…dog_pressure_evidence[i].arousal_stimulus`: those two values are what
produced `current.sheep[i].behavior`, and the arousal published beside a new
label is already one tick past the threshold that selected it.

`settled` means the sheep is not engaged: either no cause has lifted its arousal
as far as the alert threshold, or it has already come back to rest — so a
`settled` sheep may still carry a small arousal from a weak cause. `alert` and
`driven` mean a cause is acting and arousal has passed the alert or driven
threshold. `recovering` means the cause has been released and arousal has not yet
returned to rest; it is the one label a sheep can never carry while a cause is
acting on it. Each band is entered at its named arousal and left only at a lower
one, so a sheep whose arousal rests on an entry threshold holds its label instead
of alternating. No steering term reads the arousal or the label in this version:
the transitions are observational, and a scenario with them switched off is
arithmetically identical.
[`ADR 0009`](../decisions/0009-behavior-transitions-and-arousal.md) records the
non-physiological framing, the rates and thresholds, why hysteresis is separate
thresholds rather than a dwell timer, and the limits that are recorded rather
than fixed.

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
occluded sheep. The paired `sheep-paddock-collision-closed-gate` and
`sheep-paddock-collision-open-gate` fixtures share one stationary dog and five
sheep given exact initial velocities with every steering term disabled, so the
analytic paddock is the only thing that can change a sheep's straight-line
motion, and they differ only by the world gate state and the required scenario
ID. One sheep runs at the left wall, one runs at the gate line and is the paired
variable, one arrives diagonally at the right wall so one axis is blocked while
the other keeps running, one never touches anything, and one runs at the
paddock's own outer bound. The paired `sheep-temperament-neutral` and
`sheep-temperament-varied` fixtures stand five sheep on one exact 5-unit ring
around a stationary dog, keep the accepted distance-only pressure enabled and
identical in both cases, carry the same per-sheep temperaments, and differ only
by the temperament switch and the required scenario ID. Every sheep therefore
publishes the same prior-state distance and falloff, and only its bearing
differs; only the on case scales each sheep's dog response by the factor its
temperament names. The paired `sheep-combined-influence-off` and
`sheep-combined-influence-on` fixtures share one dog closing south at the
approach reference speed and looking down the same axis, keep separation,
pressure, approach, facing, and temperament enabled and identical in both cases,
and differ only by the bound switch and the required scenario ID. Both publish
identical per-term vectors and identical pre-bound summed magnitudes; only the on
case scales the sum. The paired `sheep-motion-limit-off` and
`sheep-motion-limit-on` fixtures enable no steering term at all and instead give
five sheep exact initial velocities and headings, so nothing but the two limits
can change a sheep's motion and every published number is exact arithmetic on the
fixture's own values: one sheep runs at two and a half times the maximum along
one axis, one at the same speed on an exact 3-4-5 diagonal, one under the maximum
already facing its motion, one standing still with a non-zero heading, and one
travelling in exactly the opposite direction to its heading. The paired
`sheep-avoidance-off` and `sheep-avoidance-on` fixtures likewise enable no other
steering term and give five sheep exact initial velocities, and differ only by the
avoidance switch and the required scenario ID. Every driven sheep starts exactly
half a look-ahead from the face it is heading at, so the linear falloff is exactly
one half: one runs head-on at a wall whose free edges are both further away than
it can see, one runs at the same wall near an end it can reach and is deflected
along it, one runs head-on at the exact middle of the closed gate where neither
free edge is nearer, one runs at the paddock's own outer bound — the only drop
that exists while the ground is flat — and one runs parallel to the wall line
closer than the look-ahead and is left exactly alone. The paired
`sheep-behavior-transitions-off` and `sheep-behavior-transitions-on` fixtures
enable no steering term at all — temperament is enabled, but with every dog term
off it produces no vector and only scales the arousal stimulus — and differ only
by the transition switch and the required scenario ID. They widen the shared
dog-pressure radius to exactly `8.0` so that one tick of the subject sheep's
exact `3.75` world-units/s travel moves the stimulus by exactly one part in 128.
One sheep is given that velocity and carried from exactly the stimulus radius,
straight through the dog, and out to the far side, so the dog-to-sheep distance
closes and opens exactly as it would if the dog walked in and left; one stands at
exactly the stimulus radius, where the falloff is exactly zero; one stands where
the stimulus is exactly `0.625`, inside the driven hysteresis band; and a
`nervous` and a `stubborn` sheep stand at the same exact `5`-unit distance, so an
identical cause puts one exactly on the driven entry threshold and leaves the
other below the alert threshold entirely. The unpaired
`sheep-all-influences-diagnostic` fixture is the exception to the
one-variable-at-a-time rule and is deliberately maximal: every steering term,
temperament, line of sight, the combined bound, both motion limits, and the
behavior transitions are switched on together, and the dog drives five sheep —
two `nervous`, one `stubborn` — into the closed wall line and then releases
them. It exists so that the properties that only appear when everything runs at
once can be observed: that the applied acceleration is exactly the published
terms summed and scaled, that no term oscillates between ticks, and that the
whole published sequence repeats. Its numbers are not exact fixture arithmetic
and **it is a diagnostic rather than accepted tuned gameplay**. Terrain and damping remain absent,
avoidance does not replace the hard collision authority — the paddock still
resolves every displacement last — and behavior does not feed back into
steering. A non-finite state is rejected because JSON has no portable
representation for NaN or infinity.

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

**Observed result (2026-08-21):** required per-sheep paddock-contact evidence
advanced the state dump to version 9 when sheep received the same analytic
collision authority the dog already had. In the paired collision fixture, whose
sheep start an exact `3.5` units north of the limit that will stop them and move
at `3.0` world units/s with every steering term disabled, the sheep aimed at the
left wall was refused on tick `70`, rested at exactly `16.5` — the wall face at
`16` plus the `0.5` sheep body radius — with its `z` velocity exactly `0`, and
never reached a smaller `z`. The sheep aimed at the gate line stopped at the
same exact `16.5` and published `gate` while the gate was closed; with the gate
open and nothing else changed it passed the wall line, was south of `15` by tick
`150`, and came to rest on the paddock's own southern bound at exactly `0.5`
with no named obstacle. The diagonal sheep published `right_wall`, stopped at
exactly `16.5`, kept its exact `-3.0` free-axis velocity, and continued sliding
west. The sheep aimed at the western bound stopped at exactly `0.5` with no
named obstacle. The non-contacting control sheep was bit-identical to plain
unclipped integration over 420 ticks, and every state and evidence field of all
four non-contacting sheep was identical between the two gate states. A derived
fixture that placed the dog north of the gate drove one sheep into the closed
gate from tick `76` and held it at exactly `16.5` on all `225` remaining ticks
while still publishing its live `-1.25` pressure vector at an exact distance of
`3.5`, and the same dog drove that sheep out through the open gate without a
single refused displacement. Reversed fixture storage preserved exact per-ID
state, evidence, and first-contact records; restart was exact; 600 collision
ticks allocated no heap memory. A direct comparison against a pre-change build
ran all seventeen earlier scenarios for 240 scripted ticks each: thirteen were
byte-identical once the new array and the version number were removed, and the
four that changed — `sheep-alignment-off`, `sheep-alignment-on`,
`sheep-dog-facing-off`, and `sheep-dog-facing-on` — are exactly the four in
which a sheep now contacts a wall or the gate instead of passing through it. On
WSL Ubuntu 24.04.4 with Clang 18.1.3, development, Release, and ASan/UBSan
configurations each passed 24/24 CTests; formatting and bounded clang-tidy
passed. A sheep whose cylinder already overlaps an obstacle is not pushed out,
the same limitation the dog motor has. This is synthetic causal evidence, not
calibrated sheep behavior or player-facing motion acceptance.

**Observed result (2026-08-21):** required per-sheep temperament and the
temperament response scale advanced the state dump to version 10. In the paired
temperament fixture, whose five sheep stand on one exact 5-unit ring around a
stationary dog so every sheep sees the same distance and falloff, the neutral
member gave
every sheep the exact same `0.5` of pressure and published a `1` response scale.
Switching the factor on left every published distance, bearing, approach speed,
facing alignment, and sight line identical and changed only the response: the
ordinary sheep's vector was bit-for-bit unchanged, each nervous sheep published a
`2` scale and exactly twice the neutral vector in both components, and each
stubborn sheep published a `0.5` scale and exactly half of it, giving `1` and
`0.25` of applied pressure against the ordinary `0.5`. Because the two configured
factors are powers of two those ratios are exact equalities rather than
tolerances, and the mirrored nervous/stubborn pairs on either side of the gate
line held the same exact `4:1` ratio, so the result is not an artifact of one
bearing. A derived fixture with the approach and facing terms also enabled scaled
all three vectors by the same published factor while the `3.0` and `2.4` approach
speeds and the `1.0` and `0.8` facing alignments stayed unchanged. A derived
fixture with separation, attraction, and alignment enabled published identical
social evidence in both members, so temperament modulates the dog stimulus only.
A flock whose sheep are all ordinary produced identical authoritative state on
every one of 120 ticks with the factor switched on and off. Over the same 120
ticks the nervous sheep drifted to `6.43757` from the dog while its mirrored
stubborn twin reached `5.46321`, with no sheep contacting anything. A direct
comparison against a pre-change build ran all nineteen earlier scenarios for 240
scripted ticks each and found the canonical dumps byte-identical once the two new
keys and the version number were removed, so version 10 adds fields without
reinterpreting version 9 values. The oracle also proved that the same tick's
dog-motor move did not alter prior-state temperament evidence, that the writer
rejects an unknown temperament, that reversed sheep storage preserved exact
per-ID state and evidence, that restart restored the fixture including its
labels, and that 600 enabled ticks allocated no heap memory. On WSL Ubuntu
24.04.4 with Clang 18.1.3, development, Release, and ASan/UBSan configurations
each passed 24/24 CTests; formatting and bounded clang-tidy passed. The doubling
and halving factors are a provisional legibility choice, not a measured or
biological value, and this is synthetic causal evidence rather than player-facing
motion acceptance.

**Observed result (2026-08-21):** the required per-sheep combined-influence
record advanced the state dump to version 11 when the summed steering terms
gained a single bound. In the paired combined-influence fixture, whose dog closes
south at the `3.0` approach reference speed while looking straight down the same
axis, the nervous sheep three units south of it stood under four simultaneous
influences on one axis — `1.5` of close-range separation from a neighbour `0.625`
away, plus doubled `3.0` pressure, `2.0` approach, and `1.5` facing responses — so
its six published terms summed to exactly `8.0`, twice the `4.0` bound. With the
bound off it was accelerated at that full `8.0`; with the bound on and nothing
else changed it published an applied scale of exactly `0.5`, an applied
acceleration of exactly `(0, 4)`, and an applied magnitude of exactly `4.0`, with
its `x` component exactly zero in both members, so the scaling changed magnitude
without changing direction. Because the fixture is built so the sum is an exact
power-of-two multiple of the bound, those are exact equalities rather than
tolerances. A second over-bound sheep on an exact 3-4-5 diagonal at distance
`3.75` summed to `4.35`, published a `0.919540` scale, and reached the same `4.0`
applied magnitude with a zero cross product against its unbounded sum, so the
bound is not an axis artifact. In the same tick the ordinary sheep `4.5` units
from the dog summed to exactly `1.625`, published a scale of exactly `1`, and was
bit-identical to the unbounded member, and the sheep outside the pressure radius
with no neighbour summed to exactly `0` and still published a scale of exactly
`1`. Every published social and dog-stimulus vector, and every pre-bound summed
magnitude, was identical between the two members; only the applied result
differed. Raising the same fixture's bound to exactly the over-bound sheep's
`8.0` sum reproduced the unbounded member bit for bit, so a sum exactly at the
bound is not over it. Across 120 ticks with no sheep touching the paddock, every
bounded sheep stayed within the bound on every tick while the control breached
it, and the bounded sheep ended at `27.3558` against its unbounded twin's
`30.2792`. A direct comparison against a pre-change build ran all twenty-one
earlier scenarios for 240 scripted ticks each and found every canonical dump
byte-identical once the new key and the version number were removed, so version
11 adds fields without reinterpreting version 10 values. The oracle also proved
that reversed sheep storage preserved exact per-ID state and evidence, that
restart was exact, that a fixture which sums no terms publishes the bound as
unevaluated with zeroed fields, and that 600 enabled ticks allocated no heap
memory. On WSL Ubuntu 24.04.4 with Clang 18.1.3, development, Release, and
ASan/UBSan configurations each passed 24/24 CTests; formatting and bounded
clang-tidy passed. The `4.0` magnitude is the largest single accepted per-term
maximum and is a provisional legibility choice, not a measured or tuned value,
and this is synthetic causal evidence rather than player-facing motion
acceptance.

**Observed result (2026-08-21):** the required per-sheep motion-limit record
advanced the state dump to version 12 when the result of integration gained a
speed clamp and the sheep heading began following its own motion. The paired
motion-limit fixture enables no steering term, so every observation below is
exact arithmetic on the fixture's own initial velocities rather than on an
integrated one. A sheep travelling straight down `+z` at exactly `12.5` world
units/s — two and a half times the `5.0` maximum — published an integrated speed
of exactly `12.5`, an applied scale of exactly `0.4`, an applied speed of exactly
`5.0`, and a velocity of exactly `(0, 5)`, with `x` exactly zero in both members,
so the clamp changed speed without changing direction. A second sheep on an exact
3-4-5 diagonal at the same `12.5` landed on exactly `(3, 4)` with a zero cross
product against its unclamped velocity, so the clamp is not an axis artifact. In
the same tick the under-limit sheep at `2.0` published a scale of exactly `1` and
was bit-identical to the off member, and the stationary sheep kept its heading of
exactly `1` radian while publishing `motion_heading_followed` false. A sheep
travelling at `3.0` in exactly the opposite direction to its heading rotated by
exactly `0.0625` radian — one turn budget — on each of 50 consecutive ticks,
reaching exactly `3.125`, then landed on exactly `pi` on tick 51 because the
remaining `0.016592653589793116` was smaller than a budget, and published a
change of exactly zero afterwards. For the first four of those ticks the
published `dog_relative_bearing_radians` was exactly the bearing computed from
the *prior* heading and differed from the bearing computed from the rotated one,
so a heading that changes during a tick does not alter that tick's published
stimulus. Every published social, dog-stimulus, and combined-influence record was
identical between the two members. Over 60 ticks the off member kept every
fixture velocity and every fixture heading unchanged while no sheep in either
member touched the paddock. A derived scenario that drove the accepted
combined-influence arrangement for 240 ticks with the maximum lowered to `2.0`
peaked at `2` against its unlimited control's `4.09362`, so the clamp also holds
against a speed that integration accumulated. A direct comparison against a
pre-change build ran all twenty-three earlier scenarios for 240 scripted ticks
each and found every canonical dump byte-identical once the new key and the
version number were removed, so version 12 adds fields without reinterpreting
version 11 values. The oracle also proved that reversed sheep storage preserved
exact per-ID state and evidence, that restart was exact, that a fixture without
the limits publishes an unevaluated zeroed record, and that 600 enabled ticks
allocated no heap memory. On WSL Ubuntu 24.04.4 with Clang 18.1.3, development,
Release, and ASan/UBSan configurations each passed 24/24 CTests; formatting and
bounded clang-tidy passed. The `5.0` maximum speed and `3.75` maximum turn rate
are provisional legibility choices, not measured or tuned values, and this is
synthetic causal evidence rather than player-facing motion acceptance.

**Observed result (2026-08-21):** the required per-sheep avoidance record
advanced the state dump to version 13 when steering gained obstacle and drop
avoidance. The paired avoidance fixture enables no other steering term and starts
every driven sheep exactly `3.125` — half the `6.25` look-ahead — from the face it
is heading at, so the linear falloff is exactly one half and every first-tick
number below is exact arithmetic. The sheep heading at the middle of the left
wall published `left_wall` at exactly `3.125` and a vector of exactly `(0, 2)`
with no lateral component, because its nearer free edge is `6.5` away — further
than it can see. The sheep heading at the same wall `1.5` from its `+x` end
published the same shape and distance with the two components exactly equal at
`1.4142135623730949` and a magnitude of `2`, so a reachable way round turns the
push exactly halfway toward it. The sheep heading at the exact middle of the
closed gate published `gate` with exactly `(0, 2)`, because neither free edge is
nearer and no side is invented. The sheep heading at the paddock's outer bound
published no shape, `drop_ahead` true, and exactly `(4, 0)` — the full maximum
straight back along its approach. The sheep running parallel to the wall line and
only `3.5` from it published an evaluated record with an exactly zero vector and
was bit-identical to the control, as was a derived sheep heading away from the
same wall, so the term reads travel direction rather than proximity. A derived
sheep with no velocity published an unevaluated zeroed record. A shape at exactly
the `6.25` look-ahead was named with a vector of exactly zero and one `0.05`
beyond it was not seen, so the boundary is continuous. A derived scenario with the
combined bound at `2.0` left the published avoidance vector at exactly `(4, 0)`
while the bound published a summed magnitude of exactly `4`, a scale of exactly
`0.5`, and an applied acceleration of exactly `(2, 0)`, so avoidance is bounded
with every other term rather than escaping the bound. Over 240 ticks the on member
recorded `0` contact ticks against the control's `4`, with the control first
clipped at tick 63 by a wall and at tick 71 by the outer bound; the closest any
avoiding sheep came to the wall face was `0.758357`. A derived scenario whose
maximum was lowered to `0.25` was overwhelmed: its sheep was still clipped, at
tick 65 rather than 63, by `left_wall`, and came to rest at exactly `16.5`, so the
hard collision authority still works when the steering term loses. A direct
comparison against a pre-change build ran all twenty-five earlier scenarios for
240 scripted ticks each and found every canonical dump byte-identical once the new
key and the version number were removed, so version 13 adds fields without
reinterpreting version 12 values. The oracle also proved that reversed sheep
storage preserved exact per-ID state and evidence, that restart was exact, and
that 600 enabled ticks allocated no heap memory. On WSL Ubuntu 24.04.4 with Clang
18.1.3, development, Release, and ASan/UBSan configurations each passed 24/24
CTests; formatting and bounded clang-tidy passed. The `4.0` maximum and the `6.25`
look-ahead derived from it inherit the provisional status of the accepted values
they come from. The paddock is flat, so the drop half is exercised only against
the paddock edge and nothing here is evidence about an interior ledge. This is
synthetic causal evidence rather than player-facing motion acceptance.

**Observed result (2026-08-21):** the required per-sheep arousal stimulus
advanced the state dump to version 14 when the four behavior transitions and the
explicitly non-physiological arousal proxy were implemented. The paired
transitions fixture enables no steering term, so every published position and
every evidence record was identical between the two members over 400 ticks and
only `arousal` and `behavior` differed; the off member stayed `settled` with an
arousal of exactly `0` on every tick. In one deterministic run the subject sheep
walked `settled` -> `alert` at tick `34`, `alert` -> `driven` at tick `98`,
`driven` -> `recovering` at tick `241`, and `recovering` -> `settled` at tick
`354`. Each ladder transition was produced by a prior arousal exactly on its
threshold — `0.25`, `0.75`, and `0.125` — and `recovering` was produced by the
stimulus falling to exactly the `0.125` rest threshold while the sheep still
carried `0.56640625`, so release rather than decay is what selects it. Arousal
rose by exactly `1/32` and decayed by exactly `1/256` per tick, and an exact
dog/sheep overlap drove it to exactly `1`. The sheep standing at exactly the
`8.0` stimulus radius published a stimulus of exactly `0` and never left
`settled`. The `nervous` and `stubborn` sheep at the same exact `5`-unit distance
published exactly `0.75` and `0.1875`, ending `driven` and `settled`
respectively, and a derived `nervous` sheep `1.0` from the dog clamped to exactly
`1`. Two derived runs holding exactly `0.625` — inside the `0.5`..`0.75` driven
hysteresis band — published `alert` in one and `driven` in the other with zero
label changes in 240 ticks each, and the `nervous` sheep sat on the exact `0.75`
entry threshold for 376 consecutive ticks without leaving `driven`. Under an
adversarial cause tuned so the published arousal crossed that entry threshold on
`377` of `400` ticks — 150 ticks above and 150 below in the measured tail — the
rule changed the published label twice in total and zero times after tick 100. A
derived run in which the *dog* approached, held, and left under scripted input
walked the same four states at ticks `35`, `88`, `244`, and `346`. The oracle
also proved that the same tick's dog move cannot alter the transition, that
reversed sheep storage preserved exact per-ID state and evidence, that restart
restored a non-zero starting arousal and label exactly, that an unknown behavior
state and an out-of-range arousal are both rejected by the writer, and that 600
enabled ticks allocated no heap memory. A direct comparison against a pre-change
build ran all twenty-seven earlier scenarios for 240 scripted ticks each and
found every canonical dump byte-identical once the new key and the version number
were removed, so version 14 adds a field without reinterpreting version 13
values. On WSL Ubuntu 24.04.4 with Clang 18.1.3, development, Release, and
ASan/UBSan configurations each passed 24/24 CTests; formatting and bounded
clang-tidy passed. Every rate and threshold is a provisional legibility choice
rather than a measured value, arousal is a named game parameter rather than a
physiological measurement, and no player has seen a sheep change state.

**Observed result (2026-08-22, native Windows, MSVC 19.44 x64):** making the
flock size scenario-owned advanced the state dump to version 15. A direct
comparison against a pre-change build ran all thirty pre-existing scenarios for
240 scripted ticks each under one scripted moving-dog input, dumping the
canonical state after construction and after every tick — 241 dumps per scenario,
7,230 in total. The raw dumps differ in exactly three tokens per dump: the
`"version"` value `14` becomes `15`, and each of the `previous` and `current`
objects gains `"sheep_count":5`. With those two substitutions applied, all thirty
files are byte-identical to the pre-change build, so no position, velocity,
heading, arousal, behavior, temperament, evidence record, or applied acceleration
moved. The new `fifty-sheep-paddock` scale fixture publishes fifty members,
allocates zero heap bytes across 600 ticks, reproduces its whole published
sequence in an independent run, and writes one record per active member into a
188,212-byte dump. `dev` passed 46/46 CTests and `release` 48/48; `format-check`
and `clang-tidy-check` were not run, because Clang 18 is absent on this host, and
`wide_eye.gameplay_simulation_stack_budget` is registered only on UNIX and
therefore did not run here either.

**Observed result (2026-08-16):** the earlier native Windows Release capture
commands wrote canonical version 2 state at presentation ticks 1, 61, and 121.
The independent normal, repeat-normal, and face-normal debug commands at tick 61
produced
byte-identical state files with SHA-256
`8b2921e4a87bc2e8b8b86e08f4f17d8b3a7bf7c9413293b90b03b728ec27a905`.
This is local same-platform evidence, not a cross-platform identity claim.

Native Linux graphics, a native Windows version 15 capture, JSON decoding, CLI
replay/seed ingestion, persistent replay files, the terrain pressure factor,
damping, avoidance against terrain rather than against the paddock edge, arousal
causes other than the dog, any effect of behavior state on steering, objective
outcomes,
and cross-platform state or text identity remain untested or unimplemented.
