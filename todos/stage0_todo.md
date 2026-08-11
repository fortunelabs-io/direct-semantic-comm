# Stage 0 TODO

## How this file works

Every entry below is a gate, not a task. A gate has one claim, one command, and
one criterion that can fail. It is closed when its test exits 0 with no output.

Gates are ordered by the cost of discovering a failure late, which is the work a
failed gate invalidates behind it. Nothing below an open gate gets run.

There are no time estimates here. A gate is closed or it is not.

Predictions are written into this file before the run that tests them. A
prediction added afterward is worth nothing and should be deleted rather than
backdated.

**Bench actions are not gates.** Soldering, ordering fabrication, and connecting
a reference have no command and no criterion of their own. They are listed where
they fall in the order, and the gate below each one is what proves it was done
correctly. A solder joint is proved by Tier 3 passing, not by inspecting the
joint.

**What the ladder does and does not enforce.** Each capture script refuses to
start when the file it reads is absent, so the order below is enforced by data
dependency rather than by a runner or by discipline. That mechanism enforces
*order* only. Unlike a software ladder, a hardware capture writes a
well-formed file whether or not the sense resistor was on the correct side of the
regulator. File existence cannot detect that. Only the negative control in Tier 3
can, which is why it is a gate and not a check.

---

## Where the results are

`FINDINGS.md` carries what the gates measured, with the file each figure was read
from named beside it, and it is the document to read for outcomes.

This file is the specification and stays that way. Status is not repeated here. A
number appearing in both files is a number that will disagree with itself.

---

## Precondition

Tier 0 of the Stage -1 configuration contract is closed: all five questions
answered, nothing open. Modality is a six-axis IMU at 100 Hz int16 with window
length as the swept parameter, latent width 64 int8, core runs on ESP-NOW v1.0,
shunt 0.1 ohm on both channels high side. Nothing in this file starts otherwise,
because Stage -1 costs an afternoon and can kill the direction, and everything
below is the first substantial expenditure the project makes.

---

## Repository map

Entries marked `planned` do not exist yet.

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
├── scripts/                             touch hardware, may prompt, write JSON
│   ├── capture_rate.py
│   ├── capture_jitter.py
│   ├── capture_dropped.py
│   ├── capture_stream.py
│   ├── capture_wrap.py
│   ├── capture_phase.py
│   ├── capture_link.py
│   ├── capture_incoming.py
│   ├── capture_noop.py
│   ├── capture_negctl.py
│   ├── capture_gain.py
│   ├── capture_timing.py
│   ├── capture_bandwidth.py
│   └── capture_supply.py
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

Every number that later appears in a claim is read from a named file under
`results/`. Nothing is recalled, nothing is retyped.

**Capture and test are separate, and the split is not cosmetic.** A capture script
touches hardware and may prompt for a reference reading typed from a meter. A
test reads JSON, computes, and exits. A test that prompts is a test whose result
depends on who ran it.

---

## Toolchain

| Where | What |
|---|---|
| Task isolation and tool pinning | mise. The ESP-IDF version pinned here is the one the Stage -1 contract declares, which is what turns that declaration into something enforced |
| Editor | VSCode |
| DUT firmware | ESP-IDF v5.x, Espressif IDF extension or PlatformIO. ESP32-S3 has native USB-serial-JTAG, no external programmer |
| Capture firmware | STM32F411CEU6, bare metal: arm-none-eabi-gcc, CMake, Ninja, st-flash. No CubeMX and no vendor HAL, per `docs/adr/2026-08-12-capture-engine-firmware-is-bare-metal.md` |
| Host | Python 3, pyserial, numpy, pandas, matplotlib, scipy |
| PCB | KiCad, JLCPCB for fabrication and optional assembly, LCSC for parts |
| Bench | DMM with DCV accuracy 0.1 percent or better; 0.1 percent metal film resistors; regulated linear bench supply; low-cost USB logic analyser with PulseView or sigrok |
| Not required | Oscilloscope. Supply excursion is covered by periodic bus-voltage conversion at 1.25 mV per bit, 0.04 percent of the rail |

The DMM and the resistors are the traceability standards for the gain and timing
gates and are the only real instrument purchase. The logic analyser exists only
for Tier 1, because the capture engine cannot be debugged with itself.

---

## Tier 0: nothing powered

### The capture engine part satisfies the timing budget

**Claim.** One part is selected whose datasheet shows two independent I2C masters
on separate pins, ten GPIO with edge interrupt, a microsecond timer, and USB
device.

**Command.** `mise run toolchain`

**Produces.** `results/stage0_toolchain.json`, and the part named in
`.mise.toml`.

**Passes when.** A blink builds and flashes on the selected part, and the two I2C
peripherals are recorded from the datasheet rather than from a product page.

**Cost of skipping.** Every layout decision below is drawn against a part that
may not have the peripherals.

### The three implementation choices are written down

**Claim.** Free-running or gated capture, parallel or serial phase code, and
bench sensor source are decided before firmware exists.

**Command.** none, this is a bench action

**Produces.** Three lines in this repository's README, and the four-state Gray
sequence written out for both roles.

**Recommendation on record.** Free-running for v1, because a gate that fires
wrongly loses data silently and a full stream can always be trimmed on the host.
Parallel phase code, because serial adds a latency that would itself need
characterising.

**Why the Gray sequence is written before firmware.** The capture engine samples
the phase bus asynchronously. If two bits change on one transition, a sample
landing inside it latches a code that was never intended. One bit per transition
makes a mid-transition sample read the old code or the new one and never a third.

---

## Tier 1: capture engine alone, on breakouts

Nothing here needs the device under test. The load is a resistor. What is under
test is whether the timing budget survives real interrupt latency and real driver
overhead, and four of the seven findings that can kill the design are caught in
this tier, before any fabrication is ordered.

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

**Prediction on record.** The rate follows from the datasheet conversion time and
nothing else. A rate near half of it means bus-voltage conversion is still
enabled, which is the power-on default and the single most likely configuration
error.

**Cost of skipping.** Every sample rate below is asserted against a rate that was
never confirmed.

### The timestamp is taken at the edge and not after the read

**Claim.** The conversion-ready interrupt captures the counter and queues the
read. The read does not sit inside the interrupt.

**Command.** `mise run jitter`

**Produces.** `results/stage0_jitter.json` with the inter-timestamp interval
histogram and its standard deviation.

**Passes when.** Standard deviation of the interval between consecutive
timestamps is under 2 microseconds.

**Prediction on record, the failure signature.** If the read is left in the
interrupt path, the standard deviation lands near 73 microseconds, which is the
read duration at 400 kHz with the pointer retained. The failure has a recognisable
value rather than merely a direction, so a wide histogram is diagnosable in one
glance rather than by bisection.

**Cost of skipping.** Bus latency and its scheduling jitter enter the timebase
directly, and every phase boundary downstream inherits it.

### Two channels on two buses drop nothing

**Claim.** The 52 percent per-bus utilisation derived from the timing budget holds
in practice with both channels free-running and unsynchronised.

**Command.** `mise run dropped`

**Produces.** `results/stage0_dropped.json` with edges seen, records emitted, and
their difference per channel.

**Passes when.** Zero dropped conversions over ten minutes at full rate on both
channels. Zero, not few. A drop is a bug, not a tolerance.

**Prediction on record.** A 16-bit read with the pointer retained is about 29
clock periods, 73 microseconds at 400 kHz, against a 140 microsecond conversion.
Rewriting the pointer each time would be 86 percent and two channels on one bus
would be 104 percent, which fails outright. If drops appear at two buses, the
arithmetic is intact and the overhead is in the driver, and the fallback is
high-speed mode at 2.94 MHz rather than a different sensor.

**Cost of skipping.** Discovered after fabrication, this costs a board spin
rather than an afternoon.

### The host sustains the stream

**Claim.** 114 kilobytes per second reaches disk without gaps.

**Command.** `mise run stream`

**Produces.** `results/stage0_stream.json`, and the raw capture retained.

**Passes when.** Ten minutes captured with no gaps, no host-side overflow, and a
record count matching the dropped-conversion gate.

**Prediction on record.** 8 bytes per record at 14,286 records per second is 114
kilobytes per second, which is 1.14 megabits per second on the wire. USB CDC
carries it with headroom; a UART would need 2 megabaud.

### Timestamps stay monotonic across the counter wrap

**Claim.** A run crossing the 32-bit microsecond wrap produces monotonic
timestamps on the host.

**Command.** `mise run wrap`

**Produces.** `results/stage0_wrap.json`

**Passes when.** Timestamps are monotonic across the wrap, with the counter seeded
near overflow rather than by waiting.

**Prediction on record.** A 32-bit microsecond counter wraps at about 71 minutes
and a Stage 2 campaign runs longer than that. The extension belongs in the capture
firmware and never on the host, because a wrap reaching the host unhandled
produces timestamps that look plausible and are wrong.

---

## Tier 2: device under test, still on breakouts

### The phase bus decodes with no invalid codes

**Claim.** The capture engine decodes a complete four-state Gray sequence for both
roles.

**Command.** `mise run phase`

**Produces.** `results/stage0_phase.json` with transition count, invalid code
count, dwell time per state.

**Passes when.** Ten thousand transitions decoded, zero invalid codes.

**Prediction on record.** With Gray coding, a sample landing mid-transition reads
the old code or the new one. Any invalid code at all means either the sequence is
not Gray-ordered or the marker is writing bits in more than one operation, and
both are visible in the transition that produced it.

**Note on the marker.** One masked register write, nothing else in the function.
The ESP-NOW send callback runs from a high-priority Wi-Fi task where the vendor
documentation states lengthy operations must not happen.

### The link works and the ESP-NOW version is what the contract declared

**Claim.** Frames deliver, and both nodes report the same ESP-NOW version as the
one pinned in `.mise.toml`.

**Command.** `mise run link`

**Produces.** `results/stage0_link.json` with IDF version, ESP-NOW version per
node, frames sent, frames delivered.

**Passes when.** Both nodes report the same version, that version equals the
contract's declaration, and the vendor example delivers frames.

**Why it is here.** This gate verifies a declaration rather than answering an open
question. A v1.0 receiver given a v2.0 packet longer than
`ESP_NOW_MAX_IE_DATA_LEN` truncates to the first `ESP_NOW_MAX_IE_DATA_LEN` bytes
or discards it, and truncation presents as a decode failure rather than a link
failure. Attributed to the representation, that costs a week.

### A radio-disabled image exists with an identical marker path

**Claim.** A second image runs the same phase-marker path and never brings up the
radio.

**Command.** `mise run link` (same run, separate assertion)

**Passes when.** The image builds, phase markers decode identically to the gate
above, and no frames appear on air.

**Why it is a separate image.** A runtime flag on one image means the radio-off
condition and the radio-on condition share code that could differ in the
condition being tested.

---

## Tier 3: the fabricated harness

### Bench action: layout, review, fabricate, assemble

No command. Proved by the gates below.

Three layout requirements are not negotiable and are checked at review before
Gerbers are exported. High-side sensing on both channels, so one ground is
continuous across both nodes and the capture engine; low-side would put a shunt in
each node's ground return and design in an offset between two device grounds.
Kelvin sense traces meeting the shunt at its own pads and carrying no load
current, because at 0.1 ohm one milliohm of parasitic in that path is a one
percent error, comparable to the whole budget. Datasheet input filter if
transients near the sampling rate are expected.

Ten boards of one design, not ten of several. Ten boards are spares only if they
are interchangeable, and interchangeability is the reason for fabricating ten.

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

**Why all three.** A harness reading the wrong rail, or with its sense resistor on
the far side of the regulator, or with an unconnected marker, fails none of these
visibly when only one is checked.

**Cost of skipping.** Every number produced after this point is conditional on a
harness that was never checked.

### Commanding a change moves the trace, and the markers bracket what they claim

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

**Why both.** A trace that reproduces a static load is what a correct harness
produces. It is also what a harness measuring something else that happens to be
stable produces. The static case alone cannot tell success from total failure. The
second half is what separates them, and it is the only gate in this file that can
detect a physically wrong setup, because file existence cannot.

**Cost of skipping.** If the markers are in the wrong place, every phase
attribution downstream is void and nothing in the data says so.

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
systematic gain error cancels when comparing two conditions on the same channel
and in the recovered-fraction ratio, but not between channels, and the
transmit-side against receive-side asymmetry is a stated prediction in the
thinkbook that a mismatch lands directly on.

**On the reference.** A precision resistor and an independent voltmeter are
standards. Agreement with another instrument would be a cross-check, and
traceability runs to standards rather than to instruments. If the meter has a
four-wire mode, measure the resistor directly and its tolerance drops out.

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
cycle rather than smeared toward its mean.

**Command.** `mise run bandwidth`

**Produces.** `results/stage0_bandwidth.json`

**Passes when.** Duty cycle reproduced within 5 percent.

**Prediction on record.** The front end is delta-sigma and its output is an
average over the conversion window, so a conversion straddling an edge blends both
sides. Boundary location therefore carries an intrinsic uncertainty of one
conversion time. Nothing in the thinkbook depends on locating a boundary more
precisely, because every term in the cost model is identified across runs rather
than within one.

### The supply is constant enough for the rail to be computed

**Claim.** Supply excursion under a transmit burst is small enough that the device
rail can be computed rather than converted.

**Command.** `mise run supply`

**Produces.** `results/stage0_supply.json`

**Passes when.** Supply excursion under 0.5 percent across a transmit burst,
measured by periodic bus-voltage conversion at roughly one sample in a hundred,
with VBUS taken upstream of the shunt.

**Prediction on record.** Only the supply upstream of the shunt is constant. The
rail the device sees is that supply minus the shunt drop, 33 millivolts at a 330
milliamp transmit peak, one percent. The rail per sample is therefore computed as
supply minus measured current times measured shunt value, and treating the device
rail itself as constant would put that one percent into every transmit-phase power
figure.

**Consequence if it fails.** Bus voltage must be converted every sample, which
halves the conversion budget and forces high-speed I2C.

---

## Closing

The harness is an instrument when every gate above is closed and `FINDINGS.md`
exists with each figure naming the file it was read from. Until then it is a board
that produces numbers.

`assets/harness_segmented_event.png` is drawn last: one event with its phases
segmented, annotated with its own boundary uncertainty of roughly 140
microseconds. It is illustrative and no claim rests on it.

---

## Standing rules

Tests are silent on success and exit 0.

A capture script may prompt for a reference reading typed from a meter. A test may
not. A test that prompts is a test whose result depends on who ran it.

Each capture script refuses to start when the file it reads is absent. That
enforces order and nothing else.

Only the negative control detects a physically wrong setup. No file check, no
schema, and no assertion on a well-formed capture can substitute for it, because a
capture written from the wrong rail is well-formed.

Bench actions have no pass criterion. The gate below one is what proves it.

Every asset carries the `harness_` prefix. Assets built without data carry
`harness_schematic_` instead, so a reader can tell from the filename which kind
they are holding.

An asset is not created before the file it reads from exists.

Measured shunt values replace nominal from the gain gate onward, everywhere.

No energy figure from this harness appears in a commit message, a README, or a
notebook before the negative control gate is closed.
