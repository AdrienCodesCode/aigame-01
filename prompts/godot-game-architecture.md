# Task: establish a proportional Godot game foundation

## Inputs

- Godot version and renderer: [VERSION]
- GDScript or C#: [LANGUAGE]
- Target platforms/inputs: [TARGETS]
- Approved vertical slice: [PATH]

## Assignment

Audit existing scenes, autoloads, resources, signals, input mappings, physics, UI,
and tests. Design a small scene/resource composition that keeps pure game rules
separate from Nodes and rendering. Define which state belongs to a scene, resource,
service, save, or trusted backend. Limit autoloads to true application-lifetime
services and avoid deep implicit signal webs.

Select CharacterBody, NavigationServer, AnimationTree, resources, and threaded
loading only when the slice requires them. Keep collision shapes simpler than
visual meshes and give actions stable names across keyboard, controller, and
touch where supported.

## Deliverables

- Scene tree, folder/namespace map, and dependency rules.
- Minimal runnable slice bootstrap with clean scene transitions.
- Versioned save/resource conventions and import policy.
- Unit/integration tests plus one exported-build smoke path.
- Setup, architecture, and adding-a-feature documentation.

## Gate

Verify the editor project and an exported target. Do not claim export, input, or
performance support from editor-only execution.
