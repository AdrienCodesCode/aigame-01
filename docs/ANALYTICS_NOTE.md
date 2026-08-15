# Glitch Analytics evaluation note

## Status

Glitch Analytics is a candidate service, not an approved dependency.

- Product page: <https://www.glitch.fun/publishers/analytics>
- Reviewed: 2026-08-14
- Public technical SDK/API documentation: not found on the reviewed marketing
  page; setup appears to continue behind publisher onboarding.

## What the public page advertises

- Steam wishlists, sales correlation, UTM campaigns, and real-time updates.
- Daily, weekly, and monthly active users; retention and churn views.
- Marketing funnels, campaign ROI, social growth, and community engagement.
- Web visits, approximate geolocation, session duration, and unique visitors.
- Cross-device clusters, identity-confidence scores, journey mapping,
  multi-touch attribution, and fingerprint-to-event matching.
- Cookie-free, privacy-focused, GDPR-compliant tracking claims.

These are vendor claims, not capabilities verified by this repository. In
particular, cookie-free tracking is not automatically anonymous, consent-free, or
GDPR compliant. Cross-device recognition and fingerprint-to-event matching may
be personal-data processing even when cookies are absent.

## Approval gate before integration

Obtain and review:

1. SDK and transport documentation, supported runtimes, and failure behavior.
2. The exact event and identity payloads sent from client and server.
3. Data controller/processor roles, subprocessors, hosting regions, and DPA.
4. Consent requirements by player age and region.
5. Retention, deletion, export, opt-out, and account-deletion behavior.
6. How device matching and fingerprinting work and whether they can be disabled.
7. Sampling, bot filtering, offline queues, deduplication, and event ordering.
8. Pricing or limits after the advertised free tier.
9. A test project demonstrating that blocked or failed analytics never affects
   input, saves, networking, loading, or gameplay.

## Recommended boundary

Game code should emit a small, typed, language-independent event contract to a
local analytics port. Providers subscribe outside gameplay code. The initial
border-collie slice needs only a development session log; no external telemetry
is required for supervised local playtests.

If remote testing later justifies collection, begin with consent state, build,
platform, session start/end, onboarding checkpoints, task completion, retries,
panic/split events, sheep lost/recovered, and performance failures. Do not send
raw player text, precise location, secrets, or stable cross-site identifiers.
