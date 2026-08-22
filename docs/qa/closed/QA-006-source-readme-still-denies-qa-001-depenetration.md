---
id: QA-006
title: Source ownership guide still says an obstacle-overlapping sheep is not pushed out after QA-001 added depenetration
status: fixed
severity: S3
confidence: confirmed
area: docs
reporter: agent
reported: 2026-08-22
phase: 3
platform: any
rule: src/README.md
closed: 2026-08-22
verify:
  - target:qa-check
  - manual:src README collision statement matches QA-001
---

## Symptom

[`src/README.md:219-221`](../../../src/README.md#L219-L221) says, "A sheep
whose cylinder already overlaps an obstacle is not pushed out." That statement
describes the behavior before QA-001 and contradicts the collision authority in
the current source.

Observed result (2026-08-22, WSL Ubuntu 24.04.4, source inspection at
`9cc5c7d`): [`resolve_sheep_against_paddock`](../../../src/game/sheep_rules.cpp#L83)
passes every sheep displacement to `PaddockCollisionField::resolve_cylinder_move`,
whose completed QA-001 correction depenetrates a body that starts overlapping an
obstacle. The closed
[`QA-001`](../closed/QA-001-paddock-collision-radius-band-passthrough.md)
records and verifies that behavior.

## Investigation

The stale sentence sits beside a separate statement that remains true:
sheep-versus-sheep and sheep-versus-dog hard body collision are absent. The two
claims must not be treated as one. QA-001 changed sheep-versus-paddock behavior
only; it did not add inter-agent collision.

Falsification attempted: the current resolver and the QA-001 issue were checked
to determine whether the push-out existed only in a test or temporary branch.
It is in the current authoritative rule at
[`paddock_collision.cpp`](../../../src/game/paddock_collision.cpp) and QA-001 is
closed with named verification evidence, so the README statement is definitively
stale rather than a description of an intentionally retained limitation.

## Root cause

The QA-001 documentation updates preserved the adjacent sheep-versus-sheep and
sheep-versus-dog limitation but did not replace the preceding sentence that
described the old obstacle-overlap behavior.

## Expected behavior

`src/README.md` is the source-ownership guide and must describe the current
collision boundary accurately: paddock obstacle overlap is corrected by the
game-owned collision field, while sheep-versus-sheep and sheep-versus-dog hard
body collision remain unimplemented.

## Fix notes

Scope is one paragraph in `src/README.md`. Replace only the stale
obstacle-overlap sentence, retain the explicit inter-agent collision limitation,
and link QA-001 if that makes the correction boundary clearer. No gameplay code,
test oracle, roadmap checkbox, or accepted measurement should change. Regenerate
and validate the QA index after the documentation correction.

## Resolution

Fixed on 2026-08-22. `src/README.md` now records that the game-owned paddock
collision field pushes an overlapping sheep out before resolving its requested
displacement, while preserving the separate limitation that sheep-versus-sheep
and sheep-versus-dog hard body collision remain absent. Evidence: targeted
source/ADR/QA-001 inspection and `cmake -DMODE=check -P
tools/qa/qa-tracker.cmake` on WSL Ubuntu 24.04.4.
