# Assets

**Every asset carries the `harness_` prefix.** Assets built without data carry `harness_schematic_` instead, so a reader can tell from the filename which kind they are holding.

**An asset is not created before the file it reads from exists.**

| File | Reads from | Shows |
|---|---|---|
| `harness_schematic_sense_topology.png` | — | high-side sensing, Kelvin connection, one continuous ground across both nodes and the capture engine |
| `harness_negative_control.png` | `results/stage0_negctl.json` | three conditions on one axis: static load, commanded busy loop, radio disabled |
| `harness_gain_residuals.png` | `results/stage0_gain.json` | per-channel residual against the reference, with the channel-to-channel difference on the same axis |
| `harness_segmented_event.png` | Tier 3, drawn last | one event with its phases segmented, annotated with its own boundary uncertainty of roughly 140 microseconds. Illustrative; no claim rests on it |

All four are planned. None exists.
