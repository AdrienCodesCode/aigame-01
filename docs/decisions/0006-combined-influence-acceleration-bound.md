# ADR 0006: Combined-influence acceleration bound by scaling the summed terms

**Status:** Accepted
**Date:** 2026-08-21
**Decision owner:** Project owner

## Context

Sheep steering is built from independently switchable terms. Close-range
separation, two-neighbour attraction, one-neighbour alignment, dog distance
pressure, dog approach, and dog facing each publish their own acceleration vector
and each bound their own magnitude with a scenario-owned maximum. Line of sight
and temperament add no vector: the first releases the dog terms, the second
scales them.

`advance_sheep_from_prior` then added those six vectors and integrated the sum
directly. Nothing bounded the sum. A sheep standing inside several overlapping
influences was therefore accelerated by the unbounded total of every maximum that
happened to apply, and a nervous sheep multiplied three of those maxima at once
before they were added. The Phase 3 checkpoint recorded that as a standing known
limit and named the combined bound as the next item, because the amount of
acceleration a sheep can receive is a property of the flock rule, not something
that should fall out of how many terms a scenario enables.

The roadmap item deliberately left the combination rule undecided — clamp,
prioritized ordering, per-term weighting, or another explicit scheme — and
required the choice to be made before implementation.

## Decision

- One scenario-owned `SheepCombinedInfluenceConfiguration` names a
  `maximum_acceleration` and is independently switchable like every other term.
  Its bounds are validated once at simulation construction, beside the other
  enabled-term checks, not on the fixed-tick path.
- The rule is a clamp on the **sum**: every published per-term acceleration is
  added exactly as before, and if the magnitude of that sum exceeds the bound,
  the sum as a whole is scaled to the bound. No individual term is rewritten,
  reordered, prioritized, or weighted.
- The applied scale is published per sheep as prior-state-derived evidence, along
  with the pre-bound summed magnitude and the acceleration integration actually
  used. It is exactly `1.0` whenever the bound did not bind, including when the
  term is switched off.
- The bound is applied at the single place the terms become one acceleration, in
  `advance_sheep_from_prior`, before integration and before collision.
- `maximum_acceleration` defaults to `4.0`: the largest single accepted per-term
  maximum, which is close-range separation's. The rule this states is that no
  combination of influences may accelerate a sheep harder than the strongest
  single influence the flock already accepts on its own. As a second reading,
  `4.0` world units/s² is roughly one plausible sheep sprint speed gained per
  second, so the two justifications agree. **That magnitude is a provisional
  legibility choice, not a measured, tuned, or observed value.** It has not been
  calibrated against player-facing motion, and nothing here claims it is the
  right number for the finished game.
- Collision stays a separate, later positional authority. The bound limits
  steering; it never decides where a sheep ends up.

### Alternatives considered and rejected for now

- **Prioritized ordering** — spend the acceleration budget on the most urgent
  term first (separation, then the dog terms, then the flock terms) and give the
  remainder to the rest. Rejected because it changes what a term produced: a
  fully or partially starved term would publish a vector that is no longer the
  vector the term computed, or the published vector would stop describing what
  was applied. Every paired-fixture oracle that pins exact per-term arithmetic
  reads those vectors, and the ordering itself would become a hidden tuning
  parameter with no evidence behind it.
- **Per-term weighting** — give each term a scenario-owned weight and sum the
  weighted vectors. Rejected for the same reason plus a worse one: a weight
  silently rewrites each vector before it is published, so the accepted
  distance-only pressure, the approach saturation, the facing cosine, and the
  temperament ratio would all need re-deriving against a second multiplier that
  no observation currently justifies. Weighting is also not a bound: a weighted
  sum can still exceed any limit.
- **Bounding the per-tick velocity change instead of the acceleration.** Rejected
  because at a fixed 60 Hz that is the same rule expressed in units that hide the
  60, and because bounded speed is its own later roadmap item with its own
  evidence.

The rejected schemes remain reopenable. If flock motion later needs one term to
dominate another, that is a separate decision with its own fixture and its own
evidence, and it would supersede this ADR rather than extend it silently.

## Consequences

- Every per-term vector keeps its published value, so the existing evidence is
  exactly as inspectable as it was and the oracles that pin exact per-term
  arithmetic survive unchanged. What changed is the relationship between the
  published terms and the applied acceleration: it is now
  `applied == sum * applied_scale` rather than `applied == sum`. The affected
  oracles were updated to assert the new identity.
- A scenario whose sum never reaches the bound is byte-identical to the previous
  behavior, not merely multiplied by one: the scaling arithmetic is skipped
  entirely when the sum is under the bound. All 21 pre-existing scenarios were
  measured byte-identical over 240 scripted ticks after normalizing the new key
  and the version number.
- The state dump advances to version 11 for the new per-sheep record. The writer
  requires an evaluated bound to publish a scale within `(0, 1]` — the rule is a
  bound, not a gain — and requires an unevaluated one to leave every field zero.
- The published scale is the only place the difference between "what the terms
  asked for" and "what integration used" lives, so a sheep that moves less than
  its published vectors suggest is explainable from the dump alone.
- The bound is a per-sheep, per-tick pure function of that tick's terms. It holds
  no state, allocates nothing, and does not depend on buffer order.
- This does not deliver the rest of its roadmap item. Bounded sheep speed,
  bounded turning, and obstacle/drop avoidance remain unimplemented, and this
  bound is not a substitute for any of them: it limits how hard a sheep can be
  accelerated, not how fast it can end up moving, how quickly it can turn, or
  whether it steers around anything.
