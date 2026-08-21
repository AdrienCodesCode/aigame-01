# AI Prompts for Game Development

The right AI prompts save both tokens and development time. They give the AI enough context to work with the systems that already exist instead of repeatedly rediscovering the project, duplicating code, replacing deliberate decisions, or rebuilding the same feature later.

This README is both a start-to-finish workflow and a reference guide. If you are starting a new game, follow the steps in order. If your game is already underway, jump to the section that matches the problem you need to solve.

Run one prompt at a time, review the result, and commit working changes before moving forward. Every prompt tells the AI to read, create, or update the relevant game documentation so later prompts can reuse decisions without spending tokens rediscovering them.

- [Example Working Game](https://www.glitch.fun/games/9a698a9d-1b27-4c78-9256-0f458368737d/play)
- [Open Source Code](https://github.com/Glitch-Gaming-Platform/Glitch-Games-FarmRise-Tycoon)
- [Explainer Video](https://youtu.be/rW4IriCvpyQ)

## This fork's local workbench

This fork adds locally versioned material for evaluating and applying the linked
workflow without treating external marketing claims or mutable hosted prompts as
project facts.

- [Agent operating rules](AGENTS.md)
- [Accepted development workflow and standardized feedback loop](docs/DEVELOPMENT_WORKFLOW.md)
  — coherent outcomes, verification cadence, failure evidence, visual review,
  and chat/context boundaries
- [Implementation roadmap](ROADMAP.md) — checkbox milestones and the continuation
  checkpoint for future context windows
- [QA defect tracker](docs/qa/README.md) — how a reported defect is investigated,
  filed, verified, and retired, with the generated [issue index](docs/qa/INDEX.md)
  and the manual [sweep charters](docs/qa/charters/README.md)
- [Accepted native foundation](docs/decisions/0001-native-foundation.md) — C++
  primary track, Linux/Windows matrix, procedural-first asset rule, budgets, and
  dependency/license policy
- [Ubuntu 24.04 development setup](docs/setup/UBUNTU_24_04.md) — Phase 0 local
  toolchain and SDL3/OpenGL context diagnostic
- [Native Windows development setup](docs/setup/WINDOWS.md) — MSVC/CMake/Ninja
  installation and real-GPU OpenGL 4.6 context diagnostic
- [Local prompt library](prompts/README.md) — original adaptations covering all
  prompt topics linked below
- [Technology used by the example game](docs/TECH_STACK.md)
- [Custom C++ voxel-engine option](docs/VOXEL_ENGINE_OPTION.md)
- [Agent harness, MCP/skill review, and official technical references](docs/AGENT_HARNESS_AND_TOOLS.md)
- [Agentic workflow research](docs/research/agentic-development-workflow.md) and
  its [implementation plan](docs/plans/agentic-development-workflow.md)
- [OpenGL-to-Vulkan feasibility and AI-readable rendering
  workflow](docs/research/opengl-to-vulkan-feasibility.md)
  — evidence-backed migration timing, current hardware capability observations,
  and a bounded parity direction
- [Repository workflow skills](.agents/skills/) — deep research, planning from
  research, documentation sync, and end-of-session handoff
- [Glitch Analytics evaluation note](docs/ANALYTICS_NOTE.md)
- [Border-collie game concepts and recommended vertical slice](docs/game-design/WIDE_EYE.md)
- [Broader herding gameplay, reward, progression, scale, and animal direction](docs/game-design/HERDING_GAMEPLAY.md)
- [Sheep/dog behavior and flock-scale research](docs/research/herding-simulation-and-scale.md)
  with its bounded [implementation plan](docs/plans/herding-simulation-and-scale.md)
- [Gameplay replay and state-dump contracts](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md)
  — implemented version 1 seed/action/replay contracts plus the versioned state
  dump for the dog, five authoritative sheep, selected neighbors, separated
  social influences, and dog-stimulus evidence (the format contract owns the
  current version number); the presentation capture
  CLI can write state evidence, while decoding and replay/seed ingestion remain
  pending
- [Third-person dog controller and gameplay-camera research](docs/research/third-person-dog-controller-and-camera.md)
  and [implementation plan](docs/plans/third-person-dog-controller-and-camera.md)
  — the camera-relative keyboard/mouse baseline is implemented and owner-accepted;
  tuning and physical-controller review remain deferred
- [Formatted community production prompt](game-prompt.md) and its bounded
  [ultra production-pass adaptation](prompts/final-aaa-visual-optimization.md)

The local prompts are not verbatim mirrors of the hosted Glitch prompts. The
source playbook does not currently include a license, so these files preserve the
workflow ideas in new wording and add scope, evidence, privacy, and stop gates.

### Native engine quick start

The custom C++ engine is in Phase 3. `ROADMAP.md` is the authoritative progress
and next-action source; this overview keeps only the canonical commands and
entry points.

Configure, build, and run the fast development suite:

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

Use `dev-sanitized` for Clang ASan/UBSan coverage and `release` for optimized
measurement builds. Project-owned formatting and bounded static analysis run
through:

```bash
cmake --build --preset dev --target format-check
cmake --build --preset dev --target clang-tidy-check
```

Launch the interactive paddock on a machine that provides the required OpenGL
4.6 Core context:

```bash
./build/Linux/dev/wide_eye
./build/Linux/dev/wide_eye --play-scenario paddock-start
```

The version 1 scenarios are `paddock-start`, `presentation-motion`,
`sheep-only-separation`, `sheep-only-attraction`, `sheep-alignment-off`,
`sheep-alignment-on`, `sheep-dog-pressure-off`, `sheep-dog-pressure-on`,
`sheep-dog-approach-off`, `sheep-dog-approach-on`, `sheep-dog-facing-off`,
`sheep-dog-facing-on`, `sheep-dog-line-of-sight-off`,
`sheep-dog-line-of-sight-on`, `sheep-paddock-collision-closed-gate`,
`sheep-paddock-collision-open-gate`,
`wall-contact`, `closed-gate`, and `open-gate`. The
`presentation-motion` path is a scripted render fixture. The headless social
fixtures independently isolate close-range separation, bounded two-neighbor
attraction, a paired one-neighbor alignment control, a paired distance-only
dog-pressure control, and paired closing-speed dog-approach, dog-facing, and
analytic-occluder line-of-sight controls layered on that accepted pressure
geometry. The paired paddock-collision fixtures disable every steering term so
the analytic walls, gate, and paddock bounds are the only thing that can stop a
moving sheep.
Terrain and temperament pressure factors, combined-influence acceleration
bounds, behavior transitions, and the objective loop remain unimplemented.
Gameplay uses mouse orbit and
camera-relative WASD; Shift sprints, R restarts, and Tab switches the free-debug
camera. Escape releases the captured pointer so the desktop cursor returns
without leaving the window; pressing it again recaptures. Named gamepad actions
exist, but a physical controller remains unverified.

Bounded CPU/window, render, capture, and performance entry points are documented
with their ownership and current limitations in:

- [Source boundaries](src/README.md)
- [Test coverage](tests/README.md)
- [Native Windows setup and graphics runner](docs/setup/WINDOWS.md)
- [Gameplay replay/state contract](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md)
- [Human visual-review policy](docs/review/HUMAN_VISUAL_REVIEW.md)

The native Windows runners under `tools/phase1`, `tools/phase2`, and
`tools/phase3` build source-hashed copies, retain ignored evidence packets, and
do not promote a visual golden without an explicit owner verdict. The current WSL
host can run the headless suites but exposes only OpenGL 4.5, so it cannot replace
native Windows or native Linux OpenGL 4.6 evidence.

## External example claim — unverified here

The inherited playbook says its linked example was made in two days and includes
a broad set of gameplay, media, platform, analytics, and production features.
This repository does not contain or reproduce that implementation, schedule, or
feature evidence. Treat the statement as an external unverified claim and
evaluate the linked project directly before relying on it.

<p align="center">
  <img src="assets/readme/example-game-after-1.png" alt="Polished example farm game showing improved terrain, crops, buildings, lighting, shadows, and game UI" width="49%">
  <img src="assets/readme/example-game-after-2.png" alt="Polished example farm game showing improved building presentation, terrain materials, lighting, shadows, and game UI" width="49%">
</p>

### Refinement and Bugs Disclaimer

Expect bugs and iteration. The purpose of this method is to shorten the path to
useful evidence while preserving enough structure that later changes remain
reviewable. It does not guarantee speed, quality, or production readiness.

## Phase 1: Build the game

Use this phase to define the game, create its technical and creative foundations, and produce the first complete playable build.

## 1. Define the game

Write down the player fantasy, goal, pressure, defining twist, and core gameplay loop. If you do not know the mechanics yet, use the optional generator.

[Open the optional mechanics and core-loop generator](https://www.glitch.fun/publishers/tools/ai-game-development-prompts#game-design-generator)

## 2. Create the project architecture

Choose the engine you are actually using. This creates the project structure, dependency rules, tests, and AI instructions before the codebase becomes difficult to change. It also audits the locomotion, animation, collision bodies, hitboxes, hurtboxes, traces, game menus, HUD, button states, input navigation, procedural motion, VFX movement, physics reactions, and internationalization/localization architecture the approved game will need. Player-facing text should use stable translation keys from the start so additional languages do not require rebuilding finished gameplay and menus.

- [Three.js architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=threejs-game-architecture#prompt-picker) (beginners should choose this with a NextJS backend)
- [Unity architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unity-game-architecture#prompt-picker)
- [Godot architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=godot-game-architecture#prompt-picker)
- [Unreal Engine architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unreal-game-architecture#prompt-picker)

Add remote automation only when repeated testing has a concrete need that the
engine's existing test, input, accessibility, or capture hooks cannot meet.
Define its trust boundary and maintenance cost before introducing a custom
protocol.

[Evaluate a remote game automation tool](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=remote-game-automation#prompt-picker)

## 3. Decide whether a backend is needed

Accounts, multiplayer, purchases, cloud saves, leaderboards, and persistent
economies need trusted server-side rules. A local-only game can skip this step.

- [Build a secure game backend](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=secure-game-backend#prompt-picker) (beginners should use Node/NextJS)
- [Create a reusable game SDK](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=game-backend-sdk#prompt-picker) — optional when multiple clients or tools use the backend

## 4. Establish the visual standard

Create a visual rubric before generating large amounts of art. Use an approved,
comparable reference set for the genre, camera, platform, and style, then score
the visible gap with explicit criteria. Audit representative gameplay states,
not only a beauty shot. The interface should look like part of the game, not a
website or default engine screen.

- [Create a visual quality rubric](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=visual-quality-rubric#prompt-picker)
- [Build an optimized asset pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=optimized-asset-pipeline#prompt-picker)

## 5. Plan audio and video delivery

Do not load or compress every media file the same way. Audit the game first. The audit also proposes the sound-effect families and music loops the approved mechanics, movement, environments, UI, onboarding, progression, and session structure need. Then use the implementation prompt and the prompt for your engine.

- [Analyze the game media pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=audit-game-media-pipeline#prompt-picker)
- [Implement an optimized media pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=implement-game-media-pipeline#prompt-picker)
- Engine-specific: [Three.js](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=threejs-media-optimization#prompt-picker), [Unity](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unity-media-optimization#prompt-picker), [Godot](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=godot-media-optimization#prompt-picker), or [Unreal](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unreal-media-optimization#prompt-picker)

## 6. Create and integrate representative assets

Refine a small number of important assets first, then document an export and optimization pipeline that future assets can repeat. Keep simple gameplay collision separate from detailed visual meshes and animation, and create reusable UI assets for icons, panels, cards, input glyphs, fonts, and every button state.

- [Refine artwork in Blender](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=refine-blender-art#prompt-picker)


---

## 7. Define the minimum evidence before implementation

Define only the events needed to answer the current playtest question or detect
severe technical failures. Keep identifiers stable and language-independent,
minimize data, and make collection failure-safe. Do not select or install a
provider until the experiment, consent, retention, and privacy requirements
justify it.

[Evaluate production game analytics](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=production-game-analytics#prompt-picker)

---

## 8. Implement the game

Combine the approved mechanics, core loop, architecture, representative assets, collision and hit detection, audio, video, controls, game-native HUD and menus, complete button states, analytics, internationalization/localization, and feedback into the first playable build. Route player-facing text through the approved locale system, preserve stable language-independent IDs, and test pseudolocalization, text expansion, fonts, right-to-left layout, and representative real languages. Keep it small: one complete path with meaningful success and failure states is more useful than many unfinished systems.

> **This is where planning becomes a playable game.** Do not move into release work until the core mechanics and complete gameplay loop work together in a build someone else can play.

## [▶ Implement the first playable build](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=build-playable-vertical-slice#prompt-picker)

## 9. Design and implement player onboarding

Teach the real mechanics through play once the first playable build exists. Get players to a satisfying action quickly, introduce one concept at a time, provide an early win, use clear game-styled prompts and buttons, support skipping or adaptive guidance for experienced players, protect early progress, and instrument the first-session funnel.

> **Onboarding is part of the playable game, not a separate explanation screen.** Verify it with newcomers and every supported input method before moving into release work.

## [▶ Design and implement game onboarding](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=game-onboarding-flow#prompt-picker)

---

## Phase 2: Iterate and release the game

Once the first playable build exists, use evidence from real play to improve it, complete the approved scope, and prepare a safe production release.

## 10. Playtest and improve the evidence-backed problems

Test the slice with real players. Give the AI telemetry, surveys, reviews, recordings, and bug reports so it can rank improvements by evidence instead of opinion.

[Analyze playtest data](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=analyze-playtest-data#prompt-picker)

Repeat the vertical-slice and playtest steps until the core loop is clearly working.

## 11. Optimize and verify mobile builds

If the game targets phones or tablets, test the actual playable build on physical iOS and Android devices. Measure startup, frame pacing, memory, thermals, asset residency, touch controls, game-menu targets and button states, orientation, safe areas, lifecycle recovery, and real network conditions before reducing quality or changing systems. Keep mobile-specific changes isolated and rerun desktop visual, input, UI, loading, performance, networking, and save/load tests so mobile improvements do not damage the desktop experience.

[Optimize and test the game for mobile](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=mobile-game-optimization#prompt-picker)

## 12. Run the final presentation optimization pass

Once the mobile and desktop behavior is stable, run the intensive final
presentation pass across graphics, animation, locomotion, materials, lighting,
VFX, physics presentation, game UI, technical image quality, and performance.
Judge it against the approved rubric and same-state evidence; “AAA” alone is not
an acceptance criterion.

### Example before and after

The final visual optimization pass can systematically improve terrain and material detail, lighting, shadows, environmental density, building presentation, visual hierarchy, and the overall readability of the same game.

<table>
  <tr>
    <th width="50%">Before</th>
    <th width="50%">After the final visual optimization pass</th>
  </tr>
  <tr>
    <td><img src="assets/readme/example-game-before.jpg" alt="Example farm game before the final visual optimization pass"></td>
    <td><img src="assets/readme/example-game-after-1.png" alt="Example farm game after the final visual optimization pass"></td>
  </tr>
</table>

> **Intensive-token warning:** This prompt can run for a long time, fan work out across many sub-agents, repeat visual reviews, and consume a large number of AI tokens. Set token, time, compute, and human-review checkpoints before starting.

[Run the final AAA visual optimization pass](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=final-aaa-visual-optimization#prompt-picker)

## 13. Prepare a safe release process

Create reproducible builds, required test gates, environment separation, monitoring, backups, migrations, smoke tests, and rollback instructions before treating the game as production-ready.

[Create a safe deployment pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=game-deployment-pipeline#prompt-picker)

## 14. Complete the approved game

This is the final production prompt. It reads the design, architecture, collision plan, game UI style guide, assets, media plan, analytics, playtest findings, tests, and deployment documentation created above, then completes the approved scoped game without inventing a different one.

- [Free Web Hosting](https://www.glitch.fun/publishers/hosting)
- [Build the game from all approved plans](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=build-game-from-approved-plans#prompt-picker)
