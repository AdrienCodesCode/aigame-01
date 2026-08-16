# Wide Eye C++ voxel-engine roadmap

## Current checkpoint

- **Status:** Phase 1 is complete. The versioned `core`, `platform`, `render`,
  `voxel`, `game`, `tools`, and `tests` boundaries and a minimal C++23
  `wide_eye` executable now exist. The platform window runtime owns SDL 3.4.10
  startup, a resizable window, resize/minimize/focus/close state, presentation,
  clean shutdown, an explicit OpenGL 4.6 Core debug-context request,
  context/driver reporting, and high-severity rejection. Named scenario runners
  own their configuration, render-resource lifetime, framebuffer oracles, and
  optional capture path without receiving the raw SDL window. The `render`
  boundary owns GLSL 4.60 triangle and
  perspective voxel-cube pipelines, a same-camera triangle-wireframe diagnostic,
  color/depth/framebuffer oracles, top-left RGBA8 readback, and a
  dependency-free deterministic PNG writer. The `core` boundary owns a
  steady-clock frame timer, render-cadence-independent 60 Hz fixed-step
  accumulator with a 250 ms clamp, structured logging, and automation-safe
  fatal assertions. On 2026-08-15, the initial source-hashed native Windows
  candidate build with MSVC 19.44.35228.0 passed all 14 then-current development
  CTests on Intel UHD Graphics 630. Separate hidden capture CTests required two
  independent normal runs and two independent wireframe-debug runs to produce
  byte-identical PNGs. The normal capture is a validated 64x64 RGBA8 PNG with
  SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`;
  the matching wireframe capture has SHA-256
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`
  and passed a sparse-frame oracle with 253 visible pixels. Both paths preserved
  the intended depth state and zero high-severity messages. The owner then
  launched the interactive native Windows cube, confirmed that its resizable
  window worked, reviewed both captures, reported that they looked correct, and
  explicitly accepted the packet. The complete source packet is now the first
  checked-in [Tracer 0 visual baseline](tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md).
  Its manifest retains the source/worktree state, source hashes, platform/GPU
  data, commands, configuration, state, log, and exact normal/debug hashes. A
  registered platform-independent CTest requires every retained hash plus
  exactly one recorded Accept verdict. The root [`.gitattributes`](.gitattributes)
  prevents cross-platform text conversion from changing any hash-addressed
  packet byte. After promotion, a fresh source-hashed native Windows copy-build
  passed all 15 development CTests and reproduced both accepted hashes with zero
  high-severity messages; its ignored verification packet is
  [`windows-cube-smoke-223401700`](artifacts/phase1/2026-08-15/windows-cube-smoke-223401700/manifest.json).
  The WSL development, ASan/UBSan, and release builds and their eight default
  fast tests pass. Direct 4.6 context requests still fail as expected with
  `GLXBadFBConfig` because this WSL host exposes only 4.5. System `clang-format`
  and `clang-tidy` 18.1.3 pass the project-only checks. Native Linux graphics
  remains unverified. A controlled repeat-capture mismatch still supplies the
  retained failure-path evidence; it is not a baseline. A minimal
  [GitHub Actions Linux fast gate](.github/workflows/linux.yml) now targets
  Ubuntu 24.04 with Clang 18 and runs the exact checked-in `dev`
  configure/build/test sequence. In a clean export of the staged index on WSL,
  that sequence passed all 8 tests, including 4 `headless` tests and the
  accepted-packet integrity guard. A
  controlled copied-baseline failure then exited nonzero and populated each
  CTest diagnostic selected for CI upload; checksum-verified `actionlint`
  1.7.12 also accepted the workflow. Commit
  `fcb7d70f64ca3b1c37ffc1caf64670072a5070ca` was then pushed to `main`; its
  [hosted Linux fast gate](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894480357)
  completed in 1 minute 10 seconds on GitHub's Ubuntu 24.04 image and passed all
  8 tests, including 4 labeled `headless`. A separate, temporary revision
  changed only the expected accepted-manifest hash. Its
  [controlled hosted run](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894689894)
  failed the named baseline-integrity test, passed the other 7 tests, and
  uploaded the expected dependency, environment, configure, build, and CTest
  diagnostics. GitHub reported the artifact archive as 35,894 bytes; inspection
  of its downloaded contents confirmed the actual-versus-expected hash error and
  found no credential-pattern matches in a targeted scan. The remote probe
  branch was deleted after verification. Commit
  `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f` was then exported with
  `git archive`; the export contained no `.git`, build, or artifact tree. On the
  WSL Ubuntu 24.04.4 development host, Clang 18.1.3 configured the
  `dev-sanitized` preset with strict warnings and ASan/UBSan, all 244 build
  steps passed, and all 8 CTests passed, including 7 labeled `sanitizer` and 4
  labeled `headless`. The retained logs contain no project failure marker or
  ASan, LSan, or UBSan diagnostic, and the generated project compile/link rules
  preserve the intended sanitizer flags. The ignored
  [`linux-clean-dev-sanitized-b4d5d5c`](artifacts/phase1/2026-08-15/linux-clean-dev-sanitized-b4d5d5c/manifest.json)
  packet records the source-archive hash, commands, configuration, toolchain,
  flags, test labels, logs, and their hashes. Together with the source-equivalent
  hosted `dev` proof above, this completes the clean-tree preset gate; it does
  not establish native Linux graphics or sanitizer coverage of the OpenGL 4.6
  render/capture path. The native Windows runner now also has a first-class
  `dev-sanitized` mode. On the same Windows 11/Intel UHD Graphics 630 host, MSVC
  19.44.35228.0 applied AddressSanitizer to all six project compile commands and
  the executable link, built the source-hashed copy, and passed all 15 CTests;
  14 were sanitizer-labeled and 11 headless. The five direct render/capture
  invocations reported clean shutdown, zero high-severity GL messages, and the
  accepted normal/debug hashes. An explicit scan found no project, ASan, LSan,
  UBSan, or nonzero high-severity GL marker. The ignored
  [`windows-sanitized-cube-smoke-234237930`](artifacts/phase1/2026-08-15/windows-sanitized-cube-smoke-234237930/manifest.json)
  packet passed independent manifest/file/hash validation. SDL remains a
  separately built, non-instrumented shared dependency in this configuration. A
  fresh agent then followed only the repository's native Windows setup and smoke
  runner instructions from the dirty, source-hashed working tree at commit
  `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f`. The default `dev` copy-build
  passed all 15 CTests, including 11 labeled `headless`, and the five direct
  triangle/cube/capture invocations shut down cleanly with zero high-severity GL
  messages. The two normal captures were byte-identical and matched accepted
  SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`;
  the debug capture matched accepted SHA-256
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  Independent manifest/file/hash/review validation passed for the ignored
  [`windows-cube-smoke-235312691`](artifacts/phase1/2026-08-15/windows-cube-smoke-235312691/manifest.json)
  packet, and no missing or ambiguous reproduction instruction was observed. On
  2026-08-16, the first Phase 2 foundation outcome split the lifecycle and named
  scenario ownership without changing the CLI or accepted pixels. The final WSL
  `dev` and ASan/UBSan builds each passed all 8 default CTests. A source-hashed
  native Windows MSVC 19.44.35228.0 copy-build passed all 15 CTests on Intel UHD
  Graphics 630; every direct render/capture path shut down cleanly with zero
  high-severity messages, and the repeated normal plus debug captures matched
  the accepted hashes. Evidence: the ignored
  [`Windows lifecycle-split packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-001038930/manifest.json).
  The second Phase 2 foundation outcome then renamed the tracer-specific
  `TriangleRenderer` façade to `OpenGlRenderer` and made its triangle methods
  explicit. The façade now states ownership of current-context OpenGL rendering
  entry points, render resources, draw submission, and framebuffer readback,
  while scenario runners retain oracle decisions and PNG output. WSL `dev` and
  ASan/UBSan builds each passed all 8 CTests; native Windows passed all 15 CTests
  and reproduced both accepted hashes with zero high-severity messages.
  Evidence: the ignored
  [`Windows renderer-rename packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-001947114/manifest.json).
  The third Phase 2 foundation outcome replaced both handwritten OpenGL symbol
  tables with glad 2.0.8 output generated reproducibly for OpenGL 4.6 Core with
  no extensions. The retained generator archive hash, command, source hashes,
  and permissive license terms are recorded beside the generated source. CMake
  rejects any generated-file or retained-license hash mismatch, and a controlled
  copied-source mutation made configure exit nonzero with the expected named
  checksum error. `window_runtime` performs the single glad initialization
  through SDL after making the context current; `OpenGlRenderer` consumes the
  generated API without owning another entry-point table. WSL `dev` and
  ASan/UBSan builds each passed all 8 CTests, and the project-only formatting
  and bounded static-analysis gates passed. Native Windows MSVC 19.44.35228.0
  passed all 15 CTests on Intel UHD Graphics 630, reported `loaded_gl=4.6` on
  every direct path, preserved zero high-severity messages, and reproduced both
  accepted capture hashes. Evidence: the ignored
  [`Windows generated-loader packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-003632556/manifest.json).
  The fourth Phase 2 foundation outcome established distinct signed 64-bit
  world-voxel, chunk, and local-voxel triples. Conversion uses floor division
  so local axes remain non-negative across negative world boundaries, while
  checked recomposition rejects invalid local offsets and signed overflow. The
  positive cubic edge length is caller-supplied, so this outcome did not select
  16 or 32. WSL Clang 18.1.3 development and ASan/UBSan builds each passed all
  9 CTests, and the project-only formatting and bounded static-analysis gates
  passed. Native Windows MSVC 19.44.35228.0 passed all 16 CTests, including the
  new coordinate unit test, and reproduced both accepted capture hashes with
  zero high-severity messages. Evidence: the ignored
  [`Windows coordinate-semantics packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-010409722/manifest.json).
  The fifth Phase 2 foundation outcome then compared 16³ and 32³ without
  introducing production storage or meshing. A deterministic 32³ one-byte
  occupancy fixture verified equivalent full-field occupied/visible results,
  exact modeled memory, and interior/boundary rebuild-proxy work. The selected
  initial production edge is 16: its modeled equal-world footprint was 33,120
  bytes versus 32,840 for 32³ (+0.85%), while it scanned 8× fewer cells for an
  interior edit and 4× fewer for the cross-boundary edit. On the WSL GCC 13.3
  release run, its full-rebuild median was 2.1% slower, below the predefined 25%
  defer threshold. WSL development, ASan/UBSan, and release suites each passed
  all 10 CTests; native Windows MSVC 19.44.35228.0 passed all 17 and preserved
  both accepted capture hashes with zero high-severity messages. Evidence:
  [`ADR 0002`](docs/decisions/0002-chunk-edge-length.md), the ignored
  [`measurement manifest`](artifacts/phase2/2026-08-16/chunk-size-comparison-wsl-gcc13-release-manifest.json),
  and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-013203819/manifest.json).
  The sixth Phase 2 foundation outcome implemented the production 16³ chunk as
  4,096 one-byte material IDs with zero reserved for explicit empty space.
  Bounds-safe reads return no value for invalid locals; writes distinguish
  invalid, unchanged, and changed results. Actual edits conservatively expand an
  inclusive local-space dirty region, and clearing that region preserves cell
  contents. The focused unit oracle covers empty and full chunks, all valid
  corners and invalid axis boundaries, positive and negative adjacent-chunk
  splits, independent neighboring storage, no-op and material-changing edits,
  dirty-region expansion/clear, and editing back to empty. WSL Clang 18.1.3
  development and ASan/UBSan suites each passed all 11 CTests; project-only
  formatting and bounded static analysis passed. Native Windows MSVC
  19.44.35228.0 passed all 18 CTests and preserved both accepted capture hashes
  with zero high-severity OpenGL messages. Evidence:
  [`chunk.hpp`](src/voxel/chunk.hpp),
  [`chunk_storage_tests.cpp`](tests/chunk_storage_tests.cpp), and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-110914010/manifest.json).
  The seventh Phase 2 foundation outcome implemented a deterministic naive CPU
  mesher. Every non-empty cell face adjacent to empty space emits one duplicated
  four-vertex/two-triangle quad with outward winding, a cardinal normal, and its
  material ID; adjacent non-empty materials occlude their shared face. A
  caller-owned read-only snapshot supplies the six axial chunks, and missing
  neighbors are sampled as empty. The future world/rebuild queue owns snapshot
  lifetime and must remesh both sides of a changed shared border or chunk
  load/unload; chunk storage and meshing remain read-only with respect to
  invalidation. WSL Clang 18.1.3 development and ASan/UBSan suites each passed
  all 12 CTests; project-only formatting and bounded static analysis passed.
  Native Windows MSVC 19.44.35228.0 passed all 19 CTests and preserved both
  accepted capture hashes with zero high-severity OpenGL messages. Evidence:
  [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-112151833/manifest.json).
  The eighth Phase 2 foundation outcome added palette-owned material-pass
  classification and independent opaque, cutout, and translucent CPU mesh
  buffers. Value-initialized classification keeps every material opaque, and
  the pass does not change the verified rule that any non-empty neighbor
  occludes the shared face. The focused oracle now also verifies default and
  explicit classification, exact per-pass face counts, cross-pass culling, and
  unchanged topology/material data in every output. WSL development and
  ASan/UBSan suites each passed all 12 CTests; project-only formatting and
  bounded static analysis passed. Native Windows MSVC 19.44.35228.0 passed all
  19 CTests and preserved both accepted capture hashes with zero high-severity
  OpenGL messages. Evidence: [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and the independently
  validated ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-113523193/manifest.json).
  The ninth Phase 2 foundation outcome now gives the fixed 16³ naive mesh a
  conservative ceiling of 24,576 faces, 98,304 vertices, and 147,456 indices.
  A two-pass build counts and classifies every exposed face before allocation,
  rejects arithmetic/type overflow or caller-supplied aggregate limits without
  returning partial buffers, reserves exact storage for each material pass, and
  then emits in the previously verified order. The focused checkerboard oracle
  emitted 12,288 faces from 2,048 isolated cells, accepted exact limits of
  49,152 vertices and 73,728 indices, and rejected each one-less limit. WSL
  development and ASan/UBSan suites each passed all 12 CTests; project-only
  formatting and bounded static analysis passed. Native Windows MSVC
  19.44.35228.0 passed all 19 CTests and preserved both accepted capture hashes
  with zero high-severity OpenGL messages. Evidence:
  [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and the independently
  validated ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-120029373/manifest.json).
- **Current milestone:** Phase 3 — Tracer 2: five sheep and one gate. Phase 2 is
  complete; the repository
  build scaffold, SDL window lifecycle, context/debug gate, triangle and
  depth-tested cube, core timing/logging/assertion primitives, deterministic PNG
  capture, versioned artifact/failure packet, and a documented hidden-window
  reproduction, normal/debug review packet, explicit owner acceptance, and
  protected first visual baseline are complete. The clean-tree development and
  sanitized preset gate, minimal Linux CI definition, local validation, hosted
  known-good run, controlled failure-artifact verification, native Windows
  sanitizer graphics gate, and independent documentation-only reproduction are
  complete. Every Phase 1 exit gate is closed. The first Phase 2 foundation
  boundaries are also complete: `main` dispatches named scenarios,
  `window_runtime` owns SDL/OpenGL lifecycle and presentation, and
  `scenario_runner` owns scenario-specific rendering, oracles, and capture.
  `window_runtime` also performs the one generated OpenGL-loader initialization;
  `OpenGlRenderer` names backend-level resource, drawing, and readback ownership
  without a second symbol table. The `voxel` boundary owns checked integer
  world/chunk/local conversion plus fixed 16³ one-byte material storage with
  explicit empty space, safe local access, and conservative per-chunk dirty
  regions. The coordinate API remains caller-configurable, while production
  storage fixes its selected edge at 16. The naive mesher now produces
  deterministic duplicated quads across local and explicitly sampled
  cross-chunk boundaries, classifying each material through a caller-owned
  snapshot and routing faces into independent opaque, cutout, and translucent
  buffers while preserving the opaque-default baseline. It counts before
  allocation, enforces a fixed conservative ceiling plus caller-supplied
  aggregate limits, and rejects invalid output atomically. The first visible
  Tracer 1 outcome now stores a handcrafted 32×16×32 paddock in four production
  chunks, meshes live cross-chunk boundaries, offsets each complete checked
  opaque output into one world-space mesh, and uploads/draws 2,754 quads through
  an indexed OpenGL path. The default interactive window and the bounded
  `--paddock-smoke` scenario show green ground, a light-stone wall, centered red
  gate, and a distant stepped-roof barn using a fixed blockout camera. Repeated
  native Windows 960×540 captures were byte-identical. On 2026-08-16 the owner
  explicitly accepted that named candidate; its exact normal frame is now the
  checked-in
  [first Tracer 1 visual baseline](tests/goldens/tracer1/handcrafted_paddock-v1/windows-intel-uhd-630-development-blockout/review.md),
  protected by a registered platform-independent hash/verdict CTest.
  Post-promotion WSL development and ASan/UBSan suites each passed 14/14
  CTests. The fixed geometry and camera now also have a voxel-owned seven-entry
  palette, a fixed directional light, deliberate sky and distance fog, and a
  static 1024x1024 filtered shadow map. Named same-camera chunk-bounds,
  face-normal, indexed-wireframe, and mesh-statistics scenarios expose the
  four chunks and exact mesh topology without changing the accepted blockout
  baseline. The `game` boundary now owns a 60 Hz kinematic upright-cylinder dog,
  deterministic named starting scenarios, restart, independent analytic
  paddock collision, a gameplay follow camera, and a movable free-debug camera.
  The `platform` boundary maps SDL keyboard and gamepad events to named actions,
  including disconnect/focus clearing and a tested stick dead zone. The `core`
  boundary supplies nearest-rank duration summaries and current/peak process
  RSS sampling. On 2026-08-16, WSL development and ASan/UBSan suites passed
  20/20 CTests; project formatting and bounded static analysis passed. A native
  Windows Release build passed 35/35 CTests on Intel UHD Graphics 630 with zero
  high-severity OpenGL messages. Its candidate
  [same-state review packet](artifacts/phase2/2026-08-16/tracer1-review-windows-142557466/review.md)
  contains byte-identical repeated normal frames, all four fixed-camera debug
  frames, and a grounded dog frame with a visible facing marker. The serialized
  1920x1080 static-paddock measurement used 120 warmup and 600 sampled frames;
  synchronized frame time was 2,864,800 ns p95 and 5,449,000 ns p99, GPU render
  time was 1,625,223 ns p95 and 1,774,623 ns p99, and current/peak RSS was
  104,673,280 bytes. This passed the provisional Low comparison on this Intel
  UHD 630 proxy, not on the named Iris Xe reference device. Cross-chunk rebuild
  propagation, budgeted or dynamic upload, and cutout/translucent submission
  remain unimplemented. Native Linux graphics and a physical controller remain
  unverified. The accepted Tracer 1 blockout baseline is unchanged. On
  2026-08-16, the owner explicitly accepted the broader same-state packet while
  retaining those limitations. The selected third-
  person correction is implemented: relative mouse input owns gameplay orbit
  yaw/pitch; WASD resolves from live camera yaw on the ground plane; mouse-only
  look does not rotate the
  dog; the world-space planar motor bounds acceleration/deceleration, turns by
  the shortest path, and slows through hard reversals; and previous/current dog
  and camera state share the fixed-step interpolation alpha. Relative input is
  consumed once per fixed tick and cleared across focus/capture transitions.
  Focused tests cover mapping signs at multiple yaws, normalized diagonals,
  mouse delta timing, same-tick held-W steering, orbit/body independence,
  motor/reversal bounds, interpolation, restart/mode isolation, collision, and
  repeated-sequence determinism. WSL Clang 18.1.3 development and ASan/UBSan
  suites each passed 20/20 CTests; format and bounded static analysis passed.
  The source-aligned native Windows Release run passed 35/35 CTests on Intel UHD
  Graphics 630 with zero high-severity messages. Its ignored
  [`171645758` packet](artifacts/phase2/2026-08-16/tracer1-review-windows-171645758/review.md)
  retained byte-identical normal captures and a grounded dog frame; the static
  1920×1080 proxy measurement reported synchronized p95/p99 of 2.273/2.828 ms,
  GPU p95/p99 of 1.040/1.061 ms, and peak RSS of 103,682,048 bytes. These are
  implementation/regression results. The owner subsequently reported the native
  keyboard/mouse behavior as good on 2026-08-16, including the clarified hard-
  reversal expectation, and chose to defer refinement. This accepts the direct-
  control baseline without finalizing tuning or verifying a physical controller.
  The chunk/mesh diagnostic exit gate is closed by a caller-requested deterministic
  face-decision ledger. For every side of every non-empty cell, the voxel
  boundary records source local/material/direction, wrapped neighbor local and
  material, same/adjacent/missing-chunk provenance, and whether the side emitted
  a quad or was culled. The handcrafted paddock associates each record with its
  source chunk and reconciles all 10,476 decisions—2,754 emitted and 7,722
  culled—one-to-one with the world-space opaque mesh. Focused oracles cover
  same-chunk, cross-chunk, stored-empty, and missing-chunk cases, reject duplicate
  or absent occupied-cell sides, and match every emitted record to its rendered
  quad. WSL development and ASan/UBSan suites passed 20/20 CTests; project
  formatting and bounded static analysis passed. No native graphics capture was
  rerun for this CPU diagnostic outcome, and the accepted Tracer 1 blockout
  baseline remains unchanged. The owner's explicit Accept verdict is recorded
  in the
  [`same-state normal/debug packet`](artifacts/phase2/2026-08-16/tracer1-review-windows-142557466/review.md),
  closing every Phase 2 exit gate without claiming physical-controller, native-
  Linux-graphics, or Iris Xe verification.
  The first Phase 3 outcome now gives `game` a platform-independent
  `GameplaySimulation` owner. The platform accumulator remains the only
  render-to-simulation scheduler; the simulation API accepts one domain input
  per tick and exposes no render delta. It advances an authoritative 60 Hz tick,
  owns the existing dog controller, publishes read-only previous/current dog
  snapshots, and supplies read-only interpolation for presentation. Existing
  interactive and headless dog scenarios use this path. A focused oracle drove
  the same tick-indexed input through one second partitioned as either 100 10 ms
  frames or 10 100 ms frames and obtained exactly 60 ticks plus exactly equal
  published state. On 2026-08-16, WSL Clang 18.1.3 development and ASan/UBSan
  suites each passed 21/21 CTests; project formatting and bounded static analysis
  passed. Native Windows and graphics captures were not rerun for this
  platform-independent, nonvisual outcome, and no cross-platform replay claim
  is made.
  The second Phase 3 outcome defined independent version 1 seed, action-input,
  replay, and initial dog-only state-dump contracts around that owner. A typed
  replay binds the 60 Hz rate, named scenario/version/seed, and a complete
  contiguous action for every tick. Validation rejects unsupported versions,
  rate or scenario mismatches, gaps, and invalid normalized values before the
  simulation can be mutated. Canonical compact JSON writers expose the replay
  and published previous/current state without adding file or renderer
  ownership to `game`.
  Two fresh simulations consumed the same three-tick dog-only replay and
  produced equal state plus byte-identical local state dumps. On 2026-08-16,
  WSL Clang 18.1.3 development and ASan/UBSan suites each passed 21/21 CTests;
  project formatting and bounded static analysis passed. JSON decoding, CLI
  flags, checked-in replay fixtures, objective results, native Windows, and
  cross-platform identity remain unimplemented or unverified.
  The third Phase 3 outcome adds exactly five authoritative sheep in a fixed
  contiguous buffer. Stable IDs 1–5 carry position, velocity, heading, arousal,
  explicit behavior state, and grounded state. Each tick derives the next sheep
  buffer from the immutable prior snapshot; this deliberately stationary
  baseline adds no flock behavior. Restart restores both published buffers, and
  presentation interpolation is read-only. The state-dump contract advanced to
  version 2 rather than reinterpreting dog-only version 1, and now emits both
  dog and sheep state. A focused allocation-counted oracle observed zero heap
  allocations across 600 fixed updates. On 2026-08-16, WSL Clang 18.1.3
  development and ASan/UBSan suites each passed 21/21 CTests; project formatting
  and bounded static analysis passed. Native Windows, graphics, sheep behavior,
  and cross-platform text/state identity were not tested by this nonvisual
  outcome.
  The fourth Phase 3 outcome renders five deliberately simple procedural sheep
  proxies in the gameplay paddock. One shared static mesh gives each proxy a
  cream body and tail, dark face, paired ears, and four legs. Every rendered
  pose is copied one-to-one from the interpolated published snapshot's stable
  ID, position, and heading; `game` remains the sole owner of sheep truth, and
  environment-only paddock/debug paths submit zero sheep. A focused CPU oracle
  verifies the five-pose mapping. WSL development and ASan/UBSan suites each
  passed 22/22 CTests; project formatting and bounded static analysis passed.
  A native Windows MSVC 19.44.35228.0 Release build passed 37/37 CTests on Intel
  UHD Graphics 630, including the OpenGL gameplay render path, with zero high-
  severity messages. Two independent 960x540 gameplay-camera captures were
  byte-identical with SHA-256
  `3e5e922def861473fee18a3fef234696c48638cfbbc306daab1afd9d0d2aaa5b`.
  Agent inspection found all five blocky proxies visible in their two-row
  stationary formation with the dog and gate. This is implementation evidence,
  not owner acceptance of final animal art or flock behavior. Evidence:
  [`sheep_proxy.hpp`](src/render/sheep_proxy.hpp),
  [`sheep_proxy_tests.cpp`](tests/sheep_proxy_tests.cpp), and the ignored
  [`native Windows review packet`](artifacts/phase2/2026-08-16/tracer1-review-windows-195609961/review.md).
  The fifth Phase 3 outcome adds version 1, seed-zero `presentation-motion` as
  an explicitly scripted, non-behavior fixture. `GameplaySimulation` advances
  all five sheep synchronously from the immutable prior buffer around a
  four-leg square at 1.5 world units per second while leaving behavior
  `settled` and arousal zero. The renderer still receives only interpolated
  published snapshots. A bounded `--sheep-motion-render-smoke` pre-rolls to
  tick 61 and draws interpolation alpha 0.5 through the existing shared proxy
  material, static shadow receiver, and five-draw submission path. Repeated CPU
  runs produced exactly equal state; focused tests verify all five translations,
  the first interpolated turn, prior/current publication, exact restart, and
  one-to-one renderer pose mapping. WSL development and ASan/UBSan suites each
  passed 22/22 CTests; project formatting and bounded static analysis passed.
  Native Windows/OpenGL execution, capture, performance, memory, and owner
  review remain unverified. The accepted stationary paddock
  baseline was not changed.
  The sixth Phase 3 outcome adds parameterized 1920x1080 capture of the
  presentation fixture, canonical version 2 state-dump output, a same-state
  face-normal debug view, a three-frame motion contact sheet, and a dedicated
  five-proxy measurement path. On 2026-08-16, native Windows Release passed
  39/39 CTests on Intel UHD Graphics 630 with zero high-severity OpenGL
  messages. WSL development and ASan/UBSan suites each passed 22/22 CTests;
  project formatting and bounded static analysis passed. Tick-61 normal
  captures repeated byte-identically at SHA-256
  `81726cfeb5584d33702344d0907c0adf4129e8a74531c85fbbd0d5cfc5922047`;
  the normal, repeat, and debug commands emitted byte-identical canonical state
  dumps at SHA-256
  `8b2921e4a87bc2e8b8b86e08f4f17d8b3a7bf7c9413293b90b03b728ec27a905`.
  Across 600 measured frames after 120 warmup frames, snapshot/presentation
  preparation p95/p99 was 15,200/26,200 ns, CPU submission was
  361,500/485,400 ns, GPU render was 2,367,409/2,656,996 ns, and synchronized
  frame time was 3,498,600/4,886,200 ns. Allocation-counted oracles observed
  zero allocations across 600 fixed updates and 600 snapshot/pose preparations;
  peak RSS was 104,382,464 bytes. This passes the provisional Low comparison on
  the available Intel UHD 630 proxy, not the named Iris Xe target. Agent visual
  inspection found five recognizable proxies moving and turning in the contact
  sheet while the debug frame preserved the authoritative group state. The
  owner explicitly accepted the packet on 2026-08-16 as representative enough
  for gameplay iteration with its recorded limits. The packet remains ignored
  and is not promoted as final art, accepted flock behavior, or a golden
  baseline. Evidence: the ignored
  [`Tracer 2 presentation packet`](artifacts/phase3/2026-08-16/tracer2-presentation-windows-204608051/review.md).
  The seventh Phase 3 outcome adds a pure, allocation-free five-sheep observable
  pass over the published contiguous state. It computes the three-dimensional
  centroid plus ground-plane mean radius, mean member speed, moving-member
  polarization, bounded covariance elongation, per-member and mean nearest-
  neighbor spacing, threshold-connected component count, and total/minimum/
  maximum/mean chosen-neighbor counts. Connectivity distance and the currently
  external chosen-neighbor counts are explicit read-only inputs because social
  selection does not exist yet. Symmetric-cross and separated-collinear
  fixtures establish hand-computable values and reject duplicate IDs,
  non-finite state/thresholds, and impossible neighbor counts. On WSL Ubuntu
  24.04.4 with Clang 18.1.3, development and ASan/UBSan suites each passed
  23/23 CTests; project formatting and bounded static analysis passed. This
  outcome did not implement dog-relative/response timing metrics, a spatial grid,
  social behavior, named flock scenarios, or debug rendering. Evidence:
  [`flock_observables.hpp`](src/game/flock_observables.hpp) and
  [`flock_observables_tests.cpp`](tests/flock_observables_tests.cpp).
  The eighth Phase 3 outcome adds a deterministic, ground-plane uniform spatial
  grid inside `game`. A rebuild copies ID and planar position from caller-owned
  published sheep state into fixed-capacity sorted cell and row ranges, so later
  source mutation cannot change the query snapshot. The caller's output span is
  the explicit neighbor bound; queries inspect occupied cells in the exact
  radius bounds, exclude the subject, filter by exact planar distance, and keep
  the nearest results in distance/ID/source-index order. Invalid cell sizes,
  indexed state fields, duplicate IDs, capacity overflow, subjects, and radii
  produce explicit errors without leaving a built grid after a failed rebuild.
  Hand-authored fixtures cover cell and radius boundaries, negative cells, box
  false positives, reversed storage, bounded truncation, and snapshot-copy
  ownership.
  The direct oracle observed zero allocations across repeated rebuild/query
  cycles and at the fixed 1,000-member capacity-experiment ceiling; this is
  allocation/correctness evidence, not a population performance result or
  product-scope decision. On WSL Ubuntu 24.04.4 with Clang 18.1.3, development
  and ASan/UBSan suites each passed 24/24 CTests; project formatting and bounded
  static analysis passed. Native Windows was not rerun because this outcome is
  headless and platform-independent. This does not add steering forces, dog
  pressure, behavior scenarios, or presentation. Evidence:
  [`sheep_spatial_grid.hpp`](src/game/sheep_spatial_grid.hpp) and
  [`sheep_spatial_grid_tests.cpp`](tests/sheep_spatial_grid_tests.cpp).
- **Next action:** Implement close-range sheep/sheep repulsion as the first
  independently inspectable social influence. Rebuild/query the accepted grid
  from the immutable prior sheep buffer, add a named sheep-only separation
  fixture with overlap and stable-order oracles, and keep attraction, alignment,
  dog pressure, and presentation out of that coherent outcome.
- **Next-context files:** [`AGENTS.md`](AGENTS.md), this checkpoint, the
  [development workflow](docs/DEVELOPMENT_WORKFLOW.md),
  [engine architecture boundary](docs/VOXEL_ENGINE_OPTION.md#architecture-boundary),
  [first-playable design](docs/game-design/WIDE_EYE.md),
  [herding simulation research](docs/research/herding-simulation-and-scale.md),
  [herding simulation plan](docs/plans/herding-simulation-and-scale.md),
  [`src/README.md`](src/README.md),
  [`runtime.hpp`](src/core/runtime.hpp),
  [`dog_controller.hpp`](src/game/dog_controller.hpp),
  [`gameplay_replay.hpp`](src/game/gameplay_replay.hpp),
  [`gameplay_simulation.hpp`](src/game/gameplay_simulation.hpp),
  [`flock_observables.hpp`](src/game/flock_observables.hpp),
  [`sheep_spatial_grid.hpp`](src/game/sheep_spatial_grid.hpp),
  [replay/state format](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md),
  [`gameplay_simulation.cpp`](src/game/gameplay_simulation.cpp),
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp),
  [`flock_observables_tests.cpp`](tests/flock_observables_tests.cpp),
  [`sheep_spatial_grid_tests.cpp`](tests/sheep_spatial_grid_tests.cpp),
  [`sheep_proxy.hpp`](src/render/sheep_proxy.hpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
  [`scenario_runner.cpp`](src/platform/scenario_runner.cpp), and
  [`CMakeLists.txt`](CMakeLists.txt).
- **Last reviewed:** 2026-08-16.
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

### Completed research

- [x] Define the border-collie player fantasy, core loop, and first playtest
  question. Evidence: [`WIDE_EYE.md`](docs/game-design/WIDE_EYE.md).
- [x] Compare the quick Three.js track with a custom native engine track.
  Evidence: [`TECH_STACK.md`](docs/TECH_STACK.md) and
  [`VOXEL_ENGINE_OPTION.md`](docs/VOXEL_ENGINE_OPTION.md).
- [x] Investigate LumenFall's public claims and establish a clean-room boundary.
  Evidence: [`VOXEL_ENGINE_OPTION.md`](docs/VOXEL_ENGINE_OPTION.md).
- [x] Investigate available skills, MCP candidates, official documentation, and
  the current local toolchain. Evidence:
  [`AGENT_HARNESS_AND_TOOLS.md`](docs/AGENT_HARNESS_AND_TOOLS.md).
- [x] Review the GPT transcript and generated reference images, then research
  measured sheep collective behavior, sheep-dog drives, and a scale-safe
  simulation path. Evidence:
  [`herding-simulation-and-scale.md`](docs/research/herding-simulation-and-scale.md).
- [x] Research and adversarially review an agentic development loop, verification
  cadence, visual owner gate, and context/token discipline. Evidence:
  [`agentic-development-workflow.md`](docs/research/agentic-development-workflow.md)
  and its [implementation plan](docs/plans/agentic-development-workflow.md).

### Approved decisions

- [x] Use the custom C++ engine—not the Three.js prototype—as the primary
  implementation track for the next milestone.
- [x] Develop and release for native x86-64 Linux and Windows. WSL2 development
  does not substitute for either native release test.
- [x] Use C++23, CMake presets, Ninja, SDL3, OpenGL 4.6 Core/GLSL 4.60, and
  doctest as the provisional foundation.
- [x] Use procedural-first media: no imported runtime media through Tracer 2,
  with a provenance-approved authored fallback from Tracer 3 when readability,
  accessibility, audio, or animal animation evidence justifies it.
- [x] Use the provisional low/high hardware classes and budgets for resolution,
  frame-time percentiles, memory, startup, and package size recorded in
  [`ADR 0001`](docs/decisions/0001-native-foundation.md).
- [x] Use host package managers for developer tools and immutable,
  checksum-verified CMake inputs for source dependencies; apply the license
  allowlist and review gates in [`ADR 0001`](docs/decisions/0001-native-foundation.md).
- [x] Use goal, relevant context, invariants, non-goals, and done-when evidence
  as the default coherent-outcome contract. Prescribe steps only when the
  process itself is required.
- [x] End each completed coherent outcome with an explicit fresh-chat,
  continue-chat, or compact-then-continue recommendation according to
  [`DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md).

### Workflow foundation

- [x] Define the standardized inspect/change/build/observe/review/preserve loop,
  proportional verification tiers, bug stop-loss, and review checklist.
  Evidence: [`DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md).
- [x] Define candidate artifact metadata, failure evidence, human visual review,
  and the rule that only owner acceptance promotes a golden. Evidence:
  [`HUMAN_VISUAL_REVIEW.md`](docs/review/HUMAN_VISUAL_REVIEW.md).
- [x] Add provisional checked-in `.clang-format` and bounded `.clang-tidy`
  configurations. Their execution remains unverified against engine code until
  the Phase 1 target and compilation database exist.
- [x] Ignore local build trees, tool/cache state, generated captures/profiles,
  and crash evidence while leaving future intentional golden locations
  trackable. Evidence: [`.gitignore`](.gitignore).

### Environment setup

- [x] Install or provide CMake, Ninja, Clang/clangd, GDB, `pkg-config`, SDL3
  development files, OpenGL/Mesa development files, and ccache if approved.
  Evidence: [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- [x] Record exact compiler, CMake, SDL, OpenGL driver/context, and OS versions.
  Evidence: [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- [x] Prove a hardware or software OpenGL context can run under the normal local
  session and the available virtual display (`xvfb-run`) where applicable.
  Observed result: explicit 4.5 Core debug contexts passed on both paths; the
  approved 4.6 request failed on both. Evidence:
  [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md).
- [x] Retain OpenGL 4.6 as the first graphics baseline. WSL's observed 4.5
  ceiling is development-host evidence, not a supported-target failure or
  permission to fall back. Native Windows is the first hardware context gate;
  native Linux follows on an actual Linux installation or machine. Evidence:
  [`ADR 0001`](docs/decisions/0001-native-foundation.md),
  [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md), and
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Record reproducible setup commands without embedding machine secrets.
  Evidence: [`UBUNTU_24_04.md`](docs/setup/UBUNTU_24_04.md) and the
  [`tools/phase0`](tools/phase0/) bootstrap/diagnostic sources.

### Phase 0 exit gate

- [x] All material product/platform/dependency decisions above are approved.
- [x] The native toolchain versions are recorded and a minimal compiler/context
  smoke test passes with the approved OpenGL 4.6 Core baseline. Evidence:
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] The project can proceed without installing an MCP server. The registered
  MCPs are optional inspection/debugging conveniences with CLI fallbacks.

## Phase 1 — Tracer 0: native foundation

### Repository and build

- [x] Create a clear source tree for `core`, `platform`, `render`, `voxel`,
  `game`, `tools`, and `tests` without speculative subsystem internals. Evidence:
  [`src/README.md`](src/README.md), [`src/`](src/), [`tools/`](tools/), and
  [`tests/`](tests/).
- [x] Add `CMakeLists.txt` and checked-in configure/build/test presets for
  development, sanitized development, and release. Evidence:
  [`CMakeLists.txt`](CMakeLists.txt) and [`CMakePresets.json`](CMakePresets.json).
- [x] Generate `compile_commands.json` for clangd. Observed result: the Linux
  development preset generated `build/Linux/dev/compile_commands.json` with the
  Clang 18 C++23 command, and [`.clangd`](.clangd) selects it.
- [x] Add `.gitignore` rules for build trees, captures, profiles, caches, and
  local user presets while preserving intentional golden artifacts. Evidence:
  [`.gitignore`](.gitignore).
- [x] Add a single command or documented preset sequence for configure, build,
  test, and run. Evidence: [README scaffold commands](README.md#native-scaffold-commands).
- [x] Enable strict warnings for project code without imposing them blindly on
  external dependencies. Observed result: target-scoped warnings-as-errors
  passed with Clang 18.1.3, GCC 13.3.0, and MSVC 19.44.35228.0. Evidence:
  [`WideEyeProjectOptions.cmake`](cmake/WideEyeProjectOptions.cmake).
- [x] Add AddressSanitizer and UndefinedBehaviorSanitizer coverage where the
  platform supports them. Observed result: the labeled scaffold test passed
  under Clang 18 ASan/UBSan on WSL and MSVC 19.44 ASan on native Windows on
  2026-08-15. Evidence: [`CMakePresets.json`](CMakePresets.json) and
  [`WideEyeProjectOptions.cmake`](cmake/WideEyeProjectOptions.cmake).
- [x] Validate `.clang-format` and the bounded `.clang-tidy` checks against the
  installed Clang version and real project code; expose one documented command
  without applying either policy to third-party dependencies. Observed result:
  locally provided Ubuntu LLVM 18.1.3 passed the `format-check` and
  `clang-tidy-check` targets against `src/platform/main.cpp` on WSL on
  2026-08-15. Evidence: [README scaffold commands](README.md#native-scaffold-commands)
  and [`WideEyeDeveloperTools.cmake`](cmake/WideEyeDeveloperTools.cmake).
- [x] Label automated CTest coverage as `unit`, `scenario`, `headless`,
  `sanitizer`, or `performance`; record `manual` evidence separately and keep
  the default affected-change suite fast. Observed result: the current process
  test reports `headless`, plus `sanitizer` in the sanitized preset.
- [x] Prevent CTest pass markers from masking project or sanitizer failure
  diagnostics. Observed result: every registered test rejects `failure_stage=`,
  ASan, LSan, and UBSan diagnostics, and a nested fixture proved all four
  rejection paths before the development, ASan/UBSan, release, and 12-test
  native Windows suites passed on 2026-08-15. Evidence:
  [`CMakeLists.txt`](CMakeLists.txt) and
  [`assert-ctest-failure-regex.cmake`](tests/assert-ctest-failure-regex.cmake).

### Executable smoke tracer

- [x] Open and close an SDL3 window cleanly. Observed result: SDL 3.4.10 created,
  pumped, destroyed, and shut down a bounded dummy-driver CTest in development,
  ASan/UBSan, and release presets on WSL Ubuntu; a normal WSLg smoke also passed
  through the X11 video driver. A source-hashed native Windows copy-build with
  MSVC 19.44.35228.0 passed both CTests and the normal Windows-driver smoke on
  2026-08-15. Evidence: [`main.cpp`](src/platform/main.cpp),
  [`CMakeLists.txt`](CMakeLists.txt),
  [`WideEyeDependencies.cmake`](cmake/WideEyeDependencies.cmake), and the
  [Windows smoke runner](tools/phase1/run-window-smoke.ps1).
- [x] Create an explicit OpenGL core context and report vendor, renderer,
  version, and GLSL version. Observed result: on 2026-08-15 the project requested
  OpenGL 4.6 Core without fallback, validated the returned version/profile, and
  reported Intel UHD Graphics 630, OpenGL 4.6, and GLSL 4.60 from a source-hashed
  native Windows MSVC 19.44.35228.0 copy-build. All three Windows development
  CTests and the direct `--context-smoke` passed. The same request failed with
  `GLXBadFBConfig` on the 4.5-limited WSL host, preserving the accepted baseline.
  Evidence: [`main.cpp`](src/platform/main.cpp),
  [`CMakeLists.txt`](CMakeLists.txt),
  [`WINDOWS.md`](docs/setup/WINDOWS.md), and the
  [Windows project runner](tools/phase1/run-window-smoke.ps1).
- [x] Install an OpenGL debug callback and fail tests on high-severity messages.
  Observed result: on 2026-08-15 a source-hashed native Windows MSVC
  19.44.35228.0 copy-build requested and validated a Core debug context,
  installed a synchronous callback, passed a driver-backed regression that
  injected a high-severity message and required smoke failure, then passed the
  normal Intel UHD Graphics 630 context smoke with zero high-severity messages.
  Evidence: [`main.cpp`](src/platform/main.cpp), [`CMakeLists.txt`](CMakeLists.txt),
  [`assert-opengl-high-severity.cmake`](tests/assert-opengl-high-severity.cmake),
  and [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Render a triangle, then one voxel cube with correct depth testing.
  Observed result: on 2026-08-15 a source-hashed native Windows development
  build passed all nine CTests and both direct render smokes on Intel UHD
  Graphics 630. It reported a 24-bit depth/8-bit stencil framebuffer, triangle
  center RGBA `99,127,155,255`, cube center RGBA `229,56,31,255` at depth
  `0.959411`, enabled `LESS` depth testing and depth writes, matching oracles,
  and zero high-severity messages. Evidence:
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
  [`CMakeLists.txt`](CMakeLists.txt), and [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Handle resize, minimized window, focus, and clean shutdown. Observed
  result: the state-transition unit CTest and SDL dummy-driver event-mapping
  smoke cover pixel-size change, minimize/restore, focus loss/gain, close, and
  shutdown. They passed in WSL development, ASan/UBSan, and release presets and
  in the nine-test native Windows development run. Evidence:
  [`window_state.cpp`](src/platform/window_state.cpp),
  [`main.cpp`](src/platform/main.cpp), and [`CMakeLists.txt`](CMakeLists.txt).
- [x] Add monotonic frame timing, fixed simulation timing, logging, and
  assertions. Observed result: the core runtime CTest verified a steady clock,
  exactly 60 ticks for one second partitioned as either 100 10 ms frames or 10
  100 ms frames, and a 250 ms clamp producing 15 ticks. A wrapped death test
  verified the structured fatal-assertion diagnostic and nonzero exit on WSL
  and Windows; runtime and lifecycle paths emit structured logs. Evidence:
  [`runtime.cpp`](src/core/runtime.cpp),
  [`assert-core-assertion.cmake`](tests/assert-core-assertion.cmake), and
  [`CMakeLists.txt`](CMakeLists.txt).
- [x] Save a deterministic PNG frame from a named smoke scenario. Observed
  result: on 2026-08-15 the `voxel_cube_smoke` scenario read back a top-left
  RGBA8 frame before swap and wrote a validated 64x64 non-interlaced PNG on
  native Windows. The retained capture has SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`;
  it was inspected but not promoted to a golden. A platform-independent unit
  CTest also matched the exact known bytes for a 1x1 RGBA8 PNG. Evidence:
  [`png_writer.cpp`](src/render/png_writer.cpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
  [`main.cpp`](src/platform/main.cpp), and [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Run the smoke scenario under `xvfb-run` or another documented headless
  path and retain evidence. Observed result: the source-hashed native Windows
  runner passed the hidden-window capture CTest, which invoked the scenario
  twice and rejected unequal PNG hashes, then retained a direct capture, hash,
  and log under the ignored `artifacts/phase1/2026-08-15/` tree. The Windows
  development suite passed all 12 tests. Evidence:
  [`assert-deterministic-png.cmake`](tests/assert-deterministic-png.cmake),
  [`run-window-smoke.ps1`](tools/phase1/run-window-smoke.ps1), and
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Emit a versioned artifact manifest and preserve command, platform, logs,
  state, capture, and configuration when the smoke scenario fails. Observed
  result: on 2026-08-15, the source-hashed native Windows runner passed all 11
  development CTests and two direct cube captures, emitted a schema-version 1
  packet whose normal PNG retained SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`,
  and passed independent field/file/hash validation. A controlled repeat-capture
  mismatch then exited with code 2 at `capture-repeat-compare`, retained both
  divergent captures, log, parsed state, configuration, source hashes, commands,
  platform metadata, Git/worktree state, and a failure manifest; the independent
  validator accepted that packet. Evidence:
  [`run-window-smoke.ps1`](tools/phase1/run-window-smoke.ps1),
  [`assert-artifact-manifest.cmake`](tests/assert-artifact-manifest.cmake), and
  [`WINDOWS.md`](docs/setup/WINDOWS.md).
- [x] Produce a candidate cube visual-review packet with matching normal/debug
  evidence; do not promote a golden without the owner's explicit verdict.
  Observed result: the source-hashed native Windows runner passed 14 development
  CTests, including independent two-run deterministic checks for the normal and
  wireframe-debug PNGs, then emitted a same-camera packet whose normal/debug
  hashes are
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`
  and `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  The candidate review record and its manifest passed independent validation.
  The owner subsequently launched the interactive cube, confirmed that its
  resizable window worked, reviewed both captures, reported that they looked
  correct, and explicitly accepted them. The resulting checked-in
  [baseline record](tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md)
  retains the full evidence packet, and `wide_eye.accepted_tracer0_baseline`
  verifies its manifest, files, hashes, and single Accept verdict. A fresh
  native Windows run passed all 15 CTests and reproduced both accepted hashes.
  Evidence: [`main.cpp`](src/platform/main.cpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp),
  [`assert-deterministic-png.cmake`](tests/assert-deterministic-png.cmake),
  [`assert-artifact-manifest.cmake`](tests/assert-artifact-manifest.cmake), and
  [`run-window-smoke.ps1`](tools/phase1/run-window-smoke.ps1).
- [x] After the local loop is reliable, add minimal Linux CI using the same
  configure/build/test/headless commands and retain useful failure artifacts.
  Observed result: [`.github/workflows/linux.yml`](.github/workflows/linux.yml)
  configures, builds, and tests the `dev` preset with Clang 18 on Ubuntu 24.04,
  uses read-only repository permission and immutable action commits, and selects
  bounded environment, dependency, configure, build, and CTest diagnostics for
  14-day upload on failure. On 2026-08-15, the exact three commands passed 8/8
  tests, including 4 labeled `headless`, in a fresh WSL copy. Removing the
  copied accepted-review file made the same CTest command exit 8 and populated
  the selected test log, `LastTest.log`, and `LastTestsFailed.log` with the
  named failure. Checksum-verified `actionlint` 1.7.12 passed. This validates
  the source, staged checkout bytes, and local artifact paths. Commit
  `fcb7d70f64ca3b1c37ffc1caf64670072a5070ca` then passed the
  [hosted gate](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894480357)
  in 1 minute 10 seconds with 8/8 tests and 4 `headless` tests. A temporary
  one-character expected-hash probe produced a
  [controlled hosted failure](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894689894):
  7/8 tests passed, the named accepted-baseline guard failed with the intended
  actual-versus-expected hash diagnostic, and the workflow uploaded all selected
  failure files. This does not verify native Linux graphics.

### Phase 1 exit gate

- [x] Development and sanitized presets build and test from a clean tree.
  Observed result: source commit
  `fcb7d70f64ca3b1c37ffc1caf64670072a5070ca` passed the hosted Ubuntu 24.04
  Clang 18 `dev` configure/build/test sequence with 8/8 tests. Commit
  `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f`, whose intervening changes are
  documentation only, was then exported with `git archive` on WSL Ubuntu
  24.04.4 and passed the Clang 18.1.3 `dev-sanitized` configure, 244-step build,
  and 8/8-test sequence with strict warnings plus ASan/UBSan. Seven tests carried
  the `sanitizer` label, four carried `headless`, and the retained logs contain
  no project failure marker or sanitizer diagnostic. Evidence: the
  [hosted `dev` run](https://github.com/AdrienCodesCode/aigame-01/actions/runs/31894480357),
  [`CMakePresets.json`](CMakePresets.json), and the ignored
  [`dev-sanitized` verification packet](artifacts/phase1/2026-08-15/linux-clean-dev-sanitized-b4d5d5c/manifest.json).
- [x] The executable opens, renders, captures, and shuts down without sanitizer
  failures or high-severity GL debug messages. Observed result: on native
  Windows 11 with Intel UHD Graphics 630, MSVC 19.44.35228.0 applied
  AddressSanitizer to all six project compile commands and `wide_eye.exe`, built
  the source-hashed `dev-sanitized` copy, and passed 15/15 CTests (14 labeled
  `sanitizer`, 11 `headless`). Five direct render/capture invocations reported
  clean shutdown and zero high-severity GL messages; the repeated normal and
  debug captures matched the accepted hashes. The final diagnostic scan and
  independent manifest/file/hash validation passed. Evidence: the ignored
  [`native Windows sanitizer packet`](artifacts/phase1/2026-08-15/windows-sanitized-cube-smoke-234237930/manifest.json)
  and the [Windows setup record](docs/setup/WINDOWS.md#observed-windows-host--2026-08-15).
- [x] A future agent can reproduce the smoke capture using only repository
  documentation. Observed result: in a fresh context on 2026-08-15, an agent
  followed `docs/setup/WINDOWS.md` and `tools/phase1/run-window-smoke.ps1`
  without relying on the authoring context. The native Windows 11/MSVC
  19.44.35228.0 `dev` copy-build passed 15/15 CTests (11 `headless`), all five
  direct render/capture paths reported clean shutdown and zero high-severity GL
  messages on Intel UHD Graphics 630, and the repeated normal plus debug PNGs
  matched the accepted hashes. The documented independent validator accepted
  the packet and no missing or ambiguous instruction was observed. Evidence:
  the ignored
  [`independent reproduction packet`](artifacts/phase1/2026-08-15/windows-cube-smoke-235312691/manifest.json)
  and the [Windows setup record](docs/setup/WINDOWS.md#observed-windows-host--2026-08-15).
- [x] The owner accepts, revises, or rejects the Tracer 0 visual packet; only an
  accepted packet becomes the first visual baseline. Observed result: the owner
  explicitly accepted after checking the interactive resizable window and both
  captures; the [accepted packet](tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/review.md)
  is protected by a passing manifest/hash/verdict CTest.
- [x] The minimal Linux presubmit reproduces the already-working local fast gate,
  or its remaining runner/context limitation is explicitly recorded. Observed
  result: the hosted known-good run passed 8/8 tests on Ubuntu 24.04, and a
  controlled failing revision uploaded the expected useful diagnostics. The
  headless gate does not claim native Linux OpenGL 4.6 graphics coverage.

## Phase 2 — Tracer 1: bounded voxel paddock

### Foundation boundaries before world growth

- [x] Split the tracer-sized `run_window` lifecycle from named scenario runners
  before dog, camera, or world behavior expands it; preserve the current event,
  shutdown, and smoke-test evidence. Observed result: on 2026-08-16,
  [`window_runtime`](src/platform/window_runtime.cpp) became the sole owner of
  SDL/OpenGL initialization, event polling, presentation, diagnostics, and
  teardown, while [`scenario_runner`](src/platform/scenario_runner.cpp) took
  scenario configuration, render-resource lifetime, framebuffer oracles, and
  optional capture ownership. The CLI remains unchanged. WSL Clang 18.1.3
  `dev` and ASan/UBSan builds each passed 8/8 CTests, and the project-only format
  and bounded static-analysis gates passed. A native Windows 11/MSVC
  19.44.35228.0 source copy passed 15/15 CTests on Intel UHD Graphics 630,
  preserved zero high-severity GL messages and clean shutdown across the direct
  render/capture paths, and reproduced the accepted normal/debug hashes.
  Evidence: [`CMakeLists.txt`](CMakeLists.txt), [`src/README.md`](src/README.md),
  and the ignored
  [`native Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-001038930/manifest.json).
- [x] Rename or split `TriangleRenderer` so its public ownership matches the
  triangle, cube, capture, and future mesh responsibilities before chunk
  rendering is added. Observed result: on 2026-08-16, the public façade and
  source files became `OpenGlRenderer` and `opengl_renderer.*`; the ambiguous
  `render` and `sample_center` methods became `render_triangle` and
  `sample_triangle_center`. The façade explicitly owns current-context OpenGL
  rendering entry points, render resources, draw submission, and framebuffer
  readback.
  Scenario runners still own pass/fail oracles and PNG output, and no
  speculative chunk-mesh API was introduced. WSL Clang 18.1.3 `dev` and
  ASan/UBSan builds each passed 8/8 CTests; the project-only format and bounded
  static-analysis gates passed. A native Windows 11/MSVC 19.44.35228.0 source
  copy passed 15/15 CTests on Intel UHD Graphics 630. All five direct
  render/capture invocations shut down cleanly with zero high-severity GL
  messages; repeated normal captures retained SHA-256
  `701595f448a9bb0a82e644e42873a6d1a5e119fd0402f0a6cb4e6f308236ac15`
  and the debug capture retained
  `a9bbf89ed449b5d3ffc803239486fd1bb7571a15ab263f1b4bd60cead41107e6`.
  The ignored packet passed independent manifest/file/hash validation. This was
  an invisible ownership rename, so no new visual baseline was promoted.
  Evidence: [`opengl_renderer.hpp`](src/render/opengl_renderer.hpp),
  [`scenario_runner.cpp`](src/platform/scenario_runner.cpp), and the
  [`native Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-001947114/manifest.json).
- [x] Replace the hand-loaded OpenGL entry-point table with the approved pinned,
  checksum-verified generated loader before the renderer consumes a broader GL
  API. Observed result: glad 2.0.8 generated the checked-in OpenGL 4.6 Core
  loader reproducibly with no extensions; CMake verifies all generated sources
  and the retained license before configuration proceeds. The platform runtime
  initializes it once through SDL after making the context current, and the
  renderer no longer owns per-symbol pointers. A controlled generated-source
  mutation was rejected at configure. WSL Clang 18.1.3 `dev` and ASan/UBSan
  builds each passed 8/8 CTests, formatting and bounded static analysis passed,
  and native Windows MSVC 19.44.35228.0 passed 15/15 CTests on Intel UHD
  Graphics 630 with `loaded_gl=4.6`, zero high-severity messages, and unchanged
  accepted capture hashes. Evidence: [`glad provenance`](third_party/glad/README.md),
  [`WideEyeDependencies.cmake`](cmake/WideEyeDependencies.cmake),
  [`window_runtime.cpp`](src/platform/window_runtime.cpp), and the ignored
  [`native Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-003632556/manifest.json).

### Voxel representation

- [x] Define integer world, chunk, and local coordinates with negative-coordinate
  tests. Observed result: distinct signed 64-bit triples convert through a
  caller-supplied positive cubic edge length using floor division. Explicit
  tests cover `-1`, exact negative edges, the cell before an edge, positive
  edges, invalid extents and locals, 1/3/16/32 edge lengths, exact round trips at
  both signed 64-bit endpoints, and checked recomposition underflow/overflow.
  On 2026-08-16, WSL Ubuntu 24.04.4 Clang 18.1.3 `dev` and ASan/UBSan suites
  each passed 9/9 CTests; format and bounded static analysis passed. Native
  Windows 11 MSVC 19.44.35228.0 passed 16/16 CTests and preserved the accepted
  capture hashes with zero high-severity GL messages. Evidence:
  [`coordinates.hpp`](src/voxel/coordinates.hpp),
  [`coordinates.cpp`](src/voxel/coordinates.cpp),
  [`voxel_coordinates_tests.cpp`](tests/voxel_coordinates_tests.cpp), and the
  ignored
  [`native Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-010409722/manifest.json).
- [x] Compare 16³ and 32³ chunk memory/rebuild behavior before selecting a size.
  Observed result: a deterministic 32³ field produced the same 26,211 occupied
  cells and 35,462 visible faces under both partitions. The one-byte occupancy
  plus fixture-control model used 33,120 bytes for eight 16³ chunks and 32,840
  bytes for one 32³ chunk. An interior edit scanned 4,096 versus 32,768 cells;
  the representative cross-boundary edit scanned 8,192 versus 32,768. On the
  WSL Ubuntu 24.04.4/GCC 13.3.0 release run, 21-sample medians were 1,740,255 ns
  versus 1,704,272 ns for the equivalent full rebuild, below the predefined 25%
  regression guard. The initial production edge is therefore 16, with real
  storage/meshing/upload/target-device measurements required before any future
  change. WSL development, ASan/UBSan, and release suites each passed 10/10
  CTests; native Windows MSVC 19.44.35228.0 passed 17/17 and preserved the
  accepted captures. Evidence: [`ADR 0002`](docs/decisions/0002-chunk-edge-length.md),
  [`chunk_size_comparison.cpp`](tests/chunk_size_comparison.cpp), the ignored
  [`measurement manifest`](artifacts/phase2/2026-08-16/chunk-size-comparison-wsl-gcc13-release-manifest.json),
  and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-013203819/manifest.json).
- [x] Implement palette/material IDs and an explicit empty block. Observed
  result: production chunks contain 4,096 one-byte `MaterialId` cells, with ID
  zero reserved for empty space and used by default construction. Evidence:
  [`chunk.hpp`](src/voxel/chunk.hpp).
- [x] Implement safe get/set and dirty-region tracking. Observed result: invalid
  local reads return no value, invalid writes are rejected, unchanged writes do
  not dirty storage, and actual edits expand a clearable inclusive local-space
  dirty region. Evidence: [`chunk.cpp`](src/voxel/chunk.cpp).
- [x] Test empty, full, boundary, adjacent, and edited chunks. Observed result:
  the focused unit executable covers all 4,096 cells of empty and full chunks,
  every corner, each invalid axis boundary, positive/negative adjacent chunk
  splits, independent neighboring storage, and dirty edit/clear behavior. WSL
  development and ASan/UBSan passed 11/11 CTests; native Windows passed 18/18.
  Evidence: [`chunk_storage_tests.cpp`](tests/chunk_storage_tests.cpp) and the
  ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-110914010/manifest.json).

### Meshing and rendering

- [x] Implement a correct naive exposed-face mesher first. Observed result: the
  CPU baseline emits one indexed quad per face bordering empty space, removes
  shared faces between non-empty cells, preserves material IDs, and supplies
  exact outward winding/cardinal normals. The unit oracle covers empty and
  single-cell chunks, different-material adjacency, the 1,536-face shell of a
  full isolated chunk, a fully surrounded chunk, missing and empty neighbors,
  and wrapped sampling across all six chunk borders. WSL development and
  ASan/UBSan passed 12/12 CTests; native Windows passed 19/19. Evidence:
  [`naive_mesher.cpp`](src/voxel/naive_mesher.cpp) and
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp).
- [x] Keep opaque, cutout, and translucent output separate even if only opaque is
  drawn initially. Observed result: a caller-owned `MaterialPassTable` defaults
  every ID to opaque and can classify IDs as cutout or translucent; the mesher
  routes emitted faces into three independent `ChunkMesh` buffers without
  changing non-empty-neighbor occlusion. The focused oracle verifies exact
  opaque/cutout/translucent counts of 5/5/6 for a mixed fixture, cross-pass
  shared-face removal, and topology/material preservation in every output. WSL
  development and ASan/UBSan passed 12/12 CTests; formatting and bounded static
  analysis passed. Native Windows MSVC 19.44.35228.0 passed 19/19 CTests and
  preserved the accepted captures with zero high-severity OpenGL messages.
  Evidence: [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-113523193/manifest.json).
- [x] Bound vertex/index counts and reject overflow. Observed result: the fixed
  16³ build declares conservative maxima of 24,576 faces, 98,304 vertices, and
  147,456 indices, with a compile-time proof that the vertex bound is
  representable by the `uint32` index format. The mesher counts and classifies
  first, checks arithmetic, vector, and caller-supplied aggregate limits before
  allocation, reserves exact per-pass storage, and exposes either complete
  buffers or a checked error. A 2,048-cell checkerboard emitted 12,288 faces,
  accepted exact 49,152-vertex/73,728-index limits, and rejected one-less limits;
  zero limits still accepted an empty chunk. WSL development and ASan/UBSan
  passed 12/12 CTests; formatting and bounded static analysis passed. Native
  Windows MSVC 19.44.35228.0 passed 19/19 CTests and preserved the accepted
  captures with zero high-severity OpenGL messages. Evidence:
  [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and the ignored
  [`Windows packet`](artifacts/phase1/2026-08-16/windows-cube-smoke-120029373/manifest.json).
- [x] Render one handcrafted paddock with ground, stone wall, red gate, and a
  distant barn landmark. Observed result: on 2026-08-16, a deterministic
  32×16×32 blockout stored 1,746 occupied cells across four production 16³
  chunks. Live axial neighborhoods removed shared chunk-border faces; the
  checked aggregate opaque mesh contained 2,754 faces, 11,016 vertices, and
  16,524 indices, while cutout and translucent outputs remained empty. The
  renderer uploaded that mesh once, drew it with depth testing and back-face
  culling, and exposed it through the default interactive path plus a bounded
  `--paddock-smoke` capture. WSL development and ASan/UBSan suites each passed
  13/13 CTests; formatting and bounded static analysis passed. Native Windows
  11/MSVC 19.44.35228.0 passed 22/22 CTests on Intel UHD Graphics 630, including
  the paddock center/depth oracle and repeated capture, with zero high-severity
  OpenGL messages and unchanged accepted Tracer 0 hashes. Two retained 960×540
  frames were byte-identical at SHA-256
  `173238274346f39ce3a5fae87e2524e515cb65636302e0e2c3541cf0eaec92d2`;
  agent inspection found the required ground, wall, centered red gate, and barn
  distinct. The owner explicitly accepted the
  [candidate packet](artifacts/phase2/2026-08-16/handcrafted-paddock/windows-intel-uhd-630/review.md)
  on 2026-08-16, promoting the exact normal frame to the checked-in
  [Tracer 1 baseline](tests/goldens/tracer1/handcrafted_paddock-v1/windows-intel-uhd-630-development-blockout/review.md).
  A registered platform-independent CTest pins its manifest, review, capture,
  scenario/profile, metrics, and sole Accept verdict; post-promotion WSL
  development and ASan/UBSan suites each passed 14/14 CTests. Evidence:
  [`handcrafted_paddock.hpp`](src/voxel/handcrafted_paddock.hpp),
  [`handcrafted_paddock_tests.cpp`](tests/handcrafted_paddock_tests.cpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp), and the ignored
  [native build/test packet](artifacts/phase1/2026-08-16/windows-cube-smoke-123305811/manifest.json).
- [x] Add a small palette, directional light, sky color, fog, and stable basic
  shadows only after geometry is correct. Observed result: the six visible
  paddock materials now resolve through a voxel-owned bounded palette, while
  the unchanged fixed geometry and camera render under a fixed directional
  light, deliberate blue sky, distance fog, and a static 1024x1024 depth shadow
  map with 3x3 filtering. The static scene renders the shadow map once per
  upload. A native Windows 960x540 normal capture showed readable ground, wall,
  red gate, barn, atmospheric separation, and stable wall/barn shadows; it is
  ignored implementation evidence, not a promoted golden. WSL development and
  ASan/UBSan suites each passed 14/14 CTests. Native Windows development and
  AddressSanitizer copy-builds each passed 27/27 CTests on Intel UHD Graphics
  630, including byte-identical repeated normal captures, with zero
  high-severity OpenGL messages. Evidence:
  [`handcrafted_paddock.hpp`](src/voxel/handcrafted_paddock.hpp),
  [`opengl_renderer.cpp`](src/render/opengl_renderer.cpp), and the ignored
  [development packet](artifacts/phase1/2026-08-16/windows-cube-smoke-132243278/manifest.json).
- [x] Add chunk bounds, face-normal, wireframe, and mesh-stat debug views.
  Observed result: four named same-camera scenarios now draw complete cyan
  bounds for all four production chunks; color faces by cardinal normal and
  draw one normal segment for every emitted face; expose the actual indexed
  naive topology in wireframe; or overlay a five-bar mesh-stat chart while
  logging exact chunk, occupied-block, face, vertex, index, and per-material
  face counts. The checked mesh remains 4 chunks, 1,746 occupied blocks, 2,754
  faces, 11,016 vertices, and 16,524 indices. Broad framebuffer oracles passed
  for every debug mode in both native Windows matrices, and agent inspection of
  the ignored 960x540 captures found each intended diagnostic visible and
  aligned with the fixed paddock. Evidence:
  [`scenario_runner.cpp`](src/platform/scenario_runner.cpp),
  [`opengl_renderer.hpp`](src/render/opengl_renderer.hpp),
  [`handcrafted_paddock_tests.cpp`](tests/handcrafted_paddock_tests.cpp), and the
  ignored [sanitized packet](artifacts/phase1/2026-08-16/windows-sanitized-cube-smoke-132406509/manifest.json).
- [x] Capture identical-camera normal/debug frames and frame-time/memory data.
  Observed result: the native Windows Release
  [`tracer1-review-windows-142557466`](artifacts/phase2/2026-08-16/tracer1-review-windows-142557466/review.md)
  packet retained byte-identical 960×540 normal captures plus the four named
  same-camera debug views. Its 1920×1080 static scenario measured 600 frames
  after 120 warmup frames using serialized GPU queries and swap: synchronized
  p95/p99 were 2.865/5.449 ms, GPU p95/p99 were 1.625/1.775 ms, and current/peak
  RSS was 104,673,280 bytes. The provisional Low comparison passed on the Intel
  UHD 630 proxy; the Iris Xe reference target remains unmeasured.

### Dog and camera placeholder

- [x] Add a kinematic placeholder dog with predictable ground contact.
  Evidence: the fixed-tick controller keeps its cylinder base at analytic ground
  height; focused unit/scenario tests and the native rendered dog capture pass.
- [x] Use simple analytic collision separate from voxel render geometry.
  Evidence: [`PaddockCollisionField`](src/game/dog_controller.hpp) owns paddock,
  wall, and gate shapes without querying voxel faces or renderer meshes.
- [x] Add gameplay and free-debug cameras. Evidence: gameplay orbit yaw/pitch is
  independent from dog facing, free-debug state remains isolated, and fixed-tick
  tests cover mouse orbit, movement-basis signs, free-camera movement, toggling,
  restart, angle interpolation, and repeated-sequence determinism.
- [x] Support keyboard/controller input through named actions. Evidence:
  synthetic SDL keyboard/mouse/gamepad tests cover movement, held look rates,
  accumulated transient mouse deltas, sprint, restart, source coexistence,
  dead-zone behavior, one-tick consumption, focus clearing, and disconnect
  clearing. A physical controller remains unverified.
- [x] Add restart and deterministic scenario selection. Evidence: version 1,
  seed 0 `paddock-start`, `wall-contact`, `closed-gate`, and `open-gate`
  scenarios reproduce local fixed-tick state and restart exactly.

### Phase 2 exit gate

- [x] The bounded paddock is visually readable and reproducibly captured.
- [x] Dog movement cannot tunnel through the representative wall/gate tests.
  Observed result: both giant analytic sweeps and 240 fixed-tick runs stop at the
  wall/closed gate, while the corresponding open-gate scenario passes through;
  all tests pass under WSL ASan/UBSan and native Windows Release.
- [x] Chunk/mesh debug data explains every visible face and missing face.
  Observed result: a caller-requested `ChunkFaceDiagnostic` ledger uses the same
  deterministic z/y/x/direction traversal and neighbor sampler as the naive
  mesher. Each non-empty voxel side records its source local coordinate,
  material, direction, wrapped neighbor local/material, same-, adjacent-, or
  missing-chunk provenance, and emitted/culled disposition. The four-chunk
  paddock retains the source chunk for every record and reports exactly 10,476
  decisions: 2,754 emitted quads, 7,722 occupied-neighbor culls, 9,098
  same-chunk samples, 226 adjacent-chunk samples, and 1,152 missing-chunk
  samples. The focused oracle proves exactly six unique records for every
  occupied block and none for empty blocks, matches stored source and neighbor
  materials, reconciles every emitted record in order to one actual world-space
  mesh quad, and pins representative same-chunk, cross-chunk, and missing-world
  cases. Existing same-camera chunk-bounds, face-normal, indexed-wireframe, and
  mesh-stat views remain the visual side of the diagnostic; scenario logs now
  expose the ledger totals and reject an inconsistent ledger at initialization.
  On 2026-08-16, WSL Clang 18.1.3 development and ASan/UBSan suites each passed
  20/20 CTests, and the project-only format and bounded static-analysis gates
  passed. Native OpenGL captures were not rerun for this CPU-only diagnostic
  change, and no accepted visual baseline changed. Evidence:
  [`naive_mesher.hpp`](src/voxel/naive_mesher.hpp),
  [`handcrafted_paddock.hpp`](src/voxel/handcrafted_paddock.hpp),
  [`naive_mesher_tests.cpp`](tests/naive_mesher_tests.cpp), and
  [`handcrafted_paddock_tests.cpp`](tests/handcrafted_paddock_tests.cpp).
- [x] A same-state normal/debug review packet receives an explicit owner verdict.
  Observed result: on 2026-08-16, the owner explicitly selected Accept for the
  named native Windows Release
  [`tracer1-review-windows-142557466`](artifacts/phase2/2026-08-16/tracer1-review-windows-142557466/review.md)
  packet after receiving the normal frame, four same-camera debug views, dog
  placeholder, automated evidence, and known limitations. The verdict accepts
  the diagnostics, grounded dog/facing marker, gameplay-camera starting point,
  previously reviewed keyboard/mouse behavior, and Intel UHD 630 proxy result as
  sufficient for Tracer 1. It explicitly leaves a physical controller, native
  Linux graphics, and the named Iris Xe target unverified. The checked-in
  blockout golden remains the accepted visual baseline and was not replaced.
- [x] Procedural terrain, streaming, LOD, and advanced post-processing remain out
  of scope.

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
  added. Validation rejects unsupported replay versions, rate or scenario
  mismatches, gaps, non-finite values, and out-of-range normalized movement
  before mutation. Canonical compact JSON writers expose the replay plus
  previous/current published state and reject non-finite state. The presentation
  capture CLI can now write state evidence; JSON decoding plus replay/seed input
  paths remain deferred. Evidence:
  [`GAMEPLAY_REPLAY_AND_STATE.md`](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md),
  [`gameplay_replay.hpp`](src/game/gameplay_replay.hpp), and
  [`gameplay_simulation_tests.cpp`](tests/gameplay_simulation_tests.cpp).
- [ ] Add named scenarios for calm gather, nervous sheep, stubborn sheep, split,
  collision, gate success, restart, and recovery.
- [x] Verify the same replay produces the same outcome across repeated local
  runs; record any cross-platform determinism limit honestly. Observed result:
  two fresh simulations consumed the same three-tick typed dog-only replay and
  produced equal authoritative snapshots plus byte-identical canonical state
  dumps on WSL Ubuntu 24.04.4 with Clang 18.1.3. Development and ASan/UBSan
  suites each passed 21/21 CTests. Native Windows and cross-platform identity
  were not tested, and no objective or sheep behavior exists yet. Evidence:
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
- [ ] Implement named, independently inspectable close-range repulsion,
  selected-neighbor attraction, and optional selected-neighbor alignment.
- [ ] Implement dog pressure from distance, approach velocity, facing, line of
  sight, terrain, and temperament.
- [ ] Implement obstacle/drop avoidance and bounded acceleration/turning.
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
- [ ] Compare alignment-on and alignment-off fixtures; retain explicit alignment
  only when measured behavior and legibility justify it.

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
- [ ] Evaluate RenderDoc and optionally RenderDoc MCP at this gate, not earlier.
- [ ] Consider SSAO, improved anti-aliasing, stylized water, volumetric atmosphere,
  PCSS, or reflections one at a time with identical-state evidence.
- [ ] Reject effects that reduce flock readability, temporal stability, or low-
  target performance.

### Phase 6 exit gate

- [ ] Frame-time percentiles, memory, startup, and chunk latency pass on named low
  and high targets.
- [ ] Captures show stable motion and no high-severity GL/debugger findings.
- [ ] Low/high profiles have intentional differences and tested defaults.
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
  provisional C++23/CMake/Ninja/SDL3/OpenGL 4.6/doctest foundation. Evidence:
  [`ADR 0001`](docs/decisions/0001-native-foundation.md).
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
  sources. Evidence: [README scaffold commands](README.md#native-scaffold-commands),
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
