---
id: QA-009
title: Every native Windows evidence runner aborts on a benign OpenGL notification message, because Windows PowerShell 5.1 turns any engine stderr into a terminating error
status: open
severity: S1
confidence: confirmed
area: tools
reporter: agent
reported: 2026-08-22
phase: 3
platform: windows
rule: docs/DEVELOPMENT_WORKFLOW.md
verify:
  - manual:tools/phase3/run-visual-feasibility-baseline.ps1 completes on the RTX 5070 Ti reference desktop and writes a packet whose run.log ends result=pass
  - manual:that packet still records gl_debug_high_severity_messages=0 parsed from engine stdout, and its run.log still contains the notification-severity gl_debug_message lines
  - manual:run-visual-feasibility-baseline.ps1 still fails when the engine exits non-zero (inject with a bad argv or --context-smoke-inject-high-severity)
  - manual:tools/phase0/run-context-smoke.ps1 still reports result=pass on native Windows
  - manual:tools/phase1/run-window-smoke.ps1 completes on native Windows
  - manual:tools/phase2/run-tracer1-review.ps1 completes on native Windows
  - manual:tools/phase3/run-tracer2-presentation-review.ps1 completes on native Windows
  - wide_eye.opengl_debug_high_severity
  - wide_eye.opengl_voxel_cube_smoke
  - target:qa-check
---

## Symptom

The reference-desktop Phase 0 visual-feasibility baseline cannot be produced on
the machine the owner designated as the reference desktop. The runner aborts on
an OpenGL message that is explicitly *not* a problem.

Observed result, 2026-08-22, native Windows 11 Home 10.0.26200, MSVC
19.44.35228.0, NVIDIA GeForce RTX 5070 Ti driver 32.0.15.9186, OpenGL 4.6.0
NVIDIA 591.86:

```text
powershell.exe -NoProfile -ExecutionPolicy Bypass \
    -File .\tools\phase3\run-visual-feasibility-baseline.ps1 \
    -Width 2560 -Height 1440 -RefreshHz 60
```

exited `1`. It passed `configure-release`, `build-release`, `ctest-release`
(`100% tests passed, 0 tests failed out of 47`), and
`visual-tracer-configuration`, then failed at `representative-normal` -- the
first stage that opens an OpenGL context -- with:

```text
failure_stage=visual_feasibility_baseline_runner
failure_message=gl_debug_message severity=notification source=api type=other id=131185 text=Buffer detailed info: Buffer object 1 (bound to GL_ARRAY_BUFFER_ARB, usage hint is GL_STATIC_DRAW) will use VIDEO memory as the source for buffer object operations.
```

The retained failure packet is
`artifacts/phase3/2026-08-22/visual-feasibility-baseline-180412745/`
(`manifest.json`, `inventory.json`, `run.log`, `measurements.json`, and a
`review.md` whose owner verdict boxes are correctly left unticked and whose
observed-value rows are correctly blank). The failure text is at `run.log:292`.

The reported "failure" is a notification-severity driver message. The same
engine process exits `0` and reports `gl_debug_high_severity_messages=0`.

## Investigation

**Observed result -- the engine is behaving as designed.**
[`gl_debug_callback`](../../../src/platform/window_runtime.cpp#L126) counts a
message into the state the engine reports only when
`severity == GL_DEBUG_SEVERITY_HIGH`
([`window_runtime.cpp:130-132`](../../../src/platform/window_runtime.cpp#L130-L132)),
then unconditionally writes every message, at every severity, to `std::cerr`
([`window_runtime.cpp:134-142`](../../../src/platform/window_runtime.cpp#L134-L142)).
That split is deliberate: the counted total is the assertion, the printed text
is the diagnostic trail. `id=131185` is a notification, so it is printed and not
counted.

**Observed result -- the two streams, measured directly.** Same host and build
(`build/Windows/release/wide_eye.exe`, the Release tree the failed run itself
produced), redirecting each stream to its own file:

| command | exit | `gl_debug_message` on stdout | on stderr |
| --- | --- | --- | --- |
| `--voxel-cube-smoke` | 0 | 0 | 4 |
| `--triangle-smoke` | 0 | 0 | 4 |
| `--paddock-smoke` | 0 | 0 | 7 |
| `--sheep-motion-render-smoke` | 0 | 0 | 7 |
| `--influence-debug-dump sheep-all-influences-diagnostic --tick 120` | 0 | 0 | 0 |

Every GL-context mode reported `gl_debug_high_severity_messages=0` on stdout
alongside `gl_renderer=NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2` and
`gl_version=4.6.0 NVIDIA 591.86`. The headless dump wrote 263 stdout lines and
zero stderr bytes, which is why no headless path and no CTest ever tripped this.

**Observed result -- what actually throws.** All five runners set
`$ErrorActionPreference = "Stop"` and invoke the native executable with `2>&1`.
Under Windows PowerShell 5.1 (`PSEdition=Desktop`, `PSVersion=5.1.26100.9168` on
this host) each stderr line from a native command becomes an `ErrorRecord` with
`FullyQualifiedErrorId=NativeCommandError`, and `Stop` promotes the first one to
a terminating exception -- regardless of the process exit code. Against
`wide_eye.exe --voxel-cube-smoke` in a scratch script, **both** call forms used
by the runners threw, and both threw the identical text recorded in the failure
packet:

```text
A_THREW type=System.Management.Automation.RemoteException   # $o = & $exe ... 2>&1
B_THREW type=System.Management.Automation.RemoteException   # & $exe ... 2>&1 | ForEach-Object {...}
errorId=NativeCommandError
```

**Falsification attempted, and it held.** The narrowest thing that must be true
for this theory is that the *preference*, not the redirection and not the
executable, is what converts benign output into an abort. Re-running the
identical assignment form with `$ErrorActionPreference = "Continue"` around it
alone: no throw, `exit=0`, 35 merged records (31 stdout plus 4 stderr). The
redirection and the executable are unchanged; only the preference differs.

**Observed result -- the failing call site.** `Capture-VisualTracer`
([`run-visual-feasibility-baseline.ps1:105-113`](../../../tools/phase3/run-visual-feasibility-baseline.ps1#L105-L113))
calls `Invoke-Checked` without `-WorkingDirectory`, so the engine takes the
direct branch at
[line 39](../../../tools/phase3/run-visual-feasibility-baseline.ps1#L39), and
the throw escapes `Invoke-Checked` before its own `$LASTEXITCODE` check at
[line 62](../../../tools/phase3/run-visual-feasibility-baseline.ps1#L62) can
run. The outer `catch` then writes the stderr text as `failure_message`, which
is why the packet blames a notification instead of naming a stage failure.

**Observed result -- scope is all five runners, not one.** Every PowerShell
script under `tools/` sets `Stop` and merges native stderr:

| runner | form | line(s) | GL-context engine modes it drives |
| --- | --- | --- | --- |
| [`phase0/run-context-smoke.ps1`](../../../tools/phase0/run-context-smoke.ps1#L40) | pipeline | 40, 209 | none via `wide_eye.exe` -- see below |
| [`phase1/run-window-smoke.ps1`](../../../tools/phase1/run-window-smoke.ps1#L123) | pipeline | 123 | `--triangle-smoke`, `--voxel-cube-smoke`, `--voxel-cube-debug-smoke` |
| [`phase2/run-tracer1-review.ps1`](../../../tools/phase2/run-tracer1-review.ps1#L58) | assignment | 58, 65 | `--paddock-*-smoke`, `--dog-render-smoke` |
| [`phase3/run-tracer2-presentation-review.ps1`](../../../tools/phase3/run-tracer2-presentation-review.ps1#L32) | assignment | 32, 38 | `--sheep-motion-render-smoke`, `--sheep-motion-performance-smoke` |
| [`phase3/run-visual-feasibility-baseline.ps1`](../../../tools/phase3/run-visual-feasibility-baseline.ps1#L39) | assignment | 39, 45 | `--visual-tracer-render-smoke`, `--visual-tracer-performance-smoke` |

**Honest limit -- only the phase 3 baseline was actually observed failing.** The
phase 1, phase 2, and phase 3 presentation runners were **not** executed on this
host. What is confirmed for them is each half of the mechanism independently:
their engine modes emit notification-severity stderr here (table above), and
their exact call form throws here (both forms tested). Treat those three as
**untested end to end**, not as observed failures.

**Honest nuance -- phase 0 carries the pattern but is not currently failing.**
`run-context-smoke.ps1` does not drive `wide_eye.exe` at all: it builds and runs
a standalone two-file diagnostic
([`tools/phase0/context-smoke/main.cpp`](../../../tools/phase0/context-smoke/main.cpp),
172 lines) that installs **no** GL debug callback -- a `grep` for
`glDebugMessageCallback` and `GL_DEBUG_OUTPUT` returns nothing -- and writes to
`std::cerr` only on its failure paths (lines 19, 51, 57, 165). It passed on this
host today: `artifacts/phase0/2026-08-22/windows-context-smoke-155925531.log`
records `result=pass`.

Inference, and the reason phase 0 still belongs in this issue: because that
diagnostic reports `result=fail` on stderr, a genuine phase 0 failure would
throw at
[line 209](../../../tools/phase0/run-context-smoke.ps1#L209) and surface as the
raw `result=fail` line rather than through the runner's own `smoke_exit_code=` /
`failure_stage=` reporting. The fragility is latent there, not absent.

**Observed result -- CTest is unaffected, and this is not a "suite passed while
the defect existed" finding.** The `ctest-release` stage of the *same failed run*
passed 47/47. CTest gates on the process exit code plus
`FAIL_REGULAR_EXPRESSION` `"failure_stage=|ERROR: AddressSanitizer|ERROR:
LeakSanitizer|runtime error:"`
([`CMakeLists.txt:1155-1158`](../../../CMakeLists.txt#L1155-L1158)), which the
notification text does not match. CTest reads the engine's contract correctly;
the runners do not. No separate issue is warranted for the suite -- no CTest
invokes any `tools/**.ps1` (a `grep` for `.ps1` and `powershell` in
`CMakeLists.txt` returns nothing), so this surface is outside CTest by design.

**Observed result -- why nobody hit it before.** Every prior native Windows
result came from a different machine. Both accepted goldens are stored under
`windows-intel-uhd-630-development`
([tracer0](../../../tests/goldens/tracer0/voxel_cube_smoke-v1/windows-intel-uhd-630-development/manifest.json),
[tracer1](../../../tests/goldens/tracer1/handcrafted_paddock-v1/windows-intel-uhd-630-development-blockout/manifest.json)),
whose manifest records `Intel(R) UHD Graphics 630` on an i9-8950HK;
[`docs/setup/WINDOWS.md:289`](../../setup/WINDOWS.md#L289) records the same
adapter, and `ROADMAP.md` records that laptop passing native Windows Release
46/46 with these same runners and "zero high-severity messages". This host is
the reference desktop only as of commit `ee18c4b` (2026-08-22).

Inference, with its limit stated: the NVIDIA driver emits `id=131185` where the
Intel driver evidently did not, since no prior run reported a `gl_debug_message`
line as a failure. That is an inference from the absence of any such report plus
the driver change, **not** a measurement -- no Intel adapter was available here
to test, and this issue makes no claim about which driver emits what. It does
not need to: the defect is that any stderr byte aborts the runner, whatever
produced it.

**Observed result -- no shell workaround on this host.** PowerShell 7 would not
promote native stderr this way, but `Get-Command pwsh` reports `pwsh=absent`.
Windows PowerShell 5.1 is the only shell available here.

## Root cause

The five native evidence runners decide whether a stage failed by letting
`$ErrorActionPreference = "Stop"` act on the output of `& <native exe> 2>&1`,
instead of by reading `$LASTEXITCODE`. In Windows PowerShell 5.1 that merged
redirection wraps each stderr line in a `NativeCommandError` `ErrorRecord`, and
`Stop` makes the first one terminating. The engine writes every OpenGL debug
message to `std::cerr` by design, so on any adapter whose driver emits even one
notification-severity message the runner aborts on the engine's first diagnostic
line and reports that line as the failure -- while the process it was grading
exited `0` with `gl_debug_high_severity_messages=0`.

## Expected behavior

A runner must grade the engine by the engine's stated contract: the process exit
code, plus the `key=value` state lines the engine prints on stdout, plus the
explicit gates the runner itself defines --
[`run-visual-feasibility-baseline.ps1:234-237`](../../../tools/phase3/run-visual-feasibility-baseline.ps1#L234-L237)
for `gl_debug_high_severity_messages` and `within_performance_budget`, and
[`run-window-smoke.ps1:230`](../../../tools/phase1/run-window-smoke.ps1#L230) for
its failure expression. Nothing in this repository defines "wrote to stderr" as
a failure condition, and the engine's own severity split
([`window_runtime.cpp:130-142`](../../../src/platform/window_runtime.cpp#L130-L142))
plus CTest's failure regex
([`CMakeLists.txt:1157`](../../../CMakeLists.txt#L1157)) both say the opposite.
[`src/core/runtime.cpp:35`](../../../src/core/runtime.cpp#L35) confirms the
channel convention the runners are misreading: this project routes `debug` and
`info` to stdout and `warn`, `error`, and `fatal` to stderr, so stderr is the
diagnostic channel, not a failure signal.

[`docs/DEVELOPMENT_WORKFLOW.md`](../../DEVELOPMENT_WORKFLOW.md) requires that
verification actually run be reported and that an evidence packet record what
happened. A runner that reports `failure_stage=` for a healthy process both
fails to produce the packet and misrecords the reason, which is the more
expensive half: the retained packet currently blames the graphics driver for a
shell defect.

## Fix notes

Scope is `tools/**.ps1` only. No engine source, no test, no golden, no CMake, no
preset. Blast radius is the manual native-evidence lane; nothing in
`ctest --preset dev` or `--preset release` is touched, and no ownership boundary
in [`src/README.md`](../../../src/README.md) moves.

**Candidate direction, not a decision.** In each helper, lower
`$ErrorActionPreference` to `Continue` around the native invocation only, restore
it in a `finally`, stringify the merged records, and decide pass/fail on
`$LASTEXITCODE`:

```powershell
$prev = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try { $records = & $FilePath @Arguments 2>&1 } finally { $ErrorActionPreference = $prev }
$exit = $LASTEXITCODE
$lines = @($records | ForEach-Object { $_.ToString() })
```

Observed result, 2026-08-22, prototyped in a scratch script against
`build/Windows/release/wide_eye.exe` on this host -- **prototype only; no runner
was modified**:

- `--voxel-cube-smoke`: `exit=0` read correctly, all 35 merged lines captured
  including all 4 `gl_debug_message` lines, so the run log keeps the full
  diagnostic trail;
- the `^([A-Za-z][A-Za-z0-9_.-]*)=(.*)$` state parser still extracted
  `gl_debug_high_severity_messages=0` and
  `gl_renderer=NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2` from the stringified lines;
- an invalid argv (`--no-such-mode`) still returned `exit=1`;
- `--context-smoke-inject-high-severity` still returned `exit=1` with
  `gl_debug_high_severity_messages=1`, so the injected-failure path the runners
  rely on is not blunted.

Constraints the fixer must respect:

1. **The high-severity gate must keep working.** Line 234 is the correct place
   for this runner to fail on graphics diagnostics, and it must still fire on a
   nonzero count. Do not weaken it into a substring search over the log, and do
   not let a `Continue` region swallow a genuinely nonzero exit code -- that is
   the one way this fix can do real damage, hence the negative-case `verify:`
   entries.
2. **Do not change the engine.** Routing GL debug messages to stdout would make
   the runners pass, and it is the wrong fix: it changes an engine contract to
   work around a shell defect, it crosses the ownership boundary (the bug is in
   `tools/`, not `src/platform/`), it puts a per-message diagnostic into the
   stream that CTest and every runner parse for `key=value` state, and it moves
   high-severity messages off the error stream where they belong.
3. **Fix all five runners, not just phase 3.** Phase 0 is passing today only
   because its standalone diagnostic is silent on success; leaving the pattern
   in place there keeps a failure-reporting trap.
4. Reproduce before changing anything -- the scratch probe against
   `--voxel-cube-smoke` costs seconds and does not hold the GPU, unlike the full
   baseline.

Suites that must pass: `wide_eye.opengl_debug_high_severity` and
`wide_eye.opengl_voxel_cube_smoke` to show the engine side is untouched, and
`target:qa-check` for the tracker. The real closure evidence is manual and named
in `verify:` -- the reference-desktop baseline runner completing with
`result=pass` and a packet, plus each other runner still completing, plus a
demonstrated non-zero exit still being caught.

Roadmap obligation, for the owner rather than the fixer:
[`docs/qa/README.md`](../README.md) requires an S1 to be named in the
`ROADMAP.md` Current checkpoint when found. That edit was deliberately not made
while filing this issue, because another change to `ROADMAP.md` was in flight.
