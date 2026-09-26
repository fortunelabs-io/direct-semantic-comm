# Condition A runs the same network whole on R

Date: 2026-09-26
Status: Accepted
Amends: [2026-09-26-configuration-binds-at-build-time.md](./2026-09-26-configuration-binds-at-build-time.md), the sentence "A Condition A image holds no encoder weights"

## Context

HLD O11 recorded that Algorithm 1 prices path A with `E_proc` and `Q(A)`, and that the record did not name the network behind either value. Thinkbook §3 said that in Condition A R "runs the full processing step locally", and that in Condition B R "pays a reduced use cost". Both sentences fit two readings. R could run the jointly trained encoder and decoder whole. R could also run a separate model trained on raw data for the task.

The review on 2026-09-26 found that Neurosurgeon (Kang et al., 2017) and Auto-Split (Banitalebi-Dehkordi et al., 2021) each compared placements of one model, so their comparisons separated placement from model choice. The split computing survey of Matsubara et al. (§4.2, §4.3) treated an injected bottleneck as a change to one model, judged against that model without the bottleneck. If nothing is decided, `Q(A)` and `E_proc` belong to no defined network, and no Stage 3 prediction can be written.

## Alternatives considered

- The same network. R runs the jointly trained encoder and decoder whole on the raw window. Path A and path B then differ only in where the encoder runs and in what crosses the link.
- A separate model trained on raw data and sized for R. The comparison then includes a model choice.
- Both, with the separate model as a second path A.

## Decision

Path A runs the reference network whole on R. The reference network is the jointly trained encoder and decoder at the contract point, `w = 64` and `b = 8`.

The Condition A image on R holds the reference network. The Condition A image on S holds no model weights.

Path A keeps the reference network through Stage 3b. `Q(A)`, `E_proc`, and `T_proc` do not depend on the candidate `(w, b)`.

One export produces the whole reference network and its two halves from one quantized graph.

Two predictions enter `prediction.json` before the first Stage 3 run. At the contract point, the output of R under path B equals its output under path A, sample for sample. At the contract point, `E_proc − E_enc − E_use` is close to zero.

A mismatch in the first prediction is a defect. It is fixed before any Stage 3 result is admitted.

The second prediction is reported with its measured value and interval, as a residue per thinkbook §4.3.

A separate model trained on raw data is a later sensitivity arm. It is not part of v1.

## Consequences

The compute premium on the left of the thinkbook §4.4 inequality is close to zero at the contract point. H_ledger at that point measures the link saving almost alone. The Stage 3 report states this scope.

On the host, `Q(64, 8)` equals `Q(A)`. The quality check of line 25 of Algorithm 1 passes by construction at the contract point. `ε` bounds only the loss that a narrower width or fewer bits adds in Stage 3b.

`A_QUALITY` cannot occur in v1, because `Φ` always holds the contract point. `adr/2026-09-26-partition-decision-returns-three-verdicts.md` stays valid. `A_QUALITY` becomes reachable with the sensitivity arm.

v1 does not measure what the bottleneck costs in quality against a network without one. The sensitivity arm measures it.

The sentence "A Condition A image holds no encoder weights" of the binding ADR now holds for the image on S only.

The build test of `adr/2026-09-26-encoder-toolchain-is-esp-dl.md` also compares the bottleneck exponent of the whole network with the cut exponent of the two halves.

`width_profile.json` holds `E_proc` and `T_proc` once per `W`.

Thinkbook §3 names the network of Condition A at its next revision. This closes HLD O11.
