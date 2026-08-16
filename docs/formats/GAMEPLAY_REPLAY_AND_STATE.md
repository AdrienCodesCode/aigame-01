# Gameplay replay and state-dump contracts

**Status:** Version 1 seed/action/replay and version 2 dog-and-five-sheep state
dump implemented; the presentation capture CLI can write a state dump, while
JSON decoding and general replay/seed CLI integration remain pending

**Last revised:** 2026-08-16

The `game` boundary owns these contracts. They make a fixed-tick input sequence
and its observed state inspectable without giving file or renderer code control
of `GameplaySimulation`.

The authoritative implementation is
[`gameplay_replay.hpp`](../../src/game/gameplay_replay.hpp). The JSON writers and
validation rules are in
[`gameplay_replay.cpp`](../../src/game/gameplay_replay.cpp), and the focused
oracles are in
[`gameplay_simulation_tests.cpp`](../../tests/gameplay_simulation_tests.cpp).

## Compatibility rules

The contracts use four independently named versions:

- seed format `1` identifies a named scenario, that scenario's version, and a
  64-bit seed;
- action-input format `1` records one optional dog movement action per
  authoritative tick;
- replay format `1` binds the tick rate, seed contract, action-input version,
  and complete action sequence;
- state-dump format `2` records the seed contract, tick rate, restart count, and
  published previous/current dog-and-five-sheep snapshots. Version 1 was the
  earlier dog-only contract and is not silently reinterpreted.

A replay consumer must reject an unsupported version, a tick rate other than
60 Hz, an unknown scenario, a zero scenario version, a scenario/version/seed
mismatch, a non-contiguous tick sequence, or a non-finite or out-of-range action.
Validation completes before simulation mutation. Changing the meaning or
required fields of one contract requires incrementing its corresponding
version; a reader must not silently reinterpret a version it does not support.

The current encoder writes canonical compact JSON with a fixed key order and
round-trip-safe finite `double` values. This is useful for local comparisons and
future artifact hashing. It is not a claim that floating-point state is
byte-identical across compilers or platforms.

## Replay version 1

The schema name is `wide-eye.gameplay-replay`. Actions start at tick zero and
cover every consumed tick in order. `dog_move: null` advances authoritative
time while preserving the existing free-debug behavior that suspends the dog
motor. Otherwise `world_x` and `world_z` are finite normalized domain values in
`[-1, 1]`; `sprint` is boolean.

```json
{"schema":"wide-eye.gameplay-replay","version":1,"tick_rate":60,"action_input_version":1,"scenario":{"seed_format_version":1,"id":"paddock-start","version":1,"seed":0},"actions":[{"tick":0,"dog_move":{"world_x":0,"world_z":-1,"sprint":false}},{"tick":1,"dog_move":null}]}
```

`GameplayReplay` currently owns a contiguous action vector and
`apply_gameplay_replay` consumes that typed contract in memory. A general JSON
decoder, replay file path, checked-in replay fixture, and executable
`--replay`/`--seed` flags are intentionally deferred. No untrusted or external
JSON is accepted by the engine yet.

## State dump version 2

The schema name is `wide-eye.gameplay-state`. The dump contains the scenario
seed contract, fixed rate, restart count, and complete published previous and
current snapshots. Each snapshot contains the dog position, velocity, heading,
and grounded state plus exactly five sheep in stable ID order. Every sheep
record contains ID, position, velocity, heading, arousal, explicit behavior
state, and grounded state. Version 2 currently initializes IDs 1–5 in a fixed
contiguous buffer with `settled` behavior and zero arousal. The default scenarios
keep them stationary. The version 1, seed-zero `presentation-motion` fixture
moves them through a deterministic scripted path while leaving behavior settled
and arousal zero; this is presentation evidence, not a flock-response model.
No sheep respond to the dog yet. A non-finite state is rejected because JSON has
no portable representation for NaN or infinity.

The state dump is an observation, not a save format. It does not restore a
simulation, promise save compatibility, include renderer state, or establish
cross-platform determinism. Changing required state fields or their meaning
requires another explicit state-dump version decision with focused tests.

The bounded presentation fixture can write this canonical observation beside a
capture with `--state-dump <json-path>`. That output-only platform path does not
decode JSON, restore state, or add replay/seed ingestion.

## Observed evidence and limits

**Observed result (2026-08-16):** on WSL Ubuntu 24.04.4 with Clang 18.1.3, two
fresh simulations consumed the same three-tick typed dog-input replay and
produced equal dog-and-sheep snapshots plus byte-identical canonical version 2
state dumps. Focused oracles verified contiguous IDs 1–5, immutable-prior
publication, restart, interpolation, and zero allocations across 600 fixed
updates. They also rejected incompatible replay/action/seed versions, a 59 Hz
replay, non-contiguous ticks, a mismatched seed, non-finite input, application
after tick zero, and non-finite dumped dog state. A later focused extension
verified repeated exact `presentation-motion` state, immutable prior/current
publication, midpoint facing interpolation, exact restart, and settled
zero-arousal state for all five sheep. Development and ASan/UBSan suites each
passed 22/22 CTests; project formatting and bounded static analysis passed.

**Observed result (2026-08-16):** native Windows Release capture commands wrote
canonical version 2 state at presentation ticks 1, 61, and 121. The independent
normal, repeat-normal, and face-normal debug commands at tick 61 produced
byte-identical state files with SHA-256
`8b2921e4a87bc2e8b8b86e08f4f17d8b3a7bf7c9413293b90b03b728ec27a905`.
This is local same-platform evidence, not a cross-platform identity claim.

Native Linux graphics, JSON decoding, CLI replay/seed ingestion, persistent
replay fixtures, sheep behavior, objective outcomes, and cross-platform state
or text identity remain untested or unimplemented.
