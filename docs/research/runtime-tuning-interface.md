# Research: Runtime tuning interface

**Status:** Draft research; not implemented
**Produced by:** Codex
**Date:** 2026-08-22
**Project revision:** `9cc5c7d` with pre-existing worktree changes
**Adversarial review:** Not yet reviewed

## Problem and decision

The owner wants a GUI that can tune game parameters now and expand later into
gameplay, graphics, camera, audio, and other categories. The immediate questions
are whether this would improve iteration and whether changing a value requires a
C++ rebuild.

**Decision to prepare:** whether Wide Eye should add a developer-only runtime
tuning surface, what owns the values behind it, and when each kind of edit takes
effect without weakening deterministic scenarios, replay evidence, platform
input, or release builds.

**Goal:** shorten the edit-observe-compare loop for provisional values while
keeping every accepted result attributable to an exact scenario and parameter
set.

**Success criteria:**

- common numeric, boolean, enum, and colour values can be changed without
  recompiling the executable;
- every control declares whether it applies on the next render frame, the next
  fixed simulation tick, a scenario restart, a resource rebuild, or an
  executable rebuild;
- invalid edits never partially enter the active game or renderer;
- default scenarios, state dumps, replays, captures, and performance runners
  remain reproducible when the tuning UI is disabled;
- a useful exploratory setting can be exported and deliberately promoted into a
  versioned canonical scenario rather than being lost in a developer's machine;
- the UI can add categories without putting gameplay truth in the renderer or
  OpenGL types in game code.

**Non-goals:**

- a player-facing settings menu or its accessibility/localization work;
- a generic reflection/editor framework for every C++ field;
- an arbitrary command console, memory editor, or remote mutation protocol;
- shader-source hot reload, asset authoring, terrain editing, or save-game
  persistence in the first slice;
- automatically declaring an exploratory value to be accepted game design.

This answer matters now because many sheep values are explicitly provisional,
and the approved visual-feasibility plan is about to turn hard-coded projection,
lighting, fog, and shadow values into explicit renderer-facing settings. A
tuning seam can become high leverage at that boundary, but implementing a GUI
before the underlying ownership is explicit would encode the wrong architecture.

## Verified project constraints

### Current engine and milestone

- **Confirmed fact:** the project uses C++23, SDL 3.4.10, OpenGL 4.6 Core/GLSL
  4.60, CMake, and fixed 60 Hz authoritative gameplay on native Windows/Linux.
  Dependency archives must be immutable and checksum-pinned, with permissive
  licensing preferred. See [ADR 0001](../decisions/0001-native-foundation.md).
- **Confirmed fact:** the repository already names Dear ImGui as an optional,
  high-leverage debug UI that must earn a current requirement. This tuning need
  is such a requirement, but the dependency is not yet adopted.
- **Confirmed fact:** the current checkpoint is the owner-authorized bounded
  OpenGL visual-feasibility tracer before the Phase 3 objective loop. Phase 0's
  baseline work remains the first active outcome. See
  [`ROADMAP.md`](../../ROADMAP.md).
- **Confirmed fact:** the approved visual-feasibility plan already requires
  camera range, light, colour, fog, and shadow values to move out of shader-local
  constants into validated renderer-facing settings during its Phase 1. See the
  [visual-feasibility plan](../plans/visual-feasibility-before-objective-loop.md#phase-1--establish-the-bounded-visual-scene-and-render-settings).

### Current parameter ownership

- **Confirmed fact:** sheep separation, attraction, alignment, dog pressure,
  approach, facing, line of sight, temperament, avoidance, combined influence,
  motion limits, and behavior thresholds already have typed configuration
  structs nested in `GameplayScenarioDefinition`. The simulation copies the
  definition at construction and reads that copy during fixed ticks. See
  [`gameplay_scenario.hpp`](../../src/game/gameplay_scenario.hpp) and
  [`gameplay_simulation.cpp`](../../src/game/gameplay_simulation.cpp).
- **Confirmed fact:** the simulation exposes no runtime configuration setter.
  Today, changing those scenario defaults in source requires rebuilding the
  executable and starting a new simulation.
- **Confirmed fact:** dog walk/sprint speed, acceleration/deceleration, turn
  rate, and camera sensitivity/pitch bounds are `static constexpr` class values.
  They are not currently scenario or runtime inputs. See
  [`dog_controller.hpp`](../../src/game/dog_controller.hpp) and
  [`camera_controller.hpp`](../../src/game/camera_controller.hpp).
- **Confirmed fact:** projection, light, sky, fog, and related presentation
  values are repeated as constants inside embedded GLSL strings. They currently
  require a C++ rebuild because the shader source itself is compiled into the
  executable. See [`opengl_renderer.cpp`](../../src/render/opengl_renderer.cpp).
- **Confirmed fact:** the current replay seed identifies only scenario ID,
  scenario version, and seed. Its contract explicitly says that changing
  scenario parameters requires a scenario-version increment. See
  [`gameplay_replay.hpp`](../../src/game/gameplay_replay.hpp).
- **Confirmed fact:** SDL event polling, named game input, relative mouse capture,
  fixed ticks, rendering, and swap all meet in `run_window`. A tuning overlay
  must integrate there without allowing a slider drag or text edit to move the
  dog or camera. See [`window_runtime.cpp`](../../src/platform/window_runtime.cpp).

### Worktree boundary

The worktree was already substantially modified before this research. This file
and one navigation link are the only changes made for this outcome. No current
engine code, dependency, roadmap checkbox, or approved visual plan is changed.

## Findings

### 1. Most tuning does not inherently require recompilation

**Confirmed fact:** OpenGL uniform values are runtime program state and retain
their values until changed or the program is relinked. Buffers and textures are
also runtime resources. A renderer can therefore update ordinary lighting,
fog, exposure, colour, camera, and material parameters without rebuilding the
C++ executable, provided those values are represented as uniforms or data
buffers rather than embedded GLSL constants. See the OpenGL 4.6 reference for
[`glUniform`](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUniform.xhtml)
and the [OpenGL 4.6 specification](https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf),
accessed 2026-08-22.

Gameplay values are ordinary C++ data. Once the fixed-tick owner receives a
validated runtime configuration, it can read new values on a later tick without
recompiling. The present rebuild requirement is an implementation choice—values
live in source or `constexpr` storage—not a C++ or OpenGL limitation.

The useful distinction is the cost and safety of *applying* a change:

| Apply class | Examples | Earliest safe effect | C++ recompile? |
| --- | --- | --- | --- |
| Render-live | sun colour/intensity, fog amount, material roughness-like scalar, debug-view toggle | Next rendered frame | No |
| Simulation-live | sheep influence radii/strengths, behavior rates, dog motor values after they become data | Next fixed 60 Hz tick, through one queued commit | No |
| Restart-required | initial positions, temperament assignment, gate start state, scenario fixture, values whose comparison depends on clean history | After validated scenario restart | No |
| Resource-rebuild | shadow-map resolution, generated mesh density, terrain generation inputs, render-target format | After an explicit transactional resource/scene rebuild | No executable recompile, but resources must be recreated |
| Shader-relink | shader algorithm/source edited outside the executable | After compile/link succeeds and the old program can be retained on failure | No executable recompile if shader-source hot reload exists; it does not exist here |
| Executable-rebuild | new rule code, struct layout, enum cases, templates, fixed capacities, serialization schema code, new render passes | After build/relaunch | Yes |

**Qualified finding:** “live” is not always the best default even when it is
technically possible. Sheep arousal, velocity, neighbor history, and position
already reflect earlier values. Applying a new pressure radius halfway through a
run changes the future immediately but does not rewind that history. For
comparison work, `Apply and restart` from the same state is usually more honest
than an unlabelled live edit.

### 2. The parameter model is more important than the widget library

The GUI must not point directly at scattered globals or arbitrary object memory.
That design is fast for the first slider and expensive for every later category:
there is no central validation, no apply timing, no reset behavior, no stable
identity, and no way to prove which values produced a capture.

**Recommendation:** preserve typed subsystem configurations and add a small,
typed tuning catalog over them. Each exposed parameter needs:

- a stable nonlocalized ID such as `gameplay.sheep.separation.radius`;
- value type (`bool`, integer, scalar, enum, colour/vector when earned);
- category and human label;
- units, range, step, and optional display transform;
- owner (`game`, `render`, `camera`, later `audio`);
- apply class from the table above;
- default, active, and pending values;
- validation that can examine the whole candidate configuration, not only one
  field in isolation.

The catalog should be an explicit adapter over typed structures, not a
string-to-pointer registry and not a second source of default values. C++ has no
general standardized runtime reflection in the project's current baseline;
trying to manufacture a universal reflection system would be speculative scope.

One bounded data flow is sufficient:

```text
widget edit -> pending typed profile -> whole-profile validation
            -> owner-specific apply queue -> fixed-tick / frame / restart / rebuild boundary
            -> active revision + visible status -> optional session artifact
```

Gameplay and rendering keep separate active configurations and commit points.
The UI observes and requests changes; it does not become the authority that
implements sheep rules or uploads OpenGL resources.

### 3. Determinism needs an explicit exploratory/canonical boundary

**Confirmed fact:** current canonical replay identity does not contain tuning
overrides. Silently mutating values would allow two runs with the same scenario
ID/version/seed/actions to diverge.

The smallest sound policy is:

1. Canonical scenarios remain immutable defaults.
2. Opening or editing the tuning panel creates a clearly marked **noncanonical
   tuning session** with a base scenario, tuning schema version, active revision,
   and parameter diff.
3. Gameplay changes commit only at a named fixed tick, or are staged for restart.
   Renderer changes commit at a frame boundary. Resource changes are
   transactional and retain the old valid resource on failure.
4. Captures and diagnostic output made during tuning include the noncanonical
   marker and exact parameter diff/hash.
5. A promising result is promoted by reviewing the diff, updating the typed
   canonical scenario/default, incrementing the owning scenario or settings
   version when its contract requires it, and rerunning normal evidence. The GUI
   never auto-promotes a setting.

**Inference:** a separate developer tuning-session artifact is safer initially
than immediately expanding the gameplay replay format. It preserves experimental
provenance without making an unapproved tool part of the stable player replay
contract. If exact mid-run tuning-event replay becomes a repeated need, that is a
later evidence-backed format decision.

### 4. Dear ImGui is the proportionate GUI layer

Dear ImGui is designed for C++ tool/debug interfaces, has an MIT license, and
ships standard SDL3 and modern OpenGL backends. Its official integration path
combines one platform backend with one renderer backend, and the repository has
a specific SDL3 + OpenGL3 example. See the official
[backend guide](https://github.com/ocornut/imgui/blob/v1.92.9b/docs/BACKENDS.md),
[SDL3/OpenGL3 example](https://github.com/ocornut/imgui/tree/v1.92.9b/examples/example_sdl3_opengl3),
and [MIT license](https://github.com/ocornut/imgui/blob/v1.92.9b/LICENSE.txt),
accessed 2026-08-22.

**Confirmed fact:** as of 2026-08-22, the newest tagged hotfix is `v1.92.9b` at
commit `f1cc2ae15e53a861a874c3034aae6798fde194ab`. If adopted, the project should
review that exact revision, pin an immutable archive and SHA-256 in CMake, retain
its license/notices, and build only the required core plus SDL3/OpenGL3 backend
sources. This research does not adopt it.

**Qualified finding:** use the ordinary tagged branch for the first panel. The
docking and multi-viewport branch adds no value to a single collapsible tuning
window yet. Likewise, use the upstream SDL3/OpenGL3 backends for the initial
integration rather than writing custom GUI backends.

Input integration is not optional polish. Dear ImGui's official FAQ says events
should always be forwarded to the UI backend, while `WantCaptureMouse` and
`WantCaptureKeyboard` decide whether the underlying application also consumes
them. See the [Dear ImGui input FAQ](https://github.com/ocornut/imgui/blob/v1.92.9b/docs/FAQ.md#q-how-can-i-tell-whether-to-dispatch-mousekeyboard-to-dear-imgui-or-my-application),
accessed 2026-08-22. Wide Eye must also release relative mouse mode while the
panel is being used, then restore the player's prior capture intent when it
closes. It must suppress matching key-up edges as well as captured key-downs so
opening or editing the panel cannot leave a named action stuck.

### 5. The first panel should be intentionally small

The panel should prove expansion and trustworthy application, not expose every
constant. A useful first window would contain:

- session header: base scenario/version/seed, canonical/noncanonical state,
  active revision/hash, pause/resume, restart, reset category, reset all;
- search plus collapsible `Gameplay > Sheep` and `Graphics > Scene` categories;
- roughly 6–12 parameters chosen from values already needed by the active
  visual tracer and its all-influences diagnostic;
- a badge beside every value: `Next frame`, `Next tick`, `Restart`, `Rebuild`,
  or `Compile`;
- active versus pending value, validation error, dirty diff, and explicit
  `Apply`, `Apply and restart`, and `Discard` actions;
- a read-only live-evidence area for tick, selected sheep/influence values,
  frame timing, and OpenGL error status when those observables already exist.

Do not expose compile-only values as fake sliders. Listing a small number as
read-only `Compile` values can teach the boundary, but the panel should focus on
values it can actually apply.

### 6. Persistence should follow a successful in-memory slice

Saving profiles is useful, but it adds schema migration, parsing, path,
validation, corruption, and recovery responsibilities. The first vertical slice
can keep edits in memory and emit a deterministic parameter diff to diagnostics
or an evidence packet. That is enough to establish whether the panel shortens
iteration.

If repeated use earns persistence:

- define a small versioned developer-tuning profile separate from player saves
  and canonical gameplay replay;
- load into a temporary candidate, reject unknown/duplicate/non-finite/out-of-
  range values according to an explicit compatibility policy, validate the whole
  profile, then atomically replace active pending state;
- write through a temporary file plus rename, keep the last valid profile, and
  never auto-load a failed partial write;
- store per-user files in the platform's writable preference directory, not
  beside the executable. SDL documents `SDL_GetPrefPath` as the user/app-specific
  safe write location. See [`SDL_GetPrefPath`](https://wiki.libsdl.org/SDL3/SDL_GetPrefPath),
  accessed 2026-08-22;
- decide the exact parser/dependency only when this phase becomes current. The
  project does not presently contain a general JSON decoder, so choosing a
  format now would be premature.

Developer profiles are conveniences, not evidence by themselves. An evidence
packet should still record the resolved values and a stable hash rather than
only a path to a mutable local file.

## Options and tradeoffs

| Option | Strengths | Costs/risks | Verdict |
| --- | --- | --- | --- |
| Keep editing C++ constants | No new dependency or runtime mutation | Rebuild loop, poor comparison provenance, hard to scale across categories | Retain only for structural/code values |
| CLI/config file without GUI | Headless-friendly, scriptable, useful for presets and CI | Weak interactive discovery; repeated edit/save/reload loop; still needs the same typed model | Useful companion, not the main owner workflow |
| Dear ImGui in-process developer overlay | Fast contextual iteration; proven SDL3/OpenGL3 integration; natural category growth; can display live evidence next to controls | New pinned dependency; input/mouse-capture integration; must be isolated from release and determinism | **Recommended after the typed seam exists** |
| Custom in-engine widget/UI toolkit | Maximum visual and architectural control; could inform later player UI | Requires text, focus, layout, clipping, navigation, accessibility, and rendering work unrelated to current playtest question | Reject for developer tuning |
| Separate desktop/web editor with file watch or IPC | Can run outside the game and support large tools later | Two-process synchronization, transport/security, stale state, additional packaging, no current need | Defer until a real external-tool workflow exists |
| Full data-driven/reflection editor | Potentially exposes many types automatically | Large schema/reflection/persistence system before a second proven use; encourages unsafe blanket exposure | Reject |

## Recommendation

**Recommendation:** yes, a tuning GUI will be helpful, but build it as a
developer-only client of a typed runtime tuning seam—not as a set of sliders
attached to current constants.

The timing should be deliberate:

- finish the current Phase 0 visual baseline first so the baseline is not
  contaminated by a new dependency or UI path;
- let visual-feasibility Phase 1 establish explicit validated render settings,
  because that work is required with or without a GUI;
- then implement a bounded tuning vertical slice over one small gameplay group
  and one small graphics group. This proves category expansion and both fixed-
  tick/frame-boundary application without trying to expose the whole engine.

Use Dear ImGui only in an explicit developer-enabled interactive build/path.
Bounded capture, performance, headless tests, and release packaging should not
initialize it; the eventual production target should be able to compile it out.
Player-facing graphics/accessibility settings later get their own reviewed UX
and persistence contract, even if they reuse some validated setting types.

The recommended defaults are:

- graphics scalar/colour values: apply on the next frame;
- gameplay rule values: stage edits and default the primary button to `Apply and
  restart`; allow `Live next tick` only when the control is explicitly marked and
  the session is marked noncanonical;
- initial/world fixture values: restart-required;
- GPU/scene allocation values: explicit rebuild with rollback;
- code structure and algorithms: compile-only.

**Confidence:** high that the capability is useful and does not inherently need
recompilation; high that Dear ImGui fits the existing SDL3/OpenGL stack; medium
on the exact first parameter set and persistence format because the visual
settings extraction and owner tuning workflow have not yet been observed.

## Failure modes and gotchas

- **False reproducibility:** a capture keeps the old scenario ID/version but was
  produced with live overrides. Prevent with a noncanonical marker and resolved
  diff/hash in every tuning artifact.
- **History contamination:** a value applies live but prior velocity/arousal/
  position came from old values. Default gameplay comparisons to restart and
  show the apply tick.
- **Partial apply:** one field changes before cross-field validation fails.
  Validate a complete candidate copy and commit it atomically.
- **Invalid numeric input:** NaN, infinity, negative radii, inverted thresholds,
  and values that violate relationships can enter through text editing or a bad
  profile. Reject them before queuing.
- **Input leakage:** typing in a numeric field moves the dog, Escape both closes
  UI and changes capture, or a swallowed key-up leaves movement stuck. Forward
  events to the UI first and test capture transitions and paired edges.
- **Relative-mouse conflict:** a captured pointer cannot conveniently operate
  the panel. UI visibility must have one coherent reconciliation with the
  existing capture-intent state.
- **Renderer failure:** an invalid shadow size or shader/resource rebuild
  destroys the last working resource. Allocate/validate a replacement first and
  swap only on success.
- **UI in evidence/performance paths:** hidden overlay work alters frame cost or
  captured pixels. Do not initialize or render it in canonical bounded runners;
  separately measure its developer overhead if needed.
- **Duplicate truth:** descriptor defaults drift from typed scenario defaults.
  Descriptors adapt existing values; they do not own a second set of defaults.
- **Category coupling:** a generic profile lets render settings enter game rules
  or GL objects enter game configuration. Keep owner-specific typed stores and
  apply queues.
- **Dependency drift:** following Dear ImGui `master` conflicts with the
  repository's reproducibility policy. Pin and verify one reviewed tag/archive.
- **Tool becomes product UI:** the debug panel ships raw internal terms to
  players. Compile it out of release and build product settings separately.
- **Overexposure:** hundreds of sliders make tuning less intelligible and enable
  invalid combinations. Expose only parameters tied to a current question, with
  units, safe ranges, and reset behavior.
- **Unreviewed promotion:** a visually pleasing value silently replaces a
  canonical default without same-state comparison, tests, or versioning. Export
  a diff; promote through normal code review and evidence.

## Evidence and confidence

| Claim | Basis | Confidence |
| --- | --- | --- |
| Current gameplay values require source rebuild only because they are stored in source/copies without a setter | Direct code inspection | High |
| Ordinary OpenGL presentation values can update without executable recompilation | OpenGL runtime uniform/resource model plus current shader inspection | High |
| Dear ImGui has maintained SDL3 and OpenGL3 backends compatible in shape with this stack | Official tagged sources and example | High; actual project integration unverified |
| A UI will shorten the owner's tuning loop | Inference from current source-edit/build/run workflow and provisional parameters | Medium until an owner session is timed/observed |
| Apply-and-restart is the best default for gameplay comparison | Inference from state history and deterministic evidence requirements | High for comparisons; live mode remains useful for exploration |
| In-memory-first is preferable to immediate profile persistence | Scope/risk inference from the absence of a decoder and migration contract | Medium |

No GUI dependency was downloaded, installed, linked, or run. No native Windows
or Linux graphical integration was tested for this research.

## Planning handoff

This is a proposed sequence for a later adversarial planning pass, not approved
roadmap scope.

### Stage 0 — Owner and architecture gate

- Confirm that the first purpose is developer tuning, not player settings.
- Confirm placement after the current Phase 0 baseline and after explicit visual
  scene settings exist.
- Select the first 6–12 parameters and the one tuning task whose elapsed time
  will be compared before/after.
- Decide whether developer UI is a separate target or a compile-time feature of
  the interactive target; require release/headless paths to exclude it.

**Stop if:** the owner primarily needs a player settings menu, remote editor, or
terrain authoring tool; each is a materially different outcome.

### Stage 1 — Typed tuning domain, no GUI

- Define stable parameter IDs, metadata, value types, apply classes, pending and
  active revisions, and whole-profile validation.
- Separate immutable scenario identity/defaults from runtime exploratory
  overrides without changing canonical default behavior.
- Add owner-specific apply queues: gameplay at a fixed-tick/restart boundary and
  render at a frame/rebuild boundary.
- Emit a deterministic resolved diff/hash and noncanonical state in diagnostics.

**Evidence:** headless unit/scenario tests for ID uniqueness, range and
cross-field rejection, atomic application, reset, fixed-tick ordering, and
byte-identical default scenario outputs with tuning disabled.

### Stage 2 — Minimal Dear ImGui host

- Review and pin one exact Dear ImGui release/archive and license set.
- Integrate only its core plus official SDL3 and OpenGL3 backends in the
  developer path.
- Add one toggleable empty window, correct init/shutdown order, DPI/readability
  check, event forwarding, keyboard/mouse capture filtering, and relative-mouse
  reconciliation.

**Evidence:** focused input tests where feasible, interactive Windows and native
Linux smoke, no high-severity OpenGL messages, and unchanged bounded/headless
outputs when disabled.

### Stage 3 — Bounded cross-category vertical slice

- Bind the selected gameplay and graphics parameters through the typed catalog.
- Show active/pending/default values, units, apply badges, validation, reset,
  dirty diff, and canonical/noncanonical status.
- Implement next-frame, next-tick, and apply-and-restart paths; add resource
  rebuild only if one selected parameter genuinely requires it.
- Display existing observables that directly help the selected tuning question.

**Evidence:** one scripted parameter transition per apply class, same-start
before/after state/capture packet, exact settings diff/hash, and an owner timing
of the chosen tuning task.

### Stage 4 — Decide whether persistence is earned

- If the owner repeatedly reuses profiles across runs, specify a versioned
  developer-profile format, safe per-user path, strict validation, atomic write,
  last-valid recovery, and compatibility policy.
- Add load/save/export tests including truncation, duplicates, unknown IDs,
  non-finite values, old/new schema versions, and write failure.
- Keep promotion into canonical settings a reviewed source change.

**Evidence:** round-trip and recovery tests plus one reproduced tuning session on
both native target platforms.

### Stage 5 — Expansion rule

Add a new category only when it has:

- a real subsystem-owned typed setting;
- a stated current tuning question;
- validation and an apply class;
- reset/default behavior;
- evidence output sufficient to reproduce or deliberately classify the result.

Do not add a general property inspector, docking workspace, plotting extension,
remote control, or player-facing settings until a concrete later task earns it.

## Planning questions

1. Should the first measured workflow optimize visual tuning (the active tracer)
   or sheep-behavior tuning (the eventual gameplay loop)? The architecture can
   support both, but the first panel should have one primary question.
2. Should developer UI be a separate executable target or a compile-time option
   on `wide_eye`? A separate target gives the cleanest release boundary; an
   option reduces target duplication. This should be resolved against current
   CMake target structure during planning.
3. Is in-memory tuning plus exported evidence sufficient for the first slice, or
   does the owner already need profiles to survive restarts? Persistence is
   feasible, but it should not be assumed without a repeated workflow need.

## References

Project sources:

- [`ROADMAP.md`](../../ROADMAP.md), current checkpoint and Phase 3 pause,
  inspected 2026-08-22.
- [ADR 0001: Native engine foundation](../decisions/0001-native-foundation.md),
  dependency, platform, asset, and debug-UI policy.
- [Visual-feasibility implementation plan](../plans/visual-feasibility-before-objective-loop.md),
  especially Phase 1 renderer-facing settings.
- [`gameplay_scenario.hpp`](../../src/game/gameplay_scenario.hpp), typed scenario
  and sheep configurations.
- [`gameplay_simulation.cpp`](../../src/game/gameplay_simulation.cpp), scenario
  copy, fixed-tick reads, and restart behavior.
- [`gameplay_replay.hpp`](../../src/game/gameplay_replay.hpp), scenario identity
  and replay version contract.
- [`dog_controller.hpp`](../../src/game/dog_controller.hpp) and
  [`camera_controller.hpp`](../../src/game/camera_controller.hpp), current
  compile-time motor/camera constants.
- [`opengl_renderer.cpp`](../../src/render/opengl_renderer.cpp), current embedded
  shaders and presentation constants.
- [`window_runtime.cpp`](../../src/platform/window_runtime.cpp), SDL event,
  relative-pointer, fixed-step, render, and performance ownership.

External primary sources, accessed 2026-08-22:

- Dear ImGui [`v1.92.9b` release](https://github.com/ocornut/imgui/releases/tag/v1.92.9b)
  and commit `f1cc2ae15e53a861a874c3034aae6798fde194ab`.
- Dear ImGui [backend guide](https://github.com/ocornut/imgui/blob/v1.92.9b/docs/BACKENDS.md),
  [SDL3/OpenGL3 example](https://github.com/ocornut/imgui/tree/v1.92.9b/examples/example_sdl3_opengl3),
  [input FAQ](https://github.com/ocornut/imgui/blob/v1.92.9b/docs/FAQ.md#q-how-can-i-tell-whether-to-dispatch-mousekeyboard-to-dear-imgui-or-my-application),
  and [MIT license](https://github.com/ocornut/imgui/blob/v1.92.9b/LICENSE.txt).
- Khronos [OpenGL 4.6 specification](https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf)
  and [`glUniform` reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUniform.xhtml).
- SDL 3 [`SDL_GetPrefPath`](https://wiki.libsdl.org/SDL3/SDL_GetPrefPath).

## Recommended next step

Do not interrupt the current visual-baseline outcome to implement the panel.
After that baseline is complete, run the repository's `plan-from-research`
workflow on this document together with the accepted visual-feasibility plan.
That planning pass should resolve the three owner questions, challenge the
typed ownership boundary against the then-current renderer settings, and either
place one bounded tuning slice in the roadmap or reject it with evidence.
