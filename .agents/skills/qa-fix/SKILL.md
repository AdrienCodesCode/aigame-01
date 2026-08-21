---
name: qa-fix
description: Work a filed QA issue from docs/qa/open to closed — reproduce it, implement the smallest sound fix, run the mapped CTest suites, record the resolution with evidence, archive the file, and regenerate the index. Use when the owner points at a QA id, asks to work through the tracker, or says to fix filed issues.
---

# QA Fix — take a filed issue to closed

Agents may fix filed QA issues; the owner directs which. This skill is the
discipline around that, so the tracker stays trustworthy.

Convention: `docs/qa/README.md`. Protocol: the bug and regression protocol in
`docs/DEVELOPMENT_WORKFLOW.md` — this skill is how its output gets recorded.

## Picking work

If the owner named an id, work that. If they said "work through the tracker",
read `docs/qa/INDEX.md` and propose an order **before** starting: S1 first, then
group by area so related fixes share one verification pass. Confirm the batch,
then go.

Never start on an `unconfirmed` issue by guessing. Either investigate it into
`confirmed`/`plausible` first, or tell the owner what reproduction you need.

## Per issue

### 1. Re-read before trusting

Read the issue file completely. Its investigation may be stale — the code may
have moved since filing. Re-verify that the cited `file.cpp:line` still says what
the issue claims. If the issue is now wrong, fix the issue file first.

Set `status: in-progress` if the fix will span more than the current turn.

### 2. Reproduce before changing anything

Run the reproduction named in `## Symptom` and confirm the failure with your own
eyes. Reduce it to the smallest named scenario, seed/replay, preset, platform,
and tick/frame that still fails. Add a failing regression before patching when
feasible — a defect that can silently return is not closed.

If the reproduction does not fail for you, say so and stop: that is new
information about the issue, not permission to patch anyway.

### 3. Read the contract before the code

Open the `rule:` doc named in the frontmatter, plus `src/README.md` for the
ownership boundary and any ADR in `docs/decisions/` that governs it. The fix
must satisfy the documented invariant, not just make the symptom disappear.

Hard boundaries a QA fix never crosses:

- Game rules never see SDL scancodes, buttons, axes, windows, or events, and
  never receive render-frame timing.
- Presentation interpolates published snapshot copies and never feeds back into
  simulation.
- `FixedStepAccumulator` stays the only render-to-simulation scheduler.
- Each tick derives the next sheep buffer from the immutable prior buffer.
- Replay and state contracts are versioned; compatibility validation completes
  before any mutation. Changing a format means changing its version and
  `docs/formats/GAMEPLAY_REPLAY_AND_STATE.md`.
- Harness argument parsing is positional and strict — add a new argv shape
  rather than loosening an existing one.
- An accepted golden is never promoted, overwritten, or loosened without the
  owner's explicit accept. Generating a candidate is allowed; promoting it is
  not.
- Never add a dependency as part of a QA fix.

### 4. Fix at the right altitude

Fix the cause named in the issue, not the symptom — but do not expand into a
refactor the issue did not ask for. If the honest fix needs a redesign, **stop**:
leave the issue open, add a note saying why, and tell the owner it needs a plan
doc or an ADR.

Never hide a symptom with an arbitrary delay, retry, constant, or special case.
After two materially different fixes fail the same reproduction, stop
guess-and-patch iteration: minimize the fixture again, inspect actual state,
challenge the assumed invariant, and report the new evidence or blocker.

If you find sibling defects while fixing, file them with `qa-intake` rather than
absorbing them silently.

### 5. Verify

Run the entries in the issue's `verify:` list, plus whatever the change itself
implicates — those two sets are often different, and the second one wins.

Baseline for any nontrivial change:

```bash
cmake --build --preset dev
ctest --preset dev
cmake --build --preset dev --target format-check
cmake --build --preset dev --target clang-tidy-check
```

Add `dev-sanitized` for memory, lifetime, or ownership work, and the `release`
preset when a `performance` budget is in play. Report the exact command, preset,
platform, and result for each check. Never claim done with a failing suite, and
never record a WSL run as native Windows or native Linux 4.6 evidence — if the
issue needs a display, name it as owner-side `manual:` evidence still
outstanding.

**If no existing suite would have caught this bug, say so.** For S1 and S2, add
or extend a test as part of the fix. If the invariant must hold at several call
sites, the test needs to cover the set, not the one site that failed.

### 6. Close it

Update the frontmatter:

```yaml
status: fixed # or wont-fix / duplicate / not-a-bug
closed: <today>
fix: <short sha> # once committed
```

Append a `## Resolution` section: what changed in one or two sentences, and the
evidence — commands, preset, platform, and results by name. Then move the file
and reindex:

```bash
git mv docs/qa/open/QA-NNN-slug.md docs/qa/closed/
cmake -DMODE=index -P tools/qa/qa-tracker.cmake
cmake -DMODE=check -P tools/qa/qa-tracker.cmake
```

`wont-fix`, `duplicate`, and `not-a-bug` also move to `closed/` with a
`## Resolution` explaining the call. Nothing is deleted; a `duplicate` names the
issue it duplicates.

### 7. Docs and roadmap

- If the fix changed behavior a source README, ADR, format doc, or
  `tests/README.md` describes, update it in the same session. A contract doc
  that now lies is worse than the original bug.
- **S1**: name the issue and its evidence in the `ROADMAP.md` Current checkpoint.
  A gate blocker is roadmap-visible by definition.
- **S2–S4**: one checkpoint line per session covering the batch, not one per
  issue.
- Never check a roadmap item for a result that was not run.

## Reporting

Per issue, in a line or two: id, what changed, the suites that passed with their
platform, and anything left open. Then the batch summary, then the
fresh-chat / continue / compact recommendation the development workflow
requires. If you closed something as `not-a-bug`, lead with that — the owner
reported it in good faith and deserves to know why it was not one.
