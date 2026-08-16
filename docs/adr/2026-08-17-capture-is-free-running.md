# Capture is free-running, not gated by the phase code

**Date:** 2026-08-17 **Status:** Accepted
**Argument of record:** issue #2, `S0 choices`

## Context

`harness_timing_budget.md` §10 leaves this open by name: whether the capture
engine converts and streams continuously, or starts on the device under test
leaving sleep and stops on it returning. §5 prices the difference. Free-running
is 114 kB/s sustained and about 137 MB per twenty minutes of capture. Gated
falls by whatever the duty cycle is, and at fifty events per second with a
roughly one millisecond active window that is around five percent, so about
6 kB/s and 7 MB.

The choice had a recommendation on record and no decision. Nothing in Tier 1
starts until it has one, because `stream` and `dropped` are both stated against
a sustained rate that only one of the two options produces.

## Alternatives considered

- **Gated by the phase code.** Capture runs while the phase bus is outside
  `sleep`. Twenty times less data, and sleep intervals stop consuming bandwidth
  to record a current that is below one least significant bit anyway. Rejected
  on two grounds. First, every failure of the gate is silent: a gate firing late
  loses the leading edge of the event, a gate failing to fire loses the event
  entirely, and in both cases the records that do arrive are well-formed and
  internally consistent, so the `dropped` gate cannot see the loss. The record
  count is still right. Second, it makes the instrument's behaviour depend on
  the phase bus being correct, and the phase bus is a thing the `phase` gate
  exists to test. An instrument that stops recording when its subject
  misbehaves cannot be used to characterise the misbehaviour.
- **Gated with a pre-trigger ring buffer.** Recovers the leading edge that a
  late gate loses. Rejected: it is the gated design plus a ring buffer plus a
  trigger-latency figure that would itself have to be characterised and
  defended, in exchange for disk space that is not scarce. 137 MB per twenty
  minutes is not a constraint on any machine this project runs on, and buying
  complexity in the timing-critical firmware to save it is the wrong trade at
  this stage.
- **Free-running.** Chosen.

## Decision

Capture is free-running. Both INA226 channels convert continuously at 7,143
conversions per second, and every conversion is timestamped and streamed
whether or not either node is inside an event. Restricting attention to the
event window is a host-side operation performed on a complete record.

The general form of the argument: a full stream can always be trimmed on the
host, and a trimmed stream cannot be untrimmed. Where one direction is
recoverable and the other is not, and the cost of the recoverable direction is
disk, the project takes the recoverable direction.

## What this decision does not claim

Does not claim gating is wrong in general or wrong for a later stage. If a
Stage 2 campaign length makes volume a real constraint, gating is revisited,
and it is then revisited against a measured duty cycle rather than the
estimated one above. It also does not claim the sleep-interval records carry
information: they do not, at 8.14 microamps against a 25 microamp bit. They are
kept because a gap in the timebase and a run of zeros are different objects,
and only the second one is distinguishable from a fault.

## Consequences

- The `stream` gate keeps its criterion unchanged: 114 kB/s sustained, ten
  minutes, no gaps, record count matching `dropped`. Under the gated
  alternative that criterion would have had to be rewritten against a duty
  cycle that has never been measured.
- Raw captures are retained at about 137 MB per twenty minutes. This is already
  what `harness/.gitignore` assumes when it tracks `results/*.json` and not the
  raw capture beside it.
- The capture engine takes no trigger input from the phase bus. The campaign
  trigger in the signal inventory stays a campaign-level marker and does not
  become a capture control, which keeps one line in the inventory doing one
  thing.
- `dropped` stays a meaningful gate. Under gating, a dropped conversion inside
  a sleep interval is indistinguishable from a conversion the gate correctly
  suppressed, so the zero-drop criterion would have quietly become a
  zero-drop-while-armed criterion.

## What would reopen this

A Stage 2 campaign whose volume does not fit, priced against a duty cycle
measured by this harness rather than estimated in advance of it.
