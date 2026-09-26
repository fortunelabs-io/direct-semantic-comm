# Encoder toolchain is ESP-DL v3 with ESP-PPQ

Date: 2026-09-26
Status: Accepted

## Context

Stage 3 needs an encoder on S and a decoder on R, split from one jointly trained model. The toolchain that deploys them fixes five things at once. It fixes the quantization scheme of the tensor at the cut. It fixes how faithfully the host can evaluate `Q(j, b)` for Algorithm 1. It fixes how the quantization parameters of the cut are kept equal between the S image and the R image, which is contract Q3. It fixes the cost of requantizing the cut to 2 or 4 bits, since no toolchain offers sub-byte activations. It also fixes how realistic the measured `E_enc` is.

Thinkbook §3.3 treats the encoder as adopted practice and seeks no novelty in the model. Three options were reviewed on 2026-09-26. If nothing is decided, the bit-packing rules and the evaluation of `Q` stay open.

## Alternatives considered

- TensorFlow Lite Micro with esp-nn. Activations are asymmetric with a zero point. Weights are per-channel. Host fidelity needs the reference kernels on the host. esp-nn does not state that its optimized kernels match the reference kernels bit for bit.
- ESP-DL v3 with ESP-PPQ. Quantization is symmetric with a power-of-two scale. Weights are per-tensor only on ESP32-S3. The documentation states that results on the board can be aligned with ESP-PPQ on the PC.
- Hand-written kernels and a custom quantizer. This gives full control. It has no SIMD unless the project writes it. It contradicts thinkbook §3.3.

## Decision

The encoder and the decoder use ESP-DL v3 with ESP-PPQ.

The ESP-DL version is pinned in `.mise.toml`. The ESP-PPQ version is pinned in `.mise.toml`.

The host evaluates `Q(j, b)` with the ESP-PPQ executor.

A build-time test reads the exponent of the cut tensor from the S image and from the R image. The test fails if the two exponents differ.

## Consequences

The cut tensor carries no zero point. Its scale is one integer exponent. Both images compile that exponent in, so nothing about the scale travels on the wire.

Requantization of the cut to 2 or 4 bits is an arithmetic shift with rounding and clamping. The rounding mode of that shift must match the rounding mode that ESP-PPQ uses on the host.

Weights on ESP32-S3 are per-tensor only. Per-tensor weights can cost accuracy on a small model. Layer-wise equalization (Nagel et al.) is the first mitigation, as Karic et al. used with ESP-PPQ.

Symmetric quantization leaves the sign bit unused on a non-negative tensor. At 2 bits, a tensor after ReLU keeps two of four levels. The bit-packing ADR states how a cut after ReLU is packed.

Measured `E_enc` is directly comparable with Karic et al., the closest precedent.

Watch for the accuracy cost of per-tensor weights. The host quantizes the same model twice, per-tensor with ESP-PPQ and per-channel with the TensorFlow Lite converter. If the per-tensor model loses more than half of `ε` in `Q(A)` against the per-channel model, this ADR is superseded in favor of TensorFlow Lite Micro. The check needs no hardware.

Watch for ESP-DL schema changes. Version 3.1 made new models unreadable by older versions. A version change goes through a new pin and a rebuild of every image.

The export script that produces the two model halves becomes a reusable asset. It needs a `DECISION_LOG.md` at its root once it exists.
