# Ultra prompt: run an evidence-led production-quality game pass

This working prompt folds the useful production taxonomy from the formatted
[`game-prompt.md`](../game-prompt.md) reference into the repository's experiment,
evidence, ownership, privacy, and stopping rules. “AAA” appears in the filename
only because it maps to the inherited workflow topic. It is not an acceptance
test.

Append [`_shared-guardrails.md`](_shared-guardrails.md) and obey the repository's
`AGENTS.md` before running this prompt.

## Preconditions

Do not run this pass merely because the game launches. Confirm all are true or
return a readiness report instead of editing:

- The approved core loop is playable end to end, including meaningful success,
  failure, restart, and recovery.
- The current build or commit is named and recoverable.
- Critical correctness, save, input, and gameplay tests pass or their failures
  are explicitly accepted.
- The target platforms and representative low/high hardware are named.
- The art direction and comparison references are approved.
- The run has explicit time, token, compute, asset, and human-review budgets.
- A human owner is available for irreversible taste or scope decisions.

This is a late-stage quality pass. If the core loop has not earned continued
investment through playtesting, stop and recommend the smallest earlier
experiment.

## Inputs

- Release-candidate build or commit: [BUILD]
- Representative levels, routes, modes, cameras, and player states: [STATES]
- Known issues and approved exceptions: [ISSUES]
- Art-direction statement, visual rules, and comparison rubric: [RUBRIC]
- Low/high target hardware, operating systems, displays, and input devices:
  [TARGETS]
- CPU/GPU frame-time percentiles, memory, startup, load, network, and package
  budgets: [TECHNICAL BUDGETS]
- Supported locales and accessibility modes: [ACCESSIBILITY]
- Approved asset/code sources and license policy: [PROVENANCE]
- Maximum workstreams, iterations per workstream, wall time, AI tokens/compute,
  and human-review checkpoints: [RUN BUDGET]
- Areas forbidden from modification: [DO NOT TOUCH]

If material inputs are unknown, inspect and report the uncertainty. Do not invent
a target, quality bar, license, device result, or product decision.

## Role and objective

Act as the coordinating game director for a bounded final pass. Improve the
complete player experience to the strongest coherent professional standard that
fits this game's design, technology, content, audience, and measured hardware
envelope.

The objective is not to maximize effects, polygons, systems, agents, or output.
It is to remove the most important observable weak points while preserving:

- Gameplay readability and intent.
- Input response and game feel.
- A coherent original art direction.
- Stable motion and image quality.
- Accessibility and supported locales.
- Performance, memory, loading, network, and package budgets.
- Correctness, save compatibility, and recovery behavior.

Every accepted change must answer:

> What player-visible or measured improvement does this produce, what evidence
> demonstrates it, and what regressions could it introduce?

## Operating rules

1. Inspect and run before changing anything.
2. Preserve a reproducible baseline and rollback point.
3. Rank by player impact, recurrence, reach, feasibility, risk, and cost.
4. Assign explicit file/system ownership before parallel work.
5. Use specialists only where scopes are independent and the runtime supports
   their integration. Do not invent unavailable agent or loop capabilities.
6. The implementer may not be the only reviewer of a material workstream.
7. Validate player-facing work in motion, not from source alone.
8. Change one coherent variable set per comparison.
9. Retain rejected experiments and their reasons in the work log, not necessarily
   in production code.
10. Stop at the approved budget even if optional polish remains.

## Quality maturity model

Use these levels to describe evidence, not to generate work indefinitely:

| Level | Meaning | Required evidence |
| --- | --- | --- |
| 1 — Functional | The path can be completed. | Reproduction or automated path |
| 2 — Sound | It is stable, maintainable, and inside basic budgets. | Tests, diagnostics, measurements |
| 3 — Effective | Players can perceive and use it as intended. | Runtime review and/or playtest evidence |
| 4 — Polished | Motion, transitions, feedback, composition, and edge cases are coherent. | Same-state captures and regression results |
| 5 — Accepted | A separate critic finds no feasible must-fix issue inside this pass. | Signed review with remaining uncertainty |

Not every low-risk component needs Level 5 review. Declare the required level by
player impact and failure risk before work begins.

## Phase 0 — Audit and immutable baseline

Inspect the repository and running build. Record:

- Engine/framework, renderer, platform layer, languages, and build pipeline.
- Scene/entity structure and ownership boundaries.
- Asset sources, licenses, import settings, compression, and variants.
- Character, animation, camera, input, physics, collision, AI, UI, audio,
  lighting, shadows, particles, post-processing, save, networking, analytics, and
  localization systems.
- Tests, automation hooks, debug modes, profilers, crash/error reporting, and
  current warnings.
- Target profiles and whether the current build meets their budgets.

Capture a baseline matrix across representative states:

- First launch, loading, main menu, settings, onboarding, and first minute.
- Core action at calm, typical, busy, success, failure, and recovery moments.
- Hero characters, common characters, environments, UI, particles, and camera
  motion.
- Each supported input family, aspect ratio, graphics profile, accessibility
  mode, and representative locale.
- Long-session or transition-heavy behavior where it is a known risk.

For each capture, record build, platform, resolution, settings, camera/state,
seed, input/replay, and measurement method. Never call a reconstructed state an
identical comparison.

Deliver a ranked backlog with:

- Observable defect.
- Player consequence.
- Evidence and frequency.
- Suspected system owner.
- Dependencies and risk.
- Proposed acceptance test.
- Estimated cost and budget fit.
- Must / Should / Could / Won't classification for this run.

Obtain human approval of Must work and the maximum number of Should items before
implementation.

## Phase 1 — Full-frame art direction and composition

Evaluate the whole frame before polishing isolated assets:

- Focal hierarchy, silhouette, values, palette, contrast, and saturation.
- Camera framing, scale, landmarks, path/goal clarity, and visual density.
- Consistency among characters, environments, props, effects, lighting, and UI.
- Shape language, material language, texture density, and environmental story.
- Readability at real gameplay distance and in motion.

Create or refine a short visual ruleset. Reference other works for principles such
as hierarchy, pacing, palette, or material response, never for copying protected
expression. Record what is learned and how the result remains original.

## Phase 2 — Geometry, environments, materials, and assets

Inspect:

- Placeholder or primitive-looking hero content.
- Silhouettes, proportions, bevels, normals, UVs, texel density, seams,
  intersections, floating objects, clipping, z-fighting, tiling, and repetition.
- Material roughness, metallic, normal, emissive, transparency, scale, and
  response under intended lighting.
- Layout plausibility, grounding, sightlines, collision mismatch, LOD transitions,
  and empty or noisy spaces.

Prioritize hero and high-frequency assets. Improve reusable systemic causes
before hand-fixing many instances. Keep detailed visual geometry separate from
simple authoritative collision. Record source, license, generator/model where
required, edits, and export settings for every new asset.

## Phase 3 — Lighting, shadows, and final image

Inspect:

- Lighting motivation, key/fill/rim balance, exposure, white balance, contrast,
  bounce, practicals, and dark-area readability.
- Shadow acne, peter-panning, bias, cascade transitions, shimmer, softness,
  contact, distance, and cost.
- Tone mapping, anti-aliasing, bloom, ambient occlusion, fog, color grade, depth
  of field, motion blur, sharpening, and temporal stability.
- Halos, ghosting, crawling edges, banding, crushed values, blown highlights,
  transparency artifacts, and overprocessing.

Add an effect only when it supports composition, depth, state, or action. Compare
against the effect disabled. Create Low/High profiles only for declared target
hardware, with documented visual intent and per-setting cost.

## Phase 4 — Character, creature, and world motion

Inspect in motion and slow capture where useful:

- Pose and silhouette clarity, weight, timing, spacing, arcs, balance, contacts,
  and follow-through.
- Starts, stops, turns, loops, interruptions, transitions, foot sliding, snapping,
  hovering, penetration, and locomotion-speed mismatch.
- Gaze, face, breathing, ears, tails, cloth, hair, equipment, vegetation, doors,
  machinery, water, weather, wildlife, and ambient motion.
- Procedural layers such as foot placement, slope adaptation, look-at, aim,
  leaning, recoil, and secondary motion.

Procedural motion needs a visible benefit, bounded behavior, and stable fallback.
Remove synchronized or constant ambient motion that creates noise or artificiality.

## Phase 5 — Physics, collision, and simulation presentation

Inspect:

- Tunneling, snagging, jitter, clipping, penetration, unstable constraints,
  implausible impulses, friction inconsistency, stacking, slopes, moving objects,
  ragdolls, and recovery.
- Mismatch between render and gameplay shapes.
- Frame-rate dependence and replay nondeterminism where relevant.

Use debug visualizations and repeatable edge cases. Optimize for predictable game
behavior, not realism for its own sake. Reject visual smoothing that hides an
authoritative simulation failure.

## Phase 6 — Game feel, controls, camera, and feedback

For every frequent or consequential player action, review:

- Input latency, buffering, remapping, sensitivity, and device parity.
- Acceleration, braking, turning, anticipation, impact, recovery, and cancels.
- Animation, sound, particle, UI, camera, and environment response.
- Legibility of success, failure, danger, target, resource, and state change.
- Repetition fatigue and excessive shake, flash, noise, or interruption.

For the camera, review framing, look-ahead, recentering, target tracking,
occlusion, collision, obstruction, smoothing, field of view, clipping, shake,
state transitions, and motion comfort. Preserve user controls for sensitivity,
inversion, field of view, shake, and reduced motion when appropriate.

Measure input-to-visible-response where latency is a material risk.

## Phase 7 — Particles, atmosphere, audio, and environmental life

Review anticipation, action, impact, sustain, and dissipation. Check scale,
direction, color, light response, sorting, clipping, overdraw, repetition, and
distance behavior.

Evaluate sky, fog, weather, wind, water, foliage, wildlife, crowds, props,
decals, ambient events, music, ambience, UI sounds, and action feedback as one
attention budget. Layering should create depth and information, not continual
spectacle. Provide scalable density and comfort controls where needed.

## Phase 8 — Game-native UI, accessibility, and localization

Inspect hierarchy, typography, spacing, alignment, contrast, animation, icons,
input glyphs, focus, navigation, safe areas, and obstruction of play.

Exercise:

- Keyboard, mouse, controller, touch, and remapping as supported.
- Hover, pressed, selected, disabled, loading, empty, error, retry, offline,
  reconnect, save conflict, pause, settings, tutorial, success, failure, and
  recovery states.
- Text scaling, color distinction, reduced motion, caption/subtitle settings,
  readable focus, screen-reader/accessibility-tree behavior where applicable.
- Pseudolocalization, expansion, fonts, plural/date/number handling, and RTL for
  every claimed locale capability.

Clarity precedes decoration. The UI should belong to the game's visual language
without imitating a website or sacrificing native accessibility.

## Phase 9 — Performance, loading, memory, and stability

Profile representative gameplay on named low and high targets. Capture, when
applicable:

- CPU and GPU frame-time distributions and spikes, not only average FPS.
- Memory, allocations, residency, leaks, and long-session growth.
- Draw calls, triangles, batches, overdraw, shader compilation, and uploads.
- Animation, physics, AI, simulation, UI, audio, streaming, and network cost.
- Startup, scene/level transitions, save/load, reconnect, and package size.
- Mobile thermals, battery, lifecycle, and real network conditions.

Tie every optimization to an observed bottleneck and recheck image quality,
responsiveness, and correctness. Do not build speculative scalability systems.

## Phase 10 — Micro-polish and complete-experience pass

Only after systemic Must work passes, inspect one-frame flashes, focus and cursor
inconsistency, tiny animation pops, audio/visual offsets, particle leaks, camera
clipping, seams, floating props, material/exposure/LOD/shadow pops, state reset,
and rare transition faults.

Then play the whole supported path as a player. Look specifically for seams
between individually polished systems. A collection of strong screenshots is not
a coherent experience.

## Workstream contract

Before starting a workstream, write:

- Owner and independent reviewer.
- Files and systems owned; forbidden overlaps.
- Baseline build/state and rollback point.
- One observable hypothesis.
- Acceptance and rejection criteria.
- Required captures, tests, devices, locales, and profiles.
- Maximum iterations and time/token/compute budget.

Suggested workstreams are art direction/composition, environments/assets,
materials, lighting/shadows, rendering/image, animation, physics/simulation,
camera/controls, effects/audio, UI/accessibility/localization, performance, and
QA. Instantiate only the workstreams the approved backlog needs.

When two workstreams overlap, sequence them or define an integration owner. Never
allow agents to edit the same files concurrently without coordination.

## Controlled implementation and A/B loop

For each workstream:

1. Reproduce and capture the baseline defect.
2. Implement the smallest coherent candidate.
3. Build and exercise the real runtime.
4. Recreate the same camera, state, seed, settings, and hardware when possible.
5. Capture stills, motion, metrics, logs, and test results appropriate to the
   hypothesis.
6. Blind candidate order for subjective comparisons when practical.
7. Ask the independent reviewer to find defects and tradeoffs, not to praise.
8. Accept, revise, or revert.
9. Rerun affected gameplay, accessibility, locale, save/recovery, visual,
   performance, and platform regressions.
10. Record the decision and remaining uncertainty.

Review categories may include composition, readability, cohesion, originality,
motion, feedback, image stability, technical correctness, accessibility, and
performance. Numeric scores are discussion aids. Never average them into a fake
proof of quality or require an arbitrary universal 8/10.

Reject a prettier candidate when it materially harms play, clarity, response,
comfort, accessibility, originality, stability, maintainability, or a supported
hardware budget.

## Independent critic brief

Give the critic the baseline, candidate, rubric, budgets, known issues, and
declared scope—but not the implementer's preferred conclusion. Require:

- Must-fix defects with reproduction and evidence.
- High-value improvements ranked by impact and feasibility.
- Regressions and hidden costs.
- Unsupported or untested claims.
- Accessibility, localization, platform, license, and originality concerns.
- A verdict: accept, revise, revert, or request human taste decision.

The critic must distinguish observed fact, inference, and taste. A critic who
only returns encouragement has not completed the review.

## Finite stopping rule

Never use “until perfect,” “until AAA,” “until nothing can be improved,” or an
unbounded loop.

Stop a workstream when the first applicable condition occurs:

- Its acceptance tests pass, Must findings are resolved, regressions pass, and
  the critic finds no feasible high-impact issue inside scope.
- The declared maximum iteration count is reached.
- Its time, token, compute, asset, or human-review budget is reached.
- The next improvement has lower expected player value than the next approved
  backlog item.
- Further action requires a new license, destructive change, target expansion,
  product decision, or taste choice that only the human owner can authorize.

At the run boundary, do not silently expand scope. Deliver the strongest verified
state, preserve the last known-good build, and list remaining opportunities.

## Release gate

The pass is accepted only when:

- Approved Must work meets its declared maturity level.
- Critical-path gameplay and recovery tests pass.
- Same-state evidence supports the accepted player-facing changes.
- Performance, memory, loading, package, and stability budgets pass on named
  target hardware—or failures are explicit release blockers.
- Claimed inputs, locales, accessibility modes, and profiles were actually
  exercised.
- New code and assets have acceptable provenance and licenses.
- Independent review is recorded and all high-severity findings are resolved,
  reverted, deferred by an authorized owner, or declared blockers.
- A human director reviews material taste changes and the complete experience.

## Final deliverables

- Baseline diagnosis and capture matrix.
- Approved scoped backlog and workstream contracts.
- Accepted, revised, reverted, rejected, and deferred changes with reasons.
- Identical-state before/after evidence where reproducible.
- Test, visual-regression, locale, accessibility, gameplay, save/recovery, and
  platform results.
- Low/high profile specifications and measured frame, memory, startup, load, and
  package outcomes.
- Asset/code provenance and license additions.
- Independent critic findings and human decisions.
- Known issues, blockers, uncertainty, and every untested claim.
- The next three highest-value improvements, without implementing them.

Finish with a concise release recommendation: **ready**, **ready with explicit
exceptions**, or **not ready**, supported by evidence rather than a superlative.
