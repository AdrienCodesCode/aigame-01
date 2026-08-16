# Technology reference and selected project stack

## Wide Eye decision

As of 2026-08-15, **Wide Eye's primary implementation track is the clean-room
custom C++ voxel engine**. The selected foundation is C++23, CMake presets,
Ninja, SDL3, OpenGL 4.6 Core/GLSL 4.60, and project-owned focused C++ tests
orchestrated by CTest, targeting native x86-64 Linux and Windows. The initial
doctest choice was superseded before adoption by
[`ADR 0003`](decisions/0003-project-owned-test-harness.md).

The exact platform, asset, hardware-budget, dependency, and license rules are
accepted in [`ADR 0001`](decisions/0001-native-foundation.md). The Three.js
material below remains a verified reference and possible fallback experiment;
it is not a second active implementation milestone.

## Finding

The linked FarmRise Tycoon example is a browser game built without Unity, Godot,
or Unreal. Its client is a custom TypeScript game architecture rendered with
Three.js.

Source reviewed on 2026-08-14:
[Glitch-Games-FarmRise-Tycoon](https://github.com/Glitch-Gaming-Platform/Glitch-Games-FarmRise-Tycoon).

## Verified stack

| Area | Technology | Role |
| --- | --- | --- |
| Workspace | npm workspaces, Node.js 24 | Monorepo and scripts |
| Game client | TypeScript, Three.js, Vite | Browser runtime, rendering, bundling |
| Game UI | Native DOM and CSS | Menus and HUD over the WebGL canvas |
| Server | Next.js App Router | Authentication, saves, market, economy APIs |
| Persistence | SQLite, Drizzle ORM | Local/server data and migrations |
| Shared contract | TypeScript, Zod | Schemas and deterministic rules used by client and server |
| Authentication | JOSE/JWT plus server cookies | Account sessions |
| Simulation | Fixed 60 Hz tick, seeded randomness | Reproducible economy and movement |
| Spatial logic | Custom tile grid, swept circles, A* | Collision and pathfinding without a rigid-body engine |
| 3D assets | Blender, Python, GLB/glTF | Scripted asset construction and export |
| Textures/audio | Node and Blender tooling | Generated and verified media pipeline |
| Unit/integration tests | Vitest, jsdom | Rules, client systems, server routes |
| Browser tests | Playwright | Real rendering, input, desktop and mobile flows |

The client intentionally does not use React. React is present only because the
Next.js backend needs it. The game UI uses DOM elements so text, focus, and native
accessibility remain available without drawing the interface inside WebGL.

## What this means for a new herding prototype

Three.js is a sensible choice when the intended outcome is a quickly shareable
web game and the team is comfortable building some engine systems. It gives AI
agents ordinary TypeScript, browser debugging, fast Vite reloads, and Playwright
automation. It does not supply Unity-like editors, animation graphs, navigation,
physics, entity tooling, or asset import workflows automatically.

For the proposed border-collie prototype, start with:

- TypeScript, Three.js, and Vite.
- A small deterministic herding simulation at a fixed tick rate.
- Custom kinematic steering and simple circle/capsule collision. Sheep flocking
  is a behavior problem, not a rigid-body physics problem.
- A DOM HUD containing only the current farmer signal, flock status, and restart.
- GLB assets exported from Blender after primitive grey-box play proves the loop.
- Vitest for pressure/flock rules and Playwright for one complete browser route.
- An in-memory event recorder during development.

Do not add Next.js, accounts, cloud saves, a database, or a production analytics
provider to the first slice. They do not help answer whether indirect herding is
legible and satisfying. Introduce a backend only after a feature such as shared
trials, leaderboards, cloud careers, or authoritative competition earns it.

## Alternative engine choices

- **Godot** is the strongest alternative for a small team that wants a visual
  editor, built-in navigation/animation tools, and desktop/mobile exports.
- **Unity** offers the largest ecosystem and mature character tooling, but adds
  editor, package, licensing, and build complexity.
- **Unreal** is best justified by high-end presentation or large 3D production
  needs; it is excessive for validating this small systemic loop.

The engine decision should follow the target platform and team workflow. The
herding model itself should remain engine-independent enough to unit test.

## Selected custom native voxel track

Building an engine is now an explicit creative and technical goal. The
clean-room C++/OpenGL implementation advances through small playable tracers
rather than a showcase-renderer checklist. The deterministic herding model stays
independent of presentation so it remains testable without a GPU and portable if
a later experiment needs another frontend.

See the [custom C++ voxel-engine option](VOXEL_ENGINE_OPTION.md) for the proposed
stack, architecture, art direction, risk analysis, and staged build gates.
