# AI Prompts for Game Development

The right AI prompts save both tokens and development time. They give the AI enough context to work with the systems that already exist instead of repeatedly rediscovering the project, duplicating code, replacing deliberate decisions, or rebuilding the same feature later.

This README is both a start-to-finish workflow and a reference guide. If you are starting a new game, follow the steps in order. If your game is already underway, jump to the section that matches the problem you need to solve.

Run one prompt at a time, review the result, and commit working changes before moving forward. Every prompt tells the AI to read, create, or update the relevant game documentation so later prompts can reuse decisions without spending tokens rediscovering them.

- [Example Working Game](https://www.glitch.fun/games/9a698a9d-1b27-4c78-9256-0f458368737d/play)
- [Open Source Code](https://github.com/Glitch-Gaming-Platform/Glitch-Games-FarmRise-Tycoon)
- [Explainer Video](https://youtu.be/rW4IriCvpyQ)

## Example Game Took 2 Days To Make

The example took days to make, and it only playable with a core loop but the prompts enable desktop and mobile optimization, collision detection, sound affects, music loops, visual affects, onboarding, user progression, saving and loading, menu system, full testing test and extensive documentation. This allows a full prototype of the a game to start getting feedback.

<p align="center">
  <img src="assets/readme/example-game-after-1.png" alt="Polished example farm game showing improved terrain, crops, buildings, lighting, shadows, and game UI" width="49%">
  <img src="assets/readme/example-game-after-2.png" alt="Polished example farm game showing improved building presentation, terrain materials, lighting, shadows, and game UI" width="49%">
</p>

### Refinement and Bugs Disclaimer
To be clear on expectations, with this process your game will still have bugs. It will need refinement. But the speed and the amount of tokes required to make those refinements and bug fixes will be a lot less than without properly structurally your game.

Anything the game shows to a player should be human-readable and player-focused. Menus, objectives, errors, loading states, notifications, and recovery messages should use clear game language and explain the next action. Keep stack traces, JSON, internal IDs, event names, debug values, and other technical diagnostics in developer-only logs and tools.

## Phase 1: Build the game

Use this phase to define the game, create its technical and creative foundations, and produce the first complete playable build.

## 1. Define the game

Write down the player fantasy, goal, pressure, defining twist, and core gameplay loop. If you do not know the mechanics yet, use the optional generator.

[Open the optional mechanics and core-loop generator](https://www.glitch.fun/publishers/tools/ai-game-development-prompts#game-design-generator)

## 2. Create the project architecture

Choose the engine you are actually using. This creates the project structure, dependency rules, tests, and AI instructions before the codebase becomes difficult to change. It also audits the locomotion, animation, collision bodies, hitboxes, hurtboxes, traces, game menus, HUD, button states, input navigation, procedural motion, VFX movement, physics reactions, and internationalization/localization architecture the approved game will need. Player-facing text should use stable translation keys from the start so additional languages do not require rebuilding finished gameplay and menus.

- [Three.js architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=threejs-game-architecture#prompt-picker)
- [Unity architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unity-game-architecture#prompt-picker)
- [Godot architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=godot-game-architecture#prompt-picker)
- [Unreal Engine architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unreal-game-architecture#prompt-picker)

Set up remote automation while the architecture is still flexible. This gives AI agents and conventional test runners a secure game DOM, semantic actions, real input, screenshots, events, assertions, and CI access instead of making every future test depend on pixel guessing.

[Build a remote game automation tool](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=remote-game-automation#prompt-picker)

## 3. Plan an optional backend (web games REQUIRED)

Accounts, multiplayer, purchases, cloud saves, leaderboards, and persistent economies need trusted server-side rules. Offline games may skip this step.

- [Build a secure game backend](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=secure-game-backend#prompt-picker)
- [Create a reusable game SDK](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=game-backend-sdk#prompt-picker) — optional when multiple clients or tools use the backend

## 4. Establish the visual standard

Create a visual rubric before generating large amounts of art. Use approved AAA games with a comparable genre, camera, platform, and style as the quality benchmark, then score the visible gap in characters, environments, materials, lighting, effects, locomotion, animation, HUD, menus, icons, and buttons. Audit representative gameplay states—not only a beauty shot—including complete-frame composition, camera and character staging, state and resource clarity, selection and targeting, tactile cards or game pieces, action and VFX readability, world scale, environmental storytelling, and technical image quality. The interface should look like part of the game—not a website or default engine screen.

[Create a visual quality rubric](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=visual-quality-rubric#prompt-picker)

## 5. Create and integrate representative assets

Refine a small number of important assets first, then document an export and optimization pipeline that future assets can repeat. Keep simple gameplay collision separate from detailed visual meshes and animation, and create reusable UI assets for icons, panels, cards, input glyphs, fonts, and every button state.

- [Refine artwork in Blender](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=refine-blender-art#prompt-picker)
- [Build an optimized asset pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=optimized-asset-pipeline#prompt-picker)

## 6. Plan audio and video delivery

Do not load or compress every media file the same way. Audit the game first. The audit also proposes the sound-effect families and music loops the approved mechanics, movement, environments, UI, onboarding, progression, and session structure need. Then use the implementation prompt and the prompt for your engine.

- [Analyze the game media pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=audit-game-media-pipeline#prompt-picker)
- [Implement an optimized media pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=implement-game-media-pipeline#prompt-picker)
- Engine-specific: [Three.js](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=threejs-media-optimization#prompt-picker), [Unity](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unity-media-optimization#prompt-picker), [Godot](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=godot-media-optimization#prompt-picker), or [Unreal](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unreal-media-optimization#prompt-picker)

---

## 7. Add analytics before implementation

Define the stable event taxonomy before building the first playable version. Every important player journey, mechanic, menu, onboarding step, success, failure, performance problem, and exit should be trackable from the moment it is implemented. Keep event names language-independent, include the active locale only as privacy-safe context, and make analytics failure-safe so blocked providers never break the game.

[Set up production game analytics](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=production-game-analytics#prompt-picker)

---

# Step 8: Now Implement Your Game

Combine the approved mechanics, core loop, architecture, representative assets, collision and hit detection, audio, video, controls, game-native HUD and menus, complete button states, analytics, internationalization/localization, and feedback into the first playable build. Route player-facing text through the approved locale system, preserve stable language-independent IDs, and test pseudolocalization, text expansion, fonts, right-to-left layout, and representative real languages. Keep it small: one complete path with meaningful success and failure states is more useful than many unfinished systems.

> **This is where planning becomes a playable game.** Do not move into release work until the core mechanics and complete gameplay loop work together in a build someone else can play.

## [▶ Implement the first playable build](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=build-playable-vertical-slice#prompt-picker)

## Step 9: Design and Implement Player Onboarding

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

## 12. Run the final AAA visual optimization pass

Once the mobile and desktop behavior is stable, run the intensive final presentation pass across graphics, animation, locomotion, materials, lighting, VFX, physics presentation, game UI, technical image quality, and performance. The prompt creates Low and Ultra graphics profiles, uses separate implementation and harsh-review roles, and requires blind same-state before/after comparisons instead of accepting unsupported claims of AAA quality.

### Example before and after

The final visual optimization pass can systematically improve terrain and material detail, lighting, shadows, environmental density, building presentation, visual hierarchy, and the overall readability of the same game.

<table>
  <tr>
    <th width="50%">Before</th>
    <th width="50%">After the final visual optimization pass</th>
  </tr>
  <tr>
    <td><img src="assets/readme/example-game-before.jpg" alt="Example farm game before the final visual optimization pass"></td>
    <td><img src="assets/readme/example-game-after-2.png" alt="Example farm game after the final visual optimization pass"></td>
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
