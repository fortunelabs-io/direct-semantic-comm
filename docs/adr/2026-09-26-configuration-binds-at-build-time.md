# Configuration binds at build time, one image per configuration

Date: 2026-09-26
Status: Accepted. Amended by [2026-09-26-condition-a-runs-the-same-network-whole-on-r.md](./2026-09-26-condition-a-runs-the-same-network-whole-on-r.md) in one sentence of Decision. The image count in Consequences is corrected by [2026-09-26-search-space-is-bottleneck-width-and-bit-width.md](./2026-09-26-search-space-is-bottleneck-width-and-bit-width.md).

## Context

The partition decision (Algorithm 1, HLD Appendix C) chooses a cut point `j` and a transmitted bit-width `b` for each window length `W`. The pre-computed artifact proposal of 2026-09-26 placed the quality table `Q(j, b)` and a per-layer energy profile in C headers for the firmware. The review found that the partition decision runs offline on the host, so no firmware image reads `Q`. The review also found that the per-layer profile comes from running firmware on the harness, so it cannot be an input to the build of that same firmware. That left one question open, which was when `(j, b)` binds to an image.

On the bench, the link, `W`, and the link constants are fixed per arm. A runtime chooser would therefore select the same `(j, b)` on every event and add only cost. If nothing is decided, the contents of each image, the list of ablation images, and the need for `j` and `b` in the ESCP header (HLD 4.5) all stay open.

## Alternatives considered

- Binding (a), static images. `(j, b)` is chosen offline and bound at build time, with one image per configuration.
- Binding (b), policy table. `(j, b)` is chosen offline for each link condition and bound at runtime by lookup, with every partition resident in one image.
- On-device optimization. The node holds `Q`, the cost model, and an estimate of the link constants. The node runs the argmin itself.

## Decision

Binding (a) is the v1 choice. Each image holds one partition at one bit-width. A Condition A image holds no encoder weights. Generated headers carry data only. A generated header never selects a code path. `Q(j, b)` and the cut profile stay on the host as JSON.

Binding (b) is recorded for a later stage. This ADR does not design it. Binding (b) is adopted only if it clears

`Σ_c π_c · (C(x_static, c) − C(x*_c, c)) > E_decide`

where `π_c` is the fraction of time spent in link condition `c`, `x*_c` is the best configuration for `c`, `x_static` is the best single configuration found under binding (a), and `E_decide` is the per-event cost of the lookup plus the cost of carrying every partition in one image.

On-device optimization is not pursued. A node in the field has no INA226. Its estimate of the link constants would rest on a proxy such as RSSI or retry count. That proxy is a model that has not been validated against the harness.

## Consequences

The ablation-image rule of `adr/…terms-identified-by-design-not-by-waveform.md` now covers the configuration axis.

The image count grows with the sweep. The count is bounded by Condition A and `(j*, b*)` for each `W`, plus one prefix image on S and one suffix image on R for each candidate cut. The count is not `|W| × N × |𝔅|`.

R reads the configuration from its own image. The ESCP header therefore does not need `j` or `b`. A one-byte config ID in the header detects a mis-paired S and R image.

`Q` never enters firmware under this decision.

`x_static` from binding (a) is the baseline that any later policy table must beat.

A later adoption of binding (b) supersedes this ADR. That adoption also amends the ablation-image ADR. It adds `E_decide` to the ledger of S. It adds `j` and `b`, or a policy index, to the ESCP header.

Watch for a deployment requirement in which the link condition changes during one run. That requirement is the trigger to reopen binding (b).

The generator that writes `arm_config.h` becomes a reusable asset. It needs a `DECISION_LOG.md` at its root once it exists.
