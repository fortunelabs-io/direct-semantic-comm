# Hardware design pipeline

*Everything provable about the board without touching it. One checker,
[`check.py`](./check.py), run identically at the desk and in CI, so that "clean
locally" and "clean in CI" are one claim rather than two that happen to agree.*

**Status: the pipeline exists, the KiCad project does not.** Every command below
exits 2 today, and says so. See [`review_checklist.md`](./review_checklist.md)
for why fabrication is deliberately the last thing started.

---

## What this proves, and what it cannot

It proves the layout is manufacturable and that it matches the schematic it was
generated from. That last part is the one worth having: a layout that drifted
from its schematic survives a careful reading of either file alone.

It proves nothing about whether the schematic is right. The two errors that
[`review_checklist.md`](./review_checklist.md) calls not negotiable — low-side
sensing, and a Kelvin trace carrying load current — are both electrically legal
and produce a clean ERC and a clean DRC while biasing every measurement the
harness will ever take. No automated check distinguishes them from a correct
board. A person does, before Gerbers are exported.

So this pipeline is a precondition for spending money, not evidence about the
harness. **None of it is a gate.** It writes no `results/*.json` and closes
nothing in [`../../todos/stage0_todo.md`](../../todos/stage0_todo.md); the
Tier 3 gates are what prove the fabricated board works.

---

## Commands

Run from `harness/`. All five are `mise` tasks so they run under the same pinned
interpreter as everything else, though `check.py` imports nothing outside the
standard library — what is actually pinned here is `kicad-cli`.

| Command | What it does |
|---|---|
| `mise run erc` | electrical rules on the schematic |
| `mise run drc` | design rules on the layout, **and parity with the schematic** |
| `mise run hw` | both, and the second runs even when the first fails |
| `mise run fab` | Gerbers, drill, position and BOM into `fab/` |
| `mise run fab-parity` | asserts the committed `fab/` is what this board produces |

Reports land in `reports/` as JSON and are not tracked: they are regenerable
from tracked source by a pinned checker, and a tracked report would be a number
living in two places. `fab/` **is** tracked — it is what a fabricator receives.

## Four outcomes, kept apart

| Exit | Meaning |
|---|---|
| 0 | checked, clean |
| 1 | checked, violations found — the design is wrong |
| 2 | nothing to check — no KiCad project exists yet |
| 3 | cannot check — `kicad-cli` missing, off the pin, or more than one project |

**Exit 2 is not exit 0, and it fails CI.** A checker that returns success for a
board that does not exist reports a closed gate on work never done — the same
failure as an empty test file that exits 0, which
[`../README.md`](../README.md) refuses for the same reason. Today every command
here exits 2. That is the honest state, and it is loud.

Exit 3 is separate from exit 1 because "the board is wrong" and "the checker
could not run" get the same red tick otherwise, and they call for opposite
responses.

---

## The pin

`kicad-cli` **9.0.x**, asserted by `check.py` before it will report anything. CI
runs `kicad/kicad:9.0.9`, which is the version installed on the development
machine.

Pinned to the series rather than the patch: the rule engine and the default
severity assignments move between series, so a board shown clean under 9.0 at
the desk is not shown clean under 10.0, and 9.0.x fixes do not move the rule
set. The patch level is recorded in every report instead of enforced.

Bumping is deliberate — `KICAD_SERIES` in `check.py`, the image tag in both
workflows, and a re-run of every check before the first result from the new
series is trusted.

Two things the checker fixes about its own environment:

- **Language.** `kicad-cli` translates violation text and takes the language
  from KiCad's settings, not from the shell locale, so `LC_ALL` does nothing.
  The development machine is set to Simplified Chinese and CI would report in
  English — the same board producing two reports that cannot be compared and
  neither of which can be quoted in an issue the other author reads. The checker
  copies the existing configuration to a scratch directory and overrides one
  key. It copies rather than starting empty because the library tables live in
  there, and a checker without them reports footprints missing that are present.
  Your own configuration is read and never written.
- **Stackup.** The Gerber layer list is read out of the `.kicad_pcb` rather than
  hardcoded. A hardcoded list is wrong exactly once, silently, in the direction
  that drops a plane from the outputs.

## Severity policy

**Warnings fail alongside errors.** The checks that fire at all on a board this
size are cheap to hold at zero, and a tolerated warning list is precisely how a
real one goes unread.

A violation that is genuinely acceptable is excluded **in the KiCad project
file**, where the exclusion is reviewed and carried in the diff — not tolerated
in the checker, where it would be invisible. `check.py` runs with
`--severity-all` and counts exclusions separately, so an excluded violation
still appears in the report.

---

## CI

Two workflows, because they answer different questions at different moments.

**[`hardware.yml`](../../.github/workflows/hardware.yml)** — ERC, DRC and
schematic parity on every push and PR that touches `harness/hardware/**`.
Reports upload as artefacts. The job summary explains which of the three
failures fired.

**[`hardware-fab.yml`](../../.github/workflows/hardware-fab.yml)** — on a
`harness-v*-fab` tag only. Runs the rules *and* regenerates the fabrication
outputs to compare against the committed ones.

That second check is the point of the tag. [`../../docs/sop/git_sop.md`](../../docs/sop/git_sop.md)
puts `harness-v<N>-fab` on the commit carrying the exact Gerbers sent out, so
that when board seven behaves differently from board two, the tag is what says
they should not have. A stale Gerber and a fresh one are indistinguishable to
review, and a diff cannot tell them apart either. Regenerating and comparing is
the only thing that can. Export timestamps and toolchain build strings are
ignored in the comparison; everything else must match.

---

## Order of work

1. KiCad project appears under `harness/hardware/`, one `.kicad_pro` — the
   harness is one design repeated, not several, and `check.py` refuses to run
   against more than one.
2. `mise run hw` clean.
3. **[`review_checklist.md`](./review_checklist.md) walked by a person.** This
   is the step the pipeline cannot do, and the only one that catches the errors
   that matter.
4. `mise run fab`, commit the outputs.
5. `mise run fab-parity` clean, then tag `harness-v1-fab` and push.
6. Boards arrive. `mise run incoming` — and from there it is gates, not checks.
