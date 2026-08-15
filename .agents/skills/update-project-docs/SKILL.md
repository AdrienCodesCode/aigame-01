---
name: update-project-docs
description: Audit and synchronize documentation for the custom C++ voxel engine and Border Collie game. Use when the user invokes $update-project-docs, asks to update or clean up docs, completes roadmap work, makes an architecture decision, or requests a session wrap-up. Update only authoritative files whose responsibility changed, preserve evidence labels, and never convert an unverified claim into completed work.
---

# Update Project Documentation

Keep one durable source for each fact and make future context windows cheap to resume.

## Select the Mode

- Use **normal mode** for a standalone documentation sync.
- Use **wrap-up mode** when called by `end-engine-session` or when the user is ending a session. In wrap-up mode, replace the current checkpoint in `ROADMAP.md`.

## 1. Establish Documentation Impact

1. Read `AGENTS.md`, `README.md`, `ROADMAP.md`, and the documentation index if one exists.
2. Inspect `git status --short`, the relevant diff, current implementation, and actual verification output.
3. Classify statements as Goal, Hypothesis, Observed result, Inference, or Unverified claim as required by `AGENTS.md`.
4. Preserve unrelated user changes and third-party attribution.

Evaluate each responsibility, but edit only affected files:

| Document | Responsibility |
| --- | --- |
| `ROADMAP.md` | Ordered work, gates, checkboxes, evidence, and the current cross-context checkpoint. |
| `README.md` | Concise human orientation and navigation. |
| `AGENTS.md` | Stable agent behavior, evidence rules, and source-of-truth paths. |
| `docs/DEVELOPMENT_WORKFLOW.md` | Coherent-outcome contract, feedback loop, verification cadence, failure/baseline rules, and context handoff. |
| `docs/TECH_STACK.md` | Approved stack, support matrix, dependency policy, and performance targets. |
| `docs/VOXEL_ENGINE_OPTION.md` | Durable engine/product boundaries and staged custom-engine rationale. |
| `docs/AGENT_HARNESS_AND_TOOLS.md` | Tool availability, MCP/skill setup, trust boundaries, and verified commands. |
| `docs/review/*.md` | Reusable human-review packets and accepted-baseline policy; not run-specific evidence. |
| `docs/game-design/*.md` | Player fantasy, mechanics, loop, pressure, progression, and playtest questions. |
| `docs/research/*.md` | Sourced investigations with visible draft/review status; not implementation proof. |
| `docs/plans/*.md` | Approved implementation decomposition; not completion evidence. |
| `docs/decisions/*.md` | Consequential accepted decisions and supersession history. |
| `prompts/*.md` | Reusable prompt inputs, provenance, prerequisites, outputs, and gates. |

## 2. Apply Scoped Updates

- Link to an authoritative document instead of duplicating its content.
- Check a roadmap item only when its acceptance condition was actually met.
- Record exact versions, commits, dates, platforms, and commands for version-sensitive evidence.
- Distinguish configuration from current-session tool availability and implementation from intent.
- Do not update a file merely to refresh its date.
- Do not silently vendor mutable external text or erase authorship.

In wrap-up mode, the `ROADMAP.md` checkpoint should state:

- current phase and focus;
- verified completed state;
- tests or measurements actually run;
- blockers and platform-specific limitations;
- first unblocked unchecked item;
- exact files the next context should read.

Replace stale checkpoint content instead of appending a diary.

## 3. Validate

1. Verify each changed relative Markdown link resolves.
2. Search for stale renamed paths and contradictory current-phase statements.
3. Run `git diff --check`.
4. Review the diff for unsupported performance, compatibility, quality, schedule, or completion claims.
5. State which relevant build or runtime checks were not run.

## 4. Report

Name the documents changed and why, relevant categories that needed no update, verification performed, and the first remaining decision or roadmap item.
