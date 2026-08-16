# ADR 0003: Project-owned test executables through Tracer 2

**Status:** Accepted
**Date:** 2026-08-16
**Decision owner:** Project owner
**Amends:** [`ADR 0001`](0001-native-foundation.md), test-framework clauses only

## Context

ADR 0001 selected doctest as the initial small C++ test framework. The engine
subsequently reached the Phase 3 spatial-grid checkpoint with focused test
executables registered directly in CTest, but doctest was never added to the
dependency graph or used by a test. Documentation continued to describe it as
implemented, creating a mismatch between the accepted stack and the build.

Adding a dependency solely to make the stale description true would not retire a
current product or technical risk. The existing tests are small process-level
oracles with stable result and failure markers, sanitizer labels, timeouts, and
scenario coverage. Their repeated local `check` helpers are visible but have not
yet created enough fixture or reporting complexity to justify a framework.

## Decision

- Through Tracer 2, use project-owned focused C++ test executables orchestrated
  by CTest. Doctest is not part of the current stack.
- Keep each executable narrow and deterministic. Common process contracts such
  as result markers, failure diagnostics, labels, and timeouts remain owned by
  CMake/CTest rather than a private macro framework.
- Reconsider a small test framework only when repeated fixture lifecycle,
  parameterization, assertion diagnostics, or test discovery creates a concrete
  maintenance cost. Any adoption must follow ADR 0001's immutable pinning,
  provenance, license, and cross-platform verification rules.
- Do not convert existing tests mechanically merely to standardize syntax.

## Consequences

- The dependency graph and current implementation are described honestly.
- Tests retain ordinary C++ entry points that work in development, sanitizer,
  release, Linux, and Windows configurations.
- Small assertion helpers remain duplicated. Reviewers should stop and revisit
  this decision if that duplication begins hiding setup/cleanup errors or makes
  scenario diagnostics materially harder to maintain.
- ADR 0001 remains accepted in every other respect; only its doctest selection
  is superseded.
