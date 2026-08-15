# Task: complete the approved game without redesigning it

## Inputs

- Source-of-truth design/scope and decision precedence: [PATHS]
- Current build and requirement/status matrix: [BUILD/MATRIX]
- Target platforms and release gates: [TARGETS/GATES]
- Time/content/quality budget: [BUDGET]

## Assignment

Audit every approved requirement as complete, partial, placeholder, broken,
missing, or deliberately deferred. Surface material document conflicts. Build in
vertical slices that remain playable, beginning with a validated core loop, then
complete only the approved systems and content. Preserve dependency direction,
save compatibility, authoritative boundaries, accessibility, input equivalence,
localization scope, privacy, and asset provenance.

Do not turn mocks into hidden production behavior, invent new currencies or
features, replace sound architecture without evidence, or treat generated volume
as completion.

## Deliverables

- Requirement-to-owner/status/test/document matrix.
- Phased implementation with playable acceptance at each phase.
- Passing relevant rule, integration, real-input, device, security, migration,
  performance, and production-build checks.
- Release candidate plus documented exclusions, risks, and rollback prerequisites.
- Final mapping from approved scope to verified evidence.

## Gate

Do not deploy automatically. “Complete” means the approved scoped experience is
playable end to end and every claimed target/gate has reproducible evidence.
