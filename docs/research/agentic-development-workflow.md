# Research: Agentic development workflow and verification harness

- **Status:** Owner-approved direction; promoted into the project workflow and
  roadmap on 2026-08-15
- **Produced by:** Codex
- **Date:** 2026-08-15
- **Project revision:** `main` at
  `8fe5a95b9d5726cbb35c22395c9422ea69ca4fb5`, with an existing dirty
  documentation worktree
- **Adversarial review:** Completed 2026-08-15; record below

## Problem and decision

The project needs a development loop that lets an AI agent implement a custom
C++/OpenGL voxel engine without gradually changing the game, hiding uncertainty,
or entering repeated guess-and-patch cycles. The owner also needs useful moments
to judge the visible result without becoming a full-time test runner, and Codex
context/token use should remain proportional to the task.

The decision is not whether to test or whether to plan. It is how to divide the
work among:

- durable project intent and invariants;
- a bounded assignment and its acceptance contract;
- fast automated checks during implementation;
- deterministic scenario, replay, capture, and measurement interfaces;
- milestone-level human judgment; and
- session-end documentation and maintenance.

**Recommendation:** use **goal + relevant context + invariants + acceptance
evidence** as the default task contract. Give prescribed implementation steps
only when the order or method is itself a requirement. Build the harness as a
closed observation-and-verification loop, not as a collection of MCP servers.
Run the cheapest affected checks after each coherent change, a broader evidence
gate at each tracer, and documentation/diff hygiene at session end.

## Verified project constraints

### Confirmed in the repository

- The primary track is a clean-room C++23/SDL3/OpenGL voxel engine for native
  Linux and Windows.
- The current deliverable is still five sheep, one border collie, one farmer
  signal, one gate, and explicit success/failure/restart—not a thousand-sheep
  open world.
- `ROADMAP.md` is the continuation source and forbids advancing while the
  current exit gate fails.
- `AGENTS.md` already requires an inspect/baseline/build/verify/playtest/decide
  loop, small workstreams, evidence labels, proportional engineering, and
  truthful reporting of unrun checks.
- `docs/AGENT_HARNESS_AND_TOOLS.md` already requires CMake presets, fixed ticks,
  named deterministic scenarios, replay, headless execution, PNG capture,
  structured state/metrics, debug views, tests, sanitizers, budgets, and a
  last-known-good capture/replay set.
- `end-engine-session` already provides a proportional verification and durable
  checkpoint workflow. It correctly does not postpone all validation until the
  session ends.
- No engine source, build graph, executable, automated tests, capture format, or
  CI workflow exists yet. Those claims remain future work.

### External guidance confirmed on 2026-08-15

- OpenAI's current Codex guidance recommends prompts containing goal, context,
  constraints, and a definition of done. It recommends planning before complex
  or ambiguous work, testing and reviewing changes, keeping `AGENTS.md`
  practical, using MCP for external/live context, and turning a demonstrated
  repeated workflow into a focused skill. It also warns against one ever-growing
  chat for an entire project. See [Codex best
  practices](https://learn.chatgpt.com/guides/best-practices), [prompting
  guidance](https://learn.chatgpt.com/docs/prompting), and the [`AGENTS.md`
  guide](https://learn.chatgpt.com/docs/agent-configuration/agents-md).
- CMake has native configure, build, test, and workflow preset concepts suitable
  for checked-in repeatable commands. See [CMake
  Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html).
- Clang's AddressSanitizer detects memory errors including out-of-bounds access,
  use-after-free, and invalid frees; UndefinedBehaviorSanitizer detects classes
  such as invalid bounds, alignment, null use, and signed overflow. They are
  diagnostic test configurations, not production-runtime proof. See the Clang
  [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) and
  [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
  documentation.
- Google's published testing guidance emphasizes small tests, fast and reliable
  presubmit feedback, and hermeticity/determinism while acknowledging the
  tradeoff with production fidelity. See [Testing Overview](https://abseil.io/resources/swe-book/html/ch11.html),
  [Larger Testing](https://abseil.io/resources/swe-book/html/ch14.html), and
  [Continuous Integration](https://abseil.io/resources/swe-book/html/ch23.html).

## Findings

The findings below describe the pre-integration audit. The later adversarial
review and integration record distinguish which gaps are now policy/configuration
and which still require a real engine, toolchain, or runtime result.

### 1. The present roadmap already has visual verification

The answer to “does each step let the owner verify work visually?” is **not every
individual checkbox, but every material visual tracer already has a gate**. That
is the right general shape: asking a human to inspect compiler flags, coordinate
round trips, or neighbor-grid bounds adds little value.

| Phase | Existing visual or human evidence | Audit result |
| --- | --- | --- |
| Phase 0 | Compiler and actual OpenGL context smoke | Correctly technical; no art review needed |
| Phase 1 / Tracer 0 | Triangle, voxel cube, deterministic PNG, headless capture, clean GL diagnostics | Good first visible proof |
| Phase 2 / Tracer 1 | Same-camera normal/debug frames, readable paddock, chunk/mesh explanations | Strong static visual gate |
| Phase 3 / Tracer 2 | Sheep influence overlays, deterministic replay, state/metric dumps, deliberate gate completion | Strong diagnostic gate, but motion review format is unspecified |
| Phase 4 / Tracer 3 | Approved art bible, same-camera references, animal cues, accessibility, five fresh players | Strong human/product gate |
| Phase 5 / Tracer 4 | Ten fixed valid seeds and readable flock routes | Needs an explicit fixed-camera visual sample packet |
| Phase 6 / Tracer 5 | Low/high baselines, same-state effect comparisons, stable-motion captures, optional RenderDoc | Strong performance/rendering gate, but “capture” needs standard metadata |
| Phase 7 / Tracer 6 | Linux/Windows hardware and driver verification | Needs a small cross-platform visual/startup smoke artifact |

**Qualified finding:** the roadmap has enough visual checkpoints; it lacks one
standard review contract. Currently, different agents could provide an open
window, a PNG, a cherry-picked beauty shot, or a debug capture and all call that
“visual verification.”

### 2. The missing workflow is a review packet, not constant supervision

For any tracer or change whose acceptance depends on appearance or motion, the
agent should produce one **human visual review packet**:

1. A short summary of the exact question being judged.
2. A normal frame and relevant debug frame from the same scenario, seed, tick,
   camera, viewport, and graphics profile.
3. A short clip or contact sheet for motion, frame pacing, animation, flicker,
   flock response, or temporal effects. A still image cannot establish these.
4. Before/after frames only when there is a true accepted baseline.
5. A machine-readable manifest containing build/commit, platform, GPU/driver,
   preset, scenario/replay version, seed, tick/frame, camera, viewport, active
   flags, timings, and artifact hashes.
6. The automated checks and invariant results relevant to the image.
7. Known deviations and one explicit owner verdict: `accept`, `revise`, or
   `reject`, with the observation that drove it.

The owner should review these packets at Tracer 0, Tracer 1, behavior Tracer 2,
art Tracer 3, a bounded selection of procedural-world seeds, and each accepted
renderer-depth change. The owner should not need to visually approve a header
rename, memory-layout refactor, or focused regression fix that provably preserves
the accepted replay/capture outputs.

**Recommendation:** an agent may generate candidate goldens, but it must never
silently accept or overwrite them. Human acceptance changes a candidate into a
baseline.

### 3. Exact image equality is not the primary oracle

**Confirmed constraint:** OpenGL output can vary across GPU vendors, drivers,
floating-point implementations, anti-aliasing, and shader compilers.

Therefore:

- use exact equality for serialized inputs, integer coordinates, IDs, counts,
  hashes of genuinely deterministic CPU products, objective outcomes, and
  state invariants where the implementation supports it;
- use numeric tolerances or structural comparisons for floating-point state;
- use exact or tightly tolerant image comparison only on a named reference
  machine and graphics profile;
- use masks, thresholded/perceptual differences, and semantic assertions on
  other supported machines; and
- retain human review for readability, style, motion, and whether the result
  communicates the intended game behavior.

This avoids two bad outcomes: a test suite that fails on every driver, and a
very permissive screenshot threshold that misses a semantically broken scene.
State or geometry tests should usually explain *why* an image changed before a
human decides whether the visible change is acceptable.

### 4. Goal and invariants beat either vagueness or micromanagement

OpenAI's current prompting guidance says to start with the result and describe a
process only when the process itself matters. For this project, the recommended
task contract is:

```markdown
Goal:
  One observable outcome.

Context:
  Only the relevant source, design, roadmap, replay, capture, or error paths.

Invariants:
  Architecture, behavior, ownership, determinism, performance, safety, and
  accepted visual rules that must remain true.

Non-goals:
  Tempting adjacent work that is intentionally excluded.

Done when:
  Exact build/test/scenario/capture evidence and the required human decision.

Stop and ask if:
  A discovered fact would change approved scope, public behavior, a dependency,
  a baseline, data compatibility, or a destructive/external action.
```

Detailed steps should be required when:

- destructive or irreversible work must occur in a safe order;
- a schema/build/dependency migration order is part of correctness;
- an experiment or bug reproduction must remain controlled;
- the chosen architecture or ownership boundary is itself an accepted decision;
- the user wants a specific algorithm for learning/comparison; or
- a compliance or release procedure must be followed exactly.

Otherwise, detailed steps often encode assumptions before the agent has
inspected the code. A goal with no constraints is too loose; a long, speculative
implementation recipe is too rigid. Goal + invariants + evidence gives the agent
room to diagnose while protecting the product.

### 5. “Harness” means a closed feedback system

MCP servers, skills, a compiler, and a test framework are components. The
harness is the complete control loop:

```text
task contract
    -> bounded code/data change
    -> configure/build/test/run
    -> logs + state + replay + capture + measurements
    -> automated oracles + agent diff review + human product judgment
    -> accepted baseline, regression fixture, and roadmap checkpoint
    -> next bounded task
```

The project needs seven complementary planes:

| Plane | Repository implementation |
| --- | --- |
| Intent | `AGENTS.md`, accepted design, ADRs, `ROADMAP.md`, task contract |
| Action | One documented configure/build/test/run interface |
| Observation | Logs, GL debug callback, JSON state, metrics, overlays, captures, profiles |
| Oracle | Unit, invariant, property, scenario, replay, performance, and visual checks |
| Memory | Versioned seeds/replays, accepted goldens, ADRs, roadmap checkpoint |
| Safety | Sandbox, dependency/license gates, sanitizers, bounded file/process access |
| Human | Review packets, playtests, and explicit keep/change/pivot decisions |

The harness is central because an agent can only correct what it can observe and
compare. More MCP tools do not repair a missing replay, a non-deterministic test,
or an unverifiable “looks wrong” report. MCP is valuable when the necessary
live context or action sits outside the repository. Skills become valuable after
the same successful workflow recurs. Neither should create a second private
build/test path.

### 6. Bug fixing should be hypothesis-driven, not patch-driven

The project cannot promise zero iterations. It can make failures fast,
deterministic, narrow, and informative.

For a nontrivial defect:

1. Record the observed symptom without explaining it away.
2. Reduce it to the smallest named scenario, seed/replay, machine/configuration,
   and tick/frame that still fails.
3. State the violated invariant or missing expected observation.
4. Localize the likely ownership boundary from logs/state/debug evidence.
5. Form one hypothesis and state what evidence would support or falsify it.
6. Add instrumentation or a failing regression first when feasible.
7. Make the smallest coherent patch.
8. Re-run the exact reproduction, then the smallest neighboring suite, then any
   broader gate justified by the risk.

For trivial compiler/formatting mistakes, this ceremony is unnecessary.

**Proposed stop-loss:** after two materially different fixes fail the same
reproduction, stop patching. Revert only the agent's unaccepted candidate changes
if it can do so safely, then minimize the fixture, inspect actual state, bisect
when history permits, challenge the assumed invariant, and report the blocker or
new hypothesis. Do not introduce arbitrary constants, delays, extra retries, or
special cases merely to make the observed symptom disappear.

Useful engine invariants include:

- values consumed by simulation and rendering are finite;
- world/chunk/local coordinate round trips hold, including negative boundaries;
- mesh vertex/index counts stay in bounds and visible-face expectations hold;
- fixed simulation outcomes do not change with render frame cadence;
- state transitions have named reasons and legal predecessors;
- snapshot reads are immutable within a tick;
- no steady-state simulation allocation occurs where explicitly prohibited;
- objective counts cannot double-count an animal crossing a gate;
- high-severity OpenGL debug output fails the affected smoke scenario; and
- accepted low-target budgets remain visible whenever a change affects them.

When a direct expected result is difficult to hand-author, use comparisons that
make mistakes disagree: naive versus optimized meshing, single-threaded versus
jobbed generation, full-rate versus simulation-LOD behavior, render-rate
variation against one fixed tick stream, coordinate-translated fixtures, and
Linux versus Windows outcome summaries. These checks do not prove correctness
alone; they cheaply expose divergence.

### 7. Verification needs explicit cadence

“Run everything after every prompt” wastes time and teaches people to ignore
slow or flaky gates. “Test at the end of the session” allows defects to compound.
Use the following tiers.

| Cadence | Required work |
| --- | --- |
| After each coherent code change | Build the touched target; run focused unit/invariant tests; run the affected named scenario/reproduction; inspect the diff; add/update a regression when behavior or a defect changes |
| After a visual/behavioral change | Also generate the relevant deterministic capture/state/debug evidence; agent inspects it locally; request human review only if it changes an accepted player-facing result or reaches a tracer gate |
| At a tracer/milestone gate | Clean configure/build; full relevant unit/integration/scenario suite; sanitizer preset; headless smoke/replay suite; budgets; candidate visual packet; explicit owner verdict where required |
| At session end | Inspect status and the accumulated diff; run `git diff --check`; run proportional broader checks if changes accumulated; audit artifacts/secrets; sync only documentation whose truth changed; update the roadmap checkpoint and concise handoff |
| Periodic/release-oriented | Native Windows/Linux matrix, long scale sweeps, dependency/license/security review, packaging, and deeper static/GPU analysis at their roadmap gates—not after ordinary prompts |

Do **not** defer compilation, the focused affected tests, exact bug reproduction,
or preservation of a new regression fixture until session end. Documentation
rewrites, full suites, broad performance sweeps, and release audits are the work
that can often be batched.

### 8. Additional foundations worth adding after toolchain smoke

These are the main gaps in the current roadmap/harness:

1. **Formatting and bounded static checks.** Check in one `.clang-format` early.
   Add a small, explicit `clang-tidy` profile after `compile_commands.json`
   exists; do not enable a noisy universe of checks that trains agents to ignore
   output.
2. **A capture manifest and golden-approval rule.** Define the packet metadata,
   artifact layout, comparison policy, and the fact that an agent cannot
   auto-approve changed goldens.
3. **A visual-review template.** Give the owner the same three verdicts and the
   same questions at every material gate.
4. **CI once Tracer 0 works locally.** Start with a small Linux configure/build/
   test/headless smoke job using the same presets. Add sanitizer coverage where
   runner support is reliable. Add native Windows validation when the Windows
   build and context path exists; do not claim a hosted/WSL run replaces native
   target evidence.
5. **A test label/runtime policy.** For example: `unit`, `scenario`, `headless`,
   `sanitizer`, `performance`, `manual`. A fast default must stay trustworthy;
   slower gates need named commands and owners.
6. **Failure artifact preservation.** On a scenario failure, retain the seed,
   replay, state dump, relevant logs, capture, and manifest instead of only a
   terminal message.
7. **A narrow review checklist.** At a tracer gate, review ownership/lifetime,
   integer/float boundaries, determinism, error paths, performance budgets,
   test quality, and accidental scope. Codex's `/review` can assist, but human
   acceptance still protects product intent.

The root `AGENTS.md` is already 254 lines and 13,604 bytes. OpenAI's guidance
specifically recommends keeping it short and practical and linking detailed
task-specific guidance when it grows. Therefore the details above should live
in a dedicated workflow document and future focused skills, with only a compact
link and the few highest-value invariants in `AGENTS.md`.

### 9. Token and context efficiency without quality loss

The goal is to remove rediscovery and noise, not to remove verification.

#### High-value practices

- Start one chat per coherent outcome or tightly related diagnostic thread, not
  one permanent chat for the entire engine. Use a new chat after an accepted
  tracer or when the work genuinely branches. Use `/compact` when a still-active
  thread becomes long.
- Put durable decisions, accepted invariants, exact commands, and reproducible
  failures in the repository. Point the prompt to them instead of pasting them
  again.
- Keep each task prompt small: goal, only relevant paths/evidence, the few
  invariants that matter, non-goals, and done-when checks. The root instructions
  already supply general policy.
- Save full compiler logs, state dumps, profiles, and captures as artifacts.
  Return the failing command, a small relevant excerpt, and the path instead of
  flooding the conversation with thousands of lines.
- Search narrowly and run the smallest affected test first. Broad scans and full
  suites belong at risk/gate boundaries.
- Give routine mechanical work a lower/default reasoning level; use higher
  reasoning for architecture, ambiguous behavior, concurrency, renderer bugs,
  and difficult root-cause analysis. Reasoning level is not a substitute for
  tests.
- Keep one canonical command path. Stable presets/scripts cost fewer rediscovery
  turns than prose describing slightly different commands in every chat.
- Sync documentation when its source of truth changes, then run one broader
  synchronization at session end. Do not rewrite every design document after a
  private refactor.
- Convert a workflow into a skill only after it works on at least two
  representative cases, matching this repository's existing rule. A premature
  skill makes incorrect assumptions durable.
- Add an `AGENTS.md` rule after the same mistake recurs, not for every imagined
  failure. OpenAI gives the same “mistake twice, then retrospective” guidance.

#### False economies to avoid

- Skipping compilation, regression tests, capture inspection, or diff review.
- Asking for terse answers while leaving the implementation unverified.
- Reusing a bloated chat only because it contains old context that should have
  been written into the repository.
- Loading every design/research document for a small isolated change.
- Adding multiple MCPs or agents for work that local commands handle directly.
- Running full clean builds, all seeds, all scale counts, RenderDoc, and both
  operating systems after every small edit.

## Options and tradeoffs

### Option A — Detailed step-by-step prompts for every task

**Benefit:** predictable sequence when the author already knows the correct
implementation path.

**Cost:** duplicates durable instructions, embeds uninspected assumptions,
constrains diagnosis, consumes context, and can produce superficially compliant
but architecturally wrong work.

**Verdict:** reserve for process-critical tasks.

### Option B — Goal-only autonomous prompts

**Benefit:** low prompt cost and high agent freedom.

**Cost:** unstated invariants and acceptance criteria invite scope drift and
make “done” subjective.

**Verdict:** suitable only for trivial, low-risk work whose conventions and
checks are already unambiguous.

### Option C — Goal + context + invariants + evidence contract

**Benefit:** protects intent while allowing inspection and adaptation; makes
completion reviewable; avoids repeating repository-wide rules.

**Cost:** someone must identify the important invariants and honest acceptance
oracle. Early in a subsystem, that may require a short planning/research pass.

**Verdict:** recommended default.

## Failure modes and gotchas

- Automated tests and the implementation can share the same misunderstanding.
  Scenario evidence and owner acceptance test product intent, not just internal
  consistency.
- A deterministic replay can deterministically preserve a bad behavior. It is a
  regression tool, not proof that the behavior is fun or realistic.
- Golden images can institutionalize defects. Never auto-update them merely to
  make CI green.
- A screenshot cannot detect flicker, poor frame pacing, confusing timing, or
  whether the player understands cause and effect. Use motion evidence and
  observation.
- Cross-machine pixel equality and bitwise floating-point simulation equality
  may be unavailable. Define honest, layered comparison rules rather than
  silently weakening every test.
- Overly broad static analysis or slow/flaky presubmit gates will be bypassed.
  Fast default checks need tight scope and reliable isolation.
- Excessive metrics create observation debt. Emit metrics tied to a named
  invariant, budget, diagnosis, or game-design question.
- A “bug ledger” can become another stale diary. Prefer a minimal executable
  regression fixture and a short linked issue/decision only when the defect has
  lasting architectural significance.
- Human review at every edit causes fatigue and becomes ceremonial. Reserve it
  for visible acceptance, ambiguous behavior, and tracer decisions.
- The harness itself can grow into a second product. Add interfaces that close a
  demonstrated feedback gap, not every theoretical inspection feature.

## Evidence and confidence

| Claim | Evidence class | Confidence |
| --- | --- | --- |
| The roadmap already has visual evidence at every material rendering/gameplay tracer | Direct repository audit | High |
| Review packet metadata and approval were unspecified before integration | Direct repository audit | High |
| Goal/context/constraints/done-when and plan-first guidance match current Codex recommendations | Current official OpenAI documentation | High as of access date |
| Deterministic, isolated, fast tests reduce uninformative test failures | Primary published engineering guidance and project-specific inference | High for the principle; implementation details remain project-specific |
| Exact cross-GPU pixel goldens are too brittle as the only oracle | OpenGL/platform engineering inference; must be verified on supported hardware | High |
| Two failed fixes is the right stop-loss | Proposed project policy, not an empirical universal | Medium; review after real defects |
| The exact CI/headless strategy will work on Linux and Windows | Unresolved until Phase 0/1 toolchain and context smoke | Low |
| The recommended cadence will reduce token use without quality loss | Inference from reduced rediscovery/noise while preserving affected checks | Medium-high; measure actual sessions |

## Adversarial review record — 2026-08-15

- **Confirmed:** The central task contract, closed feedback loop, proportional
  verification cadence, and owner visual gate fit the approved engine/product
  boundaries and do not expand the first playable.
- **Qualified:** A manifest field set can be required now, but its final JSON
  schema and executable CLI must wait for a real Tracer 0 producer/consumer.
- **Qualified:** Linux CI should reproduce a proven local loop after Tracer 0;
  it is not a substitute for the still-unresolved native Windows context path.
- **Qualified:** `.clang-format` and `.clang-tidy` can be versioned now, but
  their exact compatibility and signal/noise remain unverified until the Phase 0
  Clang toolchain and project source exist.
- **Qualified:** Two failed materially different fixes is accepted as a
  provisional stop-loss, not a universal empirical threshold. Review it using
  actual defect evidence.
- **Rejected:** No proposal to add more MCPs, create a custom automation service,
  or require human approval after every invisible edit was admitted.
- **Unresolved:** Cross-runner OpenGL/headless behavior and useful image
  thresholds remain platform evidence questions for Tracer 0 and later gates.

No material platform, dependency, asset, gameplay, ownership, or release
decision was overturned. The owner explicitly requested integration after this
review.

## Integration record

The accepted direction is now implemented as documentation/configuration and
tracked as runtime work:

1. [`../DEVELOPMENT_WORKFLOW.md`](../DEVELOPMENT_WORKFLOW.md) owns the task
   contract, cadence, bug protocol, evidence, review, and chat lifecycle.
2. [`../review/HUMAN_VISUAL_REVIEW.md`](../review/HUMAN_VISUAL_REVIEW.md) owns
   the human packet and golden-promotion rule.
3. [`../plans/agentic-development-workflow.md`](../plans/agentic-development-workflow.md)
   decomposes the remaining harness/CI work.
4. `AGENTS.md`, `ROADMAP.md`, the harness guide, README, and session-end skill
   route future contexts to those authorities.
5. `.clang-format` and `.clang-tidy` are present as provisional configurations;
   execution remains unchecked.

Runtime test labels, artifact emission, failure retention, CI, seed packets, and
native platform captures remain unchecked in `ROADMAP.md` until they actually
run.

## References

All web sources were accessed 2026-08-15.

- OpenAI, [Codex best practices](https://learn.chatgpt.com/guides/best-practices).
- OpenAI, [Prompting](https://learn.chatgpt.com/docs/prompting).
- OpenAI, [Custom instructions with
  `AGENTS.md`](https://learn.chatgpt.com/docs/agent-configuration/agents-md).
- OpenAI, [Model Context Protocol](https://learn.chatgpt.com/docs/extend/mcp).
- OpenAI, [Skills](https://developers.openai.com/plugins/concepts/skills).
- Kitware, [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html).
- LLVM/Clang, [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html).
- LLVM/Clang, [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html).
- Khronos, [`KHR_debug` specification](https://registry.khronos.org/OpenGL/extensions/KHR/KHR_debug.txt).
- Google, [Software Engineering at Google: Testing
  Overview](https://abseil.io/resources/swe-book/html/ch11.html).
- Google, [Software Engineering at Google: Larger
  Testing](https://abseil.io/resources/swe-book/html/ch14.html).
- Google, [Software Engineering at Google: Continuous
  Integration](https://abseil.io/resources/swe-book/html/ch23.html).

## Recommended next step

Complete the first unchecked Phase 0 toolchain item. Once Clang is available,
validate the provisional formatting/static-analysis configuration, then build
the Tracer 0 command/test/capture loop in the order tracked by
[`../plans/agentic-development-workflow.md`](../plans/agentic-development-workflow.md).
