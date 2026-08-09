# Harness v1: Signal Inventory and Timing Budget

*Stage -1 arithmetic for the two-channel metering harness. Every figure here is derived from declared constants: the INA226 datasheet, the I2C specification, and the phase structure in section 4.7 of the thinkbook. Nothing here is measured. The purpose is to fix the capture engine's requirements before any part is ordered.*

---

## 1. Signal inventory

Two metered nodes. Each contributes one conversion-ready line from its current sensor and a phase code from the device under test.

| Signal | Width | Source | Rate |
|---|---|---|---|
| CNVR, node S | 1 | INA226 Alert pin, Conversion Ready mode | one assert per conversion |
| Phase code, node S | 3 | DUT GPIO | one change per phase transition |
| CNVR, node R | 1 | INA226 Alert pin, Conversion Ready mode | one assert per conversion |
| Phase code, node R | 3 | DUT GPIO | one change per phase transition |
| Campaign trigger | 1 | host or either DUT | once per campaign |
| Spare | 1 | reserved | none |

**Ten digital inputs.** No analog inputs: the sensors are digital over I2C.

**Why a 3-bit phase code rather than one toggle line.** Node S passes through sleep, wake, encode, transmit, sleep. Node R passes through sleep, wake, receive, decode or process, sleep. Four states each, plus room for an armed state and an error state. A single toggle line encodes transitions but not identity, so one missed edge desynchronises every phase after it for the rest of the run, silently. A 3-bit code is self describing: any sample of the bus states which phase the node is in, and a missed transition costs one boundary rather than a run.

Three bits also stays inside the callback discipline. Writing three bits is one masked register write on the DUT, which is the same cost as toggling one, and the send callback runs from a high priority Wi-Fi task where nothing longer is permitted.

**Gray code the phase sequence.** The capture engine samples the phase bus asynchronously. If two bits change on one transition, a sample landing inside the transition can latch a code that was never intended. Ordering the four states so that exactly one bit changes per transition removes that failure by construction: a mid transition sample reads either the old code or the new one, never a third. This costs nothing and it removes the need for a separate strobe line.

---

## 2. Edge rate

At the configuration fixed in thinkbook 4.7, shunt only continuous mode with averaging set to one, conversion time is 140 microseconds.

| Quantity | Value |
|---|---|
| Conversions per second, per channel | 7,143 |
| CNVR edges per conversion, worst case | 2 (assert, then deassert on flag clear) |
| CNVR edge rate, per channel | 14,286 per second |
| CNVR edge rate, both channels | 28,572 per second |
| Phase edges per event, both nodes | 8 or fewer |
| Event rate during a campaign, upper estimate | 50 per second |
| Phase edge rate | 400 per second or fewer |
| **Total edge rate** | **under 30,000 per second** |

The edge rate is dominated entirely by conversion ready. Phase markers are three orders of magnitude below it and never constrain the design.

Setting the alert latch to transparent mode removes the need to clear the flag by reading the Mask/Enable register, which halves the edge rate to roughly 14,300 per second and removes one I2C transaction per conversion. Adopt transparent mode.

---

## 3. Timestamp resolution

The requirement comes from thinkbook 4.7: phase boundary uncertainty must stay below one conversion time, because that is the intrinsic blur the delta sigma window already imposes and nothing is gained by adding more.

| Quantity | Value |
|---|---|
| Intrinsic blur from the conversion window | 140 microseconds |
| Target contribution from the capture engine | under 1 percent of that |
| Required timestamp resolution | 1.4 microseconds or finer |
| Resolution at a 1 MHz counter | 1 microsecond |
| Resulting contribution against a 1 millisecond transmit phase | 0.1 percent |

**A 1 MHz counter is roughly a hundred times better than the requirement.** This is the conclusion that decides the capture engine's class: no programmable IO fabric, no FPGA, no exotic timing hardware. A standard timer with input capture, or an interrupt per edge on any modern microcontroller, clears the bar with two orders of magnitude of margin.

A 32 bit microsecond counter wraps at roughly 71 minutes. A Stage 2 campaign runs longer than that, so either the counter is extended to 64 bits in software on overflow, or each record carries a wrap counter. Handle it in the capture firmware and never in the host, because a wrap that reaches the host unhandled produces timestamps that look plausible and are wrong.

---

## 4. I2C budget, and a correction to thinkbook 4.7

One 16 bit register read per conversion per channel.

The datasheet states that the device retains the register pointer until a write changes it, so repeated reads of the same register do not resend the pointer byte. Exploiting that changes the answer.

| Transaction | Clock periods | Time at 400 kHz | Share of a 140 microsecond conversion |
|---|---|---|---|
| Read with pointer rewritten each time | about 48 | 120 microseconds | 86 percent |
| **Read with pointer retained** | **about 29** | **73 microseconds** | **52 percent** |
| Read with pointer retained, high speed mode at 2.94 MHz | about 29 | 10 microseconds | 7 percent |

Two channels sharing one bus at 400 kHz with the pointer retained is 104 percent utilisation, which fails. Two independent buses at 400 kHz is 52 percent each, which works with margin for the occasional configuration write.

**Correction.** Thinkbook 4.7 states that high speed I2C is required and that fast mode drops conversions. That is true for a naive read that rewrites the pointer every time. With pointer retention and one bus per channel, 400 kHz fast mode is sufficient. This matters for part selection: two fast mode I2C masters are universal on mid range microcontrollers, whereas high speed mode support is uneven in both peripherals and drivers. High speed mode is retained as the fallback if measured margin proves worse than this arithmetic.

**Read only the Shunt Voltage register.** The rail voltage is treated as a constant per thinkbook 4.7, so bus voltage is not converted and the Current and Power registers are not used, which also removes the need to program the Calibration register at all. Shunt voltage times the shunt value gives current on the host, where it is auditable.

---

## 5. Host throughput and volume

| Quantity | Value |
|---|---|
| Sample record: shunt reading plus timestamp plus channel tag | 8 bytes |
| Sample rate, both channels | 14,286 per second |
| **Sustained stream** | **114 kilobytes per second** |
| Equivalent line rate at 8N1 | 1.14 megabits per second |
| Minimum UART setting | 2 megabaud |
| USB CDC headroom | ample |
| Twenty minutes of continuous capture | about 137 megabytes |

USB CDC is the straightforward choice and removes the baud rate question. If capture is gated to the event window by the phase code rather than free running, volume falls by whatever the duty cycle is, and sleep intervals stop consuming bandwidth to record a current that is below one least significant bit anyway.

---

## 6. Shunt sizing and sensing topology

Device under test fixed as ESP32-S3. The binding figures come from the module datasheet, taken at a 3.3 V supply and 25 degrees ambient: 330 mA transmitting 802.11b at 1 Mbps and 20.5 dBm, and 97 mA receiving. ESP-NOW's default bit rate is 1 Mbps DSSS, so the 330 mA row is the operating case rather than a worst case that never occurs. Deep-sleep current for this module is 8.14 microamps, from Espressif's own measurement guide.

The INA226 spans 81.92 millivolts of shunt voltage at a 2.5 microvolt least significant bit, which is 32,768 counts, fixed, with no ranging.

| Shunt | Full scale | Least significant bit | Drop at 330 mA | Headroom over 330 mA |
|---|---|---|---|---|
| 0.05 ohm | 1638 mA | 50 microamps | 16.5 mV | 5.0x |
| **0.1 ohm** | **819 mA** | **25 microamps** | **33 mV** | **2.5x** |
| 0.15 ohm | 546 mA | 16.7 microamps | 49.5 mV | 1.7x |
| 0.2 ohm | 410 mA | 12.5 microamps | 66 mV | 1.2x |

**Selected: 0.1 ohm.** Two and a half times headroom over the transmit figure, which absorbs envelope peaks and radio startup inrush that a typical figure does not describe. Voltage drop is 33 millivolts, one percent of the rail, against a module specified from 3.0 to 3.6 volts. Dissipation is 10.9 milliwatts, trivial in an 0805 or 1206 part. The value is round and universally stocked, which matters when ten boards are fabricated at once.

What that resolution buys, in counts:

| Current | Counts at 25 microamps per bit | Verdict |
|---|---|---|
| Transmit, 330 mA | 13,200 | fully resolved |
| Receive, 97 mA | 3,880 | fully resolved |
| Modem sleep, tens of milliamps | around 1,000 | fully resolved |
| Deep sleep, 8.14 microamps | 0.33 | **below one bit, not measured** |

The blind spot is now a number rather than an expectation, and it is worth pricing once. Deep sleep at 8.14 microamps on 3.3 volts is 26.9 microwatts, so a second of sleep costs about 27 microjoules. One transmission at 330 mA for a millisecond costs about 1.1 millijoules. Sleep is therefore a small fraction of a single event's energy per second of sleep, it is identical in both conditions, and it cancels in every comparison the thinkbook makes. It would matter only for an absolute battery-life claim, which is not made.

**Both channels carry the same shunt.** Node R is not a receive-only node: the sufficiency constraint requires R to acknowledge delivery at the application layer, so R transmits and sees the same 330 mA peak. Identical channels also keep the ten fabricated boards interchangeable, which is the point of fabricating ten.

**Tolerance matters less than matching, and matching is solved by measurement.** A systematic gain error cancels in the comparison of one condition against another on the same channel, and in the recovered-fraction ratio. It does not cancel between the two channels, and the transmit-side against receive-side asymmetry is itself a stated prediction, so a mismatch between the two shunts lands directly on a result. Specify 0.1 percent, then measure both shunts against one reference at Stage 0 and carry the measured values rather than the nominal ones. After that only stability matters, and at 10.9 milliwatts in a low-drift part the thermal contribution is far below the INA226's own gain error. This corrects an earlier assumption that the shunt would dominate the bill of materials; at this dissipation it does not.

**High-side sensing, not low-side.** The INA226 accepts common-mode input from 0 to 36 volts independently of its own supply, so sensing above the 3.3 volt rail is unconstrained. The reason to insist on it is topological: low-side sensing puts a shunt in each node's ground return, which offsets two device grounds against each other and against the capture engine. On a two-node harness that is a designed-in ground offset. High-side sensing leaves one continuous ground across both nodes and the capture engine, which is what a shared timebase needs anyway.

**Kelvin connection is a layout requirement, not a refinement.** At 0.1 ohm, one milliohm of trace and solder resistance inside the sense path is a one percent error, comparable to the entire error budget. The sense traces meet the shunt at its own pads and carry no load current.

---

## 7. Resulting requirements on the capture engine

| Requirement | Value | Slack against typical mid range parts |
|---|---|---|
| Independent I2C masters at 400 kHz | 2 | none, this is the binding requirement |
| Digital inputs with edge timestamping | 10 | comfortable |
| Timestamp resolution | 1 microsecond | two orders of magnitude |
| Edge handling rate | 30,000 per second | three orders of magnitude |
| Sustained host stream | 114 kilobytes per second | comfortable over USB |

**The binding requirement is two independent I2C masters.** Everything else clears by orders of magnitude. Any part with two I2C peripherals, a microsecond timer, ten free GPIO with interrupt capability, and USB device support satisfies this, which includes both the RP2040 and RP2350 families and the STM32F4 and G4 classes.

The STM32 branch is worth noting against the hardware ladder already declared in the playbook: STM32 is the branch for radio excluded low power control, and a capture engine is radio excluded by definition. Selecting it here exercises a branch the ladder already contains rather than opening a new one.

---

## 8. Two design rules that fall out of the arithmetic

**Timestamp at the edge, read at leisure.** The I2C read takes 73 microseconds and its start is subject to scheduling jitter. If the timestamp were taken when the read completes, that latency and its jitter would land directly in the timebase. The conversion ready interrupt must capture the counter and queue the read; the read then happens whenever the bus is free. This decouples the timebase from the bus entirely and is the single most important structural decision in the capture firmware.

**Do not synchronise the two sensors.** The two INA226 parts free run and their conversions will not align. That is fine and should not be fixed. What the two sided ledger requires is a common *time*, not a common *sample instant*, and every record carries its own timestamp from one counter. Attempting to align conversions would add a synchronisation mechanism that buys nothing and can fail silently.

---

## 9. What this changes upstream

Thinkbook 4.7 loses the high speed I2C requirement and gains pointer retention and transparent alert latch mode. The sensor configuration bullets are otherwise unchanged.

Thinkbook Stage -1 gains this document as an input to its deliverable table: the capture engine class and the I2C bus count are now declared constants rather than open questions.

---

## 10. Open, and closed in Stage 0 Phase A

These are implementation choices, not unanswered questions upstream. They are closed as the first phase of Stage 0, before any bench work begins, and nothing after Phase A starts until they are written down.

- Whether capture is free running or gated by the phase code. Gated is cheaper on volume and slightly more complex in firmware; either satisfies the budget above.
- Whether the phase code is driven by the DUT or derived on the harness from a single strobe plus a serial phase identifier. The 3 bit parallel code assumed here costs three DUT pins; a serialised alternative costs one pin and adds latency that would have to be characterised. Parallel is assumed until pin pressure on the DUT says otherwise.
