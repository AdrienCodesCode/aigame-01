---
id: QA-008
title: Generated influence-debug evidence is committed at the repository root, outside the ignored artifacts tree, and no longer describes HEAD
status: open
severity: S3
confidence: confirmed
area: build
reporter: agent
reported: 2026-08-22
phase: 3
platform: windows
rule: docs/DEVELOPMENT_WORKFLOW.md
verify:
  - target:qa-check
  - wide_eye.influence_debug_frame_dump
  - manual:git ls-files reports no influence-tick-120.* at the repository root
  - manual:git check-ignore leaves the four tracked ref/ PNGs and the three tracked tests/goldens/ PNGs unignored
  - manual:a capture written to the repository working directory is ignored, and git status stays clean after one
---

## Symptom

Two generated evidence files sit tracked at the repository root:

```text
influence-tick-120.png   8296178 bytes in the working tree
influence-tick-120.txt     60253 bytes in the working tree (59997 as the stored blob)
```

They are the PNG capture and the frame dump of the influence debug view for
`sheep-all-influences-diagnostic` at tick 120. Generated evidence belongs under
the ignored `artifacts/` tree, so neither should be in version control at all,
and the numbers inside the `.txt` no longer match what the same command produces
at HEAD.

Observed result (2026-08-22, native Windows 11 Home 10.0.26200, MSVC
19.44.35228.0, NVIDIA RTX 5070 Ti, `build/Windows/dev`):

- `git log --oneline -1 -- influence-tick-120.png` reports `ee92cd3 before main
  pc`. The same commit added the `.txt`; `git show --stat ee92cd3` lists both
  among its 39 changed paths.
- `git check-ignore -v influence-tick-120.png` exits `1` with no output, and the
  same for the `.txt`: nothing in [`.gitignore`](../../../.gitignore) matches
  either. `git ls-files --error-unmatch` succeeds for both, so both are tracked.
- Regenerating the dump with the command the registered CTest
  `wide_eye.influence_debug_frame_dump` uses, writing outside the repository:

  ```text
  build/Windows/dev/wide_eye.exe --influence-debug-dump \
      sheep-all-influences-diagnostic --tick 120 --frame-dump <scratch>/verify-dump.txt
  ```

  produced `influence_debug_dump_result=pass` and sha256
  `d330734a04e33049...`. The committed blob's sha256 is `8ff924f7c284ff27...`.

The correctly placed regeneration of all three views already exists at
`artifacts/phase3/2026-08-22/debug-influence-views-native/` (ticks 30, 60, 120),
so the root copies carry no evidence that is not also held somewhere legitimate.

## Investigation

**Observed result -- the two files are unreferenced.** A recursive `grep` for
`influence-tick-120` across `*.md`, `*.ps1`, `*.cmake`, `*.txt`, and `*.json`,
excluding `build/` and `artifacts/`, returns nothing. No test, script, manifest,
roadmap entry, or document reads either file. They are orphans, not inputs.

**Observed result -- how they got in.** [`.gitignore`](../../../.gitignore)
scopes generated evidence by *directory* (`artifacts/`, `captures/`,
`profiles/`) plus four extensions (`*.rdc`, `*.profraw`, `*.profdata`, `*.dmp`).
A `--capture` or `--frame-dump` path given relative to the shell's working
directory therefore lands in an unignored location whenever that directory is
the repository root, and a subsequent `git add -A` stages it. The
`--influence-debug-smoke <scenario> --tick <tick> --capture <png-path>
--frame-dump <txt-path>` shape in
[`main.cpp`](../../../src/platform/main.cpp#L176-L180) accepts exactly such a
path. The registered CTest uses the four-argument dump shape, which writes to
stdout, so no automated run creates these files.

**Observed result -- the `.txt` does not describe HEAD.** Comparing the stored
blob (`git show HEAD:influence-tick-120.txt`) against this host's regeneration,
`diff` reports a single hunk, `27,256c27,256`. Lines 1-26 -- the seventeen
`influence_debug_*` header fields, the eight `influence_lane` colour
definitions, and the `dog` line -- are byte-identical. Lines 27-256 -- the
`flock` aggregate, the five `sheep` records, and all 224 `segment` records --
all differ. The sheep quantities diverge in the third or fourth decimal place;
for example sheep 1 is `x=17.364117133413909 z=16.954854898782845` committed
against `x=17.363238668125028 z=16.9364520536237` regenerated.

**Hypothesis tested and rejected: this is a determinism defect.** The narrowest
thing that would have to be true for a host or toolchain arithmetic difference
to explain the divergence is that it disturbs *some* long double-precision
accumulation in the frame, not only the sheep. It does not. The `dog` line
reproduces to all seventeen printed digits, and it is not a constant echoed back
from the scenario: `sheep-all-influences-diagnostic` starts the dog at
`x = 16.0, z = 27.0, heading = 0.0`
([`gameplay_scenario.cpp:598-599`](../../../src/game/gameplay_scenario.cpp#L598-L599)),
while the dump reports `x=17.5241575127536 z=22.91828230330664
heading=-2.0172062767526717` at tick 120. That line is the product of 120 ticks
of integration, and it matches exactly.

Inference, with its limit named: the dog controller reads no sheep state, so the
dog path exercises different arithmetic from the sheep steering terms, and this
is strong evidence rather than proof. This issue accordingly makes no
determinism claim in either direction. [`ROADMAP.md`](../../../ROADMAP.md)
records determinism as proven on one host and one architecture -- WSL Ubuntu
24.04.4 on x86-64 under Clang 18.1.3 and GNU 13 -- and explicitly declines a
cross-platform claim; nothing here changes that. **Do not reopen this as a
determinism investigation.**

**Observed result -- what does explain it.**
`git log --oneline -- src/game/sheep_rules.cpp` returns exactly two commits,
`4afdd6b` and `ee92cd3`, with `ee92cd3` the most recent, so no later commit can
account for the difference. `git show ee92cd3 -- src/game/sheep_rules.cpp` shows
that commit rewriting
[`apply_sheep_avoidance`](../../../src/game/sheep_rules.cpp#L424): the ungraded
push became a graded closing-speed response,
`closing_scale = min(1, 2 * closing_speed * direction_length / reference_speed)`
with `reference_speed = sqrt(maximum_acceleration * look_ahead_distance)`
([lines 437, 473, and 492](../../../src/game/sheep_rules.cpp#L437)), and the
point `ground_height` drop probe became `approaching_ground_boundary`. That is
the
[QA-005](../closed/QA-005-avoidance-response-is-bang-bang-near-a-face-and-at-the-drop-boundary.md)
fix. The diagnostic scenario sets `.sheep_avoidance = {.enabled = true}`
([`gameplay_scenario.cpp:870`](../../../src/game/gameplay_scenario.cpp#L870)),
so it runs the changed rule.

Inference: the committed dump is pre-QA-005 output, committed by the same commit
that landed the QA-005 fix -- generated earlier in that working session and
swept in with everything else. The PNG is stale for the same reason; `cmp`
against the correctly placed regeneration reports the two files differing from
byte 1218167.

**Observed result -- the committed bytes are not preserved either.**
`core.autocrlf` is `true` on this host, and nothing in
[`.gitattributes`](../../../.gitattributes) covers the repository root, so the
`.txt` is treated as normalizable text: the stored blob uses LF (59997 bytes,
sha256 `8ff924f7c284ff27...`) while the checked-out working-tree file uses CRLF
(60253 bytes, sha256 `afd42671275eb1a5...`). A hash-addressed evidence file
whose hash depends on which platform checked it out is not usable as evidence.
`.gitattributes` protects exactly this property for `tests/goldens/**` and
`third_party/glad/**`; a capture at the root gets none of it.

**Observed result -- no manifest.**
[`docs/DEVELOPMENT_WORKFLOW.md`](../../DEVELOPMENT_WORKFLOW.md#L227) requires an
evidence packet to carry a manifest naming commit and worktree state, preset,
OS, CPU, GPU, driver, scenario and replay versions, seed, tick, and the commands
run. The root files carry none, so which host, toolchain, or source revision
produced them is not recoverable from the repository. The staleness above had to
be established by re-running rather than by reading a manifest.

**Falsified my own framing -- this is not a repository-size problem.** The PNG
is 8296178 bytes in the working tree, which invites a history-bloat argument.
`git cat-file --batch-check '%(objectsize:disk)'` reports its packed size as
**58439 bytes**, and the `.txt` as 7738. The pack is 19.01 MiB in total, and the
four largest blobs in history are all `assets/readme/` and `ref/` images between
2.0 and 4.2 MB. The cost of this defect is misleading evidence and a broken
rule, not disk.

**Trap for the fixer: a blanket `*.png` rule would be wrong.**
`git ls-files '*.png'` returns 10 tracked PNGs: 2 in `assets/readme/` (README
screenshots), 4 in `ref/` (ideation references that
[`AGENTS.md`](../../../AGENTS.md) directs be preserved), 3 under
`tests/goldens/` (accepted baselines whose exact bytes `.gitattributes`
deliberately protects and CTest hash-verifies), and the 1 at the root. Nine of
the ten are legitimate. Any new ignore rule must be anchored so it cannot reach
`ref/`, `assets/`, or `tests/goldens/`.

## Root cause

`.gitignore` excludes generated evidence by naming the directories it is
*supposed* to be written to. It has no rule for evidence written to the wrong
place, so a capture emitted with a relative path from the repository root is
neither ignored nor flagged, and `git add -A` commits it silently. Commit
`ee92cd3` did that with an influence-debug capture and frame dump that had
already been superseded, within that same commit, by its own rewrite of
`apply_sheep_avoidance`.

## Expected behavior

[`CLAUDE.md:145-146`](../../../CLAUDE.md#L145-L146) states that generated
evidence goes under `artifacts/<phase>/<date>/`, which is gitignored.
[`docs/DEVELOPMENT_WORKFLOW.md:227`](../../DEVELOPMENT_WORKFLOW.md#L227) states
that generated evidence belongs under the ignored `artifacts/` tree until
explicitly accepted, and that an accepted packet carries a versioned manifest
with hashes, commit and preset, platform and GPU, scenario and tick, and the
commands run. [`docs/qa/README.md`](../README.md) and `.gitattributes` add that
accepted baselines live under `tests/goldens/` with their bytes preserved.

A generated capture is therefore in exactly one of two legitimate states:
ignored under `artifacts/`, or promoted into `tests/goldens/` with a manifest
and an owner accept. The repository root is neither. Separately,
[`AGENTS.md`](../../../AGENTS.md) requires an observed result to name its build,
date, platform, and method; an unmanifested tracked dump cannot satisfy that for
any reader who later finds it.

## Fix notes

Two independent halves. Both are small, and neither touches engine code.

1. **Remove the misplaced evidence.** `git rm` both root paths. Nothing
   references them, and the same three views exist correctly placed under
   `artifacts/phase3/2026-08-22/debug-influence-views-native/`, so no evidence
   is lost. If the owner wants tick-120 evidence retained for the record, the
   route is a manifested packet under `artifacts/`, not the root -- and it must
   be a regeneration at HEAD, because the committed numbers predate the QA-005
   fix.
2. **Close the ignore gap so it cannot recur.** Anchor a root-only rule: a
   leading `/` binds a `.gitignore` pattern to the repository root, leaving
   `ref/`, `assets/`, and `tests/goldens/` untouched. Do **not** add an
   unanchored `*.png`, `*.txt`, or `*.json` rule -- nine of the ten tracked PNGs
   are legitimate and three of those are hash-verified goldens, and unanchored
   text or JSON patterns would reach golden manifests, `CMakePresets.json`, and
   the `compile_commands.json` entry already handled by name.

   A stronger alternative, which belongs to the owner because it changes a CLI
   contract rather than only ignore state: make the smoke shapes reject a
   capture path that resolves inside the repository but outside `artifacts/`,
   turning a silent mistake into a failed run. Prefer the ignore rule alone
   unless the owner asks for the guard.

Blast radius: no gameplay rule, scenario, oracle, golden, or accepted
measurement. `.gitignore` is the only configuration file that needs to change,
and a wrong pattern there is the one way this fix can do damage -- hence the
`check-ignore` entries in `verify`.

Suites that must pass: `target:qa-check` for the tracker itself, and
`wide_eye.influence_debug_frame_dump` to confirm the generator that produced the
misplaced evidence is untouched and still reports
`influence_debug_dump_result=pass`. No full-suite run is implicated. Also run
`git diff --check`, and confirm on a clean tree that `git status --short`
reports nothing after a capture is written into the working directory.
