# Stage 3 sweeps the compression ratio rather than measuring one operating point

**Date:** 2026-08-09
**Status:** Accepted

## Context

The two payloads under comparison do not scale with the same quantity. `p_raw`
scales with the observation — sensor resolution, window length, sample count.
`p_lat` is a design constant set by the bottleneck width, independent of the
observation. Their ratio is therefore a free variable that grows with
observation size.

The parent build hit the same structure one tier up, where the transferred
payload scaled with the prompt and the alternative scaled with the response,
and the consequence there was that no single operating point could answer the
question. What it produced instead was a threshold stated against both lengths.

A Stage 3 run at one observation size would return a sign. The sign would be
true at that point and would say nothing about where the ledger turns, which is
the quantity a deployment is actually designed against.

## Alternatives considered

- Measure one operating point chosen to be representative. Rejected: there is
  no representative point when the ratio is the free variable.
- Sweep the bottleneck width at fixed observation size instead. Rejected:
  changes the sufficiency question at every point, since a narrower latent is a
  different representation, not the same one under different conditions.
- Sweep both. Deferred: two-dimensional, and the observation axis is the one
  that moves frame count, which is what H1 is about.

## Decision

Stage 3 sweeps observation size across at least the range that moves
`n(p_raw)` through two frame boundaries, holding the bottleneck width fixed so
that the representation under test does not change across the sweep.

The deliverable is the observation size at which the ledger crosses, reported
with `G` and confidence intervals at each point, not a single margin.

## Consequences

Multiplies Stage 3's measurement budget by the number of sweep points, and
requires `payload_gen` to produce a commanded observation size rather than a
commanded payload size.

Requires the sufficiency and collapse checks to pass independently at every
point, since a representation sufficient for a small observation need not be
sufficient for a large one, and a decoder that collapses may do so only at one
end of the sweep.

Rules out quoting a single H_ledger margin without the observation size it was
measured at.

Watch for: the crossing point may fall outside the range the chosen sensor can
produce, in which case the honest result is a bound rather than a crossing, and
it is reported as one.
