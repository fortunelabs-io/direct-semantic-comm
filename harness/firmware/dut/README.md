# DUT Firmware

ESP32-S3, ESP-IDF v5.x pinned in `../../.mise.toml`. **One image per role and per variant — never a runtime flag**, because the whole identification strategy differences whole-image totals against each other, and a flag means the two conditions share code that could differ in the condition being tested.

**Empty. Tier 2**, which does not start until Tier 1 has closed: the phase gate's criterion is *decoded by the capture engine with zero invalid codes*, and there is nothing to decode against until the capture engine works.

| Directory | What | Gate it serves |
|---|---|---|
| `phase_marker/` | one masked register write per transition, Gray-coded, nothing else in the function | phase |
| `commanded_load/` | busy loop of commanded microseconds, and a commanded-duty square wave, both echoed back over serial | negctl, timing, bandwidth |
| `espnow_link/` | unmodified vendor `wifi/espnow` example | link |
| `espnow_link_noradio/` | same marker path, radio never brought up | negctl, second half |

**`phase_marker` is one register write and nothing else** because the ESP-NOW send callback runs from a high-priority Wi-Fi task where the vendor documentation states lengthy operations must not happen. Codes for both roles: [`phase_code_map.md`](../../../docs/hardware-harness-v1/phase_code_map.md).

**`commanded_load` echoes back what it was asked for** so the host records the commanded value alongside the measured one. It is not a convenience: it is the negative control for the busy-loop fit and the bandwidth reference near 140 microseconds, and in both cases the DUT's own crystal is the standard the harness is being checked against — a reference outside the harness, which is what makes Stage 0's self-validation admissible.

**`espnow_link_noradio` is where the only gate that can detect a physically wrong setup lives.** With the radio disabled the transmit-phase integral must collapse. If it does not, the markers are in the wrong place and every phase attribution downstream is void, and nothing else in the ladder says so.

Condition A and Condition B images, `payload_gen`, the hand-rolled `fragmenter` and the ablation family are Stage 1 and beyond. Stage 0 needs only these four.
