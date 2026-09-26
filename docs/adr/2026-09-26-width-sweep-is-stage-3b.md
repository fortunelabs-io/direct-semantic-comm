# The width sweep is Stage 3b, after a host-only quality frontier

Date: 2026-09-26
Status: Accepted

## Context

`adr/2026-09-26-search-space-is-bottleneck-width-and-bit-width.md` made Algorithm 1 search over the width `w` and the bit-width `b`, and it left the stage placement of that search to HLD O10. Stage 3 of the thinkbook tested H_ledger at one point, `w = 64` and `b = 8`, across the observation sweep of the compression-ratio ADR. Stage 3 was already the most expensive stage. A sweep inside it multiplied that stage by one training run per width, each with several seeds, before the first result existed. The harness schedule did not budget the sweep.

The review on 2026-09-26 found that the two halves of the candidate table differ in what they need. The cost half needs the harness, one encoder image on S and one decoder image on R for each `(W, w)`. The quality half needs no hardware, since the host evaluates `Q(w, b)` with the ESP-PPQ executor under `adr/2026-09-26-encoder-toolchain-is-esp-dl.md`. If nothing is decided, the Stage 3 todo cannot state its image list.

## Alternatives considered

- The sweep inside Stage 3.
- A separate Stage 3b, run after Stage 3 passes its gates.
- A separate Stage 3b, preceded by a host-only frontier of `Q(w, b)` that filters the widths before any image is built.

## Decision

Stage 3 tests H_ledger at the contract point, `w = 64` and `b = 8`, across the observation sweep. Algorithm 1 at Stage 3 runs with `𝒲 = {64}` and `𝔅 = {8}`.

Before the first Stage 3 run on the harness, the host trains each width in `𝒲` with several seeds. The host evaluates `Q(w, b)` for each `b` in `𝔅` with the ESP-PPQ executor. The result is `quality_table.json`, the host-only frontier.

A width enters Stage 3b only if at least one `b` passes line 25 of Algorithm 1 on the frontier.

Stage 3b sweeps `(w, b)` over the widths that pass. Stage 3b starts after Stage 3 passes its gates.

Stage 4 and Stage 5 depend on Stage 3. They do not depend on Stage 3b.

## Consequences

The first H_ledger result needs one trained model. The same model serves path A, per `adr/2026-09-26-condition-a-runs-the-same-network-whole-on-r.md`.

The frontier costs training time and no harness time. It can run as soon as the dataset and `ε` are declared, before the harness exists.

If no width other than 64 passes, Stage 3b does not run. The frontier is then the finding, and it is recorded.

The Stage 3b todo states its image count before any image is built. The count is one encoder image on S and one decoder image on R for each `(W, w)`, over the widths that pass, per HLD 4.6.

The values 2, 4, and 16 of `b` enter with Stage 3b, per `adr/2026-09-26-escp-payload-bit-packing.md`.

A failure in Stage 3b voids the width recommendation only. H_ledger at the contract point stands.

The stage table of thinkbook §5 gains a Stage 3b row at its next revision. This closes HLD O10.

The frontier predicts quality only. The costs of the other widths stay unknown until Stage 3b measures them.

Watch for a frontier whose passing widths all fit in one frame. Such a set tests the byte term only. If Stage 3b is to test H1 on the semantic path, a width above one frame enters on purpose, per the search-space ADR.
