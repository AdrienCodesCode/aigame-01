---
id: QA-008
title: Generated influence-debug evidence is committed at the repository root, outside the ignored artifacts tree, and no longer describes HEAD
status: fixed
severity: S3
confidence: confirmed
area: build
reporter: agent
reported: 2026-08-22
phase: 3
platform: windows
rule: docs/DEVELOPMENT_WORKFLOW.md
closed: 2026-08-22
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
[QA-005](QA-005-avoidance-response-is-bang-bang-near-a-face-and-at-the-drop-boundary.md)
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

## Resolution

Fixed on 2026-08-22. Both halves of `## Fix notes` were taken and nothing else:
no engine source, test, scenario definition, oracle, golden, preset, budget,
threshold, or roadmap checkbox changed, and `.gitignore` is the only
configuration file touched.

**1. The misplaced evidence is gone.** `influence-tick-120.png` and
`influence-tick-120.txt` are out of the index and out of the working tree. That
half is already committed: `git log --oneline --all -- influence-tick-120.png`
returns exactly two commits, `ee92cd3 before main pc` (which added the pair) and
`6906770 File QA-013 and QA-014, two test-integrity gaps` (which deleted it),
and `git show --stat 6906770` lists `influence-tick-120.png | Bin 8296178 -> 0
bytes` beside `influence-tick-120.txt | 256 ------`. No evidence was lost: the
correctly placed regeneration still holds all three views under
`artifacts/phase3/2026-08-22/debug-influence-views-native/` — ticks 30, 60, and
120 as PNG plus frame dump, with `review.md`.

**2. The ignore gap is closed.** `.gitignore` gained three root-anchored
patterns, `/*.png`, `/*.txt`, `/*.json`, with a comment recording why the
leading slash must stay. This half is uncommitted in the working tree at the
time of writing, so no `fix:` sha is recorded here.

### Two things `## Fix notes` did not anticipate

**The anchored patterns capture three *tracked* root files.** `## Fix notes`
reasoned about what an *unanchored* rule would reach — `ref/`, `assets/`,
`tests/goldens/`, golden manifests — and concluded that anchoring to the root is
therefore safe. Anchoring is necessary but not sufficient, because three tracked
files live at the repository root and match:
`git ls-tree --name-only HEAD | grep -Ei '\.(png|txt|json)$'` returns exactly
`.mcp.json`, `CMakeLists.txt`, `CMakePresets.json`. `/*.txt` matches
`CMakeLists.txt`, and `/*.json` matches both `CMakePresets.json` **and**
`.mcp.json` — gitignore patterns have no shell-style leading-dot exemption, so a
dotfile is matched like any other name. Measured in a throwaway repository
holding only the three patterns, with untracked files of those names so nothing
was skipped:

```text
.gitignore:2:/*.txt     CMakeLists.txt
.gitignore:3:/*.json    CMakePresets.json
.gitignore:3:/*.json    .mcp.json
.gitignore:1:/*.png     influence-tick-120.png
```

The fix therefore carries three negations — `!/CMakeLists.txt`,
`!/CMakePresets.json`, `!/.mcp.json` — and a comment telling the next person to
add a negation when a new tracked root file of these types lands. Without them
the three files would stay tracked, because an ignore rule untracks nothing, but
a later `git add` of a modification to any of them would be silently skipped.
That is the failure mode the comment exists to prevent.

**`git check-ignore` skips tracked paths, so the obvious verification reports
all clear while the patterns genuinely match.** By default `git check-ignore`
filters paths that are in the index out before matching. The natural way to
check this fix — point the command at the files you are worried about — is
therefore blind to exactly the files the new patterns reach. Observed on git
`2.55.0.windows.3`:

```text
$ git check-ignore -v CMakeLists.txt CMakePresets.json .mcp.json
exit=1            # no output at all: all three are tracked, so all three were skipped

$ git check-ignore -v --no-index CMakeLists.txt CMakePresets.json .mcp.json
.gitignore:34:!/CMakeLists.txt          CMakeLists.txt
.gitignore:35:!/CMakePresets.json       CMakePresets.json
.gitignore:36:!/.mcp.json               .mcp.json
exit=0
```

The first form is the one a reader reaches for, and it is worthless here. Every
ignore check recorded below uses `--no-index`; anyone editing these patterns
again should do the same, because the default form cannot see the mistake it
would need to catch. With `-v --no-index` the printed line is the *last*
matching pattern, so a negation line means "matched, and therefore **not**
ignored" — the exit code is 0 because a pattern matched, not because the path is
excluded.

### Deliberately not done

The stronger alternative named in `## Fix notes` — making the `--capture` and
`--frame-dump` smoke shapes reject a path that resolves inside the repository
but outside `artifacts/`, so a misplaced capture fails the run instead of
landing silently — was **not** implemented. `## Fix notes` assigns it to the
owner because it changes a CLI contract rather than only ignore state, and
prefers the ignore rule alone unless the owner asks for the guard. That is a
recorded decision, not an oversight: if the owner wants the guard it is a
separate change with its own argv shape.

### Evidence

All observed 2026-08-22 on native Windows 11 Home `10.0.26200`, git
`2.55.0.windows.3`, `core.autocrlf=true`, against the existing
`build/Windows/dev` tree configured by the MSVC 2022 Build Tools CMake. No
compile was needed: the change is `.gitignore` plus two deletions.

- **`target:qa-check`** — `cmake --build --preset dev --target qa-check` printed
  `QA tracker check passed: 4 open, 10 closed.` before this closure, and
  `QA tracker check passed: 3 open, 11 closed.` after the status flip, the
  `git mv` into `closed/`, and
  `cmake -DMODE=index -P tools/qa/qa-tracker.cmake`
  (`Wrote docs/qa/INDEX.md — 3 open, 11 closed.`). `docs/qa/INDEX.md` was
  regenerated by the tool and never hand-edited.
- **`wide_eye.influence_debug_frame_dump`** —
  `ctest --preset dev -R wide_eye.influence_debug_frame_dump` reported
  `1/1 Test #21: wide_eye.influence_debug_frame_dump ... Passed 0.02 sec` and
  `100% tests passed, 0 tests failed out of 1`. The generator that produced the
  misplaced evidence is untouched.
- **`manual:git ls-files reports no influence-tick-120.* at the repository
  root`** — `git ls-files | grep -c influence-tick` printed `0`.
  `git ls-files --error-unmatch influence-tick-120.png`, and the same for the
  `.txt`, each exited `1` with
  `error: pathspec '...' did not match any file(s) known to git`. Both are also
  absent from the working tree; `ls` reports `No such file or directory` for
  each.
- **`manual:git check-ignore leaves the four tracked ref/ PNGs and the three
  tracked tests/goldens/ PNGs unignored`** —
  `git ls-files -z '*.png' | git check-ignore --stdin -z --no-index` produced no
  output and exited `1`: nothing matched. Nine tracked PNGs remain, the issue's
  ten minus the removed root file — the two under `assets/readme/`, the four
  under `ref/`, and the three byte-preserved baselines under `tests/goldens/`.
  Naming those seven `ref/` and `tests/goldens/` paths explicitly to
  `git check-ignore -v --no-index` also produced no output and exit `1`.
  Broadened to the whole index,
  `git ls-files | git check-ignore --stdin --no-index` over all 237 tracked
  paths produced no output and exited `1`: **no tracked file anywhere in the
  repository is ignored.**
- **`manual:a capture written to the repository working directory is ignored,
  and git status stays clean after one`** — with `git status --short` reporting
  only ` M .gitignore`, three probes were written at the root:

  ```text
  $ git check-ignore -v influence-tick-999.png state-dump.json frame.txt
  .gitignore:27:/*.png    influence-tick-999.png
  .gitignore:29:/*.json   state-dump.json
  .gitignore:28:/*.txt    frame.txt
  exit=0
  ```

  `git status --short` printed the same single ` M .gitignore` line with the
  three probes present as without them, so a stray capture can no longer be
  swept in by `git add -A`. All three probes were deleted afterwards, and the
  root holds no leftover of those types.
- **`git diff --check`** and **`git diff --cached --check`** — both silent,
  exit `0`. The working-tree copy of this file was converted from LF to
  CRLF during closure so that it matches the checked-out state of its
  siblings on this host, which is what removes the residual
  `LF will be replaced by CRLF` advisory from the `--check` output. With
  `core.autocrlf=true` the stored blob is LF either way, so the committed
  bytes are unaffected and the content diff remains the three
  frontmatter and link edits plus this section.

**One relative link was repaired by the move.** `## Investigation` linked QA-005
as `../closed/QA-005-...`, correct from `open/` and wrong once this file sits in
`closed/`; it is now a same-directory link. Every other relative link in the
file points at `../../` or `../../../` and is unaffected by the move.

**No existing suite would have caught this, and none was added.** Nothing in
CTest asserts that the repository root is free of generated captures, and
`qa-check` is deliberately outside `ctest` so that a docs-schema slip cannot
fail the engine suite. The `--no-index` sweeps above are manual commands, not a
registered test. At S3, with the recurrence path closed by the ignore rule,
extending the suite was judged out of proportion rather than overlooked; the
owner-side capture-path guard described under "Deliberately not done" is the
change that would make this machine-checkable.
