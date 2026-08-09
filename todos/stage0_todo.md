# Stage 0 TODO: build the harness and prove it is honest

*Ordered by what each item can invalidate, cheapest first. Every item states the question it answers, where it runs and with what, the output that counts as passing, and the artifact it leaves behind. An item without an exit criterion is a wish, and an item without an artifact did not happen.*

Stage 0's job is unchanged from the thinkbook: prove the meter measures the thing it is pointed at. What changed is that the meter is built here, so Stage 0 also proves the meter.

**Artifact convention.** Every number that later appears in a claim is read from a named file under `results/`, following the parent build. Nothing is recalled, nothing is retyped. Stage 0 closes with a `FINDINGS.md` in which every figure names the file it came from.

**Serial execution, no exceptions.** Phases run one at a time, in order, and nothing starts before the phase above it has produced its artifact. The same rule applies at the boundary above: this document does not start while anything upstream of it is open, which is what the precondition section enforces. The reason is attribution, not discipline for its own sake: if two things are in flight and something fails at their interface, there is no way to say which one is wrong. Developing capture firmware and device firmware at the same time means a decode failure could be either, and the cost of finding out exceeds the time saved by overlapping them.

Fabrication is deliberately the last thing started, after every question provable at the desk has been answered. The wait for boards is then genuinely idle by construction, rather than a window that invites a second track to be opened inside it.

This rule binds within this domain. Work in another domain with its own queue, incorporation and assignment being the obvious one, is not parallelisation of this work and is unaffected.

---

## Precondition: Stage -1 closed, with no open items

**Nothing in this document starts while any Stage -1 question remains open.** Not Phase A, not a part order, not a schematic.

The reason is the ordering principle the thinkbook rests on: cheapest falsification first. Stage -1 costs an afternoon and can kill the direction. The harness build costs weeks and a fabrication run and is the first real expenditure this project makes. Beginning the expensive thing while a free gate is still open inverts that order, and the fact that the harness could be built without the answer does not restore it. A technical dependency and an economic one are different things, and the second is what governs here.

| Stage -1 question | Status |
|---|---|
| 1. Is the packet-count term non-zero for the chosen sensor modality, at the candidate bottleneck width and quantisation? | **Open. This is the only item standing between here and Phase A.** |
| 2. Do both nodes run the same ESP-NOW version, and is the truncation boundary understood? | Closed by declaration: IDF version pinned and the three length constants read from the header. Verified on hardware at C3, which checks a declaration rather than answering an open question. |
| 3. Do Encoder_S and Decoder_R agree on quantisation scheme, scale granularity and tensor layout? | Closed for H_ledger by construction, since Condition B trains both halves as one design. Reopened as a fresh Stage -1 pass before Stage 5, where the halves are independent. |
| 4. What is the width ratio between the transmitted latent and what the decoder expects? | Same as 3. |
| 5. What shunt value keeps the transmit peak on scale, and where does that place the resolution floor? | Closed. 0.1 ohm, both channels, high side. Derivation in the harness signal inventory and timing budget. |
| Harness constants: capture engine class, I2C bus count, signal inventory, timing budget | Closed. Same document. |

If question 1 resolves against the design, the answer is to change modality, change the frame limit by changing ESP-NOW version, or rescope to a byte-count claim. All three are cheaper decided now than discovered after ten boards exist.

---

## Toolchain

| Where | What |
|---|---|
| Workstation | VSCode, Git, Python 3 with numpy, pandas, matplotlib, scipy, pyserial |
| DUT firmware | ESP-IDF v5.x via the Espressif IDF extension or PlatformIO. ESP32-S3 has native USB-serial-JTAG, no external programmer |
| Capture firmware | Depends on A1. RP2040: pico-sdk, CMake, CMake Tools. STM32: STM32CubeMX plus CMake. PlatformIO covers either |
| PCB | KiCad, JLCPCB for fabrication and optional assembly, LCSC for parts |
| Bench | DMM with DCV accuracy 0.1 percent or better; 0.1 percent metal film resistors; regulated linear bench supply; low-cost USB logic analyser with PulseView or sigrok |
| Not required | Oscilloscope. Supply excursion is covered by periodic bus-voltage conversion on the INA226 at 1.25 mV per bit, 0.04 percent of the rail |

The DMM and the resistors are the traceability standards for E3a and are the only real instrument purchase. The logic analyser exists only for Phase B, because the capture engine cannot be debugged with itself.

---

## Phase A: decisions before anything is ordered

Runs on paper. Cost: hours. Can invalidate: the board.

### A1. Capture engine part

**Question.** Which part satisfies two independent I2C masters at 400 kHz, ten GPIO with edge interrupt, a microsecond timer, and USB device?
**Runs on / with.** Datasheet reading; toolchain installed and a blink built to prove the chain works end to end.
**Expected output.** One part named. Two I2C peripherals confirmed on separate pins in the datasheet, not inferred from a product page.
**Artifact.** `decisions/` entry or a line in the harness spec, plus a repository that builds and flashes.

### A2. Free-running or gated capture

**Question.** Does the capture engine stream continuously, or only inside a window opened by the phase code?
**Runs on / with.** Paper, against the volume figure of 114 kilobytes per second.
**Expected output.** One chosen. Recommendation is free-running for v1: a gate that fires wrongly loses data silently, and a full stream can always be trimmed on the host.
**Artifact.** Written into the harness spec before firmware starts.

### A3. Phase code parallel or serial

**Question.** Three DUT pins and no latency, or one pin with latency that must be characterised?
**Runs on / with.** Paper, against available DUT pins.
**Expected output.** Parallel confirmed unless pin pressure says otherwise, plus the four-state Gray sequence written out explicitly for both roles.
**Artifact.** A table of code to phase for `tx_role` and `rx_role`, committed alongside `phase_marker`.

### A4. Bench sensor source

**Question.** Breakout or discrete INA226 for Phase B?
**Runs on / with.** Paper plus supplier listings.
**Expected output.** If a breakout is used, its shunt value is confirmed from its own schematic, and it is accepted that PCB v1 will not reuse it.
**Artifact.** Part numbers recorded.

---

## Phase B: bench prototype, before any PCB

Runs on breakouts and jumper wires at the desk. Cost: days, no fabrication. Can invalidate: the timing budget and the firmware architecture.

Signal integrity will be worse here than on the PCB and that does not matter. What is under test is whether the arithmetic survives real interrupt latency and real driver overhead. Four of the five findings that can kill the design are caught in this phase, before any money is spent on fabrication.

### B1. Sensor configuration reaches its rated rate

**Question.** Does one INA226, configured as the thinkbook specifies, actually deliver conversions at the datasheet rate?
**Runs on / with.** Capture engine plus one sensor breakout, one I2C bus at 400 kHz. Configuration: shunt-only continuous, conversion time 140 microseconds, averaging one, alert latch transparent, register pointer retained, shunt voltage register only.
**Expected output.** 7,143 conversion-ready edges per second, within one percent, sustained for ten minutes.
**Artifact.** `results/stage0_b1_rate.json`: configured registers read back, edge count, elapsed time, derived rate.

### B2. The timestamp is taken at the edge

**Question.** Is the timebase decoupled from the I2C bus, or is bus latency leaking into it?
**Runs on / with.** Same rig. The conversion-ready interrupt captures the counter and queues the read; the read must not sit inside the interrupt.
**Expected output.** Standard deviation of the interval between consecutive timestamps under 2 microseconds. A read accidentally left in the interrupt path shows up immediately as jitter near 73 microseconds, which is the read duration.
**Artifact.** `results/stage0_b2_jitter.json`: the interval histogram and its standard deviation.

### B3. Two channels, two buses, nothing dropped

**Question.** Does the 52 percent per-bus budget hold in practice on both channels at once?
**Runs on / with.** Capture engine, two sensors, two independent I2C buses, both free-running and unsynchronised.
**Expected output.** Zero dropped conversions over ten minutes at full rate on both channels. Zero, not few. A drop is a bug, not a tolerance. Measured by counting conversion-ready edges against records emitted.
**Artifact.** `results/stage0_b3_dropped.json`: edges seen, records emitted, difference, per channel.

### B4. The host keeps up

**Question.** Does 114 kilobytes per second sustain to disk without gaps?
**Runs on / with.** `meter_logger` in Python with pyserial over USB CDC, writing CSV.
**Expected output.** Ten minutes captured with no gaps, no host-side overflow, and a record count matching B3.
**Artifact.** `results/stage0_b4_stream.json` plus the raw capture retained.

### B5. Counter wrap is handled in firmware

**Question.** Does a run crossing the 32-bit microsecond wrap, at about 71 minutes, still produce monotonic timestamps on the host?
**Runs on / with.** Same rig, with the counter seeded near overflow rather than waiting an hour.
**Expected output.** Monotonic timestamps across the wrap. The wrap is handled in the capture firmware and never on the host, because a wrap reaching the host unhandled produces timestamps that look plausible and are wrong.
**Artifact.** `results/stage0_b5_wrap.json`.

**Phase B gate.** If B1 through B5 pass, the timing budget is confirmed and the PCB has only signal integrity and mechanical repeatability left to solve. If B3 fails, the fallback is high-speed I2C or a different capture part, and discovering that here costs days rather than a fabrication run.

---

## Phase C: DUT firmware, minimum for Stage 0

Runs on two ESP32-S3 boards, built in VSCode with ESP-IDF. Starts after Phase B has produced its artifacts, because C1's exit criterion requires a working capture engine to decode against.

### C1. `phase_marker`

**Question.** Can the capture engine decode a complete phase sequence with no invalid codes?
**Runs on / with.** ESP32-S3, ESP-IDF. One masked register write per transition, Gray-coded, nothing else in the function. The send callback runs from a high-priority Wi-Fi task where nothing longer is permitted.
**Expected output.** The full four-state sequence decoded for both roles across ten thousand transitions, zero invalid codes. Gray coding means a sample landing mid-transition reads the old code or the new one, never a third.
**Artifact.** `results/stage0_c1_phase.json`: transition count, invalid code count, dwell time per state.

### C2. Commanded load

**Question.** Can the DUT produce a load of known duration and known duty cycle, driven by its own clock?
**Runs on / with.** ESP32-S3, ESP-IDF. A busy loop of commanded microseconds, and a commanded-duty square-wave load. These are the negative control for E2a and the bandwidth reference for E3c.
**Expected output.** Duration and duty commanded over serial and echoed back, so the host records what was asked for alongside what was measured.
**Artifact.** Firmware image plus its serial command protocol, documented.

### C3. Link works, and the ESP-NOW version is pinned

**Question.** Do frames deliver, and do both nodes report the same ESP-NOW version?
**Runs on / with.** Two ESP32-S3 boards, the unmodified `wifi/espnow` example from ESP-IDF.
**Expected output.** Frames delivered, send callback firing, both roles on the same IDF version, and `esp_now_get_version()` returning the same value on both. This also closes the second Stage -1 question, the silent truncation trap between a v2.0 sender and a v1.0 receiver.
**Artifact.** `results/stage0_c3_link.json`: IDF version, ESP-NOW version per node, frames sent, frames delivered.

### C4. Radio-disabled variant

**Question.** Does a build exist with an identical phase-marker path and no transmission?
**Runs on / with.** Same firmware tree, radio not brought up. Separate image, not a runtime flag, per the build discipline.
**Expected output.** Builds, runs, phase markers identical to C1, no frames on air.
**Artifact.** Second firmware image, committed.

---

## Phase D: PCB v1

Runs in KiCad, fabricated at JLCPCB. Cost: fabrication, assembly, one to three spins.

### D1. Layout requirements that are not negotiable

**Question.** Does the layout avoid the two errors that would silently bias every measurement?
**Runs on / with.** KiCad, checked at review before Gerbers are exported.
**Expected output.** High-side sensing on both channels, so one ground is continuous across both nodes and the capture engine; low-side would put a shunt in each node's ground return and design in a ground offset. Kelvin sense traces meeting the shunt at its own pads and carrying no load current, because at 0.1 ohm one milliohm of parasitic in that path is a one percent error, comparable to the entire budget. Datasheet input filter if transients near the sampling rate are expected.
**Artifact.** Review checklist signed off against the layout, committed with the project.

### D2. Identical channels

**Question.** Are the ten boards interchangeable?
**Runs on / with.** KiCad.
**Expected output.** Same shunt value, same layout, repeated or mirrored. Ten boards are spares only if they are interchangeable; that interchangeability is the whole argument for fabricating ten.
**Artifact.** One schematic, one layout, ten assemblies.

### D3. Fabricate

**Question.** Do the boards pass basic electrical checks before power?
**Runs on / with.** JLCPCB, then DMM continuity and short checks on the bench.
**Expected output.** Ten boards of one design. Not ten of several. Continuity and shorts checked on every board before any is powered.
**Artifact.** `results/stage0_d3_incoming.json`: per-board pass or fail, with serial numbers assigned.

---

## Phase E: validation, the three halves

Runs on the assembled harness with two ESP32-S3 nodes. This is the gate itself. None of the three halves may be skipped, because the first alone is consistent with both total success and total failure.

### E1. No-op

**Question.** Does the harness reproduce a load it should reproduce, and do all expected signals appear?
**Runs on / with.** Precision resistor across the rail; DMM as the independent reference; both channels; unmodified ESP-NOW example on the nodes.
**Expected output.** Current reported within 0.5 percent of the value computed from the resistor and the DMM, on both channels. Frames delivered. Phase-marker edges and conversion-ready edges both present, from both nodes.
**Artifact.** `results/stage0_e1_noop.json`.

A harness reading the wrong rail, or with its sense resistor on the far side of the regulator, or with an unconnected marker, fails none of these visibly if only one of the three is checked.

### E2. Negative control

**Question.** Does commanding a change actually move the trace, and do the phase markers bracket what they are believed to bracket?
**Runs on / with.** C2 commanded busy loop across at least a decade of durations; C4 radio-disabled image for the second half.
**Expected output.** The integral over the busy loop is proportional to commanded duration, with the residual from a straight-line fit under 2 percent. With the radio disabled, the transmit-phase integral collapses toward the compute-only level. If it does not collapse, the markers are in the wrong place and every later phase attribution is void.
**Artifact.** `results/stage0_e2_negctl.json`: the duration sweep with its fit, and the radio-on against radio-off integrals.

### E3. Instrument check

**Question a, gain and offset.** Does each channel agree with a standard, and do the two channels agree with each other?
**Runs on / with.** Precision resistor and DMM. Use four-wire mode if the DMM has it, measuring the resistor directly so its tolerance drops out and only DMM accuracy remains.
**Expected output.** Each channel within 0.5 percent of the reference. The two channels within 0.2 percent of each other, after applying measured shunt values rather than nominal. The channel-to-channel figure is the one that matters: a systematic gain error cancels when comparing conditions on the same channel, but not between channels, and the transmit-side against receive-side asymmetry is a stated prediction that a mismatch lands directly on.
**Artifact.** `results/stage0_e3a_gain.json`, including the measured shunt value for each board, which is carried forward in place of nominal from this point on.

**Question b, timing.** Does the harness reproduce intervals the DUT commanded from its own clock?
**Runs on / with.** C2 commanded pulse widths across at least a decade.
**Expected output.** Reproduced within 2 microseconds or 0.5 percent, whichever is larger.
**Artifact.** `results/stage0_e3b_timing.json`.

**Question c, bandwidth.** Does a load toggling near the conversion time survive as a duty cycle, or smear toward its mean?
**Runs on / with.** C2 commanded-duty square wave near 140 microseconds.
**Expected output.** Duty cycle reproduced within 5 percent.
**Artifact.** `results/stage0_e3c_bandwidth.json`.

**Question d, dropped conversions.** Does B3's result hold on the real board at campaign rate?
**Runs on / with.** Full-length run on the assembled harness.
**Expected output.** Zero.
**Artifact.** `results/stage0_e3d_dropped.json`.

**Question e, supply stability.** Is the supply constant enough for the DUT rail to be computed rather than converted?
**Runs on / with.** Periodic bus-voltage conversion, roughly one sample in a hundred, VBUS taken upstream of the shunt. The DUT rail per sample is supply minus measured current times measured shunt value.
**Expected output.** Supply excursion under 0.5 percent across a transmit burst. If it exceeds that, the supply needs decoupling, or bus voltage must be converted every sample, which halves the conversion budget and forces high-speed I2C.
**Artifact.** `results/stage0_e3e_supply.json`.

**Optional, not a dependency.** If a commercial single-rail meter can be borrowed, run it alongside on one channel as a cross-check on the one case both can measure. It does not enter the method. Traceability comes from E3a and E3b, which reference standards rather than another instrument.

---

## Phase F: deliverable

**Question.** Is the harness an instrument?

**Expected output.** All six of the following exist and are consistent with each other:

1. The control traces from E1 and E2.
2. Gain and timing references with their residuals, per channel, and the channel-to-channel match figure.
3. Dropped-conversion count from the assembled board.
4. Measured shunt values for both channels, carried forward in place of nominal.
5. Supply excursion under transmit burst.
6. One segmented event plot annotated with its own boundary uncertainty, roughly 140 microseconds. Illustrative, not inferential; per the identification decision, nothing downstream depends on resolving a boundary more finely.

**Artifact.** `FINDINGS.md` for Stage 0, in which every figure names the file under `results/` that it was read from, plus a predictions ledger section carrying forward the entries from thinkbook Section 7 that Stage 1 will adjudicate.

Until these exist, the harness is not an instrument and nothing measured with it counts.

---

## What Stage 0 can kill, and where it is caught

| Finding | Consequence | Caught at |
|---|---|---|
| I2C margin worse than 52 percent under real interrupt latency | High-speed I2C, or a different capture part | B3, before fabrication |
| Conversions dropped under load | Firmware architecture wrong, most likely the read is in the interrupt path | B2 and B3, before fabrication |
| Timestamp jitter near the read duration | Timebase coupled to the bus | B2, before fabrication |
| Host cannot sustain the stream | Gated capture becomes mandatory, or a faster transport | B4, before fabrication |
| Transmit-phase integral does not collapse with the radio off | Phase markers in the wrong place; every phase attribution downstream is void until fixed | E2, after assembly |
| Channel-to-channel gain mismatch above 0.2 percent after correction | The transmit against receive asymmetry prediction cannot be tested at the precision it needs | E3a, after assembly |
| Supply excursion above 0.5 percent under transmit burst | Bus voltage converted every sample, halving the budget and forcing high-speed I2C | E3e, after assembly |

Four of the seven are caught before any fabrication spend, which is the reason Phase B exists as a separate phase rather than as bring-up.
