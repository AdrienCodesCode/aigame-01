# Task: implement the approved media-delivery plan

## Inputs

- Approved audit and budgets: [PATH]
- Engine/platform-specific task: [PATH]
- Source and generated roots: [PATHS]

## Assignment

Implement deterministic conversion, validation, manifests, loading, caching,
pooling/voice limits, buses, spatialization, captions, localization variants,
pause/lifecycle behavior, and fallbacks described by the approved audit. Preserve
source masters and make generated output replaceable.

Integrate one representative music loop, feedback cue family, ambience/voice case,
and video case only where applicable before bulk conversion.

## Deliverables

- Versioned scripts/configuration and clean-build instructions.
- Runtime media service with explicit preload, lazy, stream, release, and failure
  behavior.
- Loop-seam, missing-file, blocked-autoplay, concurrency, lifecycle, memory, and
  target-device tests.
- Before/after package, startup, memory, decode, and playback evidence.
- Updated media manifest and extension/troubleshooting documentation.

## Gate

Reject savings that cause audible artifacts, lost feedback, caption drift,
unsupported codecs, excessive decode cost, or gameplay failure.
