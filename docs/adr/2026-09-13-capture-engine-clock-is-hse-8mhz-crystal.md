# The capture-engine oscillator is an external 8 MHz HSE crystal, not the internal HSI

**Date:** 2026-09-13 **Status:** Accepted
**Argument of record:** D2 in [`../hardware-harness-v1/hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §2, recomputed in §3
**Updates:** the clock-plan *source* recorded in `harness/firmware/capture/timing_budget.h`, [`../dsc_hld.md`](../dsc_hld.md) §6.1, and [`../hardware-harness-v1/harness_spec.md`](../hardware-harness-v1/harness_spec.md) §5, all of which record an HSI-sourced PLL
**Verification (2026-09-13):** Pinout confirmed against DocID026289 Rev 7, Table 8: OSC_IN = pin 5, OSC_OUT = pin 6, bonded on UFQFPN48 and not among the ten harness inputs. PLL VCO ranges are primary-cited in [`../dsc_hld.md`](../dsc_hld.md) §6.1 (Table 41); `PLLM=4 / PLLN=96` sits inside them. HSI +/-1 % and the absence of HSI48/CRS on this part are standard and search-confirmed, not read from a primary image this session; the decision does not depend on the exact HSI figure.

## Context

The committed clock tree sources the PLL from the internal **HSI 16 MHz** RC
oscillator (`timing_budget.h`, [`../dsc_hld.md`](../dsc_hld.md) §6.1,
[`harness_spec.md`](../hardware-harness-v1/harness_spec.md) §5, whose tree begins
"HSI 16 MHz / PLLM 16"). That was correct for Tier 0, which needed only a blink and
both I2C peripherals to initialise cleanly. It is wrong for two functions the board
now commits to, and both were settled in the hardware HLD.

**USB.** The hardware HLD commits native USB CDC as the host transport (D1). USB
2.0 full-speed requires the transceiver clock within **+/-0.25 % (2500 ppm)**
(`BORROWED`, USB 2.0 spec section 7.1.11). The HSI is factory-trimmed to +/-1 % and
drifts further over temperature (`BORROWED`, DocID026289 Rev 4 HSI characteristics
table), and STM32F411 has no HSI48 oscillator or clock-recovery system (CRS) to
trim a USB clock against bus start-of-frame (`BORROWED`, pending the §12 check in
the hardware HLD). An HSI-sourced 48 MHz USB clock is therefore out of spec on this
part.

**Timebase.** The instrument exists to produce one shared microsecond timeline
([`../dsc_hld.md`](../dsc_hld.md) §1.1: "the single clock is the architecture"). At
HSI +/-1 %, a 20-minute campaign
([`harness_timing_budget.md`](../hardware-harness-v1/harness_timing_budget.md) §5)
carries up to **+/-12 s** of scale error on that timeline. The jitter budget
(sigma < 2 us, asserted in `timing_budget.h`) is short-term tick-to-tick noise and
is met by TIM2 regardless; long-term scale accuracy is a separate axis, set by the
oscillator and not by the counter. A crystal at +/-20 ppm is ~500x tighter.

## Alternatives considered

- **Keep the internal HSI.** For: zero added parts, no crystal footprint, the OSC
  pins free for GPIO. Against: native USB is out of spec across temperature on this
  part, and every timestamp carries +/-1 % scale error that lands directly on any
  energy figure integrated over an awake window. Rejected: the two functions that
  fail are the two the board is for.
- **External HSE crystal, 8 MHz.** Chosen. ~+/-20 ppm typical (`BORROWED`, crystal
  datasheet), which clocks USB in spec and holds the timebase ~500x tighter than
  HSI. 8 MHz divides to a clean 2 MHz PLL input at PLLM=4 (ST's recommended
  low-jitter VCO input) and is universally stocked.
- **HSE crystal at another frequency (25 MHz, as on several ST reference boards).**
  For: drop-in for existing 25 MHz stock. Against: no benefit here; it complicates
  PLLM with no gain, where 8 MHz keeps the recompute a two-constant change.
  Rejected on simplicity.
- **TCXO or a packaged oscillator.** For: tighter still, no load-cap tuning.
  Against: a +/-20 ppm crystal is already two orders past the requirement; a TCXO
  is cost and board area bought for accuracy no gate asks for. Held as the
  escalation if a measured campaign shows crystal drift is itself a material term,
  not before, mirroring the front-end deferral in
  [`two-channel-harness-built-in-house`](./2026-08-09-two-channel-harness-built-in-house.md).

## Decision

The capture engine takes an external **8 MHz HSE crystal** as its oscillator. The
PLL is re-sourced from HSE and re-solved so every downstream frequency is
unchanged:

```
HSE 8 MHz / PLLM 4 = 2 MHz      (Table 41: 0.95-2.10 MHz; 2 MHz is ST's low-jitter point)
        x PLLN 96  = 192 MHz    (Table 41: 100-432 MHz)   <- VCO unchanged
        / PLLP 2   = 96 MHz     SYSCLK                     <- unchanged
        / PLLQ 4   = 48 MHz     USB OTG FS, exactly        <- unchanged
AHB  /1 = HCLK  96 MHz                                     <- unchanged
APB1 /2 = PCLK1 48 MHz                                     <- unchanged
TIM2 kernel = PCLK1 x 2 = 96 MHz, PSC 95 -> 1 MHz tick    <- unchanged
```

Only **PLLM (16 -> 4)** and **PLLN (192 -> 96)** move. PLLP=2, PLLQ=4, SYSCLK
96 MHz, USB 48 MHz exactly, PCLK1 48 MHz, TIM2 kernel 96 MHz, PSC=95, and the exact
1 us tick are all identical to the committed tree. The crystal changes the
*stability* of the tick, not its value. Two load capacitors size to
`C_ext = 2 x (C_L - C_stray)` per the chosen crystal
([`hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §6); a series
drive-limit resistor footprint is fitted DNP.

The alternative **PLLM=8 / PLLN=192** holds the PLL input at 1 MHz and is the
minimal numeric diff to the committed tree; **PLLM=4 / PLLN=96** is preferred for
the 2 MHz low-jitter VCO input and is recorded as the decision.

## Consequences

- `timing_budget.h` changes: the PLL source, PLLM, PLLN, and the RCC sequence in
  `timebase.c` that currently starts HSI. HSE start-up plus the HSERDY wait and an
  HSE-fail response are added; PLLCFGR stays a read-modify-write so PLLQ is not
  zeroed ([`../dsc_hld.md`](../dsc_hld.md) §6.1). This is a firmware change under
  the bare-metal ADR, done by hand from the datasheet.
- [`harness_spec.md`](../hardware-harness-v1/harness_spec.md) §5 and
  [`../dsc_hld.md`](../dsc_hld.md) §6.1 change from "HSI 16 MHz / PLLM 16" to the
  HSE tree above. The "the record and the firmware cannot drift apart" rule from
  `sensor.h` applies: the firmware constants and the documented tree are edited in
  one change, not two.
- Two pins (OSC_IN / OSC_OUT, PH0 / PH1 on this part) are consumed by the crystal
  (`BORROWED`, pending the UFQFPN48 bonding check in
  [`hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §12). They are not
  among the ten harness inputs, so no input pin is displaced; confirm the bond-out.
- A static assertion is added in the `timing_budget.h` style so the recomputed PLL
  still yields exactly 48 MHz USB and a 1 MHz tick; a mistyped PLLN fails the build
  rather than the bench.
- Paired with
  [`host-transport-native-usb`](./2026-09-13-host-transport-native-usb-hal-scoped-to-measurement-path.md)
  of the same date: native USB is the reason USB compliance is one of the two
  arguments here, and neither ADR stands fully without the other.

## What would reopen this

- The §12 checks failing: if this part in fact carries a USB-capable HSI trim path,
  or if OSC_IN / OSC_OUT are not bonded on UFQFPN48, the source or the part is
  revisited.
- The host transport reverting off native USB (it will not, under D1 and the paired
  transport ADR), which removes the USB-compliance half of the argument but leaves
  the timebase half intact.
- A measured campaign showing +/-20 ppm crystal drift is itself a material term in
  a reported figure, which escalates to a TCXO.
