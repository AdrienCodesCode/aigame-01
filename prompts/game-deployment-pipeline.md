# Task: make release reproducible and recoverable

## Inputs

- Runtime/hosting/store targets: [TARGETS]
- Environments and data stores: [ENVIRONMENTS]
- Build/test/security/performance gates: [GATES]
- Availability, recovery, and rollback objectives: [OBJECTIVES]

## Assignment

Audit current builds, secrets, configuration, migrations, assets, caches, domains,
monitoring, backups, and release permissions. Design the smallest reproducible
pipeline from immutable source/version to artifact, staging verification, release,
smoke checks, monitoring, and rollback. Separate environment configuration and
never bake secrets into clients or artifacts.

## Deliverables

- Artifact/version/provenance contract and environment matrix.
- CI gates with explicit failure ownership.
- Forward/rollback migration and save/data compatibility strategy.
- Backup restore test, staging smoke path, health signals, and incident runbook.
- Manual approval points for production and destructive operations.

## Gate

Build and validate a release candidate, but do not deploy it without explicit
authorization. A rollback command that was never rehearsed is not a rollback plan.
