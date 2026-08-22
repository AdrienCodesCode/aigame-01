# Wide Eye herding gameplay direction

**Status:** Directional design and question log; not a complete gameplay,
progression, or economy specification

**Current approved experiment:**
[`WIDE_EYE.md`](WIDE_EYE.md)

**Research basis:**
[`herding-simulation-and-scale.md`](../research/herding-simulation-and-scale.md)

**Reviewed:** 2026-08-16

## Purpose

This is the durable home for the eventual full game: herding behavior, the
repeatable loop, challenge structure, rewards, progression, animal variety, and
the reasons play remains engaging. It intentionally begins as a set of bounded
directions and unresolved questions. The five-sheep slice must teach us how the
core interaction feels before this file becomes a production specification.

The engine and proof of concept still come first. A large content plan would not
retire the current risks: whether the flock is readable, whether pressure and
release are intentional, whether recovery is satisfying, and whether the custom
engine can reproduce and explain the behavior.

## Product thesis

The approved first-playable mode should be about **reading and influencing a
living group** through direct control of the border collie, not issuing unit
commands or chasing waypoints. Position, direction, speed, approach, and release
change local sheep responses; those responses propagate through the flock and
reshape the problem.

The distinctive experience is a causal conversation:

```text
read flock shape -> predict a response -> take position -> apply pressure
        ^                                                  |
        |                                                  v
recover or refine <- observe propagation <- release at the right moment
```

If this works, the player is learning a spatial skill rather than executing a
memorized command sequence. A calm, well-shaped gather should feel better
because the player understands why it worked.

## Current non-negotiable experiment

The approved first playable remains deliberately small:

- one player-controlled border collie;
- one farmer intent signal;
- five mixed-response sheep;
- one bounded paddock, gate, and destination pen;
- movement, facing, pressure, release, recovery, success, failure, and restart;
- deterministic replay and debug evidence;
- no campaign, economy, unlock tree, multiple species, huge flock, procedural
  open world, or inferred reference-image HUD.

Its primary question is unchanged: can a first-time player intentionally steer
five sheep through the gate using the dog's physical relationship to the flock?

## Control-mode hypotheses

Wide Eye may eventually support two substantially different ways to play. They
must remain separate experiments rather than being blended accidentally:

- **Direct dog control — approved current experiment:** a third-person camera
  follows a player-controlled border collie. The player creates flock movement
  through the dog's physical position, direction, speed, pressure, and release.
- **Overhead command control — deferred product hypothesis:** a freely
  controllable bird's-eye camera could let the player select a dog or other
  agent and issue movement or action orders at world locations. This may provide
  better awareness if later evidence supports flocks approaching the population
  scale benchmarks.

The second mode is not approved implementation scope. Selection rules, order
types, camera motion, mouse behavior, keyboard panning, and whether the player
commands one dog or multiple agents are all unresolved. Large-flock capacity
and an overhead view also do not establish that the mode is readable or fun.
For now, controller and camera work should improve only the direct third-person
experiment without closing off a later command-mode input layer.

## North-star hypotheses

These ideas are worth preserving across context windows, but they are not yet
approved features.

### Behaviorally grounded herding

The flock should exhibit recognizable group signatures drawn from sheep
research: selective local influence, compression under threat, propagation of
movement, intermittent settled/driven behavior, temporary or positional
leadership, splits, rejoining, and recovery after pressure. We are not claiming
an exact biological twin. "Realistic" means coherent enough that an informed
observer can identify contradictions and a player can form reliable intuitions.

### Large-flock mastery

Flocks much larger than five could create a rare sense of scale: pressure
travelling through hundreds of animals, long response delays, meaningful edges
and fronts, subgroups, strays, gates that behave like fluid bottlenecks, and
recoveries whose consequences are visible across a hillside.

One thousand fully simulated sheep is currently a capacity hypothesis. It is not
a promised level size or a substitute for the five-sheep fun test. The engine
plan measures 5, 14, 25, 100, 250, 500, and 1,000 separately before game design
depends on any tier.

### Animal curriculum

Progression through different herdable animals could deepen the player's
perception instead of only raising numbers. A later campaign might begin with
sheep and introduce ducks or geese, goats, and cattle only when each produces a
genuinely different spatial lesson.

The intended design standard is not "new model, new speed." Before an animal is
approved, its behavior requires research, an observable contrast with sheep, a
new player judgment, and a reason a border collie/farmer context fits. The exact
species order and unlock structure remain unresolved.

### An engine for indirect-control simulation

The transcript's broader idea—an engine specialized for worlds where the player
shapes conditions and a mass responds—is strategically useful. Sheep are the
first and only approved game system. Water, fire, snow, crowds, roots, insects,
and other simulated masses are possible future uses of the technology, not
Wide Eye scope.

## What may create fun

These are design hypotheses to test, not assertions:

- **Anticipation:** seeing a turn, compression, split, or stray form before it is
  complete.
- **Leverage:** a small change in dog position produces a large but explainable
  group response.
- **Restraint:** backing off at the correct moment is an active skilled choice.
- **Recovery:** a mistake creates a changed but still solvable flock shape.
- **Flow:** casts, turns, stops, and releases form a readable physical rhythm.
- **Responsibility:** calm animal handling matters, without turning welfare into
  a punitive meter disconnected from visible behavior.
- **Mastery transfer:** a learned principle works in a new terrain or flock, but
  not as a rote solution.
- **Scale:** later, many local responses become a legible landscape-scale event.

The first playtest should reveal whether anticipation, leverage, restraint, and
recovery already exist with five sheep. Scale cannot rescue them if they do not.

## Reward direction—not a scoring system yet

The player should eventually be rewarded for quality of handling, not raw speed
alone. Candidate dimensions are:

- completing the correct gather or sort;
- keeping the flock cohesive when cohesion is desirable;
- using proportionate pressure and allowing recovery;
- choosing a safe, efficient route;
- recovering strays or splits deliberately;
- responding correctly to the farmer's intent;
- protecting animal welfare and avoiding dangerous terrain.

These should primarily be visible in animal motion, farmer response, and the
state of the job. Abstract grades or scores may summarize performance after a
task, but no formula is approved. We need to learn whether numeric optimization
improves mastery or makes players game the meter instead of reading the flock.

## Progression questions

Do not answer these through implementation until the first-player gate has
evidence:

1. Is the main progression player knowledge, access to new jobs, dog abilities,
   farmer communication, terrain complexity, flock composition, or some mix?
2. Should mistakes cost only time and handling quality, or can animals/jobs have
   persistent consequences?
3. Does the player grow one dog/farm relationship, travel between farmers, or
   play discrete challenge trials?
4. What is a satisfying short session, and what creates a longer arc?
5. Which variables deepen the same skill, and which merely add management noise?
6. When does a larger flock add decisions rather than visual spectacle?
7. What does the next animal teach that sheep cannot?

## Camera and visual reference boundary

On 2026-08-22 the owner selected the two locally generated images as the primary
target for how the sheep game should ideally look:

- [overhead countryside reference](<../../ref/ChatGPT Image Aug 15, 2026, 03_22_55 PM.png>)
- [closer hillside reference](<../../ref/ChatGPT Image Aug 15, 2026, 03_23_03 PM.png>)

Their approved target cues are voxel-informed stylization, layered landscape
scale, dense vegetation and ground detail, readable dog/sheep silhouettes, warm
directional light, long shadows, misty atmospheric depth, flock density, and the
contrast between an overview camera and a closer embodied view. Geometry and
surface detail must be sufficient to avoid a conspicuously chunky or
aggressively low-poly result; the exact balance remains an owner-reviewed
iteration variable. This is a **Goal**, not an observed engine result: every cue
still needs same-state in-engine motion, readability, and performance evidence.

The files [visual-ref-1](../../ref/visual-ref-1.jpg),
[visual-ref-2](../../ref/visual-ref-2.png),
[visual-ref-3](../../ref/visual-ref-3.png),
[visual-ref-4](../../ref/visual-ref-4.jpg), and
[visual-ref-6](../../ref/visual-ref-6.webp) are secondary graphics-quality
references only. They may support judgments about landscape scale, atmosphere,
shadowing, surface richness, and environmental density where they agree with
the two primary images; they do not define the literal Wide Eye environment.
No `visual-ref-5` file was present when this boundary was recorded.

The readable `visual-ref-2.png` and `visual-ref-3.png` specifically add a target
for controlled softness, layered depth of field, dense fine vegetation, and
finer voxel-scale or geometry resolution than a coarse block aesthetic. The
softness is not permission for blanket gameplay blur: the dog, relevant sheep,
route, gates, and terrain edges must stay legible in the active focal region,
while stronger defocus can support close or presentation views. This balance is
an owner-reviewed visual variable, not an accepted post-processing technique.

[`real-photo-sheep3.jpg`](../../ref/real-photo-sheep3.jpg) is the owner's
real-world reference **only for sheep spatial distribution**: local spacing,
clusters, density variation, gaps, and outliers across a hillside flock. It does
not define sheep anatomy, proportions, surface treatment, or individual animal
appearance. It is visibly watermarked third-party Bigstock material, so it is
reference evidence only and cannot be treated as a redistributable or shippable
asset without documented rights.

Do not infer commands, controls, score, clock, day counter, inventory, minimap,
flock size, progression, or task structure from either image. The depicted UI
was not generated from an approved game design. The current experiment has
selected the third-person camera family, but its movement relationship, orbit,
alignment, and tuning are not final. The deferred overhead command mode would
require its own camera and control experiment.

## Evidence gates for expanding this document

### After deterministic simulation

Record which group responses are stable, understandable, biologically plausible,
and debuggable. Revise the vocabulary before writing content around a behavior
that does not survive tests.

### After the fresh-player test

Record what players noticed, predicted, misunderstood, enjoyed, repeated, and
avoided. Decide whether the core is keep, change, simplify, pivot, or discard.
Only a positive signal earns a detailed challenge and onboarding structure.

### After the population ladder

Record measured capacity and camera readability separately. Decide which flock
sizes create new play. Do not design levels around the largest number that can
technically render.

### Before each new animal

Write a small research/design note that identifies real group behavior, dog
interaction, the new player lesson, ethical framing, technical delta, and a
single-species proof scenario.

### Before campaign/reward implementation

Promote the chosen session loop, feedback, scoring, progression, and content
scope into an approved specification with explicit non-goals and playtest gates.
