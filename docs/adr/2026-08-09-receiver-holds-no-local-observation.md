# Node R holds no local observation, and the replacement-collapse lesson is not carried over

**Date:** 2026-08-09
**Status:** Accepted

## Context

The parent build established, twice, under two objectives and on two corpora,
that substituting the consumer's own representation with a projected one
collapses the consumer onto a constant output, while adding to it works. The
lesson recorded there is that transferred understanding must complement and
never overwrite.

A reader arriving at this project from that lineage will expect Condition B to
preserve something on R and will read its absence as an oversight. Left
unrecorded, this decision would be re-litigated every time someone reads
`FINDINGS.md` and then reads Section 3.

## Alternatives considered

- Give R a cheap local observation — a subsampled or degraded reading of the
  same phenomenon — so the latent adds to something and the design mirrors the
  parent build's fused arm. Rejected for the core runs; retained as a variant.
- Carry the lesson over as stated and treat substitution as a known defect.
  Rejected: it is not a defect here, and treating it as one would misattribute
  any Stage 3 failure.
- Say nothing and let the design speak. Rejected: the design is silent about
  why, and the lineage is loud.

## Decision

In Conditions A and B and in the Stage 5 transfer variant, R does not observe
the raw phenomenon. It receives, and it consumes what it receives.

The parent build's collapse is destructive interference, and it requires the
consumer to hold prior competence for the incoming representation to destroy.
R holds none, so there is nothing for a residual to preserve and substitution
destroys nothing. The mechanism does not transfer.

What does transfer, and is adopted in full, is the failure *signature*: an
output collapsed onto a constant while an aggregate metric reads the collapse
as partial success. That is a property of aggregate reporting rather than of
substitution, and it is guarded against by the per-sample records and
output-distribution check in the sufficiency section, not by the architecture.

## Consequences

Keeps the question this project asks as *what does a transferred representation
cost*, rather than *what does it add*. The local-observation variant would
change the question, which is why it is a variant and not a fix.

Commits the sufficiency check to reporting R's output distribution and its
distance from the reference label distribution, in every condition, since
architecture no longer offers any protection against collapse.

Watch for: if R is ever given local context in a later run, this entry is
superseded rather than amended, and the residual-versus-substitution question
becomes live again on the same terms the parent build settled it.
