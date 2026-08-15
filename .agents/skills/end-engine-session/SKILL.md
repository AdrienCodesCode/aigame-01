---
name: end-engine-session
description: Verify and wrap up work on the custom C++ voxel engine and Border Collie game. Use when the user invokes $end-engine-session or asks to end, wrap up, checkpoint, or hand off a development session. Run proportional build and test checks, synchronize durable documentation through update-project-docs in wrap-up mode, and leave ROADMAP.md with an evidence-backed next step.
---

# End Engine Session

Leave the repository in a state another context window can resume without relying on conversation memory.

## 1. Establish Actual State

1. Read `AGENTS.md`, `ROADMAP.md`, the relevant design or architecture documents, and any active plan.
2. Inspect `git status --short`, the relevant diff, and all files behind claims made during the session.
3. Preserve unrelated user changes. Do not stage, commit, discard, publish, or deploy unless explicitly requested.
4. Treat unchecked tasks and prior handoff prose as leads, not proof.

## 2. Verify Proportionally

Use documented commands when they exist. Never invent a passing result.

1. If CMake presets exist, configure and build the touched targets with the relevant debug preset.
2. Run focused tests first, then broader tests only when risk justifies them.
3. Run the documented headless or windowed smoke tracer when rendering, input, platform, or startup behavior changed.
4. Run shader validation, sanitizers, static analysis, frame capture, or debugger checks only when the changed area calls for them.
5. Record the exact command, platform, build type, result, and important limitation for each check.
6. Run `git diff --check` and inspect for generated files, captures, dumps, caches, secrets, or unrelated edits.

MCP registration is not verification. If an MCP tool is unavailable in the current session, report the restart or host requirement and use an ordinary local diagnostic when it provides equivalent evidence.

## 3. Audit the Documentation

Read `.agents/skills/update-project-docs/SKILL.md` completely and follow it in **wrap-up mode**. Pass it the verified implementation state, tests, measurements, blockers, decisions, and recommended next step.

In this repository, `ROADMAP.md` is the cross-context continuation source. Replace its current checkpoint with a concise current-state snapshot; do not add a second diary-style handoff file unless the repository policy changes.

## 4. Recheck After Documentation Edits

1. Rerun `git diff --check`.
2. Validate changed local Markdown links.
3. Rerun a build or test only if cleanup changed build-relevant files.
4. Confirm completed roadmap boxes have direct evidence and leave unrun work unchecked.

## 5. Report the Handoff

Finish with:

- verified build, tests, smoke checks, and measurements;
- documents changed and why;
- installed tools that require a new Codex or editor session;
- blockers or manual platform checks still outstanding;
- noteworthy unrelated or uncommitted changes;
- the first unblocked unchecked roadmap item.
- one explicit context recommendation from `docs/DEVELOPMENT_WORKFLOW.md`:
  normally start a fresh chat after a completed session/outcome, continue the
  same chat when an unresolved reproduction benefits from its diagnostic trail,
  or `/compact` before continuing when that same trail has become bloated.

Never tell the owner merely to “clear the chat.” A fresh-chat recommendation
must name the repository checkpoint and relevant plan/design files the new
context should read.

Do not begin a new feature during wrap-up unless the user explicitly asks.
