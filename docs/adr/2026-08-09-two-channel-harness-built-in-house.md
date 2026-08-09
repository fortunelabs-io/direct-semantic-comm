# The two-channel metering harness is built in house, and its scope is the timebase only

**Date:** 2026-08-09
**Status:** Accepted

Supersedes [2026-08-09-ina226-metering-with-stated-blind-spots.md](./2026-08-09-ina226-metering-with-stated-blind-spots.md),
which chose a commercial instrument as the fallback and treated the metering
choice as revisited at Stage 3. That entry's sensor configuration and its two
stated blind spots survive here unchanged; what changes is who owns the
instrument around them.

## Context

The core claim of this project is a two-sided ledger. Measuring it requires two
rails metered against one clock.

Commercial low-power current meters in reach measure one rail. Two units give
two timebases with no shared reference, and joining them is an assumption about
time rather than a measurement of it. Gap 1 in the thinkbook, the absence of any
published two-sided MCU energy ledger, may persist partly for that reason: the
standard instrument meters one node, so the published work meters one node.

Two further pressures make the dependency unacceptable rather than merely
inconvenient.

Layer 4 certification is a product. Its mechanism is that the mark is owned and
the test is real. A test that can only be run on another vendor's development
tool rents the foundation of that product, and is exposed to that tool's own
lifecycle; at least one distributor already lists the reference instrument as
retired.

Publishing Stage 0 through Stage 2 data first is meant to make this project the
reference for those measurements. Data whose reproduction requires buying a
specific vendor's tool makes the project a reference for the numbers while
someone else holds the gate.

## Alternatives considered

- Two commercial single-rail meters, timebases joined analytically. Rejected:
  substitutes an assumption for the measurement the project exists to make.
- One commercial meter, one node at a time, both nodes' phase markers into its
  digital inputs. Workable for energy, but leaves cross-node latency resting on
  paired-run equivalence, and keeps the certification dependency.
- Build the sensor front end as well, with autoranging, closing the deep-sleep
  blind spot in the same effort. Deferred, see scope below.

## Decision

A two-channel harness is built for this project: one current sensor per node
rail, and one capture engine timestamping both sensors' conversion-ready edges
and both nodes' phase markers against a single clock.

**Scope is the timebase, not the front end.** Sensors are INA226 parts used as
catalogue components. The capture engine, the timebase, the wire protocol, the
calibration procedure and the host tooling belong to this project. The
dependency being removed is on an *instrument* whose behaviour is not controlled
and cannot be reproduced, not on components, and that distinction is what keeps
the first version finishable.

The sensor layer can be replaced without touching any layer above it. That is
where the boundary is drawn and it is drawn there deliberately.

**Deferred:** an autoranging analog front end that would close the deep-sleep
blind spot. Gated on Stage 2 data showing the range binds. Building it now would
combine a higher cost of failure with a skill not yet established, on a question
not yet asked.

**Status of the artefact:** this is instance one, a fixture for this experiment.
It is not a product and it is not a platform. It earns generalisation after it
has metered three things, not before. Recorded explicitly so that the
three-instance rule is honoured rather than quietly bypassed once the harness
starts working.

## Consequences

Stage 0 becomes heavier and moves onto the critical path. It now validates the
instrument as well as the wiring, against references outside the harness: gain
and offset against a precision resistor and an independent voltmeter, timing
against pulse widths commanded by the device under test's own clock, bandwidth
against a commanded toggling load near the conversion time. A borrowed
commercial meter may serve as an optional cross-check on the single-rail case;
it is not a dependency and does not enter the method.

Stage 0 cannot be skipped or shortened. Until its deliverable exists, the
harness is not an instrument and nothing measured with it counts.

Commits the schematic and capture firmware to the open side of the front store,
since data nobody can reproduce makes nobody a reference. The calibration
procedure and the Layer 4 test vectors stay held. This mirrors the model the
business already runs: the implementation open, the test suite sold.

Concentrates more of the project on one person at a moment when the thinkbook is
also live. That is a real exposure, named here rather than discovered later, and
it is the argument for scoping the first version as narrowly as this entry does.

Watch for: scope creep from timebase into front end. Every ranging feature that
arrives before Stage 2 data asks for it is this entry being violated.
