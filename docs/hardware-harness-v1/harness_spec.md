# Harness spec: capture-engine pin allocation

**Authority.** This file is the single source of truth for pin assignment on the
two-channel capture engine (STM32F411CEU6, UFQFPN48). Firmware references pins by
the symbolic names defined here; schematic net names match these symbols exactly.

It is a pin map, and only a pin map. Board-level component counts, the BOM, and
the remote-sense topology live in the hardware HLD
([`hardware_hld.md`](./hardware_hld.md)); the clock plan, the prescaler, and every
timing constant live in [`timing_budget.h`](../../harness/firmware/capture/timing_budget.h).
Where those files and this one overlap, each owns its own layer: the HLD owns the
board and the BOM, the header owns the clock, this file owns the pins.

Two provenance regimes apply, and the difference matters:

- **Functions already built in firmware**, the two I2C buses and the TIM2
  timebase, are owned by their firmware headers, not by this file. This file
  *mirrors* [`sensor.h`](../../harness/firmware/capture/sensor.h) and
  [`timing_budget.h`](../../harness/firmware/capture/timing_budget.h) and must not
  drift from them; where the two disagree, the header is right and this file is
  stale. This follows `sensor.h`'s own rule that "the record the Tier 0 gate
  publishes and the values the firmware uses cannot drift apart: they are the
  same constants."
- **Functions not yet built**, the INA226 conversion-ready (CNVR) edge lines and
  the phase-code bits, both Tier 1 per [HLD §6.3](../dsc_hld.md), are decided
  *here first*, before firmware is written against them. For these, this file is
  the source and conflicts are resolved here.

**Provenance.** Every pin below traces to DocID026289 Rev 4 (Table 8/9), cited
where it appears, and no cell is `MEASURED` on hardware. The pin map's decision
record and its primary-source verification status live in the pin-assignment ADR
([`2026-09-09-stm32f411-pin-assignment.md`](../adr/2026-09-09-stm32f411-pin-assignment.md)),
which stays `Proposed` until the datasheet bonding and AF checks clear. This file
states the allocation as decided; the ADR is where that verification status lives.

---

## 1. Fixed by silicon

| Symbolic name | Pin  | Alternate function | Notes                        |
|---------------|------|--------------------|------------------------------|
| `SWD_IO`      | PA13 | SWDIO              | pull-up on board, 10k typical |
| `SWD_CLK`     | PA14 | SWCLK              | pull-down on board, 10k typical |
| `USB_DM`      | PA11 | OTG_FS_DM          | 22 ohm series resistor       |
| `USB_DP`      | PA12 | OTG_FS_DP          | 22 ohm series resistor       |
| `NRST`        | NRST | reset              | 100 nF to GND, no pull needed |

Unchanged from the original contract. USB device is one of the four peripherals
the Tier 0 `toolchain` gate names; the clock tree that serves it is decided and
statically checked in
[`timing_budget.h`](../../harness/firmware/capture/timing_budget.h), not here
(see [§5](#5-timebase-peripheral)).

---

## 2. I2C buses

Two independent I2C masters on separate peripherals, one per INA226 channel, so a
read stall on one bus cannot block the other. **This section mirrors committed
firmware** ([`sensor.h`](../../harness/firmware/capture/sensor.h),
[`sensor.c`](../../harness/firmware/capture/sensor.c)); the pins, alternate
functions, and package numbers below are the constants those files program.

| Symbolic name | Pin  | Peripheral | AF  | Pkg (UFQFPN48) | Bus role | Serves |
|---------------|------|------------|-----|----------------|----------|--------|
| `I2C_A_SCL`   | PB6  | I2C1       | AF4 | 42 | I2C master SCL | Channel A INA226 |
| `I2C_A_SDA`   | PB7  | I2C1       | AF4 | 43 | I2C master SDA | Channel A INA226 |
| `I2C_B_SCL`   | PA8  | I2C3       | AF4 | 29 | I2C master SCL | Channel B INA226 |
| `I2C_B_SDA`   | PB4  | I2C3       | AF9 | 40 | I2C master SDA | Channel B INA226 |

Cited from Table 9 of DocID026289 Rev 4. Channel A sits on **I2C1**, Channel B on
**I2C3**: two separate peripherals, not two pins of one, satisfying the harness
constraint that two INA226s never share a bus. [HLD §6.3](../dsc_hld.md) lists
exactly PB6/PB7/PA8/PB4 as already spoken for by the two buses.

**The two alternate-function numbers differ, and that is not a transcription
error.** Table 9 heads AF04 with `I2C1/I2C2/I2C3` and AF09 with `I2C2/I2C3`, so
I2C3 appears in both columns and which one applies is a per-pin fact. On this part
every Port B I2C2/I2C3 SDA sits at AF09 (PB3, PB4, PB8, PB9) while the clocks sit
at AF04; **AF04 on PB4 is blank, not a different function.** Reading the column
header alone and applying AF4 to all four pins is the error
[`sensor.c`](../../harness/firmware/capture/sensor.c) exists to stop being made
twice: it would have initialized cleanly and driven the wrong pin.

**Consequence: JTAG is released.** PB4 is `NJTRST` at reset and belongs to the
SWJ-DP group. Configuring it as `I2C3_SDA` releases the JTAG reset line, so **JTAG
is unavailable once `sensor_bus_init()` runs.** Harmless: the ST-Link attaches
over SWD on PA13/PA14. Recorded in `sensor.h` so it is not rediscovered as a
symptom.

**Bus speed.** The harness budget is stated at 400 kHz (52% bus utilization per
channel; one shared bus would be 104% and fail outright, per
[`review_checklist.md`](../../harness/hardware/review_checklist.md)). Tier 0
firmware brings both peripherals up at 100 kHz standard mode as a placeholder;
raising them to 400 kHz is Tier 1 work, tracked in [HLD §6.4](../dsc_hld.md). The
pins do not change with the speed.

---

## 3. CNVR (INA226 conversion-ready) edge lines

One edge line per INA226. The INA226 ALERT pin is configured as **Conversion
Ready (CNVR)**, not as an over-current alert
([`adr/2026-08-09-ina226-metering-with-stated-blind-spots.md`](../adr/2026-08-09-ina226-metering-with-stated-blind-spots.md)):
the device timestamps nothing itself and I2C reads are not deterministic, so the
CNVR edge is the only honest way to place each conversion in time. It is routed to
the capture engine alongside the phase markers and captured on **EXTI, rising
edge** ([HLD §6.2](../dsc_hld.md): "CNVR rising edge on EXTI"). The pin is
open-drain on the INA226 side and takes an internal pull-up on the STM32 side.

| Symbolic name | Pin | EXTI line | Edge   | Rate         | Serves |
|---------------|-----|-----------|--------|--------------|--------|
| `CNVR_A`      | PB0 | EXTI0     | rising | 14,286 edges/s | Channel A INA226 |
| `CNVR_B`      | PB1 | EXTI1     | rising | 14,286 edges/s | Channel B INA226 |

**Not yet in firmware.** `register_map.h` does not yet contain SYSCFG, EXTI, or
NVIC; edge capture is the first thing Tier 1 adds ([HLD §6.3](../dsc_hld.md)).

**Why EXTI0 and EXTI1.** The two CNVR lines are the timing-critical inputs
(14,286 edges/s each; the phase lines carry ~400/s in total). EXTI0 through EXTI4
have dedicated interrupt vectors while EXTI9_5 and EXTI15_10 are shared, so the two
CNVR lines **must take two of EXTI0 through EXTI4** ([HLD §6.3](../dsc_hld.md)
rule 2) and never wait behind each other. PB0 to EXTI0 and PB1 to EXTI1 satisfy
this. Because EXTI line number equals pin number regardless of port, pin numbers 0
and 1 are now **claimed by these lines and unavailable to any other edge input**,
see §4.

---

## 4. Phase-code GPIO: PA2-PA7 (O3)

Six GPIO: three bits per node, two nodes, driven by the device under test and
latched by the capture engine with one port-input-register read at the CNVR
timestamp, read, not interrupt-driven. The symbolic names, the encoding, and the
pin numbers below are fixed; the UFQFPN48 datasheet pin-out is recorded in the
hardware HLD ([`hardware_hld.md`](./hardware_hld.md)).

| Symbolic name  | Pin | EXTI | Node   | Bit index |
|----------------|-----|------|--------|-----------|
| `PHASE_A_BIT0` | PA2 | EXTI2 | Node A | 0 (LSB) |
| `PHASE_A_BIT1` | PA3 | EXTI3 | Node A | 1       |
| `PHASE_A_BIT2` | PA4 | EXTI4 | Node A | 2 (MSB) |
| `PHASE_B_BIT0` | PA5 | EXTI5 | Node B | 0 (LSB) |
| `PHASE_B_BIT1` | PA6 | EXTI6 | Node B | 1       |
| `PHASE_B_BIT2` | PA7 | EXTI7 | Node B | 2 (MSB) |

All six sit on GPIOA, so one `GPIOA->IDR` read latches both nodes:
`(IDR >> 2) & 0x7` is Node A, `(IDR >> 5) & 0x7` is Node B.

**Constraints this allocation satisfies** (all binding, from the record):

1. **Each node's three bits contiguous within one GPIO port.** Node A is PA2-PA4,
   Node B is PA5-PA7, each a contiguous triple in GPIOA, so the handler latches a
   code with one read, one shift and one mask. A code assembled from two ports is
   two reads with a window between them, and a transition landing in that window
   produces exactly the invalid code Gray coding was adopted to make impossible.
   ([`adr/2026-08-17-phase-code-is-parallel-three-bit.md`](../adr/2026-08-17-phase-code-is-parallel-three-bit.md),
   [HLD §6.3](../dsc_hld.md).)
2. **Distinct pin numbers across all ports.** EXTI line number equals pin number
   regardless of port ([HLD §6.3](../dsc_hld.md) rule 1), and under the closed
   six-state cycle all three bits toggle once per event, so `b2` (PA4, PA7) is a
   timing-relevant edge source, not a static level. Numbers **2 through 7** are
   used here; they are distinct from each other and from pin numbers **0, 1** (the
   CNVR lines, §3). The I2C, USB and SWD pins sit on their own ports and consume no
   EXTI line, so EXTI6/EXTI7 stay free for PA6/PA7 even though PB6/PB7 carry I2C1;
   PA8 (I2C3_SCL) is number 8, outside this range.
3. **`b2` (PA4, PA7) driven at all times, never floating**, on both nodes, a
   firmware and layout requirement, not a convention. A floating `b2` fabricates a
   plausible phase rather than an obvious fault
   ([`phase_code_map.md`](./phase_code_map.md), "What b2 means").

**Provenance of PA2-PA7.** This is the withdrawn draft `PA0`-`PA5` shifted up by
two, to clear pin numbers 0 and 1 which the CNVR lines claim on EXTI0/EXTI1; the
shift preserves the single-`GPIOA`-read property that motivated the original
choice. An earlier-earlier draft used `PC0`-`PC5` and was withdrawn separately
because the UFQFPN48 bonding of Port C could not be confirmed. Both drafts are
recorded so the ladder is visible; neither is carried forward. PA2-PA7 are low
Port A pins; their UFQFPN48 bonding is tracked in the pin-assignment ADR
([`2026-09-09-stm32f411-pin-assignment.md`](../adr/2026-09-09-stm32f411-pin-assignment.md)).

---

## 5. Timebase peripheral

The microsecond timebase is **TIM2** (one of the two 32-bit timers on this part,
TIM2/TIM5, on APB1). It consumes no external pin, so it appears in this map only
to reserve the peripheral: nothing else may claim TIM2.

The clock tree, the PLL configuration, the prescaler, and every derived timing
constant are decided and statically asserted in
[`timing_budget.h`](../../harness/firmware/capture/timing_budget.h) and set in
[`timebase.c`](../../harness/firmware/capture/timebase.c). They are **not restated
here** to avoid a divergent second copy; where this file and the header disagree,
the header is right and this file is stale. The clock source itself is governed by
[`../adr/2026-09-13-capture-engine-clock-is-hse-8mhz-crystal.md`](../adr/2026-09-13-capture-engine-clock-is-hse-8mhz-crystal.md)
and the hardware HLD ([`hardware_hld.md`](./hardware_hld.md) §3), not by the pin
map.

---

## 6. Pre-fabrication review

The authoritative board-review gate is
[`harness/hardware/review_checklist.md`](../../harness/hardware/review_checklist.md),
walked by a person before Gerbers are exported; high-side sensing, Kelvin sense
traces, identical channels, shunt value, and ALERT/I2C routing live there and are
**not restated here** to avoid a divergent second copy. This file adds only the
checks specific to the pin *map*:

- [ ] Every symbolic name in this file has a matching net name in the schematic.
- [ ] All ten harness inputs sit on ten distinct pin *numbers* (§4 constraint 2);
      the two CNVR lines are two of EXTI0 through EXTI4 (§3).
- [ ] The phase-code allocation (§4) has been made and verified.
- [ ] SWD pins (§1) are not reused for any other function.
- [ ] The pin-assignment ADR ([`2026-09-09-stm32f411-pin-assignment.md`](../adr/2026-09-09-stm32f411-pin-assignment.md)) is `Accepted`, that is, its primary-source datasheet check has cleared for every borrowed pin.
