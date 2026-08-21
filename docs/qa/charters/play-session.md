# Charter: play session

**Surface:** the interactive `wide_eye` executable driven by a human — window
lifetime, pointer capture, camera, dog control, and the named version 1
scenarios.

Needs a real OpenGL 4.6 Core context. The WSL development host exposes only
OpenGL 4.5, so a sweep there is a WSL sweep and must be recorded as one; it does
not substitute for native Windows or native Linux evidence.

```bash
./build/Linux/dev/wide_eye
./build/Linux/dev/wide_eye --play-scenario paddock-start
```

## Window and session

- [ ] The window opens, is the expected size, and the paddock is visible without
      any input.
- [ ] Escape toggles pointer capture both ways, and the cursor state matches
      what the program thinks it is.
- [ ] The window closes cleanly on request — no hang, no error on stderr.
- [ ] Resizing the window keeps the scene correctly framed rather than stretched
      or clipped.
- [ ] Nothing in the console reports a GL error or a failed resource load during
      a normal session.

## Camera

- [ ] Mouse orbit follows the mouse in the expected direction, with no snap,
      drift, or accumulation after a long orbit.
- [ ] Tab enters and leaves the free debug camera, and leaving it returns to a
      sane play camera rather than an arbitrary pose.
- [ ] The camera never enters geometry in a way that hides the dog for longer
      than a moment.

## Dog control

- [ ] WASD moves the dog relative to the camera, not to the world.
- [ ] Shift sprint is clearly faster and stops cleanly when released.
- [ ] Direction changes read as the dog turning, not as an instantaneous
      teleport of heading.
- [ ] Wall contact stops the dog without shaking, tunnelling, or climbing.
- [ ] R restarts the scenario to the same visible starting state every time.

## Scenarios

- [ ] `paddock-start` — the flock is where the scenario says it is, and the
      session is playable from the first frame.
- [ ] `wall-contact` — the dog reaches the wall and is stopped by it.
- [ ] `closed-gate` — the gate blocks passage for the dog and the sheep.
- [ ] `open-gate` — passage through the gate works and looks correct on both
      sides.
- [ ] `presentation-motion` — renders as the scripted fixture it is. **Not**
      accepted flock behavior; do not judge herding from it and do not file
      flock-behavior issues against it.

## Flock reading (judgment, not measurement)

- [ ] Sheep motion reads as animal-like rather than as five independent dots.
- [ ] Sheep respond to the dog's approach in a way a human would predict.
- [ ] No sheep jitters in place, vibrates against a neighbor, or slides without
      apparent cause.
- [ ] Nothing visibly pops, overlaps, or passes through another body.

## Sweep log

Record one entry per sweep — date, short sha, preset, platform, GPU and driver,
and the QA ids raised.

| Date | Build | Preset | Platform / GPU / driver | Issues raised |
| ---- | ----- | ------ | ----------------------- | ------------- |
| _(no sweep recorded yet)_ | | | | |
