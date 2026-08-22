---
id: QA-007
title: Root README still says implemented avoidance and behavior-transition rules are unimplemented
status: open
severity: S3
confidence: confirmed
area: docs
reporter: agent
reported: 2026-08-22
phase: 3
platform: any
rule: README.md
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
