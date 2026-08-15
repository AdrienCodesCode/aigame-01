# Task: verify and improve the game on real mobile devices

## Inputs

- Supported devices/OS/browser or store builds: [MATRIX]
- Critical routes and inputs: [ROUTES]
- Frame, memory, startup, thermal, network, and size budgets: [BUDGETS]
- Desktop baseline that must not regress: [BASELINE]

## Assignment

Measure first. Test installation/launch, safe areas, orientation, touch targets,
gesture conflicts, virtual controls, text/input, interruption, background/resume,
audio unlock, offline/poor network, asset residency, memory pressure, thermals,
battery, frame pacing, and recovery on representative physical devices.

Isolate capability-based quality changes and preserve gameplay information across
profiles. Avoid user-agent-only detection when feature/capability checks exist.

## Deliverables

- Device/result matrix with exact hardware/build.
- Ranked bottlenecks and smallest measured fixes.
- Mobile interaction and lifecycle tests plus desktop regression tests.
- Before/after traces, captures, and budgets.
- Known unsupported devices/features and fallback behavior.

## Gate

Do not claim “mobile optimized” from responsive CSS, emulation, or one screenshot.
Run sustained physical-device play on at least the minimum supported class.
