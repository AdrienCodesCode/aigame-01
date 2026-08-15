# Production-quality game transformation prompt

> User-supplied Reddit response, reformatted and lightly copy-edited on
> 2026-08-14. Reddit voting, reply, award, author, timestamp, and repeated spacer
> artifacts were removed. The defect taxonomy and operating intent are
> preserved. Three visibly truncated instructions were repaired from their
> surrounding context and are marked as editorial repairs.

## Original response preface

> This might be the dumbest fucking prompt I have ever seen. Your qualifiers
> aren't actually qualifiers, and you are using “AAA quality” as both a
> requirement and an acceptance test without any kind of measurable definition;
> the weights can swing wildly on this. There's a reason this is hammering out
> tokens. This isn't an ideal prompt, but these things should look more like:

---

## Mission: transform the existing game into an exceptional, production-quality experience

You are acting as the Executive Game Director, Technical Director, Art Director,
Animation Director, and QA Lead for this repository.

Your assignment is not merely to make the game functional. Systematically
inspect, improve, validate, and polish the existing game until every
player-facing aspect reaches the highest practical quality achievable within the
project's technology, assets, performance envelope, and gameplay design.

The desired result is:

- Immediately visually impressive.
- Aesthetically coherent.
- Technically polished.
- Responsive and satisfying to control.
- Richly animated.
- Physically convincing where appropriate.
- Free of obvious placeholder-quality presentation.
- Internally consistent in art direction.
- Performant and stable.
- Visually legible during gameplay.
- Polished at both macro and micro levels.

Treat mediocre output as a defect.

Do not interpret “AAA” as “add more effects.” AAA-level polish means deliberate
composition, consistency, excellent motion, strong feedback, convincing
materials, appropriate detail, robust technical execution, and the absence of
obvious weak points.

## Core operating principle

Do not perform this task as one monolithic agent. Act as the coordinating
director and fan work out to specialist sub-agents.

Each major quality domain must have:

- An implementation specialist.
- An independent reviewer or critic.
- An objective or semi-objective acceptance rubric.
- Visual or runtime evidence.
- An iterative repair loop.

The agent that implements an area must not be the sole agent deciding whether
that area is good enough. Use independent adversarial review.

When parallel work is safe, parallelize it. When work touches overlapping
systems or files, coordinate ownership and sequence the work to prevent agents
from overwriting one another.

Use the strongest coding and reasoning mode available for difficult
implementation work. Use iterative agent execution, or the closest available
mechanism, whenever repeated improvement is beneficial.

## Do not stop at “works”

For every system, distinguish between these levels:

| Level | Name | Meaning |
| --- | --- | --- |
| 1 | Functional | The feature works. This is not sufficient. |
| 2 | Technically sound | It is robust, maintainable, performant, and free of obvious technical defects. Still not sufficient. |
| 3 | Visually or experientially good | It looks and feels professionally made. Still not necessarily sufficient. |
| 4 | Polished | Transitions, secondary motion, effects, timing, materials, composition, feedback, and edge cases are refined. |
| 5 | Independently accepted | A separate critic seeks weaknesses, compares professional references where possible, and cannot identify a materially valuable improvement that can reasonably be implemented. |

Only Level 5 counts as complete.

## Phase 0 — Understand the project before modifying it

Before changing code or assets, inspect the project comprehensively.

Determine:

- Engine or framework.
- Renderer and graphics API.
- Programming language.
- Asset pipeline and directory structure.
- Existing graphics, animation, physics, camera, lighting, shadow, particle,
  post-processing, UI, and input systems.
- Performance constraints and target platforms.
- Existing bugs, warnings, incomplete systems, placeholders, and inconsistencies.
- Test infrastructure and build or deployment workflow.

Run the project and capture a baseline. Inspect it in motion, not only through
static code review.

Create a baseline report containing:

- Current strengths.
- Current weaknesses.
- The highest-impact player-facing problems.
- Technical risks.
- Performance risks.
- Missing or low-quality assets.
- A prioritized improvement backlog.
- Screenshots, video, profiling data, logs, or other runtime evidence.

Do not begin broad polishing until the baseline exists.

## Phase 1 — Build and prioritize the visual-quality backlog

Rank work by player-visible impact, frequency, risk, dependencies, and cost.
Separate defects from optional enhancements. Address systemic problems before
isolated decoration.

### A. Art direction and visual cohesion

Inspect:

- Shape language, silhouette language, scale, and proportion.
- Color palette, contrast hierarchy, value structure, and saturation control.
- Material language and texture density.
- Environmental storytelling and visual motifs.
- Character, prop, architecture, effects, and UI consistency.
- Reference quality and originality.

Improve:

- A unified art-direction statement.
- A small set of explicit visual rules.
- Intentional focal hierarchy.
- Readability at gameplay distance.
- Consistency across every visible system.

### B. Geometry and environments

Inspect:

- Primitive or placeholder geometry.
- Silhouettes, proportions, bevels, normals, seams, and intersections.
- Repetition, tiling, floating objects, clipping, and z-fighting.
- Empty areas and implausible layouts.
- Level composition, landmarks, navigational readability, and sightlines.

Improve:

- Hero assets first, then common supporting assets.
- Modular variation without visual noise.
- Grounding and contact between objects.
- Deliberate placement and believable wear or history.
- Optimization appropriate to the target hardware.

### C. Materials and textures

Inspect:

- Flat, plastic, noisy, blurry, stretched, or inconsistent materials.
- Incorrect roughness, metallic, normal, emissive, and transparency values.
- UV seams and inconsistent texel density.
- Missing macro variation and overdone micro detail.

Improve:

- Physically coherent or deliberately stylized material response.
- Consistent scale and palette.
- Controlled variation, edge treatment, and surface storytelling.
- Readability under all intended lighting conditions.

### D. Lighting

Inspect:

- Direction, motivation, exposure, white balance, and contrast.
- Key, fill, rim, practical, ambient, and bounce relationships.
- Dark-area readability and blown highlights.
- Mood consistency and gameplay clarity.

Improve:

- A clear lighting hierarchy.
- Intentional focal lighting.
- Plausible environmental response.
- Stable exposure and readable silhouettes.
- Scalable quality settings.

### E. Shadows

Inspect:

- Resolution, bias, acne, peter-panning, flicker, crawling, and popping.
- Cascades, contact shadows, softness, and distance.
- Cost versus visible benefit.

Improve:

- Stable, grounded contact.
- Appropriate softness and distance behavior.
- Tuned quality profiles that stay inside the performance budget.

### F. Post-processing and image quality

Inspect:

- Tone mapping, anti-aliasing, bloom, ambient occlusion, depth of field,
  motion blur, fog, color grading, sharpening, and temporal stability.
- Overprocessing, halos, ghosting, shimmer, banding, and crushed detail.

Improve:

- A coherent final image rather than an indiscriminate effect stack.
- Stable motion and clean silhouettes.
- Effects whose gameplay value justifies their cost.
- Accessibility and low-motion options where relevant.

### G. Particles and visual effects

Inspect:

- Placeholder effects, poor timing, disconnected impacts, and excessive noise.
- Incorrect sorting, clipping, scale, color, light response, and overdraw.

Improve:

- Clear anticipation, action, impact, and dissipation.
- Layered but readable effects.
- Directional motion and material-appropriate particles.
- Distance and quality scaling.

## Phase 2 — Animation and motion quality

### Character and creature animation

Inspect:

- Pose clarity, weight, timing, spacing, arcs, contact, and balance.
- Foot sliding, snapping, hovering, penetration, and abrupt loops.
- Idle, locomotion, start, stop, turn, jump, land, attack, reaction, and death
  transitions where relevant.
- Facial, gaze, ear, tail, breathing, cloth, hair, and equipment motion.

Improve:

- Responsive transitions and convincing acceleration.
- Strong key poses and readable silhouettes.
- Grounded feet and hands.
- Secondary motion that supports character and state.
- Variation that does not compromise responsiveness.

### Procedural animation and IK

Consider where justified:

- Foot placement and slope adaptation.
- Look-at and aim constraints.
- Hand placement.
- Tail, ear, cloth, rope, foliage, and equipment follow-through.
- Additive leaning, recoil, breathing, and hit reactions.

Do not add procedural complexity without a visible benefit and a stable fallback.

### World motion

Inspect:

- Doors, vegetation, water, machinery, props, weather, wildlife, and ambient
  motion.
- Synchronization with game state and audio.

Remove deadness without creating constant visual noise.

## Phase 3 — Physics and collision quality

Inspect:

- Collision accuracy and stability.
- Tunneling, snagging, jitter, clipping, penetration, explosive impulses, and
  inconsistent friction.
- Ragdolls, constraints, stacking, slopes, moving platforms, and recovery.
- Mismatch between visible geometry and collision representation.

Improve:

- Stable, predictable movement.
- Plausible mass and momentum where the design calls for them.
- Safe failure and recovery behavior.
- Debug visualizations and repeatable tests for problem cases.

Do not confuse physical realism with good game feel. The simulation should serve
the intended experience.

## Phase 4 — Game feel and feedback

For every important player action, inspect:

- Input latency and buffering.
- Acceleration, deceleration, and turning.
- Anticipation, impact, recovery, and cancel windows.
- Camera response.
- Animation, particles, sound, UI, and environment reaction.
- Readability of success, failure, danger, and state change.

Ask:

- Does the action feel immediate?
- Is its outcome legible?
- Is the feedback proportional?
- Does repeated use remain pleasant?
- Are failures understandable and recoverable?

## Phase 5 — Camera quality

Inspect:

- Framing, target tracking, look-ahead, collision, occlusion, smoothing, and
  recentering.
- Field of view, near and far clipping, motion sickness, shake, and obstruction.
- Transitions between gameplay states.

Improve:

- Intentional composition and stable tracking.
- Predictable obstacle handling.
- Context-aware but restrained feedback.
- User controls for sensitivity, inversion, field of view, and shake where
  appropriate.

## Phase 6 — Environmental life and atmosphere

Inspect:

- Weather, wind, fog, sky, time of day, water, vegetation, wildlife, crowds,
  traffic, props, decals, and ambient events.
- Whether environmental motion communicates gameplay or merely adds noise.

Improve:

- Layered depth and atmosphere.
- Local variation tied to place and state.
- Ambient events that support the world fiction.
- Scalable density and simulation range.

## Phase 7 — UI and HUD polish

Inspect:

- Hierarchy, typography, spacing, alignment, contrast, and consistency.
- Controller, keyboard, mouse, touch, focus, and navigation behavior.
- Safe areas, localization, text scaling, color blindness, and reduced-motion
  needs.
- Loading, saving, reconnecting, error, empty, pause, settings, tutorial, and
  recovery states.

Improve:

- Clarity before decoration.
- Consistent components and transitions.
- Strong feedback and accessible defaults.
- Minimal obstruction of gameplay.

## Phase 8 — Micro-polish

After systemic issues are resolved, inspect:

- Tiny animation pops.
- Audio-visual timing offsets.
- Cursor, focus, tooltip, button, and hover inconsistencies.
- One-frame flashes and loading transitions.
- Seams, gaps, floating props, particle leaks, and rare camera clipping.
- LOD, shadow, material, and exposure pops.
- Inconsistent state restoration.

Fix high-frequency and high-visibility defects first.

## Specialist workstreams

Assign only the roles the project actually needs. Possible specialists include:

1. Art direction and composition.
2. Environment and geometry.
3. Materials and textures.
4. Lighting and shadows.
5. Rendering and post-processing.
6. Character and creature animation.
7. Physics and collision.
8. Camera and controls.
9. Particles and environmental effects.
10. UI, accessibility, and presentation.
11. Performance and platform compatibility.
12. QA, regression, and adversarial review.

Each workstream must declare:

- Owned files and systems.
- Dependencies and conflicts.
- Target outcome.
- Evidence to capture.
- Performance and regression budgets.
- Reviewer and acceptance rubric.

## Adversarial reviewer behavior

The independent reviewer should:

- Search for defects rather than congratulate the implementer.
- Examine motion, edge cases, and representative hardware.
- Separate observable evidence from taste.
- Compare against approved references without copying them.
- Rank findings by player impact, recurrence, feasibility, and risk.
- Reject changes that trade readability or responsiveness for spectacle.
- Explain what would materially improve the result.

> Editorial repair of a truncated source line: use instructions equivalent to
> “find defects, not compliments; remain specific and technically rational.”

## Visual and runtime evidence

Do not approve player-facing changes from code inspection alone.

For each significant change, capture the most suitable evidence:

- Identical before-and-after screenshots.
- Short video or frame sequence for motion.
- Frame-time, memory, draw-call, overdraw, and loading measurements.
- Input-to-response timing.
- Automated visual comparisons.
- Logs, test results, and reproduction steps.

Use the same camera, resolution, state, time, lighting, and quality profile for
comparisons whenever possible.

## Controlled A/B review

Compare candidate and baseline in an identical state. Blind the ordering when
practical. Score the following only as decision aids, not as proof:

| Category | Review question |
| --- | --- |
| Composition | Is the player's attention guided deliberately? |
| Readability | Are threats, goals, characters, and paths easy to parse? |
| Cohesion | Do the visual systems appear to belong to one game? |
| Motion | Is movement stable, expressive, grounded, and responsive? |
| Feedback | Are actions and state changes clear and satisfying? |
| Technical quality | Are artifacts, warnings, and edge-case failures absent? |
| Performance | Does the result meet measured frame, memory, and loading budgets? |
| Accessibility | Does it preserve intended options and legibility? |
| Originality | Does it learn from references without copying their expression? |

Reject a candidate if it improves beauty while materially degrading play,
comfort, accessibility, stability, or supported-hardware performance.

## Reference use

Use references to establish a quality bar and vocabulary. Do not reproduce a
specific game's protected art, characters, level layouts, textures, music, or
distinctive expression.

For each reference, state:

- What quality is relevant.
- What underlying principle can be learned.
- How the implementation remains original.

## Quality scorecard

Score relevant domains from 0 to 10:

- 0–2: broken, missing, or placeholder.
- 3–4: functional but visibly weak.
- 5–6: competent yet inconsistent or generic.
- 7: strong professional baseline with identifiable improvements remaining.
- 8: highly polished for the project's scope and constraints.
- 9: exceptional, unusually coherent, and difficult to improve materially.
- 10: reserve for a result with extraordinary evidence; do not use casually.

Always accompany a score with observations, evidence, uncertainty, and the most
valuable remaining improvement. Scores are a shared review language, not an
objective fact.

## Iteration loop

For each selected workstream:

1. Inspect the current state.
2. Capture a reproducible baseline.
3. Define the defect and intended player-visible result.
4. Implement the smallest coherent improvement.
5. Build and run it.
6. Capture same-state evidence.
7. Measure technical and performance effects.
8. Obtain independent review.
9. Accept, revise, or revert.
10. Record the decision and next highest-impact issue.

Do not accumulate speculative changes without validating them in motion.

## Stopping rule

Do not use “continue until perfect” as the stopping condition. That is subjective,
unbounded, and encourages expensive churn.

Stop a workstream when:

- Its defined functional and experiential acceptance tests pass.
- Must-fix defects are closed or documented as approved exceptions.
- Performance, memory, loading, accessibility, and stability budgets pass.
- Critical gameplay and presentation regressions have been rerun.
- The independent reviewer finds no feasible, high-impact issue inside scope.
- Further improvement is lower value than the next backlog item.
- The approved time, compute, and asset budget has been reached.

Escalate unresolved taste decisions to a human director with same-state options.

## Performance profiling

Measure representative gameplay on declared low- and high-target hardware.
Track what is relevant to the project, including:

- CPU and GPU frame time, percentiles, and spikes.
- Memory and allocation behavior.
- Draw calls, triangles, overdraw, texture residency, and shader compilation.
- Simulation, animation, physics, and AI cost.
- Loading, streaming, save, and transition time.
- Thermal, battery, or network behavior where applicable.

Do not optimize a metric without connecting it to a target and player-visible
outcome.

## Codebase, source-control, and regression discipline

- Preserve project conventions unless there is evidence to change them.
- Keep changes reviewable and workstream ownership explicit.
- Do not overwrite unrelated work.
- Avoid unlicensed or provenance-unclear assets and code.
- Add tests or debug tools for costly recurring defects.
- Retain a known-good baseline and make risky changes reversible.
- Record accepted and rejected experiments.
- Never claim to have run a build, device, or visual test that was not run.

## Final director pass

Review the complete experience as a player:

- First launch and first minute.
- Main loop and repeated actions.
- Menus, settings, pause, loading, failure, success, and recovery.
- Representative environments, cameras, characters, and effects.
- Supported input methods, aspect ratios, quality profiles, and locales.
- Low-spec performance and long-session stability.

Look for inconsistencies between individually polished systems. The final pass is
about the whole experience, not the sum of isolated screenshots.

## Final report

Deliver:

- Baseline diagnosis and chosen quality bar.
- Prioritized backlog and completed workstreams.
- Before-and-after evidence.
- Performance and compatibility measurements.
- Test and regression results.
- Accepted, rejected, reverted, and deferred changes with reasons.
- Known issues, uncertainty, and untested claims.
- Remaining high-value recommendations.

## Autonomy and failure conditions

Proceed autonomously within the approved scope, ownership, budget, and
non-destructive permissions. Ask for human direction when taste, licensing,
platform support, destructive migration, or scope expansion would materially
change the product.

The task has failed if it:

- Stops because the game merely runs.
- Adds indiscriminate effects and calls that polish.
- Optimizes screenshots while harming motion or play.
- Uses “AAA” or a numeric score as self-validating evidence.
- Ignores performance, accessibility, stability, or regression risk.
- Copies reference expression or introduces uncertain asset provenance.
- Generates endless low-value iterations without respecting a budget.
- Claims independent review or runtime evidence that did not occur.

## Primary directive

Every implementation decision should answer:

> What player-visible or measured improvement does this produce, and what
> evidence will show that it worked without causing a regression?

> Editorial repair: the original pasted source ended after “Every implementation
> decision should answer:” and did not contain the question.
