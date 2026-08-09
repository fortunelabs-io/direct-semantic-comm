# Direct Semantic Communication on Constrained Hardware: A Thinkbook

*Notes toward a two-node, two-sided, physically instrumented test of semantic payload transfer, written before any board has been flashed. Every number in this document that was not measured by this project is marked as borrowed. Borrowed numbers are expectations, not findings.*

---

## Preface

The parent build asked whether semantic enrichment via KV-cache transfer could be reproduced and fully understood from scratch, using models smaller than the paper's. It answered that question and produced a ledger: producing an enriched state is computation, moving it is communication, using it is computation again. It priced that ledger on a development machine and found the two sides pointing in opposite directions — compute favored the exchange, communication opposed it by four to five orders of magnitude — with no way to adjudicate between them, because on that machine communication cost nothing.

What a development machine can produce is not a verdict but a threshold: the link speed above which the trade turns favorable, stated against both prompt and response length. That threshold is the last of the parent build's open items, recorded there as *the ledger reopened on constrained hardware, where the payload figure stops being free and the break-even table stops being arithmetic*.

This document is that item. It is not a derivative project. It is the only measurement that can adjudicate a question the parent build was structurally unable to answer.

The subject is no longer the KV-cache. The KV-cache does not fit on a microcontroller and neither does the machinery around it. What carries over is the shape of the trade the KV-cache experiment instantiated: whether transferring a representation that has already been understood is cheaper, in measured joules and milliseconds, than transferring raw data and forcing the receiver to understand it again.

One discipline carries over unchanged. No claim enters this document without either a local measurement or a citable source, and the two are never confused. The hardware harness described here is the physical analog of a capture script: until it runs on this project's own boards, every figure below is literature.

A second discipline carries over from the parent build's own failures. Predictions are recorded before the run that adjudicates them, in Section 7. Readings that measurement overturns are kept visible, not quietly corrected, because a claim that was corrected is worth as much as one that held.

---

## 1. Problem Statement

Two microcontrollers share a low-power radio link. Node S has produced some internal state — a reading, a signal, a partial computation — sitting in its own SRAM. Node R needs to act on what that state means. Getting meaning from S's SRAM into R's decision requires one physical act: serialize the state into a payload, hand it to the radio, let the radio cut it into frames small enough to fit the link's hardware limit, transmit those frames one at a time, and let R reassemble and interpret them.

### 1.1 Why there are exactly three options

The choice of what to serialize is not a matter of taste, and it is not specific to microcontrollers. It is the composition question, and it has a known shape.

Combining the strengths of two systems requires choosing *where in the pipeline* the combination happens, and two classical answers occupy the endpoints. One communicates before computation, at the level of inputs or parameters: cheap to move, but the receiver must do all the work, and any work the sender already did is discarded. The other communicates after computation, at the level of outputs: minimal to move, but every contribution except the final one is destroyed at the source. Both endpoints are taken. Neither considers the middle.

Mapped onto the link, those endpoints are the first two options and the middle is the third:

1. S serializes its raw observation, unprocessed. R receives it and computes the result itself, from zero.
2. S computes the result fully, then serializes only the final decision, a single value or label.
3. S serializes an intermediate representation, distilled from the raw observation but not yet collapsed into a final decision.

The middle option is only viable if three things hold: there is an intermediate state rich enough to be worth transferring, that state is detachable from the process that produced it, and the receiver can interpret it despite not having produced it. A learned encoder satisfies the first two by construction — a bottleneck activation is fully formed at a natural boundary and can be serialized. The third is the whole difficulty, and Section 3 splits it out as a separate hypothesis rather than assuming it away.

### 1.2 Why the middle is forced here rather than merely preferred

On a server the choice is unforced; both compute and bandwidth are cheap enough that any of the three works adequately. On an MCU pair two constraints bind at once and cannot be traded against each other in the usual way. First, the radio has a hard per-frame payload ceiling, and every frame, however small its contents, carries a fixed cost. Second, both S and R run on a milliamp-scale energy budget with no slack to spend on work that does not strictly have to be done.

Option 1 fails this pair in a specific way. Raw data is usually the largest of the three payloads, so it pays the most in frame overhead, and R still has to spend its own scarce energy reprocessing data that S may already have touched once. It is a double tax, paid on both ends of the same constrained link.

Option 2 fails differently, and the closest precedent concedes the failure in its own limitations section: processing on the device rather than sending means the raw data is lost for any later review, verification, or reanalysis, and where those are required the approach is unsuitable. That is not an outside criticism. It is the strongest existing measurement of Option 2 naming what Option 2 costs.

Option 3 is the only point in this design space where the cost paid at S, the cost paid on the link, and the cost paid at R can all be tuned together against the same two binding constraints, rather than relieving one by making the other worse.

### 1.3 The question

> On two ESP32 nodes linked by ESP-NOW, does a learned intermediate representation actually clear both binding constraints at once, in measured joules and milliseconds, once the sender's encode cost and the receiver's decode cost are both charged against the frames it saves?

Everything below serves that question.

---

## 2. The Gaps

Each gap is stated with the closest existing work and what that work stops short of. Where an earlier draft of this document overstated a gap, the correction is kept visible rather than silently applied.

**Gap 1: No two-sided ledger.** The strongest precedent, Karic et al., "Send Less, Save More" (arXiv:2510.24829, ESP32-S3, PPK2 with GPIO phase markers), measures energy only on the sensing node. Its own text concedes that in the cloud-inference scenario only the IoT node is metered. The receiver's cost of consuming raw data versus consuming a distilled result is never measured anywhere in that line of work. The core claim of semantic communication is precisely a claim about the *sum* of both sides; a one-sided measurement cannot confirm or refute it.

**Gap 2: No representation between raw and decision, with both sides metered.** Karic et al. transmit a class index: the final decision, Option 2 above. Split-inference work on MCUs does transfer intermediate activations between MCUs, and the intermittent-network study (arXiv:2512.24179) does so over ESP-NOW specifically. Nobody has transferred a learned intermediate representation between two MCUs while metering both nodes against a stated cost model.

*Correction to an earlier reading.* An earlier draft asserted that the split-inference line does not meter the radio at all. That is wrong for arXiv:2512.24179, whose Section 5.3 is a transmission energy profile per layer output over ESP-NOW. The gap survives, but only in the narrower form stated above.

**Gap 3: No fragmentation curve.** Five discrete points exist, in the profiling appendix of arXiv:2512.24179, spanning payload sizes that straddle the ESP-NOW v1.0 single-packet boundary. They do not constitute a curve, and they cannot be used as one. There are no repetitions and no confidence bands; the sequence is non-monotonic in payload size with no explanation offered; two of the five rows fail an internal consistency check, in that the reported duration multiplied by the reported average power does not reproduce the reported total energy; and the paper's own body text states a full-inference compute figure that differs from the sum of its layer-wise table by three orders of magnitude. No published source provides a continuous, repeated, confidence-banded curve of energy per packet, or energy per byte, versus payload size across that boundary. The step structure at the fragmentation boundary, which is where the packet-count argument for semantic compression lives or dies, has never been drawn from measurement.

**Gap 4: No protocol match.** The LPWAN protocols in the closest precedent (LoRaWAN, NB-IoT, LTE-M) carry connection-establishment costs that dominate their energy profiles and have no analog in ESP-NOW. In that precedent's own measurements, network connection is the largest single term for the cellular protocols, by a wide margin over the transmission itself. Their numbers do not transfer, and the term that dominates them does not exist here. ESP-NOW's own energy behavior under payload sweep is unmeasured to the standard Gap 3 describes.

**Gap 5: No cross-geometry bridging test on MCU pairs.** Every embedded precedent found here uses one designer's model split across devices, with encoder and decoder always optimized together. None tests the hard problem: a representation produced by one independently trained system, made usable by a second independently trained system through a small learned bridge, with both original systems frozen. Gaps 1 through 4 are closed by testing one jointly optimized system split across two devices. Gap 5 is closed only by a separate, later test where the two halves are trained independently and bridged, a deliberately narrower and later target than the first four.

**Gap 6: The compute cost of semantics is named but never priced.** The generalized semantic communication framework (Qin et al.) tabulates a cost row for semantic processing — sensing hardware, storage, GPU — alongside its architectural framework. The theoretical survey (Xin, Fan and Letaief, *Entropy* 26(2):102) lists scheduling and energy optimization as an open challenge of semantic communication systems in its own words. The field names the computational price of semantics as a category and, so far as this search found, never pays it in joules on hardware where it binds. That is the space this project occupies, and it is a stronger positioning claim than novelty of mechanism.

A search across the semantic communication literature (Tsinghua, SJTU, XJTU groups), the split-computing literature, and the embedded ML literature found no work occupying the intersection of these gaps. The claim is not "this has never been done anywhere"; it is "this has not been found after a deliberate multi-tradition search." The distinction matters and is kept.

---

## 3. The Proposed Solution

The proposed solution treats Option 3 from Section 1 as two separate hypotheses, tested in sequence, never merged under one label.

**H_ledger** proposes splitting one jointly optimized encoder-decoder system across two physical devices. Encoder and decoder share one training objective by construction, so there is no representation-alignment problem to solve here, only a hardware cost to price.

- *Question*: does splitting one jointly trained encoder-decoder system across two physical nodes cost less, energy and latency on both sides, than sending raw data and having the receiver reprocess it, at equivalent delivered information?
- *Expected result*: H_ledger holds if $C_B < C_A$ per the inequality in 4.3, and $Q(B) \geq Q(A) - \varepsilon$ holds per sample under the sufficiency and collapse checks in 4.5. The boundary is not the sign alone; it is whether the measured margin matches the inequality's predicted direction and magnitude, reported as a fraction of the achievable range per 4.4.

**H_transfer** proposes the harder mechanism: a representation produced by one system that was never trained with the consumer in mind, made usable by a second, independent system through a small learned bridge. This is the variant with the alignment problem in it.

- *Question*: can a representation from an independently trained, frozen Encoder_S be made usable by an independently trained, frozen Decoder_R through a small trained Projector, at a bridging cost ($E_{\mathrm{proj}}$) that still leaves the total ledger favorable against raw transfer?
- *Expected result*: H_transfer holds if $C_{\mathrm{transfer}} < C_A$ per the inequality in 4.7 and the sufficiency and collapse checks pass. Because $E_{\mathrm{proj}} \geq 0$, this boundary is strictly tighter than H_ledger's; a small negative margin here is a meaningful failure in a way it would not be for H_ledger.

H_ledger is tested first, in Stages −1 through 4. H_transfer is tested second, in Stage 5, once the physical ledger itself is trusted. A result for H_ledger is not a result for H_transfer; the two are reported separately throughout.

One experiment, two conditions, four measured phases, both nodes metered.

**Condition A (raw transfer).** S transmits the raw payload. R receives it and runs the full processing step locally. S pays no encode cost. The link pays for every raw byte, fragmented as needed. R pays full processing.

**Condition B (semantic transfer, H_ledger variant).** S runs a small learned encoder over the raw payload and transmits the latent representation. R runs the matching decoder, or consumes the latent directly for the task. S pays encode. The link pays only for the latent. R pays a reduced use cost. Encoder and decoder are trained together as one system and only split at deployment, so there is no representation-alignment problem here by construction.

### 3.1 The latent width is a design variable, and that is the premise of the whole argument

At the model tier the payload size is not a choice. A KV-cache is a consequence of architecture — layer count, head count, head dimension, precision — and cannot be tuned below any threshold. That is why the communication side of the parent build's ledger lost by four to five orders of magnitude with nothing available to do about it.

At this tier the bottleneck width is free. The latent can be sized, quantized, and layer-selected to land wherever the designer puts it, including deliberately just under the frame limit $L$.

This is not a convenience. It is the premise on which the packet-count argument rests. The sawtooth in energy per byte is only *actionable* because there exists a design variable that can be placed on the cheap side of a discontinuity. Without that freedom the sawtooth would be an observation about a curve; with it, the sawtooth is a design target. Section 4.3 states the win condition in those terms.

### 3.2 R holds no local observation, and the replacement lesson does not transfer intact

In the parent build the consumer had its own reading of the same input, and the mechanism that worked added to it while every mechanism that substituted for it collapsed — twice, under two objectives, on two corpora, onto two different constant outputs. The lesson recorded there is that transferred understanding must complement and never overwrite.

That lesson does not transfer here in its stated form, and assuming it does would misdesign this experiment. The collapse observed there is destructive interference, and it requires the consumer to possess prior competence for the incoming representation to destroy. In Condition B and in Stage 5, R never observes the raw phenomenon. It has no competing reading to be overwritten, so there is nothing for a residual to preserve and substitution destroys nothing.

What does transfer, fully, is the *failure signature*: an output that collapses to a constant while an aggregate metric reads the collapse as partial success. That is not a property of substitution. It is a property of aggregate reporting, and it is guarded against in 4.5 rather than designed around here.

Giving R a cheap local observation would make this experiment a structural analog of the parent build's fused arm rather than its replacing arm, and that remains an interesting variant. It is not a correction, and it is not adopted in the core runs, because it changes the question from *what does a transferred representation cost* to *what does a transferred representation add*.

### 3.3 The encoder

The learned encoder is deliberately modest: an int8-quantized autoencoder of the kind already demonstrated on ESP32-class hardware. Novelty is not sought in the model. It is sought in the ledger.

Its energy is budgeted by measurement and never by operation count. The closest precedent measured a network with more floating-point operations consuming less energy than one with fewer, on this chip family, which is sufficient to retire the practice of estimating $E_{\mathrm{enc}}$ from FLOPs anywhere in this document.

### 3.4 Build discipline

Unix discipline governs the build. Each firmware and host component does one thing:

- `payload_gen`: produces payloads of a commanded size and content class. Nothing else.
- `fragmenter`: chunks a payload into ESP-NOW frames with one sequence byte, reassembles on the far side. Nothing else. Hand-rolled, so its compute cost is visible and chargeable, not hidden in a vendor component.
- `phase_marker`: toggles one GPIO per phase transition. Nothing else.
- `tx_role` / `rx_role`: two firmware images, one role each. No dual-mode switches.
- `meter_logger` (host side): reads both INA226 traces and the GPIO edges, emits one CSV row per event. Text output, because text composes.
- `analyze` (host side): consumes CSV, emits statistics and plots. Never touches hardware.

Components communicate through flat files and serial text. Any single component can be replaced or audited without touching the others.

---

## 4. The Mathematics and the Architecture

### 4.1 Cost model

Let $p$ be payload size in bytes and $L$ the single-frame limit (250 B under ESP-NOW v1.0, 1470 B under v2.0 from ESP-IDF 5.4). Packet count is a ceiling function:

$$n(p) = \left\lceil \frac{p}{L} \right\rceil$$

Transmit-side energy is modeled as three terms, not two:

$$E_{\mathrm{tx}}(p) = E_{\mathrm{wake}} + n(p)\cdot E_{\mathrm{pkt}} + p \cdot e_{\mathrm{byte}}$$

$E_{\mathrm{wake}}$ is everything paid once per *event* regardless of how many frames it takes: leaving sleep, bringing up the radio, the active window the MCU holds open, re-entering sleep. $E_{\mathrm{pkt}}$ is everything paid once per *frame*: preamble, MAC header, the send callback. $e_{\mathrm{byte}}$ is the marginal air-time cost per payload byte at the configured PHY rate.

The receive side gets the mirrored model with its own constants, since RX listening and buffer handling need not mirror TX:

$$E_{\mathrm{rx}}(p) = E'_{\mathrm{wake}} + n(p)\cdot E'_{\mathrm{pkt}} + p \cdot e'_{\mathrm{byte}}$$

Nothing forces the primed constants to equal the unprimed ones. Whether they differ, and by how much, is itself a measurement target.

**Why the wake term is separated.** An earlier version of this model folded the per-event cost into the per-frame term, and it could not have detected the difference, because a design in which every event is one wake confounds them exactly. The separation matters for two reasons, and only the second is obvious.

The first is diagnostic: the only existing ESP-NOW energy data is consistent with a large fixed term of roughly the size of an active window multiplied by an idle current, which would mean the dominant cost is *how long the MCU stays awake* rather than *how many frames it sends*. If that reading is right, compute time and radio time compete for the same term and the packet-count argument weakens sharply. Stage 2 separates them by sending $k$ payloads inside one wake window: $E_{\mathrm{wake}}$ appears once and $E_{\mathrm{pkt}}$ scales with $k$.

The second is structural, and it is the reason this separation is a decision rather than a refinement. The wake terms appear identically in both conditions and therefore *cancel in the difference*. They do not cancel in the range the difference is measured against. A term that the sign of the result normalizes away can still determine how much the result is worth, and reporting only the sign hides exactly that. Section 4.4 makes the range explicit.

Total cost per delivered observation, both conditions:

$$C_A = E_{\mathrm{tx}}(p_{\mathrm{raw}}) + E_{\mathrm{rx}}(p_{\mathrm{raw}}) + E_{\mathrm{proc}}$$

$$C_B = E_{\mathrm{enc}} + E_{\mathrm{tx}}(p_{\mathrm{lat}}) + E_{\mathrm{rx}}(p_{\mathrm{lat}}) + E_{\mathrm{use}}$$

where $E_{\mathrm{proc}}$ is R's full processing of raw data, $E_{\mathrm{enc}}$ is S's encoder pass, and $E_{\mathrm{use}}$ is R's consumption of the latent. The same structure holds with $T$ substituted for $E$ throughout; energy and latency are two ledgers over one event stream.

### 4.2 Every energy figure is reported decomposed

No total is reported without its decomposition into the part that scales with payload and the part that does not, and the two must sum to the measured total with the residue stated explicitly. A residue that is not near zero is a modeling error, and it is found here or it is found nowhere.

This is the physical form of a discipline the parent build arrived at one tier up, where a falling total loss coexisted with a rise in the only component that could change an answer. A total that improves is not evidence that the mechanism under test did anything.

### 4.3 The claim under test

Semantic transfer wins if and only if:

$$E_{\mathrm{enc}} + E_{\mathrm{use}} - E_{\mathrm{proc}} \;<\; \left[ n(p_{\mathrm{raw}}) - n(p_{\mathrm{lat}}) \right]\left( E_{\mathrm{pkt}} + E'_{\mathrm{pkt}} \right) + \left( p_{\mathrm{raw}} - p_{\mathrm{lat}} \right)\left( e_{\mathrm{byte}} + e'_{\mathrm{byte}} \right)$$

The wake terms have cancelled. Left side: the net compute premium of the semantic path. Right side: the communication savings, split into a packet-count term and a byte-count term.

The central hypothesis, H1, is that on a low-power connectionless radio the packet-count term dominates the byte-count term, so the win condition is governed by whether the latent fits in fewer frames, not merely fewer bytes. Under H1, $E(p)/p$ is sawtooth-shaped: falling within each frame, jumping at each multiple of $L$.

H1 is only *actionable* under the premise of 3.1. If $n(p_{\mathrm{raw}}) = n(p_{\mathrm{lat}})$ the packet-count term is identically zero and the entire argument reduces to the byte-count term, which is the weaker claim H1 exists to strengthen. Confirming that the packet-count term is non-zero for the chosen sensor modality and bottleneck width is arithmetic on datasheet constants, costs nothing, and is therefore Stage −1 rather than a discovery.

H0 is the flat alternative: energy per byte constant or rising in $p$, which would collapse the packet-count argument entirely and must be excluded first.

### 4.4 The null, and what a result is reported against

An energy saving reported as a fraction of $C_A$ is uninterpretable, because it does not say how much saving was available.

Define the null as the cost of a delivered event carrying a one-byte payload with no compute charged on either side:

$$C_{\mathrm{null}} = E_{\mathrm{tx}}(1) + E_{\mathrm{rx}}(1)$$

This is the floor. No representation, however compact, can cost less than an empty event. The reported quantity is therefore the fraction of the available range that the semantic path recovers:

$$G = \frac{C_A - C_B}{C_A - C_{\mathrm{null}}}$$

$G = 1$ means the semantic path costs as little as sending nothing. $G \leq 0$ means no gain. The denominator is measured, not assumed, and it is measured at Stage 1, before any encoder exists.

The reason this is a gate rather than a presentation choice: if $C_{\mathrm{null}}$ turns out to be most of $C_A$, then the entire achievable range is small, and a perfect encoder saves whatever is left. That is a verdict on the direction, and it costs one cheap run to obtain. Discovering it at Stage 3, after an encoder has been trained and deployed, would be discovering it at roughly a hundred times the price.

Without measuring the null first, a favorable difference has no reference and cannot be distinguished from a difference that was always going to be small.

### 4.5 Sufficiency, and the collapse check

Let $Q(\text{condition})$ be the task metric computed on R. The efficiency comparison is admissible only where:

$$Q(B) \geq Q(A) - \varepsilon$$

with $\varepsilon$ declared before any run. Without this constraint the experiment degenerates into "sending less costs less," which is true, trivial, and not the question.

The aggregate metric is not sufficient on its own. A decoder that has collapsed onto a constant output scores the base rate of whatever it settled on, and against a loose $\varepsilon$ that base rate can pass. The parent build produced exactly this: a figure that was read as a degraded accuracy and was in fact the frequency of one answer in a system that had stopped reading the question. Only per-sample records separated the two; the aggregate could not.

Two things are therefore required alongside $Q$, and no efficiency number is admitted without them:

- **Per-sample records**, written for every event, in both conditions, on identical inputs.
- **The output distribution of R** under both conditions, and its total variation distance from the reference label distribution. A collapse is visible here and invisible in $Q$.

### 4.6 System architecture

```
Node S (ESP32)                          Node R (ESP32)
+--------------------+                  +--------------------+
| payload_gen        |                  |                    |
| [encoder: B only]  |                  | fragmenter (rx)    |
| fragmenter (tx)    |   ESP-NOW        | [decoder: B only]  |
| esp_now_send       | ===============> | [processor: A only]|
| phase_marker GPIO  |   n(p) frames    | phase_marker GPIO  |
+---------+----------+                  +---------+----------+
          |                                       |
    INA226 on 3.3V rail                    INA226 on 3.3V rail
          |                                       |
          +------------> meter_logger <-----------+
                              |
                          CSV rows
                              |
                          analyze
```

Phase boundaries on S: sleep → wake → encode → transmit → sleep. On R: sleep → wake → receive → decode/process → sleep. The wake and sleep transitions are marked as their own phases rather than folded into idle, because 4.1 charges them their own term. Each transition is one GPIO edge. The GPIO trace time-aligns the current trace, so each phase's energy is an integral over an interval bounded by edges, per node, per event. No shared clock between nodes is required: one-way latency is taken from the meter side, which sees both nodes' edges on one timebase, and cross-checked against RTT measured on S alone.

Operating constraints fixed for all runs: pure ESP-NOW with no AP association, Wi-Fi power save off, single fixed channel, fixed board positions and orientation, unencrypted frames in the core runs.

### 4.7 The transfer variant (H_transfer)

Condition B's model has one encode term and one use term because the encoder was built to be read by that exact decoder. H_transfer removes that assumption. Encoder_S and Decoder_R are trained independently, frozen afterward, and a small Projector is the only component allowed to adapt across the boundary. This physically mirrors the training protocol the parent build reconstructed and reproduced: both endpoints frozen, only the bridge trained.

```
Node S (ESP32)                          Node R (ESP32)
+--------------------+                  +--------------------+
| Encoder_S [frozen] |   ESP-NOW        | fragmenter (rx)    |
| fragmenter (tx)    | ===============> | Projector [trained]|
| phase_marker GPIO  |   n(p) frames    | Decoder_R [frozen] |
+---------+----------+                  | phase_marker GPIO  |
          |                             +---------+----------+
    INA226 on 3.3V rail                    INA226 on 3.3V rail
```

$$C_{\mathrm{transfer}} = E_{\mathrm{enc}}^{\mathrm{frozen}} + E_{\mathrm{tx}}(p_{\mathrm{lat}}) + E_{\mathrm{rx}}(p_{\mathrm{lat}}) + E_{\mathrm{proj}} + E_{\mathrm{use}}^{\mathrm{frozen}}$$

$E_{\mathrm{proj}}$ is charged to R because the Projector executes on R. It is charged there for that reason and no other; an earlier draft justified the placement by appeal to the receiver's local context, which R does not have in this design, per 3.2.

The win condition against Condition A's raw baseline becomes:

$$E_{\mathrm{enc}}^{\mathrm{frozen}} + E_{\mathrm{proj}} + E_{\mathrm{use}}^{\mathrm{frozen}} - E_{\mathrm{proc}} \;<\; \left[ n(p_{\mathrm{raw}}) - n(p_{\mathrm{lat}}) \right]\left( E_{\mathrm{pkt}} + E'_{\mathrm{pkt}} \right) + \left( p_{\mathrm{raw}} - p_{\mathrm{lat}} \right)\left( e_{\mathrm{byte}} + e'_{\mathrm{byte}} \right)$$

The right side is unchanged, since the communication savings do not care who trained what. The left side gains one non-negative term that H_ledger's inequality never had to pay. Because $E_{\mathrm{proj}} \geq 0$, H_transfer's break-even bar is never easier to clear than H_ledger's. Whatever margin Stage 3 records for H_ledger is an upper bound on H_transfer's margin, not a proof of it, and the two numbers must not be quoted interchangeably.

One structural note carried down from the parent build. If Encoder_S emits a latent that is much narrower than what Decoder_R expects, the Projector should widen it before combination rather than be expected to compensate inside the combination. The parent build found the producer occupying roughly a ninth of the channels entering the bridge purely as a consequence of architecture, before any learning, and the source's own stronger variant equalizes the widths first. That is a configuration fact, readable before training, and it belongs in Stage −1.

The sufficiency and collapse constraints of 4.5 carry over unchanged in form but are a strictly harder test in practice, since $Q$ is now computed through a decoder that never saw this encoder during its own training.

---

## 5. Proof Steps

Each stage is named by the condition it can kill and ordered by the cost of killing it — not by the order components appear in the system. The last column is the one that makes the ordering load-bearing: it states what becomes void if that stage fails after later stages have already been paid for.

| Stage | What it can kill | Cost | Void if it fails late |
|---|---|---|---|
| −1 | The packet-count term exists at all | Arithmetic, no hardware | The entire H1 argument, and with it the reason to prefer Option 3 over a byte-count argument |
| 0 | The meter measures the thing it is pointed at | Hours, no experiment | Every number in the project |
| 1 | The available range is worth chasing | One cheap run | The direction; Stages 2 onward are then unfunded |
| 2 | The frame boundary has a step, separable from the wake term | The long sweep | The cost model's structure |
| 3 | The two-sided ledger favors the latent | Encoder training and deployment | H_ledger |
| 4 | Each model term survives its own control | Repeats of Stage 3 key points | The attribution of any Stage 3 result to a named term |
| 5 | A bridge can carry a representation across independently trained halves | A second training regime | H_transfer only |

**Stage −1: the configuration contract.** Before any board is flashed, and before any hardware is bought. This stage is arithmetic over datasheet constants, ESP-IDF headers, and the candidate encoder's declared shapes, and it settles three questions that decide the design.

Does $n(p_{\mathrm{raw}}) - n(p_{\mathrm{lat}}) \geq 1$ hold for the chosen sensor modality at the candidate bottleneck width and quantization? If raw and latent both fit in one frame, the packet-count term is identically zero, H1 has nothing to say, and the experiment must either change modality, change $L$ by changing ESP-NOW version, or be re-scoped to a byte-count claim. Second, do Encoder_S and Decoder_R agree on quantization scheme, scale granularity, and tensor layout? A per-tensor scale on one side and a per-channel scale on the other is a contract mismatch that no amount of bridge training absorbs cleanly, and it is readable from two configuration files. Third, what is the width ratio between the latent and what the decoder expects, per the note in 4.7?

Deliverable: one table of declared constants and three answers. Cost: an afternoon and no equipment.

**Stage 0: instrument validation, both halves.** Before any experimental claim, prove the meter. This gate has two halves and neither may be skipped, because the first half alone is consistent with both total success and total failure.

The first half is the no-op: reproduce a known static load within tolerance, verify frame delivery, verify GPIO edges appear in the logger. A harness that is reading the wrong rail, or whose sense resistor is on the far side of the regulator, or whose GPIO is unconnected, can produce a trace that looks entirely plausible and is entirely stable.

The second half is the negative control, and it is what distinguishes a working meter from a meter measuring something else that happens to be steady. Command a change of known size and require the trace to move by the predicted amount: a busy loop of commanded duration must produce an integral proportional to that duration, and with the radio disabled the transmit-phase integral must *collapse*. The last of these is what proves the phase markers bracket what they are believed to bracket.

The question asked of this gate, and of every gate after it: what would make this check pass while comparing the wrong quantity? The parent build ran a full training arm on a probe that passed correctly for a full run while comparing against the wrong baseline, so this question is asked in writing and answered in writing.

Deliverable: one plot of a single transmit event with phases visibly segmented, plus the three negative-control traces. Until this exists, the harness has not been built; it has been hoped for.

**Stage 1: the null and the direction probe.** Two questions, one cheap run, raw transfer only.

First, measure $C_{\mathrm{null}}$ per 4.4: the empty event, one byte, no compute, both nodes metered. Report $C_{\mathrm{null}}/C_A$ at the smallest and largest raw payloads under consideration. If the floor is most of the cost across that whole span, the achievable range is narrow, and that is a verdict on the direction obtained for the price of one run.

Second, at three or four payload sizes, roughly 50 to 100 packets each: does $E(p)/p$ fall within a frame (H1-shaped) or stay flat (H0-shaped)? If H0, stop and rethink; the remaining stages assume H1.

**Stage 2: the fragmentation curve, with the wake term separated.** Full payload sweep across the frame boundary, at least 1000 packets per size, both nodes metered, raw transfer only. Interleaved with the sweep, the $k$-payloads-per-wake control of 4.1: at fixed payload size, vary the number of payloads sent inside one wake window and fit $E_{\mathrm{wake}}$ and $E_{\mathrm{pkt}}$ separately. Without this control the sweep produces a curve whose fixed term cannot be attributed.

Report median, P95, P99 per size; the latency distribution is expected heavy-tailed and means alone would mislead. Deliverable: the sawtooth curve with confidence bands, TX-side and RX-side plotted separately, with the wake term reported as its own quantity. This is the project's first genuinely novel artifact regardless of what comes after.

**Stage 3: the two-sided ledger, swept (H_ledger).** Introduce the jointly trained encoder on S and the matching decoder on R. Run Conditions A and B on identical input sets. Charge every phase to its node. Verify the sufficiency and collapse checks of 4.5 per sample before admitting any efficiency comparison.

This stage is a sweep, not a point. The two payloads do not scale with the same quantity: $p_{\mathrm{raw}}$ scales with the observation — sensor resolution, window length — while $p_{\mathrm{lat}}$ is a design constant independent of it. The compression ratio is therefore a free variable, and the sign of the result at any single operating point says nothing about where the ledger turns. Sweep the observation size across at least the range that moves $n(p_{\mathrm{raw}})$ through two frame boundaries.

Deliverable: $C_A$ against $C_B$ with all terms itemized and the decomposition of 4.2 residue-checked, $G$ per 4.4 with confidence intervals, and the observation size at which the ledger crosses. This stage resolves H_ledger only.

**Stage 4: controls (H_ledger).** Repeat key points with and without CCMP encryption; at the default 1 Mbps versus a raised PHY rate; and, if ESP-IDF is at 5.4 or later, under v1.0 framing versus v2.0 framing, since $L$ moves and the sawtooth should move with it. Each control isolates one term of the model. A model term that survives its control is measured; one that does not is a modeling error found early.

**Stage 5: the transfer variant (H_transfer).** Gated on Stages 3 and 4 having produced a trusted, controlled H_ledger result. Train Encoder_S and Decoder_R separately, on separate objectives, ideally with a deliberate architectural difference so their latent geometries are not accidentally compatible. Freeze both. Train only the Projector. Measure $C_{\mathrm{transfer}}$ per 4.7, verify sufficiency and the collapse check per sample before any efficiency claim, and report the break-even margin against Stage 3's H_ledger margin explicitly, not in its place.

A negative result here — ledger favorable but transfer unusable, or transfer usable but ledger unfavorable — is still an answer to the question this thinkbook exists to ask, and localizes cleanly to either the bridge or the radio.

**Statistical discipline throughout.** Paired comparisons on identical inputs where possible; bootstrap confidence intervals on medians; no representation-sufficiency number reported before its per-sample audit and collapse check; no energy number reported from a run whose decomposition residue failed. First cycle of every run discarded as warm-up, following the precedent methodology.

---

## 6. Builder Knowledge

Three classes. Class A is load-bearing: specifications and peer-reviewed measurements this design directly depends on, plus this project's own validated findings. Class B is methodological: repos and papers whose techniques are adopted or adapted. Class C is contextual: surveys and community writing that orient but never justify a design decision. A claim may cite downward for color, never upward for support.

### Class A: Primary

| Source | What it anchors |
|---|---|
| ESP-IDF Programming Guide, ESP-NOW API reference | Frame format, peer model, callback semantics, payload limits: 250 B v1.0 (`ESP_NOW_MAX_DATA_LEN`), 1470 B v2.0 (`ESP_NOW_MAX_DATA_LEN_V2`, ESP-IDF ≥ 5.4). The constant $L$, and the Stage −1 arithmetic. |
| Espressif, "Current Consumption Measurement of Modules" | The official metering methodology: external supply, dynamic ranging, GPIO alignment. Stage 0 follows it and adds the negative control it does not require. |
| ESP32 Series Datasheet | Ceiling figures for TX/RX current (borrowed, upper envelope only, never cited as per-event energy). |
| Karic et al., arXiv:2510.24829 | The phase-segmented metering methodology on this chip family. Source of Gap 1 by its own stated limits, and of Gap 2's first half. Also the source of two rules adopted here: that operation count does not predict energy on this hardware, and that on-device processing discards the raw observation — the latter being Option 2's cost stated by its strongest advocate. |
| Lee, Lee & Ko, arXiv:2512.24179, §5.3 | The only published per-payload ESP-NOW transmission energy figures found. Anchors the existence of the measurement, not its values. **Quarantined:** two of five rows fail the duration × power = energy check, the body text and the layer table disagree by three orders of magnitude on compute, and the implied compute throughput is implausible for the stated core. No figure from this source is used as a value; it is used to establish that the fixed per-event term is large enough to require the treatment of 4.1. |
| Parent build, `cache-2-cache-lite`, `FINDINGS.md` | The validated Python-tier result that a representation crosses an independently trained boundary and carries value, under paired statistics with per-sample records. Also the source of three disciplines adopted here verbatim in form: the null before the trained comparison, the decomposition of a total into the part the grader can see and the part it cannot, and the collapse signature that an aggregate cannot detect. Its third open item is this document. |
| Fu et al., C2C (ICLR 2026), §3.3.4 | The freeze-both-then-train-only-the-bridge protocol Stage 5 physically mirrors. **Scope note:** this anchors H_transfer as a fair test of C2C's *training protocol*. It is not a test of C2C's medium. C2C never claims its transferred cache is smaller than the alternative; its latency gain comes from avoiding sequential decoding, not from moving fewer bytes. The compression framing is this project's, and it is this project's to defend. |

### Class B: Secondary

| Source | What is taken from it |
|---|---|
| `leonyuhanov/ESP32_ESPIDF_ESPNOW` | Two-role ESP-IDF scaffold with microsecond RTT; adapted for tx_role/rx_role. |
| `thomasfla/Linux-ESPNOW` | Sweep-and-plot methodology for rate versus loss versus latency. |
| `FCam1/ESP32-INA226` | ESP-IDF INA226 driver; the meter side of the harness. |
| `marcoratto/DMFLib-Arduino` | Reference fragmentation design (one sequence byte, no dynamic allocation); the fragmenter follows its shape but is rewritten, so its cost is ownable and chargeable. |
| Delft split-inference on MCUs, arXiv:2605.09357 | Evidence that intermediate activations do move between MCUs in practice. Not yet read at source; the claims attributed to it here are provisional until it is. |
| Urazayev et al., IEEE SIST 2023, DOI 10.1109/SIST58284.2023.10223585 | The one peer-reviewed power-analyzer measurement of ESP-NOW located. Not yet read at source. Should be read before Stage 1, since if it contains a payload sweep it changes what Stage 1 needs to establish. |
| Shao & Zhang, BottleNet++ (IEEE ICC Workshops 2020) | The encode/channel/decode decomposition and the feature-compression framing; adopted structurally, priced here on a ledger it never used. |
| TinyML autoencoder deployments on ESP32-S3, e.g. arXiv:2606.02256 | Proof the Condition B encoder is deployable as int8 under TFLite Micro on this chip class; the encoder is adopted practice, not a contribution. |
| Parent build, `c2c_first_principle.md` | The four-condition derivation Section 1 inherits, and the build-order thesis Section 5 inherits: each condition has its own cheapest falsification, and the build order is the ascending order of those costs. |

### Class C: Tertiary

| Source | Orientation provided |
|---|---|
| Xin, Fan & Letaief, *Entropy* 26(2):102 | The conceptual vocabulary (Weaver's three levels, semantic encoder / knowledge base / semantic decoder). Two things are taken beyond vocabulary: scheduling and energy optimization is listed there as an open challenge, which is this project's positioning; and the *knowledge base* is the theoretical name for what H_transfer removes, since the survey frames shared knowledge as a precondition for encoder and decoder to understand each other. Its channel-noise machinery is explicitly *not* adopted; ESP-NOW's MAC-layer CRC and ACK make the noisy-channel problem someone else's at this tier. |
| Qin et al., generalized semantic communications | Names the computational cost of semantics as a category in its own framework and never prices it. Gap 6. Its channel-semantics half is out of scope at this tier. |
| SJTU JSCC line and `SJTU-mxtao/semantic-communication-w-codebook` | What the mature end of learned-representation transfer looks like; a source of later ideas, not current requirements. |
| Community engineering writing: sleep-mode current envelopes, latency benchmark threads, wake-transmit-sleep traces, coexistence notes | Expectation-setting for magnitudes and gotchas (power-save packet loss, callback discipline, heavy tails). Every figure from this row is borrowed and dies on contact with Stage 2 data. |
| ESP-FAQ, ESP-NOW section | Throughput context and the rationale for the v1.0 length limit. |

---

## 7. Predictions Ledger

Recorded before the runs that adjudicate them. Written down afterward, a prediction is worth nothing.

| # | Prediction | Adjudicated at | Outcome |
|---|---|---|---|
| 1 | $E(p)/p$ falls within a frame rather than staying flat | Stage 1 | — |
| 2 | The fixed per-event term exceeds the per-byte term over a full frame by at least an order of magnitude | Stage 1 | — |
| 3 | $C_{\mathrm{null}}/C_A$ exceeds one half at the smallest raw payload under consideration, and falls as observation size grows | Stage 1 | — |
| 4 | A step is visible at $L$ and is separable from the wake term by the $k$-per-wake control | Stage 2 | — |
| 5 | $E_{\mathrm{pkt}} \neq E'_{\mathrm{pkt}}$; direction not predicted | Stage 2 | — |
| 6 | Latency is heavy-tailed; P99 exceeds the median by more than a factor of three at some payload size | Stage 2 | — |
| 7 | $E_{\mathrm{enc}}$ is not predicted by operation count within a factor of two | Stage 3 | — |
| 8 | The H_ledger margin is an upper bound on the H_transfer margin | Structural; checked at Stage 5 | — |
| 9 | Decoder_R under H_transfer collapses onto a constant output at least once during training, and the aggregate $Q$ fails to flag it while the output-distribution check does | Stage 5 | — |

Prediction 9 is inherited rather than invented. The parent build produced that exact failure twice, under two objectives and two corpora, and each time the aggregate read the collapse as a partial score. It is recorded here so that when it happens it is a confirmation rather than a week spent debugging something that is working correctly.

### Readings that measurement overturned

Empty at time of writing. Entries are added here, never removed, and the corrected claim stays visible alongside its replacement.

Two readings have already been overturned by reading rather than by measurement, and are recorded for the same reason:

**That the split-inference line does not meter the radio at all.** Stated in an earlier draft of Gap 2. Contradicted by arXiv:2512.24179 §5.3, which is a per-layer ESP-NOW transmission energy profile. The gap survives in the narrower form now stated; the original wording would have been refuted by one table.

**That $E_{\mathrm{proj}}$ belongs to R because the bridge meets the receiver's local context.** Stated in an earlier draft of 4.7. R has no local context in this design. The placement is correct and the justification was not; it is now charged to R because it executes on R.

---

## Closing Note

The parent project earned its rules by breaking things: the extractor that lied politely, the position bug that passed every shape check, the probe that passed for a full run while comparing the wrong quantity. This document is an attempt to pay for fewer of those lessons twice. The contract precedes the hardware (Stage −1), the harness is validated in both directions before it is trusted (Stage 0), the null precedes the comparison it gives meaning to (Stage 1), the cheap probe precedes the expensive sweep, the sufficiency and collapse audit precedes every efficiency claim (4.5), and borrowed numbers are quarantined from measured ones by construction.

If H1 holds and Condition B clears the break-even inequality at a recovered fraction worth reporting, H_ledger is answered: a physical ledger at the MCU tier can favor a compact learned representation over raw transfer. That is not yet the harder claim. H_transfer is answered separately, at Stage 5, once Encoder_S and Decoder_R stop knowing about each other by construction and only the Projector is left to bridge them.

If both hold, the ledger becomes an engineering document at the MCU tier, the same way a validated projection made one at the model tier, and it does so honestly, having paid the extra cost the alignment problem actually demands rather than assuming it away. If either fails, the failure localizes to a named term in a stated model, and the direction dies cheaply, at the stage that could afford to be wrong.

The parent build ended with a threshold it could not adjudicate, because on a machine with no link the second half of the ledger costs nothing and therefore says nothing. This is the tier where the link costs something. Either outcome serves the compass. The point, as before, is not the table. The point is to stop being surprised by the numbers, at both the cheap layer and the hard one.

---

## Decisions

Choices made here that are hard to reverse, or that a future read would otherwise have to re-derive, are recorded as dated entries in `decisions/`:

- `2026-08-09-config-contract-precedes-hardware.md` — Stage −1 as a kill gate
- `2026-08-09-energy-reported-against-empty-event-null.md` — the null and $G$
- `2026-08-09-wake-cost-separate-from-frame-cost.md` — the three-term cost model
- `2026-08-09-receiver-holds-no-local-observation.md` — why the replacement lesson does not transfer
- `2026-08-09-compression-ratio-swept-not-fixed.md` — Stage 3 as a sweep
