"""Assert the host environment is isolated. Run before trusting any capture.

Not a gate and not a capture script. It touches no hardware and reads no
results. It exists because this machine carries a mise config at the home
directory that applies to every project on it, and a capture taken under a
leaked environment is well-formed and wrong -- the same failure mode the
negative control exists to catch one layer down.

Exit 0 with every check passing, 1 otherwise.
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path

HARNESS = Path(__file__).resolve().parent
VENV = HARNESS / ".venv"
PINNED_PYTHON = "3.12"

results: list[tuple[bool, str, str]] = []


def check(ok: bool, name: str, detail: str) -> None:
    results.append((ok, name, detail))


# --- interpreter identity -------------------------------------------------

# sys.prefix, not sys.executable: .venv/bin/python is a symlink into mise's
# install directory, so resolving the executable walks straight out of the venv
# and would fail this check on a correctly isolated environment.
check(
    Path(sys.prefix).resolve() == VENV,
    "interpreter is this project's venv",
    sys.prefix,
)

version = f"{sys.version_info.major}.{sys.version_info.minor}"
check(
    version == PINNED_PYTHON,
    f"interpreter is the pinned {PINNED_PYTHON}",
    f"{version}.{sys.version_info.micro}",
)

venv_env = os.environ.get("VIRTUAL_ENV", "")
check(
    bool(venv_env) and Path(venv_env).resolve() == VENV,
    "VIRTUAL_ENV points here",
    venv_env or "(unset)",
)

# --- sys.path hygiene -----------------------------------------------------
# The specific leak: ~/.mise.toml sets PYTHONPATH to the home directory, which
# lands ahead of the standard library and makes any stray .py there importable.

home = Path.home().resolve()
on_path = [p for p in sys.path if p and Path(p).resolve() == home]
check(
    not on_path,
    "home directory is not on sys.path",
    "clean" if not on_path else f"LEAKED: {on_path[0]}",
)

foreign = [
    p
    for p in sys.path
    if p
    and Path(p).exists()
    and not Path(p).resolve().is_relative_to(HARNESS)
    and not Path(p).resolve().is_relative_to(Path(sys.prefix).resolve())
    and not Path(p).resolve().is_relative_to(Path(sys.base_prefix).resolve())
]
check(
    not foreign,
    "no foreign paths on sys.path",
    "clean" if not foreign else "; ".join(foreign),
)

# --- pinned dependencies --------------------------------------------------

req = HARNESS / "requirements.txt"
pins = {}
for line in req.read_text().splitlines():
    line = line.strip()
    if line and not line.startswith("#") and "==" in line:
        name, _, ver = line.partition("==")
        pins[name.lower()] = ver

from importlib.metadata import PackageNotFoundError, version as dist_version  # noqa: E402

for name, want in sorted(pins.items()):
    try:
        got = dist_version(name)
    except PackageNotFoundError:
        check(False, f"{name} installed", "MISSING")
        continue
    check(got == want, f"{name}=={want}", got if got != want else "ok")

# --- ESP-IDF, needed from Tier 2 -----------------------------------------
# idf.py carries `#!/usr/bin/env python`, so it runs under whatever interpreter
# is first on PATH. That is this venv, which has no IDF tooling in it. Reported
# rather than asserted: Tier 0 and Tier 1 do not need it.

idf = shutil.which("idf.py")
if idf:
    probe = subprocess.run(
        [sys.executable, "-c", "import click, esp_idf_monitor"],
        capture_output=True,
    )
    if probe.returncode != 0:
        print(
            "NOTE  idf.py is on PATH but resolves to this venv, which has no IDF\n"
            "      tooling. That is the isolation working, not a fault. Build DUT\n"
            "      firmware with `mise run idf -- build`, which names the ESP-IDF\n"
            f"      interpreter explicitly. idf.py: {idf}\n",
            file=sys.stderr,
        )

# --- report ---------------------------------------------------------------

width = max(len(n) for _, n, _ in results)
failed = 0
for ok, name, detail in results:
    if not ok:
        failed += 1
    print(f"{'PASS' if ok else 'FAIL'}  {name:<{width}}  {detail}")

if failed:
    print(f"\n{failed} check(s) failed. Do not trust a capture taken here.", file=sys.stderr)
    sys.exit(1)

sys.exit(0)
