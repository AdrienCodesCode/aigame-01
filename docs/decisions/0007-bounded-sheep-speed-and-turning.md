# ADR 0007: Bounded sheep speed, and a heading that follows motion under a turn rate

**Status:** Accepted
**Date:** 2026-08-21
**Decision owner:** Project owner

## Context

[ADR 0006](0006-combined-influence-acceleration-bound.md) bounded the *sum* of
every published steering term, so no combination of influences can accelerate a
sheep harder than the strongest single influence the flock already accepts. It
deliberately did not bound the result of that acceleration. Two gaps remained,
and the Phase 3 checkpoint recorded both as standing limits:

- A bounded acceleration applied for long enough still accumulates without
  limit. Two seconds under the `4.0` bound is `8.0` world units/s — faster than
  the dog can sprint — so a sheep could outrun the animal herding it, which
  removes the core loop rather than making it hard.
- Nothing decided which way a sheep faced. `heading_radians` was authoritative
  state that the social fixtures never wrote, so a sheep drifting south still
  published the heading its fixture started with. Presentation reads that
  heading for the proxy transform, the dog-facing term reads it to publish a
  relative bearing, and neither had anything truthful to read.

The accepted implementation plan names the same order —
"apply bounded acceleration, speed, and turn rate after combining inspectable
influences" — so this is the second half of one already-approved rule, not a new
system.

## Decision

- One scenario-owned `SheepMotionLimitConfiguration` names a `maximum_speed` and
  a `maximum_turn_rate_radians_per_second`, and is independently switchable like
  every other term. Its bounds are validated once at simulation construction,
  beside the other enabled-term checks, not on the fixed-tick path.
- Both limits act on the **result of integration**, at the single place the
  applied acceleration has already become a velocity. They therefore run after
  the combined-influence bound and before the paddock resolves the displacement.
  No published per-term vector, and no published combined-influence scale, is
  rewritten: those records still say exactly what the terms asked for, and the
  new record says what the sheep ended up doing.
- Collision remains the last positional authority and is not reordered. The
  paddock still refuses a displacement after both limits have run, and a clipped
  axis still loses its velocity on the contact tick.
- **Speed** is a clamp with the direction preserved: if the integrated planar
  speed exceeds the maximum, both components are scaled by
  `maximum / integrated`. A sheep under the maximum takes no arithmetic at all,
  so it stays byte-identical rather than merely multiplied by one.
- **Heading follows motion.** The target is the direction of the sheep's own
  applied velocity, and the heading rotates toward it along the shorter arc by at
  most `turn_rate * fixed_delta` per tick. It is derived from the *prior*
  heading, so this tick's published dog bearing — which is relative to that prior
  heading — cannot be altered retroactively by the rotation.
- The shortest-arc rule is the dog motor's `approach_angle`, moved from
  `dog_controller.cpp` into shared `game/math.hpp` and now used by both animals.
  A second, separately written turn limit would be a silent behavior difference
  between the dog and the sheep rather than a designed one.
- A sheep whose applied planar speed is at or below
  `kSheepHeadingMotionSpeedFloor` (`1e-9` world units/s) keeps its previous
  heading and publishes no motion direction. A sheep whose steering terms exactly
  cancel ends the tick at exactly zero, and `atan2(0, 0)` is `0`, so without the
  floor an idle flock would all snap to face north. The residue left by two
  order-one terms cancelling is around `1e-16`; a real influence produces a
  velocity around `1e-2` after one tick. The floor sits between those by many
  orders of magnitude, and at `1e-9` world units/s a sheep covers under a
  micrometre in an hour of play, so nothing observable is discarded. It is a
  numerical-noise floor rather than a design parameter, so it is a named sheep
  rule rather than scenario configuration.
- Each sheep publishes a per-tick motion-limit record: the pre-clamp integrated
  speed, the scale the clamp applied, the speed it kept, whether the heading
  followed motion, the motion direction, and the signed rotation actually
  applied. The state dump advances to version 12.

### The turn rate limits the heading only

The turn rate constrains **which way a sheep faces**. It does not constrain the
direction a sheep travels. A sheep pushed sideways by separation still moves
sideways immediately, while its heading catches up over the following ticks.

This is the narrowest choice that isolates one variable, and it is chosen
deliberately:

- Slaving motion to heading is a different motion model, not a limit. It would
  make lateral acceleration impossible, so close-range separation, the two
  attraction terms, and the dog's away-vector would all stop expressing
  themselves the way their accepted fixtures observe. Every paired oracle that
  pins an exact per-term acceleration against an exact velocity change would have
  to be re-derived against a vehicle model that no observation currently
  justifies.
- It would also break the published contract that `applied_acceleration` is what
  integration used: the velocity would no longer be the prior velocity plus the
  published acceleration, because an unseen projection would have removed part of
  it.
- The dog motor's own precedent is not a projection either. It scales *speed*
  down during a large heading change (`turn_speed_scale`) and leaves the movement
  direction to the input. Copying that would add a second behavior — a speed
  penalty — to an outcome whose job is one isolated variable.

**The consequence is recorded rather than hidden:** until this is revisited, a
sheep can visibly slide in a direction it is not facing for up to about eight
tenths of a second (a full reversal takes 51 ticks at the accepted rate). That is
a known
simulation smell. Making motion follow heading, or adding a heading-error speed
penalty like the dog's, is a separate decision with its own fixture and its own
evidence, and it would supersede this section rather than extend it silently.

### The magnitudes

`maximum_speed` is `5.0` world units/s. The dog's accepted walk is `4.5` and its
accepted sprint is `8.0`, so a frightened sheep escapes a walking dog — the
player has to commit to a sprint — and the dog can always overtake and flank the
flock. A herding game in which a sheep can outrun the dog has no core loop.

`maximum_turn_rate_radians_per_second` is `3.75`, against the dog motor's
accepted `6.0`. The rule is that a sheep may not turn as fast as the dog cutting
across it, because that is the player's main lever. Among the rates below the
dog's whose per-tick budget at 60 Hz is an exactly representable binary fraction,
`3.75` is the largest: its budget is exactly `1/16` radian per tick, which lets
the paired oracle pin every turn observation with equality rather than with a
tolerance — the same reason the temperament factors are exact powers of two.

**Both magnitudes are provisional legibility choices, not measured, tuned, or
observed values.** Neither has been calibrated against player-facing motion, and
nothing here claims either is the right number for the finished game.

### Alternatives considered and rejected for now

- **Bounding the per-tick velocity change instead.** Already rejected in ADR
  0006 as the acceleration bound in disguise; it also cannot bound the
  accumulated speed, which is the gap this ADR exists to close.
- **Damping (a drag term proportional to speed).** A sheep would slow smoothly
  instead of hitting a wall at the maximum, which is probably better motion.
  Rejected here because damping changes every tick of every scenario rather than
  only the ticks that breach a limit, so it is not a limit and it cannot be
  observed as one isolated variable against a paired control. It remains an
  unimplemented roadmap item in its own right.
- **A speed penalty during large heading changes, copied from the dog motor.**
  Rejected as a second behavior in a one-variable outcome; see above.
- **Making the heading floor scenario-configurable.** Rejected because a
  scenario that could raise it would be choosing when a sheep stops steering,
  which is a behavior decision disguised as a tolerance. The floor exists only to
  refuse a direction that rounding invented.

## Consequences

- No per-term vector, no combined-influence scale, and no dog-stimulus value
  changes. The published relationship `applied == sum * applied_scale` still
  holds; what is new is that the velocity and heading the sheep ends the tick
  with are no longer that acceleration's unconstrained consequence.
- `dog_relative_bearing_radians` stays relative to the prior heading and is
  computed before the rotation, so a turning sheep publishes the bearing it
  actually reacted to. The paired oracle observes this directly for four
  consecutive ticks of a reversing sheep.
- A scenario with the term switched off is byte-identical to the previous
  behavior. All 23 pre-existing scenarios were measured byte-identical over 240
  scripted ticks after normalizing the new key and the version number, so no
  accepted measurement is invalidated. Flock polarization is computed from
  published sheep *velocity*, not heading, so the accepted 60-tick alignment
  comparison could not have moved even if the rule had been switched on there.
- The limits are a per-sheep, per-tick pure function of that tick's integrated
  velocity and the prior heading. They hold no state, allocate nothing, and do
  not depend on buffer order.
- The state dump advances to version 12 for the new per-sheep record. The writer
  requires an evaluated record to publish a scale within `(0, 1]` and a result no
  faster than the integration that produced it, requires a sheep that did not
  follow motion to publish no motion direction, and requires an unevaluated
  record to leave every field zero.
- The clamp is exact on the components. Recomputing the magnitude of a clamped
  velocity can land one unit in the last place above the maximum for an arbitrary
  direction; the paired fixture's axis-aligned and 3-4-5 cases land on exactly
  the maximum because their scale is exactly representable.
- This still does not complete its roadmap item. Obstacle and drop avoidance
  remain unimplemented, and neither limit is a substitute: nothing yet steers a
  sheep around a wall or away from a ledge, and the hard collision authority
  remains the separate later stage it already was.
