# Task: optimize media for Godot target exports

## Inputs

- Godot version/renderer/platforms: [TARGETS]
- Approved media audit and budgets: [PATH/BUDGETS]

## Assignment

Review AudioStream imports, compressed versus sampled assets, stream players,
polyphony, buses/effects, spatial audio, priorities, resource loading/release,
VideoStream behavior, captions, localized variants, and platform import settings.
Centralize policy without creating one global node that owns unrelated gameplay.

## Deliverables

- Per-family import and bus/concurrency matrix.
- Representative loader/playback integration and fallback behavior.
- Tests for loops, missing resources, scene transitions, pause/lifecycle,
  concurrency, spatial range, captions, and video support.
- Export size, startup, memory, CPU, and target-device playback evidence.

## Gate

Verify exported builds on each claimed platform. State unsupported codecs and
platform-specific fallbacks explicitly.
