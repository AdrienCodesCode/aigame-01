---
id: QA-007
title: Root README still says implemented avoidance and behavior-transition rules are unimplemented
status: fixed
severity: S3
confidence: confirmed
area: docs
reporter: agent
reported: 2026-08-22
phase: 3
platform: any
rule: README.md
closed: 2026-08-22
verify:
  - target:qa-check
  - manual:root README implementation summary matches the Phase 3 checkpoint
---

## Symptom

[`README.md`](../../../README.md) says that obstacle and drop avoidance and
behavior transitions remain unimplemented. Both are implemented, tested, and
recorded as verified Phase 3 state, so the quick-start overview sends a reader
to an older project boundary.

Observed result (2026-08-22, source inspection while fixing QA-004): the stale
sentence appears in the native-engine quick start immediately after the list of
implemented paired fixtures.

## Investigation

[`ROADMAP.md`](../../../ROADMAP.md) records completed obstacle/drop avoidance
and settled/alert/driven/recovering behavior transitions in the current
checkpoint. [`ADR 0008`](../../decisions/0008-obstacle-and-drop-avoidance.md)
and [`ADR 0009`](../../decisions/0009-behavior-transitions-and-arousal.md) are
accepted, while `wide_eye.gameplay_simulation` contains paired fixtures and
oracles for both rules. [`tests/README.md`](../../../tests/README.md) describes
that coverage in detail.

Falsification attempted: the roadmap, both owning ADRs, the current source
boundary, and registered gameplay-simulation test were checked to distinguish
implemented authoritative rules from planned presentation or terrain work.
Avoidance against an interior procedural ledge and behavior-driven steering are
still deferred, but the two rules named by the root README are not.

## Root cause

The root overview was not synchronized when ADR 0008 and ADR 0009 completed;
its broad deferred-work sentence still describes the earlier Phase 3 state.

## Expected behavior

The concise root overview should distinguish implemented obstacle/drop
avoidance and observational behavior transitions from the still-deferred
terrain pressure factor, damping, interior-terrain avoidance, and any feedback
from behavior labels into steering.

## Fix notes

Scope is the stale native-engine summary in `README.md`. Do not change a
gameplay rule, scenario, test oracle, roadmap checkbox, or accepted measurement.
Regenerate and validate the QA index after correcting the wording.

## Resolution

Fixed on 2026-08-22. The root README's native-engine quick start now records
steering-level obstacle and drop avoidance and the settled, alert, driven, and
recovering behavior states as implemented, with the narrowness the Phase 3
checkpoint records kept intact: each of the two is switched on only in its own
paired fixture and in the deliberately maximal
`sheep-all-influences-diagnostic`, the behavior states are read by no steering
term, the analytic paddock remains the last positional authority, and drop
avoidance is exercised against the paddock's own outer edge alone. The terrain
pressure factor, damping, avoidance against an interior ledge, arousal causes
other than the dog, any effect of behavior on steering, and the objective loop
stay recorded as unimplemented. The same paragraph's scenario list was completed
in the same edit — it named 25 of the 30 registered scenarios, omitting
`sheep-avoidance-off`, `sheep-avoidance-on`, `sheep-behavior-transitions-off`,
`sheep-behavior-transitions-on`, and `sheep-all-influences-diagnostic`, which is
the same desynchronization recorded in `## Root cause`. No gameplay rule,
scenario definition, test oracle, roadmap checkbox, accepted measurement, or
source file changed.

Evidence, native Windows 11 with the MSVC 2022 Build Tools CMake and the
existing `build/Windows/dev` tree. No compile or CTest run is implicated,
because the change is confined to prose in `README.md`.

- `target:qa-check` — `cmake -DMODE=check -P tools/qa/qa-tracker.cmake` and
  `cmake --build --preset dev --target qa-check` both passed after
  `cmake -DMODE=index -P tools/qa/qa-tracker.cmake` regenerated
  `docs/qa/INDEX.md` ("QA tracker check passed: 0 open, 7 closed").
- `manual:root README implementation summary matches the Phase 3 checkpoint` —
  performed as an agent cross-check, not as owner sign-off. The corrected
  paragraph was compared statement by statement against the `ROADMAP.md`
  Current checkpoint ("Verified completed state" and "Known limits"),
  [ADR 0008](../../decisions/0008-obstacle-and-drop-avoidance.md),
  [ADR 0009](../../decisions/0009-behavior-transitions-and-arousal.md), and the
  equivalent paragraph in [`src/README.md`](../../../src/README.md). The
  scenario list was compared mechanically against the 30 `.name =` entries in
  [`gameplay_scenario.cpp`](../../../src/game/gameplay_scenario.cpp) and is now
  identical to that set; `sheep_avoidance` and `sheep_behavior` are each
  enabled in exactly two definitions, which is what the new sentence claims.
  Owner review of the wording is the ordinary diff review rather than an
  outstanding run.
- Documentation validation from [AGENTS.md](../../../AGENTS.md): every relative
  link in `README.md` resolves, including the two new ADR links; heading
  hierarchy, lists, tables, and images are untouched; and `git diff --check`
  reported nothing.
