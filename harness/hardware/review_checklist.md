# PCB v1 Review Checklist

*Checked at review **before Gerbers are exported**. Fabrication is a bench action with no pass criterion of its own; the Tier 3 gates are what prove it was done correctly. This checklist exists because the two errors below bias every measurement silently rather than failing visibly, and no gate after fabrication can distinguish them from a real result.*

**Status: not started.** KiCad project is planned. Fabrication is deliberately the last thing started, after every question provable at the desk has been answered.

---

## Not negotiable

- [ ] **High-side sensing on both channels.** Low-side puts a shunt in each node's ground return, which offsets two device grounds against each other and against the capture engine. On a two-node harness that is a designed-in ground offset, and a shared timebase needs one continuous ground. The INA226 takes common-mode input from 0 to 36 V independently of its own supply, so sensing above the 3.3 V rail is unconstrained.
- [ ] **Kelvin sense traces meet the shunt at its own pads and carry no load current.** At 0.1 ohm, one milliohm of trace and solder resistance inside the sense path is a one percent error, comparable to the entire budget.
- [ ] **Datasheet input filter** if transients near the sampling rate are expected.

## Identical channels

- [ ] Same shunt value on both channels: 0.1 ohm, 0.1 percent, 0805 or 1206, 10.9 mW dissipation.
- [ ] One schematic, one layout, repeated or mirrored.
- [ ] Ten assemblies of **one** design, not ten of several.

Ten boards are spares only if they are interchangeable, and interchangeability is the entire reason for fabricating ten. Ten identical boards are spares; ten variants are thrash.

## Sensor configuration reachable from the layout

- [ ] Alert pin of each INA226 routed to the capture engine alongside the phase markers. Without a hardware edge per completed conversion there is no honest way to place a sample in time.
- [ ] Two independent I2C buses, one per channel, brought out separately. 52 percent utilisation each at 400 kHz; one shared bus is 104 percent and fails outright.
- [ ] Address strapping accessible.

## Out of scope for v1

- [ ] Confirm **no autoranging front end** has crept in. It would close the deep-sleep blind spot and it is gated on Stage 2 data showing the range actually binds. Scope here is the timebase, not the front end, and every ranging feature arriving before then is the harness ADR being violated.

---

## Incoming inspection

Continuity and shorts on **every** board before any board is powered. `mise run incoming` writes `results/stage0_incoming.json` with per-board pass or fail and assigned serial numbers.

Measured shunt value per board is recorded at the gain gate, not here, and replaces nominal everywhere from that point on.
