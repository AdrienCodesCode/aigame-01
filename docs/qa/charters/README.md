# QA charters — the manual sweep map

A charter is one surface plus the things that must be true about it when a human
drives it. Together they answer a question an empty issue tracker cannot: **what
has actually been walked on real hardware before a gate.**

Charters exist because most of this engine's risk lives where CTest cannot go: a
native OpenGL 4.6 context, a real display, motion and feel over time, input
devices, and human judgment about whether the picture reads correctly.

## How to use one

1. Pick a charter and work down it in the real program, on named hardware.
2. Tick items that pass. Leave failures unticked.
3. For each failure, file it (`/qa <what you saw>`), and set the issue's
   `charter:` to this charter's slug so coverage and defects stay linked.
4. Record the sweep in the charter's **Sweep log**: date, build
   (`git rev-parse --short HEAD`), preset, platform/GPU/driver, and the QA ids
   raised.

Ticks are **per sweep, not permanent**. A charter ticked against a build from
last week is history, not current state. Reset the boxes when you start a fresh
sweep of a surface that has changed since.

## Rules

- A charter item is an observable **behavior**, not a code path. "The dog stops
  at a closed gate instead of passing through it" — not
  "`DogController::step` clamps the blocked axis".
- If you exercise something no item covers, add an item. Charters grow from real
  sweeps; a charter that never changes is not being used.
- Charters do not replace CTest suites. Suites prove invariants hold; charters
  prove the program is drivable by a human. A gate needs both.
- **Only the owner sweeps.** An agent must never tick a charter box: agents file
  issues and run suites, and a charter tick is a claim about something seen on
  real hardware.
- A sweep is evidence, so it obeys the evidence rules in
  [AGENTS.md](../../../AGENTS.md): record the platform and build, and never
  record a WSL run as if it were native Windows or native Linux 4.6 evidence.

## Charters

| Charter                             | Surface                                                     |
| ----------------------------------- | ----------------------------------------------------------- |
| [play-session](play-session.md)     | Interactive play: window, camera, controls, named scenarios |

Owner-facing visual acceptance has its own procedure in
[docs/review/HUMAN_VISUAL_REVIEW.md](../../review/HUMAN_VISUAL_REVIEW.md) with
Windows runners under `tools/phase1..phase3/`. Charters are the broader "did a
human drive it" sweep; that document is the accepted visual verdict.
