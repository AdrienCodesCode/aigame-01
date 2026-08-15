# Task: design question-led, privacy-safe game analytics

## Inputs

- Current playtest/product questions: [QUESTIONS]
- Player journey and builds: [PATHS]
- Platforms, ages, and regions: [TARGETS]
- Candidate providers: [PROVIDERS OR NONE]
- Consent/privacy policy: [POLICY]

## Assignment

Start with decisions the team expects to make. Map only those decisions and
severe technical failures to a small event contract. Keep stable event/property
IDs independent of display language. Use a provider-neutral, asynchronous port
with schema validation, ordering, deduplication, queue limits, consent state, and
development inspection.

Evaluate every provider's payloads, identifiers, fingerprinting, retention,
deletion, subprocessors, regions, SDK weight, offline behavior, and failure mode.
Do not equate cookie-free tracking with anonymous or consent-free tracking.

## Deliverables

- Question-to-event coverage matrix with owner and expiry/review date.
- Data dictionary, identity/session model, and privacy classification.
- Development recorder before external provider integration.
- Consent, blocked-provider, offline, duplicate, order, and payload tests.
- Validation evidence from representative sessions and a deletion/opt-out runbook.

## Gate

No production collection until privacy review and explicit authorization. Remove
events that do not inform a decision or protect technical health.
