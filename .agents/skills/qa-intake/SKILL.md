---
name: qa-intake
description: File a QA issue in docs/qa/ from a rough owner report about the Wide Eye engine. Investigates the claim against the actual code before writing, classifies confidence honestly, files sibling defects found during the sweep, and regenerates the index. Use whenever the owner reports something wrong, broken, unexplained, or suspicious in the running program — however small.
---

# QA Intake — turn a rough report into a filed, investigated issue

The owner reports in natural language, mid-session, with whatever detail they
have. Your job is to **investigate first, then write**. A filed issue is a claim
about this codebase, and every claim in it obeys the evidence rules in
`AGENTS.md`: observed result (with build, date, platform, method), inference, or
unverified claim — never blurred together.

Convention: `docs/qa/README.md`. Read it if you have not this session.

## Process

### 1. Classify the report

Not everything belongs here.

- **Something exists and is wrong** → QA issue. Proceed.
- **Something does not exist yet and needs designing and building** → that is a
  plan doc in `docs/plans/`, or an ADR in `docs/decisions/` if it changes an
  ownership boundary or an accepted architecture rule. Say so; do not file a QA
  issue that is really a feature request.
- **Both** (a broken thing whose real fix is a redesign) → file the QA issue for
  the defect, note that the fix likely needs a plan doc or ADR, and let the
  owner decide.

### 2. Check for an existing issue

```bash
grep -ril "<keyword>" docs/qa/open docs/qa/closed
```

If it duplicates an open issue, add the new detail to that file rather than
filing a second one. If it duplicates a **closed** issue, that is a regression —
file a new issue and link the closed one.

### 3. Investigate in the code

This is the step that makes the tracker worth having. Before writing anything:

- Find the code that produces the reported behavior. Cite `file.cpp:line`.
- Read the owning contract first — `src/README.md` for ownership boundaries,
  `docs/decisions/` for accepted architecture, `docs/formats/` for the versioned
  replay and state contracts, `tests/README.md` for what the harness covers.
  Expected behavior in the issue comes from an invariant, not from your taste.
- Decide whether the code actually explains the symptom.
- Check which CTest test would have caught it. **A suite that passes while the
  defect exists is itself a finding** — file it separately.

Then try to **falsify your own explanation** before writing it down. Name the
narrowest thing that would have to be true for your theory to hold, and check
that specific thing. Reading code until it looks like it agrees with the report
is not investigation — the code will usually oblige.

Where an invariant must hold at many call sites, check **all** of them rather
than the one the owner hit. The count is part of the finding.

Reproduce it when you can, and say exactly how:

```bash
cmake --build --preset dev
ctest --preset dev -R wide_eye.<test>
./build/Linux/dev/wide_eye --<smoke or scenario shape>
```

Remember what this host can and cannot prove: the WSL development host exposes
only OpenGL 4.5, so it runs headless suites but is not native Windows or native
Linux 4.6 evidence. Anything needing a real display, a gamepad, motion feel, or
4.6 is `unconfirmed` with a precise ask, not a guess.

### 4. Classify confidence honestly

The field agents get wrong. Be strict with yourself:

- `confirmed` — you located the defect in code, or reproduced it with a named
  command on a named platform. Only then may the issue state it as fact.
- `plausible` — the code is consistent with the report but you could not prove
  the path. Say what you could not prove.
- `unconfirmed` — you found nothing supporting it, or it needs a run you cannot
  perform. **This is a good outcome.** State exactly what the owner should
  capture next: platform, preset, scenario, seed, tick or frame, capture,
  state dump, log excerpt.

Never upgrade confidence to sound useful.

### 5. Widen the sweep

While reading the code, if you find **other real defects** — the same mistake at
other call sites, an adjacent invariant violation, a missing bounds or finiteness
check, a scenario nothing exercises, a doc that describes code that does not
exist — file each as its own issue with `reporter: agent`, and cross-reference
the original.

Do not silently absorb them into the reported issue, and do not fix them inline
without saying so. One reported symptom usually has siblings; surfacing them is
the reason reports route through an agent rather than a form.

### 6. Write the file

```bash
cmake -DMODE=next -P tools/qa/qa-tracker.cmake     # the next free id — never pick one by hand
```

Write `docs/qa/open/QA-NNN-short-slug.md` using the frontmatter schema and
section structure in `docs/qa/README.md`.

Writing rules:

- **Use the repository's vocabulary** — the terms in `src/README.md`,
  `ROADMAP.md`, and the ADRs. The owner reports loosely; the filed issue uses
  the words that mean one thing to every later reader.
- **Keep `## Symptom` in the owner's terms**, plus the reproduction facts:
  command, preset, platform, scenario, seed, replay, tick or frame. Do not
  rewrite the observation into your theory; the theory goes in
  `## Investigation`.
- Formalize the *language*, not the *claim*. Precision, not confidence inflation.
- `## Root cause` says "unknown" when it is unknown.
- `## Expected behavior` cites the invariant, ADR, format contract, or source
  README that says so.
- `## Fix notes` states scope, blast radius, the ownership boundaries involved,
  and which suites must pass — written before the fix so the owner can judge it.
- Severity is impact on the current tracer gate, not effort to fix.
- Fill `verify:` with real CTest names, labels, targets, or `manual:` entries —
  check them against `CMakeLists.txt` or `ctest --preset dev -N`, not memory.
- Evidence files belong under `artifacts/<phase>/<date>/`; reference the path
  and the excerpt, never paste a whole log into the issue.

### 7. Reindex and report

```bash
cmake -DMODE=index -P tools/qa/qa-tracker.cmake
cmake -DMODE=check -P tools/qa/qa-tracker.cmake
```

Then report to the owner in a few lines: the id(s) filed, severity, confidence,
the one-line root cause, and — if anything is `unconfirmed` — exactly what you
need from them.

## Do not

- Do not fix while filing unless the owner asked for a fix. Intake files; the
  `qa-fix` skill fixes. A one-line obvious fix is still a separate decision.
- Do not tick charter boxes. Only the owner sweeps.
- Do not hand-edit `docs/qa/INDEX.md` — it is generated.
- Do not renumber or reuse ids, including ids in `closed/`.
- Do not check a `ROADMAP.md` item because you filed or explained an issue.
- Do not file an issue you did not investigate. If you genuinely cannot
  investigate it, file it `unconfirmed` and say why.
