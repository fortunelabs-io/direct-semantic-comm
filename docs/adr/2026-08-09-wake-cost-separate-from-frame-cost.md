# The per-event wake cost is a separate term from the per-frame cost

**Date:** 2026-08-09
**Status:** Accepted

## Context

The original cost model was `E(p) = n(p) * E_pkt + p * e_byte`. In a design
where every delivered observation is one wake cycle, the per-event cost and the
per-frame cost are confounded exactly, and no measurement taken under that
design can separate them.

This is not hypothetical. The only existing ESP-NOW energy figures fit a large
fixed term whose magnitude is close to an active window multiplied by an idle
current, with no visible step where the frame boundary should be. If the
dominant cost is how long the MCU stays awake rather than how many frames it
sends, then compute time and radio time compete for the same term and the
packet-count argument weakens sharply. A two-term model cannot represent that
possibility, let alone test it.

## Alternatives considered

- Keep two terms and treat any unexplained fixed cost as part of `E_pkt`.
  Rejected: it would let H1 pass for the wrong reason, since a large `E_pkt`
  looks like a packet-count effect whether or not frames caused it.
- Add the wake term but not the control that separates it. Rejected: an
  unidentifiable term is a modeling gesture, not a model.
- Model idle power separately and subtract it analytically. Rejected: requires
  assuming the active window duration, which is the quantity in question.

## Decision

Transmit and receive energy are each modeled with three terms:

    E_tx(p) = E_wake + n(p) * E_pkt + p * e_byte
    E_rx(p) = E'_wake + n(p) * E'_pkt + p * e'_byte

Stage 2 identifies `E_wake` and `E_pkt` separately by sending `k` payloads
inside one wake window at fixed payload size: the wake term appears once and
the frame term scales with `k`.

Every reported energy figure is decomposed into the payload-scaling part and
the part that does not scale, and the two must sum to the measured total with
the residue stated. A residue that is not near zero is a modeling error.

## Consequences

Commits Stage 2 to an additional control interleaved with the payload sweep,
and commits the firmware to a mode that sends multiple payloads per wake.

Adds phases to the GPIO marker scheme: wake and sleep transitions are marked in
their own right rather than folded into idle.

The wake terms cancel in the win inequality, since both conditions pay them
identically. This is the reason the separation is easy to skip and the reason
skipping it is wrong: a term the sign of the result normalizes away can still
govern how much the result is worth. It survives in the denominator of `G`.

Watch for: if the `k`-per-wake control shows `E_wake` dominating across the
whole payload range, H1 as stated is in trouble even if the sawtooth is
visible, and that finding belongs in the predictions ledger rather than in a
footnote.
