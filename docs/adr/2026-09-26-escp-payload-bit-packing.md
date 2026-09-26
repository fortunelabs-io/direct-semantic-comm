# ESCP payload bit packing and requantization

Date: 2026-09-26
Status: Accepted

## Context

HLD 4.5 proposed bit-level rules for the ESCP payload on 2026-09-26 and left the section open. Two later decisions narrowed those rules. `adr/2026-09-26-encoder-toolchain-is-esp-dl.md` removed the zero point and made the scale a power-of-two exponent. `adr/2026-09-26-search-space-is-bottleneck-width-and-bit-width.md` made the cut a linear bottleneck, so the latent is signed.

Two questions remained. The first was the rounding mode of the requantization to fewer than 8 bits. The ESP-DL documentation states that ESP32-S3 uses `ROUND_HALF_UP`. The ESP-PPQ source implements `ROUND_HALF_UP` on tensors as `floor(x + 0.5)`, so ties go toward positive infinity. The second was the range of `b` in v1.

A check on 2026-09-26 found a further problem. Quantizing a value to int8 and then shifting it to int4 disagrees with quantizing the same value straight to int4 at 80 of 2,560 points on a uniform grid, about 3 percent. If nothing is decided, R and the host can disagree on the value of a latent element, and `Q(w, b)` for `b` below 8 becomes a prediction about a different arithmetic.

## Alternatives considered

- Byte order. Little-endian matches both nodes. Network order adds a byte swap on both sides.
- Order of sub-byte elements. Consecutive, least significant bit first, as in ONNX INT4. Split halves, as in GGML Q4_0, which helps SIMD kernels that latents of this size do not need.
- Requantization to fewer bits. An integer shift of the int8 bottleneck output. A direct quantization of the float tensor on S, which needs a float path or a second calibrated path on S.
- Scope of `b` in v1. Rules for `b = 8` only. Rules for all four values now, with `b = 8` as the only value in use.

## Decision

Every multi-byte field is little-endian. Every 16-bit element is little-endian.

The bit-width `b` is one of 2, 4, 8, or 16.

Elements with `b ≤ 8` are packed consecutively, least significant bit first. Element `k` occupies `b` bits. Those bits start at bit `(k·b) mod 8` of byte `⌊k·b/8⌋`. No element crosses a byte boundary.

Elements are signed two's complement in `[−2^(b−1), 2^(b−1) − 1]`. R sign-extends each element on unpack.

Unused trailing bits are zero. They sit in the high bits of the last byte. No element count travels on the wire. R reads `w` from `arm_config.h`.

For `b = 8` and `b = 16`, the element is the ESP-DL output value of the bottleneck with no further step.

For `b = 2` and `b = 4`, S computes `q_b = clamp((q_8 + 2^(s−1)) >> s, −2^(b−1), 2^(b−1) − 1)` from the int8 bottleneck output. The shift is arithmetic. The shift `s` equals `e_b − e_8`, a positive integer fixed at calibration. This computation equals ESP-PPQ `ROUND_HALF_UP` on ESP32-S3. The firmware relies on GCC, which documents that signed `>>` acts on negative numbers by sign extension.

The host evaluates `Q(w, b)` for `b = 2` and `b = 4` through the same two steps, the ESP-PPQ int8 tensor and then the reference integer shift. The host never quantizes the float tensor straight to `b` bits.

R unpacks after full reassembly, byte by byte, from an aligned static buffer.

The Python reference packer is the source of record for this format. `wire_vectors.h` covers every `b`, negative values, ties at the rounding midpoint, saturation at both ends of the range, and element counts that leave trailing bits.

v1 uses `b = 8` only. The values 2, 4, and 16 enter with the `(w, b)` sweep of HLD O10. Their rules and their vectors exist from the start.

## Consequences

The packer, the unpacker, the requantizer, and the host evaluation share one arithmetic. A disagreement between R and the host is a defect that a vector detects.

The shift `s` is a calibration output for each `(w, b)`. It is recorded in `arm_config.h`. It is also recorded in `quality_table.json`.

A shift below `8 − b` gives a finer step and clips large values. A shift of `8 − b` or more keeps the full int8 range with a coarser step. Calibration chooses `s` by `Q`, so clipping is a measured tradeoff.

The rounding clause is specific to ESP32-S3. ESP-DL documents `ROUND_HALF_EVEN` for ESP32-P4. A port to another chip family supersedes the rounding clause of this ADR.

A later SIMD unpack may prefer split halves. That change needs a new ADR and a new format version in the ESCP header.

The reference packer becomes a reusable asset. It needs a `DECISION_LOG.md` at its root once it exists.
