# QA tracker — how defects get filed, fixed, and retired

This directory is the **defect lane**. It is where anything wrong with the
engine as it actually runs gets recorded: a crash, a broken invariant, a
non-deterministic replay, a wrong render, a scenario that no longer reproduces,
a control that feels wrong, a doc that describes code that does not exist.

It is deliberately not the roadmap lane and not the plan-doc lane:

| Lane          | Lives in           | For                                                                          |
| ------------- | ------------------ | ---------------------------------------------------------------------------- |
| **Roadmap**   | `ROADMAP.md`       | Which phase/tracer is current, what is verified, what the next action is     |
| **Plan docs** | `docs/plans/`      | Building something that does not exist yet — a feature, a subsystem, an ADR  |
| **QA issues** | `docs/qa/open/`    | Something that exists and is wrong, however small                            |

If it needs a design decision and a build, it is a plan doc. If it is "this is
broken / wrong / unexplained", it is a QA issue. When in doubt, file the QA
issue — it is cheap, and it can be promoted to a plan doc later.

This lane does not replace the bug and regression protocol in
[docs/DEVELOPMENT_WORKFLOW.md](../DEVELOPMENT_WORKFLOW.md); it is where that
protocol's output is written down and tracked.

## Layout

```text
docs/qa/
  README.md          this file — the convention (hand-maintained)
  INDEX.md           the checklist (GENERATED — never hand-edit)
  open/              QA-NNN-slug.md — active issues
  closed/            QA-NNN-slug.md — resolved, retired here with an outcome
  charters/          per-surface manual sweep checklists
```

`INDEX.md` is regenerated from the frontmatter of every issue file. Do not edit
it by hand — the edit will be overwritten, and a hand-maintained checklist is
exactly the artifact that goes stale and stops being trusted. The files are the
truth; the index is a view.

## Commands

No new toolchain dependency: the tracker tool is a CMake script, like
`tests/assert-*.cmake`.

```bash
cmake -DMODE=check -P tools/qa/qa-tracker.cmake   # validate schema, placement, index freshness
cmake -DMODE=index -P tools/qa/qa-tracker.cmake   # regenerate INDEX.md
cmake -DMODE=next  -P tools/qa/qa-tracker.cmake   # print the next free QA id
```

With a configured preset the same three run as build targets:

```bash
cmake --build --preset dev --target qa-check
cmake --build --preset dev --target qa-index
cmake --build --preset dev --target qa-next
```

`qa-check` is the guard against drift: it fails on an unknown field value, an
unknown key, a duplicate id, a resolved status still sitting in `open/`, a
closed issue with no `## Resolution`, or a stale `INDEX.md`. Run it before any
commit that touches the tracker. It is intentionally **not** wired into
`ctest --preset dev`: a docs-schema slip must not fail the engine test suite.

## Filing an issue

Use the **`qa-intake` skill** — `/qa <what you saw>` is the fast path mid-sweep.
Do not hand-write issue files: the skill takes the next free id, investigates the
claim against the actual code, and writes valid frontmatter. Hand-written files
drift out of schema and break the index.

The owner reports in whatever language is natural ("the sheep jitter when the
dog gets close on the left"). The agent's job is to turn that into a filed,
investigated, precisely-worded issue.

## Issue file format

Filename: `QA-NNN-short-slug.md`, zero-padded to three digits. The number is
permanent and never reused, including after the file moves to `closed/`.

```markdown
---
id: QA-014
title: Sheep separation reverses on an exact-overlap tick at the gate threshold
status: open
severity: S2
confidence: confirmed
area: game
reporter: owner
reported: 2026-08-21
phase: 3
platform: wsl-ubuntu-24.04
rule: src/README.md
charter: play-session
verify:
  - wide_eye.gameplay_simulation
  - wide_eye.sheep_spatial_grid
  - label:unit
---

## Symptom

What the owner saw, in the owner's terms, plus everything needed to reproduce
it: command line, preset, platform, scenario, seed, replay, and tick/frame.
Keep this section as-reported — do not rewrite the observation into a theory.

## Investigation

What the agent found in the code. Cite `file.cpp:line`. State plainly whether
the reported symptom is explained by the code or not, and label every statement
the way [AGENTS.md](../../AGENTS.md) requires: observed result (with build,
date, platform, method), inference, or unverified claim.

## Root cause

One paragraph. If unknown, say unknown — do not guess in a way that reads as
fact.

## Expected behavior

What it should do, and which invariant, ADR, format contract, or source README
says so.

## Fix notes

Scope, blast radius, ownership boundaries involved, and which suites must pass.
Written before the fix so the owner can judge it.
```

### Field reference

**`status`** — `open`, `in-progress`, `fixed`, `wont-fix`, `duplicate`,
`not-a-bug`. Only `open` and `in-progress` live in `open/`; everything else
belongs in `closed/`.

**`severity`**

|      | Meaning                                                                                                                                  |
| ---- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `S1` | Blocks the current tracer gate. A crash, a lost or corrupted state/replay contract, a broken determinism guarantee, an unpassable gate.  |
| `S2` | Major. A real behavior or rendering path is wrong, but there is a workaround or it is off the current tracer's critical path.            |
| `S3` | Minor. Wrong but survivable — a confusing debug view, a misleading log, an avoidable extra step, a doc that describes code inaccurately. |
| `S4` | Polish. Cosmetic or a small ergonomics win.                                                                                             |

Severity is impact on the current phase gate, not effort to fix.

**`confidence`** — the single most important field, and the one agents get wrong.

|               | Meaning                                                                                                                                          |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| `confirmed`   | The agent located the defect in code, or reproduced it with a named command on a named platform. Only then may the issue state it as fact.       |
| `plausible`   | The code is consistent with the report but the agent could not prove the path. The issue must say what could not be proven.                      |
| `unconfirmed` | Nothing found supporting the report, or it needs a run the agent cannot perform (native OpenGL 4.6, a display, a gamepad, a human motion check). |

An agent must never file `confirmed` on reasoning alone. `unconfirmed` is a
perfectly good outcome — it tells the owner exactly what to capture next. This
field is the tracker's expression of the evidence discipline in
[AGENTS.md](../../AGENTS.md): observed result, inference, and unverified claim
are not the same thing.

**`area`** — one of: `core`, `platform`, `render`, `voxel`, `game`, `tests`,
`build`, `tools`, `docs`. These match the ownership boundaries in
[src/README.md](../../src/README.md); use them to group the index and to pick
verify suites.

**`reporter`** — `owner` or `agent`. Agents file their own findings too (see
below); the distinction matters when judging whether something has been seen in
the real running program.

**`verify`** — what must pass before the issue closes. Each entry is one of:

- `wide_eye.<test>` — a CTest test name, e.g. `wide_eye.gameplay_simulation`;
- `label:<label>` — a CTest label: `unit`, `scenario`, `headless`, `sanitizer`,
  `performance`;
- `target:<target>` — a build target, e.g. `target:format-check`;
- `manual:<what>` — evidence outside CTest, e.g.
  `manual:native windows paddock-start visual review`. Manual entries name work
  only the owner can sign off.

**Optional:** `phase` (roadmap phase number), `platform` (where it was seen —
`wsl-ubuntu-24.04`, `windows`, `linux-ci`, `any`), `rule` (the owning source
README, ADR, or format doc), `charter` (which sweep found it), `blocks` /
`depends` (other QA ids), `closed` (date), `fix` (commit sha).

## Agents file issues too

When an agent investigating one report finds *other* real problems — the same
mistake at three more call sites, an adjacent invariant violation, a test that
passes while the bug exists — it files those as separate issues with
`reporter: agent`. It does not silently widen the original issue, and it does
not fix them inline without saying so.

That is the point of routing reports through an agent rather than a form: one
reported symptom usually has siblings.

A test suite that passes while the defect exists is **itself a finding**. File
it separately; do not fold it into the issue that exposed it.

## Fixing and closing

Use the **`qa-fix` skill** (`/qa-fix`). Agents may fix filed issues; the owner
directs which. The skill handles the reproduction, verification, the status
flip, the move to `closed/`, and the reindex.

An issue closes when its `verify` entries pass and the outcome is recorded in
the file:

```markdown
## Resolution

Fixed in `<sha>` on 2026-08-21. <What changed, in one or two sentences.>
Evidence: `ctest --preset dev` 24/24 on WSL Ubuntu 24.04.4 with Clang 18.1.3,
`wide_eye.gameplay_simulation`, `format-check`.
```

`wont-fix`, `duplicate`, and `not-a-bug` also move to `closed/` with a
`## Resolution` explaining the call. Nothing is deleted. A `duplicate` names the
issue it duplicates.

A fix that requires promoting, overwriting, or loosening an accepted golden
baseline stops at the owner's explicit accept — see
[AGENTS.md](../../AGENTS.md) and
[docs/DEVELOPMENT_WORKFLOW.md](../DEVELOPMENT_WORKFLOW.md). Generating a
candidate is allowed; promoting it is not.

## Charters

[`charters/`](charters/) holds one checklist per surface for manual sweeps. They
answer a question an empty tracker cannot: what has actually been exercised.
Tick items as you sweep; when an item fails, file a QA issue with `charter:` set
to that charter's slug, so coverage and defects stay linked.

## Relationship to the roadmap

QA work is a **sanctioned lane**: it does not need a per-issue approval, and it
does not count as work outside the current phase. What it does need:

- S1 issues are named in the `ROADMAP.md` Current checkpoint when found and when
  closed. A gate blocker is roadmap-visible by definition.
- S2–S4 batches get one checkpoint or decision-log line per session, not one per
  issue.
- A QA fix that turns out to need a real redesign stops being a QA issue. Write
  a plan doc in `docs/plans/` (or an ADR in `docs/decisions/`) and link it from
  the QA issue.
- Never check a roadmap item because a QA issue closed. Roadmap items are
  checked only for run results.
