# Harness spec: capture-engine pin allocation

**Authority.** This file is the single source of truth for pin assignment on the
two-channel capture engine (STM32F411CEU6, UFQFPN48). Firmware references pins by
the symbolic names defined here; schematic net names match these symbols exactly.

Two provenance regimes apply, and the difference matters:

- **Functions already built in firmware** — the two I2C buses and the TIM2
  timebase — are owned by their firmware headers, not by this file. This file
  *mirrors* [`sensor.h`](../../harness/firmware/capture/sensor.h) and
  [`timing_budget.h`](../../harness/firmware/capture/timing_budget.h) and must not
  drift from them; where the two disagree, the header is right and this file is
  stale. This follows `sensor.h`'s own rule that "the record the Tier 0 gate
  publishes and the values the firmware uses cannot drift apart: they are the
  same constants."
- **Functions not yet built** — the INA226 conversion-ready (CNVR) edge lines and
  the phase-code bits, both Tier 1 per [HLD §6.3](../dsc_hld.md) — are decided
  *here first*, before firmware is written against them. For these, this file is
  the source and conflicts are resolved here.

**Status.** Mixed, by section, and no cell is `MEASURED` — nothing here has been
reproduced on physical hardware:

| Section | Provenance | Binding? |
|---|---|---|
| §1 fixed by silicon | fixed by the part | yes, not a choice |
| §2 I2C buses | `BORROWED` (DocID026289 Rev 4, Table 9), **committed in firmware** | yes |
| §3 CNVR edge lines | `BORROWED`, **proposed**, pending §8 | no, until §8 clears |
| §4 phase-code bits | `BORROWED`, **proposed** (PA2–PA7), pending §8 | no, until §8 clears |
| §5 timebase | `BORROWED` (Rev 4), **committed in firmware** | yes |

Per the reproducibility standard this repository is governed under, a `BORROWED`
cell drawn from a search reading of the datasheet rather than the primary-source
PDF carries no binding force until it clears the check in
[§8](#8-verification-required-before-3-and-4-bind). Every borrowed figure below
carries its datasheet citation at the point it appears; an unlabelled number is
not released.

---

## 1. Fixed by silicon

Not a choice. Routing these pins to anything other than the listed function
bricks the board on first power-on: no flash path, no data path.

| Symbolic name | Pin  | Alternate function | Notes                        |
|---------------|------|--------------------|------------------------------|
| `SWD_IO`      | PA13 | SWDIO              | pull-up on board, 10k typical |
| `SWD_CLK`     | PA14 | SWCLK              | pull-down on board, 10k typical |
| `USB_DM`      | PA11 | OTG_FS_DM          | 22 ohm series resistor       |
| `USB_DP`      | PA12 | OTG_FS_DP          | 22 ohm series resistor       |
| `NRST`        | NRST | reset              | 100 nF to GND, no pull needed |

Unchanged from the original contract. USB device is one of the four peripherals
the Tier 0 `toolchain` gate names, and it is the reason the clock tree lands at
96 MHz rather than 100 MHz (see [§5](#5-timebase-timer)).

---

## 2. I2C buses

Two independent I2C masters on separate peripherals, one per INA226 channel, so a
read stall on one bus cannot block the other. **This section mirrors committed
firmware** ([`sensor.h`](../../harness/firmware/capture/sensor.h),
[`sensor.c`](../../harness/firmware/capture/sensor.c)); the pins, alternate
functions, and package numbers below are the constants those files program.

| Symbolic name | Pin  | Peripheral | AF  | Pkg (UFQFPN48) | Bus role | Serves | Origin |
|---------------|------|------------|-----|----------------|----------|--------|--------|
| `I2C_A_SCL`   | PB6  | I2C1       | AF4 | 42 | I2C master SCL | Channel A INA226 | BORROWED |
| `I2C_A_SDA`   | PB7  | I2C1       | AF4 | 43 | I2C master SDA | Channel A INA226 | BORROWED |
| `I2C_B_SCL`   | PA8  | I2C3       | AF4 | 29 | I2C master SCL | Channel B INA226 | BORROWED |
| `I2C_B_SDA`   | PB4  | I2C3       | AF9 | 40 | I2C master SDA | Channel B INA226 | BORROWED |

Cited from Table 9 of DocID026289 Rev 4. Channel A sits on **I2C1**, Channel B on
**I2C3** — two separate peripherals, not two pins of one, satisfying the harness
constraint that two INA226s never share a bus. [HLD §6.3](../dsc_hld.md) lists
exactly PB6/PB7/PA8/PB4 as already spoken for by the two buses.

**The two alternate-function numbers differ, and that is not a transcription
error.** Table 9 heads AF04 with `I2C1/I2C2/I2C3` and AF09 with `I2C2/I2C3`, so
I2C3 appears in both columns and which one applies is a per-pin fact. On this part
every Port B I2C2/I2C3 SDA sits at AF09 (PB3, PB4, PB8, PB9) while the clocks sit
at AF04; **AF04 on PB4 is blank, not a different function.** Reading the column
header alone and applying AF4 to all four pins is the error
[`sensor.c`](../../harness/firmware/capture/sensor.c) exists to stop being made
twice — it would have initialised cleanly and driven the wrong pin.

**Consequence — JTAG is released.** PB4 is `NJTRST` at reset and belongs to the
SWJ-DP group. Configuring it as `I2C3_SDA` releases the JTAG reset line, so **JTAG
is unavailable once `sensor_bus_init()` runs.** Harmless: the ST-Link attaches
over SWD on PA13/PA14. Recorded in `sensor.h` so it is not rediscovered as a
symptom.

**Bus speed.** The harness budget is stated at 400 kHz (52% bus utilisation per
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

| Symbolic name | Pin | EXTI line | Edge   | Rate         | Serves | Origin |
|---------------|-----|-----------|--------|--------------|--------|--------|
| `CNVR_A`      | PB0 | EXTI0     | rising | 14,286 edges/s | Channel A INA226 | BORROWED, proposed |
| `CNVR_B`      | PB1 | EXTI1     | rising | 14,286 edges/s | Channel B INA226 | BORROWED, proposed |

**Not yet in firmware.** `register_map.h` does not yet contain SYSCFG, EXTI, or
NVIC; edge capture is the first thing Tier 1 adds ([HLD §6.3](../dsc_hld.md)). So
these two pins are a proposal this file makes, not a firmware fact it mirrors.

**Why EXTI0 and EXTI1.** The two CNVR lines are the timing-critical inputs
(14,286 edges/s each; the phase lines carry ~400/s in total). EXTI0–EXTI4 have
dedicated interrupt vectors while EXTI9_5 and EXTI15_10 are shared, so the two
CNVR lines **must take two of EXTI0–EXTI4** ([HLD §6.3](../dsc_hld.md) rule 2) and
never wait behind each other. PB0→EXTI0 and PB1→EXTI1 satisfy this. Because
EXTI line number equals pin number regardless of port, pin numbers 0 and 1 are now
**claimed by these lines and unavailable to any other edge input** — see §4.

Whether PB0/PB1 are bonded out on UFQFPN48 is a §8 check; this file does not
assert it from memory.

---

## 4. Phase-code GPIO — proposed on PA2–PA7 (O3), pending §8

Six GPIO: three bits per node, two nodes, driven by the device under test and
latched by the capture engine with one port-input-register read at the CNVR
timestamp — read, not interrupt-driven. The symbolic names and the encoding are
fixed; the pin numbers below are **proposed** and pending the §8 datasheet check.

| Symbolic name  | Pin | EXTI | Node   | Bit index | Origin |
|----------------|-----|------|--------|-----------|--------|
| `PHASE_A_BIT0` | PA2 | EXTI2 | Node A | 0 (LSB) | BORROWED, proposed |
| `PHASE_A_BIT1` | PA3 | EXTI3 | Node A | 1       | BORROWED, proposed |
| `PHASE_A_BIT2` | PA4 | EXTI4 | Node A | 2 (MSB) | BORROWED, proposed |
| `PHASE_B_BIT0` | PA5 | EXTI5 | Node B | 0 (LSB) | BORROWED, proposed |
| `PHASE_B_BIT1` | PA6 | EXTI6 | Node B | 1       | BORROWED, proposed |
| `PHASE_B_BIT2` | PA7 | EXTI7 | Node B | 2 (MSB) | BORROWED, proposed |

All six sit on GPIOA, so one `GPIOA->IDR` read latches both nodes:
`(IDR >> 2) & 0x7` is Node A, `(IDR >> 5) & 0x7` is Node B.

**Constraints this allocation satisfies** (all binding, from the record):

1. **Each node's three bits contiguous within one GPIO port.** Node A is PA2–PA4,
   Node B is PA5–PA7 — each a contiguous triple in GPIOA, so the handler latches a
   code with one read, one shift and one mask. A code assembled from two ports is
   two reads with a window between them, and a transition landing in that window
   produces exactly the invalid code Gray coding was adopted to make impossible.
   ([`adr/2026-08-17-phase-code-is-parallel-three-bit.md`](../adr/2026-08-17-phase-code-is-parallel-three-bit.md),
   [HLD §6.3](../dsc_hld.md).)
2. **Distinct pin numbers across all ports.** EXTI line number equals pin number
   regardless of port ([HLD §6.3](../dsc_hld.md) rule 1), and under the closed
   six-state cycle all three bits toggle once per event, so `b2` (PA4, PA7) is a
   timing-relevant edge source, not a static level. Numbers **2–7** are used here;
   they are distinct from each other and from pin numbers **0, 1** (the CNVR lines,
   §3). The I2C, USB and SWD pins sit on their own ports and consume no EXTI line,
   so EXTI6/EXTI7 stay free for PA6/PA7 even though PB6/PB7 carry I2C1; PA8
   (I2C3_SCL) is number 8, outside this range.
3. **`b2` (PA4, PA7) driven at all times, never floating**, on both nodes — a
   firmware and layout requirement, not a convention. A floating `b2` fabricates a
   plausible phase rather than an obvious fault
   ([`phase_code_map.md`](./phase_code_map.md), "What b2 means").

**Provenance of PA2–PA7.** This is the withdrawn draft `PA0`–`PA5` shifted up by
two, to clear pin numbers 0 and 1 which the CNVR lines claim on EXTI0/EXTI1 — the
shift preserves the single-`GPIOA`-read property that motivated the original
choice. An earlier-earlier draft used `PC0`–`PC5` and was withdrawn separately
because the UFQFPN48 bonding of Port C could not be confirmed. Both drafts are
recorded so the ladder is visible; neither is carried forward. PA2–PA7 are low
Port A pins, but their UFQFPN48 bonding is a §8 check, not asserted here from
memory.

---

## 5. Timebase timer

One 32-bit timer, free-running, providing the microsecond counter every record is
timestamped against. **This section mirrors committed firmware** — the clock tree
and prescaler are decided, statically asserted in
[`timing_budget.h`](../../harness/firmware/capture/timing_budget.h), and set in
[`timebase.c`](../../harness/firmware/capture/timebase.c). It is **not** open.

| Parameter         | Value | Origin |
|-------------------|-------|--------|
| Peripheral        | TIM2 (32-bit; TIM2/TIM5 are the only 32-bit timers) | BORROWED (Rev 4) |
| APB bus           | APB1 | BORROWED (Rev 4) |
| Kernel clock      | 96 MHz | BORROWED — see clock tree below |
| Prescaler (PSC)   | **95** | `TIM2_PSC_VALUE`, statically checked in `timing_budget.h` |
| Counter frequency | 1 MHz, 1 µs per tick (exact) | derived |
| ARR / mode        | 0xFFFFFFFF, free-running | `timebase.c` |
| Overflow period   | 2³² / 1,000,000 s ≈ 4294.97 s ≈ 71.6 min | clears the ">1 hour" requirement |

**The clock tree, as built** ([HLD §6.1](../dsc_hld.md),
[`timing_budget.h`](../../harness/firmware/capture/timing_budget.h)):

```
HSI 16 MHz / PLLM 16 = 1 MHz
        x PLLN 192   = 192 MHz   (VCO)
        / PLLP 2     = 96 MHz    SYSCLK
        / PLLQ 4     = 48 MHz    USB OTG FS, exactly
AHB  /1 = HCLK  96 MHz
APB1 /2 = PCLK1 48 MHz           (50 MHz max on this part)
TIM2 kernel = PCLK1 x 2 = 96 MHz PSC 95 -> 1 MHz tick exactly
```

`PSC = (TIM2 kernel clock / 1,000,000) − 1 = (96,000,000 / 1,000,000) − 1 = 95.`
Because the APB1 prescaler is /2 (not 1), the timer kernel runs at twice PCLK1 —
a general STM32F4 RCC rule (Table 4 footnote 1), which is why 48 MHz on the bus
gives 96 MHz at TIM2.

The clock lands at **96 MHz rather than the part's 100 MHz maximum** because
`PLLP ∈ {2,4,6,8}` forces a 100 MHz SYSCLK to a 200 or 400 MHz VCO, and neither
divides to 48 MHz; on this part 100 MHz SYSCLK and an in-spec USB clock are
mutually exclusive, and USB is a required peripheral. This is the earlier system
clock decision the pin map used to be blocked on — it is made, and it is here.

---

## 6. Open items

| Item | Status |
|------|--------|
| I2C peripheral assignment | **Closed**, §2. I2C1 + I2C3, committed in firmware. Datasheet extraction still awaits the §8 primary-source check. |
| CNVR edge-line pins | **Proposed**, §3 (PB0/PB1). Awaiting §8 (bonded-out on UFQFPN48). Not yet in firmware. |
| Phase-code pin numbers | **Proposed**, §4 (PA2–PA7). Was the open remainder of O3; now allocated, awaiting §8 (bonded-out on UFQFPN48). Not yet in firmware. |
| Timebase peripheral + prescaler | **Closed**, §5. TIM2, PSC 95, committed in firmware. |

---

## 7. Pre-fabrication review

The authoritative board-review gate is
[`harness/hardware/review_checklist.md`](../../harness/hardware/review_checklist.md),
walked by a person before Gerbers are exported; high-side sensing, Kelvin sense
traces, identical channels, shunt value, and ALERT/I2C routing live there and are
**not restated here** to avoid a divergent second copy. This file adds only the
checks specific to the pin *map*:

- [ ] Every symbolic name in this file has a matching net name in the schematic.
- [ ] All ten harness inputs sit on ten distinct pin *numbers* (§4 constraint 2);
      the two CNVR lines are two of EXTI0–EXTI4 (§3).
- [ ] The phase-code allocation (§4) has been made and verified — this file does
      not yet carry it.
- [ ] SWD pins (§1) are not reused for any other function.
- [ ] The §8 primary-source confirmation has cleared for every borrowed pin.

---

## 8. Verification required before §3 and §4 bind

Sections 2 and 5 are already committed in firmware citing Table 9 / the clock
tree of DocID026289 Rev 4; the datasheet extraction behind them, and the
still-proposed pins in §3, are borrowed from a reading of the datasheet, not the
primary-source PDF opened directly, and not yet reproduced by a second party. Per
the reproducibility standard this repository runs under, that makes them a
proposal, not a result.

1. Open DocID026289 Rev 4, Table 8, and confirm the UFQFPN48 column carries a pin
   number (not a dash) for the pins this file names: **PB6, PB7, PA8, PB4** (§2,
   claimed as package pins 42/43/29/40), **PB0, PB1** (§3), and **PA2–PA7** (§4).
2. Confirm against Table 8/9 directly: **AF4** for PB6/PB7/PA8 and **AF9** for
   PB4 (§2); rising-edge EXTI capability on PB0/PB1 (§3).
3. Once 1 and 2 clear, this file's per-section status for §3 changes to
   `Verified`, and the paired ADR
   ([`../adr/2026-09-09-stm32f411-pin-assignment.md`](../adr/2026-09-09-stm32f411-pin-assignment.md))
   has its status line updated to `Accepted` in step, per the ADR SOP
   ([`../sop/git_sop.md`](../sop/git_sop.md)). §4 (PA2–PA7) binds once its
   confirmation in step 1 clears.
