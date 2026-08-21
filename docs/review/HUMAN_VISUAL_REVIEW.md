# Human visual-review packet

Use this template when a Wide Eye roadmap gate or material player-facing change
requires owner judgment. Copy it beside the candidate artifacts or into the
relevant evidence record; do not edit this template with one run's results.

An open native window or a cherry-picked beauty shot is not a review packet.
Screenshots establish static appearance; motion evidence is required for
animation, flicker, temporal effects, flock response, and frame pacing.

## Review question

- **Coherent outcome:**
- **Question for the owner:**
- **What intentionally changed:**
- **What must remain invariant:**
- **Known limitations or unverified claims:**

## Reproduction metadata

| Field | Value |
| --- | --- |
| Date/time and timezone | |
| Commit and dirty-worktree state | |
| Configure/build preset | |
| Executable version | |
| OS and architecture | |
| CPU | |
| GPU and driver | |
| Graphics backend/API and shader language/compiler/runtime versions | |
| Scenario and version | |
| Replay and version | |
| Seed | |
| Simulation tick/rate | |
| Camera and viewport | |
| Graphics profile and relevant flags | |
| Representative or holdout designation | |
| Render pass/debug-output schema or version, if applicable | |
| Exact reproduction command | |
| Artifact-manifest path/hash | |

## Automated evidence

| Check or budget | Command/configuration | Result | Evidence path |
| --- | --- | --- | --- |
| Focused tests | | | |
| Scenario/replay | | | |
| Graphics validation/debug diagnostics | | | |
| Sanitizers, if required | | | |
| Frame-time/memory, if required | | | |
| Named per-pass budgets, if required | | | |
| Visual comparator/FLIP, if required | | | |
| Holdout scenario, if required | | | |

Name every required check that was unavailable or intentionally not run.

## Artifacts

| Artifact | Required when | Path | What to inspect |
| --- | --- | --- | --- |
| Normal frame | Static presentation matters | | Composition, geometry, lighting, silhouettes, readability |
| Matching debug frame | Geometry/behavior diagnosis matters | | Same seed, tick, camera, viewport; explanatory overlays |
| Pass-specific debug outputs | A render pass or GPU data flow changed | | Inputs, important intermediate result, rejection/fallback state, final contribution |
| Short clip or contact sheet | Motion/temporal behavior matters | | Animation, flicker, response, pacing, transitions |
| Accepted before frame/clip | A prior approved baseline exists | | Verify it is genuinely comparable |
| Candidate after frame/clip | The visible result changed | | Intended improvement and unintended regressions |
| Difference map/comparator report | Automated image comparison is used | | Tool/version, reference/candidate, masks, threshold, score, and localized errors |
| Offline reference | A bounded calculation needs a higher-quality oracle | | Reference question, inputs/version, tolerance, and known model differences |
| GPU capture | Validation or pass interaction needs inspection | | Backend/tool version, named passes/resources, warnings, timings, and first-cause evidence |
| State and metrics dump | Behavior/performance matters | | Objective state, invariants, timing, memory |
| Relevant logs | Warnings/errors matter | | High-severity and first-cause diagnostics |

## Owner review

Review the normal and diagnostic evidence together.

- [ ] The packet answers the stated question rather than showcasing unrelated
  polish.
- [ ] Scenario, seed, tick, camera, viewport, and profile are comparable.
- [ ] Geometry, collision cues, and objectives are readable.
- [ ] Dog posture, pressure/release, sheep response, and state changes are
  understandable when applicable.
- [ ] Motion is stable; no objectionable flicker, popping, judder, or confusing
  transition is visible.
- [ ] Debug views explain surprising output rather than merely adding noise.
- [ ] Representative and holdout evidence agree where a holdout is required; the
  candidate is not accepted from one showcase camera alone.
- [ ] Named render passes remain within their budgets where pass-level evidence
  is required.
- [ ] A perceptual score or offline reference, when present, supports rather than
  replaces semantic checks and direct visual judgment.
- [ ] The low-target performance evidence remains within the current budget when
  the change can affect it.
- [ ] Known limitations are acceptable for this tracer.

### Verdict

Select exactly one:

- [ ] **Accept** — promote the named candidate artifacts to the accepted
  baseline for this scenario/profile.
- [ ] **Revise** — keep the prior baseline; the candidate needs the changes
  described below.
- [ ] **Reject** — keep the prior baseline and abandon or redesign this
  candidate direction.

**Owner observation and required follow-up:**

**Owner/date:**

## Baseline rule

Only an explicit **Accept** verdict can promote a candidate golden. An agent must
not overwrite accepted artifacts, approve its own candidate on the owner's
behalf, or widen a comparison threshold simply to make a failure disappear.

When no prior baseline exists, acceptance creates the first baseline; it does
not prove cross-GPU pixel identity, player understanding, or fun beyond the
question and evidence recorded here.
