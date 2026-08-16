# The phase code is three parallel bits per node, not serialised

**Date:** 2026-08-17 **Status:** Accepted
**Argument of record:** issue #2, `S0 choices`

## Context

`harness_timing_budget.md` §1 derives the width: three bits, because a single
toggle line encodes transitions but not identity, so one missed edge
desynchronises every phase after it for the rest of the run, silently. §10
leaves open how those three bits reach the capture engine. Parallel costs three
device pins per node. A serialised alternative costs one pin plus a strobe and
reconstructs the code on the harness.

The device under test is fixed as ESP32-S3 and no pin pressure has been
demonstrated, which is the only condition the timing budget names for
preferring the serialised form.

## Alternatives considered

- **Serialised: one data line plus a strobe.** Saves two pins per node, four
  across the harness. Rejected on three separate grounds, any one of which is
  sufficient.

  It inserts a latency between the phase changing inside the device and the
  code being complete on the wire. That latency would have to be characterised
  and subtracted from every phase boundary, and phase boundary location is the
  one quantity this harness exists to produce. Adding a correction term to the
  measurement in order to save a pin inverts the priority.

  The shift-out is more than one masked register write. The marker runs inside
  the ESP-NOW send callback, from a high-priority Wi-Fi task where the vendor
  documentation states lengthy operations must not happen. A parallel write
  costs the same as toggling one bit; a serialised write does not.

  A code that arrives over several bit times cannot be read atomically, so a
  sample landing mid-word latches a code that was never intended. That is the
  exact failure Gray ordering was adopted to remove, reintroduced at a
  different layer, where Gray ordering cannot remove it.

- **A single toggle line.** Already rejected in the timing budget and not
  reopened here. Recorded so that a reader of this file alone sees the full
  ladder rather than a two-way choice.

- **Three parallel bits, driven by the device.** Chosen.

## Decision

The phase code is three parallel bits per node, driven by the device under
test, latched by the capture engine with one read of the port input register.
Six of the ten harness inputs are phase bits, two per node times three.

Gray ordering of the sequence is what makes an asynchronous parallel read safe,
and the two decisions stand or fall together: parallel without Gray ordering
admits a mid-transition sample reading a code that was never intended.
The canonical sequence is `phase_code_map.md`, and where that file and the
firmware disagree, the file is right.

## What this decision does not claim

Does not claim three pins per node are free. It claims they are available on
this device and that the alternative charges for them in a currency this
project cannot afford, which is uncertainty in a boundary location. Does not
claim the serialised form is unworkable in general.

## Consequences

- **Binds the pin map.** Each node's three bits must be contiguous within one
  GPIO port on the capture engine, so the handler latches the whole code with
  one read, one shift and one mask. A code assembled from two ports is two
  reads with a window between them, and a transition landing in that window
  produces exactly the invalid code Gray ordering exists to prevent. Stated in
  the high-level design §6.3 and binding on the pin allocation that is still
  open.
- Costs three device pins per node. Accepted for the ESP32-S3.
- The marker stays one masked register write, which is what keeps it inside the
  send-callback discipline.
- Combined with the six-state cycle now closed in `phase_code_map.md`, all
  three bits toggle once per event in each direction. The earlier four-state
  sequence held b2 static, and a pin map drawn under that assumption would have
  under-specified b2. It is a phase bit, not a flag.
- Because six of the eight codes are now in the cycle and the remaining two are
  named, no bit pattern on the bus is meaningless. Detection of a bad read
  therefore moves from the code to the transition, which is stated where it
  binds, in the `phase` gate criterion.

## What would reopen this

Demonstrated pin pressure on the device under test that cannot be resolved
another way, which is the condition the timing budget already names. Not a
count of free pins on paper, a conflict with a function the experiment
requires.
