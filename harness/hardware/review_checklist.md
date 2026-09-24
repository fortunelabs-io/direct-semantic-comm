# PCB v1 review checklist

*The authoritative board-review gate, walked by a person **before Gerbers are exported**. Fabrication is a bench action with no pass criterion of its own; the Tier 3 gates are what prove it was done correctly. This checklist exists because the errors that matter most here bias every measurement silently rather than failing visibly, and no gate after fabrication can distinguish them from a real result.*

**Status:** The KiCad project was scaffolded on 2026-09-22 and the board is under design; this checklist walk has not begun and remains the gate before Gerbers are exported. Fabrication is the last thing started, after every question provable at the desk has been answered.

---

## How to walk this, and what it proves

`mise run hw` runs first and must be clean: ERC proves the pins connect, and DRC proves the layout matches the schematic it was generated from. Neither proves the schematic is *right*. That is this checklist. It has three jobs the automated checks cannot do:

1. Catch the two silent-bias topology errors under **Not negotiable**, which produce a clean ERC and a clean DRC while biasing every reading.
2. Confirm the schematic **realizes the HLD**, block by block, so a part the design calls for is not absent.
3. Confirm the **project-state preconditions** under *the fab trigger* are met, so ten boards are not committed to fabrication ahead of the paper and silicon evidence they rest on.

**Source-of-record rule.** [`hardware_hld.md`](../../docs/hardware-harness-v1/hardware_hld.md) owns the board and the BOM, and its §14 binds every part value; [`harness_spec.md`](../../docs/hardware-harness-v1/harness_spec.md) owns the pins. Values and pin assignments are **cited here**, so a number never lives in two places. Where this file and a cited source disagree, the source wins and this file is stale. Open the cited section alongside the schematic.

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
- [ ] Two independent I2C buses, one per channel, brought out separately. 52 percent utilization each at 400 kHz; one shared bus is 104 percent and fails outright.
- [ ] Address strapping accessible.

## The schematic realizes the HLD, block by block

*For each block, confirm the schematic carries the parts [`hardware_hld.md`](../../docs/hardware-harness-v1/hardware_hld.md) names, at the values its §14.2 binds. The checks below are the structure and the traps; the values are §14's.*

- [ ] **Power (§4, §14).** One 3.3 V LDO with its input and output caps and one VBUS bulk cap. The rail feeds the board and both remote sense sides over the JST-PH; it does **not** power the DUT, which powers itself.
- [ ] **MCU decoupling (§5, §14).** A close-in 100 nF at each supply pin the part bonds on UFQFPN48: VDD (24, 36, 48), VBAT (1), NRST (7), plus the VDD bulk reservoir. Pin 48 is the third VDD and is the one an earlier revision of §5 missed. VSS (23, 35, 47), the exposed pad, and VSSA (8) all reach GND. VCAP_1 (pin 22) carries exactly one low-ESR ceramic and VCAP_2 is absent, so a second core cap must not be added. VDDA/VREF+ (pin 9) is filtered from VDD through a ferrite, not regulated: no on-chip ADC is used.
- [ ] **Crystal (§6, §14).** One 8 MHz HSE crystal with its two matched load caps, and an OSC_OUT series-resistor footprint fitted but left DNP unless the drive-level check requires it. No 32.768 kHz LSE crystal: the timebase is TIM2, not the RTC.
- [ ] **USB-C front end (§7, §14).** Sink/UFP. **Both** CC pins carry an Rd pull-down, not one, because cable orientation is unknown at the receptacle. DM and DP each carry a series resistor, and the ESD array sits across DM, DP, and VBUS. A VBUS sense divider reaches a GPIO.
- [ ] **I2C pull-ups, indicators, trigger, SWD, boot (§8, §14).** Four local I2C pull-ups only; the remote-side pull-ups live on the sense board. Three indicator LEDs, each with a series resistor. A trigger button with its pull-up. An SWD header, with SWDIO pulled up, SWCLK pulled down, and BOOT0 pulled down so the part boots from flash.
- [ ] **PCA9615 differential buffers (§9, §14).** Two buffers, each decoupled on **both** of its supplies, V_DD(A) and V_DD(B): a missing decoupler on the line-side rail is the easy error. EN floats high to enable. The differential termination and idle-bias network is present per NXP §7.2 / Figure 5; its exact values are impedance-controlled and bind at layout, not at this review.
- [ ] **Cable interface, JST-PH (§10, §14).** Each connector carries the buffered differential I2C pair, the CNVR line, three phase bits, 3.3 V, and GND: ten pins minimum, with series protection on each single-ended cable-facing line.
- [ ] **CNVR pull-up, a divergence to resolve (§10, §12 item 7, `harness_spec.md` §3).** [`hardware_hld.md`](../../docs/hardware-harness-v1/hardware_hld.md) §10 proposes an external 10 kΩ per CNVR line at the board end; [`harness_spec.md`](../../docs/hardware-harness-v1/harness_spec.md) §3 specifies the STM32 internal pull-up. An internal pull-up over a cable is weak and noise-prone. Resolve which governs at this walk. Record the outcome. It is not yet reconciled in the pin map.
- [ ] **What is not on this board (§11).** No INA226, no 0.1 ohm shunt, and no PCA9615-to-INA226 pull-ups on the capture engine. Those belong to the companion remote sense board. This board senses each rail across the cable; it does not carry the front end.

## Pin map

- [ ] **Walk the pin-map checks in [`harness_spec.md`](../../docs/hardware-harness-v1/harness_spec.md) §6.** Every symbolic name has a matching schematic net; the ten harness inputs sit on ten distinct pin *numbers*; the two CNVR lines are two of EXTI0 through EXTI4; the phase-code allocation is made and verified; the SWD pins are not reused. Those live there and are not restated here.
- [ ] **Each node's three phase bits are contiguous within one GPIO port** (`harness_spec.md` §4), so the handler latches a code with one IDR read, one shift, and one mask. Under the closed six-state cycle all three bits toggle every event, so `b2` is a timing-relevant edge source, not a static level, and must be driven at all times ([`phase_code_map.md`](../../docs/hardware-harness-v1/phase_code_map.md), "What b2 means").

## Out of scope for v1

- [ ] Confirm **no autoranging front end** has been added. It would close the deep-sleep blind spot and it is gated on Stage 2 data showing the range actually binds. Scope here is the timebase, not the front end, and every ranging feature arriving before then is the harness ADR being violated.

---

## Before Gerbers: the fab trigger

*This walk is one of the conditions the board waits on, not the only one. [`hardware_hld.md`](../../docs/hardware-harness-v1/hardware_hld.md) §12 and §15 hold the others, and the board is not released to fabrication until all are met. Listed here as the gate; §12 and §15 are the source of record for their status.*

- [ ] **Primary-source checks cleared (§12).** The BORROWED datasheet figures that size the board are confirmed against the primary source: STM32 power pins, PCA9615, LDO caps, crystal, the USB Type-C Rd and DM/DP series values, and the CNVR pull-up divergence (item 7, resolved above).
- [ ] **The three board ADRs are Accepted (§13).** HSE 8 MHz clock, remote INA226 over PCA9615 differential I2C, and native USB transport scoped to the measurement path.
- [ ] **The pin-assignment ADR is Accepted.** Its primary-source datasheet check has cleared for every borrowed pin (`harness_spec.md` §6).
- [ ] **The §15 spike has closed its gates on real F411 silicon.** O1 (does the CNVR alert self-clear in transparent mode), O2 / R1 (the hand-written USB CDC stack holds 114 kB/s), and `rate`, `dropped`, `wrap`, `jitter`. O1 is the first to close, because the layout is drawn around the bus count and the pessimistic reading of O1 reopens it. The spike does **not** close `gain`, `negctl`, the specific crystal, or the PCA9615 remote link; those are proven elsewhere and are outside this gate's scope.

---

## Incoming inspection

*After fabrication, before power.*

Continuity and shorts on **every** board before any board is powered. `mise run incoming` writes `results/stage0_incoming.json` with per-board pass or fail and assigned serial numbers.

Measured shunt value per board is recorded at the gain gate, not here, and replaces nominal everywhere from that point on.
