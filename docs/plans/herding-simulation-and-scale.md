# Plan: Herding simulation and scale

**Status:** Draft plan; not implemented

**Date:** 2026-08-15

**Source research:**
[`herding-simulation-and-scale.md`](../research/herding-simulation-and-scale.md)

**Architecture readiness:** Localized prerequisite approved

## Objective and success criteria

Preserve the existing tracer order. Build the compiler/rendering foundation and
bounded paddock first, then implement a deterministic five-sheep simulation that
is data-oriented and measurable from its first commit. Do not make a 1,000-sheep
scenario, campaign progression, or multiple species part of the first-playable
gate.

This plan changes the quality of Tracer 2, not its product scope. "Realistic"
means that named replays exhibit explainable, research-informed group signatures;
it does not mean copying a paper's model wholesale or building an exhaustive
livestock simulator.

Success means the engine can prove five-sheep correctness and causal legibility,
then run a reproducible population ladder that labels 1,000 sheep pass, fail, or
deferred without silently weakening behavior.

## Scope and non-goals

In scope are the headless simulation contract, research-informed local behavior,
group observables, debug/state evidence, the five-sheep gameplay integration,
and population benchmarks. Out of scope are a campaign, reward economy,
additional animals, a published 1,000-sheep promise, GPU-first simulation,
generic AI architecture, and direct reuse of external MATLAB code.

## Verified current state

### Confirmed

- The C++ engine and Linux/Windows target are approved in ADR 0001.
- The current repository has no engine source or build system, so this work
  cannot start before Tracer 0 and the paddock prerequisites.
- Sheep/dog research provides candidate interaction terms and group observables.
- A uniform spatial grid is already in the roadmap and provides a credible
  bounded-neighbor path.

## Decisions and assumptions

### Qualified

- Attraction, repulsion, selected-neighbor alignment, behavioral state, and dog
  avoidance are supported as modeling ingredients, not universal biological
  laws.
- A 1,000-agent full-rate simulation may fit the provisional budgets, but only a
  benchmark on the named low target can establish that.
- Published trajectories may help calibration, but importing them is optional
  and requires license/provenance review.

### Rejected

- Do not paste or translate the external MATLAB implementation into the engine.
- Do not use an all-pairs sheep loop as a temporary implementation; it teaches
  the wrong cost model and can become accidental architecture.
- Do not add GPU simulation, multithreading, simulation LOD, a generic ECS, or a
  generic behavior tree before a profile identifies the need.
- Do not tune by hiding instability under unrecorded randomness.
- Do not use the generated reference HUD as a requirement.

### Material decisions intentionally deferred

- Whether 1,000 active sheep becomes a product requirement.
- Whether to import an external research dataset as a test fixture.
- Exact reward, progression, campaign, camera, and other-animal design.
- Whether later scale needs jobs, GPU compute, or reduced update frequency.

None blocks Tracer 0, Tracer 1, or the five-sheep correctness model.

## Prerequisites

- Complete the Phase 0 toolchain/context smoke tests.
- Complete Tracer 0's build, fixed-tick, logging, test, capture, and sanitizer
  foundation.
- Complete Tracer 1's bounded paddock, deterministic scenario selection,
  kinematic dog, analytic collision, and debug camera.
- Preserve ADR 0001's Linux/Windows, dependency, procedural-first, and budget
  constraints.

## Implementation phases

### 1. Establish the headless contract

**Outcome and ownership:** a fixed-tick sheep-simulation component owned by
`game`; it depends on core math/time but not OpenGL or render-frame timing.

- Add the fixed-tick component without committing to an exact class API before
  the source tree exists.
- Define stable sheep IDs, seeded scenario inputs, immutable previous/current
  state buffers, and a versioned state dump.
- Store hot kinematic and behavioral fields contiguously; avoid per-tick heap
  allocation and pointer-rich per-sheep object graphs.
- Add deterministic named scenarios before tuning: stationary group, sheep-only
  separation, dog behind, dog on either flank, pressure release, split, rejoin,
  obstacle, and gate.
- Record per-stage timing and allocation counts even when the flock has only five
  members.

**Evidence:** headless tests, repeated replay hashes or documented deterministic
state comparisons, state dumps, and timing output.

**Stop:** do not tune social behavior until repeated base scenarios publish the
same result and state ownership is inspectable.

### 2. Build the observable baseline

**Outcome and ownership:** pure metric calculations consume published simulation
state and expose read-only results to tests, dumps, and debug presentation.

- Compute centroid, mean radius, polarization, elongation, group speed,
  nearest-neighbor spacing, connected components, and chosen-neighbor counts.
- Add dog bearing/distance, response latency, split/rejoin time, and settle time.
- Expose each influence, selected neighbor, behavioral state, and arousal value
  to both state dumps and debug rendering.
- Unit-test metric definitions on hand-authored point/velocity arrangements.

**Evidence:** metric fixtures whose expected values can be calculated by hand.

**Stop:** do not use a metric as an acceptance criterion until its hand-authored
fixture passes.

### 3. Add local social response

**Outcome and ownership:** the `game` simulation owns bounded social queries and
forces; rendering only visualizes their published results.

- Build and test a deterministic uniform spatial grid.
- Implement collision-distance repulsion first.
- Add attraction to a bounded selected-neighbor set.
- Add alignment as an independently switchable term; compare scenarios with it
  enabled and disabled instead of assuming it is necessary.
- Apply bounded acceleration, speed, and turn rate after combining inspectable
  influences.
- Compute all next states from the same prior snapshot, then publish together.

**Evidence:** neighbor-bound tests, no-allocation steady-state check, influence
breakdowns, and stable repeated scenarios.

**Stop:** do not add dog pressure while sheep-only overlap, density, or
update-order results remain unexplained.

### 4. Add dog stimulus and behavior state

**Outcome and ownership:** player-controlled dog state enters the sheep
simulation as an explicit stimulus; behavior state remains authoritative in
`game` and presentation reads it.

- Start with dog distance and relative bearing.
- Introduce approach velocity, facing, line of sight, terrain, and individual
  sensitivity one variable at a time, with paired replays.
- Implement explicit `settled`, `alert`, `driven`, and `recovering` transitions.
- Treat `arousal` as a designed internal proxy; do not claim physiological
  stress validity.
- Verify that lateral dog movement turns the group away, excessive pressure can
  reduce cohesion or split it, and release permits a reproducible recovery.

**Evidence:** identical-input comparison captures, state traces, response
latencies, and explainable split/rejoin fixtures.

**Stop:** do not accept a new pressure variable unless a paired replay shows the
intended difference without destabilizing existing fixtures.

### 5. Calibrate for legibility before fidelity claims

**Outcome and ownership:** versioned scenario parameters and comparison reports
remain test data; external research artifacts, if approved later, remain
separate from player runtime media.

- Use the research observables to identify qualitative contradictions: constant
  milling, implausible density, permanent fixed leaders, instantaneous group
  turns, excessive overlap, unexplained oscillation, or motion without stimulus.
- Compare 5-, 14-, and 100-sheep fixtures without claiming numerical equivalence
  to a different breed/context.
- If importing the 2024 trajectories would materially improve calibration,
  perform a separate license/provenance decision and keep third-party data out of
  the default player package.
- Retain parameter sets, seeds, and before/after metrics with every accepted
  tuning change.

**Evidence:** versioned scenario configurations, group-observable reports, and
same-camera replay captures.

**Stop:** do not call the model behaviorally grounded while a listed qualitative
contradiction remains or a result depends on unexplained randomness.

### 6. Prove the five-sheep gameplay question

**Outcome and ownership:** objectives consume authoritative simulation state;
presentation and UI communicate but do not decide gate success or sheep state.

- Integrate the model with the bounded paddock, dog controller, one gate, one
  farmer signal, restart, success, and recoverable failure.
- Keep the player-facing UI minimal and derive it from playtest needs, not the
  generated images.
- Run the existing fresh-player gate before designing the campaign economy or
  implementing more animals.

**Evidence:** deterministic successful replay, headless coverage, performance
budget, and the fresh-player notes required by `ROADMAP.md`.

**Stop:** do not progress to campaign or animal expansion if the fresh-player
gate yields change, simplify, pivot, or discard.

### 7. Run the population ladder

**Outcome and ownership:** a reproducible benchmark scenario exercises the same
authoritative rules while timing simulation and presentation stages separately.

- Benchmark identical rules at 5, 14, 25, 100, 250, 500, and 1,000 agents.
- Separate spatial-grid build, neighbor selection, behavior, terrain query,
  snapshot, animation preparation, render submission, GPU, and memory costs.
- Record release preset, hardware, OS, driver, seed, duration, camera, visible
  count, and p50/p95/p99 values.
- At 100 agents, inspect whether group observables remain stable enough for the
  intended behavior. At 250–1,000, treat the result as capacity evidence only.
- Optimize the measured limiting stage. If reduced update frequency or another
  simulation LOD is proposed, compare it against full-rate behavior with the
  same seed and inputs.

**Evidence:** a checked-in benchmark report and reproducible scenario command.

**Stop:** do not optimize or promote a population tier without a named failing
budget or a behavior/readability finding.

### 8. Earn the larger game design

**Outcome and ownership:** approved product decisions move into the dedicated
game-design document and milestone-level roadmap; benchmark capacity alone does
not change scope.

- Decide whether scale adds control decisions, recoveries, spectacle, or only
  cost.
- Develop the dedicated reward/progression/session design from playtest results.
- Research each candidate animal before adding it; define the different lesson
  and group response it contributes.
- Promote only approved work into a roadmap milestone.

**Evidence:** a design decision linked to player observations and measured
technical capacity.

**Stop:** do not implement an additional animal or progression system until its
research, distinct player lesson, and proof scenario are approved.

## Verification matrix

| Risk | Fixture | Required evidence |
| --- | --- | --- |
| Update-order artifacts | Same scenario with stable/reversed storage preparation | Equivalent published result or explained difference |
| Hidden quadratic cost | Population ladder | Per-stage timings and neighbor counts |
| Unreadable causes | Dog flank, split, release | Influence overlay plus state trace |
| Unstable tuning | Repeat fixed seed and input | Same result and bounded metrics |
| Fake scale through visual-only sheep | Headless and rendered counts | Same authoritative agent count |
| Simulation LOD changes behavior | Full-rate versus candidate LOD | Observable deltas within approved tolerances |
| Reference-image scope leak | UI/design review | Every requirement linked to an approved design/playtest source |

## Performance and platform matrix

| Evidence tier | Linux development host | Native Linux target | Native Windows target |
| --- | --- | --- | --- |
| Five-sheep correctness | Required during Tracer 2 | Required before its release gate | Required before its release gate |
| 14/25/100 behavior fixtures | Required before calibration claims | Recheck deterministic limits | Recheck deterministic limits |
| 250/500/1,000 capacity | Measure on the named low/high hardware when available | Required before a Linux capacity claim | Required before a Windows capacity claim |

Record p50/p95/p99 simulation stages, GPU time, frame time, allocations, and
memory. The approved five-sheep simulation budget remains 2 ms p95 on the Low
profile; no higher-count budget is accepted until a baseline exists.

## Risks, rollback, and deferred work

- Preserve a simple selected-neighbor baseline and independently switchable
  influences so a failed term can be removed without rewriting state ownership.
- Keep full-rate CPU behavior as the comparison point before jobs, GPU compute,
  or simulation LOD.
- Revert tuning through versioned parameter/seed fixtures, not by deleting
  failed evidence.
- Defer research-data import, 1,000-sheep product scope, campaign/reward design,
  and other animals to their explicit decisions.

## Definition of done

This plan is complete only when:

- five sheep pass deterministic correctness and the first-playable gate;
- surprising flock responses can be traced to named inputs and state;
- the population ladder has measured results on named hardware;
- 1,000 sheep is labeled pass, fail, or deferred against explicit budgets;
- progression and additional animals remain deferred unless separately approved.

## Recommended first step

Continue Phase 0 rather than starting sheep code: install or provide the native
toolchain and SDL/OpenGL development inputs, record exact versions, and prove the
compiler/context smoke test. When Tracer 2 is reached, implement the headless
state and metric fixtures before tuning motion.
