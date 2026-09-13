# The remote INA226 is reached over a PCA9615 differential I2C link on a per-node sense board

**Date:** 2026-09-13 **Status:** Accepted
**Argument of record:** [`../hardware-harness-v1/hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §1, §9, §10
**Stays inside:** [`two-channel-harness-built-in-house`](./2026-08-09-two-channel-harness-built-in-house.md) (the scope boundary it draws: timebase owned, sensor a catalogue part)
**Verification (2026-09-13):** Confirmed against the NXP PCA9615 datasheet Rev 2 (2021-09-16), read directly: 2-channel differential I2C buffer; V_DD(A) 2.3-5.5 V card side and V_DD(B) 3.0-5.5 V line side (3.3 V in spec, best at 5 V); rated to 1 MHz, so 400 kHz passes by the maker's own rating; the differential termination and idle-bias network is defined in the datasheet (section 7.2, Figure 5): ~100 ohm characteristic impedance terminated at both ends, three resistors per pair. Exact resistor values bind at schematic; the decision is confirmed.

## Context

The harness meters each node's rail with an INA226 and a 0.1 ohm shunt,
Kelvin-connected
([`harness_timing_budget.md`](../hardware-harness-v1/harness_timing_budget.md) §6;
[`review_checklist.md`](../../harness/hardware/review_checklist.md), "not
negotiable"). Kelvin sensing requires the sense traces to meet the shunt at its own
pads carrying no load current; at 0.1 ohm, one milliohm of extra resistance in the
sense path is a one-percent error. That forces the shunt, and with it the INA226,
to sit **at the node**, not on the central capture board. Capture board and node
are therefore separated by a cable, and the sketch (Obol Harness v1.0) shows two
JST-PH connectors for exactly that.

The record required "two independent I2C buses, one per channel, brought out
separately" ([`review_checklist.md`](../../harness/hardware/review_checklist.md);
[`../dsc_hld.md`](../dsc_hld.md) §4.1) but never said how a 400 kHz open-drain bus
crosses a cable between two boards. It does not cross one cleanly: I2C is
single-ended, open-drain, and referenced to a ground it now does not share, and the
fast-mode 300 ns rise-time budget ([`../dsc_hld.md`](../dsc_hld.md) §4.1) collapses
under cable and connector capacitance. This ADR records how the bus is extended.

## Alternatives considered

- **Single-ended I2C straight over the cable**, pull-ups at one end. For: no buffer
  IC, fewest parts. Against: the fast-mode rise-time budget the 52 %-utilisation
  figure rests on
  ([`harness_timing_budget.md`](../hardware-harness-v1/harness_timing_budget.md) §4)
  assumes a clean, short bus; cable and connector capacitance plus any inter-board
  ground offset make the bus marginal, and a marginal bus reintroduces exactly the
  dropped conversions the bus-per-channel decision existed to remove. Rejected.
- **Single-ended I2C repeater or extender** (P82B96, PCA9600 class). For: buffers
  the drive, parts count stays low. Against: still single-ended and still
  referenced to a ground shared over the cable, so it does nothing about
  common-mode noise or the inter-board ground offset. A half-measure. Rejected.
- **Differential I2C buffer (PCA9615), one per channel.** Chosen. It converts each
  bus to a differential pair (SCL+/-, SDA+/-) with common-mode rejection over the
  cable, is hot-swap tolerant, and reconstructs a clean local bus at the remote
  INA226. It is the part the sketch already shows.
- **INA226 on the capture board, Kelvin wires to a remote shunt.** For: no remote
  I2C at all. Against: it puts the Kelvin sense path in the cable, the one thing the
  "not negotiable" checklist forbids; at 0.1 ohm the cable resistance in the sense
  path is many percent. Rejected outright.

## Decision

Each of the two I2C buses is extended to its remote INA226 through a **PCA9615
differential I2C buffer**, one on the capture board and one on a per-node sense
board, joined by the JST-PH cable. The PCA9615 joins the INA226 as a **catalogue
component**: bought not designed, replaceable without touching anything above it,
exactly as
[`two-channel-harness-built-in-house`](./2026-08-09-two-channel-harness-built-in-house.md)
draws the sensor boundary. Extending a bus is not building a front end, so this
stays inside that ADR's scope; the deferred autoranging front end is untouched.

A **per-node sense board** is created as a new artifact. It carries the 0.1 ohm /
0.1 % shunt, the INA226 (bare VSSOP-10 part, [`../dsc_hld.md`](../dsc_hld.md) O4),
the remote PCA9615, the remote-side I2C pull-ups, the INA226 decoupling and input
filter, and the CNVR / phase pickoff. It is powered from the harness over the
JST-PH (D3, sense side only). Both sense boards are **one identical design** across
the two channels, since both carry the same shunt
([`harness_timing_budget.md`](../hardware-harness-v1/harness_timing_budget.md) §6)
and interchangeability is the point
([`review_checklist.md`](../../harness/hardware/review_checklist.md)).

What crosses each JST-PH is fixed here
([`hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §10): one differential
I2C pair (SCL+/-, SDA+/-), the single-ended CNVR edge line, three single-ended
phase bits, 3.3 V and GND; ten logical pins.

**CNVR and phase stay single-ended over the cable for v1**, with series-resistor
protection at the capture board
([`hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §10). They are slow
(CNVR 14,286 edges/s, phase ~400/s total,
[`harness_timing_budget.md`](../hardware-harness-v1/harness_timing_budget.md) §2)
and cheap to protect; buffering them differentially is parts spent on a problem not
yet shown. Recorded as a watch item, not a commitment.

## Consequences

- BOM: two PCA9615 on the capture board
  ([`hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §9) and one per sense
  board, plus each buffer's decoupling and the differential termination / bias
  network. That network is `BORROWED` and unbound until the PCA9615 datasheet check
  ([`hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §12 item 3); this ADR
  does not assert its values.
- A companion HLD for the sense board is owed, on the footing the hardware HLD holds
  for the capture board. It inherits the Kelvin and identical-channel requirements
  from [`review_checklist.md`](../../harness/hardware/review_checklist.md) verbatim.
- The 400 kHz budget must now hold across two buffers and a cable. The PCA9615 adds
  propagation delay each way (`BORROWED`, PCA9615 datasheet); at a 2.5 us bit period
  this is expected to pass with margin, but it is a bring-up check, not an
  assumption, and is added to the pre-fab verification.
- The harness ground stays continuous from each node through the sense board to the
  capture engine (high-side sensing,
  [`harness_timing_budget.md`](../hardware-harness-v1/harness_timing_budget.md) §6;
  D3), which is what the shared timebase needs.
- Three-instance rule
  ([`two-channel-harness-built-in-house`](./2026-08-09-two-channel-harness-built-in-house.md)):
  the sense board is instance-one furniture for this experiment, not a product.
  Recorded so it is not generalised early.

## What would reopen this

- Measured I2C margin over the cable proving worse than the budget, which reopens
  either the bus speed (the high-speed-mode fallback the timing budget already
  holds) or the buffering.
- The single-ended CNVR / phase lines proving noise-fragile over the cable, which
  promotes them to differential and changes the JST-PH pinout.
- A sensor change ([`../dsc_hld.md`](../dsc_hld.md) reopen conditions) or DUT pin
  pressure, either of which can ripple into the link.
