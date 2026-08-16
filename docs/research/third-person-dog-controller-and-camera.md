# Research: Third-person dog controller and gameplay camera

**Status:** Research complete; the selected camera-relative keyboard/mouse
baseline is implemented and owner-accepted

**Produced by:** Codex

**Date:** 2026-08-16

**Project revision:** `main` at
`b4d5d5c4eb9421d18e74c91911ff4321d72dd41f` with a dirty Phase 2
worktree, including a provisional unaccepted heading-rate adjustment

**Adversarial review:** Not yet reviewed

## Problem and decision

Wide Eye needs a third-person control model in which movement, the dog's facing,
and the camera cooperate without producing abrupt view changes or ambiguous
left/right behavior. The immediate decision is not a tuning constant. It is the
control reference frame and the ownership relationship among:

- the player's two-dimensional movement intent;
- the world-space direction in which the dog accelerates;
- the direction the dog's body faces;
- the camera's orbit/control yaw; and
- the final camera pose presented by the renderer.

The owner directly observed on keyboard that the first implementation jumped
abruptly with WASD. A bounded heading-rate change added some continuity, but the
result remained substantially wrong; left and right appeared to cover only 180
degrees in total. Sprint, camera-mode toggling, and the free-debug camera were
reported as working. No physical controller was available for this test.

The current code explains that result exactly. It treats WASD as four fixed world
directions, makes body heading chase the velocity direction, and derives the
gameplay camera directly from that body heading. A and D therefore target -90
and +90 degrees respectively: two absolute lateral directions separated by 180
degrees. They are not continuous turn commands. Smoothing that mapping cannot
turn it into a steering controller, and mounting the camera to the smoothed body
heading still makes every body turn into a camera orbit.

The game design selects the third-person camera family for the current
experiment but does not yet select its final behavior. This research therefore
recommends a bounded prototype hypothesis and preserves a genuine control-policy
choice for the owner instead of silently declaring the provisional controller
complete.

### Owner scope decision after research

On 2026-08-16, the owner confirmed that direct third-person dog control is one
gameplay option and the current implementation focus. The owner also preserved
a separate RTS-style option for later investigation: a freely controllable
bird's-eye camera with selection and location/action orders. That deferred mode
may suit very large flocks, but its exact mouse, keyboard, selection, and order
model is not yet specified, and neither a 1,000-sheep gameplay requirement nor
its camera readability has been established.

This narrows the purpose of the present research. It should produce a good
direct-control experiment without turning its movement or camera assumptions
into engine-wide rules that would obstruct a later overhead command layer. It
does not authorize implementation of the RTS-style mode.

### Success criteria for the next control experiment

- A new player can predict what W/A/S/D will do from any camera angle.
- Starting, stopping, diagonal movement, reversals, and sprint transitions have
  no discontinuous dog or camera pose.
- Manual camera input and automatic camera alignment do not fight each other.
- Holding a movement input while the camera moves follows one documented rule;
  it does not accidentally spiral because two systems feed back into each other.
- Dog facing remains legible because facing will affect sheep pressure in the
  first playable.
- Identical named inputs and scenario state reproduce the same authoritative dog
  motion at the 60 Hz fixed step.
- Rendering at refresh rates above or between simulation ticks does not expose
  60 Hz stepping as visible judder.

### Non-goals

- Selecting final animation, root motion, lock-on combat, aiming, cinematic
  cameras, or a full accessibility/settings surface.
- Copying a commercial game's exact feel or tuning values.
- Replacing the existing analytic dog collision or importing a camera library.
- Claiming physical-controller quality before testing a real controller.

The owner subsequently selected free mouse orbit, live camera-relative
ground-plane movement, movement-driven dog facing, and no automatic recentering.
The implementation handoff and remaining acceptance gate are recorded in the
[controller/camera plan](../plans/third-person-dog-controller-and-camera.md).

## Verified project constraints at research time

### Pre-correction Wide Eye implementation

**Confirmed fact:** `DogMoveInput` contains only `move_right`, `move_forward`,
and `sprint`. In `DogController::fixed_update`, these axes become world X and
negative world Z velocity targets. Each velocity component approaches its
target independently. The velocity direction then becomes the target body
heading. The dirty worktree currently limits that body turn to 180 degrees per
second using a shortest-angle calculation. The owner has already observed that
this provisional change is insufficient.

**Confirmed fact:** The gameplay `CameraController` has no gameplay orbit state.
Its eye and target are recalculated directly from `DogState::heading_radians` on
every rendered frame. Named look actions affect only the free-debug camera.
Consequently, gameplay camera direction, body facing, and movement direction
are effectively one coupled state.

**Confirmed fact:** The 60 Hz `FixedStepAccumulator` calculates an
`interpolation_alpha`, but `window_runtime` does not pass it to the scenario or
renderer. `render_gameplay_paddock` renders the latest fixed dog state directly.
This can expose fixed-step judder when rendering and simulation ticks are not
phase-aligned, even after the control policy is corrected.

**Confirmed fact:** SDL keyboard and standardized gamepad inputs already enter
gameplay through named actions. The gamepad path uses a fixed 8,000-unit axial
dead zone. SDL documents that thumbsticks are centered within roughly 8,000 of
zero, while warning that actual dead zones vary by controller and advanced
interfaces should configure or detect them. This makes the current value a
reasonable placeholder, not verified controller tuning. See
[SDL `GamepadAxis`](https://wiki.libsdl.org/SDL3/SDL_GamepadAxis), SDL 3.2+
documentation, accessed 2026-08-16.

**Confirmed fact:** The approved first playable relies on the dog's position,
movement, facing, pressure, and release. Facing therefore cannot be dismissed as
pure animation. The broader design selects direct third-person control for the
current experiment while leaving its movement relationship, orbit, alignment,
and tuning unresolved.

### Local first-person reference

The owner asked that `/home/adunix/dev/testing-01` be inspected as a possible
reference. It was clean at commit
`1027b9d077172155dbf56e7e87b2e3fb19f67b9d` on 2026-08-16.

**Confirmed local implementation:** `PlayerController.cs` separates an input
`Vector2`, target look yaw/pitch, and rendered camera yaw/pitch. It uses
shortest-angle deltas, bounded camera lag, adaptive angle damping, and a later
camera update after movement. Its architecture document also requires one owner
of the final camera transform to avoid double damping and transform-order
jitter.

**Qualified finding:** Those separation patterns transfer to Wide Eye. The
actual movement mapping does not: `testing-01` moves along the first-person
player transform's right/forward axes, where player orientation and view intent
are deliberately coupled. Its 0.05/0.16-second damping times and 6/4-degree lag
limits are local tuning, not evidence for a third-person dog camera. This local
repository is a useful implementation reference, not independent proof of good
third-person controls.

## Findings

### 1. “Third-person movement” is a family of control reference frames

**Confirmed fact:** Mark Haigh-Hutchinson's camera-design reference distinguishes
character-relative, screen-relative, camera-relative, world-relative, and
object-relative controls. It warns that instant changes to the active control
reference frame are disorienting and recommends interpolating the frame or
retaining the previous one until the player is reoriented. Source:
[Fundamentals of Real-Time Camera Design](https://media.gdcvault.com/gdc05/slides/GD_Haigh-Hutchinson_FundamentalsReal-TimeCameraDesign.pdf),
GDC 2005, accessed 2026-08-16.

The two serious choices for this prototype are:

- **Camera-relative free-direction movement:** the movement input selects a
  ground-plane direction from the gameplay camera's yaw. The dog turns toward
  that desired direction. Camera orbit is a separate input/state.
- **Character-relative steering:** forward/back moves along the dog's facing;
  left/right continuously changes its heading. This is closer to tank or
  vehicle-like control, although an agile dog motor can turn much faster than a
  vehicle.

The current implementation is neither. It is world-relative free-direction
movement with a body-relative camera. That mismatch explains why W is always
world north while the view moves around the dog.

**Qualified finding:** Camera-relative controls are common because the stick or
WASD direction corresponds to the current view. Unreal's official input example
constructs movement forward/right from the controller camera's yaw, and keeps
turn/look as separate controller input. Source:
[Setting Up User Inputs in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-user-inputs-in-unreal-engine),
UE 5.8 documentation, accessed 2026-08-16.

**Qualified finding:** Character-relative steering remains legitimate when the
game needs persistent forward motion independent of camera changes. Its tradeoff
is learnability and reduced immediacy for lateral placement. A technical
discussion of handcrafted cameras describes both the benefit and the steeper
learning curve of tank controls, as well as a hybrid that holds the old
camera-relative input frame during a camera cut until the stick is released.
Source: [Game Developer, August 2009](https://media.gdcvault.com/GD_Mag_Archives/GDM_August_2009.pdf),
accessed 2026-08-16.

### 2. Movement orientation and control/camera orientation are separate policies

**Confirmed fact:** Unreal exposes movement-orientation and controller-desired
orientation as distinct character policies. `bOrientRotationToMovement` targets
the current movement acceleration, while `bUseControllerDesiredRotation` turns
toward controller/control rotation; both use a rotation rate rather than
requiring an instant snap. Source:
[Unreal `UCharacterMovementComponent`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent)
and
[`ComputeOrientToMovementRotation`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent/ComputeOrientToMovementRotation),
UE 5.8 documentation, accessed 2026-08-16.

This is the important transferable idea, not the Unreal API itself. Wide Eye
needs explicit policy and state for:

```text
named move/look actions
        |
        v
control yaw + planar basis -----> desired world movement
                                        |
                                        v
                              dog motor velocity
                                        |
                                        v
                                  dog body yaw

interpolated dog anchor + camera orbit/recenter state
        |
        v
collision-constrained final camera pose
```

The dog's body can orient toward desired acceleration or movement without
forcing the camera to adopt that yaw on the same tick. Conversely, manual camera
orbit can change the control basis without instantly rotating the dog.

### 3. The current motor also has direction-dependent acceleration

**Confirmed fact:** Wide Eye approaches X and Z velocity independently with the
same per-axis maximum. A diagonal transition can therefore apply up to
`sqrt(2)` times the acceleration magnitude of a cardinal transition. It also
makes a 90- or 180-degree change a component-wise blend rather than one
explicitly designed planar turn.

**Inference:** Some of the remaining “off” feeling can come from this motor even
after camera coupling is removed. The next motor should clamp the magnitude of
the planar velocity delta, with acceleration and deceleration chosen
deliberately. Body rotation should use the shortest angular path. For a dog,
sharp reversals may also need speed reduction while the body turns so the model
does not slide sideways. Exact rates remain a feel-tuning question and should
not be copied from another engine.

### 4. Manual orbit, automatic alignment, and movement mapping must not form an accidental loop

**Confirmed fact:** Freely orbiting a third-person exploration camera with the
right stick is a long-established pattern. Source:
[Creating an Emotionally Engaging Camera in Tomb Raider](https://media.gdcvault.com/gdc2013/slides/822486GDC13_Creating_an_emotionally_engaging_camera_in_Tomb_Raider.pdf),
GDC 2013, accessed 2026-08-16.

**Inference:** For Wide Eye, manual gameplay look should update an independent
camera orbit yaw/pitch through named actions (mouse delta on PC and right stick
when hardware is available). Automatic alignment should be a lower-priority
assist that starts only after a grace period and should be suppressed while the
player is actively looking.

There is still a product choice while movement remains held:

- A **live camera basis** continually remaps the movement vector as an automatic
  camera yaw changes. This produces curved steering and may suit a running dog,
  but can create a self-reinforcing orbit when the player holds lateral input.
- A **latched basis** keeps the world movement direction stable until the input
  returns near neutral, even if the camera automatically recenters. This avoids
  unexpected curves but means the held key may no longer match screen direction
  after the camera has moved.
- A middle policy can inhibit auto-recentering for predominantly lateral input
  or cap how quickly the control basis changes.

No source can select among these based on Wide Eye's feel. The owner report about
left/right makes this a required A/B test, not an implementation detail.

### 5. Camera smoothing needs separate channels and priorities

**Confirmed fact:** Unity Cinemachine 3.1 separates camera position control from
rotation control, supports orbital follow with player input, and allows distinct
tracking and look-at targets. Its third-person-follow documentation commonly
uses an invisible orientation target to decouple camera aim from the visible
player model. Sources:
[Cinemachine Camera](https://docs.unity.cn/Packages/com.unity.cinemachine%403.1/manual/CinemachineCamera.html)
and
[Create a Third Person Camera](https://docs.unity.cn/Packages/com.unity.cinemachine%403.1/manual/ThirdPersonCameras.html),
Cinemachine 3.1, accessed 2026-08-16.

**Confirmed fact:** Cinemachine's Position Composer treats position and rotation
as separate responsibilities and exposes per-axis damping plus dead/soft
composition zones. It warns that predictive lookahead can amplify noisy target
motion into camera jitter. Source:
[Cinemachine Position Composer](https://docs.unity.cn/Packages/com.unity.cinemachine%403.1/manual/CinemachinePositionComposer.html),
Cinemachine 3.1, accessed 2026-08-16.

**Inference:** The first custom camera does not need all of Cinemachine. It does
need the same ownership discipline:

- manual orbit input has priority and should feel direct;
- automatic yaw alignment has a delay, a bounded rate or frame-rate-independent
  damping, and no effect while manual look is active;
- positional follow, yaw alignment, pitch restoration, and collision recovery
  have separate rates;
- camera position follows an interpolated dog anchor, not the visual dog's raw
  head/body animation; and
- exactly one system produces the final view pose.

“Smooth everything with one spring” is not a safe shortcut. It can add lag to
manual input, hide causal dog motion, oscillate, or apply damping twice.

### 6. Camera collision is part of a usable third-person controller

**Confirmed fact:** Unreal's spring arm maintains a desired camera distance,
retracts for collision, supports separate location/rotation lag, a maximum lag
distance, and optional substepping for fluctuating frame rates. Source:
[Unreal `USpringArmComponent`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USpringArmComponent),
UE 5.8 documentation, accessed 2026-08-16.

**Confirmed fact:** Godot's `SpringArm3D` casts a ray or volume along the desired
camera arm, moves children close to the hit with a margin, and notes that the
player collider should be excluded. A shape cast represents camera volume
better than a center ray. Source:
[Godot `SpringArm3D`](https://docs.godotengine.org/en/stable/classes/class_springarm3d.html),
stable documentation, accessed 2026-08-16.

**Inference:** Wide Eye should eventually sweep a small camera collision volume
from its dog-relative pivot toward the desired eye, retract promptly to preserve
line of sight, and restore distance more slowly to prevent popping. This should
query simple analytic gameplay/camera collision rather than voxel render faces,
consistent with the existing collision boundary. It is a later subproblem than
the movement reference frame, but the camera architecture must leave room for
it.

### 7. Fixed simulation needs an interpolated presentation

**Confirmed fact:** A fixed-step simulation rendered without interpolation can
show unpleasant stutter when render and simulation rates are not aligned.
Interpolating previous and current state by the accumulator remainder is a
standard remedy. Source:
[Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep/) by Glenn
Fiedler, 2004, accessed 2026-08-16.

**Confirmed project fact:** Wide Eye already calculates exactly this accumulator
fraction but currently discards it before rendering.

**Recommendation:** Preserve previous and current authoritative dog/motor and
camera-control states, then interpolate position and shortest-path yaw for the
render pose using `interpolation_alpha`. Do not feed the interpolated pose back
into collision, sheep pressure, or deterministic gameplay. This presentation
change is distinct from movement acceleration and camera auto-alignment; each
needs its own test so one filter does not conceal another defect.

### 8. Camera-relative movement changes deterministic replay requirements

**Inference:** If raw move input is converted to world motion using camera yaw,
that yaw becomes part of authoritative input interpretation. Deterministic
scenarios must therefore either:

- update and record a deterministic fixed-step control yaw alongside raw named
  move/look inputs; or
- record the resolved world-space desired movement vector for each tick.

Presentation-only damping and collision distance can remain non-authoritative,
but the control yaw used to resolve movement cannot depend on render timing.
Mouse deltas must be accumulated and consumed deterministically at the input
snapshot boundary rather than applied directly during rendering.

## Options and tradeoffs

| Option | What A/D means | Camera relationship | Benefit | Main risk | Disposition |
| --- | --- | --- | --- | --- | --- |
| Current world-relative free movement | Fixed world west/east | Hard-mounted behind body heading | Minimal code | View and controls disagree; directly reproduces owner complaint | Reject |
| Camera-relative free-direction movement | Screen/camera left and right | Independent manual orbit, optional delayed alignment | Familiar, fast lateral placement, supports wide casts | Live auto-alignment can curve held input; dog may slide unless motor/facing cooperate | Recommended first hypothesis |
| Character-relative steering | Continuously turn dog left/right | Follow/recenter behind dog, with optional manual orbit | Persistent facing, continuous 360-degree turning, natural arcs | Harder for new players; slower lateral correction and camera can feel restrictive | Preserve as A/B alternative |
| Speed-dependent hybrid steering/strafe | Turn at speed, more free-direction movement near rest | Multiple camera policies | Could make an agile dog expressive | More states and edge cases before the basic choice is validated | Defer |
| Fixed/isometric world-relative view | Screen/world plane movement | Camera largely independent of body | Stable overview of flock | Less embodied; final camera direction is unchosen and current close view does not support it | Product alternative, not current fix |

## Recommendation

Use **camera-relative free-direction movement with an independent gameplay
camera orbit** as the first implementation hypothesis, but do not declare it the
final control scheme until it is compared with character-relative steering.
This recommendation has medium confidence because it best supports first-time
comprehension and rapid wide casting, while the owner's left/right expectation
could favor steering.

The minimal coherent model is:

1. Named movement and look actions remain device-independent.
2. A fixed-step `GameplayCameraState` owns control yaw, pitch, orbit distance,
   manual-look activity, and automatic-alignment state.
3. Movement input is normalized and resolved on the ground plane from a named,
   documented control basis. The camera yaw used for that resolution is fixed
   step state, never a render-time transform.
4. A dog motor applies vector-bounded planar acceleration/deceleration and
   analytic collision.
5. Dog body yaw follows the desired movement/acceleration by the shortest path
   at a bounded, tuneable rate. For sharp reversals, speed can fall while the
   body turns rather than allowing obvious sideways sliding.
6. Manual camera orbit overrides automatic alignment. Automatic alignment waits
   after look input and follows a selected dog signal—movement direction or body
   yaw—without directly overwriting either.
7. The renderer interpolates previous/current dog and camera-control state; it
   does not alter gameplay state.
8. A later camera-volume sweep retracts the desired eye around walls and gates,
   with fast obstruction response and slower restoration.

Do not reuse the provisional 180-degree-per-second constant as an accepted feel
decision. It merely proved that shortest-path bounded turning removes the
instant heading snap. Likewise, do not import the first-person camera's tuning
constants; reuse its separation of target and rendered state, shortest-angle
math, lag bounds, and final-transform ownership.

## Failure modes and gotchas

- **Feedback orbit:** camera auto-aligns to dog heading while held camera-relative
  lateral input rotates the desired dog direction again.
- **Double damping:** body heading, camera target yaw, and final camera yaw all
  smooth the same error, making input mushy and hard to tune.
- **Sideways dog:** velocity immediately changes direction while body yaw turns
  slowly, contradicting the dog's visual facing and sheep-pressure direction.
- **Direction-dependent motor:** per-axis acceleration makes diagonal response
  faster than cardinal response.
- **Manual/automatic fight:** auto-center remains active during mouse or right
  stick input.
- **Angle wrap:** ordinary subtraction near -180/180 or 0/360 chooses the long
  turn or jumps. Every angular stage needs shortest-path tests.
- **Render-rate authority:** camera-relative movement uses a presentation yaw
  updated at render frequency, breaking deterministic replay.
- **Fixed-step judder hidden as bad steering:** motor behavior is correct but the
  renderer shows only 60 Hz states on a higher- or mismatched-refresh display.
- **Camera wall pop:** a center ray misses near-plane corners, or obstruction
  recovery expands the camera immediately after a hit.
- **Keyboard-only overfit:** digital WASD has full magnitude and no stick noise;
  it cannot validate radial dead zones, low-magnitude walking, or right-stick
  camera response.
- **Camera too close to dog intent:** always centering behind the dog can hide the
  flock area the player is trying to inspect. Wide Eye may eventually need a
  look target or composition bias informed by flock/destination, but adding that
  before base control works would obscure the current decision.

## Evidence and confidence

| Claim | Basis | Confidence |
| --- | --- | --- |
| The reported 180-degree A/D range follows from the current mapping | Direct code inspection and elementary angle mapping | High |
| Current camera/body coupling causes body turns to orbit the view | Direct code inspection | High |
| Independent movement, body yaw, control yaw, and render pose are required | Project diagnosis plus convergent Unreal, Unity, and camera-design sources | High |
| Render interpolation is missing and can cause visible judder | Direct code inspection plus fixed-step technical reference | High |
| Camera-relative free direction is the best first Wide Eye hypothesis | Design fit and common official engine examples, not a Wide Eye playtest | Medium |
| Character-relative steering would better match the owner's intended feel | Interpretation of one hurried report | Low; owner decision required |
| Exact acceleration, turn, recenter, lag, and dead-zone values | Not measured for this game | Unresolved |
| Gamepad behavior is acceptable | Synthetic SDL tests only; no device test | Unverified |

## Planning handoff

An implementation plan should not begin by changing constants. It should first
record the control-policy decision and create a deterministic A/B scenario that
can exercise both mappings without duplicating the motor or camera.

The broader mode choice no longer blocks this work: third-person direct control
is the current experiment, and overhead command control is deferred. The
remaining questions below apply only within the third-person experiment.

The architecture is ready for planning after the owner answers these feel
questions, ideally after returning with the additional examples already
mentioned:

1. With no camera input, should holding A/D make the dog keep turning through a
   full circle, or move left/right relative to the screen?
2. Should mouse/right-stick input freely orbit the gameplay camera while the dog
   continues independently?
3. If the camera automatically moves while a direction remains held, should the
   dog's world trajectory stay stable until release, or curve with the new view?
4. On a hard reversal, may the dog visibly move sideways while turning, or
   should it slow and carve an arc?

The eventual verification matrix should include:

- cardinal, diagonal, partial-magnitude, and opposite inputs from rest and while
  moving;
- 179/-179-degree and 359/0-degree shortest turns in both directions;
- hold-forward during manual orbit and automatic recenter;
- hold-lateral during automatic recenter to expose feedback or spiraling;
- start, stop, sprint, wall slide, closed gate, restart, and scenario replay;
- 30, 60, 120, and 144 Hz presentation over the 60 Hz fixed simulation;
- camera obstruction at walls, corners, and the gate;
- keyboard/mouse first, then at least one physical gamepad with small stick
  deflections and drift;
- logs that separate desired move, resolved world direction, velocity, body
  yaw, control yaw, final camera yaw, and collision distance.

## References

- Wide Eye local source at commit
  `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f`, dirty Phase 2 worktree,
  inspected 2026-08-16: `dog_controller`, `camera_controller`, named input,
  scenario runner, and fixed-step runtime.
- Owner-provided local first-person reference `/home/adunix/dev/testing-01` at
  commit `1027b9d077172155dbf56e7e87b2e3fb19f67b9d`, inspected 2026-08-16.
- [Fundamentals of Real-Time Camera Design](https://media.gdcvault.com/gdc05/slides/GD_Haigh-Hutchinson_FundamentalsReal-TimeCameraDesign.pdf),
  Mark Haigh-Hutchinson, GDC 2005, accessed 2026-08-16.
- [Game Developer, August 2009](https://media.gdcvault.com/GD_Mag_Archives/GDM_August_2009.pdf),
  camera-control discussion, accessed 2026-08-16.
- [Game Developer, September 2011](https://media.gdcvault.com/GD_Mag_Archives/GDM_September_2011.pdf),
  third-person camera smoothing and obstacle-avoidance discussion, accessed
  2026-08-16.
- [Creating an Emotionally Engaging Camera in Tomb Raider](https://media.gdcvault.com/gdc2013/slides/822486GDC13_Creating_an_emotionally_engaging_camera_in_Tomb_Raider.pdf),
  GDC 2013, accessed 2026-08-16.
- [Unreal Engine: Setting Up User Inputs](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-user-inputs-in-unreal-engine),
  UE 5.8 documentation, accessed 2026-08-16.
- [Unreal Engine `UCharacterMovementComponent`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent),
  UE 5.8 documentation, accessed 2026-08-16.
- [Unreal Engine `USpringArmComponent`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USpringArmComponent),
  UE 5.8 documentation, accessed 2026-08-16.
- [Unity Cinemachine Camera](https://docs.unity.cn/Packages/com.unity.cinemachine%403.1/manual/CinemachineCamera.html),
  Cinemachine 3.1, accessed 2026-08-16.
- [Unity Cinemachine third-person camera](https://docs.unity.cn/Packages/com.unity.cinemachine%403.1/manual/ThirdPersonCameras.html),
  Cinemachine 3.1, accessed 2026-08-16.
- [Unity Cinemachine Position Composer](https://docs.unity.cn/Packages/com.unity.cinemachine%403.1/manual/CinemachinePositionComposer.html),
  Cinemachine 3.1, accessed 2026-08-16.
- [Godot `SpringArm3D`](https://docs.godotengine.org/en/stable/classes/class_springarm3d.html),
  stable documentation, accessed 2026-08-16.
- [SDL `GamepadAxis`](https://wiki.libsdl.org/SDL3/SDL_GamepadAxis), SDL 3.2+
  documentation, accessed 2026-08-16.
- [Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep/),
  Glenn Fiedler, 2004, accessed 2026-08-16.

## Implementation outcome

The selected control experiment was planned, implemented, and verified through
the repository's automated and native Windows gates. On 2026-08-16 the owner
reported the keyboard/mouse behavior as good enough to continue and deferred
refinement. Treat the control policy as the accepted baseline while keeping
sensitivity, motor tuning, physical-controller feel, and the other named
follow-ups provisional or deferred.
