# Results

**Every number that later appears in a claim is read from a named file here.** Nothing is recalled, nothing is retyped.

JSON tracked. Raw captures not tracked, but retained where a gate calls for it.

| File | Tier | What it holds |
|---|---|---|
| `stage0_toolchain.json` | 0 | selected part, its two I2C peripherals as recorded from the datasheet |
| `stage0_rate.json` | 1 | registers read back, edge count, elapsed time, derived rate |
| `stage0_jitter.json` | 1 | inter-timestamp interval histogram and its standard deviation |
| `stage0_dropped.json` | 1 | edges seen, records emitted, difference, per channel |
| `stage0_stream.json` | 1 | ten-minute stream result; raw capture retained |
| `stage0_wrap.json` | 1 | monotonicity across the counter wrap |
| `stage0_phase.json` | 2 | transition count, invalid code count, dwell time per state |
| `stage0_link.json` | 2 | IDF version, ESP-NOW version per node, frames sent, frames delivered |
| `stage0_incoming.json` | 3 | per-board continuity and shorts, assigned serial numbers |
| `stage0_noop.json` | 3 | current against the resistor reference, both channels; all four edge sources |
| `stage0_negctl.json` | 3 | duration sweep with its fit; radio-on against radio-off integrals |
| `stage0_gain.json` | 3 | per-channel and channel-to-channel figures, **and the measured shunt per board** |
| `stage0_timing.json` | 3 | commanded against reproduced pulse widths |
| `stage0_bandwidth.json` | 3 | commanded against reproduced duty cycle |
| `stage0_supply.json` | 3 | supply excursion across a transmit burst |

`stage0_gain.json` is the one file that feeds forward into every later stage: **measured shunt values replace nominal from that gate onward, everywhere.**

No energy figure from this harness appears in a commit message, a README, or a notebook before `stage0_negctl.json` exists and its gate is closed.
