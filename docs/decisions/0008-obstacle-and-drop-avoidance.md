# ADR 0008: Steering-level obstacle and drop avoidance

**Status:** Accepted; near-boundary contact behavior and response continuity
corrected 2026-08-22 (see the QA-003 and QA-005 correction sections below)
**Date:** 2026-08-21
**Decision owner:** Project owner

## Context

[ADR 0006](0006-combined-influence-acceleration-bound.md) bounded the sum of
every steering term and [ADR 0007](0007-bounded-sheep-speed-and-turning.md)
bounded the result of integrating it. Both decide how hard a sheep may be pushed
and how fast it may end up going. Neither looks at *where the sheep is going*.

The consequence was recorded as a standing Phase 3 limit: nothing steered a
sheep around a wall or away from a ledge. A sheep driven at the closed gate ran
into it and was stopped by `resolve_sheep_against_paddock`, which is a boundary
doing a steering job. The hard clip is the right last authority and the wrong
first response: it produces a sheep pinned motionless against a fence rather
than a sheep that turned along it.

The accepted design already asks for the behavior — "prefer a visible route and
avoid fences, water, cliffs, and blocked gates" — and the accepted plan lists
dog avoidance and obstacle scenarios as modeling ingredients. This is the
remaining half of one already-approved roadmap item, not a new system.

## Decision

- One scenario-owned `SheepAvoidanceConfiguration` names a `look_ahead_distance`
  and a `maximum_acceleration`, is independently switchable like every other
  term, and is validated once at simulation construction beside the other
  enabled-term checks rather than on the fixed-tick path.
- **Avoidance is a steering term, not a second collision rule.** It publishes one
  acceleration vector, that vector is added to the same sum as every other term,
  and the combined-influence bound scales it exactly as it scales the rest. It
  never moves a sheep by itself, never rewrites another term's vector, and never
  consults or modifies the collision stage.
- **The hard boundary is unchanged and is not reordered.**
  `resolve_sheep_against_paddock` still runs last, still refuses a displacement,
  and still clears the blocked axis on the contact tick. A steering nudge that
  can be overwhelmed is not a boundary, so the boundary stays.
- Both halves probe along the sheep's **own prior direction of travel**. A sheep
  whose prior planar speed is at or below the shared
  `kSheepHeadingMotionSpeedFloor` has no direction to probe, so the term does not
  run for it at all and publishes an unevaluated record. Avoidance is therefore
  direction-aware rather than proximity-aware: a sheep standing beside a wall, or
  running parallel to one, feels nothing.
- **Obstacles** are the analytic paddock shapes, queried through a new
  `PaddockCollisionField::approaching_obstacle`. It sweeps the body against the
  *same radius-expanded rectangles* `resolve_cylinder_move` stops it against, so
  the shape a sheep steers around and the shape that refuses its displacement can
  never be two different shapes. Voxel faces and render meshes do not
  participate, and no second obstacle representation is introduced.
- The response direction is the outward normal of the face the sheep would enter,
  **plus** — only when the geometry names a nearer free edge of that same shape
  *and* that edge lies within the same look-ahead — the direction along the face
  toward it. That turns the push exactly halfway toward the way round, so the
  sheep is deflected as well as slowed. An edge further away than the sheep can
  see is not a way round it can take, and a sheep exactly between the two edges
  has no nearer edge at all; both keep the pure away-from-the-face push rather
  than committing to a side the geometry did not name.
- The magnitude falls off linearly over the look-ahead — the same shape the
  accepted dog terms use over their radius — so a shape exactly at the look-ahead
  distance is published by name with a vector of exactly zero and the boundary is
  continuous rather than a step.
- **Drops** are where `ground_height` is not finite. The term probes the ground
  under the look-ahead point; when it is not finite, the response is the full
  maximum straight back along the sheep's own approach.
- The two halves are summed and the total is held to the term's own
  `maximum_acceleration`, exactly as close-range separation holds the sum of its
  per-neighbour pushes, so reacting to two things at once cannot make avoidance
  the strongest influence in the flock.
- Each sheep publishes a per-tick avoidance record: whether the term ran, the
  named shape ahead and its distance, whether a drop is ahead, and the
  acceleration vector produced. The state dump advances to version 13.

### The magnitudes are derived, not picked

`maximum_acceleration` is `4.0` world units/s², the same value as close-range
separation's maximum and the combined-influence bound: **no avoidance push is
stronger than the strongest single influence the flock already accepts.**

`look_ahead_distance` is `6.25` world units, and it is derived from that. Under
the linear falloff, a term of maximum `A` acting over a look-ahead `L` removes
`A * L / 2` of kinetic energy per unit mass, so a sheep travelling straight at a
face at speed `v` is brought to rest exactly at that face when `L = v² / A`.
With the accepted `5.0` maximum sheep speed from ADR 0007 and the `4.0` above,
that is exactly `25 / 4`. The term has exactly enough room to stop the fastest
sheep the game allows, and no more.

**Both magnitudes inherit the provisional status of the accepted values they are
derived from.** Neither has been calibrated against player-facing motion, and the
derivation is a continuous-time argument: the 60 Hz Euler integration the game
actually runs stops a sheep short of the face rather than on it.

### Why the drop response was binary (superseded by QA-005)

`ground_height` answers a point question — is there ground here — not a distance
question. Two known points (the sheep, which is standing on ground, and the
look-ahead probe, which is not) do not locate the edge between them. Grading the
response, or steering *along* a ledge instead of retreating from it, would need a
boundary shape the query does not expose.

The alternative was to clip the probe against the paddock's own outer bounds,
which would give an exact distance. That was rejected because it hard-codes the
flat-paddock assumption into a sheep rule: it would stop working the moment
terrain exists, and it would make "the drop" a synonym for "the paddock edge" in
code rather than only in today's evidence.

**The consequence is that the term is verified only against the paddock edge.**
The handcrafted paddock has one constant ground height, so there is no interior
drop and none of the evidence below exercises a ledge inside the world. Phase 5
procedural terrain is when this half gets a real test.

### Known limits recorded rather than hidden

- **The rule considers one obstacle at a time.** It reacts to the nearest shape
  along the path and names that shape's nearer free edge, so the lateral escape
  can point toward another obstacle — steering around the closed gate's `+x` end
  aims at ground the right wall occupies. Avoidance is a local steering nudge,
  not a route planner, and the hard boundary is what makes that safe.
- **A stationary sheep is not avoided for.** A sheep that has been stopped — by
  the clip, or by avoidance itself — publishes an unevaluated record and stays
  where it is. That is intended: it is not moving into anything. It does mean a
  sheep can come to rest very close to an edge.
- **This does not fix, and must not be read as fixing,
  [QA-001](../qa/closed/QA-001-paddock-collision-radius-band-passthrough.md).** A
  body that starts inside an obstacle's radius band still passes through it; the
  fixtures that reproduce that defect do not enable avoidance and still
  reproduce it. (QA-001 was corrected separately on 2026-08-22 in the hard
  boundary, not here — see
  [ADR 0005](0005-paddock-collision-ownership.md#correction--a-body-that-starts-inside-a-shape-2026-08-22-qa-001).
  This decision's limit is unchanged: avoidance is still a steering nudge and
  still does not answer for a body that is already inside a shape.)

### Alternatives considered and rejected for now

- **Making avoidance a collision-stage correction** — clamping the displacement
  before the paddock sees it. Rejected outright: it would move a sheep without
  appearing in any published acceleration, breaking the contract that applied
  acceleration is what integration used, and it would make a steering behavior
  invisible to the paired-fixture oracles.
- **Exempting avoidance from the combined-influence bound** so a safety term can
  always win. Rejected: it would make avoidance a special case that can exceed
  the strongest accepted influence, and the whole point of ADR 0006 is that no
  combination may. The hard boundary is what guarantees safety, not this term.
- **Parallel or angled whisker probes** instead of the analytic face query.
  Rejected because they need a probe count or a probe angle that nothing in this
  project has observed, and because they answer "is something there" where the
  analytic shapes can answer "which face, how far, and where does it end".
- **Always taking the nearer free edge, however far away.** Rejected because a
  sheep at the middle of a fourteen-unit wall would commit to sliding six units
  sideways on the strength of a rounding difference between two nearly equal
  distances. Requiring the edge to be within the look-ahead makes the choice mean
  "there is a way round I can reach".
- **A repulsion from the nearest point of every obstacle regardless of heading.**
  Rejected as proximity-aware rather than direction-aware: it would push a
  resting flock off every wall permanently and would fire for sheep travelling
  away from the thing being avoided.

## Consequences

- No per-term vector, no combined-influence scale, no motion limit, and no
  dog-stimulus value changes. A scenario with the term switched off is
  byte-identical to the previous behavior; all 25 pre-existing scenarios were
  measured byte-identical over 240 scripted ticks after normalizing the new key
  and the version number.
- The state dump advances to version 13 for the new per-sheep record. The writer
  requires an unevaluated record to leave every field zero, requires a published
  distance to name a shape, and requires a push to name either a shape or a drop.
- The term is a per-sheep, per-tick pure function of that sheep's prior state and
  the analytic field. It holds no state, allocates nothing, and does not depend
  on buffer order.
- `PaddockCollisionField` gains one read-only query and no new shapes. Dog
  collision, sheep collision, the sight line, and avoidance all read the same
  three rectangles.
- The published evidence is its own per-sheep record rather than fields folded
  into the existing paddock-contact record. That choice is about ownership —
  collision is a later authority than steering and the two must not share a
  record — and it costs nothing: measured with Clang 18, both shapes are exactly
  48 bytes per sheep, because padding absorbs the duplicated subject ID either
  way.
- This completes its roadmap item. What it does **not** do is make the flock
  navigate: there is no route planning, no memory of what a sheep has already
  bounced off, and no interaction between avoidance and the unimplemented
  behavior states.

## Correction — the near boundary (2026-08-22, QA-003)

This section corrects the decision above rather than restating it. It was written
after [QA-003](../qa/closed/QA-003-avoidance-flaps-between-maximum-and-zero-at-a-contact-face.md)
measured the rule as accepted and found the near end of the look-ahead
discontinuous at exactly the position the collision authority parks a sheep at.

**What was wrong.** `approaching_obstacle` treated a body that merely touched the
boundary of a radius-expanded rectangle as being inside it, and clamped a
negative entry fraction to zero so that such a body was reported as contacting
the shape *now*, at a distance of zero, through whichever slab it had entered
last — for a body travelling along a face, one it had already passed. The linear
falloff turned that zero distance into the term's full maximum, pointing
backwards along the sheep's own travel. Because that push moved the sheep off the
contact line, the next tick reported nothing at all, and any influence that
pressed the sheep back onto the face restarted the cycle: the term alternated
between `4.0` and exactly `0` for as long as a sheep was held on a wall.

**What changed.** Two corrections, both inside the query, neither touching the
collision authority:

- A reported contact now always lies **at or ahead of the body**. A body already
  inside an expanded rectangle has no face ahead of it and reports no obstacle;
  pushing such a body out belongs to the collision authority, which
  [QA-001](../qa/closed/QA-001-paddock-collision-radius-band-passthrough.md) made
  it do on 2026-08-22.
- Touching a rectangle's boundary while travelling along it is **not** being
  inside it. `overlaps`, the test the hard clip applies to the axis a body is not
  moving along, is strict for exactly this reason — it is what lets a body parked
  at face plus radius slide along that face — so the sweep now agrees with it.
  This is the decision's own promise that the shape a sheep steers around and the
  shape that refuses its displacement can never be two different shapes.

**What a contact distance of zero now means, and why that is the right answer.**
It means the body is touching the face it is moving into, right now, and the
falloff's maximum is the correct response to it: it points out of that face and
it is the *same* answer on every tick the sheep keeps pressing into it, so a
sheep held against a wall gets a steady push rather than an alternating one. A
sheep travelling along a face it touches gets nothing, which is this decision's
accepted "a sheep standing beside a wall, or running parallel to one, feels
nothing at all" applied at zero clearance. The alternative — a nonzero response
for a body touching a face and travelling along it — was rejected on the
evidence: any such push moves the body off the contact, the term then switches
off, and the alternation returns. Making it continuous instead would require the
graded proximity repulsion this decision already rejected as proximity-aware
rather than direction-aware.

**Continuity.** The far boundary was already continuous; the near boundary now is
too. `0.5` clear of a face still reports `0.5`, a hundredth clear still reports a
hundredth of the way along the path, and zero clear reports zero. The step that
used to sit between "touching" and "a billionth clear" is gone.

**What did not change.** No published field, contract version, scenario, or
magnitude. Measured on WSL Ubuntu 24.04.4 with Clang 18.1.3, the canonical state
dumps of 29 of the 30 named scenarios are byte-identical over 240 scripted ticks;
only `sheep-all-influences-diagnostic` differs, first at tick 113, which is the
first tick in any fixture where a sheep is held on a contact face. Every accepted
avoidance measurement is unchanged, including zero clips with the term on against
four with it off, and a closest approach to the wall face of `0.758357`.

**What is still wrong, and is not corrected here.** The response is bang-bang
rather than proportional to the closing it corrects. A sheep a hundredth of a
unit clear of a face with a near-parallel heading has its contact a full unit
ahead, so the falloff answers with most of the maximum — a push hundreds of times
larger than the closing rate justifies — and the drop half remains binary by the
design above. Both still alternate on a two-to-four tick cycle, and both are
filed as [QA-005](../qa/closed/QA-005-avoidance-response-is-bang-bang-near-a-face-and-at-the-drop-boundary.md).
Correcting them means changing the *shape* of the response, which is a decision
for this ADR's owner rather than a defect fix.

## Correction — closing-speed and ground-boundary response (2026-08-22, QA-005)

This owner-approved correction supersedes the binary-drop section and the
distance-only magnitude above. It preserves avoidance as a bounded steering
term, preserves the hard collision authority as the last positional authority,
and changes no published field or format version.

**What was wrong.** The obstacle falloff used distance but not actual speed into
the named face, while the drop half used neither distance nor a boundary normal.
A grazing or very slow diagonal drift could therefore receive nearly the same
push as a fast head-on approach. Near the outer-wall corners, the obstacle's
lateral suggestion could also point toward an expanded edge that coincided with
the last legal centre position, while the drop response reversed the entire
travel vector. Those responses repeatedly removed and recreated one another's
input instead of settling.

**Geometry ownership.** `PaddockCollisionField` now exposes the first exact
ground-boundary distance along a planar path and the normal back into grounded
space. The current implementation is exact for the field's rectangular finite
ground; a future terrain implementation must keep the query at this ownership
boundary rather than teach sheep rules terrain bounds. Obstacle lateral escape
also requires legal centre space beyond the radius-expanded edge. An outer edge
coincident with the paddock's cylinder limit is therefore not a route around a
shape; a farther reachable edge may still be named, and the existing look-ahead
test rejects it when it is out of reach.

**Response shape.** Let `A` be the term maximum, `L` its look-ahead, `d` the
reported contact distance, `c` actual prior velocity into the boundary normal,
and `V = sqrt(A * L)` the reference speed from which the look-ahead was derived.
Below saturation, the boundary-normal acceleration is
`A * (1 - d / L) * 2c / V`. The factor of two retains the accepted linear
stopping integral for a head-on approach, while using `c` rather than a
normalized direction component makes an arbitrarily slow drift produce an
arbitrarily small response. When an obstacle adds a lateral escape, the total
vector is compensated before normalization so its component through the face
keeps that same value; the term maximum still caps the result. Drop response
uses the same distance and closing-speed shape but points only along the inward
ground normal. If both halves act, they sum and the existing term maximum clamps
the sum as before.

**Observed result.** On WSL Ubuntu 24.04.4 with Clang 18.1.3, the focused
gameplay oracle measures the grazing fixture at `2` flaps in 240 ticks, down from
`36`. In the 600-tick all-influences diagnostic, worst-sheep avoidance is `4`
flaps with a longest run of `1`, and the applied sum is `9` with a run of `2`;
both now meet the shared five-per-hundred, four-consecutive-tick allowance. The
paired avoidance fixture still records zero hard clips with avoidance on versus
four with it off. Its corrected closest wall gap is `0.172705`; the corrected
drop rest position is `x = 0.919383`. These are deterministic scenario evidence,
not a player-facing tuning verdict.

**Remaining limit.** The rule still considers one obstacle at a time. A lateral
edge can therefore lead toward a neighbouring obstacle, and the hard collision
authority remains the safety boundary. The new regression covers the distinct
outer-wall/drop corner where an edge is physically unreachable, not general
route planning or an interior terrain ledge.
