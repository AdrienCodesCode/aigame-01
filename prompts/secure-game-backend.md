# Task: design only the backend the approved game requires

## Inputs

- Game/client and trusted features: [DESCRIBE]
- Expected launch/test scale: [SCALE]
- Identity, purchases, shared state, and persistence needs: [NEEDS]
- Hosting/database constraints: [CONSTRAINTS]
- Privacy, age, and region requirements: [REQUIREMENTS]

## Assignment

First decide whether a backend is required. If an offline/local solution answers
the current goal, document that decision and stop.

Otherwise threat-model the actual assets and actors. Define server authority,
authentication, authorization, schemas, idempotency/replay protection,
concurrency rules, rate limits, migrations, backups, observability, deletion, and
recovery. Keep routes thin and domain decisions testable. Do not default to JWT,
microservices, a database abstraction, or a particular cloud when an existing
project convention is safer and simpler.

## Deliverables

- Trust/data-flow diagram and abuse cases.
- Minimal schema/API contract with error and version behavior.
- Phased implementation beginning with one end-to-end trusted operation.
- Unit, integration, authorization, concurrency, migration, and recovery tests.
- Runbook for secrets, local setup, deploy prerequisites, rollback, and support.

## Gate

Prove an authorized success, anonymous/forbidden failure, invalid/replayed
request, concurrent conflict, backup/restore, and client failure-safe behavior.
