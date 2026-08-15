# Custom C++ voxel-engine option

## Decision summary

On 2026-08-15, the project owner selected the **custom C++ voxel engine** as
Wide Eye's primary implementation track. Engine construction, procedural voxel
worlds, native performance, and low-level rendering are intended outcomes, even
though this is not the cheapest way to answer whether pressure-and-release
herding is fun.

The Three.js plan remains a lower-cost fallback experiment and technical
reference. Do not develop both implementations simultaneously before the C++
herding model has a deterministic test harness. The complete accepted platform,
asset, performance, dependency, and license constraints are in
[`ADR 0001`](decisions/0001-native-foundation.md).

Implementation progress belongs in the repository
[`ROADMAP.md`](../ROADMAP.md). Tool installation, MCP/skill candidates, official
references, and the agent-observable build/replay/capture contract live in
[`AGENT_HARNESS_AND_TOOLS.md`](AGENT_HARNESS_AND_TOOLS.md).

The herding simulation should be portable, data-oriented C++ with minimal
dependencies on rendering. Its rules can then be tested without a GPU and, if
desired, compared with a web prototype.

The engine's useful specialization is **indirect-control simulation**: it should
render broad outdoor spaces and make a locally interacting flock observable,
replayable, and efficient. It does not need a generic editor, scripting language,
general rigid-body world, arbitrary scene graph, prefab system, or universal
navigation mesh before Wide Eye requires one. This is an architectural boundary,
not permission to add other simulated masses to the current game.

## What LumenFall establishes—and what it does not

The [LumenFall Reddit post](https://www.reddit.com/r/aigamedev/comments/1uozuxz/lumenfall_a_voxel_world_on_an_engine_i_wrote_from/)
describes a C++ and raw-OpenGL voxel engine with procedural terrain, biomes,
caves, settlements, creatures, advanced rendering effects, chunk streaming, and
level of detail. The author also describes directing and testing while AI models
performed most coding.

That is useful evidence that this workflow can produce an ambitious native
prototype. It is not a reusable engine reference:

- The public [LumenFall repository](https://github.com/vengeful180-dot/LumenFall)
  contains a README and Windows executable, not source code.
- The repository has no visible license granting reuse.
- The [v0.22 release](https://github.com/vengeful180-dot/LumenFall/releases/tag/v0.22)
  is also a Windows executable, so the architecture and “no libraries” claim
  cannot be audited from source.
- The author reports performance on an RX 7900 XT and notes in comments that the
  most distant configuration has substantial memory requirements. That is not a
  representative minimum specification.
- The author also calls the NPCs buggy in comments. Renderer sophistication does
  not establish that the game simulation or player loop is mature.

The one-week LumenFall report and the "days to two weeks" first-slice estimate in
[`ref/gpt-chat.md`](../ref/gpt-chat.md) are unverified schedule claims. They do
not set a milestone date, demonstrate maintainability, or establish how long the
Wide Eye behavior and playtest gates will take.

Do not download or run the executable as part of development. Build our engine
clean-room from public specifications and our own design. Cite LumenFall only as
inspiration and an unverified external implementation claim.

## Is C++ the best language for a game engine?

C++ is an excellent fit for a native voxel engine, but there is no universally
best engine language.

It is strong here because it offers:

- Direct control over memory layout, allocations, threading, and graphics APIs.
- Mature compiler, debugger, profiler, build, and platform ecosystems.
- Predictable native performance for meshing, culling, streaming, and simulation.
- A large body of graphics and game-engine literature.

Its costs are real:

- Undefined behavior and lifetime bugs can be severe.
- Build systems and platform APIs consume time that does not improve the game.
- Iteration is slower than browser scripting or an editor-led engine.
- Rendering, asset import, animation, audio, UI, input, and tooling all become our
  responsibility.

Use modern C++ deliberately: value types and RAII, explicit ownership, bounds-
checked spans at boundaries, sanitizers, warnings-as-errors in CI, and no custom
allocator until profiling proves one is needed.

## “From scratch” should not mean “needlessly isolated”

The engine should own its simulation, voxel storage, meshing, renderer,
streaming, tools, and game architecture. That still permits small, well-understood
platform dependencies.

Recommended foundation:

| Area | Initial choice | Why |
| --- | --- | --- |
| Language | C++23 | Native control with modern library and ownership tools |
| Build | CMake presets and Ninja | Reproducible local and CI builds |
| Platform | SDL3 | Window, input, controllers, audio, and portable GL context |
| Graphics | OpenGL 4.6 Core and GLSL | Inspectable, sufficient for the style, simpler than a Vulkan-first engine |
| Tests | doctest plus the engine's scenario harness | Small unit tests and headless simulation evidence |
| Debug UI | Dear ImGui, optional | High-leverage tuning and visualization, never required at runtime |
| Assets | Code-generated media through Tracer 2; small provenance-approved authored fallback later | Procedural identity without sacrificing animal or UI readability |

If a one-megabyte, Windows-only executable with no runtime dependencies becomes a
real product constraint, replace SDL behind a narrow platform interface later.
Making raw Win32/WGL the first milestone would test platform plumbing instead of
herding or voxel rendering.

## Art direction options

The FarmRise example's Blender pipeline is not an art-direction requirement.
Blender is a tool for authoring, rigging, and export; it does not imply realistic,
low-poly, pixel, or voxel art.

Viable directions include:

- **2D sprites or pixel art:** fastest for expressive poses, but awkward for a
  freely rotating 3D herding field unless many directional views are authored.
- **Low-poly 3D:** excellent silhouettes and animation, with a conventional mesh
  and rig pipeline.
- **Pure voxel:** visually distinctive and ideal for procedural terrain, but a
  rigid block dog can make gaze, footwork, ears, tail, and sheep stress harder to
  read.
- **Voxel/low-poly hybrid — recommended:** a voxel landscape, walls, gates,
  barns, vegetation, and props with articulated voxel-built or low-poly skinned
  animals. Palette, lighting, and scale unify the two.

For **Wide Eye**, animal animation is gameplay information. The dog needs a
readable facing direction, crouch, acceleration, stop, eye/head direction, ears,
and tail. Sheep need readable facing, bunching, hesitation, alarm, and settling.
Protect those signals even if that means breaking strict voxel purity.

The two generated images in `ref/` are mood/composition references for landscape
depth, silhouettes, and possible camera distances only. Their HUD, commands,
score, inventory, minimap, and task structure are not requirements. See the
[herding gameplay direction](game-design/HERDING_GAMEPLAY.md).

## Architecture boundary

```text
platform + input + audio
           |
core time, jobs, logging, math, resources
           |
  +--------+---------+
  |                  |
voxel world       renderer
  |                  |
  +------ game ------+
       simulation
           |
 replay, tests, debug views, optional saves
```

The renderer consumes snapshots; it does not own gameplay truth. The fixed-tick
simulation does not query render-frame timing. Debug and replay tools observe the
same public state used by the presentation layer.

Suggested modules:

- `platform`: lifecycle, window, input, controller, audio device, file paths.
- `core`: time, math, logging, assertions, handles, jobs, memory accounting.
- `render`: GL resources, shaders, frame graph, cameras, culling, lighting.
- `voxel`: chunk storage, palettes, generation, edits, meshing, streaming.
- `game`: dog controller, pressure field, sheep behavior, objectives, scoring.
- `animation`: authored clips and small procedural layers.
- `presentation`: particles, audio cues, HUD, menus, accessibility options.
- `tools`: tuning panels, profilers, inspectors, capture, and replay controls.
- `tests`: headless simulation, math, serialization, generation, and regression.

Avoid a general-purpose entity-component system initially. A few explicit
collections—dogs, sheep, gates, obstacles, and chunks—will be easier to reason
about until real composition pressure appears.

## Voxel world and renderer

Start with a bounded handcrafted paddock stored in the same voxel structures that
later procedural terrain will use.

Initial representation:

- Integer world and chunk coordinates.
- Palette/material IDs rather than a large texture per block.
- Fixed-size chunks; begin by testing 16³ and 32³ rather than assuming one.
- Solid, cutout, and translucent render passes kept separate.
- Greedy meshing only after a naive face mesher is verified.
- Chunk rebuild queues with explicit time and memory budgets.
- A height/slope query for animal locomotion independent of triangle collision.

Rendering order of investment:

1. Correct depth, camera, palette, normals, and debug overlays.
2. Directional light, stable basic shadows, fog, and sky color.
3. Frustum culling and measured mesh/upload budgets.
4. Stylized water and vegetation only if the slice needs them.
5. Chunk streaming and worker-thread meshing once a bounded world works.
6. Level of detail only after profiling identifies draw distance as the real
   constraint.
7. SSAO, temporal AA, volumetric clouds, PCSS, reflections, and god rays only
   after gameplay readability and target frame time pass.

Do not begin by reproducing LumenFall's effects list. A coherent palette, stable
shadows, readable silhouettes, and good motion will do more for the first game.

## Herding simulation

Run game rules at a fixed tick, initially 60 Hz. Make inputs and random seeds
recordable so a failure can be replayed exactly.

Each sheep uses stable identity, explicit behavioral state, and bounded, named
influences computed from an immutable prior snapshot:

- Separation from nearby sheep.
- Attraction toward a limited, selected set of local sheep.
- Optional alignment with a smaller selected subset, retained only if scenario
  evidence justifies it.
- Pressure away from the dog based on distance, approach velocity, facing, line
  of sight, terrain, and temperament.
- Obstacle and drop avoidance from a navigable height/slope field.
- Settled, alert, driven, and recovering transitions, with later grazing or
  lamb/adult behavior added only when a scenario requires it.
- An inspectable arousal/recovery proxy that may rise under crowding, isolation,
  speed, and abrupt turns, then fall when pressure is released.

Use a uniform grid for neighbors and dog pressure, contiguous hot state, stable
iteration, synchronous publication, and no steady-state per-agent allocation.
Keep behavior forces, selected neighbors, state, and group observables visible in
a debug overlay and state dump. Cap acceleration and turning; do not hide
instability with arbitrary randomness.

Generic boids are a diagnostic starting point, not a realism claim. The
[research synthesis](research/herding-simulation-and-scale.md) covers measured
sheep/dog behavior, and the
[implementation plan](plans/herding-simulation-and-scale.md) defines the
correctness, calibration, and population ladders.

The dog is a kinematic character in the first slice. It needs predictable ground
contact and collision, not a full rigid-body character controller. Gates and
fences can use simple analytic shapes even when their render meshes are detailed.

## Incremental build plan

### Tracer 0 — Native foundation

- Open a window, handle clean shutdown, render a triangle and one voxel cube.
- Establish CMake presets, warnings, sanitizers, unit tests, logging, and frame
  timing.
- Pass on one declared development machine before adding architecture.

### Tracer 1 — One paddock

- Render one bounded voxel paddock with a gate and simple light.
- Add a free debug camera and a kinematic dog capsule/cylinder.
- Visualize collision, coordinates, and frame time.

### Tracer 2 — The actual game question

- Add five placeholder sheep, three temperaments, a pressure debug field, one
  gate objective, restart, success, and recoverable failure.
- Record inputs and seeds; run headless behavior tests and emit group
  observables.
- Build data layout and bounded-neighbor queries that can run 14-, 25-, and
  100-sheep diagnostic scenarios without making them player content.
- Do not add procedural terrain, chunk streaming, weather, accounts, or advanced
  post-processing.

### Tracer 3 — Readable presentation

- Replace the dog and sheep placeholders with one representative articulated
  style.
- Add locomotion state, facing, ears/tail/head cues, stress feedback, farmer
  whistle, minimal HUD, and essential audio.
- Test the core playtest question with fresh players.

### Tracer 4 — Voxel world depth

- Add deterministic terrain generation, biome palette rules, chunk save/load,
  and a bounded horizon.
- Introduce worker generation and meshing with cancellation and budgets.
- Preserve deterministic replays of the handcrafted challenge.

### Tracer 5 — Scale only from measurements

- Add culling, streaming range, mesh caching, level of detail, and graphical
  profiles in the order profiles identify as necessary.
- Set targets for frame-time percentiles, memory, startup, and chunk latency on
  low and high supported hardware.
- Benchmark the same authoritative behavior at 5, 14, 25, 100, 250, 500, and
  1,000 sheep, separating simulation, terrain, presentation, draw, GPU, and
  memory costs.
- Treat 1,000 as capacity evidence until behavior, readability, performance, and
  playtests justify making it a gameplay requirement.

### Tracer 6 — Product hardening

- Package clean builds, licenses, crash diagnostics, save recovery, settings,
  accessibility, input remapping, and regression captures.
- Add distribution, telemetry, or a backend only when product requirements earn
  them.

## First go/no-go gate

Continue investing in the custom engine only when all are true:

- The Tracer 2 executable starts reliably on the supported development target.
- Five sheep can be driven through one gate using the intended pressure model.
- A recorded run replays to the same outcome.
- Debug views explain every surprising flock response.
- Frame time and memory stay within declared budgets with ample headroom.
- Engine work has not prevented a fresh-player test of the core loop.

If these do not pass, simplify the renderer and simulation. Do not compensate by
adding more engine features.

## Current recommendation for Wide Eye

Choose the hybrid voxel look and build the C++ path as a series of playable
tracers. Make the first world deliberately tiny: a sunlit upland paddock, dry-
stone walls, one red gate, one distant barn, five sheep, a farmer silhouette, and
one black-and-white collie whose posture reads from the gameplay camera.

The engine's first impressive feature should be a flock that visibly understands
the dog's pressure—not volumetric clouds.
