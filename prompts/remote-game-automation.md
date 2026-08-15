# Task: expose the smallest safe semantic test surface

## Inputs

- Engine/platform and existing test hooks: [CONTEXT]
- Critical automated scenario: [SCENARIO]
- Local, CI, QA-device, or remote need: [NEED]
- Threat boundary: [BOUNDARY]

## Assignment

First attempt the scenario with existing engine automation, browser semantics,
accessibility hooks, input APIs, screenshots, logs, and fixtures. Build a custom
protocol only for gaps that materially block testing.

If needed, define stable automation IDs, capability discovery, safe observation,
semantic actions, real input, deterministic waits, reset fixtures, assertions,
screenshots, and event capture. Bind locally by default. Production builds must
exclude privileged mutation and arbitrary console/file/memory access.

## Deliverables

- Build-versus-buy/gap assessment and threat model.
- Versioned minimal schema and environment capability profiles.
- Typed client/adapter for one complete representative scenario.
- Auth, malformed-input, disconnect, held-input, reset, and production-gating
  tests.
- Instructions for adding one new entity, action, or state assertion.

## Gate

Measure flake rate and maintenance cost. Reject the custom surface if ordinary
automation answers the same question more simply.
