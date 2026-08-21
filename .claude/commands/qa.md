---
description: File a QA issue in docs/qa/ from a rough report — investigated, formalized, indexed
argument-hint: <what looks wrong>
---

Read `.agents/skills/qa-intake/SKILL.md` and follow it for this report: $ARGUMENTS

Investigate the claim in the actual code before writing the issue file. Be
strict about the `confidence` field — `unconfirmed` with a precise request for
reproduction steps beats a `confirmed` guess, and a WSL run is never native
OpenGL 4.6 evidence. File any sibling defects you find during the sweep as their
own issues with `reporter: agent`.
