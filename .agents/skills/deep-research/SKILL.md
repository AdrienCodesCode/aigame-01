---
name: deep-research
description: Perform evidence-driven research for a consequential custom C++ engine, OpenGL, voxel rendering, procedural-generation, tooling, or game-architecture question and write a sourced implementation handoff under docs/research. Use when the user asks for deep research, battle-tested approaches, official guidance, comparisons, feasibility analysis, or a durable research file. This skill researches and recommends; it does not silently implement or expand the roadmap.
---

# Deep Research

Investigate a decision deeply enough that a later planning pass can challenge and act on it.

## 1. Frame the Question

1. Read `AGENTS.md`, `ROADMAP.md`, and directly relevant project documents and code.
2. State the decision, project constraints, success criteria, non-goals, and why the answer matters now.
3. Ask the user only when an unresolved choice would materially change the research.

## 2. Preflight Evidence and Tools

1. Establish the actual repository, platform, compiler, graphics API, and dependency state.
2. Use relevant MCP tools only when they are callable in the current session and add information unavailable through normal local tools.
3. Do not treat MCP registration, an IDE extension, or an external demo as proof that this project supports a capability.
4. Record important gaps instead of blocking all research on a nonessential integration.

## 3. Research in Source Order

Use current sources for version-sensitive claims and prefer:

1. standards, specifications, and official C++, Khronos, SDL, CMake, compiler, debugger, and vendor documentation;
2. this repository and upstream source code at a named version or commit;
3. primary research papers and technical presentations from the implementers;
4. maintained public implementations with license and maintenance review;
5. issue trackers and strong engineering write-ups for failure modes;
6. community posts only for experience or leads, clearly labeled as anecdotal.

When inspecting an external repository is necessary, use a read-only shallow clone under a temporary directory, record the commit, and do not vendor it. Compare conflicting advice by version, hardware, scope, and assumptions.

## 4. Synthesize

Write `docs/research/<topic-slug>.md`; if that path already exists, update it only when the user asked for a revision. Use:

```markdown
# Research: <Topic>

**Status:** Draft research; not implemented
**Produced by:** <agent identity>
**Date:** YYYY-MM-DD
**Project revision:** <commit or worktree state>
**Adversarial review:** Not yet reviewed

## Problem and decision
## Verified project constraints
## Findings
## Options and tradeoffs
## Recommendation
## Failure modes and gotchas
## Evidence and confidence
## Planning handoff
## References
## Recommended next step
```

For consequential claims, label the basis as Confirmed fact, Qualified finding, Inference, or Unresolved. Cite close to the claim and include versions, access dates, or immutable links where possible.

## 5. Boundaries

- Recommend the simplest option that answers the current milestone.
- Keep the custom-engine learning goal separate from claims about player value.
- Treat procedural assets, tiny package size, and advanced rendering as constraints or hypotheses until measured.
- Do not install dependencies, change engine code, update completion checkboxes, publish, or deploy unless explicitly requested.
- Apart from the requested research document and navigation needed to find it, leave project files unchanged.

## 6. Validate and Report

Verify new local links, run `git diff --check`, identify unverified claims, and report the research path, recommendation, confidence, rejected alternatives, and planning questions.
