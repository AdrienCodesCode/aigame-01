# Task: refine one Blender asset against an approved rubric

## Inputs

- `.blend` source and asset identifier: [PATH]
- Intended camera distance, animation, collision, and platform: [USE]
- Visual rubric and references: [PATHS]
- Triangle, material, texture, rig, and export budgets: [BUDGETS]

## Assignment

Inspect topology, silhouette, scale, pivots, transforms, UVs, materials, rig,
deformation, naming, and export settings. Improve the smallest high-impact issues
visible in representative gameplay. Preserve nondestructive source and documented
generation scripts. Keep collision proxies separate.

Do not add detail invisible at target distance, multiply materials without need,
or copy reference assets. Record source/license information for every input.

## Deliverables

- Before/after captures from identical camera and lighting.
- Source and exported asset with deterministic settings.
- Measured geometry/material/texture/animation changes.
- Import smoke test in the target engine and fallback/LOD behavior if required.
- Updated asset manifest and extension instructions.

## Gate

Accept only if the blind comparison improves the rubric without exceeding budgets
or breaking rig, scale, collision alignment, import, or animation.
