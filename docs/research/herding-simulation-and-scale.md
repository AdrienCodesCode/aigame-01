# Research: Herding simulation and flock scale

**Status:** Draft research; not implemented

**Produced by:** Codex

**Date:** 2026-08-15

**Project revision:** `main` at `8fe5a95b9d5726cbb35c22395c9422ea69ca4fb5`
with documentation work in progress

**Adversarial review:** Completed during the planning pass on 2026-08-15

## Problem and decision

How can Wide Eye build a behaviorally grounded sheep simulation with a credible
path to large flocks without expanding the five-sheep proof of concept
prematurely?

There is directly relevant research. The strongest current source is a 2024
peer-reviewed study that tracked 14 Merino sheep and a working border collie
during repeated drives, then published the trajectories, analysis, and an
agent-based model. Earlier studies measured dog-induced packing, spontaneous
grazing/packing cycles in groups of 100 sheep, and temporary leadership during
small-flock movement.

This evidence supports a better model than generic boids: sheep should have
local and selective social interactions, distinct grazing/settling and driven
states, a response to dog position and motion, and group-level behavior that can
be inspected through measurable observables. It does not supply a universal
"real sheep" formula. Breeds, individuals, terrain, flock size, context, and dog
behavior all matter.

A fully simulated 1,000-sheep flock is a credible **engineering hypothesis**, not
an observed project capability. Bounded neighbor queries can avoid the quadratic
cost of comparing every sheep with every other sheep, but simulation, terrain,
animation, visibility, rendering, and frame pacing still need to be measured on
the approved low target. The correct sequence remains five sheep for behavior
and fun, then 25, 100, 250, 500, and 1,000 as explicit benchmark tiers.

## Verified project constraints

- The custom C++ engine is the approved primary track, targeting native x86-64
  Linux and Windows with C++23, SDL3, and OpenGL 4.6 Core.
- At research time, the repository had no engine source or build system and the
  Phase 0 toolchain/context smoke was next. Current implementation truth belongs
  in [`ROADMAP.md`](../../ROADMAP.md).
- Five sheep, one dog, one farmer signal, one gate, and pressure/release remain
  the approved correctness and first-fun experiment.
- The 60 Hz fixed simulation, low-target 2 ms p95 five-sheep budget,
  deterministic replay, procedural-first asset policy, and dependency/license
  rules in ADR 0001 remain in force.
- No external sheep model, code, or dataset has been imported.

## Findings

### Confirmed facts

1. **A border-collie drive has been measured at individual-animal resolution.**
   Jadhav et al. tracked one flock of 14 Merino ewes, one trained border collie,
   and the shepherd in an 80 m by 50 m field using Ultra-Wide-Band tags. During
   the drives, the flock was cohesive and aligned; the paper reports group
   cohesion, polarization, elongation, speed, dog position, turning, and
   directional influence. On short timescales, directional information flowed
   from the front of the moving group toward the rear, while the dog also
   adjusted to flock motion. The authors explicitly caution that the study used
   one flock and one dog. Source: [Jadhav et al., Communications Biology 7,
   1543 (2024)](https://doi.org/10.1038/s42003-024-07245-8), version of record
   2024-11-20, accessed 2026-08-15.

2. **That study includes a reproducible research artifact.** The authors
   released raw trajectories, MATLAB analysis, and model code. The immutable
   Zenodo release is `comm-bio-r3`, DOI
   [10.5281/zenodo.13982895](https://doi.org/10.5281/zenodo.13982895), described
   as CC-BY-4.0. The mutable GitHub repository was inspected at HEAD commit
   `2c02a8832880318438f6fe96035895f749dfd7cf` on 2026-08-15 and declares
   GPL-3.0. Source: [authors' research repository](https://github.com/tee-lab/collective-responses-of-flocking-sheep-to-herding-dog).
   Because the archive and live repository present different licensing
   contexts, importing code or data requires a file-level license review. No
   source has been copied into this project.

3. **Simple interactions can reproduce some observed drive signatures.** The
   2024 model combines heading persistence and noise with sheep attraction,
   alignment, close-range repulsion, and repulsion from a nearby dog. It limits
   attraction/alignment to selected neighbors rather than assuming that every
   visible sheep contributes equally. This is evidence for a useful baseline,
   not proof that every term is biologically necessary; the paper notes that
   other empirical work does not always find explicit velocity matching.

4. **Threat can increase attraction toward the flock center.** GPS trajectory
   analysis found that sheep moved strongly toward the flock center as a dog
   approached, consistent with a selfish-herd response. Source: [King et al.,
   Current Biology 22 (2012)](https://doi.org/10.1016/j.cub.2012.05.008),
   accessed through the [UCL open-access record](https://discovery.ucl.ac.uk/id/eprint/1366736/)
   on 2026-08-15.

5. **Undisturbed sheep motion is intermittent, not constant-speed boid motion.**
   Field observations of 100 Merino sheep found slow grazing/spreading phases
   interrupted by rapid packing events, and an individual-based model reproduced
   important features using behavioral state changes and local imitation.
   Source: [Ginelli et al., PNAS 112 (2015)](https://doi.org/10.1073/pnas.1503749112),
   accessed 2026-08-15.

6. **Leadership need not be a permanent character class.** Experiments with
   small sheep groups found collective-motion episodes separated by grazing,
   with a temporary leader during each motion episode and individuals
   alternating leader/follower roles. Source: [Gomez-Nava, Bon, and Peruani,
   Nature Physics 18 (2022)](https://doi.org/10.1038/s41567-022-01769-8),
   accessed 2026-08-15.

7. **Collecting and driving are useful shepherding concepts.** Strömbom et al.
   presented a heuristic that switches between collecting a dispersed group and
   driving an aggregated group, and compared model features with sheep-dog GPS
   data. It models the shepherding agent rather than the player's skill and must
   not be mistaken for a complete sheep-behavior model. Source: [Strömbom et
   al., Journal of the Royal Society Interface 11 (2014)](https://doi.org/10.1098/rsif.2014.0719),
   accessed 2026-08-15.

8. **Local-neighbor acceleration is established simulation practice.** Craig
   Reynolds' original boids work uses local separation, alignment, and cohesion.
   The author's technical note states that a straightforward all-pairs
   implementation is quadratic, while an appropriate spatial structure can make
   neighborhood lookup close to linear for bounded local density. Source:
   [Craig Reynolds, Boids background and update](https://www.red3d.com/cwr/boids/),
   accessed 2026-08-15.

### Qualified findings

- "Flocking" is valid collective-behavior terminology for sheep, while
  "herding" describes the dog/shepherd task. The documents may use both, but
  should not use generic flocking as a synonym for a realistic sheep model.
- Cohesion, polarization, elongation, flock speed, nearest-neighbor spacing,
  split count, response latency, and settle/rejoin time are candidate validation
  observables. Exact acceptable ranges must be calibrated for our scale,
  timestep, terrain, and stylization; copying one paper's numbers would be false
  precision.
- A grazing state is biologically motivated. It is not automatically required
  for the first gate-driving challenge; the initial implementation can include
  `settled`, `alert`, and `driven` transitions and add richer foraging only when
  the scenario uses it.
- Temperament can represent persistent response differences, but `nervous` and
  `stubborn` are game-readable design labels rather than validated biological
  categories.
- A custom engine makes data layout, update scheduling, debug visibility, and
  render batching controllable. It does not make 1,000 agents fast by itself.

### Rejected interpretations

- **"Three boids forces equals realistic sheep."** They are a useful diagnostic
  baseline, not a validated end state.
- **"All sheep continuously average every nearby heading."** The 2024 study
  uses selective influence and also explains why explicit alignment remains an
  open modeling choice.
- **"The dog unilaterally dictates the flock's direction."** Measured drives
  show coupled behavior; the dog responds to the flock, and front-positioned
  sheep can dominate short-timescale directional information.
- **"A fixed leader personality explains motion."** Temporary and
  context-dependent leadership is better supported for the studied conditions.
- **"Published 100-sheep observations prove our 1,000-sheep target."** They do
  not establish our algorithms, terrain costs, rendering costs, or hardware
  budget.
- **"LumenFall's reported one-week build supplies a schedule."** It is an
  unaudited anecdote and does not measure this engine, this simulation, or this
  game's acceptance gates.

## Options and tradeoffs

| Option | Benefit | Cost or failure mode | Disposition |
| --- | --- | --- | --- |
| Generic three-force boids | Fast visual-motion baseline | Constant motion and global-looking averages can contradict measured sheep behavior | Diagnostic only |
| Research-informed CPU agents | Inspectable, deterministic, data-oriented, and compatible with gameplay | Requires calibration and does not guarantee biological fidelity | Recommended |
| Direct port of a published model | Faster reproduction of one paper | Context mismatch and license review; optimizes for a paper rather than play | Reject for engine code |
| GPU-first flock simulation | Potential throughput at very high counts | Harder replay, inspection, game-state access, and cross-platform determinism | Defer until measured |
| Hand-authored flock animation | Strong cinematic control | Cannot support player-caused emergent splits and recovery | Presentation reference only |

## Recommendation

Treat behavioral realism as reproducible group signatures, not as the number of
rules or the use of biological vocabulary.

### Individual state

Keep stable sheep IDs and explicit inspectable state. A first model needs:

- position, heading, speed, and bounded turn/acceleration;
- behavioral state such as `settled`, `alert`, `driven`, and `recovering`;
- a small set of response parameters for persistent individual variation;
- selected social neighbors and the reason each was selected;
- current dog stimulus, obstacle response, and stress/arousal proxy.

The stress value is a gameplay-facing hypothesis, not a direct cortisol model.
Name it `arousal` internally until evidence establishes what it represents.

### Interactions

Start with independently switchable and recordable terms:

1. collision-distance repulsion from sheep;
2. attraction to a limited set of local sheep;
3. optional alignment with a smaller selected subset;
4. dog avoidance based on distance and relative bearing, with approach speed,
   line of sight, terrain, and individual sensitivity introduced separately;
5. bounded obstacle/drop avoidance;
6. state transition, persistence, and recovery terms.

Compute the next state from an immutable snapshot, then commit updates together.
This avoids update-order artifacts that look like leadership. Randomness must be
seeded, recorded, and removable from diagnostic scenarios.

### Group observables

Every named scenario should be able to emit:

- flock centroid and mean radius;
- polarization and elongation relative to travel direction;
- speed distribution and group speed;
- nearest-neighbor distance distribution and chosen-neighbor count;
- number and membership of connected flock components;
- dog bearing/distance to the flock and rear sheep;
- response latency, split/rejoin time, and settle time;
- per-system CPU time, allocation count, and agent count.

These measurements are more useful than accepting a clip because it "looks
alive." Visual judgment still matters, but it becomes traceable to a replay and
state dump.

### Scale strategy

The gameplay proof and the scale proof answer different questions:

| Tier | Purpose | What it may prove |
| --- | --- | --- |
| 5 | First gate challenge | Legibility, control, deterministic correctness |
| 14 | Research-comparison fixture | Whether our observables and drive shape can be compared with the 2024 study |
| 25 | Small-flock stress fixture | Splits, recovery, debug usability |
| 100 | Large-flock behavior fixture | Packing/spreading signatures and bounded-neighbor behavior |
| 250, 500, 1,000 | Performance ladder | Measured CPU/GPU/memory cost and visual readability only |

Use a structure-of-arrays or similarly contiguous layout for hot sheep state, a
uniform spatial grid rebuilt without per-agent heap allocation, stable iteration
order, and bounded neighbor selection. Measure grid build, neighbor search,
behavior, terrain queries, snapshot publication, animation preparation, draw
submission, GPU time, and memory separately.

Do not introduce simulation level of detail, GPU behavior simulation, or
lower-frequency distant updates until the full-rate CPU baseline identifies a
bottleneck. Those techniques can alter propagation, splits, and player feedback;
if adopted, compare their group observables against the baseline using identical
seeds and inputs.

The 1,000 tier becomes a gameplay target only after it:

- passes the approved low-target frame-time and memory budgets;
- preserves behavior within explicitly chosen tolerances;
- remains readable and controllable from a tested camera;
- produces more fun or a distinct experience in playtests than a smaller flock.

Until then, it is a capacity experiment.

## Failure modes and gotchas

- Overfitting one Merino flock and one border collie can produce false precision.
- A force-weight copy can reproduce a chart while feeling unreadable at a 60 Hz
  gameplay tick on uneven terrain.
- In-place updates can manufacture leaders and propagation through array order.
- Unbounded density can turn a nominally local grid into an all-pairs problem.
- Simulation LOD can change split, wave, and response timing even when individual
  trajectories look acceptable.
- A technically rendered 1,000-sheep scene can be unplayable because the camera
  cannot communicate individual or subgroup causes.
- The live research repository and immutable archive expose different license
  labels; reuse needs file-level review.
- "Arousal," "temperament," and other game variables can become misleading if
  documented as direct measures of physiology or personality.

## Evidence and confidence

- **High confidence:** sheep/dog drives can be described with measurable group
  observables; threat-related compression, intermittent grazing/packing, and
  context-dependent leadership are directly observed in the cited studies.
- **Moderate confidence:** selected local interactions plus explicit behavior
  states are a strong game-simulation baseline. Multiple models can reproduce
  similar group signatures, so individual terms remain testable hypotheses.
- **Moderate confidence:** a deterministic uniform-grid CPU implementation can
  preserve a path to much larger counts without harming the five-sheep design.
- **Low confidence until measured:** 1,000 full-rate sheep meet this project's
  low-target budget and produce better play than a smaller flock.

## Planning handoff

The most defensible point of difference is not "a lot of sheep." It is a flock
that behaves consistently enough for players to read its shape, predict a
response, apply pressure, notice propagation, and repair a mistake. Scale can
amplify that fantasy after the causal language works with five animals.

Progression through ducks, geese, goats, cattle, or other working-dog contexts is
promising because it could require players to unlearn assumptions and read a new
kind of group. Each species needs its own research and behavior model; changing
the mesh and a few speed values would undermine the premise. This progression
remains a design hypothesis until the sheep loop passes fresh-player testing.

Likewise, rewards should eventually recognize calm control, route accuracy,
animal welfare, recovery, and judgment rather than speed alone. Exact scoring,
economy, unlocks, and campaign structure should be designed from observed play,
not inferred from the reference-image HUD.

### Reference-material boundary

The transcript in [`ref/gpt-chat.md`](../../ref/gpt-chat.md) is ideation. Its
"indirect-control simulation" framing and narrow game-specific engine are useful
strategic inputs. Its reported one-week and days-to-two-weeks estimates are
unverified claims and are not roadmap commitments.

The two generated images in `ref/` are mood and composition references only.
They suggest a readable voxel/low-poly countryside, strong silhouettes, flock
density, depth, and both overview and closer cameras. Their commands, HUD,
inventory, score, clock, minimap, flock sizes, and depicted interactions are not
game-design evidence.

### Unresolved questions

- Which sheep breed, terrain, and working context should anchor later
  authenticity review?
- Can the published trajectories be used as a calibration fixture under the
  project's dependency and license policy? This is optional and needs explicit
  file-level review.
- Which selected-neighbor rule best balances observed motion, player
  legibility, determinism, and cost?
- Is explicit velocity alignment needed, or do attraction, repulsion, state
  propagation, and dog stimulus reproduce the desired signatures?
- What population remains readable and controllable from the eventual camera?
- Does large-flock play create new decisions, or only spectacle?
- Which animal should follow sheep, if any, and what genuinely different
  herding lesson would it teach?

The bounded implementation sequence is in
[`docs/plans/herding-simulation-and-scale.md`](../plans/herding-simulation-and-scale.md).
The larger game questions belong in
[`docs/game-design/HERDING_GAMEPLAY.md`](../game-design/HERDING_GAMEPLAY.md), not
the first-playable specification.

## References

- Jadhav et al., [Collective responses of flocking sheep to a herding dog
  (2024)](https://doi.org/10.1038/s42003-024-07245-8), with immutable
  [research artifact](https://doi.org/10.5281/zenodo.13982895).
- King et al., [Selfish-herd behaviour of sheep under threat
  (2012)](https://doi.org/10.1016/j.cub.2012.05.008).
- Ginelli et al., [Intermittent collective dynamics emerge from conflicting
  imperatives in sheep herds
  (2015)](https://doi.org/10.1073/pnas.1503749112).
- Gomez-Nava, Bon, and Peruani, [Intermittent collective motion in sheep results
  from alternating the role of leader and follower
  (2022)](https://doi.org/10.1038/s41567-022-01769-8).
- Strombom et al., [Solving the shepherding problem
  (2014)](https://doi.org/10.1098/rsif.2014.0719).
- Reynolds, [Boids background and update](https://www.red3d.com/cwr/boids/).

## Recommended next step

Do not implement this plan yet. Complete the current Phase 0 native toolchain
and OpenGL-context smoke-test items, then follow the first incomplete tracer in
`ROADMAP.md`. When Tracer 2 begins, establish deterministic state/observable
fixtures before tuning sheep motion.
