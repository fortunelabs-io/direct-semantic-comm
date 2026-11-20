# ESCP Configuration Contract

*This document declare the ESP-IDF ESP-NOW header, the INA226 datasheet, and the ESP32-S3-WROOM-1 datasheet.This data gathered from verified documentation across sources.*

**Purpose.**  To set the direction between hardware and software specification.

---

We have 5 question to answer before commit anything in this repo, those are:

## Q1: is the packet-count term non-zero?

This is the only question that can set the direction of H1. If raw and latent both fit in one frame, then $n(p_{\mathrm{raw}}) = n(p_{\mathrm{lat}})$, the packet-count term is identically zero, and the argument collapses to a byte-count claim, which is the weaker thing H1 exists to strengthen.

### Selection criteria

**Acquisition energy must be small.** Sensor acquisition is paid identically in both conditions and cancels in the difference, but a large acquisition term adds variance to every event and makes the compute terms harder to resolve against it. This is the criterion that most affects whether the ledger is measurable, and it is the reason a camera is not the first choice despite giving the largest payload ratios.

**The observation must have one natural scalar size parameter.** Stage 3 sweeps observation size through two frame boundaries. A modality whose size is set by a single physically meaningful number sweeps cleanly; one that requires resampling introduces artefacts into the very axis being swept.

**Processing raw must be required.** The win condition is easier to clear when $E_{\mathrm{proc}}$ is large. Choosing a modality because its raw processing is expensive would bias the result. The requirement is that the processing be genuinely necessary for the task.

**The encoder must stay modest.** Novelty is not sought in the model.

### Candidate arithmetic

Payload sizes before framing. Latent is a bottleneck of width 64 quantised to int8, giving 64 bytes. The ESCP header of `adr/2026-09-26-escp-header-layout.md` adds one frame byte and a six-byte message header, so the latent is 71 bytes on the wire, which is one frame under either ESP-NOW version. Frame counts include that header, `n(p) = ⌈(p + 6)/(L − 1)⌉`. Against the header-free count only the 96 x 96 image row changes, from 37 to 38 frames under v1.0, and that row exceeds the 32-frame cap of the header.

| Modality | Observation | Raw bytes | n at L = 250 | n at L = 1470 |
|---|---|---|---|---|
| IMU, 6 axis, int16, 100 Hz | 0.25 s | 300 | 2 | 1 |
| | 0.5 s | 600 | 3 | 1 |
| | 1.0 s | 1200 | 5 | 1 |
| | 2.0 s | 2400 | 10 | 2 |
| | 4.0 s | 4800 | 20 | 4 |
| Audio, mel spectrogram, int8 | 16 x 16 | 256 | 2 | 1 |
| | 32 x 32 | 1024 | 5 | 1 |
| | 64 x 64 | 4096 | 17 | 3 |
| Image, greyscale, int8 | 32 x 32 | 1024 | 5 | 1 |
| | 96 x 96 | 9216 | 38 | 7 |
| Latent, width 64 int8, ESCP header included | any | 64 body, 71 on the wire | 1 | 1 |

### Answer

**Yes, for all three candidates under ESP-NOW v1.0.** The packet-count term is non-zero across every listed observation size, and the difference $n(p_{\mathrm{raw}}) - n(p_{\mathrm{lat}})$ ranges from 1 to 37.

**Recommended modality: six-axis IMU at 100 Hz, int16, with window length as the swept parameter.** It has three advantages:

- **Clean ledger.** Acquisition draws only milliamps, so it adds the least non-link energy.
- **Simple sweep.** Window length is one physical number, so the Stage 3 sweep runs in seconds of motion.
- **Small encoder.** A windowed inertial signal needs the smallest encoder, and activity recognition from inertial windows is established practice, so the processing is genuinely required.

Audio is the second choice and is the closest precedent alignment, since the one prior ESP-NOW split-inference study used spectrograms. Image gives the largest ratios and demonstrates best in a pitch, and it is the weakest choice for this particular measurement because the capture and encode path adds the largest term that is not the link.

### The minimum observation threshold

There is a minimum observation below which the packet-count argument is void, and it moves with the ESP-NOW version. The minimums include the ESCP header, which moves the first step from `L` to `L − 7` bytes of raw payload.

| Version | Frame limit | Minimum raw payload | Minimum IMU window at 100 Hz |
|---|---|---|---|
| v1.0 | 250 B | above 243 B | above 0.20 s |
| v2.0 | 1470 B | above 1463 B | above 1.22 s |

**The minimum payload fixes the version choice.** Core runs use v1.0, whose packet-count term is non-zero across the entire sweep. v2.0 is the Stage 4 control, and its near-zero packet-count term below 1470 B is the prediction under test. Under v1.0, crossing the limit adds a frame, with its own acknowledgement and retransmission opportunity. Under v2.0, it adds a vendor-specific element inside the same frame, at a cost of seven bytes.

---

## Q2: do both nodes agree on ESP-NOW version?

**Declared.** Both nodes run one pinned ESP-IDF version and therefore one ESP-NOW version. The three length constants are read from the header `ESP_NOW_MAX_DATA_LEN` for v1.0, `ESP_NOW_MAX_DATA_LEN_V2` for v2.0, and `ESP_NOW_MAX_IE_DATA_LEN` for the interoperation boundary.

!!Version mismatch risk. a v1.0 receiver given a v2.0 packet longer than `ESP_NOW_MAX_IE_DATA_LEN` truncates the data to the first `ESP_NOW_MAX_IE_DATA_LEN` bytes or discards the packet. Truncation presents as a decode failure instead of a link failure, and would be attributed to the representation than to the transport.

Verified on hardware by the Stage 0 `link` gate.

---

## Q3: do Encoder_S and Decoder_R agree on quantisation, scale granularity and tensor layout?

**Closed for H_ledger by construction.** Condition B trains both halves as one system and splits them only at deployment, so the quantisation scheme, scale granularity and tensor layout are shared by definition.

**Reopened before Stage 5.** When the halves are trained independently, a per-tensor scale on one side and a per-channel scale on the other is a contract mismatch that no amount of bridge training absorbs cleanly, and it is readable from two configuration files. That check is a fresh Stage -1 pass.

---

## Q4: what is the width ratio between the transmitted latent and what the decoder expects?

**Closed for H_ledger by construction**, same reason as Q3: one design, one width.

**Reopened before Stage 5**, where the parent build's finding applies: if the producer's representation is much narrower than what the consumer expects, the bridge should widen it before combining instead of be expected to compensate inside the combination.

---

## Q5: what shunt keeps the transmit peak on scale, and where is the resolution floor?

**Closed.** 0.1 ohm on both channels, high side.

| Quantity | Value |
|---|---|
| Full scale | 819 mA, against a 330 mA transmit figure, 2.5x headroom |
| Least significant bit | 25 microamps |
| Drop at 330 mA | 33 mV, one percent of the rail |
| Dissipation | 10.9 mW |
| Transmit resolved to | 13,200 counts |
| Receive resolved to | 3,880 counts |
| Deep sleep, 8.14 microamps | 0.33 counts, below one bit, not measured |

Derivation in the harness signal inventory and timing budget.

---

## Declared constants

| Constant | Value | Source |
|---|---|---|
| Device under test | ESP32-S3 | selected |
| Transmit current, 802.11b 1 Mbps at 20.5 dBm | 330 mA | ESP32-S3-WROOM-1 datasheet |
| Receive current, 802.11b/g/n HT20 | 97 mA | same |
| Deep sleep current | 8.14 microamps | Espressif current consumption measurement guide |
| ESP-NOW default bit rate | 1 Mbps | ESP-IDF ESP-NOW reference |
| Frame limit, core runs | 250 B, v1.0 | same |
| Frame limit, Stage 4 control | 1470 B, v2.0 | same |
| Framing overhead per frame | 43 B | ESP-IDF frame format, confirmed independently by Urazayev et al. |
| ESCP header | 1 B per frame, 6 B per message | `adr/2026-09-26-escp-header-layout.md` |
| INA226 shunt full scale | 81.92 mV | INA226 datasheet |
| INA226 shunt least significant bit | 2.5 microvolts | same |
| Conversion time, fastest | 140 microseconds | same |
| Shunt, both channels | 0.1 ohm, 0.1 percent | this document |
| Modality | six-axis IMU, int16, 100 Hz | this document |
| Swept parameter | window length, 0.25 s to 2.0 s or wider | this document |
| Latent width | 64, int8 | this document |
| Capture engine inputs | 10 digital | harness timing budget |
| I2C buses | 2, fast mode 400 kHz | same |
| Timestamp resolution | 1 microsecond | same |

---

## What would reopen this

Question 1 reopens if the modality changes, if the latent width grows past the frame limit, or if the core runs move to v2.0, in which case the minimum viable window rises roughly sixfold and the lower half of the intended sweep loses its packet-count term. It also reopens if the ESCP header changes, since the header sets the frame count.

Question 5 reopens if the device under test changes, since the shunt is sized against the ESP32-S3 transmit figure.

Questions 3 and 4 can be reopen freely before Stage 5.

---

## Status

**Closed.** 

## Bibliography
- *H1*: packet-count dominance. [Thinkbook 4.4](../docs/dsc_first_principle.md#44-the-claim-under-test)
- *H_ledger*: joint encoder-decoder split across two nodes. [Thinkbook 3](../docs/dsc_first_principle.md#3-the-proposed-solution)
- *Stage -1*: packet-count term check, arithmetic only. [Thinkbook 5](../docs/dsc_first_principle.md#5-proof-steps)
- *Stage 0*: instrument validation. [Thinkbook 5](../docs/dsc_first_principle.md#5-proof-steps)
- *Tier 0*: Stage 0 gates with nothing powered. [Stage 0 todo](../todos/stage0_todo.md#tier-0)
- *Stage 3*: two-sided ledger. *Stage 4*: controls. *Stage 5*: transfer variant. [Thinkbook 5](../docs/dsc_first_principle.md#5-proof-steps)
- *Condition A*: raw transfer. *Condition B*: semantic transfer. [Thinkbook 3](../docs/dsc_first_principle.md#3-the-proposed-solution)
- *Encoder_S*, *Decoder_R*: encoder on sender, decoder on receiver. [Thinkbook 3](../docs/dsc_first_principle.md#3-the-proposed-solution)
- *ESCP header*: 1 B per frame, 6 B per message. [Thinkbook 4.1](../docs/dsc_first_principle.md#41-cost-model), [ADR](../docs/adr/2026-09-26-escp-header-layout.md)

## Source
- *ESP-NOW frame limits, framing overhead, bit rate*: Espressif Systems, "ESP-NOW", ESP-IDF Programming Guide, API Reference. [docs.espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_now.html)
- *Transmit and receive current*: Espressif Systems, "ESP32-S3-WROOM-1 & ESP32-S3-WROOM-1U Datasheet". [espressif.com](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- *Deep sleep current*: Espressif Systems, "Current Consumption Measurement of Modules", ESP-IDF Programming Guide, API Guides. [docs.espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/current-consumption-measurement-modules.html)
- *INA226 full scale, LSB, conversion time*: Texas Instruments, "INA226 High-Side or Low-Side Measurement, Bi-Directional Current and Power Monitor with I2C Compatible Interface", SBOS547B. [ti.com](https://www.ti.com/lit/ds/symlink/ina226.pdf)
- *Framing overhead, independent*: Urazayev et al., IEEE SIST 2023. DOI [10.1109/SIST58284.2023.10223585](https://doi.org/10.1109/SIST58284.2023.10223585)
- *Prior ESP-NOW split-inference study*: Lee, Lee and Ko, section 5.3. arXiv:[2512.24179](https://arxiv.org/abs/2512.24179)
- *Harness constants, Q5 derivation*: [Harness timing budget](../docs/hardware-harness-v1/harness_timing_budget.md)

**Amended 2026-09-26.** Question 1 restates its frame counts and thresholds with the ESCP header of `adr/2026-09-26-escp-header-layout.md`. The answer to Question 1 does not change.
