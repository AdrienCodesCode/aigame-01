# Task: establish a proportional Three.js game foundation

## Inputs

- Game/design document: [PATH]
- Target browsers/devices: [TARGETS]
- First playtest question: [QUESTION]
- Online requirements: [NONE OR DESCRIBE]
- Performance budgets: [BUDGETS OR PROPOSE FOR REVIEW]

## Assignment

Audit the approved slice, then create only the architecture it needs. Use
TypeScript, Three.js, and the existing build tool; prefer Vite for a new browser
client. Separate the fixed-step simulation, rendering/interpolation, scene
lifecycle, input actions, assets, audio, game rules, UI, persistence, diagnostics,
and optional networking behind small interfaces. Use one composition root.

Choose collision and pathfinding from actual mechanics. Do not add a rigid-body
engine, ECS, UI framework, backend, or remote protocol without a concrete need.
Keep player-facing UI in accessible DOM unless canvas UI is justified.

## Deliverables

- Dependency diagram and enforced import rules.
- Minimal scene with resize, pause, input, teardown, and deterministic update.
- Asset manifest/loader boundary and failure behavior.
- Test split for pure rules, DOM systems, and real WebGL browser behavior.
- `README`, architecture note, game-loop note, and feature-extension guide.

## Gate

Stop before content expansion. The foundation must build, launch, dispose cleanly,
and pass one automated end-to-end smoke path on a target browser.
