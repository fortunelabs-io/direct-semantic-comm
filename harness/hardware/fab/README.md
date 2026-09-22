# Fabrication outputs

*What a fabricator receives, and nothing else: Gerbers, drill files, the
position file, and the BOM.*

`mise run fab` writes them here from the one KiCad project in
[`../kicad/`](../kicad/). The stackup is read from the board file rather than
assumed, so the copper-layer set matches whatever the layout actually has.

**Tracked, unlike [`../reports/`](../README.md).** These are the exact files sent
out. `mise run fab-parity` regenerates them into a scratch directory and asserts
the committed copies are the ones the board file produces, because a stale Gerber
and a fresh one are indistinguishable to review and to a diff. The git SOP tags
the commit carrying them `harness-v<N>-fab`.

**Do not hand-edit anything here.** A hand-edited fab output is not what the
board produces, and parity is the only thing that can catch it.

Empty until `mise run fab` runs against a real project.
