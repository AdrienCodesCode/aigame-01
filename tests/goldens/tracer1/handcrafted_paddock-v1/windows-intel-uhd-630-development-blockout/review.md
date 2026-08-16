# Tracer 1 accepted handcrafted paddock baseline

The project owner explicitly accepted this packet on 2026-08-16. It is the
first visual baseline for the named Tracer 1 scenario, blockout profile, and
reference machine.

## Review question

- **Coherent outcome:** Render one bounded handcrafted paddock through the
  verified naive voxel mesh.
- **Question for the owner:** Are the ground, stone wall, red gate, and distant
  barn all immediately readable as the intended first environment blockout?
- **What intentionally changed:** The default interactive path and the
  `--paddock-smoke` scenario render a static four-chunk scene with a fixed
  camera, six flat material colors, depth testing, and back-face culling.
- **What must remain invariant:** Tracer 0 cube pixels,
  renderer/platform/scenario ownership, checked naive meshing, and the
  separation of opaque, cutout, and translucent CPU outputs.
- **Known limitations or unverified claims:** This packet does not claim final
  art direction, lighting, shadows, fog, procedural terrain, camera behavior,
  collision, gameplay, motion quality, frame-time performance, memory budgets,
  cross-GPU pixel identity, player understanding, or fun.

## Reproduction metadata

| Field | Value |
| --- | --- |
| Date/time and timezone | 2026-08-16 12:35:03 +07 |
| Commit and dirty-worktree state | `b4d5d5c4eb9421d18e74c91911ff4321d72dd41f`; dirty source-hashed copy |
| Configure/build preset | `dev` / `dev` |
| Executable version | Wide Eye 0.1.0 |
| OS and architecture | Microsoft Windows 11 Pro 10.0.26200; AMD64 |
| CPU | Intel Core i9-8950HK |
| GPU and driver | Intel UHD Graphics 630; 27.20.100.9664 |
| OpenGL and GLSL versions | OpenGL 4.6.0 / GLSL 4.60, build 27.20.100.9664 |
| Scenario and version | `handcrafted_paddock` v1 |
| Replay and version | Not applicable; static scene |
| Seed | Not applicable; handcrafted scene |
| Simulation tick/rate | No behavior tick sampled; runtime fixed rate remains 60 Hz |
| Camera and viewport | `tracer1_fixed_paddock_blockout`; 960×540 |
| Graphics profile and relevant flags | `development-blockout`; `--paddock-smoke --capture` |
| Exact reproduction command | `wide_eye.exe --paddock-smoke --capture <output-path>` on the named native Windows build |
| Artifact-manifest path/hash | manifest.json / b3825afde591210c855f3f779c822899d5311aeb1d2ea1f5557fd329ebd1ec93 |

## Automated evidence

| Check or budget | Command/configuration | Result | Evidence path |
| --- | --- | --- | --- |
| Focused scene test | `wide_eye_handcrafted_paddock_tests` | Pass: 4 chunks, 1,746 occupied blocks, 2,754 faces, 11,016 vertices, 16,524 indices | `tests/handcrafted_paddock_tests.cpp` |
| WSL development suite at capture | `ctest --preset dev` | 13/13 pass | Candidate packet |
| WSL ASan/UBSan suite at capture | `ctest --preset dev-sanitized` | 13/13 pass; 12 sanitizer-labeled | Candidate packet |
| Native Windows suite at capture | `ctest --preset dev` | 22/22 pass, including paddock render and two-run capture | Candidate packet |
| GL diagnostics | Paddock direct capture on Intel UHD 630 | Center oracle pass; zero high-severity messages | `manifest.json` |
| Repeat capture | Two direct 960×540 PNGs | Byte-identical SHA-256 `173238274346f39ce3a5fae87e2524e515cb65636302e0e2c3541cf0eaec92d2` | Candidate packet |
| Tracer 0 regression | Native normal/debug cube captures | Accepted hashes unchanged | Candidate packet |
| Format/static analysis at capture | Project-only checked-in targets | Both pass | Candidate packet |
| Post-promotion WSL suites | `ctest --preset dev`; `ctest --preset dev-sanitized` | 14/14 pass in each; 13 sanitizer-labeled | Current promotion verification |
| Post-promotion integrity and source checks | Registered baseline CTest; format; bounded static analysis | All pass | Current promotion verification |

Frame-time and memory measurements were intentionally not run because this
outcome retires the first geometry/upload risk; their roadmap item remains
unchecked. WSL OpenGL rendering was unavailable: the required 4.6 context
failed at `context_create` with `GLXBadFBConfig` before scene initialization.

## Artifacts

| Artifact | Required when | Path | What to inspect |
| --- | --- | --- | --- |
| Accepted normal frame | Static presentation matters | `normal-frame.png` | Ground extent, wall continuity, red gate contrast, barn silhouette, fixed-camera framing |
| Matching debug frame | Later Phase 2 debug-view outcome | Not produced | Intentionally deferred; no chunk-debug view exists yet |
| Motion evidence | Motion/temporal behavior matters | Not applicable | Static scene only |
| State and metrics | Geometry correctness matters | `manifest.json` | Chunk/block/face/vertex/index counts and pass state |

## Agent pre-review

Observed result: the 960×540 candidate visibly contains one large green ground
plane, a continuous light-stone wall, a saturated red gate centered between
raised posts, and a stepped dark-roof barn with a contrasting doorway behind
the wall. The fixed view keeps all required landmarks distinct. No missing
faces, reversed winding, obvious depth failure, or magenta unknown-material
fallback was visible. This observation is separate from the owner's verdict.

## Owner review

- [x] The packet answers the stated question rather than showcasing unrelated
  polish.
- [x] Scenario, camera, viewport, and profile are correctly recorded.
- [x] Ground, wall, gate, and barn geometry are readable.
- [x] The lack of lighting, fog, shadows, debug views, motion, and gameplay is
  acceptable for this blockout outcome.

### Verdict

Select exactly one:

- [x] **Accept** - promote the named candidate artifact to the accepted
  baseline for this scenario/profile.
- [ ] **Revise** - keep no accepted baseline; the candidate needs the changes
  described below.
- [ ] **Reject** - keep no accepted baseline and abandon or redesign this
  candidate direction.

**Owner observation and required follow-up:** Accepted the named handcrafted
paddock blockout without requesting a visual revision. Continue with the
bounded palette, lighting, sky, fog, and shadow presentation pass.

**Owner/date:** Project owner / 2026-08-16 (Asia/Bangkok)

## Baseline rule

This explicit **Accept** verdict promotes `normal-frame.png` as the first
reference-machine baseline for this scenario/profile. It does not establish
cross-GPU pixel identity, final-art approval, player understanding, or any
feature intentionally deferred above.
