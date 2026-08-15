# Task: create a narrow typed game-service client

## Inputs

- Approved API/schema source: [PATH]
- Supported clients/runtimes: [TARGETS]
- Authentication/session mechanism: [MECHANISM]
- Offline and retry requirements: [REQUIREMENTS]

## Assignment

Confirm that multiple consumers or repeated protocol logic justify an SDK. Build
one small client around generated or shared schemas, transport, authentication,
timeouts, cancellation, version negotiation, and typed errors. Keep domain rules
and provider secrets out of the SDK. Avoid hiding network cost or retrying unsafe
mutations.

Design explicit behavior for offline use, expired sessions, partial responses,
rate limits, protocol mismatch, and telemetry failure. Package only for verified
runtimes and preserve tree-shaking where relevant.

## Deliverables

- Public API and compatibility matrix.
- One read and one idempotent/guarded mutation integrated end to end.
- Contract, serialization, auth, timeout, retry, and version tests.
- Minimal usage examples and a migration/versioning policy.

## Gate

Demonstrate that a client upgrade and server mismatch fail clearly without data
loss. Do not publish a package or registry version without explicit authorization.
