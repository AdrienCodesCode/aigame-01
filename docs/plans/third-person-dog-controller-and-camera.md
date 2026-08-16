# Plan: Third-person dog controller and gameplay camera

**Status:** Implemented; native keyboard/mouse baseline accepted, refinement deferred

**Date:** 2026-08-16

**Source research:**
[Third-person dog controller and gameplay camera](../research/third-person-dog-controller-and-camera.md)

**Architecture readiness:** Localized prerequisite approved — the existing
`platform` input, fixed-tick `game`, scenario, and immutable renderer-snapshot
boundaries are suitable, but transient relative-mouse input and render
interpolation must be carried through those boundaries before feel tuning.

## Objective and success criteria

Create the first owner-testable third-person direct-control model for Wide Eye.
Horizontal mouse motion rotates the gameplay camera's control direction, and
WASD selects a ground-plane movement direction relative to that camera. The dog
turns and accelerates toward the resolved world direction without snapping,
while the camera remains independently controllable.

The experiment succeeds when:

- moving the mouse right rotates the view/control yaw right, and holding W at
  the same time curves the dog's world trajectory toward the new view;
- mouse movement while no movement key is held orbits the camera without
  rotating the dog in place;
- W/S move camera-forward/back and A/D move camera-left/right from every tested
  yaw, with normalized diagonals and no fixed world-axis behavior;
- dog velocity, body facing, and camera pose contain no discontinuous first-tick
  jump during start, stop, diagonal, sprint, 90-degree turn, or reversal cases;
- a hard reversal slows the dog while it turns instead of preserving full speed
  through an obvious sideways slide;
- manual mouse/right-stick look is never opposed by automatic yaw alignment in
  this experiment;
- repeated fixed-tick input sequences reproduce the same dog and authoritative
  gameplay-camera state;
- presentation at render rates above or between 60 Hz does not expose the fixed
  simulation as obvious dog/camera stepping; and
- analytic collision, deterministic scenarios/restart, free-debug controls,
  accepted voxel captures, and existing test/performance evidence remain valid.

## Scope and non-goals

### In scope

- Relative mouse yaw and pitch for the interactive gameplay camera.
- Equivalent named right-stick look-rate semantics, pending later hardware
  verification.
- Camera-relative WASD/left-stick movement on the ground plane.
- Independent gameplay control yaw/pitch and dog body yaw.
- Vector-bounded planar acceleration/deceleration and deliberate reversal
  behavior.
- Previous/current fixed state and interpolated presentation snapshots.
- Focus, restart, scenario, camera-toggle, and input-transient regression tests.
- One native Windows keyboard/mouse feel review with motion/state evidence.

### Non-goals

- The deferred RTS-style overhead selection/order mode.
- Character-relative tank steering or a permanent runtime A/B setting.
- Automatic gameplay-camera recentering, flock-aware composition, lock-on,
  aiming, camera shake, or cinematics.
- Camera obstruction sweeps; the architecture must leave room for them, but the
  base movement/view relationship should be accepted first.
- Final sensitivity, acceleration, speed, pitch, turn-rate, or accessibility
  settings. Initial values remain provisional until owner review.
- Input remapping, menus, cursor-driven UI, final animation, root motion, sheep
  pressure, or a formal Phase 3 replay-file format.
- Claiming controller support from synthetic SDL events without a physical
  device test.

## Verified pre-implementation state

- `DogController` currently interprets movement axes directly as world X and
  negative world Z, approaches velocity per component, and turns body yaw
  toward velocity at a provisional 180 degrees per second.
- `CameraController` currently derives gameplay eye/target directly from dog
  heading. Gameplay look input changes only the free-debug camera.
- `NamedInputState` maps keyboard and standardized gamepad state to named
  actions but does not consume `SDL_EVENT_MOUSE_MOTION` or distinguish
  transient mouse deltas from held stick rates.
- `window_runtime` owns the SDL window, event loop, input snapshot cadence, and
  60 Hz accumulator. `scenario_runner` owns dog/camera orchestration. These are
  the correct boundaries for mouse capture, named input, and fixed-tick control
  state respectively.
- `FixedStepAccumulator` already publishes `interpolation_alpha`, but the window
  runtime discards it and `RenderScenarioRunner` renders the latest dog/camera
  state directly.
- SDL 3.4.10 is pinned. SDL's current API provides per-window relative mouse
  mode and `SDL_MouseMotionEvent::xrel/yrel`; relative mode is a main-thread
  platform concern and flushes pending motion when toggled. See
  [SDL relative mouse mode](https://wiki.libsdl.org/SDL3/SDL_SetWindowRelativeMouseMode)
  and
  [`SDL_MouseMotionEvent`](https://wiki.libsdl.org/SDL3/SDL_MouseMotionEvent),
  accessed 2026-08-16.
- On 2026-08-16, the existing WSL development build passed the five focused
  `wide_eye.dog_controller`, `wide_eye.input_actions`, and named wall/gate
  scenario CTests. These tests establish the current placeholder baseline, not
  acceptable player feel.
- The owner observed that bounded body turning improved continuity but the
  coupled controller remained substantially wrong. Sprint, camera toggle, and
  free-debug movement worked; a physical controller was unavailable.

## Adversarial review result

| Finding | Classification | Planning consequence |
| --- | --- | --- |
| Fixed-world WASD plus a body-mounted camera produces the reported 180-degree A/D relationship | Confirmed | Remove fixed-world interpretation from gameplay input resolution. |
| Mouse horizontal motion should directly rotate the dog even while stationary | Rejected | Mouse changes camera/control yaw; dog facing changes from movement intent. |
| A/D should be continuous dog-turn keys in this experiment | Rejected by the selected camera-relative policy | A/D mean screen/camera left and right; mouse/right stick owns yaw. |
| Held W should react to manual mouse yaw | Confirmed by owner direction | Use the live authoritative control yaw each fixed tick, so the trajectory curves. |
| Delayed automatic camera alignment is needed immediately | Qualified but deferred | Start with no automatic yaw alignment, eliminating manual/automatic feedback while base feel is judged. |
| Existing named action values can represent mouse motion unchanged | Rejected | Mouse delta and stick look rate require different transient/integration semantics. |
| Per-axis acceleration is an adequate planar motor | Rejected | Bound the magnitude of the planar velocity change and design reversal behavior explicitly. |
| Interpolating only the dog is sufficient | Rejected | Preserve prior/current dog and gameplay-camera state and create one coherent render snapshot. |
| Camera collision must be implemented before movement can be judged | Qualified but deferred | Preserve a desired-pose/final-pose seam; test base control before adding obstruction response. |
| Existing ownership boundaries require a new dependency or engine subsystem | Rejected | Extend the current `platform`, `game`, scenario, and render-snapshot seams only. |

The adversarial pass does not overturn the research recommendation. It narrows
it: camera-relative free-direction movement is now the selected experiment, the
character-steering A/B runtime mode is unnecessary, and automatic alignment is
removed from the first pass.

## Decisions and assumptions

### Player-facing control contract

| Input/state | Selected behavior |
| --- | --- |
| Mouse right/left | Increase/decrease gameplay control yaw and orbit the camera around the dog. |
| Mouse up/down | Change gameplay pitch with a bounded non-inverted default; pitch never tilts the ground-plane movement basis. |
| W/S | Move along/opposite the camera yaw's ground-plane forward direction. |
| A/D | Move along camera yaw's ground-plane left/right direction; these are not turn-in-place keys. |
| Diagonal WASD | Normalize the two-dimensional intent before applying walk/sprint speed. |
| Mouse with no move input | Orbit the camera only; retain the dog's last meaningful body facing. |
| Mouse while move input is held | Re-resolve movement from the live control yaw on each fixed tick, producing a deliberate curved path. |
| Hard reversal | Begin the shortest body turn immediately, reduce speed during the large heading error, then accelerate in the new direction. |
| Shift/sprint | Change target speed without changing the control reference frame or bypassing turn/reversal limits. |
| Tab/free-debug | Preserve the independent free-debug pose and controls; freeze the dog as today. Returning restores the gameplay camera state rather than adopting debug-camera yaw. |
| Restart | Restore the named scenario's dog state and deterministic gameplay-camera yaw/pitch; clear transient look input. |

No automatic gameplay-camera yaw recentering is part of this experiment. The
camera follows the dog's position, but its yaw remains where the player placed
it. If later testing shows that controller users need assistance, delayed
alignment can be a separate, measured outcome.

### State and dependency ownership

```text
SDL keyboard/mouse/gamepad events
        |
        v
platform named held actions + transient mouse look delta
        |
        v  (one snapshot per 60 Hz tick)
gameplay control yaw/pitch ----> camera-relative world move intent
        |                                  |
        |                                  v
        |                           dog motor + collision
        |                                  |
        +-------------------+--------------+
                            v
                 previous/current fixed state
                            |
                            v
          interpolation alpha -> immutable render snapshot
                            |
                            v
                     OpenGL renderer
```

- `platform` owns SDL event interpretation, window-relative mouse mode, focus
  behavior, and accumulation/consumption of transient pointer motion.
- `game` owns deterministic control yaw/pitch, movement-basis math, dog motor,
  body facing, and camera desired pose.
- Scenario orchestration applies look before resolving movement on the same
  tick, then advances the dog and publishes prior/current state.
- `render` receives only the interpolated camera and dog snapshot. It does not
  derive authoritative heading or feed presentation state back into gameplay.
- The later RTS command layer can produce world-space movement intent without
  changing the dog motor or making it depend on a camera class.

### Input timing semantics

Mouse displacement is a transient delta, while right-stick look is a held rate:

- accumulate all relative mouse `xrel/yrel` values until a fixed tick consumes
  them;
- apply the accumulated mouse delta once, independent of fixed delta time, then
  clear it with other transient input;
- retain it across a render frame that produces zero fixed ticks;
- when one render frame produces multiple catch-up ticks, apply the queued
  mouse delta to the first eligible tick only rather than multiplying it;
- integrate right-stick look rate on every tick using fixed delta time;
- combine the resulting angular changes, not their raw device units; and
- record per-tick named move/look values or resolved world move intent in future
  replay work, never a render-time camera transform.

Exact sensitivity and stick response values are provisional constants with
named tests for sign, units, clamp, and frame-rate independence. They are not
accepted feel claims.

## Prerequisites

The work needs two localized seams, neither of which changes the accepted
engine architecture or adds a dependency:

1. **Transient named look input:** extend the platform input snapshot so mouse
   deltas are accumulated and consumed differently from held action values and
   stick rates. The SDL window runtime, not game code, enables/disables relative
   mouse mode on the main thread and clears pending deltas across focus/capture
   transitions.
2. **Render interpolation contract:** pass the existing accumulator alpha to
   interactive scenario presentation and retain previous/current fixed state.
   Bounded static smoke scenarios can use an explicit terminal alpha without
   becoming gameplay simulations.

If either seam requires exposing `SDL_Window*` to game/scenario code, making the
renderer authoritative, or changing the 60 Hz simulation rate, stop: that would
violate the accepted ownership constraints rather than satisfy the prerequisite.

## Implementation phases

### Phase 0 — Prove transient mouse input and render-state seams

**Outcome:** The platform can supply one deterministic transient look delta per
fixed tick, and the scenario-render interface can receive interpolation alpha,
without changing visible gameplay yet.

**Likely files/components:** `src/platform/input.*`,
`src/platform/window_runtime.*`, `tests/input_tests.cpp`, focused runtime tests,
and the interactive scenario interface.

**Dependency direction:** SDL event/window details stay in `platform`; snapshots
contain device-independent movement, look-rate, and look-delta values only.

**Checkable tasks:**

- Add failing tests for multiple mouse-motion events accumulating, zero-tick
  retention, one-time consumption, focus clearing, sign, and coexistence with
  right-stick look rate.
- Enable relative mouse mode only for the interactive input configuration;
  handle failure as a named platform failure and disable/clear it during focus
  loss and shutdown.
- Preserve current keyboard/gamepad held actions and rising-press behavior.
- Carry interpolation alpha to interactive rendering with explicit behavior for
  bounded one-frame scenarios.

**Validation:** focused input/runtime tests, current window-state test, build
with strict warnings, and inspection that no SDL type crossed into `game`.

**Evidence artifact:** focused CTest output plus a short diagnostic showing one
mouse delta consumed on exactly one fixed tick.

**Stop if:** relative mode cannot be owned entirely by the main-thread platform
runtime, or transient values would be applied once per render instead of once
per authoritative tick.

### Phase 1 — Add authoritative gameplay-camera orbit and movement basis

**Outcome:** Gameplay camera yaw/pitch is independent from dog body yaw, and
WASD/left stick resolves to a world-space ground direction from control yaw.

**Likely files/components:** `src/game/camera_controller.*`, a small
platform-independent movement-basis helper if warranted,
`src/platform/scenario_runner.cpp`, and `tests/dog_controller_tests.cpp` or a
focused new game-control test target.

**Dependency direction:** game orchestration reads named input and produces
world-space move intent; `DogController` does not depend on `CameraController`
or SDL.

**Checkable tasks:**

- Store gameplay control yaw/pitch and deterministic restart values separately
  from free-debug camera state.
- Apply mouse delta and stick rate before resolving movement on each tick.
- Resolve camera-forward/right on the XZ plane, normalize diagonal intent, and
  leave pitch out of movement.
- Make gameplay pose follow dog position using control yaw/pitch; remove direct
  gameplay yaw derivation from dog heading.
- Preserve gameplay and free-debug state independently across Tab toggles.
- Add no automatic yaw alignment.

**Validation:** exact cardinal/diagonal mappings at 0, 90, 179/-179, and 359/0
degrees; mouse-only orbit; held-W-plus-yaw curved trajectory; restart; mode
toggle; repeated sequence equality.

**Evidence artifact:** a deterministic per-tick trace containing raw move,
mouse delta/stick rate, control yaw/pitch, and resolved world move direction.

**Stop if:** movement depends on render-time yaw, mouse-only orbit rotates the
dog, or toggling the debug camera replaces gameplay control yaw.

### Phase 2 — Replace component-wise motion with a planar dog motor

**Outcome:** The dog accelerates, stops, turns, sprints, and reverses smoothly
under world-space move intent without diagonal acceleration bonus or full-speed
sideways reversal.

**Likely files/components:** `src/game/dog_controller.*` and
`tests/dog_controller_tests.cpp`; analytic collision remains unchanged.

**Dependency direction:** The dog motor consumes normalized world-space intent
and sprint state. It knows nothing about keys, mouse, camera mode, or SDL.

**Checkable tasks:**

- Bound the magnitude of planar velocity change rather than clamping X and Z
  components independently.
- Keep walk/sprint target speed separate from acceleration/deceleration.
- Turn body yaw toward nonzero desired movement by the shortest path.
- Reduce target speed during large heading errors so a hard reversal decelerates
  while the body turns, then accelerates into the new direction.
- Preserve last meaningful facing at rest and collision-resolved velocity after
  wall/gate contact.
- Retain tunable values as named provisional constants; do not carry forward
  the current 180-degree-per-second value as accepted feel.

**Validation:** cardinal versus diagonal acceleration magnitude; start/stop;
walk/sprint transition; 90/180-degree changes; opposite input before and after
zero speed; shortest-angle wrap; wall slide; closed/open gate; 240-tick
non-tunneling; repeated run and restart equality.

**Evidence artifact:** state trace of desired direction, speed, velocity, body
yaw, heading error, and collision-resolved position for turn and reversal cases.

**Stop if:** fixing visual facing changes analytic collision truth, introduces
frame-delta dependence, or requires animation/root motion.

### Phase 3 — Interpolate one coherent dog/camera render snapshot

**Outcome:** Rendering consumes a previous/current fixed-state interpolation;
gameplay remains authoritative at 60 Hz and manual look has no extra damping
layer.

**Likely files/components:** fixed-state storage in scenario/game presentation,
the scenario render contract, interpolation helpers/tests, and immutable
renderer input snapshots.

**Dependency direction:** interpolation is presentation-only. No interpolated
value returns to dog collision, movement resolution, restart, or later sheep
pressure.

**Checkable tasks:**

- Retain previous/current dog position, body yaw, gameplay control yaw/pitch,
  and required camera anchor data around every fixed tick.
- Interpolate positions linearly and angles along the shortest path using the
  accumulator alpha.
- Build the camera and dog renderer inputs from the same interpolated snapshot.
- Snap previous/current state together on initialization, restart, and camera
  mode transitions to prevent stale-state blends.
- Avoid a second camera smoothing spring in this outcome.

**Validation:** alpha 0/0.5/1 endpoints; angle wrap; restart/toggle snaps;
synthetic 30/60/120/144 Hz render cadence over identical 60 Hz fixed states;
proof that authoritative end state is identical for all presentation cadences.

**Evidence artifact:** cadence test output and a short motion/contact-sheet or
video packet from the native Windows build.

**Stop if:** interpolation changes authoritative state, camera and dog use
different alphas, or mouse response is damped twice.

### Phase 4 — Native keyboard/mouse review and bounded tuning

**Outcome:** The owner can judge one coherent third-person candidate and either
Accept, Revise, or Reject it without conflating control policy with unimplemented
camera collision or animation.

**Likely files/components:** only provisional constants/tests needed by observed
behavior, the existing Phase 2 artifact path, and documentation after a verdict.

**Dependency direction:** tuning changes game constants only; no new library,
renderer authority, or platform-specific gameplay branch.

**Checkable tasks:**

- Run the full development and sanitized suites plus project format/static
  analysis after focused tests pass.
- Reproduce wall, gate, restart, sprint, Tab, and no-input behavior.
- On native Windows, test mouse-only orbit, W plus mouse steering, A/D screen
  movement, diagonal, stop, reversal, and focus loss/regain.
- Capture a review packet with build/worktree state, controls, provisional
  constants, motion evidence, state trace, platform/refresh rate, and blank
  owner verdict.
- Tune only one coherent constant group per review: mouse/pitch, planar motor,
  body turn/reversal, or camera composition.

**Validation:** owner observation on keyboard/mouse; native Windows OpenGL debug
output; existing accepted-capture hashes where applicable; no physical-
controller acceptance claim.

**Evidence artifact:**
`artifacts/phase2/<date>/third-person-controller-review/` containing the review
record, log/state trace, and motion evidence. Promotion or roadmap completion
requires an explicit owner verdict.

**Stop if:** two materially different tuning passes fail the same behavior. In
that case minimize the scenario and revisit the control invariant rather than
adding another smoothing layer.

## Verification matrix

| Area | Automated evidence | Manual evidence |
| --- | --- | --- |
| Mouse transient input | Accumulation, one-time consumption, zero-tick retention, focus clear, sign | Cursor capture/release and no post-focus jump on native Windows |
| Camera basis | Cardinal/diagonal yaw fixtures, pitch independence, angle wrap | Mouse right turns view/rightward travel as expected |
| Held movement plus mouse | Repeated fixed input produces expected curved positions | Hold W while sweeping mouse slowly and quickly |
| Mouse without movement | Camera yaw changes; dog position/body yaw do not | Orbit around a stationary dog |
| A/D semantics | Camera-left/right world vectors at multiple yaw values | Screen-relative lateral movement remains predictable |
| Motor | Equal-magnitude cardinal/diagonal response, bounded delta, start/stop/reversal | No obvious snap, skid, or mushy delay |
| Collision | Existing wall/gate sweeps and tick scenarios plus angled wall contact | Wall/gate movement remains controllable |
| Restart/modes | Exact dog/gameplay-camera restart; independent debug pose | R and Tab preserve expected control state |
| Determinism | Repeated tick sequence equality | None required for local deterministic claim |
| Presentation | Alpha endpoints, angle wrap, multi-cadence authoritative equality | Compare 60 Hz and available high-refresh presentation |
| Gamepad | Existing synthetic actions plus rate/delta separation tests | Deferred until a physical controller is available |

## Performance and platform matrix

| Environment | Required in this plan | Claim limit |
| --- | --- | --- |
| WSL Ubuntu development | Configure/build, focused and full development CTests, format/static analysis | Headless logic and compile evidence only; not native Linux graphics or mouse feel |
| WSL ASan/UBSan | Full affected suite at the coherent-outcome gate | Project-code sanitizer evidence; no OpenGL 4.6 execution on this host |
| Native Windows, Intel UHD 630 | OpenGL/debug run, keyboard/mouse behavior, motion evidence, regression captures | Reference-machine prototype evidence, not broad Windows support |
| Native Linux | Deferred to the existing native Linux gate | WSL does not substitute |
| Physical gamepad | Deferred until hardware is available | Synthetic SDL event tests only |

The controller math should allocate no steady-state memory and add negligible
CPU cost relative to the 2 ms Low-profile five-sheep simulation budget. The
review records frame-time/RSS only to catch regressions; this task does not claim
new renderer performance. Relative mouse mode must be exercised on the main
thread and does not add a dependency beyond pinned SDL 3.4.10.

## Risks, rollback, and deferred work

- **Mouse/view sign mismatch:** test named right/left outcomes at both the input
  and rendered camera levels; do not compensate with unrelated axis negations.
- **Delta/rate confusion:** keep mouse angular delta and stick angular rate
  distinct until they become a common angular change.
- **Catch-up multiplication:** transient mouse input must be consumed once even
  when an outer frame advances multiple fixed ticks.
- **One-tick orchestration lag:** apply look before movement resolution so mouse
  yaw affects held W on the same authoritative tick.
- **Feedback orbit:** no automatic yaw alignment in the initial candidate.
- **Double damping:** interpolate the render snapshot once; do not smooth motor,
  camera target, and final view for the same error.
- **Sideways dog:** slow during large desired/body heading disagreement; keep
  exact thresholds provisional and observable.
- **Collision-camera confusion:** gameplay camera obstruction remains deferred;
  do not tune motor behavior to hide a wall-clipping camera.
- **Physical-controller gap:** keep right-stick semantics testable but label feel
  unverified until hardware exists.
- **Future RTS coupling:** keep the dog motor world-intent based so a later order
  system does not depend on gameplay-camera yaw.

Rollback is by coherent phase. Input/state separation and vector-bounded motor
tests should remain useful even if the owner later rejects camera-relative
movement. In that case replace the game-level mapping policy with
character-relative steering; do not restore fixed-world axes or recouple camera
yaw to body yaw.

Deferred follow-ups after base-control acceptance are camera-volume collision,
optional delayed controller auto-alignment, sensitivity/inversion settings,
input remapping, physical gamepad tuning, animation-aware facing, flock-aware
composition, and the separate overhead command-mode experiment.

## Definition of done

This plan's implementation is complete only when:

- the selected player-facing contract is implemented without fixed-world WASD
  or a body-mounted gameplay yaw;
- relative mouse input has tested accumulation, fixed-tick consumption, focus,
  and restart semantics;
- movement-basis, motor, angle-wrap, reversal, collision, restart, camera-mode,
  determinism, and interpolation tests pass;
- the WSL development and ASan/UBSan affected suites plus project formatting and
  static analysis pass;
- a native Windows OpenGL 4.6 build reports zero high-severity messages and the
  existing visual/collision regressions remain valid;
- a keyboard/mouse motion/state packet exists with platform, build, refresh,
  commands, provisional constants, and blank owner verdict;
- the owner records Accept, Revise, or Reject for third-person feel;
- physical-controller, native Linux, auto-alignment, camera collision, and RTS
  command support remain explicitly unverified/deferred; and
- only after verified implementation and owner review are authoritative docs,
  roadmap evidence, and any accepted baseline updated.

## Implementation checkpoint

Phases 0–3 are implemented. Relative mouse deltas have fixed-tick transient
semantics; gameplay yaw/pitch is independent from dog facing; WASD resolves from
camera yaw; the dog motor uses bounded planar velocity changes and reversal
slowdown; and one previous/current dog-camera snapshot is interpolated for
presentation. WSL development and ASan/UBSan suites, format, static analysis,
and the native Windows Release/OpenGL matrix pass.

On 2026-08-16 the owner reported the native keyboard/mouse behavior as good,
including the clarified hard-reversal expectation, and chose to refine it later.
This closes the plan's owner-feel gate as an accepted baseline. It does not make
the provisional sensitivity or motor constants final. Physical-controller feel,
native Linux graphics, camera obstruction, sensitivity/inversion settings,
auto-alignment, and the RTS-style mode remain unverified or deferred.
