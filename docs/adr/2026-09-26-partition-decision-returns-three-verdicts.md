# Partition decision returns three verdicts and records the candidate table

Date: 2026-09-26
Status: Accepted

## Context

The first draft of Algorithm 1 (HLD Appendix C) returned the boolean `C_B < C_A`. A flowchart of that draft on 2026-09-26 showed three defects. An empty feasible set `F` left the argmin undefined. The boolean merged two different ways for path B to lose, a loss on quality and a loss on cost, and those two losses point to different fixes. `C_A` and `C_B` are both pairs `(E/W, T)`, so the comparison did not state which component it compared.

The draft also evaluated `Q` for every candidate `(j, b)` and kept only `(j*, b*)`. That discarded the rest of the cost-quality frontier, which the draft had already computed. If nothing is decided, a gate records a verdict that cannot say why path B lost, and an empty `F` has no defined outcome.

## Alternatives considered

- Keep the boolean return and treat an empty `F` as `C_B = ∞`. This defines the empty case. It still merges the two losses.
- Return three verdicts and record only `(j*, b*)`.
- Return three verdicts and record the full candidate table.
- Sort candidates by cost and evaluate `Q` lazily until the first feasible pair. This yields the same `(j*, b*)` with fewer evaluations of `Q`. It does not produce the frontier.

## Decision

Algorithm 1 evaluates every candidate once into a candidate table `Φ`. Each row of `Φ` holds `(j, b, Q(j, b), COST(j, b))`. `F` is the set of rows in `Φ` whose quality is at least `Q(0, ·) − ε`.

If `F` is empty, the verdict is `A_QUALITY`. In that case `j*`, `b*`, and `C_B` are undefined.

If `F` is not empty and `C_B.x < C_A.x`, the verdict is `B_WINS`. If `F` is not empty and `C_B.x ≥ C_A.x`, the verdict is `A_COST`. The component `x` is the one that `OptTarget` names, `E/W` or `T`.

`prediction.json` records `(j*, b*, C_A, C_B, Φ, v)` before any harness run.

## Consequences

The `prediction.json` schema carries a verdict field with three values. The schema also carries the full candidate table.

A gate can report why path B lost. `A_QUALITY` points to the extractor. `A_COST` points to the link economics, `W`, or the cut point.

A reviewer of a gate can read how close the losing candidates came from `Φ`.

Full evaluation costs `N × |𝔅|` evaluations of `Q` for each `W`. At tens of candidates this cost is small. If the evaluation of `Q` becomes expensive, lazy evaluation is the fallback. Lazy evaluation loses the frontier. It needs its own ADR.

This decision holds with or without the `(j, b)` search space of HLD O8. If the fixed 64 int8 latent remains, `Φ` holds one row. `F` holds one configuration or none.

The `prediction.json` schema becomes a reusable asset. Its writer needs a `DECISION_LOG.md` at its root once it exists.
