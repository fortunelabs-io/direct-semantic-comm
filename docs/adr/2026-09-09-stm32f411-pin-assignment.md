# The capture-engine pin map is I2C1+I2C3, CNVR on EXTI0/1, TIM2 timebase — phase-bit pins excepted

**Date:** 2026-09-09 **Status:** Proposed
**Argument of record:** O3 in [`docs/dsc_hld.md`](../dsc_hld.md), and the pin-map
constraints in [HLD §6.3](../dsc_hld.md)
**Populates:** [`docs/hardware-harness-v1/harness_spec.md`](../hardware-harness-v1/harness_spec.md)

## Context

`harness_spec.md` is the reserved home for pin allocation (O3, and the HLD file
map). It did not exist; O3 records that the pin allocation "is a decision" that
the constraints in §6.3 set up but do not make. KiCad is gated on it. This ADR
records the decision that populates `harness_spec.md`.

Two of the functions are no longer open when this is written: the firmware
already commits the two I2C buses ([`sensor.h`](../../harness/firmware/capture/sensor.h))
and the TIM2 microsecond timebase ([`timing_budget.h`](../../harness/firmware/capture/timing_budget.h),
[`timebase.c`](../../harness/firmware/capture/timebase.c)). This ADR records those
as decided and mirrors them; it decides only the not-yet-built parts — the CNVR
edge lines and the phase-code bits.

Populating the pin map is worth an ADR, not a commit message, because reversing
it after fabrication means a respin and after firmware means rework across both.
The board's own checks do not catch a wrong-but-legal assignment: ERC and DRC
pass on a board with every one of these choices wrong, and the error surfaces
only at bring-up.

## Alternatives considered

- **Populate from memory of the pinout, without a citation.** Rejected: the
  standing instruction is that concrete technical claims are not asserted without
  verification, and an unlabelled memory-sourced pin assignment is exactly the
  failure mode it guards against.
- **Channel B on I2C2 (PB10 / PB3, SDA at AF9).** This was the draft proposal.
  Rejected: the committed firmware already routes Channel B to **I2C3 on PA8 / PB4**,
  and `sensor.h`'s own rule is that the record the Tier 0 gate publishes and the
  values the firmware uses "cannot drift apart." A contract that named I2C2 while
  the firmware drives I2C3 would be precisely that drift. [HLD §6.3](../dsc_hld.md)
  independently lists PB6/PB7/PA8/PB4 as the two buses.
- **Phase-code group on `PA0`–`PA5`.** Rejected: `PA0`/`PA1` reuse pin numbers 0
  and 1, which the CNVR lines claim (`PB0`/`PB1` → EXTI0/EXTI1). Because EXTI line
  number equals pin number regardless of port, that is a collision, violating
  [§6.3 rule 1](../dsc_hld.md). An earlier `PC0`–`PC5` draft was rejected for
  unconfirmed UFQFPN48 bonding. The group is instead proposed on `PA2`–`PA7`, the
  same triples shifted up two to clear EXTI0/EXTI1.
- **Mark the map `Accepted` on the strength of this search.** Rejected: the
  reproducibility standard this repository runs under treats a claim as a proposal
  until a second party reproduces it against the primary source. That check
  (`harness_spec.md` §8) has not run.

## Decision

Populate `harness_spec.md` as follows, datasheet-sourced values `BORROWED` from
the STM32F411xC/xE datasheet (STMicroelectronics **DocID026289 Rev 4**, Table 8
pins and Table 9 alternate functions), cross-checked but not yet reproduced
against the primary-source PDF:

- **I2C Channel A on I2C1:** `SCL = PB6` (AF4), `SDA = PB7` (AF4). **Committed in
  firmware.**
- **I2C Channel B on I2C3:** `SCL = PA8` (AF4), `SDA = PB4` (AF9). **Committed in
  firmware.** Configuring PB4 as `I2C3_SDA` releases NJTRST, so JTAG is
  unavailable after `sensor_bus_init()`; harmless over SWD.
- **CNVR (INA226 conversion-ready) edge lines:** `CNVR_A = PB0` (EXTI0),
  `CNVR_B = PB1` (EXTI1), rising edge. Two of EXTI0–EXTI4 with dedicated vectors,
  per §6.3 rule 2. **Proposed; not yet in firmware.**
- **Phase code:** three parallel bits per node, **proposed on `PA2`–`PA7`**
  (Node A `PA2`/`PA3`/`PA4`, Node B `PA5`/`PA6`/`PA7`) — the withdrawn `PA0`–`PA5`
  shifted up two to clear the CNVR pins on EXTI0/EXTI1, keeping both nodes in one
  `GPIOA` read. Pending §8 bonded-out confirmation; not yet in firmware.
- **Timebase:** `TIM2`, APB1, kernel clock 96 MHz, **`PSC = 95`** for an exact
  1 MHz tick, free-running 32-bit. **Committed in firmware**, statically asserted
  in `timing_budget.h`.

Full detail and the reasoning behind each choice live in `harness_spec.md`; this
ADR records the decision and its status, not a duplicate of the tables.

## Consequences

This commits the schematic to routing **I2C1 and I2C3** (not I2C2) for the two
INA226 channels, and to the CNVR edges on EXTI0/EXTI1. It rules out the `PA0`–`PA5`
phase group as drawn. It does **not** leave the system clock or the timebase
prescaler open — both are decided (SYSCLK 96 MHz, `PSC = 95`); the earlier
"prescaler blocked on a clock decision" is closed.

The phase-code pins are now **proposed** on `PA2`–`PA7` rather than open; what
remains for the whole map is the single §8 primary-source check, after which O3
closes and KiCad is unblocked.

Before this ADR moves to `Accepted`, the checks in `harness_spec.md` §8 must
clear: confirmation against the datasheet PDF (not a search extraction) that the
named pins are bonded out on UFQFPN48, and that the AF4/AF9 assignments hold in
Table 9. Per [`git_sop.md`](../sop/git_sop.md), only an `Accepted` record is immutable;
while `Proposed`, a failed check is corrected in
place (the status line is the one field an ADR edits), not by a new file. Once
`Accepted`, a changed decision gets a new dated file superseding this one.

Watch for: `PA2`–`PA7` bonding on UFQFPN48 is unconfirmed until §8; if any of the
six is not bonded, the triple moves rather than the whole map.
