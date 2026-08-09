# The cost model's terms are identified by run design, not by instrument bandwidth

**Date:** 2026-08-09
**Status:** Accepted

## Context

The three-term cost model needs `E_wake`, `E_pkt` and `e_byte` separated, plus
the encode, process and use terms charged to their nodes. The obvious way to get
them is to resolve the current waveform inside a single event and integrate
between phase markers. That reading makes instrument bandwidth the binding
constraint on the whole project, and it is what drove the harness toward
instruments in the hundred-dollar class.

It is also avoidable. Every quantity in the model is an integral over an event
of tens of milliseconds. None of them is a shape inside one.

If nothing is decided, the project buys an instrument to answer a question it
could have answered by arranging its runs differently, and the answer to
"can we afford this" gets made on the wrong axis.

## Alternatives considered

- Resolve phases within one event and integrate between markers. Rejected as the
  primary method: it makes the result hostage to sampling rate, and a conversion
  straddling a boundary blends two phases no matter how the markers are placed.
- Estimate the fixed term analytically from datasheet idle current times a
  measured window duration. Rejected: assumes the quantity in question.
- Accept coarse totals and report only `C_A` against `C_B` without itemising.
  Rejected: an unitemised total cannot be attributed to a named term, which is
  what Stage 4's controls exist to do.

## Decision

Each term is identified across runs rather than within events.

`E_wake` and `E_pkt` come from a regression on the number of payloads sent
inside one wake window: intercept and slope over `k = 1, 2, 4, 8, 16` at fixed
payload size. `e_byte` comes from the slope of the payload sweep within a frame.
The retransmission share of `E_pkt` comes from running the same sweep in
broadcast, which carries no acknowledgement and therefore no retries, against
unicast, which retries up to its configured limit. The compute terms come from
firmware-variant ablation: separate images performing strict subsets of the
event on identical wake and sleep structure, differenced. Operations too short
to measure once are repeated a commanded number of times within one event and
taken as a slope.

The critical path is therefore the number and design of runs. The instrument
must integrate an event faithfully; it does not have to draw one.

## Consequences

Commits the firmware to a family of ablation images rather than one image with
compile-time flags, which the Unix discipline in the build section already
implies.

Commits every ablation-derived term to carrying a propagated error, since
differencing accumulates it. That error budget is a Stage 3 deliverable and it
is the price paid for not needing a faster meter.

Makes the instrument choice reversible. A better meter improves the illustrative
plots and the error bars; it does not change which quantities are obtainable.

Watch for: any future result that can only be obtained by resolving a boundary
inside an event. If one appears, this decision is superseded rather than
stretched, because the whole argument above rests on no such result existing.
