# AI Prompts for Game Development

AI can generate a game from one prompt, but that usually creates a project that is difficult to maintain, test, optimize, and finish. A better approach is to make the important decisions in order, save them in the game's documentation, and then ask AI to build from those approved plans.

Use the steps below in order. Run one prompt at a time, review the result, and commit working changes before moving forward. Every prompt tells the AI to create or update the relevant game documentation.

## 1. Define the game

Write down the player fantasy, goal, pressure, defining twist, and core gameplay loop. If you do not know the mechanics yet, use the optional generator.

[Open the optional mechanics and core-loop generator](https://www.glitch.fun/publishers/tools/ai-game-development-prompts#game-design-generator)

## 2. Create the project architecture

Choose the engine you are actually using. This creates the project structure, dependency rules, tests, and AI instructions before the codebase becomes difficult to change.

- [Three.js architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=threejs-game-architecture#prompt-picker)
- [Unity architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unity-game-architecture#prompt-picker)
- [Godot architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=godot-game-architecture#prompt-picker)
- [Unreal Engine architecture](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unreal-game-architecture#prompt-picker)

## 3. Plan the backend only if the game needs one

Accounts, multiplayer, purchases, cloud saves, leaderboards, and persistent economies need trusted server-side rules. Offline games may skip this step.

- [Build a secure game backend](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=secure-game-backend#prompt-picker)
- [Create a reusable game SDK](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=game-backend-sdk#prompt-picker) — optional when multiple clients or tools use the backend

## 4. Establish the visual standard

Create a visual rubric before generating large amounts of art. Use it to keep characters, environments, materials, lighting, effects, and animation consistent.

[Create a visual quality rubric](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=visual-quality-rubric#prompt-picker)

## 5. Create and integrate representative assets

Refine a small number of important assets first, then document an export and optimization pipeline that future assets can repeat.

- [Refine artwork in Blender](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=refine-blender-art#prompt-picker)
- [Build an optimized asset pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=optimized-asset-pipeline#prompt-picker)

## 6. Plan audio and video delivery

Do not load or compress every media file the same way. Audit the game first, then use the implementation prompt and the prompt for your engine.

- [Analyze the game media pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=audit-game-media-pipeline#prompt-picker)
- [Implement an optimized media pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=implement-game-media-pipeline#prompt-picker)
- Engine-specific: [Three.js](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=threejs-media-optimization#prompt-picker), [Unity](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unity-media-optimization#prompt-picker), [Godot](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=godot-media-optimization#prompt-picker), or [Unreal](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=unreal-media-optimization#prompt-picker)

## 7. Add analytics before testing

Define stable events before the first serious playtest so you can see where players stop, fail, repeat actions, or misunderstand the game.

[Set up production game analytics](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=production-game-analytics#prompt-picker)

## 8. Build the smallest playable vertical slice

Prove the complete core loop with one representative path, meaningful success and failure states, final-quality feedback for the defining mechanic, and enough instrumentation to answer the core playtest question.

[Build a playable vertical slice](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=build-playable-vertical-slice#prompt-picker)

## 9. Playtest and improve the evidence-backed problems

Test the slice with real players. Give the AI telemetry, surveys, reviews, recordings, and bug reports so it can rank improvements by evidence instead of opinion.

[Analyze playtest data](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=analyze-playtest-data#prompt-picker)

Repeat the vertical-slice and playtest steps until the core loop is clearly working.

## 10. Prepare a safe release process

Create reproducible builds, required test gates, environment separation, monitoring, backups, migrations, smoke tests, and rollback instructions before treating the game as production-ready.

[Create a safe deployment pipeline](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=game-deployment-pipeline#prompt-picker)

## 11. Build the complete approved game

This is the final prompt. It reads the design, architecture, assets, media plan, analytics, playtest findings, tests, and deployment documentation created above, then implements the complete scoped game without inventing a different one.

[Build the game from all approved plans](https://www.glitch.fun/publishers/tools/ai-game-development-prompts?prompt=build-game-from-approved-plans#prompt-picker)
