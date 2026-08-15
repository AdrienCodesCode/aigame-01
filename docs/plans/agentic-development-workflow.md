# Plan: Agentic development workflow and verification harness

**Status:** Accepted foundation; scaffold command/test loop underway, runtime
capture harness pending
**Date:** 2026-08-15
**Source research:**
[`../research/agentic-development-workflow.md`](../research/agentic-development-workflow.md)
**Architecture readiness:** Ready for Tracer 0 — Phase 0 verified the native
Clang/CMake/SDL3/OpenGL toolchain; runtime capture and CI evidence remain to be
implemented.

## Objective and success criteria

Establish one repository-native feedback loop that helps humans and agents frame,
implement, observe, diagnose, verify, and preserve each coherent engine/game
outcome without relying on conversation memory or unverifiable visual claims.

Success means:

- tasks use the accepted goal/context/invariants/evidence contract;
- focused checks run with each affected change and broader checks at gates;
- named scenarios retain reproducible failure evidence;
- visual changes produce comparable owner-review packets;
- accepted goldens cannot be changed without owner approval;
- each completed outcome ends with a fresh/continue/compact recommendation;
- formatting, static analysis, test labels, artifacts, and CI have one canonical
  repository path; and
- the workflow remains fast enough that agents do not bypass it.

## Scope and non-goals

In scope:

- durable workflow rules and navigation;
- C++ formatting and bounded static-analysis configuration;
- CMake/CTest integration requirements;
- named failure/review artifacts and their metadata;
- human visual acceptance;
- Linux-first CI followed by native Windows validation; and
- context/session hygiene.

Out of scope:

- implementing the engine before Phase 0 exits;
- installing more MCP servers;
- finalizing a JSON schema before a real executable emits evidence;
- treating cross-GPU pixel identity as a universal oracle;
- building a remote automation service; and
- creating engine-specific skills before the workflow succeeds twice.

## Verified current state

- [x] The owner approved the standardized feedback-loop direction.
- [x] [`../DEVELOPMENT_WORKFLOW.md`](../DEVELOPMENT_WORKFLOW.md) defines the
  task contract, cadence, bug protocol, artifacts, review, and context lifecycle.
- [x] [`../review/HUMAN_VISUAL_REVIEW.md`](../review/HUMAN_VISUAL_REVIEW.md)
  defines the owner review packet and accepted-baseline rule.
- [x] `.clang-format` and `.clang-tidy` are checked in.
- [x] Ubuntu LLVM 18.1.3 `clang-format` and `clang-tidy` are available through
  the ignored local fallback, and both configurations passed against the real
  scaffold source on WSL on 2026-08-15.
- [ ] Engine source, CMake presets, CTest labels, executable scenarios, artifact
  manifests, captures, and CI exist.

## Decisions and assumptions

- `AGENTS.md` remains concise and links to the detailed workflow.
- `ROADMAP.md` remains the cross-context state and order of work.
- CMake presets and CTest are the canonical build/test interface.
- Generated evidence stays ignored under `artifacts/` until explicitly promoted.
- An owner `accept` verdict is required to promote a visual golden.
- Exact comparisons are reserved for genuinely deterministic products;
  rendered comparisons use named-machine tolerances plus semantic and human
  review.
- Two failed materially different fixes trigger a root-cause reset. This is a
  provisional stop-loss to review after real defect data exists.
- A completed independent outcome normally starts a fresh chat; an unresolved
  outcome continues, compacting first only when context is bloated.

## Prerequisites

- Completed: Phase 0 toolchain and OpenGL context smoke tasks.
- Completed: actual Clang/clang-format/clang-tidy versions recorded before
  configuration validation.
- Keep the first capture/manifest design narrow enough for Tracer 0.
- Preserve current Linux/Windows and dependency/license decisions.

## Implementation phases

### Phase A — Durable workflow foundation

**Outcome:** Every future context can discover the same task, verification,
visual-review, and chat-boundary rules.

- [x] Write the authoritative development workflow.
- [x] Add the human visual-review packet.
- [x] Add checked-in formatting and bounded static-analysis configuration.
- [x] Link the workflow from agent, roadmap, harness, session, and README paths.
- [x] Record engine-runtime tasks in `ROADMAP.md` without claiming they ran.

**Validation:** Markdown links resolve, configuration files parse when their
tools become available, `git diff --check` passes, and no engine checkbox is
checked without runtime evidence.

**Stopping condition:** Finish documentation/configuration only; do not bypass
the Phase 0 toolchain gate.

### Phase B — Tracer 0 command and test loop

**Outcome:** One canonical command path builds, tests, runs, captures, and
explains the cube smoke tracer.

- [x] Add CMake configure/build/test or workflow presets.
- [x] Integrate format checking and the approved narrow clang-tidy set for
  project code only.
- [x] Label automated tests `unit`, `scenario`, `headless`, `sanitizer`, or
  `performance`; record `manual` as a separate evidence category.
- [x] Make pass-marker tests reject project failure stages and ASan, LSan, or
  UBSan diagnostics; retain a nested regression for the failure regex itself.
- [x] Emit a versioned Tracer 0 artifact manifest and retain failure evidence.
  Observed result: the native Windows runner's schema-version 1 pass and
  controlled repeat-mismatch packets passed independent required-field,
  retained-file, and SHA-256 validation on 2026-08-15.
- [ ] Create a candidate cube review packet using the accepted template.
- [ ] Obtain the owner's explicit verdict before promoting a first golden.

**Likely components:** root CMake/presets, tests, smoke executable CLI, ignored
artifact writer, and a small checked-in accepted-evidence location chosen during
implementation.

**Validation:** clean development and sanitizer builds, focused CTest labels,
headless context/capture smoke, manifest inspection, and owner review.

**Stopping condition:** do not add voxel-world or gameplay architecture merely
to enrich the smoke tracer.

### Phase C — Presubmit automation

**Outcome:** A small automated gate reproduces the already-working local loop.

- [ ] Add Linux CI using the exact checked-in configure/build/test/headless
  commands.
- [ ] Retain failure logs/manifests/captures as CI artifacts without secrets.
- [ ] Keep the default job fast and deterministic; quarantine or redesign flaky
  checks rather than normalizing retries.
- [ ] Add native Windows build/context/smoke validation when the target path
  exists; WSL does not count as native release evidence.

**Validation:** a known-good revision passes, an intentional fixture failure
fails with useful artifacts, and local/CI commands do not diverge.

**Stopping condition:** do not add a hosted GPU/Windows promise before a real
runner proves the required context behavior.

### Phase D — Extend the loop by tracer

**Outcome:** Each later tracer adds only the observation/oracle surfaces required
by its new risk.

- [ ] Tracer 1 adds voxel/mesh/collision debug frames and relevant invariants.
- [ ] Tracer 2 adds versioned gameplay replay/state/metrics and motion review.
- [ ] Tracer 3 creates accepted procedural-art and animal-motion baselines.
- [ ] Tracer 4 samples fixed world seeds with validity plus visual evidence.
- [ ] Tracer 5 adds measured scale and same-state renderer comparisons.
- [ ] Tracer 6 records comparable native Linux/Windows startup/render smoke.
- [ ] After two successful review cycles, evaluate focused tracer/visual skills.

**Validation:** use each tracer's existing roadmap exit gate plus the common
workflow and review packet.

**Stopping condition:** reject harness features that do not close a demonstrated
feedback gap.

## Verification matrix

| Change | Focused check | Gate evidence | Human decision |
| --- | --- | --- | --- |
| Build/platform | Configure/build/context smoke | Clean and sanitizer presets | Only for platform blockers |
| Math/data | Unit and invariant tests | Full relevant unit suite | No |
| Gameplay behavior | Named scenario/replay/state | Headless scenario suite and metrics | At behavior tracer or ambiguous intent |
| Rendering/visual | GL diagnostics and capture | Same-state normal/debug/motion packet | Yes for accepted player-facing baseline |
| Performance | Named benchmark and budget | Low/high target measurements | Yes when tradeoff changes product quality |
| Bug fix | Exact reproduction and regression | Neighboring/broader suite by risk | Only when visible intent changes |
| Docs/config | Link/config/diff checks | Proportional audit | Owner approves policy decisions |

## Performance and platform matrix

- Focused development checks must remain short enough to run after affected
  changes; record duration once commands exist.
- Sanitizer and performance jobs may be separate named presets/jobs.
- Do not compare performance from sanitized and release builds as equivalent.
- Record hardware/driver metadata with rendered or performance artifacts.
- Linux development evidence does not establish native Windows support.
- Cross-GPU images use layered comparison rather than assumed exact equality.

## Risks, rollback, and deferred work

- **Workflow bloat:** keep detail here and in the authoritative workflow, not
  duplicated throughout `AGENTS.md` and the roadmap.
- **Noisy clang-tidy:** remove or gate checks that produce unactionable noise;
  do not ignore warnings wholesale.
- **Flaky visual tests:** compare deterministic state first and use named-machine
  image tolerances; preserve human review.
- **Golden normalization:** candidate updates cannot replace accepted artifacts
  without owner approval.
- **Slow CI:** keep a fast default and move long scale/platform work to explicit
  gates.
- **Chat churn:** do not start fresh while the same unresolved reproduction
  benefits from continuity; use `/compact` when appropriate.
- **Rollback:** workflow/config changes are plain versioned files. Revert only
  the affected accepted change; preserve evidence explaining why the policy was
  changed.

Deferred until demonstrated:

- a custom engine MCP;
- automatic perceptual thresholds across every supported GPU;
- scheduled background workflow maintenance; and
- engine-specific implementation/diagnostic skills.

## Definition of done

This plan is complete when:

- Phase 0 toolchain evidence exists;
- the Tracer 0 command loop, labels, failure artifacts, and manifest are real;
- Linux CI reproduces the local fast gate;
- the first visual packet receives an explicit owner verdict;
- accepted baseline protection is tested;
- later roadmap tracers reference the common workflow rather than inventing
  competing loops; and
- context recommendations occur at completed coherent-outcome boundaries.

## Recommended first step

Complete the first unchecked Phase 0 toolchain item in `ROADMAP.md`. Once Clang
is available, validate `.clang-format` and `.clang-tidy` before treating either
configuration as an enforced gate.
