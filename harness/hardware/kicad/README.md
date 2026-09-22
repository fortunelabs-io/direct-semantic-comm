# KiCad project

*The one KiCad project for the harness lives here. Everything else in
[`../`](../) reasons about the files in this folder without opening them.*

The project is four files sharing one stem: `.kicad_pro`, `.kicad_sch`,
`.kicad_pcb`, and the custom-rules file `.kicad_dru`. They stay together in this
folder. KiCad references them by relative path, and [`../check.py`](../check.py)
derives the schematic and layout as siblings of the `.kicad_pro` it discovers.
Splitting them into per-type folders breaks both the editor and the checker.

**One project, not several.** `check.py` refuses more than one `.kicad_pro`, for
the reason [`../review_checklist.md`](../review_checklist.md) gives under
"Identical channels": the harness is one design repeated.

## Where the rules live

- **ERC severities and DRC constraints** are stored inside the `.kicad_pro`, set
  in Board Setup. A violation that is genuinely acceptable is excluded there, in
  the diff, per the severity policy in [`../README.md`](../README.md), not in the
  checker where it would be invisible.
- **Custom DRC rules**, if any, go in `<project>.kicad_dru`, next to the project.

## Where the BOM lives

Not here. `mise run fab` generates it from the schematic into [`../fab/`](../fab/)
as `<project>-bom.csv`. A hand-kept BOM would be a second source for a fact the
schematic already carries.

## Nothing is committed here yet

Until a `.kicad_pro` appears, `mise run hw` reports exit 2, absent, which is not
a pass. See [`../README.md`](../README.md), "Order of work", for why fabrication
is the last thing started.
