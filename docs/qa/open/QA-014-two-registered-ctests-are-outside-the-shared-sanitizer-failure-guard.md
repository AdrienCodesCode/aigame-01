---
id: QA-014
title: Two registered CTests are outside the shared sanitizer failure guard, so `wide_eye.visual_tracer_configuration` can print a LeakSanitizer report, exit nonzero, and still pass on its pass marker
status: open
severity: S3
confidence: confirmed
area: build
reporter: agent
reported: 2026-08-22
phase: 3
platform: windows
rule: tests/README.md
verify:
  - wide_eye.ctest_failure_regex
  - wide_eye.visual_tracer_configuration
  - wide_eye.compiler_profile
  - target:qa-check
  - manual:a dev-sanitized configure/build/ctest run on Linux or WSL with Clang 18 in which both newly guarded tests pass and no legitimate output matches the guard
  - manual:a negative probe under dev-sanitized in which a deliberate LeakSanitizer or runtime-error line in the visual tracer test's output fails wide_eye.visual_tracer_configuration once the guard is applied
---

## Symptom

Agent finding, 2026-08-22, made while reading `CMakeLists.txt` for
[QA-013](QA-013-influence-sweep-digest-is-printed-and-never-compared.md). Nobody
saw this in a running program; it is a configuration defect located in code and
a CTest behavior demonstrated on this host.

The engine applies one shared `FAIL_REGULAR_EXPRESSION` -- matching
`failure_stage=`, `ERROR: AddressSanitizer`, `ERROR: LeakSanitizer`, and
`runtime error:` -- to a hand-maintained list named `wide_eye_registered_tests`,
so that a test cannot pass on its own pass marker while its output carries a
project failure stage or a sanitizer diagnostic. Two registered tests are not on
that list and therefore do not carry the guard.

Reproduced by reading the generated CTest registry rather than the source, in
both configured trees on this host:

```text
grep 'add_test(\[=\[wide_eye\.'          build/Windows/dev/CTestTestfile.cmake   # 45
grep FAIL_REGULAR_EXPRESSION             build/Windows/dev/CTestTestfile.cmake   # 43
```

The two without it are `wide_eye.compiler_profile` and
`wide_eye.visual_tracer_configuration`. The `release` tree gives 47 and 45, the
same two names.

## Investigation

**Observed result -- the guard, and the fact that it is opt-in.** The list is
built at [`CMakeLists.txt:1093-1121`](../../../CMakeLists.txt#L1093-L1121) and
extended conditionally for Unix
([1122-1124](../../../CMakeLists.txt#L1122-L1124)), for
`WIDE_EYE_ENABLE_OPENGL_CONTEXT_TEST`
([1125-1145](../../../CMakeLists.txt#L1125-L1145)), and for Release
([1146-1153](../../../CMakeLists.txt#L1146-L1153)). One call then applies the
guard to whatever the list ended up containing
([1155-1159](../../../CMakeLists.txt#L1155-L1159)):

```cmake
set_tests_properties(
    ${wide_eye_registered_tests}
    PROPERTIES FAIL_REGULAR_EXPRESSION
               "failure_stage=|ERROR: AddressSanitizer|ERROR: LeakSanitizer|runtime error:"
)
```

Nothing derives that list from the tests that exist. An `add_test` that is not
appended by hand is silently outside the guard, and no configure step, test, or
target notices.

**Observed result -- the exact set difference, computed two independent ways.**
This is the part the report has to get right, so it was counted rather than
trusted.

*From the source:* `CMakeLists.txt` contains **48** `add_test(... NAME
wide_eye.*)` registrations and **46** distinct names inside the list and its
three conditional appends. Difference: `wide_eye.compiler_profile` and
`wide_eye.visual_tracer_configuration`. Nothing appears in the list that has no
`add_test`, so the list has no stale entries either.

*From the generated registry,* which is the authority because it reflects the
conditionals as actually configured:

| Tree | `add_test` entries | Carrying the guard | Missing |
| --- | --- | --- | --- |
| `build/Windows/dev` | 45 | 43 | `compiler_profile`, `visual_tracer_configuration` |
| `build/Windows/release` | 47 | 45 | `compiler_profile`, `visual_tracer_configuration` |

**Correction to the report this was filed from: it is two tests, not one.** The
second, `wide_eye.compiler_profile`, is materially different from the first, and
conflating them would overstate the defect -- see below.

**Observed result -- the one that matters, and exactly what it is registered
with.** `wide_eye.visual_tracer_configuration` is registered at
[`CMakeLists.txt:611-614`](../../../CMakeLists.txt#L611-L614) and given its
properties at [951-956](../../../CMakeLists.txt#L951-L956). Its labels include
`sanitizer` under `WIDE_EYE_ENABLE_SANITIZERS`
([778](../../../CMakeLists.txt#L778) and
[817](../../../CMakeLists.txt#L817)), so it is *declared* to be part of the
sanitized suite. The generated line, from `build/Windows/release/CTestTestfile.cmake:46`:

```text
set_tests_properties([=[wide_eye.visual_tracer_configuration]=] PROPERTIES
    LABELS "unit;scenario;headless"
    PASS_REGULAR_EXPRESSION "visual_tracer_configuration_result=pass"
    TIMEOUT "10" ...)
```

It runs a real C++ test binary that goes through `wide_eye_set_project_options`
([`CMakeLists.txt:543`](../../../CMakeLists.txt#L543)), so under `dev-sanitized`
it is compiled and linked with the sanitizers
([`WideEyeProjectOptions.cmake:26-45`](../../../cmake/WideEyeProjectOptions.cmake#L26-L45)).
It is the only sanitizer-instrumented executable in the suite whose verdict rests
on a pass expression with nothing checking the rest of its output.

**Observed result -- the second one, and why it is a consistency gap rather than
a safety gap.** `wide_eye.compiler_profile`
([`CMakeLists.txt:547-559`](../../../CMakeLists.txt#L547-L559)) runs
`${CMAKE_COMMAND} -P tests/assert-compiler-profile.cmake` and carries exactly one
property, `LABELS "headless;unit"`
([559](../../../CMakeLists.txt#L559)) -- **no** `PASS_REGULAR_EXPRESSION` and no
`TIMEOUT` (CTest computed 1500 s for it in the verbose run below). Two
consequences, both checked:

1. It is `cmake.exe`, not an instrumented Wide Eye binary, so it cannot emit an
   ASan, LSan, or UBSan diagnostic at all, and it cannot emit `failure_stage=`
   (a `grep` shows that marker is written only by
   [`src/platform/scenario_runner.cpp`](../../../src/platform/scenario_runner.cpp#L907), 14
   sites, and [`src/platform/window_runtime.cpp`](../../../src/platform/window_runtime.cpp),
   2 sites -- both inside the `wide_eye` executable, and neither reachable from a
   `-P` script run).
2. With no pass expression, its exit code governs, so nothing can be masked in
   the first place -- it fails through `message(FATAL_ERROR)`
   ([`assert-compiler-profile.cmake:59`](../../../tests/assert-compiler-profile.cmake#L59)).

Adding it to the list is worth doing for uniformity, and because "every
registered test" is the recorded invariant, but **no scenario has been
identified in which its omission can hide a failure.** Saying otherwise would be
the alarmist version of this issue.

**Observed result -- how CTest actually combines the two properties, measured on
this host rather than recalled.** The question decides the whole finding, so it
was run. CTest `3.31.6-msvc6`, 2026-08-22, using a throwaway fixture built the
same way
[`assert-ctest-failure-regex.cmake`](../../../tests/assert-ctest-failure-regex.cmake)
builds its nested one -- in a scratch directory outside the repository, with
nothing in the working tree modified. Each fixture printed
`visual_tracer_configuration_result=pass`, then a marker, and optionally exited
nonzero:

| Fixture | Marker printed after the pass marker | Exit | Properties | CTest verdict |
| --- | --- | --- | --- | --- |
| 1 | `runtime error: intentional fixture` | 0 | PASS only | **Passed** |
| 2 | `runtime error: intentional fixture` | 0 | PASS + the shared guard | Failed -- "Error regular expression found in output" |
| 3 | `ERROR: LeakSanitizer: detected memory leaks` | **1** | PASS only | **Passed** |
| 4 | `ERROR: LeakSanitizer: detected memory leaks` | 1 | PASS + the shared guard | Failed -- same reason |
| 5 | `ERROR: LeakSanitizer: detected memory leaks` | 1 | neither regex | Failed -- exit code |

Two behaviors follow, and both are load-bearing:

- **`FAIL_REGULAR_EXPRESSION` overrides a matching `PASS_REGULAR_EXPRESSION`**
  (2 and 4 against 1 and 3). The repository already proves this half in-tree:
  `wide_eye.ctest_failure_regex` builds four fixtures that emit `smoke_result=pass`
  *and* a failure marker and requires CTest to fail all four, which
  [`tests/README.md:225-227`](../../../tests/README.md#L225-L227) states as
  "rejects project failure markers and ASan, LSan, and UBSan diagnostics **even
  when output also contains a configured pass marker**".
- **Once `PASS_REGULAR_EXPRESSION` is set, a nonzero exit code no longer fails
  the test** (3 against 5). This is the half nothing in the repository
  demonstrated, and it is what makes the missing guard consequential rather than
  merely untidy.

**Observed result -- which diagnostics actually escape, given this project's
sanitizer flags.** `dev-sanitized` sets `WIDE_EYE_ENABLE_SANITIZERS`, which is
MSVC `/fsanitize=address` on Windows and, on Clang/GCC,
`-fsanitize=address,undefined` **with `-fno-sanitize-recover=all`**
([`WideEyeProjectOptions.cmake:30-42`](../../../cmake/WideEyeProjectOptions.cmake#L30-L42)).
That flag narrows the hole, and the narrowing must be stated:

| Diagnostic | What happens to `wide_eye.visual_tracer_configuration` today |
| --- | --- |
| UBSan `runtime error:` | `-fno-sanitize-recover=all` aborts at the diagnostic, so `visual_tracer_configuration_result=pass` ([`visual_tracer_configuration_tests.cpp:91`](../../../tests/visual_tracer_configuration_tests.cpp#L91)) is never printed, the pass expression misses, and the test **fails anyway**. |
| ASan error mid-run | `halt_on_error=1` is the default; same reasoning, the test **fails anyway**. |
| ASan error at exit (static destruction, a use-after-free in teardown, anything after the marker is written) | The marker is already in the output. Fixture 3 above: **passes**. |
| LeakSanitizer | LSan runs after `main` returns, so the marker is always already printed, and LSan exits `23`. Fixture 3 above: **passes**. This is the live hole. |
| `failure_stage=` | This binary never prints it, so that quarter of the guard is irrelevant here. |

So the accurate statement is narrow and still real: **a leak report, or an
address error occurring after the pass marker is written, is swallowed for this
one test under `dev-sanitized` on Clang/GCC.**

**Platform honesty, and it limits the claim.** `dev-sanitized` has no host
condition and can be configured on Windows, but MSVC provides only ASan -- no
LSan and no UBSan -- so the two escaping cases above are Linux/WSL cases.
CI does **not** exercise them either:
[`.github/workflows/linux.yml`](../../../.github/workflows/linux.yml) configures
and runs the `dev` preset only (lines 74 and 84), never `dev-sanitized`. This
host has no Clang 18 toolchain, so **no sanitized run was performed for this
issue and no swallowed sanitizer report was observed.** The gap is latent: proved
in the build configuration and in CTest's measured behavior, not in a captured
failure.

**Falsification attempted.** The narrowest thing that would make this a
non-finding is that the guard reaches these two tests by some other route.
Checked, all four candidates: there is no second `FAIL_REGULAR_EXPRESSION` --
`grep` finds the literal exactly twice in `CMakeLists.txt`, at
[1158](../../../CMakeLists.txt#L1158) (applied) and
[629](../../../CMakeLists.txt#L629) (handed to the nested fixture as a `-D`
argument); there is no directory-level or global property doing it; there is no
`CTestCustom.cmake` and no `CTEST_CUSTOM_ERROR_MATCH` anywhere in the repository;
and the generated `CTestTestfile.cmake` -- which is what CTest reads -- shows the
two entries without the property in both configured trees. The theory survived.

**Observed result -- when it drifted, which shows the mechanism.** Both tests
entered in `ee92cd3`. `git show ee92cd3 -- CMakeLists.txt` adds
`NAME wide_eye.compiler_profile`, its `LABELS` line, and
`NAME wide_eye.visual_tracer_configuration`, and contains **no diff line
touching `wide_eye_registered_tests` at all**; the list's last edit was the
earlier `5e0a6fa`. The accepted invariant was true when it was recorded on
2026-08-15 and drifted the moment two tests were added without anyone
remembering a list in a different part of a 1,160-line file.

**Nothing guards the guard, which is why the drift was silent.**
`wide_eye.ctest_failure_regex` proves the regex *string* rejects all four markers
and that the performance pass expression is exact. It does not assert that the
property is attached to every registered test, and it does not cover the
complementary case that let this through -- a marker in the output with **no**
fail regex being accepted (fixtures 1 and 3 above). Per
[`docs/qa/README.md`](../README.md), a suite that passes while the defect exists
is itself a finding; here the suite and the defect are the same object, so it is
recorded in this issue rather than split off.

### What is not wrong

- **The guard string.** All four alternatives are the right ones, and
  `wide_eye.ctest_failure_regex` verifies each independently.
- **The list-based approach itself.** An explicit list is legible and reviewable;
  its defect is that nothing checks it for completeness.
- **`compiler_profile` having no pass expression.** For a `cmake -P` contract
  script, gating on the exit code is correct, and adding a pass expression would
  *weaken* it -- fixture 5 above is that test's configuration, and it is the only
  one of the five where the exit code still decides.
- **The pass-marker convention generally.** Every engine test printing an exact
  terminal marker is what makes a truncated or crashed run detectable. The guard
  is the counterweight to it, not a replacement for it.

## Root cause

`wide_eye_registered_tests` is a hand-maintained, opt-in list that is the sole
carrier of the shared `FAIL_REGULAR_EXPRESSION`, and it lives roughly 480 lines
below the `add_test` calls it is supposed to mirror. `ee92cd3` added two tests
and did not append them. For `wide_eye.visual_tracer_configuration` the omission
is consequential because CTest, once a `PASS_REGULAR_EXPRESSION` is set, decides
the verdict on that expression alone and ignores the process exit code -- so the
only remaining way to fail that test is for the pass marker to be absent, and a
LeakSanitizer report (or any ASan error raised after the marker is written) does
not remove it.

## Expected behavior

An accepted, checked roadmap item states the invariant in exactly these terms
([`ROADMAP_ARCHIVE.md:148-154`](../../../ROADMAP_ARCHIVE.md#L148-L154)):

> Prevent CTest pass markers from masking project or sanitizer failure
> diagnostics. Observed result: **every registered test** rejects
> `failure_stage=`, ASan, LSan, and UBSan diagnostics, and a nested fixture
> proved all four rejection paths before the development, ASan/UBSan, release,
> and 12-test native Windows suites passed on 2026-08-15.

The same item is checked in
[`docs/plans/agentic-development-workflow.md:142-143`](../../plans/agentic-development-workflow.md#L142-L143)
("Make pass-marker tests reject project failure stages and ASan, LSan, or UBSan
diagnostics"), and [`tests/README.md:225-227`](../../../tests/README.md#L225-L227)
describes the guard as "the common failure regex".

That statement was an observed result on 2026-08-15 and is no longer true of the
current tree. Under [`AGENTS.md`](../../../AGENTS.md), recorded verification must
match what actually runs; the correct repair is to bring the configuration back
to the recorded invariant, **not** to soften the record. `ROADMAP_ARCHIVE.md` is
an archived verbatim history and must not be edited to match the drift.

## Fix notes

Scope is [`CMakeLists.txt`](../../../CMakeLists.txt) alone. No engine source, no
test source, no published contract, no format version, no golden, no budget, no
ownership boundary in [`src/README.md`](../../../src/README.md). Blast radius is
two tests gaining one property.

**Reproduce first, and it needs no build:** count `add_test` entries against
`FAIL_REGULAR_EXPRESSION` occurrences in `build/<host>/<preset>/CTestTestfile.cmake`,
as in the Symptom.

**The minimal fix is two lines** -- append `wide_eye.compiler_profile` and
`wide_eye.visual_tracer_configuration` to the base list at
[`CMakeLists.txt:1093-1121`](../../../CMakeLists.txt#L1093-L1121). Both are
registered unconditionally, so neither belongs in a conditional append. Adding a
name to that list does not disturb the per-test `set_tests_properties` calls;
`LABELS`, `PASS_REGULAR_EXPRESSION`, and `TIMEOUT` set earlier are preserved and
merged, as every other test in the generated registry already shows.

**The safety precondition is already checked.** Neither test's legitimate output
can match the guard, observed by running all three affected tests verbosely on
this host on 2026-08-22 (`ctest --test-dir build/Windows/dev -R
"wide_eye.compiler_profile|wide_eye.visual_tracer_configuration|wide_eye.ctest_failure_regex"
-V`, 3/3 passed):

- `wide_eye.compiler_profile` prints one line,
  `-- Compiler profile contract passed: host=Windows; profile=clang-18; CXX=MSVC 19.44.35228.0`;
- `wide_eye.visual_tracer_configuration` prints eleven `<check>=pass` lines
  ([`visual_tracer_configuration_tests.cpp:11`](../../../tests/visual_tracer_configuration_tests.cpp#L11))
  and its result marker.

Neither contains `failure_stage=`, `ERROR: AddressSanitizer`,
`ERROR: LeakSanitizer`, or `runtime error:`.

**The durable fix is a separate decision, and it belongs to the owner.** The
two-line change repairs today's tree and leaves the mechanism that produced it.
Options, offered rather than chosen:

- **Derive the list instead of maintaining it.**
  `get_property(tests DIRECTORY PROPERTY TESTS)` after the last `add_test` yields
  exactly the registered set, and cannot drift. It also means any future test is
  guarded by default, including one whose output might legitimately contain a
  marker, so it needs a documented opt-out and a comment saying why.
- **Keep the explicit list and add a completeness check at configure time.**
  Compare `DIRECTORY PROPERTY TESTS` against `wide_eye_registered_tests` and
  `message(FATAL_ERROR)` on a difference. Smaller behavior change, keeps the list
  reviewable, and converts a silent drift into a build failure.
- **Extend the nested fixture with the half it does not prove.**
  [`assert-ctest-failure-regex.cmake`](../../../tests/assert-ctest-failure-regex.cmake)
  currently proves only that a guarded test rejects each marker. Adding the
  complement -- an unguarded fixture with the same output being *accepted*,
  including with a nonzero exit -- documents in-tree the CTest behavior this
  issue had to measure, and keeps a future reader from having to rediscover it.

Constraints the fixer must respect:

1. **Do not remove a `PASS_REGULAR_EXPRESSION` to "fix" the exit-code override.**
   The markers are the project's crash and truncation detection. The guard is the
   answer; deleting pass expressions is not.
2. **Do not loosen or reword the guard string.** It is duplicated verbatim at
   [629](../../../CMakeLists.txt#L629) (the value handed to the nested fixture)
   and [1158](../../../CMakeLists.txt#L1158) (the value applied). Verified
   byte-identical today. If it must change, change both, or hoist it into one
   variable that both read -- which would be a small improvement worth making
   while here.
3. **Verify where it matters.** Windows MSVC has neither LSan nor UBSan, so a
   Windows-only run cannot demonstrate the repair. The `manual:` entries require a
   `dev-sanitized` Clang 18 run on Linux or WSL.
4. **Do not edit `ROADMAP_ARCHIVE.md`.** It is archived verbatim and was accurate
   when written.
5. **Re-run the whole suite after adding the names**, not just the two. The point
   of the guard is that it applies to output nobody was reading; confirm nothing
   else in the suite prints a string that now trips it.

Suites that must pass: `wide_eye.ctest_failure_regex` (the guard's own nested
regression), `wide_eye.visual_tracer_configuration` and
`wide_eye.compiler_profile` (the two changed registrations), and a full
`ctest --preset dev` plus a `ctest --preset dev-sanitized` on Linux or WSL.
`target:qa-check` for the tracker.

Related but distinct: [QA-009](../closed/QA-009-windows-evidence-runners-abort-on-benign-opengl-notification-stderr.md)
quotes the same guard string and concluded, correctly, that CTest read the
engine's contract properly while the PowerShell runners did not. That conclusion
is unaffected -- the tests it discussed are all on the list. This issue is about
two tests that are not.
