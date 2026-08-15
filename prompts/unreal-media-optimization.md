# Task: optimize media for Unreal target builds

## Inputs

- Unreal version/platforms: [TARGETS]
- Approved media audit and budgets: [PATH/BUDGETS]
- Packaging/chunk strategy: [STRATEGY]

## Assignment

Review SoundWave loading/streaming, compression quality, sample rate, MetaSounds,
SoundClasses/Submixes, attenuation, concurrency and priority, voice virtualization,
Media Framework, localization, captions, cook rules, chunks, and platform
overrides. Tune by media family and measured runtime behavior.

## Deliverables

- Asset/import/cook matrix for each platform.
- Representative concurrency, submix, spatial, lifecycle, and media-player setup.
- Automation plus packaged-build playback, loop, missing-media, transition, and
  memory tests.
- Cook size, startup, memory, CPU, voice, and media-thread measurements.

## Gate

Editor playback is not acceptance. Verify packaged target builds and record every
platform codec or streaming limitation.
