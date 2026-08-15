# Task: optimize media for Unity target builds

## Inputs

- Unity version/platforms: [TARGETS]
- Approved media audit and budgets: [PATH/BUDGETS]
- Addressables/asset-bundle strategy: [CURRENT STRATEGY]

## Assignment

Review AudioClip load type, compression format/quality, sample rate, channels,
preload, decompression memory, streaming CPU, AudioSource pooling, mixer groups,
concurrency, spatial settings, VideoPlayer, Addressables, localization, captions,
and platform overrides. Make per-family decisions instead of applying one import
preset globally.

## Deliverables

- Import-rule matrix by media family and platform.
- Representative implementation with mixer, voice priority, lifecycle, and
  failure fallbacks.
- Automated import checks plus device playback/loop/video tests.
- Build size, runtime memory, CPU, latency, and audible/visible comparison.

## Gate

Verify player builds on target hardware. Editor profiler results alone do not
prove mobile or console decoding and streaming behavior.
