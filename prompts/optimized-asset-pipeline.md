# Task: build a repeatable asset path from source to runtime

## Inputs

- Engine/platforms and current asset inventory: [CONTEXT]
- Source tools/formats: [TOOLS]
- Representative assets and budgets: [ASSETS/BUDGETS]

## Assignment

Audit asset origin, licenses, dimensions, geometry, materials, textures, audio,
animation, naming, duplicates, loading, memory, and runtime use. Design one
repeatable pipeline for source, validation, conversion, optimization, manifesting,
loading, versioning, and rollback. Prefer measured, reversible transformations.

Do not compress or atlas everything uniformly. Preserve high-quality sources and
choose variants from target-device evidence.

## Deliverables

- Source/runtime directory contract and provenance manifest.
- Deterministic commands with pinned tool versions.
- Budgets and automated checks for size, format, dimensions, geometry, materials,
  glyphs, animation, and missing references.
- Representative before/after size, load, memory, and visible-quality results.
- Failure recovery and instructions for adding one asset family.

## Gate

Run clean rebuild and engine import on representative assets. Generated output
must be reproducible without overwriting irreplaceable source files.
