# Phase Code Map

*The canonical code-to-phase table for both DUT roles. Stage 0 Phase A, item A3. This is the table the capture engine decodes against and the table `phase_marker` writes; if the two ever disagree, this file is right and the firmware is wrong.*

**Status: OPEN.** The encoding choice is A3 in the [harness spec](./harness_spec.md) §4. The sequences below are written out under the standing recommendation, parallel three-bit, and are not committed until A3 closes.

---

## Why three bits, and why Gray

Carried from [timing budget §1](./harness_timing_budget.md), not re-argued here.

A single toggle line encodes transitions but not identity, so one missed edge desynchronises every phase after it for the rest of the run, silently. A three-bit code is self describing: any sample of the bus states which phase the node is in, and a missed transition costs one boundary rather than a run.

The capture engine samples the phase bus asynchronously. Ordering the states so that exactly one bit changes per transition means a sample landing inside a transition reads the old code or the new one, never a third. This costs nothing and removes the need for a strobe line.

Writing three bits is one masked register write on the DUT, the same cost as toggling one, which is what keeps it inside the ESP-NOW send callback's discipline.

---

## Node S, `tx_role`

Phases per thinkbook 4.7: sleep, wake, encode, transmit, sleep. Plus an armed state and an error state, per timing budget §1.

| Code (b2 b1 b0) | Decimal | Phase | Entered from | Bit that changed |
|---|---|---|---|---|
| 0 0 0 | 0 | sleep | *open* | *open* |
| 0 0 1 | 1 | wake | *open* | *open* |
| 0 1 1 | 3 | encode | *open* | *open* |
| 0 1 0 | 2 | transmit | *open* | *open* |
| 1 1 0 | 6 | *reserved / armed* | *open* | *open* |
| 1 1 1 | 7 | *reserved / error* | *open* | *open* |

**The cycle above is Hamming distance 1 at every transition including the wrap**, since 000 → 001 → 011 → 010 → 000 is a two-bit Gray cycle on b0 and b1 with b2 held at zero. That leaves b2 as the out-of-cycle bit, which is what the reserved codes use.

**To close:** decide what b2 means. As written, 010 (transmit) and 110 (armed) differ by one bit, so a single stuck or glitched b2 during transmit decodes as a valid reserved state rather than as an error. That is tolerable if b2 is only ever asserted deliberately and the capture engine flags any b2 assertion inside an event, and it is not tolerable if b2 is left floating. Whichever, it is written down here, not left to the firmware.

**Condition A note.** In Condition A, node S runs no encode phase. Whether the encode code is skipped or dwelt in for zero time is a firmware decision with a decode consequence, and it is recorded here when Phase C writes `phase_marker`.

---

## Node R, `rx_role`

Phases per thinkbook 4.7: sleep, wake, receive, decode or process, sleep.

| Code (b2 b1 b0) | Decimal | Phase | Entered from | Bit that changed |
|---|---|---|---|---|
| 0 0 0 | 0 | sleep | *open* | *open* |
| 0 0 1 | 1 | wake | *open* | *open* |
| 0 1 1 | 3 | receive | *open* | *open* |
| 0 1 0 | 2 | decode (B) / process (A) | *open* | *open* |
| 1 1 0 | 6 | *reserved / armed* | *open* | *open* |
| 1 1 1 | 7 | *reserved / error* | *open* | *open* |

**Decode and process share a code deliberately.** They are the same slot in the event structure and are never both present in one run, since Conditions A and B are separate firmware images per the build discipline. The condition is a property of the image, recorded per run, not of the phase bus.

**R also transmits.** The application-layer acknowledgement thinkbook 4.6 requires means R sends. Whether that acknowledgement gets its own code or is charged inside the decode-or-process phase is open, and it matters, because R's transmit peak is the same 330 mA as S's.

---

## Exit criterion for A3

The table above is closed when every *open* cell has a value, the Hamming-distance-1 property is verified by inspection for both roles across the full cycle including the wrap, and the pin allocation in [harness spec](./harness_spec.md) §4 is filled.

Graded on hardware at Phase C, item C1: the full four-state sequence decoded for both roles across ten thousand transitions, zero invalid codes, artifact `results/stage0_c1_phase.json`.
