# Wide Eye

**Status:** Approved first-playable experiment, not the full product
specification. Broader herding, reward, progression, scale, and animal ideas are
tracked in [`HERDING_GAMEPLAY.md`](HERDING_GAMEPLAY.md).

## Recommendation

Build **Wide Eye** as a small 3D herding simulation-puzzle in which the player is
the border collie, not the farmer. The sheep are never selected or ordered. The
player communicates entirely through position, movement, facing, pressure, and
release while an experienced farmer communicates intent through whistles.

This is more distinctive than a general farming game because the central verb is
reading and shaping a living group rather than collecting and upgrading objects.

## Player fantasy

Be an intensely perceptive working dog: sweep wide across a hillside, feel the
flock turn as you cross its balance point, release pressure before it panics, and
bring every animal calmly through a gate on the farmer's whistle.

The desired feelings are attentiveness, speed, trust, responsibility, and the
quiet satisfaction of coordinated movement.

## Design pillars

1. **Indirect control** — sheep respond to the dog and environment, never to UI
   selection or direct orders.
2. **Read before acting** — flock shape, ears, calls, spacing, facing, and motion
   communicate what is about to happen.
3. **Pressure and release** — skilled play uses restraint. Constant chasing is
   slower and less reliable than choosing the right angle and distance.
4. **Trust, not domination** — calm handling and animal welfare matter more than
   raw completion speed.

## Visual direction

The visual identity does not need to copy FarmRise or any other example linked by
the playbook. Blender is an optional authoring, rigging, and export tool—not an
art style.

Recommended direction: a stylized voxel/low-poly hybrid. Use a voxel hillside,
dry-stone walls, gate, barn, trees, and distant landscape, then use articulated
voxel-built or low-poly animals where extra joints improve communication. A
restricted natural palette, chunky forms, stable sun/shadows, and carefully
staged silhouettes can make it feel cohesive without photorealism.

Readability outranks voxel purity. The dog must clearly communicate facing,
crouch, acceleration, stopping, gaze, ears, and tail. Sheep must communicate
facing, cohesion, hesitation, alarm, and release. If rigid cubes obscure those
signals, use a small rig and curved or beveled pieces.

Pixel-art sprites remain a valid alternative, especially for a fixed or
isometric camera, but freely rotating billboards would require many authored
directions and make spatial pressure harder to read. Full low-poly 3D is the
safest animation pipeline; the hybrid keeps the stronger voxel-world identity.

## Core simulation

The first model combines a few understandable, independently inspectable
influences:

- Stay near flock neighbours without overlapping them.
- Move away from the dog's pressure vector.
- Prefer a visible route and avoid fences, water, cliffs, and blocked gates.
- Respond to a limited, selected set of nearby sheep rather than a global flock
  average.
- Seek an adult when the sheep is a lamb.
- Accumulate stress from excessive speed, proximity, isolation, and sudden
  direction changes; recover when pressure is released.

The dog's effective pressure depends on distance, approach speed, facing, terrain,
and the sheep's temperament. A wide, controlled cast changes the flock's direction.
A fast close approach creates more movement but risks a split or panic.

`Stress` is player-facing shorthand for an arousal/recovery design variable, not
a claimed physiological model. The model should also distinguish settled,
alert, driven, and recovering behavior so sheep do not move like constant-speed
boids. Temporary leadership, selective neighbors, and the group observables used
to judge the result are developed in the
[herding research](../research/herding-simulation-and-scale.md) and
[implementation plan](../plans/herding-simulation-and-scale.md).

## Moment-to-moment loop

1. **Hear** — receive a whistle and see the intended flock and destination.
2. **Read** — inspect flock shape, outliers, temperaments, route, and gates.
3. **Cast** — travel around the flock outside its active flight zone.
4. **Take balance** — cross or hold the balance point opposite the destination.
5. **Pressure** — move inward at a deliberate angle to start motion.
6. **Release** — widen or pause to preserve cohesion and let sheep settle.
7. **Shape** — alternate pressure and release to turn, hold, split, or funnel.
8. **Recover** — collect a stray or repair a bad angle without abandoning the
   flock.
9. **Secure** — move the correct animals through the gate and hold the opening.
10. **Review** — receive simple feedback on calmness, accuracy, and recoveries.

## First playable slice

The first slice should take three to five minutes and contain only:

- One readable hillside paddock.
- One dog, one farmer, five sheep, one open gate, and one destination pen.
- Three sheep behaviors: ordinary, nervous, and stubborn.
- Walk, sprint, facing/eye direction, and a visible pressure-zone debug mode.
- One gather-and-drive whistle.
- Restart, success, and soft coaching after repeated failure.

Do **not** include weather, stamina, progression, money, buildings, breeding,
predators, online accounts, analytics vendors, or multiple farms yet. Those
systems would obscure the first question.

The custom C++ voxel track is the approved primary implementation. A bounded
handcrafted paddock comes before procedural terrain, chunk streaming, level of
detail, or advanced post-effects. The earlier Three.js plan remains a fallback
experiment and reference, not a parallel milestone. See the
[custom engine decision](../VOXEL_ENGINE_OPTION.md).

## Core playtest question

Can a first-time player intentionally steer five mixed-temperament sheep through
one gate using only the dog's movement, facing, pressure, and release?

Success evidence:

- At least four of five fresh players complete the task within ten minutes after
  a short controls-only introduction.
- Players can explain why the flock turned or split in their own words.
- At least three players deliberately release pressure instead of only chasing.
- No player attributes a major flock response to randomness when the simulation
  was behaving as designed.

Failure evidence:

- Players orbit or chase without understanding the balance point.
- Sheep feedback is noticed only after a split has already happened.
- Recovery feels impossible or arbitrary.
- Success occurs accidentally and cannot be repeated.

## Escalation after the loop works

Add one variable at a time:

1. A closed gate that requires temporarily leaving the flock.
2. A lamb whose attachment changes the safest route.
3. A narrow lane that rewards holding a line.
4. Two marked groups that must be split into different pens.
5. Weather that changes scent, visibility, footing, or urgency.
6. A second dog controlled by simple farmer whistles or local cooperation.

Progression should deepen communication and judgment—new whistles, terrain,
temperaments, and multi-dog coordination—not merely increase stats.

Large flocks and progression through different herdable animals are promising
north-star hypotheses, not permission to enlarge this slice. Five sheep remain
the correctness and first-fun gate. See the broader
[herding gameplay direction](HERDING_GAMEPLAY.md).

## Other promising concepts

### The Long Gather

An atmospheric journey bringing a scattered flock down from open mountain before
nightfall. Strong on exploration and weather, but too broad for the first test.

### Two Good Dogs

A cooperative game where two collies must communicate through position and
whistles to manage a large flock. Distinctive, but networking would complicate
early iteration.

### Market Day

Short challenge levels involving sheep, ducks, geese, goats, and cattle, each
responding differently to pressure. Excellent expansion content once sheep alone
feel convincing.

### Lost Lamb

A narrative stealth-herding adventure in which the dog finds and guides animals
through dangerous terrain. Emotionally strong, but it risks turning the flock
system into occasional set dressing.

## Origin note

The live Glitch game-design generator was tested with this concept on 2026-08-14.
It proposed pressure fields, flock cohesion, temperament roles, a balance point,
stamina, whistles, and gates. This document keeps the useful systemic core but
defers stamina and weather so the first experiment measures herding legibility.
