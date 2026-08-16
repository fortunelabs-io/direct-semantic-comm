# Stage -1 Configuration Contract

*The five questions of Stage -1, answered. Arithmetic over declared constants only: the ESP-IDF ESP-NOW header, the INA226 datasheet, and the ESP32-S3-WROOM-1 datasheet. Nothing here is measured. Cost to produce: an afternoon and no equipment.*

**Purpose.** Nothing downstream begins while any answer here is open, and that includes building the harness. This stage exists to kill the direction for the price of an afternoon, before the first substantial expenditure.

---

## Question 1: is the packet-count term non-zero?

This is the only question that can kill the direction, and it decides whether H1 has anything to say at all. If raw and latent both fit in one frame, then $n(p_{\mathrm{raw}}) = n(p_{\mathrm{lat}})$, the packet-count term is identically zero, and the argument collapses to a byte-count claim, which is the weaker thing H1 exists to strengthen.

### Selection criteria, in order of weight

**Acquisition energy must be small.** Sensor acquisition is paid identically in both conditions and cancels in the difference, but a large acquisition term adds variance to every event and makes the compute terms harder to resolve against it. This is the criterion that most affects whether the ledger is measurable, and it is the reason a camera is not the first choice despite giving the largest payload ratios.

**The observation must have one natural scalar size parameter.** Stage 3 sweeps observation size through two frame boundaries. A modality whose size is set by a single physically meaningful number sweeps cleanly; one that requires resampling introduces artefacts into the very axis being swept.

**Processing raw must be honestly required.** The win condition is easier to clear when $E_{\mathrm{proc}}$ is large. Choosing a modality because its raw processing is expensive would bias the result. The requirement is that the processing be genuinely necessary for the task, not that it be heavy.

**The encoder must stay modest.** Novelty is not sought in the model.

### Candidate arithmetic

Payload sizes before framing. Latent is a bottleneck of width 64 quantised to int8, giving 64 bytes plus one sequence byte from the fragmenter, so 65 bytes, which is one frame under either ESP-NOW version.

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
| | 96 x 96 | 9216 | 37 | 7 |
| Latent, width 64 int8 plus sequence byte | any | 65 | 1 | 1 |

### Answer

**Yes, for all three candidates under ESP-NOW v1.0.** The packet-count term is non-zero across every listed observation size, and the difference $n(p_{\mathrm{raw}}) - n(p_{\mathrm{lat}})$ ranges from 1 to 36.

**Recommended modality: six-axis IMU at 100 Hz, int16, window length as the swept parameter.** Acquisition current is milliamp-scale rather than tens of milliamps, so it contaminates the ledger least. Window length is a single physically interpretable number, so the Stage 3 sweep is a sweep in seconds of motion rather than in resampling resolution. The encoder for a windowed inertial signal is the smallest of the three. Activity recognition from inertial windows is established practice, so the processing is honestly required rather than chosen for its cost.

Audio is the second choice and is the closest precedent alignment, since the one prior ESP-NOW split-inference study used spectrograms. Image gives the largest ratios and demonstrates best in a pitch, and it is the weakest choice for this particular measurement because the capture and encode path adds the largest term that is not the link.

### A threshold that falls out, and matters

There is a minimum observation below which the packet-count argument is void, and it moves with the ESP-NOW version.

| Version | Frame limit | Minimum raw payload | Minimum IMU window at 100 Hz |
|---|---|---|---|
| v1.0 | 250 B | above 250 B | above 0.21 s |
| v2.0 | 1470 B | above 1470 B | above 1.23 s |

**This makes the version choice derived rather than arbitrary.** Core runs use v1.0, because the packet-count term is then non-zero across the entire intended sweep. v2.0 becomes the Stage 4 control, where its near-zero packet-count term below 1470 bytes is not an inconvenience but the prediction being tested: under v1.0 crossing the limit adds a frame with its own acknowledgement and its own retransmission opportunity, while under v2.0 it adds a vendor-specific element inside one frame at a cost of seven bytes.

---

## Question 2: do both nodes agree on ESP-NOW version?

**Declared.** Both nodes run one pinned ESP-IDF version and therefore one ESP-NOW version. The three length constants are read from the header rather than assumed: `ESP_NOW_MAX_DATA_LEN` for v1.0, `ESP_NOW_MAX_DATA_LEN_V2` for v2.0, and `ESP_NOW_MAX_IE_DATA_LEN` for the interoperation boundary.

The trap being closed: a v1.0 receiver given a v2.0 packet longer than `ESP_NOW_MAX_IE_DATA_LEN` truncates the data to the first `ESP_NOW_MAX_IE_DATA_LEN` bytes or discards the packet. Truncation presents as a decode failure rather than a link failure, and would be attributed to the representation rather than to the transport.

Verified on hardware by the Stage 0 `link` gate, which checks this declaration rather than answering an open question.

---

## Question 3: do Encoder_S and Decoder_R agree on quantisation, scale granularity and tensor layout?

**Closed for H_ledger by construction.** Condition B trains both halves as one system and splits them only at deployment, so the quantisation scheme, scale granularity and tensor layout are shared by definition.

**Reopened before Stage 5.** When the halves are trained independently, a per-tensor scale on one side and a per-channel scale on the other is a contract mismatch that no amount of bridge training absorbs cleanly, and it is readable from two configuration files. That check is a fresh Stage -1 pass, taken in its own turn.

---

## Question 4: what is the width ratio between the transmitted latent and what the decoder expects?

**Closed for H_ledger by construction**, same reason as Question 3: one design, one width.

**Reopened before Stage 5**, where the parent build's finding applies: if the producer's representation is much narrower than what the consumer expects, the bridge should widen it before combining rather than be expected to compensate inside the combination.

---

## Question 5: what shunt keeps the transmit peak on scale, and where is the resolution floor?

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

Question 1 reopens if the modality changes, if the latent width grows past the frame limit, or if the core runs move to v2.0, in which case the minimum viable window rises roughly sixfold and the lower half of the intended sweep loses its packet-count term.

Question 5 reopens if the device under test changes, since the shunt is sized against the ESP32-S3 transmit figure.

Questions 3 and 4 reopen before Stage 5 by design.

---

## Status

**Closed.** All five answered, nothing open. Stage 0 may begin at Phase A.
