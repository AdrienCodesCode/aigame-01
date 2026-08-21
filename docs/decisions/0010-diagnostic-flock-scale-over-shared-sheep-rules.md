# ADR 0010: Diagnostic flock scale over shared sheep rules

**Status:** Accepted
**Date:** 2026-08-22
**Decision owner:** Project owner

## Context

Two Phase 3 items ask for non-player diagnostic fixtures at 5, 14, 25, and 100
sheep, and for spatial-grid build, neighbour selection, behaviour, terrain query,
snapshot, allocation, and total simulation costs to be recorded separately. The
roadmap attaches an explicit constraint to the first one: it must happen
"without making large flocks a Tracer 2 *content* requirement".

Everything authoritative is shaped by the number five.
`kGameplaySheepCount` is `5`, `SheepStateBuffer` is `std::array<SheepState, 5>`,
each of the six published evidence buffers is a five-member array of the same
shape, `GameplaySnapshot` holds all seven of them by value,
`GameplayScenarioDefinition::initial_sheep` is one more, the state-dump writer
walks exactly five sheep, and the pure flock-observable pass takes the fixed
buffer. That five is not incidental: the accepted first-playable design is five
sheep and one gate, and the replay/state contract is versioned around it.

The rules that would actually be measured were, until this decision, private to
one translation unit. Every steering term, the dog stimulus, the behaviour
transition, the avoidance probe, the combined bound, the motion limits, and the
paddock resolve lived in the anonymous namespace of `gameplay_simulation.cpp`,
reachable only through `GameplaySimulation::fixed_update`. A rule only one caller
can reach can only be measured through that caller.

## Decision

### The authoritative contract does not grow a member

`kGameplaySheepCount`, `SheepStateBuffer`, every evidence buffer,
`GameplaySnapshot`, `GameplayScenarioDefinition`, the replay and state-dump
formats and their version numbers, and every named scenario are unchanged by
this outcome. No published field was added, removed, widened, or renamed, and
the canonical 240-tick state dumps of all 30 named scenarios are byte-identical
to the pre-change build.

Three alternatives were rejected for the same reason:

- **Template `GameplaySimulation` on the member count.** Every published type
  becomes a template, the state-dump writer and the observable pass follow, and
  the version-12 contract acquires a compile-time parameter — all to serve a
  measurement no player ever sees.
- **Make the published snapshot variable-length.** The snapshot stops being a
  fixed-size, allocation-free, comparable value. That is the property the
  determinism evidence, the replay contract, and the "no allocation on the
  authoritative fixed-tick path" invariant all stand on.
- **Raise `kGameplaySheepCount` to 100 and let scenarios use fewer.** This is
  precisely the "large flocks as a content requirement" the roadmap forbids: it
  multiplies every evidence buffer by twenty, grows `GameplaySnapshot` twentyfold
  in the shipping binary, and makes the first playable owe an audience ninety-five
  sheep nobody asked for.

### The rules become functions over prior state; the diagnostic drives them

`src/game/sheep_rules.hpp` now declares the accepted per-sheep rules and
`sheep_rules.cpp` defines them, moved verbatim out of `gameplay_simulation.cpp`.
The only signature change is that the three social terms take
`std::span<const SheepState>` where they took `const SheepStateBuffer&`, which a
five-member array converts to implicitly.

This is a boundary correction of the same kind as
[ADR 0004](0004-gameplay-scenario-ownership.md) and
[ADR 0005](0005-paddock-collision-ownership.md), and it is justified on its own
terms: these functions never read the state they write, never allocate, never
hold a clock, and depend on the member count only through an index. They were
already pure functions over prior state; they were just not reachable as such.

`GameplaySimulation` remains the **sole authoritative caller**. It still owns the
tick order, the five-member buffers, the published snapshots, the scenario
validation, and the replay contract. What it no longer owns is the private
monopoly on the arithmetic.

The diagnostic itself lives entirely in `tests/flock_scale_diagnostic.cpp` and is
never linked into `wide_eye`. It holds its own flock in caller-owned heap
storage, publishes no snapshot, has no replay contract, and nothing in the engine
can reach it. That placement is the concrete form of "not a content requirement":
the shipping library gained a header of declarations for rules it already had,
and gained no diagnostic code at all.

### The diagnostic sweeps by stage, and proves it may

The authoritative tick applies every rule to one sheep before moving to the next.
The diagnostic applies one rule to every sheep before moving to the next rule.
Both orders produce identical state, because each rule reads only the immutable
prior buffer and the prior dog and writes only its own member's records.

Sweeping by stage is what makes the cost decomposition affordable: six clock
reads per tick instead of six per sheep per tick, which at 100 members would have
added more measurement overhead than the stage being measured. The price is that
the equivalence has to be *proved* rather than assumed, so the diagnostic's
registered check is a five-member comparison against `GameplaySimulation` itself:
same scenario, same scripted dog, and every tick the diagnostic's sheep, dog, and
all six evidence records must equal the published snapshot exactly. If the two
ever diverge, the larger runs are measuring something other than the game and the
CTest says so by name.

### Neighbour selection is separated from the term that consumes it

The roadmap asks for neighbour-selection cost separately from behaviour cost, so
`select_sheep_neighbors` now runs the grid queries into caller-owned scratch and
`apply_sheep_separation`/`_attraction`/`_alignment` consume the result. The
arithmetic and its order are unchanged; what changed is that the query and the
response are two calls instead of one, which is what lets either caller time them
apart. The authoritative path calls them back to back.

### The diagnostic reports; it does not gate

`--validate-only` is the registered `performance`/`headless` CTest and asserts
only what is genuinely deterministic: the five-member reference equality, zero
heap allocation across the measured ticks, repeated-run equality of every
published record and every work counter, finite state inside the paddock bounds,
and that separation selects every neighbour within its radius rather than
truncating. `--benchmark` adds the host timings and is deliberately **not** a
CTest.

That split follows the precedent [ADR 0002](0002-chunk-edge-length.md) set for
the chunk-size comparison — "timing is reported without a machine-dependent pass
threshold. CTest instead requires exact cell counts" — and the reason is stated
rather than inherited: **no accepted performance budget exists above five
sheep.** A timing gate needs a budget on named hardware, and
this project has neither for 14, 25, or 100 members. Registering one would
manufacture an acceptance criterion out of a shared WSL development host.

## Consequences

- The accepted five-sheep game is bit-for-bit unchanged, and the regression
  evidence for that claim is per-scenario rather than aggregate.
- The rules are now reachable by name, which makes them measurable, and also
  makes them easier to misuse. `sheep_rules.hpp` says in its own comment that
  `GameplaySimulation` is the authoritative caller; nothing enforces it.
- `gameplay_simulation.cpp` is about 500 lines shorter and now reads as tick
  order rather than as rule arithmetic.
- The `dev` gameplay-simulation harness's stack requirement moved by one 5 KiB
  grid step (190 to 195 KiB) against the named 512 KiB budget; `release` and
  `dev-sanitized` did not move. The 100-member diagnostic needs 16 to 24 KiB,
  because every buffer it owns is on the heap.
- The diagnostic measures the *rules* rather than the whole shipping path. It
  does not exercise the platform scheduler, the render interpolation, the replay
  writer, the state-dump writer, or presentation, and its costs must not be read
  as a frame cost.
- The 1,000-member capacity ceiling in `SheepSpatialGrid` is now measured rather
  than merely inherited: its fixed-size arrays cost about 115 KiB and its rebuild
  zero-initializes a 1,000-entry ID array on every tick regardless of the flock
  size. Shrinking it remains its own outcome and its own decision; this one
  deliberately measures and does not tune.
