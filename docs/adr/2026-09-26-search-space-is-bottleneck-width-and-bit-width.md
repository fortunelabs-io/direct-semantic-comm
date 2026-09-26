# Search space is bottleneck width and bit-width, with the cut fixed at the bottleneck

Date: 2026-09-26
Status: Accepted

## Context

HLD O8 recorded that the first draft of Algorithm 1 treated the cut point `j` as a decision variable over the layers of an existing model. That draft conflicted with contract Stage -1, which fixes the latent at width 64 int8 behind an injected bottleneck. It also conflicted with the compression-ratio ADR, which assumes that the latent size does not depend on the observation.

On 2026-09-26 two forms were compared. Form J cut an existing classifier at layer `j`. Form W injected a bottleneck of width `w` and searched over that width. Two findings decided the comparison. Under J, S and R are the same chip, so moving layers across the link leaves total compute nearly unchanged, and the cheapest feasible candidate is almost always `j = N`, the final decision. That is Option 2 of thinkbook §1.1, which the thinkbook rejected for a reason outside the cost function. Under W, the width is a free design variable, so the latent can sit on either side of a frame boundary on purpose. That makes the sawtooth of H1 a design target on the semantic path, which is the premise of thinkbook §3.1.

## Alternatives considered

- Form J. Cut an existing classifier at layer `j` and quantize the cut tensor to `b` bits. One training run serves every candidate. The optimum tends toward `j = N` unless `j < N` is imposed.
- Form J restricted to `j < N` and to cuts after temporal pooling. This keeps Option 3. The architecture still fixes the latent size.
- Form W. Inject a linear bottleneck of width `w` after temporal pooling. Train the encoder and the decoder jointly. Quantize the latent to `b` bits. Each width needs its own training run.

## Decision

The cut is fixed at an injected linear bottleneck placed after temporal pooling.

Algorithm 1 searches over `(w, b)`. The width `w` comes from a declared candidate set `𝒲`. The bit-width `b` comes from `𝔅`.

The encoder and the decoder are trained jointly for each `w`. Each `w` is trained with several seeds.

The contract value, `w = 64` at `b = 8`, is one candidate in the search.

## Consequences

Algorithm 1 in HLD Appendix C replaces `j` with `w`. The body size of path B is `⌈w·b/8⌉`.

`adr/2026-09-26-partition-decision-returns-three-verdicts.md` applies unchanged with `(w, b)` in place of `(j, b)`. `adr/2026-09-26-configuration-binds-at-build-time.md` applies unchanged with `(w, b)` in place of `(j, b)`. In both, a reference to the cut point now reads as a reference to the width.

The binding ADR bounds its image count without the factor of `W`. The encoder and the decoder process a whole window, so their costs depend on `W`. The cost profile therefore needs one encoder image on S and one decoder image on R for each `(W, w)`. Packing and unpacking are the only parts that depend on `b`. The profile measures them by repetition, per thinkbook §4.2, so the image count does not grow with `|𝔅|`.

The latent is signed, because the bottleneck is linear. Bit-packing uses signed two's complement for every `b`.

The latent size does not depend on `W`. The compression-ratio ADR stays valid.

Contract Q1 keeps its structure. A one-frame arm satisfies `w·b/8 + h_msg ≤ L − h_frag`. An arm that tests H1 on the semantic path may exceed that bound on purpose.

Seed variance enters `Q(w, b)`. The candidate table records the spread of `Q` across seeds.

Watch for a width set so narrow that the latent approaches the class count. A latent that small behaves like a decision, which is Option 2.

The stage placement of the `(w, b)` sweep is not decided here. HLD O10 carries it.
