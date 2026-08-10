# Harness v1

Two-channel metering rig for the Stage 0 through Stage 2 campaigns. One current sensor per node rail, one capture engine timestamping both sensors' conversion-ready edges and both nodes' phase markers against a single clock.

**Scope is the timebase, not the front end.** The INA226 parts are catalogue components. The capture engine, the timebase, the wire protocol, the calibration procedure and the host tooling belong to this project. The sensor layer can be replaced without touching any layer above it.

**Specification:** [`../todos/stage0_todo.md`](../todos/stage0_todo.md). Gates, commands, criteria and predictions live there and are not repeated here.
**Arithmetic:** [`../docs/hardware-harness-v1/harness_timing_budget.md`](../docs/hardware-harness-v1/harness_timing_budget.md).
**Outcomes:** `FINDINGS.md`, once gates start closing.

---

## The three implementation choices

Tier 0, second gate. Written down before firmware exists. **All three are open.**

1. **Capture free-running or gated:** *open.* Recommendation on record is free-running for v1, because a gate that fires wrongly loses data silently and a full stream can always be trimmed on the host.
2. **Phase code parallel or serial:** *open.* Recommendation on record is parallel, because serial adds a latency that would itself need characterising. Codes for both roles: [`phase_code_map.md`](../docs/hardware-harness-v1/phase_code_map.md).
3. **Bench sensor source:** *open.* If a breakout is used, its shunt value is confirmed **from its own schematic**, not from a product listing. Many INA226 breakouts ship 0.002 ohm for high-current use, which would put the 330 mA transmit peak at 165 microvolts of the 81.92 millivolt span and resolve nothing. PCB v1 does not reuse the breakout.

---

## Parts

| Item | Requirement | Value |
|---|---|---|
| Capture engine | two independent I2C masters at 400 kHz **on separate pins**, ten GPIO with edge interrupt, microsecond timer, USB device | STM32, *part open, Tier 0 gate 1* |
| Current sensor | INA226, Alert pin broken out | *open* |
| Shunt | 0.1 ohm, 0.1 percent, high side, Kelvin | fixed by the timing budget |
| DUT | ESP32-S3 dev board, native USB-serial-JTAG | *open*, 2 off |
| DMM | DCV 0.1 percent or better, four-wire preferred | *open* |
| Precision resistors | 0.1 percent metal film | *open* |
| Bench supply | regulated linear | *open* |
| Logic analyser | USB, PulseView or sigrok. Tier 1 only | *open* |
| Oscilloscope | not required | — |

The DMM and the resistors are the traceability standards for the gain and timing gates, and are the only real instrument purchase.

---

## Layout

```
mise trust && mise run setup && mise run doctor
```

`mise run doctor` asserts the environment is isolated: the interpreter is this venv, the home directory is not on `sys.path`, and every dependency matches its pin. Run it before trusting any capture. It exists because this machine carried a mise config at the home directory that applied to every project on it, and a capture taken under a leaked environment is well-formed and wrong — the same failure the negative control catches one layer down.

DUT firmware builds with `mise run idf -- build`, not bare `idf.py`. Inside this directory `idf.py` resolves to this venv, which has no IDF tooling; the task names the ESP-IDF interpreter explicitly.

Entering this directory activates `.venv` at the pinned interpreter. Every capture script and every test runs against that one and no other: a result must not depend on whose shell ran it. `mise run <gate>` is how each gate in the specification is invoked, and a gate whose script does not exist yet fails loudly rather than passing vacuously.

| Path | What | State |
|---|---|---|
| `.mise.toml` | tool pinning and task isolation | Python 3.12.13 pinned; ESP-IDF and capture toolchain open |
| `requirements.txt` | host dependencies, fully pinned including transitives | installed |
| `.venv/` | the interpreter every gate runs under | created, not tracked |
| `firmware/capture/` | capture engine: `timebase.c`, `sensor.c`, `stream.c` | planned |
| `firmware/dut/` | ESP32-S3, one image per role | empty, Tier 2 |
| `scripts/` | touch hardware, may prompt, write JSON | empty, see below |
| `tests/` | pure, read JSON, silent on success | empty, see below |
| `hardware/` | KiCad project, the review checklist, and the checks that run against both | checker and CI in place, KiCad project not started |
| `results/` | JSON tracked, raw captures not | empty |
| `assets/` | figures, `harness_` prefixed | empty |

**No empty `.py` files were created in `scripts/` or `tests/`.** An empty test file is silent and exits 0, which is exactly the signature of a passing gate. A directory of them would report a closed ladder on an unbuilt harness. Files appear here when they do something.

---

## Two structural rules the capture firmware is graded on

Both from timing budget §8, restated because a firmware that violates either is wrong before it is tested.

**Timestamp at the edge, read at leisure.** The conversion-ready interrupt captures the counter and queues the read; the read does not sit inside the interrupt. The I2C read is 73 microseconds and its start is subject to scheduling jitter, and that jitter would otherwise land directly in the timebase.

**Do not synchronise the two sensors.** They free run and their conversions will not align. The two-sided ledger needs a common *time*, not a common *sample instant*, and every record carries its own timestamp from one counter.
