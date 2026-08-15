---
name: plan-from-research
description: Challenge a custom-engine research document and turn it into an evidence-checked, incremental implementation plan. Use when the user invokes $plan-from-research or explicitly asks to plan, refine, or create roadmap work from a file under docs/research. Review the research adversarially, apply an architecture readiness gate, stop for material product or ownership decisions, and do not implement the plan.
---

# Plan From Research

Treat the source research as evidence to test, not an approved specification.

## 1. Validate the Input

1. Require one research Markdown file, normally under `docs/research/`.
2. Select it automatically only when exactly one obvious match exists.
3. Reject a plan or prompt file as the research source.
4. Preserve original authorship and review history.

## 2. Establish Current Evidence

Read:

- `AGENTS.md`, `README.md`, `ROADMAP.md`;
- the complete research file and directly relevant links or project files it names;
- relevant architecture, design, harness, and decision documents;
- current code, build configuration, dependency versions, tests, and measured results;
- current primary sources for disputed, consequential, or version-sensitive claims.

Use available semantic, debugger, or graphics MCP tools when they are relevant and callable, but do not block planning solely because a convenience integration needs a restart.

## 3. Perform One Adversarial Pass

1. Extract key claims, assumptions, constraints, and the proposed approach.
2. Search for contradictions, newer guidance, license or platform gaps, hidden prerequisites, failure modes, and simpler alternatives.
3. Compare the proposal with this repository's actual milestone and cross-platform C++/OpenGL boundaries.
4. Challenge scope, testability, frame-time and memory budgets, deterministic simulation, thread ownership, renderer/game separation, procedural-generation cost, and packaging impact where relevant.
5. Classify material findings as Confirmed, Qualified, Rejected, or Unresolved.

Add a dated `Adversarial review record` to the research file only when correction is part of the user's request. Otherwise report proposed corrections without mutating the source.

If the review overturns the central recommendation or materially changes scope, stop before writing a plan and ask for the product or architecture decision.

## 4. Apply the Readiness Gate

Classify the work:

- **Ready:** Existing boundaries support it.
- **Localized prerequisite:** A bounded foundation task is required; make it Phase 0 and say why.
- **Material decision:** It changes an approved platform, dependency policy, state owner, renderer contract, gameplay loop, asset rule, or release promise.

For a material decision, stop and present the smallest viable options and recommendation. Do not create an ADR or silently alter `ROADMAP.md` without approval.

## 5. Write the Plan

Write `docs/plans/<topic-slug>.md` with:

```markdown
# Plan: <Topic>

**Status:** Draft plan; not implemented
**Date:** YYYY-MM-DD
**Source research:** [relative link]
**Architecture readiness:** Ready | Localized prerequisite approved

## Objective and success criteria
## Scope and non-goals
## Verified current state
## Decisions and assumptions
## Prerequisites
## Implementation phases
## Verification matrix
## Performance and platform matrix
## Risks, rollback, and deferred work
## Definition of done
## Recommended first step
```

Each phase must have an outcome, likely files or components, dependency direction, checkable tasks, validation method, evidence artifact, and stopping condition. Keep tasks small enough for one coherent review. Do not claim exact APIs or class structures without evidence.

Update `ROADMAP.md` only when the user explicitly asks to integrate the plan. If asked, link the detailed plan and add only milestone-level checkboxes; do not duplicate every subtask.

## 6. Validate and Report

Verify changed links, run `git diff --check`, confirm no implementation or completion state changed, and report research corrections, readiness result, plan path, unresolved decisions, and the first implementation task.
