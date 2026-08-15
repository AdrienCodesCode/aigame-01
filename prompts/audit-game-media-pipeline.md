# Task: audit game audio and video before changing delivery

## Inputs

- Project/media roots: [PATHS]
- Platforms and session/loading constraints: [TARGETS]
- Approved mechanics, UI, onboarding, and accessibility needs: [PATHS]

## Assignment

Inventory every audio/video source and runtime reference. Classify music, ambience,
UI, voice, one-shot, loop, spatial, streamed, preload, and cinematic use. Measure
codec, duration, channels, sample rate, loudness, silence, loop seams, dimensions,
bitrate, decode cost, memory, download, concurrency, and missing fallbacks.

Also identify required cue families that do not yet exist. Prioritize information
and feedback before content volume.

## Deliverables

- Referenced/orphaned/missing media inventory with provenance.
- Platform risk table and preload/stream/lazy-load recommendations.
- Voice/concurrency, bus, ducking, caption, localization, and fallback plan.
- Baseline download, startup, memory, and representative playback measurements.
- Ordered implementation work with explicit non-goals.

## Gate

This is an audit. Do not rewrite files or install codecs until the plan and quality
trade-offs are approved.
