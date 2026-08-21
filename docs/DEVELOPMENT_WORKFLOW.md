# Development workflow and feedback loop

**Status:** Accepted project workflow

This document is the authoritative implementation loop for the Wide Eye C++
voxel engine. `AGENTS.md` supplies durable project constraints, `ROADMAP.md`
orders the work, and this file defines how one coherent outcome is framed,
implemented, observed, reviewed, and preserved.

The general supporting research and tradeoffs are recorded in
[`research/agentic-development-workflow.md`](research/agentic-development-workflow.md).
Renderer-experiment evidence and graphics-backend timing are supported by
[`research/opengl-to-vulkan-feasibility.md`](research/opengl-to-vulkan-feasibility.md).

## What counts as one coherent outcome

A coherent outcome is the smallest result that can receive one meaningful
acceptance decision. Examples include:

- prove that an SDL3/OpenGL core context opens and closes cleanly;
- render and reproducibly capture one voxel cube;
- make negative world/chunk/local coordinate conversions correct;
- reproduce and fix one named replay failure; or
- make dog-pressure direction visible and explainable in one scenario.

Do not combine unrelated architecture, gameplay, rendering, tooling, and polish
work merely because they fit in one chat. A tracer may contain several coherent
outcomes, but each outcome should leave the repository buildable and reviewable.

## Task contract

Before material implementation, establish the following from the user's request,
the current roadmap checkpoint, and relevant project files. Do not force the user
to fill out a form when the repository already answers an item.

```markdown
Goal:
  One observable outcome.

Context:
  Only the source, design, roadmap, replay, capture, or error paths that can
  change the result.

Invariants:
  The architecture, behavior, ownership, determinism, performance, safety, and
  accepted visual rules that must remain true.

Non-goals:
  Adjacent work intentionally excluded from this outcome.

Done when:
  Exact build, test, scenario, capture, measurement, and human evidence required.

Stop and ask if:
  A discovered fact would change approved scope, a dependency, public behavior,
  data compatibility, an accepted baseline, or an external/destructive action.
```

Start with the desired result. Prescribe steps only when process or order is part
of correctness, such as a controlled reproduction, destructive operation,
migration, dependency transition, accepted architecture boundary, or release
procedure. Otherwise inspect first and choose the narrowest sound implementation.

## Standardized feedback loop

```text
task contract
    -> inspect and reproduce/baseline
    -> make one bounded change
    -> configure/build/test/run
    -> collect logs + state + replay + capture + measurements
    -> automated oracles + agent diff review + human product judgment
    -> preserve regression/baseline + update the roadmap truth
    -> context recommendation for the next coherent outcome
```

The loop is complete only when the agent can both act on the program and observe
whether the intended result occurred. An MCP registration, successful compile,
open window, or generated screenshot is one signal—not completion by itself.

## Verification cadence

### After each coherent code change

Run the cheapest checks that can falsify the change:

1. Build the touched target.
2. Run the focused unit/invariant tests.
3. Run the affected named scenario or exact bug reproduction.
4. Inspect the relevant output and the code diff.
5. Add or update a regression fixture when behavior or a defect changed.

Do not postpone compilation, focused tests, or exact reproduction until session
end. A discussion-only or documentation-only turn does not require an engine
build that cannot affect its result.

### After a visual or behavioral change

Also generate the relevant deterministic capture, state dump, metrics, and debug
view. The agent inspects these before requesting owner review. Ask for owner
review when an accepted player-facing result may change or a tracer reaches its
visual gate; do not demand manual approval for an invisible refactor whose
automated evidence remains unchanged.

Use the reusable [human visual-review packet](review/HUMAN_VISUAL_REVIEW.md).

### Renderer experiment evidence

When a coherent outcome introduces or materially changes a render pass, GPU data
flow, lighting/material calculation, temporal effect, culling/LOD path, upload
path, or graphics backend, make that feature observable without building a
speculative diagnostics framework around features that do not yet exist.

- Use one deterministic representative scenario, camera route, seed, exposure,
  viewport, and graphics profile. Once renderer tuning could overfit that state,
  also use at least one small owner-controlled holdout scene, camera, or seed
  that was not the tuning target.
- Add feature-owned debug output for the inputs, important intermediate result,
  rejection/fallback state, and final contribution needed to diagnose that
  feature. Depth, normals, motion vectors, shadow masks, history rejection, LOD,
  overdraw, or residency views are required only when their owning feature
  exists and the output answers a named question.
- Give material passes, resources, and command regions stable human-readable
  labels where the API and tooling support them. Record per-pass GPU time and
  relevant draw, dispatch, upload, or memory values when they inform a named
  budget, regression, or optimization decision; retain total frame evidence.
- Treat exact or perceptual image comparison as one signal. Record the tool and
  version, reference and candidate, resolution, color/exposure assumptions,
  masks, threshold, aggregate result, and difference map. A FLIP or similar
  score does not replace semantic assertions, motion evidence, or owner review.
- Add a slow offline or higher-quality reference only for a bounded calculation
  with a defined reference question, inputs, version, and tolerance. Do not make
  an offline renderer a prerequisite for unrelated visual work.
- Use graphics validation and a GPU capture when a backend migration, pass
  interaction, driver issue, or difficult GPU defect needs them. Record the
  backend, enabled validation modes, tool/version, capture path, and relevant
  findings; a clean capture alone is not proof of correctness or quality.

The pass and its minimum useful instrumentation should land in the same coherent
outcome. Broader dashboards, render graphs, capture automation, and comparison
infrastructure still require a demonstrated repeated need.

### At a tracer or milestone gate

Run, as applicable:

- clean configure and build using checked-in presets;
- the full relevant unit, integration, and named-scenario suite;
- sanitized development tests;
- documented headless smoke and replay tests;
- affected time, memory, allocation, startup, and package budgets;
- a candidate human review packet; and
- an explicit owner verdict where appearance, motion, feel, or product intent is
  part of acceptance.

### At session end

- Inspect status and the accumulated diff.
- Run `git diff --check` and proportional broader verification.
- Audit for accidental captures, dumps, caches, secrets, or generated files.
- Synchronize only documentation whose truth changed.
- Update the `ROADMAP.md` checkpoint with evidence and the first next action.
- Give the owner the context recommendation defined below.

### Periodic and release gates

Native Linux/Windows matrices, long population/seed sweeps, dependency and
license audits, packaging, deeper static analysis, and GPU capture belong at
their named roadmap gates. They are not routine after every edit.

## Verification labels and default suites

Once the harness exists, classify checks explicitly rather than relying on
tribal knowledge. Use CTest labels for automated checks; record `manual` as an
evidence category outside CTest when it cannot be automated.

| Label | Purpose | Default cadence |
| --- | --- | --- |
| `unit` | Pure, fast math/data/algorithm invariants | Every affected change |
| `scenario` | Deterministic gameplay or engine behavior | Every affected change |
| `headless` | Executable/context/capture smoke without manual window use | Affected changes and gates |
| `sanitizer` | ASan/UBSan configuration | Tracer gates and risky memory/lifetime work |
| `performance` | Named timing, allocation, memory, startup, or scale budget | Affected optimization and gates |
| `manual` | Hardware, input, visual, motion, feel, or playtest acceptance | Named roadmap gates only |

The normal development preset must stay fast and reliable. Slow or
machine-specific checks need a named command and must never masquerade as having
run when they were unavailable.

## Bug and regression protocol

For a nontrivial defect:

1. Record the observed symptom.
2. Reduce it to the smallest named scenario, seed/replay, platform/configuration,
   and tick/frame that still fails.
3. Name the violated invariant or missing expected observation.
4. Use logs, state, assertions, or debug views to localize the ownership boundary.
5. State one hypothesis and the evidence that would support or falsify it.
6. Add instrumentation or a failing regression before patching when feasible.
7. Apply the smallest coherent fix.
8. Re-run the exact reproduction, focused neighbors, and any broader risk gate.

For a trivial compiler or formatting correction, use proportionate judgment.

A nontrivial defect is also **filed**, so it survives the chat that found it.
`docs/qa/` is the defect lane: the `qa-intake` skill files the investigated
issue and the `qa-fix` skill carries it to closed with named evidence. The
convention, the frontmatter schema, and the tracker commands are in
[docs/qa/README.md](qa/README.md). Steps 1–8 above are what the issue's
`## Symptom`, `## Investigation`, `## Root cause`, and `## Fix notes` sections
record; filing does not replace the protocol, it captures its output.

After two materially different fixes fail the same reproduction, stop
guess-and-patch iteration. Minimize the fixture again, inspect actual state,
bisect when history permits, challenge the assumed invariant, and report the
new evidence or blocker. Never hide the symptom with an arbitrary delay, retry,
constant, or special case without a demonstrated reason.

Failed scenarios must retain enough evidence to reproduce the failure: command,
build/preset, platform, seed, replay, relevant log, state/metrics dump, capture
when applicable, and artifact manifest. Retain only privacy-safe, non-secret
inputs.

## Verification artifacts and accepted baselines

Generated evidence belongs under the ignored `artifacts/` tree until explicitly
accepted. Each material packet eventually needs a versioned machine-readable
manifest with at least:

- schema version and artifact hashes;
- commit/worktree state and build preset;
- OS, CPU, GPU, driver, graphics backend/API, and shader
  language/compiler/runtime versions;
- executable/scenario/replay versions, seed, tick/frame, and simulation rate;
- camera, viewport, graphics profile, and relevant flags;
- commands and pass/fail results;
- timing, memory, and allocation measurements required by the gate;
- representative/holdout designation plus any required pass/debug-output,
  comparator, offline-reference, validation, or GPU-capture metadata; and
- links among the normal frame, debug frame, motion evidence, state, metrics,
  and logs.

Until the executable exists, these are required fields, not a claim that a final
JSON schema or CLI has been implemented.

An agent may generate a candidate golden but may never silently promote,
overwrite, or loosen comparison thresholds for an accepted baseline. Promotion
requires the owner's explicit `accept` verdict recorded with the packet.

Use exact comparisons for deterministic integer/state products where supported,
numeric or structural tolerances for floating-point state, and reference-machine
image comparison plus semantic/human review for rendered output. Cross-GPU pixel
identity is not assumed.

## Review checklist

Before accepting a coherent outcome, inspect proportionally for:

- match to the task contract and absence of accidental adjacent scope;
- ownership, lifetime, error paths, and thread boundaries;
- coordinate, integer/float, allocation, and serialization boundaries;
- deterministic tick/replay behavior and version compatibility;
- high-severity graphics API/validation diagnostics and visible debug evidence;
- performance-budget impact on the named low target;
- tests that could share the same misunderstanding as the implementation;
- generated artifacts, dependency/license changes, and secrets; and
- whether documentation describes observed truth rather than intended support.

Codex `/review` or another review pass may assist at milestone gates. It does not
replace the owner's judgment of product intent, readability, motion, or fun.

## Chat and context lifecycle

Repository files—not one permanent conversation—carry project memory. At the end
of every completed coherent outcome, the agent must include one explicit context
recommendation in its final handoff:

- **Fresh chat recommended:** the current outcome is accepted or complete and
  the next task is independent. Point the new chat to `AGENTS.md`, the current
  `ROADMAP.md` checkpoint, and the relevant plan/design paths.
- **Continue this chat:** the same outcome remains unresolved and its diagnostic
  history materially helps the next attempt.
- **Compact, then continue:** the same outcome remains active but the chat has
  accumulated substantial logs, discarded hypotheses, or unrelated history.
  Recommend `/compact`; do not discard the repository checkpoint or regression
  evidence.

Do not tell the owner to clear or refresh the chat mechanically after every
message. Recommend a boundary only when a coherent outcome ends or context
quality is materially deteriorating. “Start a fresh chat” is clearer than
“clear the chat,” because the finished conversation may remain useful as an
audit trail.

## Context and token discipline

- Read the roadmap checkpoint and only the documents that can change the task.
- Put durable rules in `AGENTS.md` or this document and detailed evidence in its
  owning research, plan, decision, replay, or artifact—not repeated prompts.
- Save long logs/profiles as artifacts; report the relevant excerpt and path.
- Search narrowly and run focused checks before broad repository work.
- Use one canonical command path through checked-in presets/scripts.
- Use ordinary reasoning for mechanical work and deeper reasoning for ambiguous
  architecture, concurrency, renderer, simulation, or root-cause work.
- Do not use extra MCPs, skills, or agents unless they close a demonstrated
  observation or coordination gap.
- Turn a repeated workflow into a skill after at least two representative
  successes; update durable instructions after the same mistake recurs.
- Never save tokens by skipping the affected build, regression, capture
  inspection, diff review, or truthful handoff.

## Definition of done

A coherent outcome is done only when:

- its task contract is satisfied or the unresolved blocker is explicit;
- affected automated and manual gates actually run or are named as unrun;
- failure/review artifacts are reproducible and correctly classified;
- no baseline was changed without owner approval;
- the diff contains no accidental adjacent work;
- authoritative docs and the roadmap checkpoint reflect verified truth; and
- the handoff contains the appropriate fresh/continue/compact context
  recommendation.
