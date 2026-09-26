# ESCP header is one byte per frame and six bytes per message

Date: 2026-09-26
Status: Accepted

## Context

HLD 4.5 proposed three header layouts for the ESCP frame on 2026-09-26 and recommended layout (i), one byte per frame and two bytes per message. The record before that date held one line on the header. Thinkbook §3.4 and the fragmenter row of HLD 3.1 said "one sequence byte", and contract Q1 priced the latent at 64 bytes plus that byte, 65 in all. HLD O9 recorded that the per-frame header moves every tooth of the sawtooth of H1, so the thinkbook and the contract state their frame counts without headers.

A review of fragmentation and integrity practice on 2026-09-26 found five things. RFC 4944 fragments a datagram with a 4-byte header on the first fragment and a 5-byte header on each later fragment, each carrying the datagram size and a tag. RFC 8724 (SCHC) carries one reassembly check sequence per packet, a CRC32 by default, in the last fragment. The ESP-IDF ESP-NOW guide states that a successful send callback means delivery at the MAC layer only, and it recommends a sequence number to drop duplicate packets. The Espressif esp-now component carries a 16-bit magic value for the same purpose. Stone and Partridge (SIGCOMM 2000) found that between 1 in 1,100 and 1 in 32,000 TCP packets failed the TCP checksum after passing the link CRC, which is the end-to-end argument of Saltzer, Reed, and Clark measured on real traffic.

Layout (i) relied on stop-and-wait under the app-layer ACK to tell messages apart, and it carried no check above the 802.11 frame check sequence. That check covers each frame on the air. It does not cover reassembly, memory, or the path inside either node. If nothing is decided, the Stage 2 tooth positions, the contract thresholds, and the ACK format stay open.

## Alternatives considered

- Layout (i). One byte per frame and two bytes per message. No sequence number and no end-to-end check.
- Layout (ii). Two bytes per frame with an 8-bit sequence, a 7-bit fragment index, and a last-fragment flag, plus two bytes per message.
- Layout (iii). Seven bytes per frame with a magic byte, `j`, `b`, a sequence number, and a CRC16. It carries no fragment index, so it cannot reassemble a multi-frame message.
- An RFC 4944 style header. Four bytes on the first frame and five on each later frame.
- A CRC16 on every frame. It repeats on each frame the check the 802.11 FCS already makes.
- A CRC32 once per message, as in SCHC. Two bytes more per message than a CRC16.
- Layout (i′). One byte per frame and six bytes per message, with a 16-bit sequence number and one CRC16 per message.

## Decision

The ESCP header follows layout (i′). The per-frame header `h_frag` is 1 byte. The per-message header `h_msg` is 6 bytes.

Every ESCP frame starts with one frame byte. Bits 0 to 4 hold the fragment index. Bit 5 holds the last-fragment flag. Bits 6 and 7 hold the message parity, which is the two low bits of the message sequence number.

A message is one byte string. The string is a 4-byte prefix, then the body, then a 2-byte CRC16. The fragmenter cuts the string into pieces of at most `L − h_frag` bytes. Each piece travels behind one frame byte.

The prefix holds three fields in this order. The first byte holds the format version in bits 0 to 3 and the message type in bits 4 to 7. The second byte holds the config ID from `arm_config.h`. The last two bytes hold the 16-bit sequence number, little-endian.

The format version is 1. The message type is 1 for raw data, 2 for latent data, and 3 for the ACK. The value 0 is invalid in both fields.

S increments the sequence number once per message.

The CRC16 covers the prefix and the body. It is the value of the ESP32-S3 ROM function `esp_rom_crc16_le`, polynomial 0x1021. It is little-endian on the wire.

The Python reference packer of `adr/2026-09-26-escp-payload-bit-packing.md` is the source of record for the header too. `wire_vectors.h` includes whole messages with their CRC16, a message whose CRC16 straddles two frames, and an ACK.

R reassembles by fragment index into the aligned static buffer of `adr/2026-09-26-escp-payload-bit-packing.md`. R ends a message as incomplete if a fragment index is missing or if the parity changes before the last-fragment flag. R checks the CRC16 after the last fragment.

R sends no ACK for a message that ends incomplete, fails the CRC16, or repeats the sequence number of the last accepted message. R logs each such message with its reason. The sample is excluded from `Q` and reported as a loss, per thinkbook §4.6.

If the version or the config ID differs from R's own, R drives the phase bus to error code `111` (HLD 4.2).

The ACK is a one-frame message of type 3 with an empty body. Its sequence field echoes the sequence number of the message it confirms. Its frame byte carries index 0, the last-fragment flag, and the parity of the echoed sequence number. The ACK is 7 bytes on the wire.

The layout is the same under ESP-NOW v1.0 and v2.0. The layout is the same in Condition A and Condition B.

## Consequences

The frame count is `n(p) = ⌈(p + 6)/(L − 1)⌉`, and the bytes on the wire are `p + 6 + n(p)`.

The contract latent, 64 int8 elements, is 71 bytes on the wire. It is one frame under either version.

The first tooth of the raw window sits at a body of 243 bytes under v1.0, a window of `243/1200 = 0.2025 s`. Under v2.0 it sits at 1,463 bytes, a window of `1463/1200 ≈ 1.219 s`. The frame counts of the IMU rows of contract Q1 do not change. The 96 × 96 image row moves from 37 to 38 frames under v1.0.

Contract Q1 and thinkbook §4.1 restate `n(p)` and the threshold table from this ADR. This closes HLD O9.

One frame under v1.0 holds a latent of at most 243 elements at `b = 8`, 486 at `b = 4`, 972 at `b = 2`, and 121 at `b = 16`.

The 5-bit fragment index caps a message at 32 frames. Under v1.0 the cap is a body of 7,962 bytes, an IMU window of 6.635 s. The 96 × 96 image row of contract Q1 exceeds it. A longer message needs a new format version. The generator of `arm_config.h` refuses an arm above the cap.

The six message bytes appear once in each condition and cancel in `C_A − C_B`. The frame byte scales with `n` and joins the packet-count term.

The Stage 2 images run this format. The CRC16 cost per byte therefore enters the measured `e_byte` and `e'_byte`.

The ACK is identical across conditions. Its cost lands in `E_ack` of Algorithm 1.

A CRC16 failure at Stage 0 or Stage 2 is a finding. The 802.11 FCS passed for every frame of that message, so the error arose in reassembly, in memory, or inside a node. It is investigated before any result that depends on that capture.

The sequence number wraps after 65,536 messages. The host unwraps it by capture order.

Adoption of binding (b) of `adr/2026-09-26-configuration-binds-at-build-time.md` adds `w` and `b`, or a policy index, and a new format version.

The fragmenter becomes a reusable asset. It needs a `DECISION_LOG.md` at its root once it exists.
