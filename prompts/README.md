# Local prompt library

These are original, locally versioned adaptations of the workflow topics linked
from the root README. They are not verbatim copies of the hosted Glitch prompts.

The source repository currently has no explicit license, and the hosted prompts
can change without a commit here. This library therefore preserves the useful
task structure in new wording and adds proportional scope, evidence gates, and
privacy safeguards.

## How to use a prompt

1. Read the repository's `AGENTS.md`,
   [`DEVELOPMENT_WORKFLOW.md`](../docs/DEVELOPMENT_WORKFLOW.md), and applicable
   project documentation.
2. Choose one task file below.
3. Replace its bracketed inputs. Do not leave material decisions for the model to
   guess merely because a form permits blank values.
4. Append [`_shared-guardrails.md`](_shared-guardrails.md) to the task.
5. Run one task at a time; inspect its assumptions, diff, and evidence before the
   next task.
6. Save approved decisions and observed results in the game repository.

## Foundation

- [`cpp-voxel-game-engine.md`](cpp-voxel-game-engine.md) — local clean-room
  engine track built through playable tracers
- [`threejs-game-architecture.md`](threejs-game-architecture.md)
- [`unity-game-architecture.md`](unity-game-architecture.md)
- [`godot-game-architecture.md`](godot-game-architecture.md)
- [`unreal-game-architecture.md`](unreal-game-architecture.md)
- [`secure-game-backend.md`](secure-game-backend.md)
- [`game-backend-sdk.md`](game-backend-sdk.md)
- [`remote-game-automation.md`](remote-game-automation.md)
- [`production-game-analytics.md`](production-game-analytics.md)

## Visuals, assets, and media

- [`visual-quality-rubric.md`](visual-quality-rubric.md)
- [`refine-blender-art.md`](refine-blender-art.md)
- [`optimized-asset-pipeline.md`](optimized-asset-pipeline.md)
- [`audit-game-media-pipeline.md`](audit-game-media-pipeline.md)
- [`implement-game-media-pipeline.md`](implement-game-media-pipeline.md)
- [`threejs-media-optimization.md`](threejs-media-optimization.md)
- [`unity-media-optimization.md`](unity-media-optimization.md)
- [`godot-media-optimization.md`](godot-media-optimization.md)
- [`unreal-media-optimization.md`](unreal-media-optimization.md)

## Build, learn, and release

- [`build-playable-vertical-slice.md`](build-playable-vertical-slice.md)
- [`game-onboarding-flow.md`](game-onboarding-flow.md)
- [`analyze-playtest-data.md`](analyze-playtest-data.md)
- [`mobile-game-optimization.md`](mobile-game-optimization.md)
- [`final-aaa-visual-optimization.md`](final-aaa-visual-optimization.md) — the
  bounded ultra production-pass prompt
- [`game-deployment-pipeline.md`](game-deployment-pipeline.md)
- [`build-game-from-approved-plans.md`](build-game-from-approved-plans.md)

## Source map

The topic names and original sequence were inspired by the public
[Glitch AI game-development prompt library](https://www.glitch.fun/publishers/tools/ai-game-development-prompts),
reviewed on 2026-08-14. Each upstream-derived local filename matches the
corresponding `prompt=` query value in the root README so changes can be compared
deliberately. `cpp-voxel-game-engine.md` is a local extension.

No hosted prompt text should be copied into this library until its reuse license
is explicit. When upstream changes, review the idea and update the local version
with a normal commit rather than automatically synchronizing mutable text.
