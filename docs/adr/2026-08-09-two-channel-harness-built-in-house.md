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

Four further pressures make the dependency unacceptable rather than merely
inconvenient.

**Availability.** Landed cost of the reference instrument in Indonesia is around
five million rupiah per channel, sourced from Europe. Two channels is ten
million and one point of failure with an intercontinental replacement path.
Stage 2 is a long campaign, a thousand packets per payload size across two nodes.
An instrument failure mid-campaign costs the campaign and the schedule, not the
instrument. Ten fabricated boards make a failure a swap. The operating principle
the business already holds for people, that solo execution across parallel
streams defeats the purpose of diversification, applies to instruments and is
sharper there, because an instrument cannot be reasoned with once it is dead.

**Unattributable anomalies.** A closed instrument cannot be interrogated. When a
measurement looks wrong, the question is whether the radio or the meter produced
it, and with a closed instrument that question has no answer short of buying a
second one. A project whose entire discipline is separating measured from
borrowed cannot afford an unattributable term inside its own instrument.

**Certification adoption.** Layer 4 certification is a product, and its mechanism
is that the mark is owned and the test is real. A test that only runs on another
vendor's development tool taxes adoption of this project's own standard, in
someone else's favour, and is exposed to that tool's lifecycle; at least one
distributor already lists the reference instrument as retired. A rig that any
partner can fabricate is a rig certification can scale on.

**Reference status and self-qualification.** Publishing Stage 0 through Stage 2
data first is meant to make this project the reference for those measurements.
Data whose reproduction requires buying a specific vendor's tool makes the
project a reference for the numbers while someone else holds the gate. A
published harness design is also a genuinely usable artefact rather than a
teaser: anyone who fabricates it has self-qualified as a serious prospect, which
a bought instrument can never do.

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
against a commanded toggling load near the conversion time.

Those references are standards, and that is deliberate. An earlier draft of this
entry offered agreement with a commercial meter as the stronger Stage 0 claim.
That was wrong. Agreement between two instruments is a cross-check; traceability
runs to standards, and a development tool is not a calibration instrument and
carries no certificate. A precision resistor and a crystal-derived pulse train
are the better reference, not the affordable substitute for one.

Stage 0 cannot be skipped or shortened. Until its deliverable exists, the
harness is not an instrument and nothing measured with it counts.

Commits the schematic and capture firmware to the open side of the front store,
since data nobody can reproduce makes nobody a reference. The calibration
procedure and the Layer 4 test vectors stay held. This mirrors the model the
business already runs: the implementation open, the test suite sold.

Concentrates more of the project on one person at a moment when the thinkbook is
also live. That is a real exposure, named here rather than discovered later, and
it is the argument for scoping the first version as narrowly as this entry does.

Cost is bill of materials plus spins, not the fabrication price of the bare
boards. It remains an order of magnitude below the instrument path.

Watch for: scope creep from timebase into front end. Every ranging feature that
arrives before Stage 2 data asks for it is this entry being violated. Watch also
for ten boards becoming ten revisions. Ten identical boards are spares; ten
variants are thrash.
