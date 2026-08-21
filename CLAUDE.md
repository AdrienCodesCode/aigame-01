# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Authoritative instructions

Read these before material work; do not restate or contradict them here.

- [AGENTS.md](AGENTS.md) — durable project rules: evidence and claim discipline,
  proportional engineering, contribution rules, definition of done. It applies to
  the whole repository and to Claude Code.
- [ROADMAP.md](ROADMAP.md) — the cross-context continuation source. Its
  "Current checkpoint" section names the current phase, verified state, known
  limits, and the next action. Work the first unblocked unchecked item of the
  current phase; never check an item for an unrun result.
- [docs/DEVELOPMENT_WORKFLOW.md](docs/DEVELOPMENT_WORKFLOW.md) — the accepted
  task contract, build/observe/review loop, verification cadence, regression
  protocol, artifact/golden rules, and the required fresh-chat / continue /
  compact recommendation at the end of each coherent outcome.

The repository is two things at once: a fork of an external AI game-development
playbook (`README.md`, `prompts/`, `game-prompt.md`) whose claims are explicitly
unverified here, and the **Wide Eye** custom C++23/SDL3/OpenGL voxel engine,
which is the approved primary implementation track.

## Build, test, run

Toolchain: CMake ≥ 3.28, Ninja, Clang 18 (`clang-format-18` / `clang-tidy-18`
for the developer targets). Ubuntu setup and the local fallback bootstrap are in
[docs/setup/UBUNTU_24_04.md](docs/setup/UBUNTU_24_04.md); native Windows in
[docs/setup/WINDOWS.md](docs/setup/WINDOWS.md).

```bash
cmake --preset dev                 # or dev-sanitized (ASan/UBSan), release
cmake --build --preset dev
ctest --preset dev                 # presets already set output-on-failure + stopOnFailure
```

Build output lands in `build/<HostSystemName>/<preset>/`, e.g.
`./build/Linux/dev/wide_eye`.

```bash
ctest --preset dev -R wide_eye.gameplay_simulation   # one test
ctest --preset dev -L unit                           # by label
cmake --build --preset dev --target format-check     # clang-format 18, dry-run
cmake --build --preset dev --target clang-tidy-check # bounded static analysis
cmake --build --preset dev --target qa-check         # QA tracker schema/index drift
cmake --build --preset dev --target qa-index         # regenerate docs/qa/INDEX.md
cmake --build --preset dev --target qa-next          # next free QA issue id
```

The `qa-*` targets wrap `tools/qa/qa-tracker.cmake`, which also runs without a
configured build directory (`cmake -DMODE=check -P tools/qa/qa-tracker.cmake`).
They are deliberately outside `ctest`: a docs-schema slip must not fail the
engine suite.

CTest labels are `unit`, `scenario`, `headless`, `sanitizer`, `performance`
(`sanitizer` is appended automatically in the `dev-sanitized` preset). `manual`
is an evidence category outside CTest.

Interactive play (needs a real OpenGL 4.6 Core context):

```bash
./build/Linux/dev/wide_eye
./build/Linux/dev/wide_eye --play-scenario paddock-start
```

Version 1 scenarios: `paddock-start`, `presentation-motion` (a scripted render
fixture, **not** accepted flock behavior), `wall-contact`, `closed-gate`,
`open-gate`, plus the headless `sheep-*` behavior fixtures. Controls: mouse
orbit, camera-relative WASD, Shift sprint, R restart, Tab free-debug camera,
Escape toggle pointer capture.

The single `wide_eye` executable is also the harness: `src/platform/main.cpp`
dispatches exact argv shapes to named smokes (`--runtime-smoke`,
`--window-state-smoke`, `--dog-scenario <name>`, `--paddock-smoke [--capture P]`,
`--dog-render-smoke`, `--sheep-motion-render-smoke [--tick N --view V --capture P
--state-dump P]`, the `--paddock-*-smoke` debug views, …). Argument parsing is
positional and strict — add a new shape rather than loosening an existing one.

### Graphics and performance test gating

- Display-backed OpenGL tests are guarded by `WIDE_EYE_ENABLE_OPENGL_CONTEXT_TEST`
  (default ON on Windows, OFF elsewhere). The WSL development host exposes only
  OpenGL 4.5, so it can run the headless suites but cannot substitute for native
  Windows or native Linux 4.6 evidence.
- `wide_eye.opengl_*_performance` tests are registered only for
  `CMAKE_BUILD_TYPE=Release` and pass only on the literal marker
  `within_provisional_low_budget=yes`.
- CI ([.github/workflows/linux.yml](.github/workflows/linux.yml)) runs the
  Clang 18 `dev` preset configure/build/test and uploads failure evidence.

## Architecture

Ownership boundaries are enforced deliberately; read
[src/README.md](src/README.md) (detailed and current) and
[tests/README.md](tests/README.md) before adding APIs. Summary:

- `src/core` — monotonic time, the 60 Hz `FixedStepAccumulator` (the *only*
  render-to-simulation scheduler), assertions, duration statistics, process
  memory, and the typed performance budgets.
- `src/platform` — entry point, SDL lifecycle, `window_runtime` (window/GL
  context lifetime, event polling, relative-mouse capture, presentation),
  `window_state` reducer, `scenario_runner` (scenario config, render-resource
  lifetime, framebuffer oracles, capture paths), and `input` translating
  keyboard/mouse/gamepad into `NamedInputState`.
- `src/render` — `OpenGlRenderer` façade over GL resources, draws, and RGBA8
  readback plus deterministic PNG encoding. Holds no authoritative state.
- `src/voxel` — signed 64-bit world/chunk/local coordinate types (floor
  division), 16³ chunks of one-byte material IDs, the two-pass naive exposed-face
  mesher with conservative ceilings and checked aggregate limits, and the
  handcrafted 32×16×32 paddock.
- `src/game` — authoritative fixed-tick rules: `GameplayScenarioDefinition`
  (version/seed/dog config/sheep fixture), `GameplaySimulation` (one domain
  input per tick, publishes read-only previous/current snapshots of the dog and
  five sheep), `dog_controller`, `camera_controller`, `paddock_collision` (the
  analytic paddock walls/gate used for dog collision, sheep collision, and sheep
  sight lines), `flock_observables`, `sheep_spatial_grid`, `gameplay_replay`.

Invariants that repeatedly matter:

- Game rules never see SDL scancodes, buttons, axes, windows, or events, and
  never receive render-frame timing. Presentation interpolates published copies
  and never feeds back into simulation.
- Each tick derives the next sheep buffer from the immutable prior buffer.
- Replay/state contracts are versioned; the current numbers live in
  `gameplay_replay.hpp` and
  [docs/formats/GAMEPLAY_REPLAY_AND_STATE.md](docs/formats/GAMEPLAY_REPLAY_AND_STATE.md)
  — do not restate them in prose docs. Compatibility validation completes
  before any mutation.
- Architectural changes of this kind get an ADR in [docs/decisions/](docs/decisions/)
  (0001 native foundation, 0002 chunk edge length, 0003 project-owned test
  harness — no doctest, 0004 gameplay-scenario ownership, 0005 game-owned
  paddock collision and gate state).

Dependencies: SDL 3.4.10 via `FetchContent` (most subsystems force-disabled in
[cmake/WideEyeDependencies.cmake](cmake/WideEyeDependencies.cmake)) and a
vendored, checksum-verified glad OpenGL 4.6 Core loader in `third_party/glad`.
Adding a dependency requires a current requirement plus the AGENTS.md checks.

## Evidence, artifacts, and goldens

- Generated evidence goes under `artifacts/<phase>/<date>/`, which is
  gitignored. Report the relevant excerpt and path, not the whole log.
- `tests/goldens/` holds accepted baselines with manifests and SHA-256 hashes;
  CTest validates required fields, retained files, hashes, and an owner `Accept`
  verdict. Never promote a candidate golden, overwrite a baseline, or loosen a
  threshold without the owner's explicit accept — generating a candidate is
  allowed, promoting it is not.
- `.gitattributes` preserves golden and glad bytes exactly; do not normalize them.
- Owner-facing visual review uses
  [docs/review/HUMAN_VISUAL_REVIEW.md](docs/review/HUMAN_VISUAL_REVIEW.md); the
  Windows runners are `tools/phase1..phase3/*.ps1`.

## Repository skills

`.agents/skills/` holds plain-Markdown workflows (`deep-research`,
`plan-from-research`, `update-project-docs`, `end-engine-session`, `qa-intake`,
`qa-fix`). Read the relevant `SKILL.md` and follow it rather than inventing a
competing process. In particular, when the user asks to end, wrap up,
checkpoint, or hand off a session, follow `end-engine-session`, which updates
the `ROADMAP.md` checkpoint instead of adding a diary file.

## QA defect tracker

`docs/qa/` is the defect lane, separate from the roadmap and from plan docs:
something that exists and is wrong is a QA issue; something that does not exist
yet is a plan doc. The convention, frontmatter schema, severity/confidence
meanings, and closure rules are in [docs/qa/README.md](docs/qa/README.md).

- File with the `qa-intake` skill (`/qa <what you saw>`) — investigate in the
  code first, take the id from `qa-next`, and never file `confirmed` on
  reasoning alone. A WSL run is not native OpenGL 4.6 evidence.
- Fix with the `qa-fix` skill (`/qa-fix`) — reproduce first, run the issue's
  `verify:` entries, record the resolution with commands and platform, then
  `git mv` the file to `docs/qa/closed/` and regenerate the index.
- `docs/qa/INDEX.md` is generated; never hand-edit it. Ids are permanent and
  never reused. Only the owner ticks a charter box in `docs/qa/charters/`.

## Writing style for docs in this repo

Documentation states observed truth. Label statements as goal, hypothesis,
observed result (with build, date, platform, method), inference, or unverified
claim. Do not repeat the inherited "two days", "15 hours of gameplay", or "AAA"
claims as facts, and do not describe intended support as verified support.
