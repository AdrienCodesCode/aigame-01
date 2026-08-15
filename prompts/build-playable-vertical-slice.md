# Task: build the smallest slice that answers one playtest question

## Inputs

- Approved game design and core question: [PATH/QUESTION]
- Architecture and target platforms: [PATHS]
- Must / Should / Could / Won't scope: [SCOPE]
- Time, quality, and performance budget: [BUDGET]

## Assignment

Define one path from entry to meaningful success or failure that expresses the
fantasy, core verbs, signature interaction, one decision/pressure, readable
feedback, and representative controls. Use final-direction behavior only where it
affects the question; mark other content as placeholder or deferred.

Make the baseline build/tests pass, then implement in thin playable increments.
Instrument only the core question and severe technical failures. Include a fast
restart and a development view that explains hidden simulation state.

## Deliverables

- Slice map, exclusions, and acceptance evidence.
- Runnable implementation with one success and one recoverable failure path.
- Unit tests for core rules and an end-to-end real-input smoke test.
- Target-device frame/startup/memory check proportional to the stage.
- Playtest script that does not coach the mechanic being tested.

## Gate

Do not add progression, content breadth, cosmetics, accounts, or polish systems
until fresh players can answer the core question and repeat the intended loop.
