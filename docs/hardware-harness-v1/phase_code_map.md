# Phase Code Map

*The canonical code-to-phase table for both DUT roles. This is the table the capture engine decodes against and the table `phase_marker` writes; if the two ever disagree, this file is right and the firmware is wrong.*

**Status: CLOSED for the code table.** The encoding choice is closed by [`adr/2026-08-17-phase-code-is-parallel-three-bit.md`](../adr/2026-08-17-phase-code-is-parallel-three-bit.md), written against issue #2, `S0 choices`. Every cell below has a value.

**Pin allocation is not in this file and no longer gates it.** See [Exit criterion](#exit-criterion) for why that dependency was split.

Graded on hardware by the `phase` gate in Tier 2, artifact `results/stage0_phase.json`.

---

## Why three bits, and why Gray

Carried from [timing budget §1](./harness_timing_budget.md), not re-argued here.

A single toggle line encodes transitions but not identity, so one missed edge desynchronises every phase after it for the rest of the run, silently. A three-bit code is self describing: any sample of the bus states which phase the node is in, and a missed transition costs one boundary rather than a run.

The capture engine samples the phase bus asynchronously. Ordering the states so that exactly one bit changes per transition means a sample landing inside a transition reads the old code or the new one, never a third. This costs nothing and removes the need for a strobe line.

Writing three bits is one masked register write on the DUT, the same cost as toggling one, which is what keeps it inside the ESP-NOW send callback's discipline.

---

## Why six states and not four

The sequence was written out as four states while it was still a recommendation. Two things already on record push it to six, and both were found by writing the table rather than by running anything.

**Sleep entry is a marked phase in its own right.** [`adr/2026-08-09-wake-cost-separate-from-frame-cost.md`](../adr/2026-08-09-wake-cost-separate-from-frame-cost.md) states it directly: wake and sleep transitions are marked in their own right rather than folded into idle. A four-state cycle folds sleep entry into sleep.

**Both roles transmit, and both roles receive.** The sufficiency constraint requires R to acknowledge delivery at the application layer, so R transmits and sees the same 330 mA peak as S. It follows that S receives that acknowledgement, because an acknowledgement the sender never receives is not one. Charging either node's acknowledgement radio time to a neighbouring phase contaminates a compute term with a radio term, which is the confound the three-term cost model exists to prevent.

That gives five named phases per role. **Five cannot close.** In a closed cycle every bit returns to its starting value, so every bit changes an even number of times, so the total number of transitions is even. A five-state cycle at Hamming distance 1 does not exist on any number of bits. The sixth state is therefore not padding; the parity argument is what turned "R needs an acknowledgement code" into "both roles need six", and the sixth slot was then filled by the phase the wake-cost ADR had already required.

---

## The cycle, shared by both roles

```
  000 -> 001 -> 011 -> 010 -> 110 -> 100 -> 000
    b0     b1     b0     b2     b1     b2
```

Six transitions, one bit each, including the wrap from `100` back to `000`. Each of b0, b1 and b2 changes exactly twice per cycle, which is the parity check that the cycle closes.

**The two roles share codes slot for slot.** The same code means the same position in the event structure in both roles, which is the convention already used where decode and process share a code. A decoder does not need to know which role it is reading in order to know where in an event it is.

---

## Node S, `tx_role`

Phases per thinkbook 4.7: sleep, wake, encode, transmit, sleep. Plus acknowledgement reception and sleep entry, per the argument above.

| Code (b2 b1 b0) | Decimal | Phase | Entered from | Bit that changed |
|---|---|---|---|---|
| 0 0 0 | 0 | sleep | 100 sleep entry | b2 |
| 0 0 1 | 1 | wake | 000 sleep | b0 |
| 0 1 1 | 3 | encode | 001 wake | b1 |
| 0 1 0 | 2 | transmit | 011 encode | b0 |
| 1 1 0 | 6 | acknowledgement receive | 010 transmit | b2 |
| 1 0 0 | 4 | sleep entry | 110 acknowledgement receive | b1 |

**Condition A note.** In Condition A, node S runs no encode phase. The encode code is entered and left with the phase doing no work rather than skipped, so that the cycle observed on the bus is the same six-state cycle in both conditions and the decoder needs no per-condition variant. The dwell in `011` under Condition A is then a measured near-zero rather than an absent state, and a genuinely absent `011` is a fault rather than a condition. Skipping it would have made the two conditions produce two different legal transition sets, which moves a per-run property into the decoder.

---

## Node R, `rx_role`

Phases per thinkbook 4.7: sleep, wake, receive, decode or process, sleep. Plus acknowledgement transmission and sleep entry, per the argument above.

| Code (b2 b1 b0) | Decimal | Phase | Entered from | Bit that changed |
|---|---|---|---|---|
| 0 0 0 | 0 | sleep | 100 sleep entry | b2 |
| 0 0 1 | 1 | wake | 000 sleep | b0 |
| 0 1 1 | 3 | receive | 001 wake | b1 |
| 0 1 0 | 2 | decode (B) / process (A) | 011 receive | b0 |
| 1 1 0 | 6 | acknowledgement transmit | 010 decode or process | b2 |
| 1 0 0 | 4 | sleep entry | 110 acknowledgement transmit | b1 |

**Decode and process share a code deliberately.** They are the same slot in the event structure and are never both present in one run, since Conditions A and B are separate firmware images per the build discipline. The condition is a property of the image, recorded per run, not of the phase bus.

**R's transmit now has its own code.** It previously had none, and the file noted that this mattered because R's transmit peak is the same 330 mA as S's. Code `110` resolves it, and the cost model can charge R's radio time to R's radio term.

---

## What b2 means

Under the four-state sequence, b2 was held at zero throughout and was available as an out-of-cycle flag. **That is no longer true and the old reading must not be carried forward.** At six states b2 is an ordinary phase bit: it is set on entering the acknowledgement phase and cleared on entering sleep, once each per event.

What b2 marks, read as a bit, is the second half of the event: the acknowledgement and the wind-down, as against the wake and the payload work. That is a consequence of the ordering rather than a design intent, and no firmware or decoder should rely on it in place of the table.

**b2 is driven at all times and is never left floating**, on both nodes, and this is a firmware and layout requirement rather than a convention. A floating b2 under the four-state sequence produced a spurious reserved code. Under six states it produces a spurious *phase*, which is worse: the decoder has no way to know it is wrong, and a floating b2 would fabricate acknowledgement and sleep-entry phases with plausible dwell times. The requirement is a defined output driver on the DUT pin from reset, and a defined pull on the harness so that a disconnected node reads as one thing rather than as noise.

---

## Out of cycle

Two codes remain outside the six-state cycle. They are named here rather than left reserved, because there is no longer a spare bit to identify them by.

| Code (b2 b1 b0) | Decimal | Meaning | Entered from | Exits to |
|---|---|---|---|---|
| 1 0 1 | 5 | armed | undefined, at init only | 001 wake |
| 1 1 1 | 7 | error | any code | run terminates |

**Armed is entered once per campaign, before any event.** The transition into it is not at Hamming distance 1, and does not need to be: it happens at initialisation from an undefined port state, before the campaign trigger, with no event in flight, so there is nothing for a mid-transition sample to corrupt. Its exit, `101` to `001` wake, is at distance 1.

**Error is deliberately not distance-constrained.** A transition into error ends the event, and a sample misread during it costs nothing that the error has not already cost.

**Neither code may appear inside an event.** The capture engine flags either one occurring between `001` wake and `100` sleep entry as an anomaly, and an anomaly is a `type:anomaly` issue, not a dropped sample.

---

## The Hamming-distance-1 property, verified

By inspection, over the full cycle including the wrap, for both roles, since both roles use the same cycle.

| From | To | XOR | Bits differing |
|---|---|---|---|
| 000 | 001 | 001 | 1 |
| 001 | 011 | 010 | 1 |
| 011 | 010 | 001 | 1 |
| 010 | 110 | 100 | 1 |
| 110 | 100 | 010 | 1 |
| 100 | 000 | 100 | 1 |

Bit change counts over one cycle: b0 twice, b1 twice, b2 twice. All even, so the cycle closes, which is the same parity fact that ruled out five states.

---

## What this changes in the `phase` gate

**Recorded here because closing this table moved the gate's criterion, and the two must not drift apart.**

The gate was stated as *ten thousand transitions decoded, zero invalid codes*. At four states, four of the eight bit patterns were unused, so a bad read usually landed on an unused pattern and "invalid code" was a real detector.

At six states, with the remaining two codes named, **no bit pattern on the bus is meaningless**, and a criterion counting invalid codes would count zero of them no matter what the bus did. The detector moves from the code to the transition. The legal transition set is the six edges in the cycle above, plus `101` to `001`, plus any code to `111`. Any observed transition outside that set is invalid, and because every legal edge is one bit, **any observed transition of two or three bits at once is invalid by construction**, which is the case a mid-transition sample or a non-atomic marker write produces.

This is a stronger criterion than the one it replaces, not a weaker one, and it is the reason the sequence is written before firmware exists rather than after.

---

## Exit criterion

**Met, for the code table.** Every cell has a value, the out-of-cycle codes are named, b2 is resolved, and the Hamming-distance-1 property is verified above by inspection for both roles across the full cycle including the wrap.

**The pin allocation was removed from this criterion.** The previous version of this file could not close until §4 of `harness_spec.md` was filled, and that file does not exist. The dependency was wrong in both directions: a code table is not made correct by a pin map, and a pin map cannot be drawn without the constraints in high-level design §6.3, which are a separate piece of work with a separate cost. Holding a finished table open behind an unwritten document would have reported this decision as open when it is not.

The pin allocation stays open as O3 in the high-level design and carries one constraint from this file: all three bits toggle every event now, so b2 is a timing-relevant input rather than a static level, and each node's three bits must be contiguous within one GPIO port.

**Proved by** the `phase` gate in Tier 2: ten thousand transitions decoded for both roles, zero invalid transitions, artifact `results/stage0_phase.json`. Nothing in this file is proved by inspection of this file.
