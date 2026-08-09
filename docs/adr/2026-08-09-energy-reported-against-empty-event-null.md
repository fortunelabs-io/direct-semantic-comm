# Energy results are reported against an empty-event null, not as a fraction of the raw baseline

**Date:** 2026-08-09
**Status:** Accepted

## Context

A saving reported as a percentage of `C_A` does not say how much saving was
available. If the cost of a delivered event carrying one byte is already most
of the cost of a delivered event carrying the full raw payload, then a perfect
encoder saves only the remainder, and the direction is dead regardless of how
good the representation is.

The only existing ESP-NOW per-payload energy data is consistent with exactly
that regime: a large fixed term, of roughly the size of an active window
multiplied by an idle current, with a small marginal per-byte term on top.

The parent build hit the same problem one tier up and solved it by measuring a
null before training anything, on the grounds that a falling loss curve without
a reference cannot be distinguished from a projection collapsing toward the
mean of its target.

## Alternatives considered

- Report `(C_A - C_B) / C_A`. Rejected: the denominator includes cost no
  intervention can reach, so the figure understates a good result and overstates
  a bad one, and does so by an unknown amount.
- Report absolute joules only. Rejected: comparable across nothing, including
  across this project's own operating points.
- Measure the null at Stage 3 alongside the conditions. Rejected: by then the
  encoder has been trained and deployed, so a narrow available range is
  discovered at the most expensive possible moment.

## Decision

`C_null` is defined as the cost of a delivered event carrying a one-byte
payload with no compute charged on either side, and is measured at Stage 1
before any encoder exists.

Every efficiency result is reported as `G = (C_A - C_B) / (C_A - C_null)`, the
fraction of the achievable range recovered, with confidence intervals. `G = 1`
means the semantic path costs as little as sending nothing. Absolute joules are
reported alongside, never instead.

`C_null / C_A` at the smallest and largest raw payloads under consideration is
itself a Stage 1 deliverable and a kill criterion for the direction.

## Consequences

Commits Stage 1 to two questions rather than one, and to metering both nodes
there rather than one, since `C_null` is a two-sided quantity.

Rules out reporting a headline percentage saving without its denominator, in
this document or anywhere downstream of it.

Watch for: `C_null` is not a single number. It moves with PHY rate, with
encryption, and with sleep-mode configuration, so it is re-measured under each
Stage 4 control rather than carried across them.
