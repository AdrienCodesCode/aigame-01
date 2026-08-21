# Wide Eye C++ voxel-engine roadmap

## Current checkpoint

- **Current milestone:** Phase 3 — Tracer 2: five sheep and one gate. Phases 0–2
  and their exit gates are complete. Phase 3 has not yet implemented the
  playable herding loop.
- **Verified completed state:**
  - the accepted native C++23/SDL3/OpenGL foundation, bounded voxel paddock,
    and fixed 60 Hz gameplay;
  - versioned seed/action/replay contracts and the version 7
    dog-plus-five-sheep/social-and-dog-stimulus-evidence state output;
  - five contiguous authoritative sheep, snapshot-driven procedural proxies,
    and an owner-accepted presentation/measurement packet;
  - fixed five-sheep observables and a deterministic allocation-free uniform
    spatial grid;
  - synchronous, acceleration-bounded close-range sheep separation with
    deterministic exact-overlap recovery;
  - bounded two-neighbor attraction with exact chosen-neighbor evidence;
  - provisionally retained, independently switchable one-neighbor alignment
    with paired control evidence;
  - a paired, independently switchable distance-only dog-pressure term with
    prior-state distance/bearing evidence and linear radius falloff;
  - a paired, independently switchable dog-approach term whose prior-state
    closing speed responds only to a closing dog and saturates at a scenario
    reference speed; and
  - a paired, independently switchable dog-facing term whose prior-state
    heading cosine responds only to a dog looking toward the sheep.

  Detailed evidence remains with the checked Phase 3 items and their owning
  source, decision, format, test, and artifact records; completed Phase 0–2
  records are archived verbatim in
  [`ROADMAP_ARCHIVE.md`](ROADMAP_ARCHIVE.md).
- **Architecture correction (observed result, 2026-08-16):** review remediation
  moved `Vec3` into shared game math and moved deterministic scenario
  ID/version/seed, dog configuration, and sheep fixture ownership into
  `GameplayScenarioDefinition`. `DogController` now accepts only motor and
  analytic-collision configuration. That correction left the then-existing
  serialized scenario names and replay/state format versions unchanged.
  Evidence:
  [ADR 0004](docs/decisions/0004-gameplay-scenario-ownership.md),
  [`gameplay_scenario.hpp`](src/game/gameplay_scenario.hpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- **Correctness correction (observed result, 2026-08-16):** a clipped dog
  displacement clears the blocked velocity axis on the first contact tick;
  flock-observable validation now rejects non-finite heading/arousal and unknown
  behavior state; focused regression tests cover both contracts.
- **Evidence correction (observed result, 2026-08-16):** performance budgets now
  have one typed source in `core`. Tracer 1 uses the general Low 1 GiB RSS cap,
  Tracer 2 uses its tighter 512 MiB cap, both keep the accepted Low frame limits,
  and a failed budget returns process failure. Release CTests require the exact
  `within_provisional_low_budget=yes` marker. The Phase 3 packet hashes CMake
  presets/modules and third-party build inputs in addition to source and tests.
  Existing native measurements were not rerun by this headless remediation.
- **Test-stack correction:** doctest was never adopted. Project-owned focused
  executables plus CTest remain the accepted Tracer 2 harness under
  [ADR 0003](docs/decisions/0003-project-owned-test-harness.md); framework
  adoption is deferred until a concrete maintenance cost justifies it.
- **Verification run (2026-08-17, dog facing):** on WSL Ubuntu 24.04.4 with
  Clang 18.1.3, development, Release, and ASan/UBSan configurations each built
  and passed 24/24 CTests; project formatting and bounded clang-tidy passed. The
  gameplay-simulation oracle observed the paired facing fixture publish exact
  `1.0` alignment with the full `(0, -1)` acceleration straight ahead, `0` abeam
  with distance pressure only, `-1.0` behind with no facing vector, the exact
  `0.8` diagonal cosine with its `(0.12, -0.16)` vector, and `1.0` alignment
  without influence outside the 6-unit radius. A half-turn of the same dog
  swapped the front and back results exactly. The off case reproduced identical
  distance, bearing, approach, and alignment evidence and byte-identical
  distance-only pressure with a zero facing vector; the published sum matched
  applied acceleration; the same tick's dog-motor turn left prior-state facing
  evidence unchanged; exact overlap invented no direction; reversed storage,
  restart, and 600 zero-allocation ticks held. Evidence:
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp) and the
  ignored
  [oracle output](artifacts/phase3/2026-08-17/dog-facing-headless/gameplay-simulation-oracle.txt).
- **Prior verification run (2026-08-17):** on WSL Ubuntu 24.04.4 with Clang
  18.1.3,
  development, Release, and ASan/UBSan configurations each built and passed
  24/24 CTests.
  Project formatting and bounded clang-tidy passed. The gameplay-simulation
  oracle observed the separation regressions, the four-candidate/two-selected
  attraction case, and a 60-tick alignment pair whose polarization was
  `0.824621` off versus `0.924042` on. It also matched exact selected IDs and
  applied vectors, preserved exact per-ID state/evidence after reversing
  storage, restored coherent state on restart, and observed zero allocations
  across separate 600-tick separation, attraction, alignment, dog-pressure, and
  dog-approach runs. The distance-only dog pair published identical prior-state
  geometry off/on, matched exact falloff/direction to applied acceleration, and
  rejected pressure at and beyond its radius without inventing a direction for
  exact dog/sheep overlap. The dog-approach pair reproduced that distance-only
  pressure unchanged, published the same prior-state approach speeds off/on,
  saturated a head-on `4.0` closing speed at `1.333333` acceleration, measured
  exactly `0` abeam and `-4.0` behind the dog with no approach vector, and
  matched the exact `2.4` diagonal projection.
- **Review remediation (observed result, 2026-08-21):** a code-and-docs review
  of the uncommitted Tracer 2 work corrected stale state-dump version claims in
  `README.md`, `src/README.md`, and `docs/AGENT_HARNESS_AND_TOOLS.md` — prose
  now defers to the format contract instead of restating its number — and
  labeled `ref/longterm.md` as unverified external ideation. In code, the
  social-grid rebuild moved out of its assertion expression, enabled-term
  configuration validation moved from every fixed tick to simulation
  construction, the sheep behavior pass split into named per-term functions
  (`evaluate_dog_stimulus`, `apply_separation`, `apply_attraction`,
  `apply_alignment`) with unchanged arithmetic and evidence, and the Escape
  capture toggle now discards only stale mouse deltas via a new
  `clear_mouse_look` so a same-frame press such as restart survives, with
  focused input coverage. Completed Phase 0–2 checklists were archived
  verbatim to `ROADMAP_ARCHIVE.md`. On WSL Ubuntu 24.04.4 with Clang 18.1.3,
  development, Release, and ASan/UBSan configurations each built and passed
  24/24 CTests after these changes; project formatting and bounded clang-tidy
  passed. Native graphics and measurements were not rerun because no
  presentation path changed.
- **Known limits:** native Windows/OpenGL capture, the presentation performance
  scenarios, and human visual review were not rerun because this outcome changes
  authoritative headless sheep behavior rather than pixels. Native Linux
  graphics, the named Iris Xe target, and a physical controller remain
  unverified. Damping, combined-influence acceleration bounds, bounded
  speed/turning, dog line-of-sight/terrain/temperament pressure factors,
  behavior transitions, objectives, success/failure, HUD, and fresh-player
  evidence remain unimplemented. The distance, approach, and facing dog terms
  are currently summed without a combined bound.
- **Next action:** add dog line of sight as the next isolated dog-stimulus
  variable with a paired fixture and explicit prior-state evidence, using the
  existing analytic paddock obstacles as occluders. As part of the same
  outcome, move the analytic obstacle shapes out of `dog_controller.hpp` into a
  neutral game-owned header so sheep rules do not depend on a dog-named
  boundary; dog collision behavior itself must not change. Preserve the
  accepted distance-only, approach, and facing controls and the existing social
  fixtures; do not add terrain, temperament, combined-influence bounds,
  behavior transitions, or presentation in the same coherent outcome.
- **Next-context files:** [`AGENTS.md`](AGENTS.md), this checkpoint, the
  [development workflow](docs/DEVELOPMENT_WORKFLOW.md),
  [engine boundary](docs/VOXEL_ENGINE_OPTION.md#architecture-boundary),
  [first-playable design](docs/game-design/WIDE_EYE.md),
  [herding plan](docs/plans/herding-simulation-and-scale.md),
  [ADR 0004](docs/decisions/0004-gameplay-scenario-ownership.md),
  [`gameplay_scenario.hpp`](src/game/gameplay_scenario.hpp),
  [`dog_controller.hpp`](src/game/dog_controller.hpp),
  [`gameplay_simulation.hpp`](src/game/gameplay_simulation.hpp),
  [`gameplay_replay.hpp`](src/game/gameplay_replay.hpp),
  [`flock_observables.hpp`](src/game/flock_observables.hpp),
  [`sheep_state.hpp`](src/game/sheep_state.hpp),
  [`sheep_spatial_grid.hpp`](src/game/sheep_spatial_grid.hpp), and their
  focused tests.
- **Last reviewed:** 2026-08-21.
- **Primary playtest question:** Can a first-time player intentionally steer five
  mixed-temperament sheep through one gate using only the dog's movement,
  facing, pressure, and release?

## How future context windows use this file

1. Read [`AGENTS.md`](AGENTS.md), this checkpoint, the current phase, and the
   accepted [`development workflow`](docs/DEVELOPMENT_WORKFLOW.md).
2. Inspect `git status` and preserve unrelated user work.
3. Verify the evidence behind checked items; a checkbox is not proof by itself.
4. Work on the first unblocked unchecked item in the current phase unless the
   user explicitly reprioritizes.
5. Do not start a later phase while the current exit gate is failing.
6. After work, update checkboxes, evidence links, decisions, measurements, and
   the current checkpoint in the same change.
7. Never check an item for a planned, mocked, or unrun result.

Supporting references:

- [Game design](docs/game-design/WIDE_EYE.md)
- [Broader herding gameplay direction](docs/game-design/HERDING_GAMEPLAY.md)
- [Herding simulation and scale research](docs/research/herding-simulation-and-scale.md)
- [Herding simulation and scale implementation plan](docs/plans/herding-simulation-and-scale.md)
- [Accepted native foundation decision](docs/decisions/0001-native-foundation.md)
- [C++ voxel-engine decision](docs/VOXEL_ENGINE_OPTION.md)
- [Agent harness and tooling](docs/AGENT_HARNESS_AND_TOOLS.md)
- [Development workflow and standardized feedback loop](docs/DEVELOPMENT_WORKFLOW.md)
- [Workflow implementation plan](docs/plans/agentic-development-workflow.md)
- [Human visual-review packet](docs/review/HUMAN_VISUAL_REVIEW.md)
- [C++ engine implementation prompt](prompts/cpp-voxel-game-engine.md)

## Phase 0 — Product constraints and toolchain

Complete. All research, decision, workflow-foundation, and environment items
passed with their observed results. The full checklist and evidence record is
archived verbatim in
[`ROADMAP_ARCHIVE.md`](ROADMAP_ARCHIVE.md#phase-0--product-constraints-and-toolchain).

### Phase 0 exit gate

Passed in full: the material product/platform/dependency decisions were
approved, the recorded toolchain passed the OpenGL 4.6 Core smoke on native
Windows, and the project proceeds without a required MCP server. Evidence: the
archived [Phase 0 record](ROADMAP_ARCHIVE.md#phase-0-exit-gate).

## Phase 1 — Tracer 0: native foundation

Complete. The repository/build scaffold, executable smoke tracer, OpenGL 4.6
debug context, deterministic PNG capture, artifact manifests, owner-accepted
Tracer 0 visual baseline, and minimal Linux CI all passed with recorded
evidence. The full checklist is archived verbatim in
[`ROADMAP_ARCHIVE.md`](ROADMAP_ARCHIVE.md#phase-1--tracer-0-native-foundation).

### Phase 1 exit gate

Passed in full: clean-tree development and sanitized builds/tests, clean native
render/capture/shutdown, a documented independent reproduction, the explicit
owner Accept for the Tracer 0 visual packet, and the hosted Linux presubmit
with controlled failure evidence. Evidence: the archived
[Phase 1 record](ROADMAP_ARCHIVE.md#phase-1-exit-gate).

## Phase 2 — Tracer 1: bounded voxel paddock

Complete. The lifecycle/renderer ownership splits, pinned checksum-verified
glad loader, signed voxel coordinates, the 16-edge chunk decision (ADR 0002),
the bounded naive mesher with pass separation, the owner-accepted handcrafted
paddock baseline with lighting/shadow/debug views and measured captures, the
kinematic dog with analytic collision, gameplay/free-debug cameras, named
input with the pointer-capture follow-up, and deterministic restartable
scenarios all passed with recorded evidence. The full checklist is archived
verbatim in
[`ROADMAP_ARCHIVE.md`](ROADMAP_ARCHIVE.md#phase-2--tracer-1-bounded-voxel-paddock).

### Phase 2 exit gate

Passed in full: readable reproducible paddock captures, no dog tunneling
through the wall/gate tests, a complete chunk-face diagnostic ledger, and an
explicit owner Accept for the Tracer 1 review packet with its recorded limits;
procedural terrain, streaming, LOD, and advanced post-processing stayed out of
scope. Evidence: the archived
[Phase 2 record](ROADMAP_ARCHIVE.md#phase-2-exit-gate).

## Phase 3 — Tracer 2: five sheep and one gate

### Deterministic simulation harness

- [x] Run authoritative gameplay at a fixed 60 Hz tick independent of rendering.
  Observed result: `GameplaySimulation` owns the authoritative tick, existing dog
  controller, and read-only previous/current snapshots. Its update API accepts
  one domain input without a caller-supplied delta; `FixedStepAccumulator`
  remains the sole render-cadence scheduler. The interactive and three headless
  dog scenarios use this owner. A focused test drove the same tick-indexed input
  through 100×10 ms and 10×100 ms render partitions and produced exactly 60
  ticks with identical final state, while interpolation left authoritative state
  unchanged. WSL development and ASan/UBSan suites each passed 21/21 CTests;
  format and bounded static analysis passed. Native Windows was not rerun, so
  this does not establish cross-platform replay identity. Evidence:
  [`gameplay_simulation.hpp`](src/game/gameplay_simulation.hpp),
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp), and
  [`scenario_runner.cpp`](src/platform/scenario_runner.cpp).
- [x] Define versioned seed, action-input, replay, and state-dump formats.
  Observed result: independent version 1 seed/action/replay contracts bind the
  existing named scenario/version/seed, fixed 60 Hz rate, and complete
  tick-indexed domain actions. The dog-only state dump began at version 1 and
  advanced explicitly to version 2 when five required sheep records were
  added, to version 3 when attraction-neighbor social evidence became required,
  and to version 4 when alignment selection and influence fields became
  required, then to version 5 when dog distance, signed relative bearing,
  stimulus evaluation, and a separate pressure vector became required.
  Validation rejects unsupported replay versions, rate or scenario
  mismatches, gaps, non-finite values, and out-of-range normalized movement
  before mutation. Canonical compact JSON writers expose the replay plus
  previous/current published state and reject non-finite state. The presentation
  capture CLI can now write state evidence; JSON decoding plus replay/seed input
  paths remain deferred. Evidence:
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md),
  [`gameplay_replay.hpp`](src/game/gameplay_replay.hpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- [x] Establish a versioned game-owned catalog for deterministic scenarios.
  Observed result: the catalog currently owns the base paddock, presentation,
  wall-contact, closed/open-gate, sheep-only separation/attraction, and paired
  alignment-off/on plus distance-only dog-pressure-off/on starting contracts
  above controller-level configuration. Later behavior fixtures remain
  unchecked with the rules they must prove; their absence is not hidden by this
  foundation item.
  Evidence: [`ADR 0004`](docs/decisions/0004-gameplay-scenario-ownership.md),
  [`gameplay_scenario.hpp`](src/game/gameplay_scenario.hpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- [x] Verify the same replay produces the same outcome across repeated local
  runs; record any cross-platform determinism limit honestly. Observed result:
  two fresh simulations consumed the same three-tick typed dog-only replay and
  produced equal authoritative snapshots plus byte-identical canonical state
  dumps on WSL Ubuntu 24.04.4 with Clang 18.1.3. Development and ASan/UBSan
  suites each passed 21/21 CTests. Native Windows and cross-platform identity
  were not tested; this replay fixture does not exercise the later sheep-only
  social scenarios or an objective. Evidence:
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).

### Representative sheep and render envelope

- [x] Store the minimal hot sheep state contiguously with stable IDs, explicit
  behavior state, synchronous updates from an immutable prior snapshot, and no
  steady-state per-agent allocation. Observed result: one fixed five-element
  buffer owns IDs 1–5 plus kinematic, arousal, behavior, and grounded fields;
  prior/current publication and restart are coherent, interpolation is
  read-only, and an allocation-counted 600-tick loop observed zero heap
  allocations. The required state dump advanced from dog-only version 1 to
  dog-and-sheep version 2. WSL development and ASan/UBSan suites passed 21/21
  CTests; format and bounded static analysis passed. Native Windows, graphics,
  sheep behavior, and cross-platform identity were not tested. Evidence:
  [`gameplay_simulation.hpp`](src/game/gameplay_simulation.hpp),
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp), and
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md).
- [x] Render five simple, recognizable procedural sheep proxies from published
  read-only snapshots in the accepted paddock with the dog and intended gameplay
  camera. Keep authoritative collision and rules separate from visual geometry;
  articulated motion, wool detail, final materials, and final tuning remain
  Phase 4 work. Observed result: one shared blocky mesh supplies a cream
  body/tail, dark face, paired ears, and four legs. A fixed renderer-facing pose
  buffer copies stable IDs 1-5 plus interpolated positions/headings one-to-one
  from the published snapshot and retains no authoritative state. Static
  environment/debug scenarios submit zero sheep, and no accepted baseline file
  was changed. WSL development and ASan/UBSan suites passed 22/22 CTests; format
  and bounded static analysis passed. Native Windows Release passed 37/37
  CTests on Intel UHD Graphics 630 with zero high-severity messages. Two
  independent 960x540 gameplay captures were byte-identical at SHA-256
  `3e5e922def861473fee18a3fef234696c48638cfbbc306daab1afd9d0d2aaa5b`,
  and agent inspection found all five proxies visible with the dog and gate.
  This does not accept final animal art or behavior. Evidence:
  [`sheep_proxy.hpp`](src/render/sheep_proxy.hpp),
  [`sheep_proxy_tests.cpp`](tests/sheep_proxy_tests.cpp), and the ignored
  [`native Windows packet`](artifacts/phase2/2026-08-16/tracer1-review-windows-195609961/review.md).
- [x] Add a deterministic presentation fixture that moves and turns the five
  proxies without pretending that scripted transforms are accepted flock
  behavior. Observed result: version 1, seed-zero `presentation-motion` moves
  all five authoritative fixture records synchronously around a four-leg square
  while retaining settled/zero-arousal non-behavior state. The bounded render
  smoke pre-rolls to tick 61 and presents alpha 0.5 through the existing proxy
  material, static shadow receiver, and draw path. Repeated CPU state, midpoint
  interpolation, restart, immutable-prior publication, and one-to-one rendered
  pose mapping pass on WSL. At that checkpoint native OpenGL execution remained
  for the following checked capture/measurement packet.
- [x] Capture same-state normal/debug frames plus a short motion/contact-sheet
  packet, and record CPU snapshot/presentation preparation, render submission,
  GPU/frame p50/p95/p99, allocations, and RSS at 1920x1080 on the available
  native Windows proxy. Observed result: the native Windows Release packet
  passed 39/39 CTests and retained tick 1/61/121 normal frames, a tick-61 debug
  frame, a three-frame contact sheet, canonical version 2 state dumps, source
  hashes, and serialized measurements. The repeated normal frame and all three
  tick-61 state dumps were byte-identical. Snapshot/presentation preparation,
  CPU submission, GPU render, and synchronized frame p95 values were 15,200,
  361,500, 2,367,409, and 3,498,600 ns; p99 values were 26,200, 485,400,
  2,656,996, and 4,886,200 ns. Fixed-update and presentation-preparation
  allocation oracles each observed zero allocations across 600 iterations;
  peak RSS was 104,382,464 bytes. This passes the provisional Low comparison on
  Intel UHD Graphics 630 but does not establish the named Iris Xe target.
  Evidence: the ignored
  [`candidate packet`](artifacts/phase3/2026-08-16/tracer2-presentation-windows-204608051/review.md).
- [x] Optimize only a named failing budget or measured bottleneck, then obtain
  an explicit owner verdict that this five-animal packet is representative
  enough for gameplay iteration. The verdict does not approve final animal art.
  Observed result: no named budget failed, so no optimization was authorized or
  performed. On 2026-08-16, the owner selected **Accept** for the packet as
  representative enough to begin gameplay iteration with its documented limits.
  Acceptance does not approve final animal art, flock behavior, the named Iris
  Xe target, or native Linux graphics.

### Sheep behavior

- [x] Implement a uniform spatial grid for bounded neighbor queries. Observed
  result: `game` now owns a fixed-capacity ground-plane grid that copies
  published sheep ID/position fields into deterministic sorted cell/row ranges.
  Caller-owned output spans bound nearest-neighbor selection; exact-distance
  filtering and distance/ID/source-index ordering are independent of storage
  order. Focused tests cover negative and exact cell/radius boundaries, cell-box
  false positives, subject exclusion, truncation, reversed storage, snapshot
  immutability, invalid inputs, and capacity failure. The direct WSL oracle
  observed zero allocations across repeated rebuild/query cycles and at the
  1,000-member capacity-experiment ceiling. Development and ASan/UBSan suites
  each passed 24/24 CTests; formatting and bounded static analysis passed.
  Native Windows was not rerun for this headless platform-independent outcome.
  This does not establish 1,000-sheep performance or product scope.
- [x] Reconcile the senior-review contract findings before social behavior.
  Observed result: shared game math and complete gameplay-scenario ownership are
  independent of `DogController`; clipped first-contact velocity, full sheep
  state validation, milestone-specific 1 GiB/512 MiB performance budgets, exact
  Release budget pass markers, and complete Phase 3 build-input hashes have
  focused coverage. WSL development, Release, and ASan/UBSan configurations each
  passed 24/24 CTests; formatting and bounded static analysis passed. Native
  graphics and measurements were not rerun. Evidence:
  [`ADR 0003`](docs/decisions/0003-project-owned-test-harness.md),
  [`ADR 0004`](docs/decisions/0004-gameplay-scenario-ownership.md),
  [`performance_tests.cpp`](tests/performance_tests.cpp), and
  [`dog_controller_tests.cpp`](tests/dog_controller_tests.cpp).
- [x] Implement named, independently inspectable close-range repulsion from the
  immutable prior sheep buffer. Observed result: the version 1, seed-zero
  `sheep-only-separation` scenario owns an exact-overlap five-sheep start plus a
  1.0-world-unit radius and 4.0-world-unit/s² acceleration cap. Each fixed tick
  rebuilds and queries the accepted grid from the prior buffer, applies a linear
  planar falloff, caps the combined acceleration, and publishes all next states
  synchronously. Exact overlap uses an antisymmetric stable-ID direction rather
  than randomness. The focused oracle proved initially out-of-range rejection,
  recovery beyond the configured radius, acceleration bounds on every sampled
  tick, exact per-ID results after reversing storage, coherent restart, and zero
  allocations across 600 fixed updates. WSL development, Release, and
  ASan/UBSan configurations each passed 24/24 CTests; formatting and bounded
  clang-tidy passed. Native graphics and measurements were not rerun because no
  presentation path changed. At that checkpoint attraction, alignment, damping,
  bounded speed/turning, dog pressure, and behavior transitions remained
  unimplemented.
  Evidence: [`gameplay_scenario.hpp`](src/game/gameplay_scenario.hpp),
  [`gameplay_simulation.cpp`](src/game/gameplay_simulation.cpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- [x] Add attraction to a bounded selected-neighbor set with explicit chosen-
  neighbor evidence. Observed result: the version 1, seed-zero
  `sheep-only-attraction` scenario enables attraction independently from
  separation, places four candidates within sheep 1's 4.0-world-unit radius,
  and selects at most two nearest neighbors through the accepted prior-snapshot
  grid. Selection order is distance then stable ID; acceleration pulls toward
  the selected prior-position centroid, is capped at 1.5 world units/s², and is
  published separately from separation with exact subject/neighbor IDs and
  selected/candidate counts. Required social evidence advanced the canonical
  state dump to version 3. The focused oracle observed sheep 1 select IDs 2 and
  3 from four candidates, matched the published vector to applied acceleration,
  proved exact per-ID state/evidence after reversed storage, restored the dense
  fixture on restart, and observed zero allocations across 600 attraction
  ticks. WSL development, Release, and ASan/UBSan configurations each passed
  24/24 CTests; formatting and bounded clang-tidy passed. Native graphics and
  measurements were not rerun because presentation did not change. At that
  checkpoint alignment, combined-influence acceleration bounds, damping,
  bounded speed/turning, dog pressure, and behavior transitions remained
  unimplemented. Evidence:
  [`sheep_state.hpp`](src/game/sheep_state.hpp),
  [`gameplay_simulation.cpp`](src/game/gameplay_simulation.cpp),
  [`gameplay_replay.cpp`](src/game/gameplay_replay.cpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- [x] Add alignment as an independently switchable term; keep or remove it only
  after paired alignment-on/alignment-off fixtures are observed. Observed
  result: version 1, seed-zero `sheep-alignment-off` and
  `sheep-alignment-on` scenarios share the same five moving sheep and otherwise
  identical configuration after normalizing their required distinct scenario
  IDs. The on case queries the immutable-prior grid, selects at most one nearest
  neighbor by the accepted distance/ID ordering, and accelerates toward its
  prior velocity under a 1-second response time and 1.5-world-unit/s² cap.
  Required alignment IDs, selected/candidate counts, and a separate alignment
  vector advanced the state dump to version 4. Across 60 ticks the focused
  oracle measured polarization `0.824621` off versus `0.924042` on, so alignment
  is retained provisionally for later dog-response scenarios. It also matched
  sheep 1's exact selected ID and applied vector, proved exact per-ID state and
  evidence after reversed storage, restored the fixture on restart, and
  observed zero allocations across 600 alignment ticks. WSL development,
  Release, and ASan/UBSan configurations each passed 24/24 CTests; formatting
  and bounded clang-tidy passed. Native graphics and measurements were not
  rerun because presentation did not change. This is synthetic causal evidence,
  not biological validation or player-facing motion acceptance. Evidence:
  [`gameplay_scenario.hpp`](src/game/gameplay_scenario.hpp),
  [`gameplay_simulation.cpp`](src/game/gameplay_simulation.cpp),
  [`gameplay_replay.cpp`](src/game/gameplay_replay.cpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- [ ] Implement dog pressure from distance, approach velocity, facing, line of
  sight, terrain, and temperament.
  Partial observed result (2026-08-16): version 1, seed-zero
  `sheep-dog-pressure-off` and `sheep-dog-pressure-on` scenarios share one
  stationary dog/five-sheep fixture and differ only by the pressure switch plus
  required scenario ID. Both publish per-sheep prior-state planar dog distance
  and signed bearing relative to sheep heading; only the on case applies a
  separate vector directly away from the dog with linear falloff inside a
  6-world-unit radius and a 3-world-unit/s² maximum. The focused oracle matched
  exact geometry, falloff, direction, and applied acceleration; proved that
  same-tick dog movement did not alter prior-state stimulus; rejected pressure
  at and beyond the radius; published zero bearing/vector at exact dog/sheep
  overlap instead of an arbitrary direction; preserved exact per-ID state/
  evidence under reversed storage; restored the fixture on restart; and observed
  zero allocations across 600 enabled ticks. Required evidence advanced the
  state dump to version 5.
  WSL development, Release, and ASan/UBSan configurations each passed 24/24
  CTests; formatting and bounded clang-tidy passed. Native graphics and
  measurements were not rerun because presentation did not change.
  Partial observed result (2026-08-17): version 1, seed-zero
  `sheep-dog-approach-off` and `sheep-dog-approach-on` scenarios add one dog
  moving at 4.0 world units/s, keep the accepted distance-only pressure enabled
  and byte-identical in both cases, and differ only by the approach switch plus
  required scenario ID. Both publish per-sheep prior-state dog approach speed,
  the component of prior dog velocity along the dog-to-sheep direction; only the
  on case adds a separate away-from-dog vector that responds to a closing dog,
  shares the pressure radius and linear falloff, and saturates at the scenario
  reference speed. In the first tick the focused oracle observed a head-on
  `4.0` closing speed above the `3.0` reference saturating at `1.333333`
  acceleration, exactly `0` approach speed abeam, `-4.0` behind the dog with no
  vector so a leaving dog releases rather than pulls, the exact `2.4` projection
  and `(0.16, 0.213333)` vector on a 3-4-5 diagonal, and `4.0` approach speed
  published without influence outside the 6-unit radius. It also matched the
  published pressure-plus-approach sum to applied acceleration, proved that the
  same tick's dog-motor velocity change did not alter prior-state approach
  evidence, invented no direction at exact dog/sheep overlap, preserved exact
  per-ID state/evidence under reversed storage, restored the fixture on restart,
  and observed zero allocations across 600 enabled ticks. Required evidence
  advanced the state dump to version 6. WSL development, Release, and ASan/UBSan
  configurations each passed 24/24 CTests; formatting and bounded clang-tidy
  passed. Native graphics and measurements were not rerun because presentation
  did not change.
  Partial observed result (2026-08-17): version 1, seed-zero
  `sheep-dog-facing-off` and `sheep-dog-facing-on` scenarios share one
  stationary-dog/five-sheep fixture, keep the accepted distance-only pressure
  enabled and byte-identical in both cases, and differ only by the facing switch
  plus required scenario ID. Both publish per-sheep prior-state dog facing
  alignment, the cosine between the prior dog forward direction and the
  dog-to-sheep direction; only the on case adds a separate away-from-dog vector
  scaled by the positive part of that alignment under the shared pressure radius
  and linear falloff. In the first tick the focused oracle observed exact `1.0`
  alignment and the full `(0, -1)` facing acceleration for the sheep straight
  ahead at distance 2, exactly `0` alignment and distance pressure only abeam,
  `-1.0` alignment with no vector directly behind so a dog looking away releases
  rather than pulls, the exact `0.8` cosine and `(0.12, -0.16)` vector on a
  3-4-5 diagonal, and `1.0` alignment published without influence outside the
  6-unit radius. Turning the same fixture's dog through half a turn without
  moving any position swapped the front and back results exactly, proving the
  term reads dog heading rather than fixture layout. The oracle also matched the
  published pressure-plus-facing sum to applied acceleration, proved that the
  same tick's dog-motor turn did not alter prior-state facing evidence, invented
  no direction at exact dog/sheep overlap, preserved exact per-ID state/evidence
  under reversed storage, restored the fixture on restart, and observed zero
  allocations across 600 enabled ticks. Required evidence advanced the state
  dump to version 7. WSL development, Release, and ASan/UBSan configurations
  each passed 24/24 CTests; formatting and bounded clang-tidy passed. Native
  graphics and measurements were not rerun because presentation did not change.
  The roadmap item remains unchecked: line of sight,
  terrain, and temperament are still absent, the three dog terms are summed
  without a combined-influence bound, and these synthetic fixtures are not
  player-facing motion acceptance. Evidence:
  [`sheep_state.hpp`](src/game/sheep_state.hpp),
  [`gameplay_scenario.hpp`](src/game/gameplay_scenario.hpp),
  [`gameplay_simulation.cpp`](src/game/gameplay_simulation.cpp),
  [`gameplay_replay.cpp`](src/game/gameplay_replay.cpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- [ ] Give sheep the same analytic paddock/wall/gate collision authority the
  dog already has, so a wall or closed gate physically stops a driven sheep and
  the open gate is the only way through. Steering-level avoidance below is not
  a substitute for this hard boundary, and voxel faces and render meshes stay
  out of gameplay rules. Without this, the Phase 3 exit-gate replay cannot be
  trusted.
- [ ] Implement obstacle/drop avoidance and bounded acceleration/turning,
  including the combined-influence acceleration bound across all social and dog
  terms. Decide the combination rule (clamp, prioritized weighting, or another
  explicit scheme) before implementing it, and plan for the paired-fixture
  oracles that pin exact per-term accelerations to be re-derived when the
  bound lands; until then the terms remain summed without a combined bound.
- [ ] Implement ordinary, nervous, and stubborn temperaments.
- [ ] Implement settled, alert, driven, and recovering transitions plus an
  explicitly non-physiological arousal/recovery proxy.
- [ ] Add debug arrows/labels for every influence, chosen neighbor, arousal,
  target, state, and balance point.
- [ ] Test that randomness never masks unstable or unexplained steering.

### Behavior observability and early scale hygiene

- [x] Emit centroid, mean radius, polarization, elongation, group speed,
  nearest-neighbor spacing, connected components, and chosen-neighbor counts
  from published five-sheep state. Observed result: a pure fixed-size pass uses
  ground-plane distances/velocities, reports per-member spacing plus aggregate
  values, and takes connectivity distance and precomputed chosen-neighbor counts
  explicitly without selecting neighbors or mutating simulation state.
- [ ] Add dog bearing/distance, response latency, split/rejoin time, and settle
  time when the required named behavior scenarios and transitions exist.
  Partial observed result (2026-08-16): the distance-only pressure pair now
  publishes per-sheep causal dog distance and relative bearing in state-dump
  version 5. Flock/rear-sheep summaries, response latency, split/rejoin time, and
  settle time remain absent pending the required transition scenarios, so this
  item remains unchecked.
- [x] Unit-test the implemented observable definitions on hand-authored positions
  and velocities. Observed result: exact symmetric-cross and separated-line
  fixtures cover isotropic/collinear shape, aligned/opposed/zero motion,
  transitive connectivity, spacing, and neighbor-count summaries; invalid
  identity, finite-value, threshold, and count inputs are rejected. WSL
  development and ASan/UBSan suites each passed 23/23 CTests, and formatting
  plus bounded static analysis passed. Native Windows was not rerun because the
  outcome is headless and platform-independent.
- [ ] Add non-player diagnostic fixtures for 5, 14, 25, and 100 sheep without
  making large flocks a Tracer 2 content requirement.
- [ ] Record spatial-grid build, neighbor selection, behavior, terrain query,
  snapshot, allocation, and total simulation costs separately.
- [x] Compare alignment-on and alignment-off fixtures; retain explicit alignment
  only when measured behavior and legibility justify it. Observed result: the
  paired 60-tick metric and exact selected-ID/vector state evidence above
  improved directional agreement and kept the cause inspectable, justifying
  provisional retention until player-facing dog-response evidence can confirm
  or reject its usefulness.

### Objective loop

- [ ] Spawn one dog, farmer placeholder, five sheep, one gate, and destination
  pen.
- [ ] Implement one gather-and-drive farmer whistle.
- [ ] Implement explicit success, recoverable failure, restart, and concise
  coaching after repeated failure.
- [ ] Add minimal HUD showing only farmer signal, flock status, objective, and
  debug mode.

### Phase 3 exit gate

- [ ] A recorded input sequence deliberately moves all five sheep through the
  gate.
- [ ] The replay reaches the same objective result on repeated runs.
- [ ] Debug views explain surprising flock responses without guessing.
- [ ] Headless tests cover pressure direction, neighbor bounds, temperament,
  arousal/recovery, obstacles, split/rejoin, gate counting, and restart.
- [ ] The representative paddock, dog, five-sheep, objective, and debug workload
  meets provisional frame-time and memory budgets with ample headroom; any
  optimization is tied to a recorded bottleneck.
- [ ] A short motion/contact-sheet packet plus matching debug/state evidence
  receives an explicit owner verdict on flock readability and causality.

### Post-Phase 3 graphics-backend decision

This is a decision checkpoint after the playable-loop gate, not a Phase 3 exit
condition and not authorization to begin a renderer migration.

- [ ] Record one owner verdict before material renderer growth: either retain
  OpenGL through the currently justified work and revisit only on a measured
  capability/bottleneck, or authorize Vulkan parity planning. A Vulkan-planning
  verdict requires a native Windows/Linux capability inventory on the actual
  target classes, adversarial planning from
  [`docs/research/opengl-to-vulkan-feasibility.md`](docs/research/opengl-to-vulkan-feasibility.md),
  an accepted superseding ADR, preservation of OpenGL as a temporary known-good
  reference, and explicit parity/cancellation/retirement gates before backend
  implementation. Mesh shaders, ray tracing, Brixelizer GI, async compute, and
  permanent dual-backend support remain separate unapproved decisions.

## Phase 4 — Tracer 3: readable procedural art and feedback

Phase 4 refines or replaces the deliberately simple Phase 3 animal proxies only
after the five-sheep loop and representative render envelope have earned deeper
presentation investment.

### Art bible in code

- [ ] Approve grid scale, silhouette rules, palette/value hierarchy, material
  response, biome rules, lighting intent, and controlled variation limits.
- [ ] Define when a generated variation is invalid rather than accepting every
  seed.
- [ ] Produce identical-camera reference captures for the approved look.
- [ ] Record the approved look and animal-motion evidence through a visual-review
  packet; an agent cannot approve or overwrite the baseline.

### Code-generated animals

- [ ] Generate an articulated border collie from named body parts and joints.
- [ ] Generate readable procedural black/white markings.
- [ ] Generate articulated sheep with controlled body/wool/face variation.
- [ ] Implement dog idle, walk, sprint, crouch, turn, stop, head/gaze, ears, and
  tail cues.
- [ ] Implement sheep idle, locomotion, hesitation, alarm, bunching, and settling
  cues.
- [ ] Add slope-aware placement or simple foot correction only if visible
  evidence justifies it.
- [ ] Preserve simple authoritative collision regardless of visual complexity.

### Feedback and atmosphere

- [ ] Add farmer whistle and essential dog/sheep/environment audio, procedural or
  provenance-approved according to the Phase 0 decision.
- [ ] Add restrained dust/grass/contact feedback where it improves pressure and
  movement readability.
- [ ] Add accessible controls for camera sensitivity, inversion, shake, contrast,
  and reduced motion as applicable.

### Fresh-player gate

- [ ] Re-capture and remeasure the representative whole-scene workload after
  articulated animals, animation, and feedback replace the Phase 3 proxies.
  Compare against the proxy envelope and clear named frame-time, GPU, allocation,
  and memory regressions before recruiting players.
- [ ] Run the first-playable test with at least five fresh players and record
  build, platform, method, behavior, comments, and failures.
- [ ] Confirm at least four of five complete within ten minutes after a controls-
  only introduction, or revise the design instead of declaring success.
- [ ] Confirm players can explain why the flock turns/splits and at least three
  intentionally release pressure.
- [ ] Decide keep/change/simplify/pivot from evidence.
- [ ] Only after a positive core-loop decision, develop and approve the detailed
  reward, progression, session, and additional-animal design; do not infer it
  from the generated reference-image UI.

## Phase 5 — Tracer 4: procedural voxel world

- [ ] Define deterministic terrain inputs and versioned generation parameters.
- [ ] Add bounded hills and valleys while preserving navigable herding surfaces.
- [ ] Add biome palette rules and deliberate transitions.
- [ ] Generate stone walls, paths, gates, barns, trees, hedges, rocks, and
  landmarks from readable placement grammars.
- [ ] Reject invalid placements and unreachable objectives.
- [ ] Add caves, water, weather, or additional biomes only when the approved game
  design needs them.
- [ ] Add chunk serialization and migration/version behavior.
- [ ] Add background generation/meshing with explicit ownership, cancellation,
  stale-result rejection, and per-frame budgets.
- [ ] Preserve all handcrafted herding replays as regression scenarios.
- [ ] Capture comparable normal/debug evidence for the fixed seed set, including
  camera, generation parameters, validity results, and performance metadata.

### Phase 5 exit gate

- [ ] Ten fixed seeds produce traversable, coherent worlds without intersections,
  impossible gates, or unreadable flock routes.
- [ ] Generation and meshing latency stay within named budgets.
- [ ] The world supports the herding loop rather than distracting from it.
- [ ] The bounded fixed-seed visual packet receives an explicit owner verdict.

## Phase 6 — Tracer 5: measured scale and renderer depth

- [ ] Establish low/high hardware capture baselines before optimizing.
- [ ] For each renderer-depth candidate, define a deterministic representative
  scene/route and at least one small owner-controlled holdout camera or seed that
  was not the tuning target; retain comparable normal, debug, and motion
  evidence for both where applicable.
- [ ] Add feature-owned debug outputs and stable human-readable pass/resource
  labels with the passes that need them. Do not build unused depth, normal,
  motion-vector, shadow-mask, history-rejection, LOD, overdraw, or residency
  outputs before their owning feature exists.
- [ ] Record per-pass GPU timing and relevant draw, dispatch, upload, and memory
  values when tied to a named budget or optimization decision; retain total
  frame percentiles and reject measurement-only dashboards with no decision.
- [ ] Benchmark authoritative flock simulation at 5, 14, 25, 100, 250, 500,
  and 1,000 sheep with fixed scenarios and separate CPU-stage, GPU, allocation,
  and memory measurements.
- [ ] Compare 100-sheep group observables with smaller full-rate fixtures before
  treating higher counts as behaviorally valid.
- [ ] Treat 250-, 500-, and 1,000-sheep results as capacity evidence until camera
  readability and playtests show that scale adds decisions or fun.
- [ ] If simulation LOD, jobs, or GPU compute is proposed, prove a measured
  bottleneck first and compare behavior against the full-rate baseline.
- [ ] Add frustum culling and measure it.
- [ ] Add mesh caching and budgeted GPU upload queues and measure them.
- [ ] Add greedy meshing only if it improves the measured bottleneck without
  breaking boundary tests.
- [ ] Add streaming distance and eviction with explicit memory budgets.
- [ ] Add LOD only if draw distance remains a measured constraint.
- [ ] Evaluate FLIP or another maintained perceptual image comparator at this
  gate. Pin its version/license, calibrate reference-machine masks and thresholds,
  retain a difference map, and use it alongside semantic assertions, motion
  evidence, and owner review rather than as an automatic quality oracle.
- [ ] Add a slow offline or higher-quality reference only when a named
  lighting/material calculation has a bounded reference question, versioned
  inputs, and tolerance; otherwise record that it was intentionally deferred.
- [ ] Evaluate RenderDoc and optionally RenderDoc MCP at this gate when a real
  capture answers a named pass, resource, timing, or driver question. A backend
  migration or difficult GPU defect must also record the graphics validation
  modes, capture tool/version, findings, and unresolved warnings.
- [ ] Consider SSAO, improved anti-aliasing, stylized water, volumetric atmosphere,
  PCSS, or reflections one at a time with identical-state evidence.
- [ ] Reject effects that reduce flock readability, temporal stability, or low-
  target performance.

### Phase 6 exit gate

- [ ] Frame-time percentiles, memory, startup, and chunk latency pass on named low
  and high targets.
- [ ] Captures show stable motion and no high-severity graphics
  API/validation/debugger findings.
- [ ] Low/high profiles have intentional differences and tested defaults.
- [ ] Each accepted renderer-depth candidate has feature-owned debug evidence,
  named-pass measurements where relevant, and representative/holdout comparison
  results; any perceptual or offline-reference result is reproducible and is not
  the sole acceptance oracle.
- [ ] Same-state normal/debug/motion evidence receives an explicit owner verdict
  before a renderer-depth candidate replaces an accepted baseline.

## Phase 7 — Tracer 6: product hardening

- [ ] Add settings persistence, safe save/load, schema/version handling, and
  recovery from interrupted writes.
- [ ] Add complete input remapping and supported controller coverage.
- [ ] Complete UI states, accessibility, localization architecture, and claimed
  locale tests.
- [ ] Add crash diagnostics and evaluate Sentry only if product/privacy
  requirements approve it.
- [ ] Add privacy-minimized analytics only after a specific playtest/product
  question requires it.
- [ ] Create reproducible release packaging, dependency licenses, symbols, smoke
  tests, and rollback instructions.
- [ ] Verify supported Windows/Linux hardware and driver matrix.
- [ ] Retain comparable native Linux/Windows startup, render, and smoke-capture
  manifests; WSL evidence is labeled separately.
- [ ] Run the bounded ultra production pass only after core gameplay and release
  readiness gates pass.

## Deferred ideas—not current scope

- [ ] Multiple farms or open world.
- [ ] Weather affecting scent, footing, or urgency.
- [ ] Lamb/adult attachment.
- [ ] Ducks, geese, goats, or cattle.
- [ ] A researched animal-learning progression in which each species introduces
  a distinct group behavior and player judgment.
- [ ] Up to 1,000 authoritative sheep as an actual gameplay requirement rather
  than a capacity benchmark.
- [ ] Multi-dog or cooperative play.
- [ ] RTS-style overhead command mode with agent selection and location/action
  orders; exact camera, mouse, keyboard, and command semantics require a
  separate design experiment.
- [ ] Accounts, cloud saves, leaderboards, backend, or online competition.
- [ ] Strict one-megabyte executable or zero-dependency challenge.
- [ ] Advanced volumetric renderer feature parity with LumenFall's claims.

Checking a deferred item does not authorize implementation; move it into an
approved milestone first.

## Decision and evidence log

Add short dated entries here or link a dedicated decision record once
implementation begins.

- **2026-08-14 — Clean-room boundary:** LumenFall is inspiration only; its public
  repository did not provide source or a reuse license.
- **2026-08-14 — Tooling:** No MCP is required for Phase 0/1. Prefer normal CLI,
  official documentation, deterministic captures, and tests. External candidates
  and their adoption gates are recorded in
  [`AGENT_HARNESS_AND_TOOLS.md`](docs/AGENT_HARNESS_AND_TOOLS.md).
- **2026-08-14 — Scope:** The current game experiment remains five sheep, one
  dog, one farmer signal, one gate, success/failure/restart, and debug evidence.
- **2026-08-15 — Foundation:** The project owner selected the custom C++ engine
  as the primary track and approved native Linux/Windows releases plus the
  provisional C++23/CMake/Ninja/SDL3/OpenGL 4.6 foundation. The initially named
  doctest framework was never adopted and was superseded by the project-owned
  CTest harness decision on 2026-08-16. Evidence:
  [`ADR 0001`](docs/decisions/0001-native-foundation.md) and
  [`ADR 0003`](docs/decisions/0003-project-owned-test-harness.md).
- **2026-08-15 — Assets:** Use procedural-first media. Tracer 0–2 remains free
  of imported runtime media; later authored exceptions require readability or
  feedback evidence plus complete provenance and license review.
- **2026-08-15 — Budgets and dependencies:** Low/high targets, frame-time,
  memory, startup, package-size limits, dependency pinning, and the license
  allowlist are accepted provisionally in
  [`ADR 0001`](docs/decisions/0001-native-foundation.md).
- **2026-08-15 — Optional MCPs:** `mcp-cpp` 0.2.2, GDB MCP at commit
  `605220a4bbbbbe2e87629f29dc1136fb970f6525`, and `renderdoc-mcp` 0.3.0 are
  installed and registered. They require a new Codex session and do not replace
  builds, tests, replays, or captures. DebugMCP remains pending explicit approval
  of its local debugger code-execution surface. Evidence:
  [`AGENT_HARNESS_AND_TOOLS.md`](docs/AGENT_HARNESS_AND_TOOLS.md).
- **2026-08-15 — Herding research and scale:** Keep five sheep as the correctness
  and first-fun gate, but make Tracer 2 data-oriented and observable from the
  start. Research-comparison fixtures use 14 and 100 sheep; later capacity
  benchmarks use 250, 500, and 1,000. No large count is a product promise until
  performance, behavior, camera readability, and playtests support it. Evidence:
  [`herding-simulation-and-scale.md`](docs/research/herding-simulation-and-scale.md)
  and its [implementation plan](docs/plans/herding-simulation-and-scale.md).
- **2026-08-15 — Reference boundary:** The two generated images supply
  mood/composition cues only. Their HUD, commands, score, inventory, minimap,
  camera, and task structure are not requirements. Broader gameplay hypotheses
  live in [`HERDING_GAMEPLAY.md`](docs/game-design/HERDING_GAMEPLAY.md).
- **2026-08-15 — Standardized feedback loop:** The owner approved the
  goal/context/invariants/evidence task contract, proportional verification
  cadence, reproducible failure artifacts, explicit human visual-baseline gate,
  and fresh/continue/compact context handoff. The `.clang-format` and
  `.clang-tidy` files were initially provisional; their subsequent validation
  is recorded below. Evidence:
  [`DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md), the
  [visual-review packet](docs/review/HUMAN_VISUAL_REVIEW.md), and the
  [implementation plan](docs/plans/agentic-development-workflow.md).
- **2026-08-15 — Phase 0 toolchain and context observation:** Ubuntu 24.04.4
  tooling was first provided through a verified ignored local fallback, then
  installed system-wide by the owner. Clang 18.1.3 built the pinned SDL 3.4.10
  diagnostic as C++23. The normal WSLg X11 session and `xvfb-run` both passed
  explicit OpenGL 4.5 Core debug-context creation and clean shutdown using Mesa
  25.2.8 `llvmpipe`; both rejected the approved 4.6 request with
  `GLXBadFBConfig`. This is development-host capability evidence, not native
  Linux/Windows release support. Evidence:
  [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- **2026-08-15 — Platform workflow and Windows inventory:** The owner retained
  OpenGL 4.6, selected WSL Ubuntu for everyday non-hardware work, selected native
  Windows for the first real-GPU build/render gate, and deferred native Linux
  proof to an actual Linux installation or machine. Windows 11 build 26200
  reports Intel UHD Graphics 630 and NVIDIA GTX 1050 Ti Max-Q adapters, but no
  native CMake, Ninja, MSVC/clang-cl, Visual Studio, or `vswhere` was present in
  the initial inventory, so the Windows compiler/context gate was unrun at that
  point. The following entry records its subsequent completion. Evidence:
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- **2026-08-15 — Native Windows Phase 0 gate:** Visual Studio Build Tools
  17.14.37 provided CMake 3.31.6-msvc6, Ninja 1.12.1, and MSVC 19.44.35228.0.
  The pinned SDL 3.4.10/C++23 diagnostic built natively and passed the explicit
  OpenGL 4.6 Core debug-context request three times, including an initial build
  and cached reruns. Windows selected the Intel UHD Graphics 630 hardware
  renderer (driver 27.20.100.9664), which reported OpenGL 4.6 and GLSL 4.60.
  This completes Phase 0 but is not a full performance or release-support
  claim. Evidence: [`WINDOWS.md`](docs/setup/WINDOWS.md).
- **2026-08-15 — Project developer checks:** Ubuntu LLVM 18.1.3
  `clang-format` and `clang-tidy` were first provided through the ignored local
  fallback, then installed system-wide by the owner. The documented
  `format-check` and `clang-tidy-check` CMake targets prefer the system tools,
  fall back locally, and passed against the SDL lifecycle source while the
  development and ASan/UBSan tests passed. The targets enumerate only Wide
  Eye-owned source files and do not apply either policy to fetched dependency
  sources. Evidence: [README native engine quick start](README.md#native-engine-quick-start),
  [`WideEyeDeveloperTools.cmake`](cmake/WideEyeDeveloperTools.cmake), and
  [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- **2026-08-15 — SDL window lifecycle:** The main build now fetches the pinned,
  checksum-verified SDL 3.4.10 source, disables unowned audio/controller/rendering
  subsystems. Its bounded `--window-smoke` regression opens and closes a native
  window without creating a GL context. The dummy-driver scenario passed in
  development, ASan/UBSan, and release presets; a normal WSLg run reported
  `video_driver=x11`. The native Windows development preset passed both then-
  current CTests with MSVC 19.44.35228.0, and its normal smoke reported
  `video_driver=windows`. Native Linux remains unverified. Evidence:
  [`main.cpp`](src/platform/main.cpp),
  [`CMakeLists.txt`](CMakeLists.txt),
  [`WideEyeDependencies.cmake`](cmake/WideEyeDependencies.cmake), and the
  [Windows smoke runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — Project OpenGL context reporting:** The interactive executable
  and bounded `--context-smoke` now request OpenGL 4.6 Core, validate the actual
  version and Core-profile bit, report vendor/renderer/GL/GLSL strings, and shut
  down cleanly without rendering. A source-hashed native Windows MSVC
  19.44.35228.0 copy-build passed three CTests and the direct context smoke on
  Intel UHD Graphics 630, reporting OpenGL 4.6 and GLSL 4.60. Direct development
  and sanitized requests on WSL failed at context creation with the previously
  observed `GLXBadFBConfig`; no lower or compatibility fallback was added.
  Native Linux remains unverified. Evidence: [`main.cpp`](src/platform/main.cpp),
  [`CMakeLists.txt`](CMakeLists.txt), [`WINDOWS.md`](docs/setup/WINDOWS.md), and
  the [Windows project runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — First triangle:** The `render` boundary now loads its required
  OpenGL entry points, compiles and links GLSL 4.60 shaders, owns a VAO/VBO, and
  draws one colored triangle. A dedicated hidden smoke reads the center pixel
  before presentation and rejects the clear color. A source-hashed native
  Windows MSVC 19.44.35228.0 copy-build passed five development CTests and the
  direct triangle smoke on Intel UHD Graphics 630; the sampled center was
  `99,127,155,255` with zero high-severity messages. Development, ASan/UBSan,
  and release builds plus their default fast tests passed on WSL, but its 4.5
  ceiling prevents executing the accepted 4.6 triangle path there. The cube,
  depth testing, PNG capture, visual packet, native Linux proof, and broad GPU
  support remain unverified. Evidence:
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
  [`main.cpp`](src/platform/main.cpp), [`CMakeLists.txt`](CMakeLists.txt),
  [`WINDOWS.md`](docs/setup/WINDOWS.md), and the
  [Windows project runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — Cube, lifecycle, and runtime foundation:** The interactive
  executable now renders a perspective voxel cube with explicit depth state,
  tracks drawable resize, minimize/restore, focus, and close transitions, and
  advances a steady-clock 60 Hz fixed-step accumulator independently of render
  cadence. Core logging and fatal assertions have automated checks. A
  source-hashed native Windows MSVC 19.44.35228.0 copy-build passed all nine
  development CTests and both direct render smokes on Intel UHD Graphics 630.
  The cube oracle observed a D24S8 framebuffer, center RGBA `229,56,31,255` at
  depth `0.959411`, `LESS`, depth writes, and zero high-severity messages. WSL
  development, ASan/UBSan, and release presets passed their five fast tests,
  plus project formatting and bounded static analysis. At that checkpoint, PNG
  capture, headless graphics reproduction, the artifact manifest, visual review,
  and native Linux proof remained open; the following entry resolves only the
  first two. Evidence: [`runtime.cpp`](src/core/runtime.cpp),
  [`window_state.cpp`](src/platform/window_state.cpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
  [`WINDOWS.md`](docs/setup/WINDOWS.md), and the
  [Windows project runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — Deterministic cube capture:** The named `voxel_cube_smoke`
  scenario now supports `--capture <png-path>`. The renderer reads the RGBA8
  framebuffer before swap and normalizes OpenGL row order; a project-owned,
  dependency-free writer emits fixed-filter, uncompressed PNG bytes. A known-
  byte unit test passed in WSL development, ASan/UBSan, and release builds. A
  source-hashed native Windows MSVC 19.44.35228.0 copy-build passed all 11
  development CTests, including a hidden two-run hash comparison, and retained
  a valid 64x64 capture with SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`.
  Agent inspection found the expected colored cube against the dark clear
  color, but at that checkpoint no golden or owner visual verdict existed. The
  artifact manifest,
  failure packet, visual-review packet, and native Linux graphics proof remain
  open. Evidence: [`png_writer.cpp`](src/render/png_writer.cpp),
  [`assert-deterministic-png.cmake`](tests/assert-deterministic-png.cmake),
  [`WINDOWS.md`](docs/setup/WINDOWS.md), and the
  [Windows project runner](tools/phase1/run-window-smoke.ps1).
- **2026-08-15 — Candidate cube visual packet:** The renderer gained a bounded
  `--voxel-cube-debug-smoke` wireframe view that reuses the normal cube geometry,
  camera, viewport, shader, and depth state. A source-hashed native Windows MSVC
  19.44.35228.0 copy-build passed 14 development CTests on Intel UHD Graphics
  630, including two-run deterministic PNG checks for both normal and debug
  views. The runner emitted the ignored
  [`windows-cube-smoke-220642406`](artifacts/phase1/2026-08-15/windows-cube-smoke-220642406/review.md)
  candidate packet; its normal PNG retained SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`,
  its debug PNG has SHA-256
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`,
  and all direct render runs reported zero high-severity GL messages. Agent
  inspection found a matching normal cube and explanatory wireframe; this is
  not an owner verdict. The manifest and controlled mismatch packet passed
  independent file/hash validation. No golden was created or promoted.
- **2026-08-15 — Accepted Tracer 0 visual baseline:** The owner launched the
  native Windows interactive cube, verified the resizable window, reviewed the
  normal and wireframe captures together, reported that both looked correct,
  and explicitly selected Accept. The complete packet was promoted to the
  checked-in
  [`voxel_cube_smoke-v1` baseline](tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md).
  Its CTest guard validates all manifest-linked file hashes and requires exactly
  one Accept verdict with an owner observation/date. WSL development,
  ASan/UBSan, and release presets each passed 8/8 tests after promotion. A fresh
  native Windows MSVC 19.44.35228.0 copy-build passed 15/15 tests and reproduced
  the accepted normal/debug hashes with zero high-severity GL messages. This is
  a reference-machine engineering-tracer baseline, not a cross-GPU identity,
  gameplay, motion, performance, or production-art claim.
- **2026-08-15 — Minimal Linux CI:** The new
  [Linux fast gate](.github/workflows/linux.yml) targets GitHub's Ubuntu 24.04
  image, selects Clang 18, installs only the current X11/OpenGL build
  dependencies, and runs the documented `cmake --preset dev`,
  `cmake --build --preset dev`, and `ctest --preset dev` sequence. Checkout and
  failure-upload actions are pinned to their v7.0.1 commit SHAs; repository
  permission is read-only and checkout credentials are not persisted. A fresh
  WSL source copy passed 8/8 tests, and a copied-baseline failure confirmed the
  selected CTest diagnostics. The workflow passed checksum-verified
  `actionlint` 1.7.12. A clean staged-index export preserved every accepted
  packet byte and passed all 8 tests after [`.gitattributes`](.gitattributes)
  disabled text conversion under `tests/goldens/`. The resulting commit
  `fcb7d70f64ca3b1c37ffc1caf64670072a5070ca` passed its
  [GitHub-hosted run](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894480357)
  on Ubuntu 24.04 with Clang 18: configure, build, and all 8 tests succeeded in
  1 minute 10 seconds, including 4 `headless` tests. A temporary revision then
  changed only the expected accepted-manifest hash; its
  [hosted run](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894689894)
  failed only `wide_eye.accepted_tracer0_baseline` and successfully uploaded the
  selected failure bundle. Downloaded inspection found all five workflow logs,
  `CMakeConfigureLog.yaml`, `LastTest.log`, and `LastTestsFailed.log`; the named
  hash mismatch was actionable, and a targeted credential-pattern scan returned
  no matches. Native Linux OpenGL 4.6 remains unverified.
- **2026-08-15 — Clean committed-tree sanitizer gate:** A `git archive` export
  of commit `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f` contained no repository
  metadata, prior build, or artifact tree. On WSL Ubuntu 24.04.4, Clang 18.1.3
  configured `dev-sanitized` with strict warnings and ASan/UBSan, completed all
  244 build steps, and passed all 8 CTests; 7 were labeled `sanitizer` and 4
  `headless`. Generated project compile/link rules contain the expected
  sanitizer flags, and a scan of the retained logs found no project failure
  marker or ASan, LSan, or UBSan diagnostic. The source changes since the
  passing hosted `dev` revision are documentation only, so these two runs close
  the clean-tree preset gate. The ignored
  [`verification packet`](artifacts/phase1/2026-08-15/linux-clean-dev-sanitized-b4d5d5c/manifest.json)
  preserves the source-archive hash, commands, configuration, flags, tests, and
  hashed logs. This WSL result does not exercise the OpenGL 4.6 path or establish
  native Linux support.
- **2026-08-15 — Native Windows sanitizer graphics gate:** The Windows runner
  gained an explicit `dev-sanitized` evidence mode without changing the accepted
  visual baseline or the existing development review flow. On Windows 11 with
  Intel UHD Graphics 630, MSVC 19.44.35228.0 applied AddressSanitizer to all six
  project compile commands and the executable link, completed the source-hashed
  copy-build, and passed 15/15 CTests; 14 carried `sanitizer` and 11 `headless`.
  The direct triangle, cube, two normal captures, and wireframe capture each
  reported clean shutdown and zero high-severity GL messages. Both
  normal captures matched the accepted SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`;
  the debug capture matched accepted SHA-256
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  The runner's diagnostic scan and an independent manifest/file/hash validation
  passed. The ignored
  [`sanitizer packet`](artifacts/phase1/2026-08-15/windows-sanitized-cube-smoke-234237930/manifest.json)
  retains the exact commands, platform/driver data, source hashes, configuration,
  state, log, and captures. SDL was not sanitizer-instrumented, so this closes
  the project-code sanitizer graphics gate rather than claiming full dependency
  instrumentation.
- **2026-08-15 — Independent Phase 1 reproduction:** In a fresh context, an
  agent used only the repository's native Windows setup and smoke-runner
  instructions to run a source-hashed `dev` copy-build from commit
  `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f` with a dirty worktree. MSVC
  19.44.35228.0 built the project, all 15 CTests passed, and the direct OpenGL
  4.6 triangle, cube, repeated normal capture, and wireframe capture paths shut
  down cleanly with zero high-severity messages on Intel UHD Graphics 630. The
  normal and debug PNGs matched the accepted hashes, independent manifest,
  file, hash, and review validation passed, and no reproduction instruction was
  missing or ambiguous. The ignored
  [`reproduction packet`](artifacts/phase1/2026-08-15/windows-cube-smoke-235312691/manifest.json)
  closes the final Phase 1 exit gate; it is verification evidence, not a new
  accepted visual baseline.
- **2026-08-16 — Window lifecycle/scenario ownership split:** The former
  tracer-sized `run_window` implementation was separated into a platform
  lifecycle runtime and named scenario runners. `window_runtime` now owns SDL
  initialization, raw window access, the OpenGL context/debug callback, event
  polling, swaps, fixed-step cadence, failure reporting, and ordered teardown.
  Scenario runners declare bounded/interactive requirements and own renderer
  initialization, framebuffer oracles, optional deterministic capture, and
  release of graphics resources while the context is current. `main` retains
  CLI dispatch plus the pure runtime/window-state smoke checks. On WSL Ubuntu
  24.04.4, the Clang 18.1.3 `dev` and ASan/UBSan targets built and each passed
  8/8 CTests; the project-only `clang-format` and bounded `clang-tidy` 18.1.3
  gates passed. On native Windows 11 with Intel UHD Graphics 630, MSVC
  19.44.35228.0 built a source-hashed copy and passed 15/15 CTests. The direct
  triangle, cube, repeated normal capture, and debug capture paths all reported
  clean shutdown and zero high-severity GL messages. The normal captures stayed
  byte-identical at
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`,
  and the debug capture retained
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  The ignored
  [`verification packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-001038930/manifest.json)
  records the source/worktree state, commands, platform/driver data, logs,
  configuration, state, source hashes, and captures. This was an invisible
  ownership refactor; it did not create or promote a new visual baseline.
- **2026-08-16 — Renderer façade ownership:** The tracer-specific
  `TriangleRenderer` name was retired before chunk rendering could depend on it.
  At that checkpoint, `OpenGlRenderer` owned the current-context OpenGL
  entry-point table, triangle/cube GPU resources, draw submission, and
  framebuffer readback; `scenario_runner` retained scenario selection,
  framebuffer-oracle decisions, and PNG output. Triangle-only method names were
  explicit. No interface was invented for the still-unimplemented chunk mesh,
  and the generated-loader replacement was the next foundation outcome; the
  following entry records its completion. WSL `dev` and ASan/UBSan builds each
  passed 8/8 CTests, formatting and bounded static analysis passed, and a native
  Windows MSVC 19.44.35228.0 copy passed 15/15
  CTests on Intel UHD Graphics 630. The direct paths reported clean shutdown and
  zero high-severity GL messages, and both accepted image hashes were unchanged.
  The ignored
  [`verification packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-001947114/manifest.json)
  passed independent validation. This rename did not create or promote a visual
  baseline.
- **2026-08-16 — Generated OpenGL loader:** glad 2.0.8 at commit
  `73db193f853e2ee079bf3ca8a64aa2eaf6459043` generated the checked-in OpenGL
  4.6 Core loader reproducibly with zero extensions and SDL-owned lookup. CMake
  verifies the generated sources and retained license hashes at configure time;
  a controlled copied-source mutation was rejected with the intended named hash
  mismatch. Platform startup initializes glad once after making the context
  current, and the renderer uses the generated API without a hand-maintained
  table. WSL development and ASan/UBSan suites, formatting, static analysis, and
  the native Windows 15-test graphics/capture matrix passed. Windows reported
  `loaded_gl=4.6`, zero high-severity messages, and unchanged accepted pixels.
  Evidence: [`glad provenance`](third_party/glad/README.md),
  [`WideEyeDependencies.cmake`](cmake/WideEyeDependencies.cmake), and the
  ignored
  [`verification packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-003632556/manifest.json).
- **2026-08-16 — Integer voxel-coordinate semantics:** World voxel cells, chunk
  indices, and local voxel offsets are distinct signed 64-bit triples. A valid
  caller-supplied cubic edge length is positive; floor division maps negative
  cells to a chunk plus local axes in `[0, edge)`. Reverse conversion validates
  every local axis and rejects values outside the signed world range without
  overflowing an intermediate. The size remains undecided pending the 16³/32³
  memory/rebuild comparison. WSL development and ASan/UBSan suites passed 9/9
  CTests, formatting and static analysis passed, and native Windows passed 16/16
  CTests while preserving the accepted tracer hashes. Evidence:
  [`coordinates.hpp`](src/voxel/coordinates.hpp),
  [`voxel_coordinates_tests.cpp`](tests/voxel_coordinates_tests.cpp), and the
  ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-010409722/manifest.json).
- **2026-08-16 — Initial chunk edge:** The isolated comparison selected 16³ for
  the first production chunk storage and rebuild queue. Against the same
  deterministic 32³ field, its modeled fixture footprint was 0.85% larger than
  32³, its interior and representative boundary edits scanned 8× and 4× fewer
  cells, and its WSL/GCC 13.3 release full-rebuild median was 2.1% slower—below
  the predefined 25% defer threshold. This does not establish production RSS,
  meshing, upload, draw-call, or provisional Low-profile performance. WSL
  development, ASan/UBSan, and release suites passed 10/10 CTests; native
  Windows MSVC 19.44.35228.0 passed 17/17 and preserved the accepted images with
  zero high-severity messages. Evidence:
  [`ADR 0002`](docs/decisions/0002-chunk-edge-length.md), the ignored
  [`measurement manifest`](artifacts/phase2/2026-08-16/chunk-size-comparison-wsl-gcc13-release-manifest.json),
  and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-013203819/manifest.json).
- **2026-08-16 — Production chunk storage:** A fixed 16³ `Chunk` now stores one
  byte per material ID and reserves zero for explicit empty space. Safe access
  rejects locals outside `[0, 16)`; changed writes accumulate a conservative
  inclusive dirty AABB, while reads and unchanged writes do not. The unit oracle
  covers empty, full, boundary, adjacent, and edited chunks. WSL development and
  ASan/UBSan passed 11/11 CTests, formatting and bounded static analysis passed,
  and native Windows MSVC 19.44.35228.0 passed 18/18 while preserving the
  accepted Tracer 0 images with zero high-severity messages. Chunks intentionally
  do not own neighbor invalidation; that responsibility must be defined with the
  first mesher/rebuild boundary. Evidence: [`chunk.hpp`](src/voxel/chunk.hpp),
  [`chunk_storage_tests.cpp`](tests/chunk_storage_tests.cpp), and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-110914010/manifest.json).
- **2026-08-16 — Naive exposed-face mesher:** The `voxel` boundary now produces
  deterministic CPU vertex/index data with one duplicated quad for each
  non-empty cell face adjacent to empty space. Each quad carries its material,
  cardinal normal, exact outward winding, and stable local position. A
  caller-owned six-chunk neighborhood supplies border samples; missing chunks
  are empty for this bounded-world baseline. The future world/rebuild queue owns
  neighborhood lifetime and must invalidate both sides after a border edit or
  chunk load/unload. WSL development and ASan/UBSan suites passed 12/12 CTests,
  and formatting plus bounded static analysis passed. Native Windows MSVC
  19.44.35228.0 passed 19/19 CTests, reported zero high-severity GL messages,
  and retained both accepted Tracer 0 capture hashes. The ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-112151833/manifest.json)
  passed independent manifest/file/hash/review-linkage validation. Render-pass
  classification, explicit capacity/overflow rejection, GPU upload, drawing,
  rebuild-queue implementation, and greedy meshing remain unimplemented.
  Evidence: [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp) and
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp).
- **2026-08-16 — Mesher pass separation:** A caller-owned 256-entry material
  table now classifies mesh output as opaque, cutout, or translucent, defaulting
  every ID to opaque so existing callers and geometry retain the verified
  baseline. The naive mesher returns three independent vertex/index buffers and
  still lets any non-empty neighbor occlude a shared face regardless of pass.
  The unit fixture verifies 5/5/6 faces for adjacent opaque/cutout plus isolated
  translucent cells and validates each output's topology and material. WSL
  development and ASan/UBSan suites passed 12/12 CTests; formatting and bounded
  static analysis passed. Native Windows MSVC 19.44.35228.0 passed 19/19 CTests,
  reported zero high-severity GL messages, and retained both accepted Tracer 0
  capture hashes. The ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-113523193/manifest.json)
  passed independent manifest/file/hash/review-linkage validation. Explicit
  capacity/overflow rejection, GPU upload, drawing, rebuild-queue
  implementation, and greedy meshing remain unimplemented. Evidence:
  [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp) and
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp).
- **2026-08-16 — Bounded mesher counts:** The fixed 16³ naive build now declares
  conservative output ceilings of 24,576 faces, 98,304 vertices, and 147,456
  indices. It performs one read-only count/classification pass, validates
  arithmetic, `uint32` indexing, vector capacity, and caller-supplied aggregate
  limits before allocation, then reserves exact per-pass storage and emits in
  the same traversal order. Rejection returns a typed error without partial
  buffers. The focused oracle filled one checkerboard parity, observed 2,048
  occupied cells and 12,288 exposed faces, accepted exact 49,152-vertex and
  73,728-index limits, and rejected either limit when reduced by one. WSL
  development and ASan/UBSan suites passed 12/12 CTests; formatting and bounded
  static analysis passed. Native Windows MSVC 19.44.35228.0 passed 19/19 CTests,
  reported zero high-severity OpenGL messages, and retained both accepted Tracer
  0 capture hashes. The ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-120029373/manifest.json)
  passed its runner validation. GPU upload, chunk drawing, rebuild-queue
  implementation, and greedy meshing remain unimplemented. Evidence:
  [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp) and
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp).
- **2026-08-16 — Handcrafted paddock render:** A static 32×16×32 Tracer 1
  blockout now uses four production chunks and six opaque material IDs for
  ground, stone wall and raised posts, a centered red gate, barn walls, a
  stepped roof, and a contrasting barn door. Each chunk is meshed against its
  live axial neighbors; only complete checked outputs are offset into the
  aggregate world-space mesh. The resulting 1,746 occupied blocks emitted 2,754
  faces, 11,016 vertices, and 16,524 indices. `OpenGlRenderer` validates and
  uploads that indexed opaque mesh once, applies fixed-camera projection, flat
  material colors, per-face tint, depth testing, and back-face culling, and
  draws it for both the default interactive window and `--paddock-smoke`.
  Focused material/topology tests and the full WSL development and ASan/UBSan
  suites passed 13/13 CTests; project formatting and bounded static analysis
  passed. Native Windows 11/MSVC 19.44.35228.0 passed 22/22 CTests on Intel UHD
  Graphics 630, including a red-gate center/depth oracle and byte-identical
  repeated capture, while reporting zero high-severity GL messages and retaining
  both accepted Tracer 0 image hashes. Two retained 960×540 frames matched
  SHA-256 `173238274346f39ce3a5fae87e2524e515cb65636302e0e2c3541cf0eaec92d2`;
  agent inspection found all four required scene landmarks distinct. WSL could
  not create the required OpenGL 4.6 context and stopped at `context_create`
  before scene initialization. The ignored
  [candidate review](artifacts/phase2/2026-08-16/handcrafted-paddock/windows-intel-uhd-630/review.md)
  received an explicit owner Accept verdict on 2026-08-16. Its exact normal
  frame is now the checked-in
  [first Tracer 1 baseline](tests/goldens/tracer1/handcrafted_paddock-v1/windows-intel-uhd-630-development-blockout/review.md),
  with a registered integrity test pinning the manifest, review, frame,
  scenario/profile, scene metrics, and verdict. Post-promotion WSL development
  and ASan/UBSan suites each passed 14/14 CTests. Lighting, fog, shadows,
  rebuild/upload queues, debug views, performance measurements, and gameplay
  remain unimplemented. Evidence:
  [`handcrafted_paddock.cpp`](src/voxel/handcrafted_paddock.cpp),
  [`handcrafted_paddock_tests.cpp`](tests/handcrafted_paddock_tests.cpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp), and the ignored
  [native packet](artifacts/phase1/2026-08-16/windows-cube-smoke-123305811/manifest.json).
- **2026-08-16 — Bounded presentation and mesh diagnostics:** The accepted
  handcrafted geometry and fixed camera were preserved while material color
  ownership moved to a bounded voxel palette. `OpenGlRenderer` now supplies a
  fixed directional light, deliberate sky and distance fog, and a static
  1024x1024 filtered shadow map for the unchanged scene. Four named diagnostic
  paths render complete chunk bounds, per-face normal colors and vectors, the
  actual indexed wireframe, or a mesh-stat chart and exact text metrics. The
  focused palette/material-count oracle, WSL development and ASan/UBSan 14-test
  suites, and native Windows development and AddressSanitizer 27-test matrices
  passed. Both Windows matrices used MSVC 19.44.35228.0 on Intel UHD Graphics
  630 and reported zero high-severity OpenGL messages. Agent-inspected 960x540
  implementation captures under
  `artifacts/phase2/2026-08-16/presentation-debug/` show all five intended views;
  they are ignored evidence and do not replace the accepted Tracer 1 blockout
  baseline. The next outcome must add named frame-time/memory evidence and an
  explicit owner review. Evidence: [`CMakeLists.txt`](CMakeLists.txt),
  [`handcrafted_paddock.cpp`](src/voxel/handcrafted_paddock.cpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp), and the ignored
  [native development packet](artifacts/phase1/2026-08-16/windows-cube-smoke-132243278/manifest.json).
- **2026-08-16 — Tracer 1 measurement and dog/camera placeholder:** The renderer
  now accepts explicit camera and optional dog render snapshots without owning
  gameplay truth. A fixed-tick kinematic dog uses a separate analytic paddock
  collision field, four versioned seed-zero starting scenarios, and exact
  restart. Gameplay-follow and free-debug cameras live with game behavior; SDL
  keyboard and gamepad events become named action snapshots at the platform
  boundary. Focus/disconnect clearing, stick dead zones, rising presses,
  representative wall/gate non-tunneling, open-gate passage, grounded motion,
  scenario repeatability, restart, and both camera modes have focused tests.
  WSL development and ASan/UBSan suites passed 20/20 CTests, and project-only
  format/static-analysis gates passed. Native Windows Release passed 35/35
  CTests on Intel UHD Graphics 630 with zero high-severity OpenGL messages. The
  ignored
  [`candidate packet`](artifacts/phase2/2026-08-16/tracer1-review-windows-142557466/review.md)
  retains byte-identical normal frames, all four same-camera diagnostics, the
  grounded dog frame, and serialized release measurements. Across 600 sampled
  1920×1080 frames after 120 warmup frames, synchronized p95/p99 were
  2,864,800/5,449,000 ns, GPU p95/p99 were 1,625,223/1,774,623 ns, and current
  plus peak RSS was 104,673,280 bytes. This passes the provisional Low limits on
  an older Intel UHD 630 proxy but does not establish the named Iris Xe target,
  native Linux graphics, physical-controller behavior, or human camera feel.
  The accepted blockout baseline remains unchanged; only the owner may complete
  the candidate verdict.
- **2026-08-16 — Control-mode scope:** Direct third-person dog control is the
  current first-playable experiment. A separate RTS-style option with a
  controllable bird's-eye camera, selected agents, and location/action orders is
  preserved as a deferred product hypothesis, particularly for later large-
  flock evaluation. Its exact input and command semantics are unresolved, and
  neither 1,000-sheep gameplay nor overhead-camera readability is established.
  Evidence: [`WIDE_EYE.md`](docs/game-design/WIDE_EYE.md) and
  [`HERDING_GAMEPLAY.md`](docs/game-design/HERDING_GAMEPLAY.md).
- **2026-08-16 — Accepted third-person controller baseline:** The owner selected free
  mouse orbit, live camera-yaw-relative ground movement, movement-driven dog
  facing, and no automatic recentering. The implementation keeps transient SDL
  mouse/capture ownership in `platform`, deterministic yaw/basis/motor state in
  `game`, analytic collision independent from voxel rendering, and interpolation
  presentation-only. WSL development/ASan/UBSan, formatting/static analysis,
  and native Windows Release/OpenGL gates pass. The owner subsequently reported
  the native keyboard/mouse behavior as good on 2026-08-16, including the
  clarified reversal behavior, and deferred refinement. This accepts the
  control policy while leaving constants provisional; physical-controller
  behavior remains unverified. Evidence: the
  [implementation plan](docs/plans/third-person-dog-controller-and-camera.md)
  and ignored [native packet](artifacts/phase2/2026-08-16/tracer1-review-windows-171645758/review.md).
- **2026-08-16 — Complete paddock face-decision ledger:** The naive mesher now
  exposes a caller-requested diagnostic record for every side of every non-empty
  cell without retaining that ledger in ordinary chunk builds. The record names
  source local/material/direction, wrapped neighbor local/material,
  same/adjacent/missing-chunk provenance, and emitted/culled disposition. The
  handcrafted paddock adds source-chunk identity and reconciles 10,476 records
  to 2,754 emitted world-space quads plus 7,722 culls. Focused tests prove exact
  occupied-side coverage, no duplicates, live source/neighbor material matches,
  representative boundary reasons, and one-to-one emitted-record/mesh topology.
  WSL development and ASan/UBSan suites passed 20/20 CTests; formatting and
  bounded static analysis passed. Native graphics were not rerun, so this closes
  the data/topology exit gate without creating visual or cross-platform graphics
  evidence. The following owner verdict closes the remaining product-review gate.
- **2026-08-16 — Tracer 1 accepted:** The owner explicitly selected Accept for
  the named same-state normal/debug and dog-placeholder packet after the
  deterministic face ledger closed the remaining diagnostic-evidence gap. This
  accepts the four fixed-camera debug views, grounded placeholder dog and facing
  marker, gameplay-camera starting point, prior keyboard/mouse check, and proxy
  performance evidence as sufficient to close Tracer 1. The verdict does not
  replace the existing blockout golden or verify a physical controller, native
  Linux graphics, or the named Iris Xe target. Every Phase 2 exit gate is now
  closed; the first Phase 3 outcome is authoritative fixed-60-Hz gameplay
  cadence independent of rendering.
- **2026-08-16 — Authoritative gameplay cadence:** `GameplaySimulation` now
  owns the authoritative game tick, existing dog controller, and published
  previous/current snapshots behind an update API with no render-frame delta.
  `FixedStepAccumulator` remains the sole scheduler, and both interactive and
  headless dog scenarios use the same simulation owner. The focused cadence
  oracle produced 60 identical ticks/state from 100×10 ms and 10×100 ms render
  partitions. WSL development and ASan/UBSan suites passed 21/21 CTests; format
  and bounded static analysis passed. Native Windows and graphics were not
  rerun, replay/state formats remain unimplemented, and cross-platform
  determinism remains unclaimed.
- **2026-08-16 — Versioned replay/state contract:** The `game` boundary now
  initially gained independent version 1 seed, action-input, replay, and
  dog-only state-dump contracts. Replays bind the existing named
  scenario/version/seed, 60 Hz rate, and one contiguous domain action per tick.
  Structural and simulation compatibility validation completes before mutation
  and returns named errors; canonical compact JSON writers expose replay and
  previous/current published state. At that checkpoint, two fresh simulations
  produced equal state and byte-identical local state dumps from the same
  three-tick dog-only replay. WSL development and ASan/UBSan suites passed 21/21
  CTests; format and bounded static analysis passed. JSON decoding, CLI
  integration, persistent replay fixtures, sheep state, objective results,
  native Windows, and cross-platform identity were unimplemented or unverified;
  the following entry supersedes only the sheep-state limitation. Evidence:
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md),
  [`gameplay_replay.hpp`](src/game/gameplay_replay.hpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- **2026-08-16 — Minimal authoritative five-sheep state:** `GameplaySimulation`
  now publishes exactly five sheep in fixed contiguous previous/current
  buffers. Stable IDs 1–5 carry position, velocity, heading, arousal, explicit
  settled/alert/driven/recovering state, and grounded state. The deliberately
  stationary update pass reads the immutable prior buffer and adds no social
  behavior. Restart and interpolation preserve coherent identity, while an
  allocation-counted 600-tick oracle observed zero heap allocations. Required
  sheep fields advanced the state dump to version 2 instead of reinterpreting
  dog-only version 1. WSL development and ASan/UBSan suites passed 21/21 CTests;
  project formatting and bounded static analysis passed. Native Windows,
  graphics, sheep behavior, and cross-platform text/state identity remain
  unverified. Evidence:
  [`gameplay_simulation.hpp`](src/game/gameplay_simulation.hpp),
  [`gameplay_replay.hpp`](src/game/gameplay_replay.hpp),
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp), and
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md).
- **2026-08-16 — Representative presentation before behavior depth
  (goal/decision):** The owner approved a hybrid Tracer 2 order. The accepted
  paddock remains the environment/render baseline; after the versioned
  replay/state contract and minimal contiguous sheep state, five simple
  procedural sheep proxies will be rendered from published snapshots and the
  complete paddock, dog, sheep, camera, material, shadow, and debug workload will
  be captured and measured before deep social-behavior tuning. Only a named
  failing budget or measured bottleneck authorizes optimization. This early
  packet is intended to prevent environment-only performance work from
  overfitting an unrepresentative scene; it does not approve final animal art or
  move articulated presentation polish out of Phase 4. No new performance or
  visual result is claimed by this sequencing decision.
- **2026-08-16 — Deterministic presentation-motion fixture:** The new version
  1, seed-zero `presentation-motion` scenario advances the five authoritative
  fixture records synchronously around a four-leg square without changing their
  settled behavior or zero arousal. Presentation interpolates the published
  prior/current snapshots and retains stable IDs through the renderer-facing
  pose copy. A bounded render smoke selects tick 61 at alpha 0.5 so a capable
  OpenGL target exercises facing, the existing proxy material/static-shadow
  receiver, and all five draw submissions. WSL development and ASan/UBSan
  suites passed 22/22 CTests; formatting and bounded static analysis passed.
  Native Windows/OpenGL capture, measurements, owner review, and final
  animal/flock behavior remain unverified.
- **2026-08-16 — Five-proxy capture and measurement packet:** A parameterized
  native capture path now selects a presentation tick and normal or face-normal
  view at 1920×1080 while optionally writing the canonical version 2 gameplay
  state. The dedicated five-proxy performance path times immutable snapshot and
  renderer-pose preparation separately from CPU submission, GPU work, and the
  synchronized frame; fixed-update and presentation-preparation allocation
  oracles remain independently counted. Native Windows Release passed 39/39
  CTests on Intel UHD Graphics 630. Repeated tick-61 normal frames and the
  normal/repeat/debug state dumps were byte-identical. Across 600 samples after
  120 warmup frames, synchronized p95/p99 was 3.4986/4.8862 ms, GPU p95/p99 was
  2.367409/2.656996 ms, CPU submission p95/p99 was 0.3615/0.4854 ms,
  snapshot/presentation preparation p95/p99 was 0.0152/0.0262 ms, both
  allocation oracles reported zero, and peak RSS was 104,382,464 bytes. This
  passes the provisional Low comparison on an older proxy, not the named Iris
  Xe device. Agent inspection found the five proxies recognizable across the
  three-frame motion sheet and present in the same-state debug view. On
  2026-08-16, the owner selected **Accept**, making the packet representative
  enough to begin gameplay iteration with its documented limits. It remains an
  ignored packet and is not a promoted final-art, flock-behavior, or golden
  baseline. Evidence: the ignored
  [`candidate packet`](artifacts/phase3/2026-08-16/tracer2-presentation-windows-204608051/review.md).
- **2026-08-16 — Review remediation before social behavior:** The engine now
  owns complete deterministic scenarios above controller-specific configuration,
  and shared game math no longer comes from the dog header. Replay/state wire
  values remain unchanged. Dog collision clears a clipped axis on the first
  contact tick, flock observables validate every authoritative finite field and
  behavior enum, and typed performance budgets enforce the general Low 1 GiB
  cap or Tracer 2's 512 MiB cap as appropriate. Budget failure is a failing
  process and Release CTest requires the exact `yes` marker. The Phase 3 packet
  hashes presets, CMake modules, and third-party build inputs. WSL development,
  Release, and ASan/UBSan suites each passed 24/24 CTests; formatting and bounded
  clang-tidy passed. Native graphics/measurements were not rerun. This closes
  the review findings without checking sheep behavior or changing the accepted
  visual packet. Evidence: [`ADR 0003`](docs/decisions/0003-project-owned-test-harness.md)
  and [`ADR 0004`](docs/decisions/0004-gameplay-scenario-ownership.md).
- **2026-08-16 — Renderer-evidence workflow integration (goal/decision):** The
  owner authorized integration of the rendering-feasibility findings into the
  current and future workflow. The accepted development loop now requires
  proportional feature-owned debug outputs, representative/holdout evidence,
  named pass labels and measurements, reproducible perceptual-comparison
  metadata, conditional offline references, and validation/capture evidence
  when the owning renderer outcome needs them. The reusable visual-review packet
  is graphics-backend-neutral, and Phase 6 contains the corresponding unchecked
  implementation/evidence gates. A separate post-Phase-3 owner decision now
  prevents Vulkan planning from silently becoming implementation scope. This
  documentation decision implements no renderer feature, changes no accepted
  OpenGL result, and does not approve Vulkan migration. Evidence:
  [`DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md),
  [`HUMAN_VISUAL_REVIEW.md`](docs/review/HUMAN_VISUAL_REVIEW.md), and the
  [harness/tool record](docs/AGENT_HARNESS_AND_TOOLS.md), plus the
  [rendering feasibility research](docs/research/opengl-to-vulkan-feasibility.md).
- **2026-08-16 — First sheep social influence (observed result):** The version
  1, seed-zero `sheep-only-separation` scenario now owns an exact-overlap start,
  the authoritative initial sheep buffer, and close-range configuration.
  `GameplaySimulation` rebuilds the accepted spatial grid from the immutable
  prior buffer, applies a linear planar repulsion with a hard acceleration cap,
  resolves exact overlap through an antisymmetric stable-ID direction, and
  publishes next states synchronously. Focused tests proved recovery beyond the
  configured radius, initially out-of-range rejection, per-tick acceleration
  bounds, exact per-ID results after reversed storage, restart, and zero
  allocations across 600 behavior ticks. WSL development, Release, and
  ASan/UBSan configurations each passed 24/24 CTests; formatting and bounded
  clang-tidy passed. This outcome adds no attraction, alignment, damping,
  speed/turn limit, dog pressure, state transition, or presentation change.
- **2026-08-16 — Bounded selected-neighbor attraction (observed result):** The
  new version 1, seed-zero `sheep-only-attraction` scenario enables attraction
  independently from separation and uses the same immutable-prior spatial-grid
  path. It selects at most two nearest neighbors by distance then stable ID,
  pulls toward their prior-position centroid under a 1.5-world-unit/s² cap, and
  publishes exact chosen IDs, selected/candidate counts, and separate
  separation/attraction vectors. Those required observation fields advance the
  canonical state dump to version 3; older version 1/2 dumps are not silently
  reinterpreted. The focused oracle observed IDs 2 and 3 selected from four
  candidates for sheep 1, matched published and applied acceleration, proved
  exact per-ID state/evidence under reversed storage, exact restart, and zero
  allocations across 600 ticks. WSL development, Release, and ASan/UBSan
  configurations each passed 24/24 CTests; formatting and bounded clang-tidy
  passed. Native graphics and performance measurements were not rerun because
  presentation did not change. Alignment, combined-influence acceleration
  bounds, damping, speed/turn limits, dog pressure, and behavior transitions
  remain unimplemented. Evidence:
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md),
  [`gameplay_simulation.cpp`](src/game/gameplay_simulation.cpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- **2026-08-16 — Paired selected-neighbor alignment (observed result):** Two
  version 1, seed-zero scenarios share an identical moving five-sheep fixture
  and isolate the alignment switch. The enabled path selects at most one
  nearest prior-snapshot neighbor through the accepted grid, applies a bounded
  response toward its velocity, and publishes exact selection plus a separate
  alignment vector in canonical state-dump version 4. Polarization after 60
  ticks was `0.824621` off and `0.924042` on; exact vector application,
  reversed-storage identity, restart, and zero allocations across 600 enabled
  ticks also passed. WSL development, Release, and ASan/UBSan configurations
  each passed 24/24 CTests; formatting and bounded clang-tidy passed. Alignment
  is therefore retained provisionally, not declared biologically realistic or
  player-accepted. Native graphics and measurements were not rerun. Evidence:
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md),
  [`gameplay_simulation.cpp`](src/game/gameplay_simulation.cpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- **2026-08-16 — Distance-only dog pressure (observed result):** Two version 1,
  seed-zero scenarios share one dog/five-sheep fixture and isolate the pressure
  switch. Both publish prior-state dog distance and signed sheep-relative
  bearing; the enabled case applies a separate, linear-falloff vector directly
  away from the dog inside its 6-unit radius and 3-unit/s² cap. Required causal
  evidence advanced the canonical state dump to version 5. The focused oracle
  matched exact geometry and applied acceleration, proved prior-dog and reversed-
  storage invariants, exact restart, radius rejection, zero-direction overlap,
  and zero allocations across 600 ticks. WSL development, Release, and
  ASan/UBSan configurations each
  passed 24/24 CTests; formatting and bounded clang-tidy passed. This completes
  only the distance variable: approach velocity, facing, line of sight, terrain,
  temperament, transitions, and player-facing motion evidence remain absent.
  Native graphics and measurements were not rerun. Evidence:
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md),
  [`gameplay_simulation.cpp`](src/game/gameplay_simulation.cpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- **2026-08-21 — Review remediation, roadmap archival, and version
  single-sourcing (observed result):** A code-and-docs review of the
  uncommitted Tracer 2 work produced the corrections recorded in the current
  checkpoint: stale state-dump version claims fixed so prose defers to the
  format contract; `ref/longterm.md` labeled as unverified external ideation;
  the social-grid rebuild hoisted out of an assertion expression;
  configuration validation moved to simulation construction; the behavior pass
  split into named per-term functions with unchanged arithmetic; and the
  Escape capture toggle narrowed to discard only stale mouse deltas. Completed
  Phase 0–2 checklists moved verbatim to
  [`ROADMAP_ARCHIVE.md`](ROADMAP_ARCHIVE.md) so the live roadmap stays small;
  phase headings and exit-gate anchors remain here. The state-dump format
  version now has exactly two authoritative statements:
  [`gameplay_replay.hpp`](src/game/gameplay_replay.hpp) and
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md);
  other documents must link rather than restate it. Phase 3 gained an explicit
  sheep analytic-collision item, and the avoidance item now owns the
  combined-influence acceleration bound with its planned oracle re-derivation.
  WSL development, Release, and ASan/UBSan configurations each passed 24/24
  CTests after these changes; formatting and bounded clang-tidy passed.
