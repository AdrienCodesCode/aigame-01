# Task: establish a proportional Unity game foundation

## Inputs

- Unity/editor version and render pipeline: [VERSIONS]
- Target platforms and inputs: [TARGETS]
- Approved vertical slice: [PATH]
- Online requirements: [NONE OR DESCRIBE]

## Assignment

Inspect the project and design the smallest assembly/package and scene structure
that supports the approved slice. Separate deterministic game rules from
MonoBehaviour presentation, input, animation, physics, UI, saving, audio, assets,
and online services. Prefer ScriptableObjects for authored configuration, not
mutable global state. Define scene/bootstrap ownership, lifecycle, and additive
loading only where needed.

Choose Input System, Addressables, URP/HDRP, DOTS, Cinemachine, and third-party
packages only from demonstrated requirements. Do not introduce a service locator
or universal manager that owns unrelated systems.

## Deliverables

- Assembly-definition dependency rules and folder/namespace map.
- One runnable scene proving input, simulation, rendering, pause, and teardown.
- Prefab/configuration conventions and save-version strategy.
- EditMode, PlayMode, build, and representative device smoke tests.
- Setup, architecture, extension, and validation documentation.

## Gate

The project must open without missing references, build for one target, complete
the smoke path, and report untested packages/platforms honestly.
