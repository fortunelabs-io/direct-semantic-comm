# TODO

## How this file works

Every entry below is a gate, which is different from an ordinary task. Each gate owns one claim, one command, and
one finish criterion. Each gate closes when the final test exits 0. (TODO: The test gate is still being figured out.)

Gates are grouped into **tiers**. A tier is a group of gates that share one hardware setup, from Tier 0 (nothing powered) to Tier 3 (the fabricated harness). Tiers run in order, and each tier needs the earlier tiers closed.

No gate is bounded by a deadline or ETA. This is to make sure that each gate is done properly, with better intention from researchers and developers.

// MUST BE: We set the prediction first, before we create the run test and set the pass criterion

**Bench actions are not gates by definition.** Soldering, ordering fabrication, and connecting
a reference have no command and no criterion in this repository. They are external activities for building the actual physical system that supports the research. Also, we cannot write a capture or CI check for joining a cable or soldering an MCU.

**What the tier order does and does not enforce.** Each capture script refuses to
start when the file it reads is absent, so the order below is enforced by data
dependency. That mechanism enforces
*order* only. A hardware capture writes a valid file even if the sense resistor was on the wrong side of the
regulator. A file check cannot detect that. Only the negative control in Tier 3
can, which is why it is a gate and not just a check mechanism.

---

## Where the results are

The results belong to `FINDINGS.md`. It records what each gate measured and names the source file beside every figure (read from `results/`). This file defines the gates and the predictions made before each run. It holds no measured results.

---

## Precondition

The Stage -1 configuration contract is closed, and all five questions are answered:
1. Modality is a six-axis IMU at 100 Hz int16.
2. Window length is the swept parameter.
3. Latent width is 64 int8.
4. The core runs on ESP-NOW v1.0.
5. The shunt is 0.1 ohm on both channels, high side.

---

## Repository map

This is the `planned` repository map.

```
harness/
├── .mise.toml                           tool pinning and task isolation
├── TODO.md                              this file, the specification
├── FINDINGS.md                          what the gates measured            planned
├── firmware/
│   ├── capture/                         capture engine                     planned
│   │   ├── timebase.c                   counter, wrap extension, edge capture
│   │   ├── sensor.c                     INA226 config and read, one per bus
│   │   └── stream.c                     record framing to host
│   └── dut/                             ESP32-S3, ESP-IDF, one image per role
│       ├── phase_marker/                one masked register write, nothing else
│       ├── commanded_load/              busy loop and square wave
│       ├── espnow_link/                 unmodified vendor example
│       └── espnow_link_noradio/         same path, radio never brought up
├── scripts/                             capture_<gate>.py, one per gate: touch hardware, may prompt, write JSON
├── tests/                               pure, read JSON, silent on success
│   └── one per gate below
├── hardware/
│   ├── harness.kicad_pro                                                   planned
│   └── review_checklist.md
├── results/                             json tracked, raw captures not
└── assets/
    ├── harness_schematic_sense_topology.png                                planned
    ├── harness_negative_control.png                                        planned
    ├── harness_gain_residuals.png                                          planned
    └── harness_segmented_event.png                                         planned
```

Every number that appears in a claim is read from a named file under
`results/`. 

**Capture and test are separate.** A capture script touches hardware and may prompt for a reference reading typed from the DMM. A test reads JSON, computes, and exits, so anyone can rerun it on the same capture and get the same result.
---

## Toolchain

| Where | What |
|---|---|
| Task isolation and tool pinning | mise. It pins the ESP-IDF version declared in the Stage -1 contract, so that declaration is enforced |
| Editor | VSCode |
| DUT firmware | ESP-IDF v5.x, Espressif IDF extension or PlatformIO. ESP32-S3 has native USB-serial-JTAG, so no external programmer is needed |
| Capture firmware | STM32F411CEU6, bare metal: arm-none-eabi-gcc, CMake, Ninja, st-flash. No CubeMX and no vendor HAL, per `docs/adr/2026-08-12-capture-engine-firmware-is-bare-metal.md` |
| Host | Python 3, pyserial, numpy, pandas, matplotlib, scipy |
| PCB | KiCad, JLCPCB for fabrication and optional assembly, LCSC for parts |
| Bench | DMM with DCV accuracy 0.1 percent or better; 0.1 percent metal film resistors; regulated linear bench supply; low-cost USB logic analyzer with PulseView or sigrok |
| Not required | Oscilloscope. Supply changes are covered by periodic bus-voltage readings at 1.25 mV per bit, which is 0.04 percent of the rail |

The DMM and the resistors are the reference standards for the gain and timing
gates. The logic analyzer is only for Tier 1.

---

## Tier 0

### The capture engine part satisfies the timing budget

**Claim.** One part is selected whose datasheet shows two independent I2C masters
on separate pins, ten GPIO with edge interrupt, a microsecond timer, and USB
device.

**Command.** `mise run toolchain`

**Produces.** `results/stage0_toolchain.json`, and the part named in
`.mise.toml`.

**Passes when.** The clock plan holds against every limit copied from the
part's datasheet, and the two I2C peripherals are recorded from the datasheet,
on separate pins with their alternate function numbers.

**Why this is checkable with nothing powered.** The datasheet limits are
copied into `firmware/capture/timing_budget.h` as constants, and the clock
plan is asserted against them at compile time. The header is integer arithmetic
with nothing target-specific, so the host compiler evaluates the same
assertions the cross compiler will, and a plan that violates a datasheet constraint would fail to compile. That is a criterion that can fail even without real hardware.

### The three implementation choices are written down

**Claim.** Free-running or triggered capture, parallel or serial phase code, and
bench sensor source are decided before firmware exists.

**Command.** none, this is a bench action

**Produces.** Three lines in `harness/README.md`, one architecture decision
record for each of the first two choices, and the Gray sequence written out for
both roles in `docs/hardware-harness-v1/phase_code_map.md`.

**Decision on record, first.** Capture is free-running, per
`docs/adr/2026-08-17-capture-is-free-running.md`. A triggered capture that fires
wrongly loses data without any warning. A full stream can be trimmed on the host
later, but a trimmed stream cannot be restored.

**Decision on record, second.** The phase code is three parallel bits per node,
per `docs/adr/2026-08-17-phase-code-is-parallel-three-bit.md`. A serial code adds
a delay that would need to be measured and subtracted from every phase boundary,
and phase boundaries are the one thing this harness exists to measure.

**Decision on record, third.** The bench sensor is the bare INA226AIDGSR in
VSSOP-10, LCSC C49851. No breakout is used at any tier. The shunt is 0.1 ohm, fixed
by the timing budget and the Stage -1 contract. Many breakouts use 0.002 ohm, where
one count is 1.25 mA. That is 1.3 percent of the 97 mA receive level, which already
exceeds the 0.5 percent limit of the gain gate.

**Consequence.** VSSOP-10 cannot be breadboarded, so Tier 1 and Tier 2 need a
hand-assembled sensor board. It needs the same Kelvin sense traces as the
fabricated harness: at 0.1 ohm, one milliohm in the sense path is a one percent
error.

**Why the Gray sequence is written before firmware.** The capture engine can read
the phase bus while it is changing. If only one bit changes per step, a read sees
the old code or the new one, never a code that was not sent. Writing the sequence
first found two problems:

1. The sequence needs six states per role, not four. Sleep entry is its own phase,
   and both roles send an acknowledgement (ACK). Five states are impossible,
   because a cycle of one-bit steps needs an even number of states.
2. With six states, every code on the bus is meaningful. The `phase` gate
   therefore counts invalid transitions, not invalid codes.

Both are recorded in `phase_code_map.md`. The second one changes a gate below.

---

## Tier 1: capture engine alone, on breakouts

Nothing here needs the device under test. The load is a resistor. These gates check
the timing budget against real interrupt delay and driver overhead, before any
fabrication is ordered.

### Code built for the selected part runs on it

**Claim.** The cross toolchain produces an image that the selected part executes.

**Command.** `mise run blink`

**Produces.** `results/stage0_blink.json`

**Passes when.** A blink builds with `arm-none-eabi-gcc`, flashes over SWD, and
is observed running on the board.

### The sensor reaches its rated conversion rate

**Claim.** One INA226 configured as the thinkbook specifies delivers conversions
at the datasheet rate: shunt-only continuous, 140 microseconds, averaging one,
alert latch transparent, register pointer retained, shunt voltage register only.

**Command.** `mise run rate`

**Produces.** `results/stage0_rate.json` with the registers read back, edge count,
elapsed time, derived rate.

**Passes when.** 7,143 conversion-ready edges per second within one percent,
sustained ten minutes, with the configuration read back from the device rather
than assumed from what was written.

**Prediction on record.** The rate should match the datasheet conversion time
exactly. A rate near half means bus-voltage conversion is still enabled. That is
the power-on default and the most likely configuration mistake.

### The timestamp is taken at the edge and not after the read

**Claim.** The conversion-ready interrupt only records the counter and queues the
read. The read itself does not happen inside the interrupt.

**Command.** `mise run jitter`

**Produces.** `results/stage0_jitter.json` with the inter-timestamp interval
histogram and its standard deviation.

**Passes when.** Standard deviation of the interval between consecutive
timestamps is under 2 microseconds.

**Prediction on record, the failure signature.** If the read stays inside the
interrupt, the standard deviation will be near 73 microseconds, which is the read
time at 400 kHz with the pointer retained. A failure has a recognizable value, so
a wide histogram can be diagnosed directly instead of by trial and error.

### Two channels on two buses drop nothing

**Claim.** The 52 percent per-bus utilization from the timing budget holds in
practice with both channels free-running and not synchronized.

**Command.** `mise run dropped`

**Produces.** `results/stage0_dropped.json` with edges seen, records emitted, and
their difference per channel.

**Passes when.** Zero dropped conversions over ten minutes at full rate on both
channels. Zero, not few. A drop is a bug, not a tolerance.

**Prediction on record.** A 16-bit read with the pointer retained takes about 29
clock periods, which is 73 microseconds at 400 kHz, against a 140 microsecond
conversion. Rewriting the pointer on every read would raise the load to 86
percent, and two channels on one bus would reach 104 percent, which fails.
If drops still appear with two buses, the arithmetic is right and the
overhead is in the driver. The fallback is high-speed mode at 2.94 MHz, not a
different sensor.

### The host keeps up with the stream

**Claim.** 114 kilobytes per second reaches disk without gaps.

**Command.** `mise run stream`

**Produces.** `results/stage0_stream.json`, and the raw capture retained.

**Passes when.** Ten minutes captured with no gaps, no host-side overflow, and a
record count matching the dropped-conversion gate.

**Prediction on record.** 8 bytes per record at 14,286 records per second is 114
kilobytes per second, or 1.14 megabits per second on the wire. USB CDC carries
this with margin. A UART would need 2 megabaud.

### Timestamps stay monotonic across the counter wrap

**Claim.** A run crossing the 32-bit microsecond wrap produces monotonic
timestamps on the host.

**Command.** `mise run wrap`

**Produces.** `results/stage0_wrap.json`

**Passes when.** Timestamps are monotonic across the wrap, with the counter seeded
near overflow rather than by waiting.

**Prediction on record.** A 32-bit microsecond counter wraps after about 71
minutes, and a Stage 2 campaign runs longer than that. The wrap must be handled in
the capture firmware, never on the host. An unhandled wrap that reaches the host
produces timestamps that look right but are wrong.

---

## Tier 2: device under test, still on breakouts

### The phase bus decodes with no invalid transitions

**Claim.** The capture engine decodes a complete six-state Gray cycle for both
roles.

**Command.** `mise run phase`

**Produces.** `results/stage0_phase.json` with transition count, invalid
transition count, dwell time per state.

**Passes when.** Ten thousand transitions decoded, zero invalid transitions. The
legal set is the six edges of the cycle in
`docs/hardware-harness-v1/phase_code_map.md`, plus armed to wake, plus any code
to error. Everything else is invalid.

**Prediction on record.** With Gray coding, a read in the middle of a transition
sees the old code or the new one. Every legal edge changes exactly one bit. If two
or three bits change at once, either the sequence is not Gray-ordered or the
marker writes bits in more than one operation. Both show up in the transition that
caused them.

### The link works and the ESP-NOW version is what the contract declared

**Claim.** Frames deliver, and both nodes report the same ESP-NOW version as the
one pinned in `.mise.toml`.

**Command.** `mise run link`

**Produces.** `results/stage0_link.json` with IDF version, ESP-NOW version per
node, frames sent, frames delivered.

**Passes when.** Both nodes report the same version, that version equals the
contract's declaration, and the vendor example delivers frames.

**Why it is here.** A v1.0 receiver cuts or drops a v2.0 packet longer than
`ESP_NOW_MAX_IE_DATA_LEN`. That looks like a decode failure, not a link failure.

### A radio-disabled image exists with an identical marker path

**Claim.** A second image runs the same phase-marker path and never brings up the
radio.

**Command.** `mise run link` (same run, separate assertion)

**Passes when.** The image builds, phase markers decode identically to the gate
above, and no frames appear on air.

---

## Tier 3: the fabricated harness

### Bench action: layout, review, fabricate, assemble

No command. Proved by the gates below.

Three layout requirements are checked at review, before Gerbers are exported:

1. High-side sensing on both channels, so one ground is continuous across both
   nodes and the capture engine.
2. Kelvin sense traces that meet the shunt at its own pads and carry no load
   current. At 0.1 ohm, one milliohm in that path is a one percent error.
3. If transients near the sampling rate are expected, the datasheet input filter.

Ten boards of one design, so any board can replace another.

**Produces.** `results/stage0_incoming.json` with per-board continuity and short
checks and assigned serial numbers, written by `mise run incoming` before any
board is powered.

### The harness reproduces a load, and every expected signal appears

**Claim, three parts.** A known static load is reproduced within tolerance on both
channels; frames deliver; phase-marker edges and conversion-ready edges are both
present from both nodes.

**Command.** `mise run noop`

**Produces.** `results/stage0_noop.json`

**Passes when.** Current reported within 0.5 percent of the value computed from a
precision resistor and an independent voltmeter, on both channels, and all four
edge sources present.

### Commanding a change moves the trace, and the markers mark the start and end of what they claim

This is one gate with two halves. Neither half is meaningful alone.

**Claim, first half.** The integral over a commanded busy loop is proportional to
its commanded duration.

**Claim, second half.** With the radio disabled, the transmit-phase integral
collapses toward the compute-only level.

**Command.** `mise run negctl`

**Produces.** `results/stage0_negctl.json` with the duration sweep and its fit,
and the radio-on against radio-off integrals.

**Passes when.** The residual from a straight-line fit over at least a decade of
durations is under 2 percent, and the radio-off transmit integral collapses.

**Why both.** A harness measuring something else that happens to be stable also
reproduces a static load. Only the second half tells the two apart.

**Visual.** `assets/harness_negative_control.png`. Three conditions on one axis:
static load, commanded busy loop, radio disabled.

### Both channels agree with a standard, and with each other

**Claim.** Each channel is within 0.5 percent of a reference, and the two channels
are within 0.2 percent of each other after measured shunt values are applied.

**Command.** `mise run gain`

**Produces.** `results/stage0_gain.json`, including the measured shunt value for
each board, which replaces nominal from this point on.

**Passes when.** Both figures hold, per channel and between channels.

**Prediction on record.** The channel-to-channel figure is the one that matters. A
systematic gain error cancels when two conditions are compared on the same
channel, and in the recovered-fraction ratio. It does not cancel between channels.
The thinkbook predicts a transmit-side against receive-side asymmetry, and a
channel mismatch would affect it directly.

**On the reference.** The reference is a precision resistor and an independent
voltmeter. With a four-wire meter, measure the resistor directly so its tolerance
no longer matters.

**Visual.** `assets/harness_gain_residuals.png`. Per-channel residual against the
reference, with the channel-to-channel difference drawn on the same axis.

### Commanded intervals are reproduced

**Claim.** Pulse widths commanded by the device under test's own clock are
reproduced by the harness.

**Command.** `mise run timing`

**Produces.** `results/stage0_timing.json`

**Passes when.** Within 2 microseconds or 0.5 percent, whichever is larger, across
at least a decade of widths.

### A load toggling near the conversion time survives as a duty cycle

**Claim.** A commanded square wave near 140 microseconds is recorded as a duty
cycle rather than averaged toward its mean.

**Command.** `mise run bandwidth`

**Produces.** `results/stage0_bandwidth.json`

**Passes when.** Duty cycle reproduced within 5 percent.

**Prediction on record.** The front end is delta-sigma, so its output is an
average over the conversion window. A conversion that crosses an edge blends
both sides. A boundary can therefore only be located to within one conversion
time. Nothing in the thinkbook needs a more precise boundary, because every term
in the cost model is found by comparing runs, not within a single run.

### The supply is constant enough for the rail to be computed

**Claim.** The supply changes so little during a transmit burst that the device
rail can be computed instead of measured.

**Command.** `mise run supply`

**Produces.** `results/stage0_supply.json`

**Passes when.** The supply changes by less than 0.5 percent across a transmit
burst, measured by a bus-voltage reading about once every hundred samples, with
VBUS taken upstream of the shunt.

**Prediction on record.** Only the supply upstream of the shunt is constant. The
rail the device sees is that supply minus the shunt drop, which is 33 millivolts
(one percent) at a 330 milliamp transmit peak. So the rail for each sample is
computed as supply minus (measured current times measured shunt value). Treating
the device rail as constant would add that one percent error to every
transmit-phase power figure.

**Consequence if it fails.** Bus voltage must be converted every sample, which
halves the conversion budget and forces high-speed I2C.

---

## Closing

The harness is an instrument when every gate above is closed and `FINDINGS.md`
exists with each figure naming the file it was read from. Until then it is a board
that produces numbers.

`assets/harness_segmented_event.png` is drawn last and is illustrative only.

---

## Standing rules

Every asset has the `harness_` prefix. Assets made without data use
`harness_schematic_` instead.

An asset is not created before the file it reads from exists.

No energy figure from this harness appears in a commit message, a README, or a
notebook before the negative control gate is closed.
