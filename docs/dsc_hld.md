# Direct Semantic Communication: High-Level Design, v1

**Status: Draft.**
The architecture of the two-node semantic transfer experiment and the
instrument that meters it. Derived from the ADRs in `adr/`, from
DocID026289 Rev 4 (STM32F411xC/xE) and SBOS547B (INA226), and from the firmware
skeleton in `harness/firmware/capture/`.

---

## Preface: what this document is, and what it is not

The project already holds four kinds of document, and `docs/sop/git_sop.md` is
explicit that they do not overlap: `contracts/` declares, `todos/` specifies
gates, `docs/adr/` records decisions, `harness/results/` holds what was
measured. `docs/dsc_first_principle.md` sits above all four as the thinkbook.

So this document does three things and refuses a fourth:

1. **Decomposes** the system into components and states the interface at every
   boundary that crosses a component, a board, or a wire.
2. **Traces** each element to the ADR that decided it, the datasheet table that
   constrains it, or the source file that implements it.
3. **Names** what is unbuilt, undecided, or in tension, ranked by the cost of
   finding out late.

**Source-of-record rule.** Figures appear here only where the design is
unreadable without them. Each one names the file it was carried from. Where this
document and its source disagree, **the source wins and this document is wrong**;
that is the only arrangement in which a duplicated number is safe.

---

## 1. Scope, and the two tiers

The system is two tiers that share nothing but a question, and confusing them is
the most likely way to misread the architecture.

**The experiment tier** is what the thinkbook is about: two ESP32-S3 nodes
exchanging a payload over ESP-NOW, under Condition A (raw) and Condition B
(learned latent), across a swept observation size. This tier produces the claim.

**The instrument tier** is the two-channel metering harness: two INA226 current
sensors, one STM32F411CEU6 capture engine, one host. This tier produces the
*evidence for* the claim and is otherwise uninteresting. It exists because,
per `adr/2026-08-09-two-channel-harness-built-in-house.md`, no affordable
commercial instrument meters two rails against one clock, and a two-sided ledger
cannot rest on an assumption about time.

The tiers are asymmetric in an important way. The experiment tier can be wrong
and the data will say so. **The instrument tier can be wrong and the data will
look fine** — this is stated in the bare-metal ADR as the reason the capture
engine is the highest cost-of-failure software in the project. That asymmetry is
why the majority of this document is about the instrument.

**v1 scope.** Stages -1 through 2 (H1 and the fragmentation curve). The encoder,
decoder, projector and their training regime (Stages 3 and 5) are named in the
decomposition so the interfaces are reserved, and are not designed here.

### 1.1 System context

```
                          EXPERIMENT TIER
   +-----------------------+                   +-----------------------+
   |  Node S  (ESP32-S3)   |     ESP-NOW       |  Node R  (ESP32-S3)   |
   |  payload_gen          |    v1.0, 1 Mbps   |  fragmenter (rx)      |
   |  [encoder: B only]    | ================> |  [decoder: B only]    |
   |  fragmenter (tx)      |     n(p) frames   |  [processor: A only]  |
   |  phase_marker         | <================ |  phase_marker         |
   +---+---------------+---+   app-layer ACK   +---+---------------+---+
       | 3.3 V rail    | 3b phase                  | 3.3 V rail    | 3b phase
  =====+===============|===========================+===============|=======
       |               |     INSTRUMENT TIER       |               |
  +----v----+          |                      +----v----+          |
  | 0R1     |          |                      | 0R1     |          |
  | shunt   |          |                      | shunt   |          |
  +----+----+          |                      +----+----+          |
       | Kelvin        |                           | Kelvin        |
  +----v----+  CNVR    |                      +----v----+  CNVR    |
  | INA226  +------+   |                      | INA226  +------+   |
  |  ch S   |      |   |                      |  ch R   |      |   |
  +----+----+      |   |                      +----+----+      |   |
    I2C1 (400k)    |   |                        I2C3 (400k)    |   |
       |           |   |                           |           |   |
  +----v-----------v---v---------------------------v-----------v---v----+
  |            CAPTURE ENGINE  -  STM32F411CEU6, bare metal              |
  |   TIM2: free-running 32-bit, 1 us tick  <- the single clock          |
  |   EXTI: timestamp at the edge, read at leisure                       |
  |   two independent I2C masters, one per channel                       |
  +---------------------------------+------------------------------------+
                                    | USB CDC, ~114 kB/s   (OPEN, see 4.3)
                                    v
                     +--------------------------------+
                     |  HOST   meter_logger -> CSV    |
                     |         analyze    -> stats    |
                     |         tests      -> JSON     |
                     +--------------------------------+
```

**The single clock is the architecture.** Every other element is replaceable.
TIM2 on the capture engine is the only thing in the system that makes a
two-sided ledger a measurement rather than a stitch, and every design rule in
section 6 exists to keep bus latency, driver scheduling, and counter wrap out of
it.

---

## 2. Derivation

Each row is an architectural element, what pins it, and where the pin is written
down. A row with no source is an invention and does not belong in the system.

| Element | Pinned by | Source of record |
|---|---|---|
| Two nodes, both metered | The claim is about the *sum* of both sides (Gap 1) | thinkbook §2, §4.1 |
| One capture engine, two channels, one clock | Two single-rail meters give two timebases | `adr/…two-channel-harness-built-in-house.md` |
| Three-term cost model (`E_wake`, `E_pkt`, `e_byte`) | A one-wake-per-event design confounds the first two exactly | `adr/…wake-cost-separate-from-frame-cost.md` |
| Terms identified across runs, not within an event | Removes instrument bandwidth from the critical path | `adr/…terms-identified-by-design-not-by-waveform.md` |
| Ablation images, not compile-time flags | Same ADR; a flag shares code across the condition under test | same, and thinkbook §3.4 |
| `C_null` measured at Stage 1, before any encoder | A saving with no reference cannot be sized | `adr/…energy-reported-against-empty-event-null.md` |
| Stage 3 sweeps observation size | `p_raw` and `p_lat` do not scale with the same quantity | `adr/…compression-ratio-swept-not-fixed.md` |
| R holds no local observation | Collapse-by-substitution needs prior competence R lacks | `adr/…receiver-holds-no-local-observation.md` |
| Stage -1 precedes all hardware | The packet-count term is arithmetic, and can kill the direction | `adr/…config-contract-precedes-hardware.md` |
| ESP-NOW v1.0 for core runs, `L` = 250 B | Only version where the packet-count term is non-zero across the whole sweep | `contracts/stage_minus1_contract.md` Q1 |
| IMU, 6-axis, int16, 100 Hz; latent width 64 int8 | Lowest acquisition current; one scalar size parameter | same, Q1 |
| Shunt 0.1 Ω, high side, Kelvin | 2.5× headroom over the 330 mA transmit figure | same, Q5; timing budget §6 |
| INA226 as a catalogue part, everything above it owned | Boundary drawn so the sensor is replaceable | `adr/…two-channel-harness-built-in-house.md` |
| Shunt-only continuous, 140 µs, AVG = 1 | POR default (1.1 ms ×2) is longer than a whole transmission | SBOS547B Tables 7-3/7-5/7-6; thinkbook §4.7 |
| Alert pin as Conversion Ready | The INA226 timestamps nothing and I²C reads are not deterministic | SBOS547B §7.1.7 (CNVR, bit 10) |
| Two I²C buses, fast mode | One bus at 400 kHz with pointer retained is 104 % utilised | timing budget §4 |
| Capture engine is STM32, family not capability | Sourcing depth and established skill; RP2040 also met all four | `adr/…capture-engine-is-stm32-part-still-open.md` |
| Part is STM32F411CEU6 | Stock depth; F401CEU6 satisfied the gate identically | `adr/…capture-engine-part-is-stm32f411ceu6.md` |
| Bare-metal, register level, no HAL | The ISR-to-read path must be this project's own code | `adr/…capture-engine-firmware-is-bare-metal.md` |
| SYSCLK 96 MHz, not the part's 100 MHz | 100 MHz and an in-spec USB clock are mutually exclusive on this part | `timing_budget.h`; DocID026289 Rev 4 Table 41 |
| 3-bit Gray-coded phase bus per node | A single toggle desynchronises a whole run on one missed edge | timing budget §1; `phase_code_map.md` |
| Timestamp at the edge, read at leisure | The 73 µs read and its jitter would land in the timebase | timing budget §8 |
| Sensors free-run, never synchronised | The ledger needs a common *time*, not a common *sample instant* | same |

---

## 3. Component decomposition

Per thinkbook §3.4: each component does one thing, and components communicate through flat files and serial text so any one can be replaced or audited without touching the others.

### 3.1 Experiment tier

| Component | Runs on | Does | Explicitly does not |
|---|---|---|---|
| `payload_gen` | S | Produce a payload of commanded **observation** size and content class | Know about frames |
| `encoder` (B only) | S | int8 autoencoder bottleneck, width 64 | Exist in Condition A images |
| `fragmenter` | S and R | Chunk to `L`, one sequence byte, reassemble | Allocate dynamically |
| `phase_marker` | S and R | **One masked register write.** Nothing else | Compute, log, or branch |
| `processor` (A only) | R | Full processing of raw data | Exist in Condition B images |
| `decoder` (B only) | R | Consume the latent for the task | Exist in Condition A images |
| `projector` (Stage 5) | R | Bridge frozen Encoder_S to frozen Decoder_R | Exist before Stage 3 is trusted |

`payload_gen` takes an observation size rather than a payload size. That is not
a naming preference: `adr/…compression-ratio-swept-not-fixed.md` makes the
observation the swept axis, and a component that accepts bytes would silently
move the sweep onto the wrong variable.

**The ablation family.** Because terms are identified across runs, each of these
is a separate image, not a mode: full Condition A, full Condition B, encode-only,
transmit-only, radio-disabled, commanded-load, and the `k`-payloads-per-wake
variant. The count is the cost of not needing a faster meter, and it is paid
in build and flash discipline rather than in instrument budget.

### 3.2 Instrument tier: the capture engine

The repository map in `todos/stage0_todo.md` names three files. This is what each
owns, and what exists today.

| Module | Owns | State |
|---|---|---|
| `timebase.c` | Clock tree, TIM2 microsecond tick, DWT, **and (Tier 1) EXTI edge capture** | Clock + tick built; edge capture not written |
| `sensor.c` | Both I²C masters; INA226 configure, read, and pointer discipline | GPIO/peripheral bring-up built; INA226 layer not written |
| `stream.c` | Record framing to the host | **Does not exist** |
| `main.c` | Glue and, at Tier 0, a blink | Built; flagged in-file as a fourth file the map did not call for |
| `register_map.h` | The registers this firmware touches | RCC, GPIO, I²C, TIM2, FLASH, DWT. **No SYSCFG, EXTI, NVIC, or USB** |
| `timing_budget.h` | Datasheet limits and the clock plan, statically asserted | Built; the discipline here is the model for everything else |

`timing_budget.h` deserves a note as architecture rather than as a file. It keeps
`LIMIT_*` (transcribed from the datasheet, with the table cited) apart from
chosen values, and asserts only the second against the first. Its header records
why: an earlier revision asserted `1 * 2 <= 2` and passed with no part in the
room. **That separation is the pattern every later budget in this project should
follow**, and it is the reason a paper gate can fail honestly.

### 3.3 Host tier

`meter_logger` reads the stream and emits one CSV row per event. `analyze`
consumes CSV and never touches hardware. Under them sit the Stage 0 capture
scripts and tests, and those two are separate by rule: **a capture script may
prompt for a reference reading typed from a meter; a test may not.** A test that
prompts is a test whose result depends on who ran it.

---

## 4. Interfaces

An interface is where this design can be got wrong by two people who each read
their own side correctly. There are four.

### 4.1 Capture engine ↔ INA226 (I²C)

**Topology.** One master per channel. I2C1 on PB6/PB7, I2C3 on PA8/PB4, per
Table 9 of DocID026289 Rev 4, recorded in `sensor.h` and programmed from those
same constants in `sensor.c` so the Tier 0 datasheet record and the firmware
cannot drift apart.

The alternate function numbers are **not uniform**: AF4 on PB6, PB7, PA8, and
**AF9 on PB4**. `sensor.c`'s header explains why this is the trap it is — Table 9
heads AF04 "I2C1/I2C2/I2C3" and AF09 "I2C2/I2C3", so I2C3 appears in both
columns and which applies is a per-pin fact. AF4 on PB4 is blank, so the wrong
reading would have initialised cleanly, reported no error, and never reached the
pad. That comment is load-bearing and should survive any refactor.

**Register configuration** (SBOS547B §7.1, Tables 7-1 through 7-12):

| Register | Value | Meaning |
|---|---|---|
| Configuration `00h` | `0x4005` | AVG = 1, VSHCT = 140 µs, MODE = 101 shunt-continuous; bits 14:12 left at POR |
| Mask/Enable `06h` | `0x0400` | CNVR (bit 10) on; LEN = 0 transparent; APOL = 0 active-low |
| Shunt Voltage `01h` | read only | Two's complement, LSB 2.5 µV, full scale 81.92 mV |
| Calibration `05h` | never written | Current is computed on the host, where the shunt value is auditable |

POR is `0x4127` — shunt *and* bus continuous at 1.1 ms each. A rate near half of
7,143 conv/s is therefore the signature of a configuration that never took, and
`todos/stage0_todo.md` already records that prediction against the `rate` gate.
**The configuration is read back from the device rather than assumed from what
was written**, which is the only way that gate distinguishes the two.

**Bus timing.** Fast mode, 400 kHz, DUTY = 0. At PCLK1 = 48 MHz this is
`CCR = PCLK1 / (3 × f_SCL) = 40` and `TRISE = (48 × 300)/1000 + 1 = 15`, using
the 300 ns fast-mode rise limit rather than the 1000 ns standard-mode one. The
resulting split is t_LOW = 1.67 µs and t_HIGH = 0.83 µs, against INA226
Table 6-3 minima of 1300 ns and 600 ns. **Both clear.**

One note is worth closing here before it alarms a later reader: RM0383 states
that f_PCLK1 must be a multiple of 10 MHz to reach 400 kHz in fast mode, and 48
MHz is not. That constraint belongs to **DUTY = 1** (16/9), where
`CCR = PCLK1 / (25 × f_SCL)` and exactness requires a 10 MHz multiple. At
DUTY = 0 the divisor is 3 and 48 MHz gives exactly 400 kHz. The clock plan and
the 400 kHz requirement are compatible, and the reason is the duty selection.

**Budget** (timing budget §4, carried):

| Transaction | Clocks | At 400 kHz | Share of a 140 µs conversion |
|---|---|---|---|
| Read, pointer retained | ~29 | 73 µs | **52 %** |
| Read, pointer rewritten | ~48 | 120 µs | 86 % |
| Two channels, one bus, retained | — | — | 104 %, fails |

**Pointer retention is therefore an interface requirement, not an optimisation.**
The device retains the register pointer until a write changes it, so the steady
state is: set the pointer to `01h` once at configuration, then issue address-plus-
read forever. Any code path that writes the pointer inside the sample loop moves
the design from 52 % to 86 % and puts the `dropped` gate at risk.

### 4.2 DUT ↔ capture engine (phase bus)

Three lines per node, parallel, Gray-ordered, one masked register write per
transition. The canonical table is `phase_code_map.md` and **that file is right
and the firmware is wrong** wherever they disagree.

The width is derived, not chosen: a single toggle line encodes transitions but
not identity, so one missed edge desynchronises every phase after it, silently,
for the rest of the run. Three bits are self-describing, cost the same single
register write, and stay inside the ESP-NOW send-callback discipline where the
vendor documentation forbids lengthy operations.

**Both questions this section previously left open are now closed** by issue #2,
in `phase_code_map.md` and in
`adr/2026-08-17-phase-code-is-parallel-three-bit.md`. The recommendation offered
here was adopted and then extended, and the extension is worth recording because
it came from a parity argument rather than from a preference.

The cycle is six states, and **both roles use it**, slot for slot:

```
000 sleep -> 001 wake -> 011 encode/receive -> 010 transmit/decode -> 110 ack -> 100 sleep-entry -> 000
      b0         b1              b0                    b2               b1              b2
```

Three things drove it past four states. `adr/2026-08-09-wake-cost-separate-from-frame-cost.md`
already requires sleep entry to be marked in its own right rather than folded
into idle. The sufficiency constraint requires R to acknowledge, so R transmits,
and it follows that S receives that acknowledgement, so the acknowledgement slot
is symmetric rather than R's alone. And five phases **cannot** close at Hamming
distance 1 on any number of bits, since a closed cycle changes every bit an even
number of times and therefore has even length. The sixth state is not padding:
parity is what turned "R needs an ack code" into "both roles need six".

`b2` is resolved as an ordinary phase bit rather than an out-of-cycle flag, which
is what it was under the four-state sequence. The armed and error codes are
therefore named directly, `101` and `111`, since no spare bit identifies them any
more. `b2` must be driven and never floating: floating, it no longer fabricates a
reserved code but a plausible *phase*, with a plausible dwell time, which the
decoder cannot detect.

**One consequence lands on a gate.** Six of eight codes are in the cycle and the
other two are named, so no bit pattern on the bus is meaningless and the `phase`
gate can no longer count invalid codes. It counts invalid transitions instead,
against the six legal one-bit edges plus armed-to-wake plus any-code-to-error.
The criterion in `todos/stage0_todo.md` was edited by the `spec` commit that
closed the choice, before the gate was written or run.

### 4.3 Capture engine → host (wire protocol) — **OPEN, and the largest hole**

The timing budget §5 prices this interface at an 8-byte record (shunt reading,
timestamp, channel tag) at 14,286 records/s, giving 114 kB/s. `stream.c` does not
exist, and two requirements collide inside those 8 bytes.

The `wrap` gate requires monotonic timestamps across the 32-bit microsecond wrap
at ~71.6 minutes, and the timing budget is explicit that the extension belongs in
the capture firmware and **never** on the host, because a wrap reaching the host
unhandled produces timestamps that look plausible and are wrong. An 8-byte record
carrying a 32-bit timestamp has no room for the extension.

Three resolutions, with what each costs:

| Option | Record | Rate | Cost |
|---|---|---|---|
| (a) 32-bit stamp + mandatory in-band epoch record on every wrap | 8 B | 114 kB/s | Host must combine; parser must **refuse** to emit a timestamp before it has seen an epoch record, or the rule is only nominally kept |
| (b) 64-bit stamp in every record | 12 B | 171 kB/s | +50 % bandwidth, no reconstruction anywhere |
| (c) 48-bit stamp (8.9 years at 1 µs) | 10 B | 143 kB/s | +25 %, and a field width nobody will guess wrong |

**Recommendation: (c), and an ADR.** USB CDC carries any of the three with
headroom, so bandwidth is not the deciding axis; the deciding axis is whether a
host-side defect can produce a plausible wrong timestamp, and (b) and (c) make
that impossible by construction while (a) makes it a parser invariant somebody
must maintain. The 114 kB/s figure in the timing budget is stated against a
record format that cannot satisfy the `wrap` gate, and that document should be
corrected rather than this one made to agree with it.

The record must also carry a **type** field, because at least four kinds cross
this wire: shunt sample (per channel), phase transition, epoch, and status or
overflow. Conflating a phase transition with a sample would make a dropped
conversion and a missed phase edge indistinguishable at the host, which is the
one distinction the `dropped` and `phase` gates are separately stated against.

**The transport itself is unbuilt and unpriced.** `register_map.h` has no USB
peripheral and there is no device stack. Under
`adr/…capture-engine-firmware-is-bare-metal.md`, no vendor HAL may be used, so
USB CDC here means a hand-written USB device stack — substantially more firmware
than everything currently in `firmware/capture/` combined. The timing budget's
own fallback (UART at 2 Mbaud, which it prices and then sets aside because "USB
CDC removes the baud rate question") is the cheap path and should be re-costed
against the bare-metal ADR before the USB path is started. See risk R1.

### 4.4 Host artifacts

`results/*.json` per gate, one CSV row per event from `meter_logger`, and
`FINDINGS.md` as the only place a figure is quoted with the file it was read
from. Tests are silent on success and exit 0. Every number in a claim is read
from a named file; nothing is recalled and nothing is retyped.

---

## 5. Budgets

Carried from `docs/hardware-harness-v1/harness_timing_budget.md`, which is the
source of record.

| Quantity | Value | Slack |
|---|---|---|
| Conversion time, both channels | 140 µs | fixed by configuration |
| Conversions/s per channel | 7,143 | — |
| Edge rate, total | < 30,000/s | three orders of magnitude |
| I²C utilisation per bus | 52 % | the binding budget |
| Timestamp resolution | 1 µs | ~100× the 1.4 µs requirement |
| Jitter budget (`jitter` gate) | σ < 2 µs | TIM2 tick gives 2× margin, asserted in `timing_budget.h` |
| Host stream | 114 kB/s | see 4.3 — the record format under this figure is open |
| Shunt full scale | 819 mA vs 330 mA transmit | 2.5× |
| Resolution | 25 µA/LSB; transmit = 13,200 counts | — |
| Deep sleep, 8.14 µA | 0.33 counts | **below one bit; not measured, by declaration** |

Two limits are architecture, not tolerance, and both are stated in advance rather
than discovered:

**Boundary blur.** The INA226 front end is delta-sigma and its output is a window
average, so a conversion straddling a phase boundary blends both phases. Boundary
location carries an intrinsic uncertainty of one conversion time, ~140 µs, which
against a ~1 ms transmit phase is order 10 %. Nothing in the project depends on
locating a boundary more precisely, because every term is identified across runs
— that is the whole content of
`adr/…terms-identified-by-design-not-by-waveform.md`, and any future result that
requires resolving inside an event supersedes that ADR rather than stretching it.

**Deep sleep is a blind spot.** A shunt sized for transmit peaks puts sleep
current below one LSB. It is identical across conditions and cancels in every
comparison the project makes. The consequence is stated plainly: **no absolute
battery-lifetime claim can come from this harness**, and none is made.

---

## 6. Capture engine firmware design

### 6.1 Clock plan, as built

From `timing_budget.h`, asserted at compile time against DocID026289 Rev 4:

```
HSI 16 MHz / PLLM 16 = 1 MHz      (Table 41: 0.95 - 2.10 MHz)
        x PLLN 192   = 192 MHz    (Table 41: 100 - 432 MHz)
        / PLLP 2     = 96 MHz     (Table 41: 24 - 100 MHz)
        / PLLQ 4     = 48 MHz     USB OTG FS, exactly
AHB /1  = HCLK 96 MHz             (§2.2: 100 MHz max)
APB1 /2 = PCLK1 48 MHz            (§2.2: 50 MHz max)
TIM2 kernel = PCLK1 x 2 = 96 MHz, PSC 95 -> 1 MHz tick exactly
```

The interesting number is the one **not** taken. 96 MHz rather than the part's
100 MHz maximum, because PLLP ∈ {2,4,6,8} forces a 100 MHz SYSCLK to a 200 or
400 MHz VCO, and neither divides to 48 MHz. On this part **100 MHz SYSCLK and an
in-spec USB clock are mutually exclusive**, and the Tier 0 claim names USB device
as one of the four required peripherals. Four megahertz buys back a required
peripheral, and the assertion that would have caught the alternative is in the
file.

`timebase.c` sets flash wait states before raising the clock and bus prescalers
before the switch, so no bus is briefly overclocked as SYSCLK steps from 16 to
96 MHz. PLLCFGR is written read-modify-write, because a wholesale write would
zero PLLQ and leave USB unclocked.

### 6.2 The ISR-to-read split

This is the one structural decision the whole instrument rests on, and both the
bare-metal ADR and timing budget §8 identify it as where the `jitter` gate is
won or lost.

```
CNVR rising edge on EXTI
        |
        v
  [ EXTI ISR, highest priority ]        <- must be a handful of instructions
    t = TIM2->CNT                       <- timestamp taken HERE
    enqueue(channel, t, seq)
    return
        |
        v
  [ I2C read, lower priority ]          <- 73 us, whenever the bus is free
    read shunt register (pointer retained)
    pair with queue entry BY SEQUENCE, never "the latest"
        |
        v
  [ stream, lowest priority ]
```

**The pairing rule is not pedantry.** The read is 73 µs against a 140 µs
conversion, so in the nominal case it finishes before the next edge and "latest"
would work. It works right up until it does not, and when it stops working it
mislabels a sample rather than dropping one — a corruption the `dropped` gate
cannot see, because the record count is still right. Queue depth ≥ 2 and explicit
sequence pairing make the failure a detectable drop instead of a silent
mislabel.

**The failure signature is already on record**, and it is a good one: if the read
is left inside the ISR, σ lands near 73 µs — the read duration at 400 kHz with
the pointer retained. A wide histogram is then diagnosable in one glance rather
than by bisection.

**Priorities.** CNVR EXTI above I²C events, I²C events above the host stream. The
stream may be starved briefly; the timestamp may not be delayed at all.

### 6.3 Interrupt allocation, and a constraint the pin map must satisfy

On STM32F4, **EXTI line number equals pin number regardless of port** (RM0383,
via SYSCFG_EXTICR). Two consequences bind the pin allocation that
`harness_spec.md` §4 is supposed to hold:

1. **The ten harness inputs must occupy ten distinct pin numbers.** Not ten free
   pins — ten distinct *numbers* across all ports. PA8 and PB8 cannot both be
   edge sources.
2. **EXTI0 through EXTI4 have dedicated IRQs; EXTI9_5 and EXTI15_10 are shared.**
   The two CNVR lines carry 14,286 edges/s each and are the timing-critical
   inputs; the six phase lines carry ~400/s in total. **The two CNVR lines should
   take two of EXTI0–EXTI4**, so neither ever waits behind the other in a shared
   vector, and the phase lines can share.

A third rule falls out of the phase bus rather than the interrupt controller:
**each node's three phase bits should be contiguous within one GPIO port**, so
the handler latches the whole code with one IDR read, one shift and one mask. A
code assembled from two ports is two reads with a window between them, and a
transition landing in that window produces exactly the invalid code Gray coding
was adopted to make impossible.

Already spoken for and unavailable: PB6, PB7, PA8, PB4 (the two I²C buses),
PA13/PA14 (SWD), and PC13 if the breakout LED is retained. Note also that
configuring PB4 as I2C3_SDA releases NJTRST, so **JTAG is unavailable once
`sensor_bus_init()` runs** — harmless because the ST-Link attaches over SWD, and
recorded in `sensor.h` so it is not rediscovered as a symptom.

**`register_map.h` does not yet contain SYSCFG, EXTI, or NVIC**, nor
`RCC_APB2ENR_SYSCFGEN`. That is correct for Tier 0, which touches no edges, and
it is the first thing Tier 1 adds.

### 6.4 What Tier 1 changes in the existing skeleton

The skeleton is honest about being a skeleton, but two things in it are Tier 0
placeholders that will silently under-deliver if carried forward:

**I²C is configured for 100 kHz standard mode.** `timing_budget.h` sets
`I2C_TARGET_SCL_HZ` to 100000 with `CCR = 240`, `TRISE = 49`. The whole harness
budget is stated at 400 kHz. At 100 kHz a pointer-retained 16-bit read is ~290 µs
against a 140 µs conversion — **207 % utilisation**, which drops roughly every
other conversion on both channels. This is not a defect today: `sensor.h` states
that the INA226 layer is Tier 1 work and Tier 0 only needs both peripherals to
initialise cleanly. It becomes a defect the moment the `rate` gate runs against
it, and the failure would present as a sensor or driver problem rather than as a
clock configuration. The move to fast mode also needs `I2C_CCR_FS` (bit 15) and
`I2C_CCR_DUTY` (bit 14) added to `register_map.h`, neither of which is defined.

**`startup.s` has a four-entry vector table.** Reset, NMI, HardFault and the
stack pointer. It says so, and says the table must be extended before any
EXTI-based capture interrupt is wired. Worth pairing with a default handler that
traps rather than falling through, so an unconfigured vector is a stop rather
than a wander.

Both of these are the skeleton behaving as designed. They are listed because the
gap between "correct at Tier 0" and "correct at Tier 1" is exactly where a paper
gate's authority runs out.

---

## 7. Open items, ranked by the cost of finding out late

The ordering is the project's own rule: cost of discovering a failure late, which
is the work a failed gate invalidates behind it.

**O1 — Does the Conversion Ready alert actually self-clear in transparent mode?**
The design halves the edge rate and drops one bus transaction per conversion by
setting LEN = 0, on the reading that the flag self-clears. SBOS547B does not
settle this in one place. §7.1.7 says the Alert Latch Enable bit in Transparent
mode resets the pin "when the fault has been cleared"; the same section says the
Conversion Ready Flag (CVRF, bit 3) clears under exactly two conditions —
writing the Configuration Register, or **reading the Mask/Enable Register**.
For the CNVR alert function those two statements point in different directions,
and the design depends on which governs.

If the pessimistic reading holds, the Alert pin asserts once and stays asserted,
and **every conversion after the first is untimestamped while the stream still
looks well-formed**. The remedy is a Mask/Enable read per conversion, which
breaks pointer retention as well as adding a transaction: roughly 96 clocks,
240 µs at 400 kHz against a 140 µs conversion, which fails outright and forces
high-speed mode at 2.94 MHz (~33 µs, 23 %) — the fallback the timing budget
already names.

This is the cheapest thing on this list to test and the most expensive to
discover late, because it invalidates the bus-count decision, and the bus count
is what the PCB is laid out around. It is caught by the `rate` gate. Its
prediction should be written into the issue **before** that gate runs: the
optimistic reading gives 7,143 edges/s, the pessimistic reading gives *one*.

**O2 — The wire protocol and its transport (4.3).** Record format cannot satisfy
the `wrap` gate as priced, and the USB CDC transport is unwritten, unpriced, and
constrained by the bare-metal ADR to be hand-written. Blocks `stream`, and
`stream` blocks every Tier 1 gate that reads a capture.

**O3. `harness_spec.md` does not exist.** Still open, but it no longer blocks
`phase_code_map.md`. That file's exit criterion cited §4 for pin allocation and
could not close without a document nobody had written; issue #2 split the
dependency, on the grounds that a code table is not made correct by a pin map and
that holding a finished table open behind an unwritten one reports a closed
decision as open. What remains here is the pin allocation itself. Section 6.3
above states the constraints; it does not make the allocation, which is a
decision. One constraint is now tighter than when 6.3 was written: under the
closed six-state cycle **all three phase bits toggle once per event in each
direction**, so `b2` is a timing-relevant edge source rather than a static level,
and a pin map drawn against the old four-state assumption would have
under-specified it.

**O4. The three Phase A implementation choices. Closed** by issue #2, 2026-08-17.
Capture is free-running (`adr/2026-08-17-capture-is-free-running.md`), the phase
code is three parallel bits per node
(`adr/2026-08-17-phase-code-is-parallel-three-bit.md`), and the bench sensor is
the bare INA226AIDGSR in VSSOP-10, LCSC C49851, with no breakout at any tier.

The trap attached to the third is retained here, corrected, because it prices a
requirement rather than merely warning about a part. **The figure previously given
in this section, 165 µV, was wrong by a factor of four:** 0.002 Ω against 330 mA
is 660 µV, not 165 µV, of an 81.92 mV span. The stronger form of the argument is
not in microvolts anyway. At 0.002 Ω one LSB is 2.5 µV / 0.002 Ω = **1.25 mA**,
so the 97 mA receive level is 78 counts and quantisation alone contributes 1.3 %
per count, against the `gain` gate's 0.5 % criterion. Such a breakout fails that
gate arithmetically, whatever calibration is applied to it. Buying the bare part
removes the question rather than answering it: there is no inherited shunt, so no
product listing is relied on, and the 0.1 Ω the timing budget fixed is a part this
project fits.

The consequence is priced in `todos/stage0_todo.md`: VSSOP-10 is 0.5 mm pitch and
cannot be breadboarded, so Tier 1 and Tier 2 need a hand-assembled sensor carrier
holding the same Kelvin sense requirement as the fabricated board.

**O5 — Periodic bus-voltage conversion is not free.** The design monitors the
supply assumption with roughly one bus conversion in a hundred, costed as one
percent of the conversion budget. But switching between shunt-only and
shunt-and-bus is a Configuration Register write, and SBOS547B §7.1.1 states that
writing the Configuration Register **halts any conversion in progress** and
restarts on completion, and clears CVRF. So the true cost is a perturbed cadence
plus a cleared ready flag at every switch, not one conversion slot. The `supply`
gate should be stated against that, and the alternative — a third INA226 on the
supply, or a periodic dedicated window — costed before the board is laid out.

**O6 — ESP-IDF and toolchain pins are open.** `.mise.toml` names ESP-IDF v5.x
with no patch level, and `arm-none-eabi-gcc` is unpinned because it is not
installed on this machine. The file is candid that pinning an unrun toolchain
would record a claim about something that has never run. Both close at first
use, and the ESP-IDF pin is what turns the Stage -1 version declaration into
something enforced rather than asserted.

**O7. Two discrepancies in the record itself. Closed**, 2026-08-17. Both misled a
reader who landed on one file rather than the set, which is the failure mode this
section exists to catch, so both are recorded here rather than deleted.

`contracts/stage_minus1_contract.md` cited "Stage 0 item C3" for the ESP-NOW
version check, and the tier ordering in `todos/stage0_todo.md` puts the third
item of Tier 2 elsewhere. It now names the `link` gate. A gate cited by position
acquires a second name the moment a gate is inserted above it; cited by handle it
cannot. `docs/sop/issue_sop.md` had carried this as a discrepancy to reconcile
and now carries it as the worked example of the rule.

`adr/2026-08-09-capture-engine-is-stm32-part-still-open.md` carried
**Status: Accepted** though both clauses making its title true were gone: the
part ADR declares "Supersedes:" against it and the bare-metal ADR declares
"Amends:" against its toolchain clause. Its status line now points forward to
both successors and names which parts of its body no longer hold. The Decision
section was not touched, per the rule in `git_sop.md` that an edited decision
destroys the evidence that the project once believed otherwise.

---

## 8. Risks

**R1 — USB CDC under the bare-metal ADR.** The ADR was written about the
jitter-critical path, where its argument is strong: the ISR-to-read boundary must
be this project's own code. Applied to a USB device stack it is the same rule
governing code with no bearing on the timebase, at a cost plausibly exceeding the
rest of the capture firmware. Two exits exist without weakening the ADR: take the
UART path the timing budget already priced at 2 Mbaud, or amend the ADR to scope
"no HAL" to the measurement path and admit a stack for transport. Either is
cheaper than discovering the cost mid-Tier-1. This should become an ADR before
`stream.c` is started, not after.

**R2 — The instrument is validated by the same hand that built it.** Named in
the thinkbook and mitigated by construction: Stage 0's references are a precision
resistor, an independent voltmeter, and the DUT's own clock — standards, not
instruments. The question is asked of every gate in writing and answered in
writing: *what would make this check pass while comparing the wrong quantity?*
Only the `negctl` gate can detect a physically wrong setup, because a capture
written from the wrong rail is well-formed.

**R3 — Concentration on one person.** Named in
`adr/…two-channel-harness-built-in-house.md` as a real exposure, and it is the
argument for scoping v1 as narrowly as it is scoped.

**R4 — Scope creep from timebase into front end.** Same ADR. Any ranging feature
arriving before Stage 2 data asks for it is that entry being violated. Likewise,
ten identical boards are spares; ten variants are thrash.

**R5 — H1 weakening under its own control.** If the `k`-per-wake regression shows
`E_wake` dominating across the whole payload range, H1 is in trouble even with a
visible sawtooth. That belongs in the predictions ledger rather than a footnote,
and it is already prediction 2 there.

---

## 9. What would reopen this document

The modality, the latent width, or the ESP-NOW version changing, since Stage -1's
Question 1 reopens with any of them and the frame arithmetic is upstream of the
whole architecture.

The capture engine part changing, which would reopen the clock plan, the pin
constraints of 6.3, and the shunt sizing that is pinned to the ESP32-S3
transmit figure.

Any result that can only be obtained by resolving a boundary *inside* an event,
which supersedes `adr/…terms-identified-by-design-not-by-waveform.md` and, with
it, section 5's acceptance of a 140 µs boundary blur.

O1 resolving pessimistically, which reopens the bus count and therefore the
board.

---

## Appendix A: file map

| Path | What it holds | State |
|---|---|---|
| `docs/dsc_first_principle.md` | The thinkbook: gaps, hypotheses, cost model, stages | Complete |
| `docs/dsc_hld.md` | This document | Draft |
| `contracts/stage_minus1_contract.md` | Five questions, answered from declared constants | Closed |
| `todos/stage0_todo.md` | Gates: claim, command, criterion, prediction | Complete |
| `docs/adr/` | Thirteen entries; one marked superseded, one superseded but unmarked (O7) | Live |
| `docs/hardware-harness-v1/harness_timing_budget.md` | The arithmetic the harness is sized by | Complete |
| `docs/hardware-harness-v1/phase_code_map.md` | Code-to-phase table, both roles, six states each | **Closed**, proved by `phase` in Tier 2 |
| `docs/hardware-harness-v1/harness_spec.md` | Pin allocation, §4 | **Does not exist** (O3) |
| `harness/firmware/capture/` | Bare-metal capture engine | Tier 0 subset built |
| `harness/firmware/dut/` | One image per role and per ablation | Empty, Tier 2 |
| `harness/scripts/`, `harness/tests/` | Capture (may prompt) and test (may not) | `toolchain`, `blink` only |
| `harness/results/` | One JSON per gate | `stage0_toolchain.json` |
| `harness/FINDINGS.md` | Every figure, naming the file it was read from | Does not exist yet |

## Appendix B: symbols

| Symbol | Meaning |
|---|---|
| `p`, `L`, `n(p)` | Payload bytes; single-frame limit (250 B under v1.0); frame count `⌈p/L⌉` |
| `E_wake`, `E_pkt`, `e_byte` | Per-event, per-frame, per-byte energy on S; primed forms are R's |
| `E_enc`, `E_proc`, `E_use`, `E_proj` | Encode on S; full raw processing on R; latent consumption on R; bridge on R |
| `C_A`, `C_B`, `C_null` | Raw-transfer cost; semantic-transfer cost; empty-event floor |
| `G` | `(C_A − C_B) / (C_A − C_null)`, the fraction of achievable range recovered |
| `Q(·)`, `ε` | Task metric on R, and the declared sufficiency tolerance |
| H1, H_ledger, H_transfer | Packet-count dominance; joint encoder-decoder split; independently trained halves bridged |
