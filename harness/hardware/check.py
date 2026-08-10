"""Electrical and design rule checks for the harness PCB, and the fabrication
outputs derived from it.

Not a gate. It touches no hardware, reads no results, and produces no number
that a claim rests on. It runs at the desk and in CI against the same pinned
kicad-cli, and it is the last thing between a layout error and ten fabricated
boards that all share it.

Four outcomes, kept distinct on purpose:

    0  checked, clean
    1  checked, violations found        the design is wrong
    2  nothing to check                 no KiCad project exists yet
    3  cannot check as specified        kicad-cli missing, off the pin, or
                                        more than one project in the tree

Exit 2 is not exit 0. The harness has no KiCad project yet, and a checker that
returns success for a board that does not exist reports a closed gate on work
never done -- the same failure as an empty test file that exits 0, which
``../README.md`` refuses for the same reason. Absent is never success.

What this cannot do is the point of ``review_checklist.md``. The two errors that
bias every measurement silently -- low-side sensing, and a Kelvin trace carrying
load current -- are both electrically legal and produce a clean ERC and a clean
DRC. This file proves the layout is manufacturable and matches its schematic. It
does not prove the schematic is right. Nothing automated does.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HARDWARE = Path(__file__).resolve().parent
REPORTS = HARDWARE / "reports"
FAB = HARDWARE / "fab"

EXIT_CLEAN = 0
EXIT_VIOLATIONS = 1
EXIT_ABSENT = 2
EXIT_UNRUNNABLE = 3

# Pinned to the series, not the patch. The rule engine and the default severity
# assignments change between series -- a board shown clean under 9.0 at the desk
# is not shown clean under 10.0 in CI, and CI exists here to reproduce the desk
# rather than to hold a second opinion. Patch level is recorded in every report
# instead of enforced, because 9.0.x fixes do not move the rule set and pinning
# a patch would break CI on an unrelated upgrade.
#
# Bumping this is a deliberate act: change the series here, change the image tag
# in ../../.github/workflows/hardware.yml to match, and re-run every check
# before trusting the first result the new series produces.
KICAD_SERIES = "9.0"

# Warnings fail alongside errors. On this board the checks that fire at all are
# cheap to hold at zero, and a tolerated warning list is exactly how a real one
# goes unread. A violation that is genuinely acceptable is excluded in the
# project file, where the exclusion is reviewed and carried in the diff, rather
# than tolerated here where it would be invisible.
FAIL_SEVERITIES = {"error", "warning"}

# DRC and ERC reports each scatter findings across several arrays.
VIOLATION_ARRAYS = ("violations", "unconnected_items", "schematic_parity")

# Lines carrying the export timestamp and toolchain build. They differ on every
# run of an unchanged board, so parity comparison ignores them.
VOLATILE = re.compile(
    r"CreationDate|G04 Created by|GenerationSoftware|^; DRILL file|^;FORMAT=|"
    r"^%TF\.SameCoordinates|Created on|^\.CreationDate",
    re.IGNORECASE,
)


# Populated in main() with a scratch KICAD_CONFIG_HOME. See neutral_config().
ENV = {**os.environ}


def neutral_config(scratch: Path) -> str:
    """Give kicad-cli a config that reports in English, without touching the GUI's.

    kicad-cli translates violation text, and it takes the language from KiCad's
    own settings rather than from the shell locale, so LC_ALL does nothing. On
    this machine the setting is Simplified Chinese and every report came back in
    Chinese; the CI container's default would come back in English. Same board,
    same pinned series, two reports that cannot be compared and neither of which
    can be quoted in an issue the other author reads.

    The fix copies the existing config and overrides one key. Copying rather
    than starting empty is the point: the library tables and the KICAD_USER_*
    paths live in there, and a checker running without them would report
    footprints missing that are present, which is a false violation -- the one
    failure mode worth more than the inconvenience it was avoiding.

    The operator's own configuration is read and never written.
    """
    source = Path(os.environ.get("KICAD_CONFIG_HOME") or Path.home() / ".config" / "kicad")
    target = scratch / KICAD_SERIES

    if (source / KICAD_SERIES).is_dir():
        shutil.copytree(source / KICAD_SERIES, target)
    else:
        # No configuration on this machine -- a fresh CI container. kicad-cli
        # writes its own defaults, which resolve the stock libraries.
        target.mkdir(parents=True)

    common = target / "kicad_common.json"
    settings = json.loads(common.read_text()) if common.exists() else {}
    settings.setdefault("system", {})["language"] = "Default"
    common.write_text(json.dumps(settings, indent=2))

    return str(scratch)


class Unrunnable(Exception):
    """The check cannot be performed as specified."""


class Absent(Exception):
    """There is nothing to check yet."""


# --- environment ----------------------------------------------------------


def kicad_cli() -> str:
    cli = shutil.which("kicad-cli")
    if cli is None:
        raise Unrunnable(
            "kicad-cli is not on PATH. Install KiCad "
            f"{KICAD_SERIES}.x, or run this under the pinned container named in "
            ".github/workflows/hardware.yml"
        )
    return cli


def kicad_version(cli: str) -> str:
    out = subprocess.run([cli, "--version"], capture_output=True, text=True, env=ENV)
    found = re.search(r"\d+\.\d+\.\d+", out.stdout + out.stderr)
    if found is None:
        raise Unrunnable(f"kicad-cli --version returned nothing parseable: {out.stdout!r}")
    return found.group(0)


def assert_pinned(cli: str) -> str:
    version = kicad_version(cli)
    if not version.startswith(f"{KICAD_SERIES}."):
        raise Unrunnable(
            f"kicad-cli is {version}, the pin is {KICAD_SERIES}.x. "
            "A result from an unpinned series says nothing about the pinned one."
        )
    return version


# --- discovery ------------------------------------------------------------


class Project:
    def __init__(self, pro: Path) -> None:
        self.pro = pro
        self.sch = pro.with_suffix(".kicad_sch")
        self.pcb = pro.with_suffix(".kicad_pcb")
        self.name = pro.stem


def discover() -> Project:
    pros = [p for p in sorted(HARDWARE.rglob("*.kicad_pro")) if "-backups" not in p.parts]

    if not pros:
        raise Absent(
            f"no .kicad_pro under {HARDWARE.name}/. The KiCad project is planned, "
            "not started -- see review_checklist.md."
        )
    if len(pros) > 1:
        listed = ", ".join(str(p.relative_to(HARDWARE)) for p in pros)
        raise Unrunnable(
            f"{len(pros)} KiCad projects found: {listed}. The harness is one "
            "design repeated, not several -- review_checklist.md, 'Identical "
            "channels'. Fix the tree or fix the checklist; do not check both."
        )
    return Project(pros[0])


# --- report parsing -------------------------------------------------------


def collect(node: object, found: list[dict]) -> None:
    """Walk the report and gather every violation array, wherever it sits.

    ERC nests findings one level down per sheet and DRC keeps three arrays at
    the top; walking rather than indexing means a report shape that shifts in a
    later release degrades into finding nothing new, not into a crash that gets
    worked around under time pressure before a fabrication run.
    """
    if isinstance(node, dict):
        for key, value in node.items():
            if key in VIOLATION_ARRAYS and isinstance(value, list):
                found.extend(v for v in value if isinstance(v, dict))
            else:
                collect(value, found)
    elif isinstance(node, list):
        for item in node:
            collect(item, found)


def counts(violations: list[dict]) -> tuple[list[dict], dict[str, int]]:
    failing, tally = [], {}
    for v in violations:
        severity = str(v.get("severity", "error")).lower()
        if v.get("excluded") or severity in ("ignore", "exclusion"):
            severity = "excluded"
        tally[severity] = tally.get(severity, 0) + 1
        if severity in FAIL_SEVERITIES:
            failing.append(v)
    return failing, tally


def describe(v: dict) -> str:
    text = str(v.get("description") or v.get("type") or "unnamed violation")
    where = ""
    for item in v.get("items") or []:
        if isinstance(item, dict) and item.get("description"):
            where = f"  [{item['description']}]"
            break
    return f"{str(v.get('severity', '?')).upper():<8}{text}{where}"


# --- checks ---------------------------------------------------------------


def rule_check(kind: str, project: Project, cli: str, version: str) -> int:
    source = project.sch if kind == "erc" else project.pcb
    if not source.exists():
        raise Absent(
            f"{project.name}.kicad_pro exists but {source.name} does not. "
            f"{kind.upper()} has nothing to read."
        )

    REPORTS.mkdir(parents=True, exist_ok=True)
    report = REPORTS / f"{kind}.json"

    command = [cli, "sch" if kind == "erc" else "pcb", kind]
    if kind == "drc":
        # Parity is why DRC is worth running headless at all. It catches the
        # layout that drifted from the schematic it was generated from, which is
        # the failure that survives a visual review of either file alone.
        command.append("--schematic-parity")
    command += ["--format", "json", "--severity-all", "--units", "mm",
                "-o", str(report), str(source)]

    run = subprocess.run(command, capture_output=True, text=True, env=ENV)
    if not report.exists():
        raise Unrunnable(
            f"kicad-cli {kind} wrote no report (exit {run.returncode})\n"
            f"{run.stderr.strip() or run.stdout.strip()}"
        )

    violations: list[dict] = []
    collect(json.loads(report.read_text()), violations)
    failing, tally = counts(violations)

    summary = ", ".join(f"{n} {s}" for s, n in sorted(tally.items())) or "nothing reported"
    print(f"{kind.upper()}  {source.name}  kicad-cli {version}")
    print(f"  {summary}")
    print(f"  report: {report.relative_to(HARDWARE.parent.parent)}")

    if not failing:
        return EXIT_CLEAN

    print()
    for v in failing:
        print(f"  {describe(v)}")
    print(f"\n{len(failing)} violation(s) at {'/'.join(sorted(FAIL_SEVERITIES))}.")
    return EXIT_VIOLATIONS


# --- fabrication outputs --------------------------------------------------


def copper_layers(pcb: Path) -> list[str]:
    """Read the stackup from the board rather than assuming two layers.

    A hardcoded layer list is wrong exactly once, silently, in the direction
    that drops a plane from the Gerbers and is only discovered by a fabricator
    or by an assembled board that does not work.
    """
    block = re.search(r"\(layers\s*(.*?)\n\s*\)", pcb.read_text(), re.DOTALL)
    if block is None:
        raise Unrunnable(f"no (layers ...) block in {pcb.name}")
    names = re.findall(r'\(\s*\d+\s+"([^"]+)"', block.group(1))
    copper = [n for n in names if n.endswith(".Cu")]
    if not copper:
        raise Unrunnable(f"no copper layers parsed from {pcb.name}")
    return copper


def export_fab(project: Project, cli: str, version: str, into: Path) -> None:
    if not project.pcb.exists():
        raise Absent(f"{project.pcb.name} does not exist. Nothing to fabricate.")

    into.mkdir(parents=True, exist_ok=True)
    copper = copper_layers(project.pcb)
    layers = copper + [
        "F.Silkscreen", "B.Silkscreen",
        "F.Mask", "B.Mask",
        "F.Paste", "B.Paste",
        "Edge.Cuts",
    ]

    steps = [
        ["pcb", "export", "gerbers", "--layers", ",".join(layers),
         "--no-protel-ext", "--use-drill-file-origin", "-o", str(into),
         str(project.pcb)],
        ["pcb", "export", "drill", "--format", "excellon", "--excellon-units", "mm",
         "--excellon-separate-th", "--drill-origin", "plot", "--generate-map",
         "--map-format", "gerberx2", "-o", str(into), str(project.pcb)],
        ["pcb", "export", "pos", "--format", "csv", "--units", "mm", "--side", "both",
         "--use-drill-file-origin", "--exclude-dnp",
         "-o", str(into / f"{project.name}-pos.csv"), str(project.pcb)],
    ]
    if project.sch.exists():
        steps.append(
            ["sch", "export", "bom", "--group-by", "Value,Footprint",
             "--fields", "Reference,Value,Footprint,${QUANTITY},${DNP}",
             "-o", str(into / f"{project.name}-bom.csv"), str(project.sch)],
        )

    for step in steps:
        run = subprocess.run([cli, *step], capture_output=True, text=True, env=ENV)
        if run.returncode != 0:
            raise Unrunnable(
                f"kicad-cli {' '.join(step[:3])} failed (exit {run.returncode})\n"
                f"{run.stderr.strip() or run.stdout.strip()}"
            )

    print(f"FAB   {project.pcb.name}  kicad-cli {version}")
    print(f"  stackup: {len(copper)} copper layer(s) -- {', '.join(copper)}")
    print(f"  {len(sorted(into.iterdir()))} file(s) in {into}")


def normalise(path: Path) -> list[str]:
    text = path.read_text(errors="replace").replace("\r\n", "\n")
    return [line for line in text.split("\n") if not VOLATILE.search(line)]


def fab_parity(project: Project, cli: str, version: str) -> int:
    """Assert the committed fabrication outputs are the ones this board produces.

    The git SOP puts a ``harness-v<N>-fab`` tag on the commit carrying the exact
    Gerbers sent out, so that ten boards can be compared against one commit.
    That tag is worth something only if the Gerbers under it were actually
    generated from the board file under it. Regenerating and comparing is the
    only thing that shows it; a diff cannot, because a stale Gerber and a fresh
    one look identical to review.
    """
    if not FAB.exists() or not any(FAB.iterdir()):
        raise Absent(
            f"no fabrication outputs under {FAB.name}/. Run `mise run fab` and "
            "commit them before tagging."
        )

    with tempfile.TemporaryDirectory() as tmp:
        fresh = Path(tmp) / "fab"
        export_fab(project, cli, version, fresh)

        committed = {p.name for p in FAB.iterdir() if p.is_file()}
        regenerated = {p.name for p in fresh.iterdir() if p.is_file()}

        missing = sorted(regenerated - committed)
        extra = sorted(committed - regenerated)
        differing = sorted(
            name for name in committed & regenerated
            if normalise(FAB / name) != normalise(fresh / name)
        )

    print(f"\nPARITY  {len(committed)} committed vs {len(regenerated)} regenerated")
    if not (missing or extra or differing):
        print("  committed outputs match the board file")
        return EXIT_CLEAN

    for name in missing:
        print(f"  MISSING    {name}  regenerated but not committed")
    for name in extra:
        print(f"  ORPHANED   {name}  committed but no longer produced")
    for name in differing:
        print(f"  STALE      {name}  committed copy differs from the board file")
    print("\nThe committed outputs are not what this board produces. "
          "Re-run `mise run fab` and commit, or do not tag this commit.")
    return EXIT_VIOLATIONS


# --- entry ----------------------------------------------------------------


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument(
        "command",
        choices=["version", "erc", "drc", "check", "fab", "fab-parity"],
        help="version: assert the pin. check: erc then drc. "
             "fab: write fabrication outputs. fab-parity: assert the committed "
             "outputs match the board.",
    )
    command = parser.parse_args().command

    try:
        cli = kicad_cli()
        version = assert_pinned(cli)

        if command == "version":
            print(f"kicad-cli {version}, on the {KICAD_SERIES}.x pin")
            return EXIT_CLEAN

        with tempfile.TemporaryDirectory(prefix="kicad-config-") as scratch:
            ENV["KICAD_CONFIG_HOME"] = neutral_config(Path(scratch))

            project = discover()
            print(f"project: {project.pro.relative_to(HARDWARE.parent.parent)}\n")

            if command in ("erc", "drc"):
                return rule_check(command, project, cli, version)
            if command == "check":
                # Both run even when the first fails. Two reports from one
                # invocation beats fixing ERC only to find DRC on the next pass.
                erc = rule_check("erc", project, cli, version)
                print()
                drc = rule_check("drc", project, cli, version)
                return max(erc, drc)
            if command == "fab":
                export_fab(project, cli, version, FAB)
                return EXIT_CLEAN
            if command == "fab-parity":
                return fab_parity(project, cli, version)

    except Absent as absent:
        print(f"ABSENT: {absent}", file=sys.stderr)
        print("Nothing was checked. This is not a pass.", file=sys.stderr)
        return EXIT_ABSENT
    except Unrunnable as broken:
        print(f"UNRUNNABLE: {broken}", file=sys.stderr)
        return EXIT_UNRUNNABLE
    except OSError as io:
        # Typically a read-only or wrong-owner workspace, which is a container
        # invocation error rather than anything about the board. It surfaced
        # first as a traceback, and a traceback in CI reads as the checker being
        # broken -- which is indistinguishable, at a glance, from the board
        # being fine.
        print(f"UNRUNNABLE: {io}", file=sys.stderr)
        return EXIT_UNRUNNABLE

    return EXIT_UNRUNNABLE


if __name__ == "__main__":
    sys.exit(main())
