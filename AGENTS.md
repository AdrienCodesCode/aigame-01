# Agent instructions

## Scope

These instructions apply to the entire repository.

This repository is currently a **playbook**, not a game project. It contains a
workflow in `README.md` and three example images. The detailed prompts, example
game, analytics, hosting, and other tools are external resources. Do not imply
that their source or behavior is present or verified here.

The purpose of this fork is to develop a versioned, engine-agnostic, and
evidence-driven method for using AI during game development. Optimize for the
time it takes to learn whether a game idea works, not for generated code volume,
feature count, claimed hours of content, or visual superlatives.

For the custom C++ voxel-engine track, [`ROADMAP.md`](ROADMAP.md) is the
cross-context continuation source. Read its current checkpoint and work from the
first unblocked unchecked item in the current phase. Update the checkpoint and
evidence when completing roadmap work; never check an item for an unrun result.

For every implementation or diagnostic outcome, follow the accepted
[`DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md). It defines the task
contract, standardized build/observe/review loop, proportional verification
cadence, regression protocol, artifact and golden rules, human visual review,
and context-window handoff. Do not create a competing private workflow in a
prompt, skill, MCP, or script.

The custom C++ engine is the approved primary implementation track. Native
Linux/Windows support, the C++23/SDL3/OpenGL foundation, procedural-first asset
policy, provisional budgets, and dependency/license rules are recorded in
[`docs/decisions/0001-native-foundation.md`](docs/decisions/0001-native-foundation.md).

The transcript and generated images under `ref/` are ideation inputs, not
authoritative specifications. Preserve the images as visual references, but do
not infer commands, controls, HUD, scoring, inventory, minimap, camera, flock
size, or progression from their depicted UI. The authoritative first-playable
design is [`docs/game-design/WIDE_EYE.md`](docs/game-design/WIDE_EYE.md); broader
unapproved directions and explicit unknowns live in
[`docs/game-design/HERDING_GAMEPLAY.md`](docs/game-design/HERDING_GAMEPLAY.md).
Treat the transcript's schedule estimates and large-flock claims as unverified
until this repository measures them.

When the user asks to end, wrap up, checkpoint, or hand off a development
session, read and follow
[`end-engine-session`](.agents/skills/end-engine-session/SKILL.md). It composes
the repository's documentation-sync skill and updates the current checkpoint in
`ROADMAP.md`; do not create a competing diary-style handoff.

## How to interpret the inherited playbook

Adopt these principles:

- Define the player fantasy, core verbs, pressure, twist, and repeatable loop.
- Build the smallest end-to-end slice with meaningful success and failure.
- Inspect the existing project before changing it and preserve sound systems.
- Keep gameplay rules, presentation, input, persistence, networking, and
  telemetry behind clear ownership boundaries.
- Test on the real target devices and inputs, then playtest with people outside
  the development loop.
- Use telemetry and qualitative feedback to choose the next experiment.
- Keep documentation synchronized so later agents do not rediscover decisions.

Do not adopt these ideas uncritically:

- A long checklist is not automatically the right scope for a prototype.
- A prompt is not a specification, implementation, test result, or proof of
  quality.
- Fast generation does not establish fun, originality, maintainability,
  accessibility, security, performance, or commercial readiness.
- “AAA” is not a useful acceptance criterion without an approved comparison
  set, a rubric, same-state captures, measured target hardware, and human review.
- Session duration and funnel completion are signals, not direct measurements of
  enjoyment. Crashes, idle tabs, confusion, acquisition source, and sample bias
  can all distort them.
- Architecture, analytics, localization, automation, backends, multiplayer,
  asset pipelines, and Low/Ultra graphics profiles are conditional investments,
  not mandatory systems for every first prototype.

## Evidence and claim discipline

Clearly label statements as one of:

- **Goal**: what the project intends to achieve.
- **Hypothesis**: what a playtest is intended to learn.
- **Observed result**: what was directly seen or measured, with build, date,
  platform, sample, and method.
- **Inference**: an interpretation of observed results, including uncertainty.
- **Unverified claim**: an external or anecdotal statement not reproduced here.

The inherited “two days,” “15 hours of gameplay,” “full game,” and “AAA” claims
are not reproducible from this repository. The example screenshots demonstrate a
visible presentation change only. They do not prove the claimed development
time, content length, gameplay quality, performance, or production readiness.
Do not repeat those claims as established facts.

The linked Reddit results are also exploratory: the shown funnel starts with 53
players and reaches 3 at its final named step, while most shown sessions are
under five minutes. Treat this as a reason to investigate, not as proof that the
loop succeeds or fails. Record denominators, cohort definitions, acquisition
source, build version, platform, consent behavior, missing events, and confidence
limits before drawing conclusions.

For external evidence, link the original source and record an access date,
version, commit, or immutable capture when possible. External prompt pages are
mutable; a link alone is not a reproducible input.

## Default development loop

Use the following loop for downstream game work and for examples added here:

1. **Frame the experiment**
   - Name the target player and platform.
   - Write the core loop in one sentence.
   - State one primary playtest question.
   - Define success, failure, pivot, and stop criteria before implementation.
   - Create a Must / Should / Could / Won't scope for the iteration.
2. **Inspect and baseline**
   - Read applicable instructions and design documents.
   - Run the existing build and relevant tests.
   - Capture current behavior and performance before changing it.
   - Identify placeholders, unknowns, and risks instead of guessing silently.
3. **Build the thinnest honest vertical slice**
   - Include entry, the core action, feedback, a meaningful choice or pressure,
     and success and failure.
   - Prefer representative quality where presentation affects the question;
     use clearly marked placeholders elsewhere.
   - Avoid systems that do not help answer the current playtest question or
     retire a concrete technical risk.
4. **Verify before recruiting players**
   - Test the critical path, input, save/recovery behavior, and obvious failure
     states.
   - Measure on representative target hardware.
   - Instrument only the events needed to answer the stated question and detect
     severe technical failures.
5. **Playtest and interpret**
   - Observe players without coaching first.
   - Combine behavior, telemetry, interviews, and bug/performance evidence.
   - Separate bugs and discoverability failures from problems with the core
     design.
   - Treat small samples as directional and list plausible alternative causes.
6. **Decide**
   - Keep, change, simplify, pivot, or discard the idea.
   - Change the smallest coherent variable set in the next iteration.
   - Update the source-of-truth documents with the evidence and decision.
7. **Harden only after a positive signal**
   - Expand content and production systems after the loop earns that investment.
   - Revisit security, accessibility, localization, data safety, deployment,
     observability, platform compliance, and performance at the appropriate gate.

Run one major prompt or workstream at a time. Review its assumptions, diff,
tests, and evidence before starting the next. Never let a generated plan silently
become approved scope.

Frame material work as one coherent outcome with a goal, only relevant context,
the invariants that must survive, explicit non-goals, done-when evidence, and
conditions that require the owner. Prescribe detailed steps only when their order
or method is itself a requirement.

At the end of every completed coherent outcome, remind the owner whether to
start a **fresh chat** for the next independent outcome, **continue this chat**
for the same unresolved outcome, or **compact, then continue** when the same
outcome remains active but context is bloated. Do not mechanically ask for a new
chat after every message or discard an unfinished diagnostic trail. A fresh chat
must be pointed to `AGENTS.md`, the current `ROADMAP.md` checkpoint, and the
relevant plan/design files.

## Proportional engineering

- Start with the simplest architecture that preserves the current slice and a
  credible next iteration. Avoid both monoliths and speculative abstraction.
- Add a backend only when trusted shared state, accounts, cloud persistence,
  purchases, competition, or another concrete requirement needs it.
- When a backend exists, keep secrets and authoritative rules off untrusted
  clients; validate identity, authorization, inputs, concurrency, and replay
  behavior at the server boundary.
- Separate visual meshes and animation from authoritative collision and game
  rules. Test frame-rate variation, repeated contacts, tunneling, and interruption
  where those risks apply.
- Build custom remote-automation protocols only when the expected testing value
  justifies their security and maintenance cost. Prefer existing engine, browser,
  accessibility, and test-runner hooks first.
- Design stable player-facing string and content IDs early, but claim support
  only for locales and accessibility modes actually tested.
- Keep analytics provider-neutral behind a failure-safe boundary. Use data
  minimization, consent, retention limits, and age/region-appropriate privacy
  rules. Do not install Google Analytics, Clarity, Glitch Analytics, or any other
  provider merely because a template names it.
- Test mobile work on physical representative devices. Emulation and screenshots
  are useful evidence but not substitutes for touch, lifecycle, thermals, memory,
  network, and frame-pacing tests.
- Add dependencies only for a current requirement. Check maintenance status,
  package size, license, security posture, and whether the platform already
  provides the capability.
- Preserve save compatibility or provide a tested migration and recovery path.

## Player and creator safeguards

- Player-facing output must be understandable without developer context. Never
  expose raw exceptions, secrets, internal IDs, telemetry payloads, or debug UI
  in production player surfaces.
- Treat accessibility as part of interaction design: readable contrast, scalable
  text, remappable or equivalent inputs where appropriate, reduced motion,
  captions, clear focus, and non-color-only meaning.
- Record the provenance, license, model/tool, and allowed uses of every imported
  or generated asset. Do not imitate a living artist or copy a commercial game's
  protected assets to satisfy a style reference.
- Do not manipulate retention through deceptive UI, undisclosed odds, coercive
  timers, or dark patterns. Purchases, ads, loot systems, and play involving
  minors require explicit product and legal review.
- Never deploy, publish, contact playtesters, enable data collection, or mutate an
  external service unless the user explicitly requests that action.

## Repository contribution rules

- Inspect `git status` before editing and preserve unrelated user changes.
- Read `README.md` completely before changing the workflow or its terminology.
- Keep `README.md` as the concise navigation and start-to-finish overview. Put
  detailed, versioned material in clearly named Markdown files rather than making
  the overview endlessly longer.
- Do not silently copy mutable hosted prompts into this fork. Before vendoring or
  adapting third-party text, establish provenance and permission. If prompt text
  becomes local source, prefer `prompts/<stable-slug>.md` and record its purpose,
  prerequisites, placeholders, expected artifacts, verification gates, source,
  and revision date.
- Preserve stable anchors and relative asset paths. Give images descriptive alt
  text and do not use a beauty shot as the sole evidence for a workflow claim.
- Distinguish a prototype, vertical slice, alpha, beta, release candidate, and
  production release. Do not call them interchangeable.
- Prefer precise language over marketing copy. Fix factual and structural errors
  without changing the project's thesis.
- Keep changes small enough to review. Do not combine a methodology rewrite,
  prompt import, asset replacement, and link migration without a stated reason.

## Licensing caveat

There is currently no license file or detected repository license. A public
GitHub repository is not automatically permission to reuse its text and images
outside the rights GitHub needs to host and fork it. Preserve attribution, do not
relicense or redistribute imported material, and ask the owner to add an explicit
license before treating this fork as a reusable open-source prompt library.

## Validation for this repository

The Phase 1 scaffold has a code build plus labeled unit, scenario, and headless
CTest coverage. For documentation changes:

1. Run `git diff --check`.
2. Review the rendered heading hierarchy, lists, tables, and image paths.
3. Check every new or changed link and record when a gated or dynamic target
   cannot be verified.
4. Confirm terminology, step numbering, and relative paths with targeted `rg`
   searches.
5. Inspect changed images at their actual dimensions and at a readable rendered
   size.
6. Review the diff specifically for unsupported quality, schedule, performance,
   player-count, or content-length claims.

If tools for Markdown linting or link checking are later added, use the commands
documented in the repository rather than inventing a competing workflow.

## Definition of done

A change is complete only when:

- Its purpose and affected workflow stage are clear.
- Assumptions, evidence, uncertainty, and intentionally deferred scope are
  explicit.
- New instructions are proportional to the project stage and do not contradict
  another source of truth.
- Links, anchors, images, and Markdown structure were checked.
- Verification actually run is reported; unrun or unavailable checks are named.
- Relevant documentation is updated without claiming unverified support.
- The final handoff explains what changed, what was learned, and the next decision
  that still belongs to a human.
- The final handoff includes the fresh/continue/compact context recommendation
  required by the development workflow.
