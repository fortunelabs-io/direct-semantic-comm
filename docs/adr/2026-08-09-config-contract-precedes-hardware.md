# A configuration contract gates the experiment before any hardware

**Date:** 2026-08-09
**Status:** Accepted

## Context

The packet-count argument (H1) is the reason to prefer a learned intermediate
representation over a byte-count argument that would hold trivially. That
argument requires `n(p_raw) - n(p_lat) >= 1`. Whether it holds is arithmetic
over the ESP-NOW frame limit, the sensor modality, and the candidate bottleneck
width, all of them declared constants and none of them measurements.

If nothing is decided, that arithmetic gets done implicitly at Stage 3, after a
harness has been bought, a board flashed, and an encoder trained, at roughly a
hundred times the cost of doing it on paper. The parent build found its own
cheapest gate to be a read of two config files that decided the shape of the
bridge before a parameter existed.

## Alternatives considered

- Fold the check into Stage 0 as a note. Rejected: Stage 0 requires hardware,
  and this check does not.
- Assume it holds because raw sensor data is obviously larger than a latent.
  Rejected: obviousness is not a frame count, and at small observation sizes
  both payloads can fit in one frame.
- Skip it and let Stage 1 reveal the problem. Rejected: Stage 1 measures the
  radio, not the design constants, and would not surface a zero packet-count
  term as such.

## Decision

Stage -1 exists and precedes all hardware work. It answers three questions from
declared constants only: whether the packet-count term is non-zero for the
chosen modality and bottleneck width; whether Encoder_S and Decoder_R agree on
quantization scheme, scale granularity, and tensor layout; and what the width
ratio is between the transmitted latent and what the decoder expects.

A failure at Stage -1 is not a setback. It re-scopes the project to a
byte-count claim, or changes the modality, or changes `L` by changing ESP-NOW
version, before anything has been spent.

## Consequences

Commits to publishing the contract table as a deliverable, so that a reader can
check the arithmetic without re-deriving the constants.

Rules out starting hardware work on a modality whose raw payload fits in one
frame, unless the project has explicitly accepted the weaker claim.

Watch for: a contract that passes at the nominal bottleneck width but fails
after quantization changes the latent size, or after an ESP-IDF upgrade moves
`L` from 250 B to 1470 B. The contract is re-run whenever either changes.
