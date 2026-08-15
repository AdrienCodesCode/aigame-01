# Task: optimize media for a Three.js browser game

## Inputs

- Browser/device matrix: [TARGETS]
- Approved media audit: [PATH]
- Startup/download/memory/voice budgets: [BUDGETS]

## Assignment

Audit Web Audio unlock, AudioContext lifecycle, decoding, buffers, streaming HTML
media, spatial nodes, buses, voice stealing, visibility changes, iOS behavior,
caching/service workers, and video texture use. Define codec fallbacks from tested
browsers, not assumptions.

Load only critical feedback before play. Lazy-load music and long content. Pool
sources where it reduces churn and release buffers/video textures on transitions.

## Deliverables

- Browser capability/fallback matrix.
- Central media service and manifest changes for representative assets.
- Tests for gesture unlock, mute/pause, background/foreground, missing/blocked
  media, concurrency, disposal, and reduced-data behavior.
- Measured transfer, startup, memory, decode, and frame impact on target devices.

## Gate

Verify real Safari/iOS and Android Chrome when claimed. Desktop Chromium alone is
not browser-media compatibility evidence.
