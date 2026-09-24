# Hardware HLD: capture-engine board (Obol Harness v1)

**Authority.** This file is the board-level design record for the capture-engine
PCB sketched as *FortuneLabs/Obol Harness v1.0*. It sits below the system HLD
([`../dsc_hld.md`](../dsc_hld.md)) and beside the pin map
([`harness_spec.md`](./harness_spec.md)) and the arithmetic the harness is sized
by ([`harness_timing_budget.md`](./harness_timing_budget.md)). Those three are the
source of record for, respectively, the architecture, the pin allocation, and the
signal/timing budget; where this file disagrees with any of them, **they win and
this file is stale** - the same rule [`../dsc_hld.md`](../dsc_hld.md) states for
itself.

What this file adds that none of them holds: the **count of support components**
(decoupling, LDO, crystal network, pull-ups, series and termination resistors,
protection, indicators), each derived from a declared formula or carried from a
datasheet with its citation. It is the input to the schematic and the BOM, and
the precondition for the KiCad project that
[`../../harness/hardware/README.md`](../../harness/hardware/README.md) tracks,
scaffolded on 2026-09-22 and now under design.

**The single clock is still the architecture.** Every hardware decision below is
subordinate to one goal from [`../dsc_hld.md`](../dsc_hld.md) §1.1: keep the TIM2
microsecond timebase honest. That is why §3 moves the clock off the internal RC
and onto a crystal, and why the analog-adjacent parts get filtered rather than
shared blindly.

**Status.** Mixed by section, and **no cell is `MEASURED`** - nothing here has
touched hardware. Per the reproducibility standard this repository runs under
([`../sop/git_sop.md`](../sop/git_sop.md)), a `BORROWED` figure drawn from a
datasheet reading rather than the primary-source PDF opened directly carries no
binding force until it clears the check in
[§12](#12-verification-required-before-counts-bind). Every borrowed figure carries
its citation at the point it appears; an unlabelled number is not released.

| Section | Content | Provenance |
|---|---|---|
| §1 | Board architecture, remote-sense topology | `DERIVED` from the sketch + [`two-channel-harness`](../adr/2026-08-09-two-channel-harness-built-in-house.md) |
| §2 | Decisions locked this session | records three choices; two conflict with committed docs (see §11) |
| §3 | Clock tree recompute (HSE 8 MHz) | `DERIVED`; supersedes the HSI tree in `timing_budget.h` (pending ADR) |
| §4 | Power tree + LDO sizing | `DERIVED` budget (est. currents); LDO chosen in §14 |
| §5-§10 | Per-block component counts | `DERIVED` formulas; §5/§9 primary-confirmed; parts in §14 |
| §11 | Consolidated BOM | roll-up of §4-§10 |
| §12 | Verification required | the primary-source checks that bind the `BORROWED` cells |
| §15 | Open items + the spike that closes them | what the fab trigger now waits on, ranked by cost of finding out late |

The counts below are the intended design; they are not released to fab on
datasheet reading alone. §15 places a throwaway spike (a Black Pill and an
INA226 breakout) in front of the fab trigger, to close O1 and the architecture
gates and to demonstrate the USB transport (R1) on real F411 silicon first. No
decision in §13 is reversed by it.

---

## 1. Board architecture

The sketch resolves a topology the written record left implicit. The system HLD
([`../dsc_hld.md`](../dsc_hld.md) §1.1) draws each INA226 next to its node's shunt
with a Kelvin connection, and [`harness_timing_budget.md`](./harness_timing_budget.md)
§6 makes the Kelvin connection "a layout requirement, not a refinement." A shunt
sensed by Kelvin traces must sit **at the node**, not on the capture board - so
each INA226 is **remote**, and the I²C bus to it crosses a cable.

I²C was not designed to cross a cable. The sketch's answer is the **PCA9615**, a
differential I²C bus buffer, one per channel: it converts each single-ended
open-drain bus into a differential pair that survives a cable, then converts it
back at the far end. This is the mechanism behind the requirement in
[`review_checklist.md`](../../harness/hardware/review_checklist.md) that the two
buses be "brought out separately," and it introduces a new catalog component on
the same footing as the INA226 - see §11's consequences.

```
        CAPTURE-ENGINE BOARD (this file)                 REMOTE (per node)
  +-----------------------------------------+
  |  USB-C --22R--> STM32F411   PA11/PA12    |
  |  (native USB CDC, §7)   |                |
  |                         | I2C1 (PB6/PB7) |          +------------------+
  |   HSE 8MHz xtal --------+---> PCA9615 ===|== diff ==>| PCA9615 -> INA226|--Kelvin--> node S shunt
  |   (§3)                  |    (ch A, §9)  | JST-PH 1 |  (remote)        |
  |                         | I2C3 (PA8/PB4) |  (§10)   |  CNVR, 3b phase  |--> from node S
  |   5V->3.3V LDO ---------+---> PCA9615 ===|== diff ==>| PCA9615 -> INA226|--Kelvin--> node R shunt
  |   (§4)   3.3V to remote |    (ch B, §9)  | JST-PH 2 |  (remote)        |
  |          over JST-PH    | EXTI PB0/PB1   |  (§10)   |  CNVR, 3b phase  |--> from node R
  |   LEDs (§8) SWD (§8)    | EXTI PA2..PA7  |          +------------------+
  +-----------------------------------------+
```

**Signals crossing each JST-PH** (from [`harness_spec.md`](./harness_spec.md) §2-§4
and [`harness_timing_budget.md`](./harness_timing_budget.md) §1): one differential
I²C bus (SCL±, SDA± = 4 lines), one CNVR edge line (single-ended), three phase
bits (single-ended), 3.3 V and GND. Ten logical pins per connector - see §10. The
campaign-trigger and spare inputs from the ten-input inventory are board-side, not
on the cable (§8).

**Power boundary (locked, §2):** the board supplies 3.3 V to the remote INA226 and
the remote PCA9615 over the JST-PH; it does **not** power the DUT. The DUT powers
itself, and the harness only senses its rail. This keeps the sense side on the
harness ground - which is what a shared timebase needs - and keeps the LDO small
(§4).

---

## 2. Decisions locked this session

Three choices were open where the sketch met the record. Each is recorded here
with its consequence; §11 lists the ADRs each one obliges.

| # | Decision | Chosen | Displaces / conflicts with |
|---|---|---|---|
| D1 | Host data path | **Native USB CDC** on OTG_FS (PA11/PA12) | none - confirms [`harness_spec.md`](./harness_spec.md) §1 and [`harness_timing_budget.md`](./harness_timing_budget.md) §5; **forecloses** the UART exit named in [`../dsc_hld.md`](../dsc_hld.md) R1 (see §11) |
| D2 | Clock source | **HSE crystal, 8 MHz** | supersedes the **HSI** PLL source in [`../dsc_hld.md`](../dsc_hld.md) §6.1 / `timing_budget.h` (§3) |
| D3 | Remote power | **Sense side only** (3.3 V to remote INA226 + PCA9615; DUT self-powered) | new; sizes the LDO in §4 |

**Why native USB.** The stream is 114 kB/s
([`harness_timing_budget.md`](./harness_timing_budget.md) §5); USB full-speed is
12 Mbit/s, ~10× headroom, and the STM32 has the peripheral on-die, so native USB
adds no bridge IC and removes the baud question. The cost is firmware, not parts,
and it lands on an already-open item (§11, R1).

**Why the crystal.** Two reasons, either sufficient:

1. **Timebase accuracy.** HSI is ±1 % over temperature (`BORROWED`, DocID026289
   Rev 4, HSI accuracy table - §12 check). Over a 20-minute campaign
   ([`harness_timing_budget.md`](./harness_timing_budget.md) §5) that is up to
   ±12 s of scale error on the one clock the whole instrument rests on. A crystal
   at ±20 ppm (`BORROWED`, crystal datasheet - §12) is ±0.024 s over the same
   window, ~500× better. For an instrument whose sole purpose is timestamping,
   the clock accuracy *is* the measurement.
2. **USB compliance.** USB 2.0 full-speed requires ±0.25 % (2500 ppm) transceiver
   clock (`BORROWED`, USB 2.0 spec §7.1.11). F411 has no HSI48/CRS trimming path,
   so an HSI-sourced USB clock is out of spec across temperature. D1 therefore
   *requires* D2.

**Why power on sense side only.** Powering the DUT would mean sourcing two ESP32-S3
peaks (~330 mA each, [`harness_timing_budget.md`](./harness_timing_budget.md) §6)
and injecting the harness's own regulator noise into the very rail being metered.
Powering nothing remote would put the sense side on the DUT's ground and power
decisions. Powering only the sense side keeps the LDO at a few hundred mA (§4) and
the sense ground continuous with the timebase ground.

---

## 3. Clock tree, recomputed for HSE 8 MHz

The committed tree ([`../dsc_hld.md`](../dsc_hld.md) §6.1, `timing_budget.h`)
sources the PLL from **HSI 16 MHz**. D2 moves it to an **8 MHz crystal**. The
recompute keeps every downstream frequency identical:

```
HSE 8 MHz / PLLM 4 = 2 MHz      (Table 41: 0.95-2.10 MHz; 2 MHz is ST's low-jitter point)
        x PLLN 96  = 192 MHz    (Table 41: 100-432 MHz)   <- VCO unchanged
        / PLLP 2   = 96 MHz     SYSCLK                     <- unchanged
        / PLLQ 4   = 48 MHz     USB OTG FS, exactly        <- unchanged
AHB  /1 = HCLK  96 MHz                                     <- unchanged
APB1 /2 = PCLK1 48 MHz                                     <- unchanged
TIM2 kernel = PCLK1 x 2 = 96 MHz, PSC 95 -> 1 MHz tick    <- unchanged
```

Only two constants change: `PLLM` 16→4 and `PLLN` 192→96. `PSC = 95`, the exact
1 MHz tick, the 71.6-minute wrap, and the σ < 2 µs jitter budget asserted in
`timing_budget.h` all survive untouched - the crystal changes the *stability* of
the tick, not its *value*. (`DERIVED`; the Table 41 ranges are `BORROWED`,
DocID026289 Rev 4, and already cited in [`../dsc_hld.md`](../dsc_hld.md) §6.1.)

An equivalent set (`PLLM 8`, `PLLN 192`) holds the PLL input at 1 MHz and changes
only `PLLM`; it is the minimal numeric diff to the committed tree. `PLLM 4` /
`PLLN 96` is preferred here because 2 MHz is ST's recommended VCO-input frequency
for lowest PLL jitter, and the timebase is the thing that benefits.

**Component consequence:** one HSE crystal network (§6). No LSE / 32.768 kHz
crystal - there is no RTC requirement; the timebase is TIM2, not the RTC.

---

## 4. Power tree and LDO sizing

One rail: 5 V from USB-C VBUS → 3.3 V. Under D3 the 3.3 V rail feeds the whole
board **and** the two remote sense sides over the JST-PH cables.

**Current budget** (`DERIVED` sum; per-device figures from vendor datasheets; LDO
selected in §14):

| Load | Qty | Per-unit | Source | Subtotal |
|---|---:|---:|---|---:|
| STM32F411 run @ 96 MHz, USB + peripherals active | 1 | ~50 mA | est. for sizing, DocID026289 I_DD run (conservative) | 50 mA |
| PCA9615 local (on-board) | 2 | ~5 mA | est. for sizing, PCA9615 Rev 2 | 10 mA |
| PCA9615 remote (over cable) | 2 | ~5 mA | est. for sizing, PCA9615 Rev 2 | 10 mA |
| INA226 remote | 2 | 0.42 mA | SBOS547B I_Q max | 1 mA |
| Indicator LEDs | 3 | ~1.5 mA | `DERIVED` §8 | 5 mA |
| Pull-up / bus sink, average | - | - | `DERIVED`, negligible average | ~2 mA |
| **Subtotal** | | | | **~78 mA** |
| Design margin (≥50 %) | | | inrush, tolerance, USB enumeration peak | ~40 mA |
| **Design point** | | | | **~120 mA** |

**LDO selection.** One LDO, rated **≥ 300 mA** (chosen ≥ 500 mA class for
headroom and startup inrush). Dropout at 5 V→3.3 V is 1.7 V, trivial for any
part. A single LDO suffices; there is no second analog rail because the sensors
are digital over I²C and no on-chip ADC is used - VDDA needs filtering, not
regulation (§5). (`DERIVED` from the budget; the specific part and its dropout/PSRR
are `BORROWED` once chosen.)

**LDO support passives** (`BORROWED`, per the chosen LDO's datasheet - §12): input
capacitor + output capacitor sized for stability. Placeholder count: 1× C_in
(1 µF), 1× C_out (value per datasheet; AMS1117-class wants ~22 µF, modern
low-dropout parts ~1-10 µF), 1× bulk 10 µF on VBUS. Remote IR drop at ~15 mA/node
over a short JST-PH lead is < 1 % of 3.3 V and is neglected; noted, not counted.

**Count: 1 LDO + 3 capacitors** (C_in, C_out, VBUS bulk).

---

## 5. Decoupling network: STM32F411CEU6 (UFQFPN48)

Method: one 100 nF close-in capacitor per supply pin, plus one bulk reservoir per
ST's power-supply scheme, plus the two mandatory analog/core nets the part
requires. The pin count and VCAP presence are now **confirmed against DocID026289
Rev 7, Table 8** (§12 item 1 cleared): three VDD pins (24, 36, 48), three VSS pins
(23, 35, 47) plus the exposed pad, VSSA bonded (pin 8), VCAP_1 bonded (pin 22).

| Net | Rule | Count | Provenance |
|---|---|---:|---|
| VDD (pins 24, 36, 48) | 1× 100 nF per VDD pin | 3× 100 nF | confirmed, DocID026289 Rev 7 Table 8 |
| VDD bulk | 1× 4.7 µF reservoir near package | 1× 4.7 µF | `BORROWED` ST AN4488 / power scheme |
| VDDA/VREF+ (pin 9, merged on 48-pin) | ferrite bead from VDD, then 1 µF + 10 nF at the pin | 1 ferrite + 1 µF + 10 nF | confirmed pin, DocID026289 Rev 7 Table 8; no ADC, so filter not regulate |
| VBAT (pin 1) | 1× 100 nF (VBAT tied to VDD) | 1× 100 nF | confirmed, DocID026289 Rev 7 Table 8 |
| VCAP_1 (pin 22, internal core LDO) | 1× 2.2 µF low-ESR ceramic | 1× 2.2 µF | confirmed present (VCAP_2 absent), DocID026289 Rev 7 Table 8 |
| NRST (pin 7) | 1× 100 nF to GND | 1× 100 nF | [`harness_spec.md`](./harness_spec.md) §1 |
| VSS (23, 35, 47) + exposed pad, VSSA (8) | tie to GND; no component | 0 | confirmed, DocID026289 Rev 7 Table 8 |

**Count (confirmed): 100 nF ×5** (VDD ×3, VBAT, NRST), 4.7 µF ×1, 1 µF ×1,
10 nF ×1, 2.2 µF ×1 (VCAP_1), ferrite bead ×1. Bound against DocID026289 Rev 7
Table 8; not yet measured on hardware.

**Correction, 2026-09-22.** This section read "two VDD pins (24, 36)" and counted
100 nF ×4 until the STM32 block was drawn. Table 8 bonds a **third VDD at pin 48**,
so the close-in count is five, not four. The error reached §11 and §14.2 and is
corrected in both. It was caught by placing the part in KiCad, which is the check
a datasheet reading does not replace.

---

## 6. Crystal network - HSE 8 MHz

Load capacitors are matched, one per oscillator pin:

```
C_ext = 2 x (C_L - C_stray)
```

with `C_L` the crystal's specified load capacitance and `C_stray` the OSC pin +
trace parasitic (~5 pF typical on F4). Worked example for `C_L = 18 pF`:
`C_ext = 2 x (18 - 5) = 26 pF` → nearest standard **27 pF**. The formula is
`DERIVED`; the selected 8 MHz crystal fixes `C_L = 18 pF` (§14), giving **2x 27 pF
C0G** load caps (final trim per AN2867 at layout).

| Item | Count | Value | Provenance |
|---|---:|---|---|
| HSE crystal | 1 | 8 MHz | D2, this file |
| Load capacitors | 2 | `C_ext` per formula (~27 pF for C_L = 18 pF) | `DERIVED` over `BORROWED` C_L |
| Series resistor R_ext (OSC_OUT) | 0-1 | often DNP on F4; fit footprint, populate only if drive-level check requires | `BORROWED` ST oscillator design note |
| Feedback resistor | 0 | internal to F4 | `BORROWED` DocID026289 |

**Count: 1 crystal + 2 caps** (+ 1 DNP resistor footprint).

---

## 7. USB-C front end (native USB CDC)

The board is a bus-powered **sink/UFP** on a USB-C receptacle. That fixes several
counts by spec rather than by choice.

| Item | Count | Value | Provenance |
|---|---:|---|---|
| USB-C receptacle | 1 | - | D1 |
| CC pull-downs (Rd) | 2 | 5.1 kΩ, one on CC1 and CC2 | `BORROWED` USB Type-C Cable & Connector spec, Rd = 5.1 kΩ (orientation unknown → both) |
| DM/DP series resistors | 2 | 22 Ω | [`harness_spec.md`](./harness_spec.md) §1 |
| ESD protection | 1 | USBLC6-2SC6 (SOT-23-6) on DM/DP + VBUS | selected, §14; ST USBLC6-2 |
| VBUS sense divider | 2 | 10 kΩ / 10 kΩ (5 V → 2.5 V) to a GPIO | selected, §14 |
| VBUS bulk / decoupling | (counted in §4) | 10 µF + 100 nF | `BORROWED` |
| VBUS protection (PTC/fuse) | 0-1 | resettable, recommended | design choice |

**Count (recommended set): 2× 5.1 kΩ + 2× 22 Ω + 1 ESD array + 2 divider R + 1
receptacle** (+ optional PTC).

The two 22 Ω and the SWD pins (§8) are already `fixed by silicon` in
[`harness_spec.md`](./harness_spec.md) §1; this section only adds the connector,
the Type-C sink resistors, and the protection the pin map does not cover.

---

## 8. I²C pull-ups, indicators, trigger, SWD, boot

### 8.1 Local I²C pull-ups (STM32 ↔ PCA9615 local side)

Two buses × two lines. Sizing bounds (`BORROWED` limits, `DERIVED` result):

```
R_p(min) = (VDD - V_OL) / I_OL = (3.3 - 0.4) / 3 mA   = 967 ohm
R_p(max) = t_r / (0.8473 x C_b) = 300 ns / (0.8473 x 40 pF) = 8.85 kohm
```

`I_OL = 3 mA`, `V_OL = 0.4 V`, fast-mode `t_r(max) = 300 ns` (`BORROWED` I²C spec
UM10204 / INA226 SBOS547B). Local `C_b ≈ 40 pF` - the bus sees only the STM32 pin
and the PCA9615 local side; the remote INA226 is *behind* the buffer and adds no
capacitance here. Chosen: **2.2 kΩ**, comfortably inside [967 Ω, 8.85 kΩ].

**Count: 4 × 2.2 kΩ.** (The remote-side pull-ups - PCA9615 ↔ INA226, 2 per bus -
live on the remote sense board, not this BOM; noted in §11.)

### 8.2 Indicators

`R = (3.3 - V_f) / I_f`, `DERIVED`. For a green LED (`V_f ≈ 2.0 V`, `BORROWED`
LED datasheet) at `I_f = 1.5 mA`: `R = 1.3 / 0.0015 ≈ 870 Ω` → **1 kΩ** standard.

| LED | Driven by | Count |
|---|---|---:|
| Power (3.3 V present) | rail | 1 + 1 R |
| Heartbeat / status | GPIO | 1 + 1 R |
| Capture-active / USB activity | GPIO | 1 + 1 R |

**Count: 3 LEDs + 3 resistors.**

### 8.3 Campaign trigger + spare

From the ten-input inventory ([`harness_timing_budget.md`](./harness_timing_budget.md)
§1) these two are board-side, not on the cable. Trigger: 1 push-button + 1× 10 kΩ
pull-up + optional 100 nF debounce (debounce may be firmware - count the RC as
optional). Spare: 1 test point, no components.

**Count: 1 button + 1 × 10 kΩ (+ optional 100 nF).**

### 8.4 SWD and boot

| Item | Count | Value | Provenance |
|---|---:|---|---|
| SWD connector | 1 | 4-pin or 2×5 1.27 mm | design choice |
| SWDIO pull-up | 1 | 10 kΩ | [`harness_spec.md`](./harness_spec.md) §1 |
| SWCLK pull-down | 1 | 10 kΩ | [`harness_spec.md`](./harness_spec.md) §1 |
| BOOT0 pull-down | 1 | 10 kΩ (boot from flash) | `BORROWED` DocID026289 boot table |
| DFU jumper/test point | 0-1 | native USB DFU is available under D1 | design choice |

**Count: 3 × 10 kΩ + 1 connector** (+ optional DFU jumper).

---

## 9. PCA9615 differential I²C buffers

Two buffers, one per channel. The PCA9615 has **two supplies** (V_DD(A) card side
2.3-5.5 V, V_DD(B) line side 3.0-5.5 V, best at 5 V), so each IC decouples on both
rails. The differential bus is terminated per the datasheet (section 7.2, Figure 5):
~100 ohm characteristic impedance terminated at both ends, three resistors per pair
(1 termination + 2 idle-bias to V_DD(B)/V_SS). Confirmed against NXP PCA9615 Rev 2.

| Item | Count | Value | Provenance |
|---|---:|---|---|
| PCA9615 IC | 2 | - | this file (new catalog part) |
| Decoupling | 4 | 100 nF per supply pin, both V_DD(A) and V_DD(B) | confirmed, PCA9615 Rev 2 (two supplies) |
| EN handling | 0 | internal pull-up to V_DD(A); floats high to enable | confirmed, PCA9615 Rev 2 Table 3 |
| Differential termination / bias | 3 per pair | ~100 ohm term both ends + idle bias, section 7.2 / Fig 5 | confirmed structure, PCA9615 Rev 2; values at schematic |

**Count: 2 PCA9615 + 4 × 100 nF** (two supplies each). The differential termination
network is now defined (3 resistors per pair, terminated both ends); exact resistor
values bind at schematic.

The differential *bus* pull-ups and the remote local-side pull-ups belong to the
remote sense board; they are enumerated when that board gets its own HLD (§11).

---

## 10. Cable interface: CNVR and phase, single-ended over JST-PH

Per node the JST-PH carries the differential I²C pair (buffered, §9) plus four
single-ended lines: CNVR (1) and phase (3). Two nodes → **8 single-ended
cable-facing inputs.**

**CNVR pull-up.** [`harness_spec.md`](./harness_spec.md) §3 specifies the STM32
*internal* pull-up on the CNVR line. Over a cable an internal pull-up is weak and
noise-prone, so this file **proposes an external 10 kΩ per CNVR line** at the
board end (2 total), and flags the divergence from the pin map as a decision for
§12/the review walk - not a silent override.

**Series protection.** A series resistor on each cable-facing single-ended line
(CNVR ×2, phase ×6 = 8) limits injected transients and edge overshoot. Value
33-100 Ω (`BORROWED` general practice; low enough not to slow the ~400/s phase
edges or the 14,286/s CNVR edges). Recommended, not mandatory.

**ESD.** Optional TVS array(s) on the eight cable lines; recommended if the cables
leave the enclosure.

| Item | Count | Value | Status |
|---|---:|---|---|
| CNVR external pull-up | 2 | 10 kΩ | **proposed** (diverges from `harness_spec.md` §3 internal) |
| Series protection R | 8 | 33-100 Ω | recommended |
| Cable-line TVS | 0-2 arrays | low-cap | optional |
| JST-PH connector | 2 | ≥10-pin (3V3, GND, SCL±, SDA±, CNVR, phase[3]) | this file |

**JST-PH pin count (`DERIVED`):** 3.3 V (1) + GND (1) + SCL± (2) + SDA± (2) +
CNVR (1) + phase (3) = **10 pins minimum** per connector; grounding practice for
the single-ended returns may push to a larger shell or a second ground pin -
settled at layout.

---

## 11. Consolidated BOM count (per capture-engine board)

Roll-up of §4-§10. The §5 and §9 counts are now confirmed against primary sources
(§12); LDO, crystal, and Type-C values still bind at part selection. Nothing here
is `MEASURED`.

| Block | Actives | Passives (count) |
|---|---|---|
| §4 Power | 1 LDO | 3 caps (C_in, C_out, VBUS bulk) |
| §5 MCU decoupling | - | 100 nF ×5, 4.7 µF ×1, 1 µF ×1, 10 nF ×1, 2.2 µF ×1 (VCAP_1), ferrite ×1 |
| §6 Crystal | 1 crystal | 2 load caps (+1 DNP R) |
| §7 USB-C | 1 receptacle, 1 ESD array | 5.1 kΩ ×2, 22 Ω ×2, divider ×2 (+optional PTC) |
| §8.1 I²C pull-ups | - | 2.2 kΩ ×4 |
| §8.2 Indicators | 3 LEDs | 3 R |
| §8.3 Trigger | 1 button | 10 kΩ ×1 (+optional 100 nF) |
| §8.4 SWD/boot | 1 connector | 10 kΩ ×3 (+optional DFU jumper) |
| §9 PCA9615 | 2 PCA9615 | 100 nF ×4, differential termination (3 R/pair) |
| §10 Cable interface | - | 10 kΩ ×2, 33-100 Ω ×8, TVS ×0-2 |

**Active total:** STM32F411 ×1, LDO ×1, PCA9615 ×2, ESD array ×1 = **5 ICs**
(+ 3 LEDs, 1 crystal, 3 connectors, 1 button).

**Passive total (expected case):** resistors ≈ 4 (I²C) + 2 (CC) + 2 (VBUS
divider) + 3 (LED) + 1 (trigger) + 3 (SWD/boot) + 2 (CNVR) + 8 (series) = **~25
resistors**; capacitors ≈ 3 (power) + 9 (MCU decoupling) + 2 (crystal) + 2
(PCA9615) = **~16 capacitors**; plus 1 ferrite. Termination passives (§9) and
optional TVS/PTC are added once §12 and the review walk settle them.

**Not on this board (companion remote sense board, listed so the count is not
mistaken for the whole harness):** per node - 1 INA226, 1 PCA9615, 1× 0.1 Ω /
0.1 % shunt ([`harness_timing_budget.md`](./harness_timing_budget.md) §6),
remote-side I²C pull-ups (2), CNVR pull-up, decoupling, and the INA226 input
filter noted in [`review_checklist.md`](../../harness/hardware/review_checklist.md).
That board deserves its own HLD; this file scopes only the capture engine.

---

## 12. Verification required before counts bind

Per the reproducibility standard ([`../sop/git_sop.md`](../sop/git_sop.md)), the
`BORROWED` cells above are proposals until reproduced against the primary source.
Status as of 2026-09-13 (primary sources read directly this session where marked
**CLEARED**):

1. **STM32F411 UFQFPN48 power pins. CLEARED** (DocID026289 Rev 7, Table 8): VDD at
   pins 24, 36 and 48 (three), VSS at 23, 35 and 47 plus the exposed pad, VSSA at
   pin 8, VCAP_1 bonded at pin 22 (VCAP_2 absent), VBAT pin 1, VDDA/VREF+ merged at
   pin 9. §5 counts are bound. Re-read 2026-09-22: the earlier entry recorded two
   VDD pins and missed pin 48, which is the correction noted in §5.
2. **HSI and HSE tolerances. PARTIAL.** PLL VCO ranges are primary-cited in
   [`../dsc_hld.md`](../dsc_hld.md) §6.1 (Table 41) and `PLLM=4 / PLLN=96` sits
   inside them; HSI ±1 % and the absence of HSI48/CRS are standard and
   search-confirmed, not read from a primary image this session. The clock ADR was
   accepted on this basis (the decision does not depend on the exact HSI figure).
3. **PCA9615. CLEARED** (NXP PCA9615 Rev 2, read directly): differential I2C
   buffer, two supplies (V_DD(A) 2.3-5.5 V, V_DD(B) 3.0-5.5 V), rated to 1 MHz,
   termination network defined in section 7.2 / Figure 5. §9 counts are bound;
   resistor values land at schematic.
4. **LDO caps. CLEARED** (§14): AP2112K-3.3 selected; C_in = C_out = 1 µF ceramic
   per its datasheet, VBUS bulk 10 µF.
5. **Crystal. CLEARED** (§14): 8 MHz, C_L = 18 pF selected; load caps 2× 27 pF C0G
   (§6). OSC pins confirmed bonded (pins 5/6, Table 8).
6. **USB Type-C Rd.** Confirm Rd = 5.1 kΩ for a sink and the DM/DP 22 Ω (§7). USB
   device pins confirmed bonded (PA11/PA12 = pins 32/33, Table 8).
7. **CNVR pull-up divergence (§10).** Resolve the external-vs-internal CNVR
   pull-up against [`harness_spec.md`](./harness_spec.md) §3 at the review walk.

None of these is `MEASURED`; they are primary-source confirmations that turn a
`BORROWED` proposal into a bound figure. The board is not sent to fab until they
clear, the person-walk in [`review_checklist.md`](../../harness/hardware/review_checklist.md)
is done, and the §15 spike has closed O1, `rate`, `dropped`, `wrap`, `jitter` and
demonstrated the USB transport (R1) on silicon - this file is a precondition for
spending money, not evidence about the
harness ([`../../harness/hardware/README.md`](../../harness/hardware/README.md)).

---

## 13. Consequences: records this file obliges

Three of the decisions here change committed artifacts, and in this repository a
change to a committed artifact is an ADR, not a quiet edit. All three ADRs are
**Accepted** (2026-09-13), paired with this file:

- **D2 changes the clock source.** [`../dsc_hld.md`](../dsc_hld.md) §6.1,
  `timing_budget.h`, and [`harness_spec.md`](./harness_spec.md) §5 record an **HSI**
  PLL source; §3 moves it to an 8 MHz **HSE** crystal. Recorded in
  [`2026-09-13-capture-engine-clock-is-hse-8mhz-crystal.md`](../adr/2026-09-13-capture-engine-clock-is-hse-8mhz-crystal.md),
  which updates the firmware and doc clock constants (source, `PLLM` 16->4,
  `PLLN` 192->96) in one change so the header and the record "cannot drift apart."
  The pin-assignment ADR's timebase decision (TIM2, PSC 95) is unaffected: only the
  oscillator feeding the PLL moves, and every downstream frequency is identical.
- **The PCA9615 is a new catalog component.** The in-house-harness ADR
  ([`2026-08-09-two-channel-harness-built-in-house.md`](../adr/2026-08-09-two-channel-harness-built-in-house.md))
  scopes the harness to "the timebase, not the front end," with the INA226 as a
  catalog part. Adding a differential I²C buffer between the capture engine and
  each remote INA226 is within that scope (it extends the bus, not the front end),
  but it introduces a part the ADRs do not name and a **remote sense board** the
  file map does not list. Recorded in
  [`2026-09-13-remote-ina226-over-pca9615-differential-i2c.md`](../adr/2026-09-13-remote-ina226-over-pca9615-differential-i2c.md);
  the companion sense-board HLD it names is still owed.
- **D1 forecloses R1's cheap exit.** [`../dsc_hld.md`](../dsc_hld.md) R1/O2 hold
  the host transport open and name the 2 Mbaud UART as the cheap escape from a
  hand-written USB device stack under the bare-metal ADR. Committing the *board*
  to native USB removes that escape. Recorded in
  [`2026-09-13-host-transport-native-usb-hal-scoped-to-measurement-path.md`](../adr/2026-09-13-host-transport-native-usb-hal-scoped-to-measurement-path.md),
  which takes R1's remaining exit: it amends the bare-metal ADR to scope "no HAL" to
  the measurement path and admits an audited USB device stack for transport only.
  This partly discharges R1/O2; the `wrap`-safe record format (O2) stays open.

D1-D3 are **Accepted** (2026-09-13). Parts are selected and every value bound in
§14. The only outstanding SOP bookkeeping is a paired `type:decision` issue per ADR
(GitHub was not authorized in-session); the ADRs already carry their live
alternatives, so the anti-reconstruction intent is met and the issues can be
backfilled.

---

## 14. Selected parts and bound values (BOM)

Part selection for v1. **This section governs:** where §4, §6, §7 or §8 left an
exact value to part selection, the value here binds it. Actives cite the part
datasheet; passives are standard values sized by the formulas in the sections
named. Nothing here is `MEASURED`; measured shunt values still replace nominal from
the gain gate onward per
[`review_checklist.md`](../../harness/hardware/review_checklist.md).

### 14.1 Active and mechanical

| Function | Part | Key spec | Source |
|---|---|---|---|
| MCU | STM32F411CEU6 (LCSC C60420) | UFQFPN48 | [part ADR](../adr/2026-08-12-capture-engine-part-is-stm32f411ceu6.md) |
| 3.3 V LDO | AP2112K-3.3 (SOT-23-5) | 600 mA, dropout ~250 mV @600 mA, I_q ~55 µA, ceramic-stable | Diodes AP2112 |
| Diff I2C buffer ×2 | PCA9615DP (TSSOP10) | 2-ch differential I2C, dual supply, to 1 MHz | NXP PCA9615 Rev 2 |
| USB ESD | USBLC6-2SC6 (SOT-23-6) | low-cap TVS, DM/DP + VBUS | ST USBLC6-2 |
| HSE crystal | 8 MHz SMD 3225, C_L 18 pF, ±20 ppm | e.g. YXC X322508MSB4SI | [clock ADR](../adr/2026-09-13-capture-engine-clock-is-hse-8mhz-crystal.md) |
| VDDA ferrite | 600 Ω @ 100 MHz, 0603 (e.g. BLM18PG600SN1) | analog isolation | Murata BLM18 |
| USB-C | 16-pin receptacle (e.g. TYPE-C-31-M-12) | sink/UFP | vendor |
| JST-PH ×2 | 10-pin, 2.0 mm (B10B-PH-K class) | node cable | JST PH |
| SWD | 2×5, 1.27 mm Cortex debug header | SWD | Arm |
| LEDs ×3 | 0603; power green, status green + red | V_f ~2.1 V | vendor |
| Trigger | SMD tactile switch | campaign trigger | vendor |

### 14.2 Passive values (bound)

| Function (section) | Value | Qty |
|---|---|---:|
| MCU decoupling, 0402 X7R 16 V (§5) | 100 nF | 5 |
| VDD bulk (§5) | 4.7 µF 0805 X5R | 1 |
| VDDA filter (§5) | 1 µF + 10 nF | 1 + 1 |
| VCAP_1 (§5) | 2.2 µF 0603 X5R | 1 |
| LDO caps (§4) | C_in 1 µF, C_out 1 µF, VBUS bulk 10 µF | 3 |
| Crystal load (§6) | 27 pF C0G 0402 | 2 |
| I2C pull-ups (§8.1) | 2.2 kΩ | 4 |
| Type-C CC (Rd) (§7) | 5.1 kΩ | 2 |
| DM/DP series (§7) | 22 Ω | 2 |
| VBUS divider (§7) | 10 kΩ / 10 kΩ | 2 |
| LED series (§8.2) | 1 kΩ (~1.2 mA) | 3 |
| Trigger pull-up (§8.3) | 10 kΩ | 1 |
| SWD/BOOT0 pulls (§8.4) | 10 kΩ (SWDIO up, SWCLK down, BOOT0 down) | 3 |
| CNVR pull-up (§10) | 10 kΩ | 2 |
| Cable series protection (§10) | 100 Ω | 8 |
| PCA9615 decoupling, both rails (§9) | 100 nF | 4 |
| PCA9615 diff termination (§9) | 100 Ω across each pair, both cable ends | 4 |
| PCA9615 idle-bias (§9) | bias pair to V_DD(B)/V_SS, PCA9615 Fig 5 | 4 |

The differential termination is 100 Ω at each cable end (PCA9615 §7.2, read
directly); the idle-bias resistor value tracks the chosen V_DD(B) and cable length
and is set on the impedance-controlled layout, which is where every
transmission-line value is fixed. All other datasheet-derived values (LDO caps,
crystal load, ferrite, ESD) are bound to the parts above and confirmed on hardware
at Tier 3 bring-up, not here.

---

## 15. Open items that should close before fabrication

Every count in §4 through §11 is `BORROWED` or `DERIVED`, and the Status line is
blunt that **no cell is `MEASURED`**. §12 turned datasheet readings into figures
bound *on paper*. This section is the step that binds them *on silicon*, and it
deliberately does not wait on the fabricated board. Ordered, like
[`../dsc_hld.md`](../dsc_hld.md) §7, by the cost of finding a failure out late.

### 15.1 The proving rig is a spike

Per [`../sop/git_sop.md`](../sop/git_sop.md), a `spike/` branch is throwaway by
declaration: never merged, its survivors rewritten onto a typed branch, so
exploring the architecture never enters the record as though the dev-board
hardware were the design. This rig is exactly that. **No ADR is written for it and
no Accepted decision in §13 is reversed by it.** The clock, USB, and PCA9615
choices remain the plan for the fabricated board; the spike is a precondition
placed *in front of* the fab trigger, not a competitor to it.

Two off-the-shelf parts, chosen for availability rather than for the design:

| Rig part | Stands in for | What transfers, and what does not |
|---|---|---|
| WeAct STM32F411CEU6 "Black Pill" | the MCU (§5, §14.1) | Same silicon as the [part ADR](../adr/2026-08-12-capture-engine-part-is-stm32f411ceu6.md). Its HSE is **25 MHz** (`BORROWED`, WeAct board; confirmed the instant the PLL locks), not the 8 MHz of the [clock ADR](../adr/2026-09-13-capture-engine-clock-is-hse-8mhz-crystal.md). `PLLM 25 -> 1 MHz`, then `PLLN 192 -> 192 MHz VCO -> /2 = 96 MHz SYSCLK -> /4 = 48 MHz USB exactly`: the whole downstream tree of §3 transfers, only `PLLM` differs. The ±20 ppm *stability* of the chosen crystal does not transfer and is not claimed here. |
| Aftermarket INA226 breakout | the remote INA226 (§9, §10) | An INA226 on 2.54 mm headers is the only way onto a breadboard: the design's bare VSSOP-10 cannot be, which is the whole reason O4 ([`../dsc_hld.md`](../dsc_hld.md) §7) owes a hand-assembled carrier. Whatever shunt the module carries **never enters a graded metrology gate**. O4 stands unchanged. |

### 15.2 What the spike closes, and what it must not be read as closing

**Closes** (architecture and plumbing, none of it dependent on the shunt value or
the crystal part):

- **O1, does the CNVR alert self-clear in transparent mode.**
  [`../dsc_hld.md`](../dsc_hld.md) §7 names this the cheapest thing to test and the
  most expensive to discover late, because the pessimistic reading reopens the bus
  count and the board is laid out around the bus count. The breakout answers it
  directly; whether the flag self-clears has nothing to do with the shunt.
- **O2 / R1, the hand-written USB CDC stack holds 114 kB/s on F411 silicon.**
  [`../dsc_hld.md`](../dsc_hld.md) §4.3 prices this transport as "substantially
  more firmware than everything currently in `firmware/capture/` combined," and
  unbuilt. Learning after fab that it does not hold rate is the exact late cost
  this ordering exists to avoid.
- `rate`, `dropped`, `wrap`, `jitter`: configuration-took, pointer retention at
  400 kHz, monotonic timestamps across the 71.6-minute wrap, and the
  timestamp-at-edge / read-at-leisure split ([`../dsc_hld.md`](../dsc_hld.md)
  §6.2). The 1 µs tick and the scheduling jitter it must survive are independent
  of whether the PLL source is 8 or 25 MHz.

**Must not be read as closing:**

- `gain` and `negctl`: both need the 0.1 Ω / 0.1 % shunt on a Kelvin carrier. The
  breakout fails `gain` arithmetically (O4), so **no current-accuracy figure is
  drawn from this rig, ever.**
- The specific 8 MHz ±20 ppm crystal and its load network (§6, §14.1): proven only
  on the fabricated board.
- The **PCA9615 remote-differential link** (§9, §10). The breakout sits
  single-ended on the breadboard beside the MCU; the cable, the buffer, and the
  termination belong to the companion sense board (§11) and are proven when it
  gets its HLD. The spike proves the capture-engine side, not the remote link.

### 15.3 The open items, ranked

| # | Open item | Closed by | Cost if found late |
|---|---|---|---|
| A | **O1** CNVR self-clear in transparent mode | spike, §15.2 | reopens the bus count, and the layout is drawn around it |
| B | **O2 / R1** USB CDC transport unbuilt and unpriced | spike demonstrates on silicon; the [USB-transport ADR](../adr/2026-09-13-host-transport-native-usb-hal-scoped-to-measurement-path.md) already scopes a stack to transport | blocks `stream`, and `stream` blocks every Tier-1 gate that reads a capture |
| C | CNVR external-vs-internal pull-up (§10 vs [`harness_spec.md`](./harness_spec.md) §3) | review walk; the spike can measure the internal pull-up's margin over a representative lead | a weak pull-up over a cable is noise-prone, and the divergence is not yet in the pin map |
| D | Companion **remote sense board** HLD still owed (§11, §13) | its own HLD | it carries the PCA9615 link, the INA226 carrier, and the remote pull-ups; blocks the system, not this board's fab |
| E | Layout-bound transmission-line values (§9 termination / idle-bias, §14.2) | the impedance-controlled layout | neither paper nor spike fixes them; they bind where every transmission-line value binds |

**The decision this defers.** Items A and B are what the fab trigger now waits on,
and both close on a breadboard rig rather than on a fabricated board. Until they
close, §11's BOM and §14's part list are the *intended* design held one step back
from fab, not released to it. This is the "prove something to close a gate" step
in full: it does not prove the board, it removes the two holes whose late
discovery would invalidate the layout.
