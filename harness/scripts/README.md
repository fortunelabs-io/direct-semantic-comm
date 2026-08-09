# Capture Scripts

Touch hardware. May prompt for a reference reading typed from a meter. Write JSON to `../results/`.

One per gate, named in [`../../todos/stage0_todo.md`](../../todos/stage0_todo.md): `capture_rate`, `capture_jitter`, `capture_dropped`, `capture_stream`, `capture_wrap`, `capture_phase`, `capture_link`, `capture_incoming`, `capture_noop`, `capture_negctl`, `capture_gain`, `capture_timing`, `capture_bandwidth`, `capture_supply`.

**Each refuses to start when the file it reads is absent.** That is the whole order-enforcement mechanism — no runner, no discipline. It enforces order and nothing else: a capture written from the wrong rail is well-formed, and only the negative control gate detects that.

**Empty.** Files appear when they do something.
