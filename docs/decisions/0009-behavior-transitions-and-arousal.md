# ADR 0009: Behavior transitions and the non-physiological arousal proxy

**Status:** Accepted
**Date:** 2026-08-21
**Decision owner:** Project owner

## Context

`SheepBehaviorState` and `SheepState::arousal` have existed in the authoritative
sheep buffer since the first sheep landed, and nothing has ever written either
one. Every sheep in every scenario has therefore been permanently `settled` with
an arousal of exactly zero, while the accepted design asks the model to
"distinguish settled, alert, driven, and recovering behavior so sheep do not move
like constant-speed boids" and to "accumulate stress ... and recover when
pressure is released".

Eight accepted rules now decide how hard a sheep is pushed, how fast it may end
up moving, and where it is allowed to go. None of them says what the sheep *is*.
Without that, the moment-to-moment loop's `Release` and `Recover` verbs have
nothing to act on and the deferred flock observables — response latency, settle
time, split/rejoin time — have no state to measure against.

The research record is explicit about the risk in naming this variable:
"'Arousal,' 'temperament,' and other game variables can become misleading if
documented as direct measures of physiology or personality," and "the stress
value is a gameplay-facing hypothesis, not a direct cortisol model. Name it
`arousal` internally until evidence establishes what it represents."

## Decision

### Arousal is a named game parameter, not a claim about animal physiology

This is a design constraint, not a caveat. Nothing in this project has measured
an animal. `arousal` is a bounded `[0, 1]` design variable whose only job is to
select one of four labels. It is not a heart rate, a stress hormone, a cortisol
level, a fear intensity, or any other physiological quantity, and no comment,
identifier, document sentence, or evidence key may imply otherwise. The internal
name stays `arousal` precisely because the research record asked for a name that
does not commit to a meaning; the player-facing word "stress", if it is ever used
at all, is shorthand chosen for players and not a second claim.

The bounds `kSheepMinimumArousal` and `kSheepMaximumArousal` live beside the
field in `sheep_state.hpp` and the state-dump writer rejects a value outside
them: a bounded design parameter that can leave its range is not bounded.

### Arousal is a rate-limited follower of the published dog stimulus

One scenario-owned `SheepBehaviorConfiguration` names two rates and four
thresholds, is independently switchable like every other rule, and is validated
once at simulation construction beside the other enabled-term checks.

The cause is `arousal_stimulus`: the *same* linear distance falloff the three
accepted dog terms already use, over the *same* scenario-owned pressure radius,
released by the *same* line-of-sight rule and scaled by the *same* temperament
factor — expressed as a dimensionless `[0, 1]` fraction instead of as an
acceleration, and clamped, because a nervous sheep's response scale can carry the
product above one. Reusing the accepted geometry rather than introducing a second
radius is deliberate: a sheep's stimulus boundary and its pressure boundary can
never disagree about where the dog stops mattering.

Arousal then moves toward that stimulus at up to `rise_rate_per_second` while the
stimulus is higher and at up to `recovery_rate_per_second` while it is lower, and
is assigned exactly when the stimulus is within one tick's budget. Two properties
follow directly and are worth more than the model's realism:

- it stays inside `[0, 1]` without a clamp, because both endpoints do; and
- it **cannot overshoot**, so it can never oscillate on its own — every
  oscillation observable in arousal is an oscillation in the cause.

`rise_rate_per_second` is `1.875` and `recovery_rate_per_second` is exactly one
eighth of it, `0.234375`. Under a saturated stimulus a sheep reaches full arousal
in 32 ticks — about half a second — and needs 256 ticks — about four seconds — to
shed it, because the accepted loop makes *release* the slow, deliberate half of
the pressure/release pair: "widen or pause to preserve cohesion and let sheep
settle." Both rates are chosen so that one tick at 60 Hz is an exactly
representable binary fraction, `1/32` and `1/256`, which lets the paired oracle
pin them with equality rather than with a tolerance. **Both are provisional
legibility choices, not measured or tuned values.**

### Four states, selected from prior state with explicit hysteresis

The transition reads only prior state: the prior label, the arousal that label
produced, and the prior-state stimulus. Nothing reads the state being written, so
the same tick's arousal update cannot change the label, and the dump's
`previous.sheep[i].arousal` plus `current.…evidence[i].arousal_stimulus` are
exactly the two numbers that produced `current.sheep[i].behavior`.

- `settled` — this sheep is not engaged: either no cause has lifted its arousal
  as far as the alert threshold, or it has already come all the way back to rest.
- `alert` — a cause is acting and arousal has passed `alert_arousal` but not
  `driven_arousal`.
- `driven` — a cause is acting and arousal has passed `driven_arousal`.
- `recovering` — the cause has been released and arousal has not yet returned to
  rest. It is the one state a sheep can never be in while a cause is acting on
  it, which is what makes release a verb rather than an absence.

**The hysteresis rule is a Schmitt trigger on arousal: each band is entered at
its named arousal and left only at a lower one.** `rest_arousal` is `0.125`,
`alert_arousal` is `0.25`, `driven_release_arousal` is `0.5`, and
`driven_arousal` is `0.75` — all exact binary fractions, all provisional
legibility choices. `rest_arousal` does double duty as the "is a cause acting at
all" test on the stimulus, because a stimulus too weak to lift a sheep out of
rest is not pressure; using one number for both keeps the ladder and the release
test from disagreeing about whether this sheep is under pressure.

Separate thresholds were chosen over a dwell timer for two reasons. A dwell timer
would need a per-sheep tick counter in the authoritative buffer — new state, more
stack in a harness that is already close to its limit (QA-002), and a new field
in the replay contract — and it delays a transition rather than preventing one:
it makes a flapping sheep flap slower. Separate thresholds are a pure function of
values that are already published, and they *imply* a dwell anyway: the driven
band is `0.25` wide and the alert band `0.125`, which at the recovery rate is 64
and 32 ticks respectively before either can be left downward, and at the faster
rise rate still 8 and 4 ticks.

### Behavior is observational in this outcome

**No steering term reads the arousal or the label.** This is deliberate, and it
is the single most important scope decision here: a feedback loop from behavior
into steering would re-derive every accepted per-term oracle at once, and this
project changes one isolated variable at a time. The paired fixture proves the
claim rather than asserting it — with the switch off and on, every published
position, velocity, heading, and every social, dog-stimulus, collision,
avoidance, combined-influence, and motion-limit record is identical, and only
`arousal` and `behavior` differ.

### Evidence lives on the records that already exist

`arousal` and `behavior` are already fields of `SheepState`, so the state a
transition produced needs no new storage at all. The cause needs one number, and
it is published as `arousal_stimulus` on the existing `SheepDogPressureEvidence`
rather than as a new parallel per-sheep array. That is an ownership judgement —
the dog stimulus is the only cause that drives arousal in this outcome, so the
record that already explains the dog stimulus is where the fraction it produced
belongs — and unlike ADR 0008's avoidance record it is also cheaper: measured
with Clang 18, a separate record costs 144 bytes per sheep against the extended
record's 136, because padding does not absorb the duplicated subject ID here.
The stimulus is published whether the transitions are switched on or off, so a
paired control publishes an identical cause and only the applied arousal differs.

The state dump advances to version 14.

## Consequences

- No steering term, no bound, no motion limit, and no collision result changes.
  All 27 pre-existing scenarios were measured byte-identical over 240 scripted
  ticks after normalizing the new key and the version number.
- The state-dump writer gains two rules: an arousal outside its stated range is
  rejected, and an unevaluated stimulus must leave `arousal_stimulus` zero. The
  existing rejection of an unknown behavior state is unchanged and still holds in
  `flock_observables` as well as in the writer.
- The rule is a per-sheep, per-tick pure function of that sheep's prior state and
  one published number. It holds no extra state, allocates nothing, and does not
  depend on buffer order.
- Arousal is bounded, so it cannot accumulate without limit the way the accepted
  design's word "accumulate" might suggest. That is intentional: an unbounded
  accumulator has no meaningful thresholds and no reproducible recovery time.

### Known limits recorded rather than hidden

- **The dog is the only cause.** The accepted design also lists excessive speed,
  isolation, and sudden direction changes as things a sheep should accumulate
  stress from. None of them drives arousal here. Adding one is a separate
  isolated variable with its own paired fixture.
- **A cause that flickers on and off every tick still flips the label.** The
  hysteresis is on arousal, and arousal cannot oscillate on its own — but the
  released/acting test is instantaneous, so a dog crossing an occluder edge on
  alternating ticks would alternate `driven` and `recovering`. No memoryless rule
  can suppress that, and the alternative — a dwell timer — was rejected above.
  The observed adversarial case is the realistic one: a cause tuned so the
  *arousal* crosses the driven entry threshold on 377 of 400 ticks, where the
  rule changed the published label zero times after it settled.
- **A settled sheep is not necessarily at rest.** A cause too weak to reach
  `alert_arousal` still raises arousal above `rest_arousal` and the sheep stays
  `settled`; the stubborn sheep in the paired fixture sits at exactly `0.1875`
  forever. That is the Schmitt design working as intended, and it means
  "`settled`" must be read as "not engaged", not as "arousal is zero".
- **Nothing here is calibrated.** Every rate and threshold is a legibility choice
  made to produce exactly representable arithmetic and a legible sequence. No
  player has seen a sheep change state, and the accepted design's questions about
  whether pressure and release feel intentional remain unanswered.
- **This is invisible.** With no debug arrows or labels, a state change is
  observable only in the state dump. Making it visible is the next roadmap item
  and the first Phase 3 sheep-behavior outcome that will need human visual
  review.

### Alternatives considered and rejected for now

- **An accumulator that charges and discharges at fixed rates** (`arousal +=
  rise * stimulus * dt` while pressed, `-= decay * dt` otherwise). Rejected
  because it has no interior equilibrium: under any constant cause it ramps to
  one bound or the other, so a sheep under moderate pressure ends up fully
  aroused given enough time and the thresholds stop meaning anything.
- **An exponential approach** (`arousal += (stimulus - arousal) * k * dt`).
  Rejected because the time to reach a threshold depends on the distance to it,
  so "how long does release take" has no answer that can be stated, and because
  it never reaches its target exactly, which would force every oracle onto
  tolerances.
- **A dwell timer instead of separate thresholds.** Rejected above: new
  authoritative per-sheep state, new contract field, more stack, and it slows a
  flap rather than preventing one.
- **Letting behavior scale the steering terms** — a `driven` sheep responding
  harder. Rejected for this outcome: it would couple the new variable to eight
  accepted rules at once and invalidate every per-term oracle's isolation. It is
  the obvious next question once the states are visible and reviewed.
- **A second, arousal-specific stimulus radius and falloff.** Rejected because
  two radii can disagree, and nothing has been observed that would justify a
  different shape from the one the dog terms already use.
- **Making `recovering` depend on arousal falling rather than on the cause being
  released.** Rejected because it makes the label a derivative rather than a
  cause: a sheep under steady pressure whose arousal is drifting down by rounding
  would be published as recovering while the dog is still on top of it.
