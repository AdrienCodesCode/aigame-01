# Tracer 0 accepted cube visual baseline

The project owner explicitly accepted this packet on 2026-08-15. It is the
first visual baseline for the named scenario, profile, and reference machine.

## Review question

- **Coherent outcome:** Reproducibly render and explain the Tracer 0 perspective voxel cube on the approved native Windows OpenGL 4.6 path.
- **Question for the owner:** Is the colored perspective cube, together with its matching wireframe diagnostic, acceptable as the first visual baseline for Tracer 0?
- **What intentionally changed:** The normal cube presentation is unchanged. A same-camera wireframe diagnostic and this review record were added.
- **What must remain invariant:** The 64x64 viewport, fixed camera, cube geometry, normal face colors, depth test/write state, deterministic PNG capture, and zero high-severity GL result.
- **Known limitations or unverified claims:** This is a static engineering tracer at 64x64, not representative game art. The wireframe exposes submitted triangle edges, including face diagonals; it is not a voxel/chunk or collision view. Motion and performance are outside this review question. There was no prior accepted baseline before this verdict, and native Linux remains unverified.

## Reproduction metadata

| Field | Value |
| --- | --- |
| Date/time and timezone | 2026-08-15 22:06:42 +07:00 |
| Commit and dirty-worktree state | 3793e036e72ec04f777304d7d2d846009a39c288; worktree dirty |
| Configure/build preset | dev / dev |
| Executable version | Wide Eye 0.1.0 |
| OS and architecture | Microsoft Windows 11 Pro 10.0.26200 build 26200; AMD64 |
| CPU | Intel(R) Core(TM) i9-8950HK CPU @ 2.90GHz |
| GPU and driver | Intel(R) UHD Graphics 630 driver 27.20.100.9664; NVIDIA GeForce GTX 1050 Ti with Max-Q Design driver 32.0.15.8157; active renderer Intel(R) UHD Graphics 630 |
| OpenGL and GLSL versions | 4.6.0 - Build 27.20.100.9664; GLSL 4.60 - Build 27.20.100.9664 |
| Scenario and version | voxel_cube_smoke, version 1 |
| Replay and version | Not applicable; static smoke scenario |
| Seed | Not applicable |
| Simulation tick/rate | No simulation tick; fixed-step configuration 60 Hz |
| Camera and viewport | tracer0_fixed_perspective; 64x64 |
| Graphics profile and relevant flags | development; normal and wireframe_debug captures |
| Exact reproduction command | powershell.exe -NoProfile -ExecutionPolicy Bypass -File "\\wsl.localhost\Ubuntu\home\adunix\dev\aigame-01\tools\phase1\run-window-smoke.ps1" |
| Artifact-manifest path/hash | manifest.json / 378d2c8cf2a5ec02e5b543311b70bf1a935e4191b24f7cee1b0a6ce84f0b8634 |

## Automated evidence

| Check or budget | Command/configuration | Result | Evidence path |
| --- | --- | --- | --- |
| Focused tests | ctest --preset dev | Pass | run.log |
| Scenario/replay | Normal and wireframe debug capture commands | Pass; matching scenario/camera/viewport | normal-frame.png, debug-frame.png, state.json |
| GL diagnostics | OpenGL 4.6 Core debug callback | Pass; zero high-severity messages | run.log, state.json |
| Sanitizers, if required | Not run by this native Windows packet | Intentionally separate; WSL sanitizer coverage cannot execute this host's GL 4.6 path | Not included |
| Frame-time/memory, if required | Not required for this static Tracer 0 visual question | Not run | Not included |

## Artifacts

| Artifact | Required when | Path | What to inspect |
| --- | --- | --- | --- |
| Normal frame | Static presentation matters | normal-frame.png | Perspective, silhouette, three readable colored faces, dark clear color |
| Matching debug frame | Geometry diagnosis matters | debug-frame.png | Same silhouette/camera and visible triangle edges, including face diagonals |
| Short clip or contact sheet | Motion/temporal behavior matters | Not applicable | Static tracer; no motion claim |
| Accepted before frame/clip | A prior approved baseline exists | None | This packet creates the first accepted baseline |
| Accepted frame/debug view | The visible result changed | normal-frame.png, debug-frame.png | Accepted normal output plus explanatory diagnostic |
| State and metrics dump | Behavior/performance matters | state.json, configuration.json | Scenario, viewport, depth state, hashes, GL/GLSL configuration |
| Relevant logs | Warnings/errors matter | run.log | Commands, CTest results, capture hashes, GL diagnostics |

## Owner review

Review the normal and diagnostic evidence together.

- [x] The packet answers the stated question rather than showcasing unrelated polish.
- [x] Scenario, seed, tick, camera, viewport, and profile are comparable.
- [x] The perspective, geometry, face separation, and silhouette are readable at the captured size or a nearest-neighbor zoom.
- [x] The wireframe explains the submitted cube triangles without being mistaken for player-facing presentation.
- [x] The zero-high-severity GL result and deterministic capture evidence are sufficient for this tracer.
- [x] Known limitations are acceptable for Tracer 0.

### Verdict

Select exactly one:

- [x] **Accept** - promote the named candidate artifacts to the accepted baseline for this scenario/profile.
- [ ] **Revise** - keep no baseline; the candidate needs the changes described below.
- [ ] **Reject** - keep no baseline and abandon or redesign this candidate direction.

**Owner observation and required follow-up:** The interactive cube rendered in
a resizable native Windows window, and both the normal and wireframe screenshots
looked correct. No visual revision was requested.

**Owner/date:** Project owner / 2026-08-15 (Asia/Bangkok)

## Baseline rule

This explicit **Accept** verdict promotes the named artifacts as the first
reference-machine baseline for this scenario/profile. It does not establish
cross-GPU pixel identity, player understanding, or production-art approval.
