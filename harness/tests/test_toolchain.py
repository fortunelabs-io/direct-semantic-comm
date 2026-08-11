"""Grade the Tier 0 toolchain gate from its results file.

Pure: reads ../results/stage0_toolchain.json, computes, exits. Silent on
success with exit 0, one line per failure and exit 1 otherwise. It never
prompts and never touches hardware -- the capture side of this gate lives
in ../scripts/toolchain.sh, and that split is why the two directories
exist.

The criterion is stated in todos/stage0_todo.md and is not restated here
beyond what the checks below encode: a blink builds and flashes on the
selected part, and the two I2C peripherals are recorded from the datasheet
rather than from a product page.
"""

import json
import sys
from pathlib import Path

HARNESS = Path(__file__).resolve().parent.parent
RESULTS = HARNESS / "results" / "stage0_toolchain.json"

# The part the specification requires be named in .mise.toml. Duplicated
# here on purpose: if the two disagree, the disagreement is the finding.
EXPECTED_PART = "STM32F411CEU6"

failures: list[str] = []


def fail(message: str) -> None:
    failures.append(message)


if not RESULTS.exists():
    print(f"{RESULTS} does not exist; the gate has not been run")
    sys.exit(1)

try:
    result = json.loads(RESULTS.read_text())
except json.JSONDecodeError as exc:
    print(f"{RESULTS} is not valid JSON: {exc}")
    sys.exit(1)

# --- the run itself -------------------------------------------------------

if result.get("status") != "pass":
    fail(f"status is {result.get('status')!r}, failed at stage "
         f"{result.get('failed_stage')!r}")

# A capture taken under a leaked environment is well-formed and wrong.
if result.get("doctor") != "clean":
    fail(f"doctor was {result.get('doctor')!r} for the run that produced "
         f"this file")

# "Which commit produced this number" has no answer if the tree was dirty.
if result.get("git_dirty") is not False:
    fail("the working tree was dirty when this result was captured")

if not result.get("git_commit"):
    fail("no git commit recorded")

# --- the criterion --------------------------------------------------------

if result.get("blink_observed") != "yes":
    fail(f"blink_observed is {result.get('blink_observed')!r}; the "
         f"criterion requires a blink that flashes, not one that links")

if result.get("part") != EXPECTED_PART:
    fail(f"part is {result.get('part')!r}, expected {EXPECTED_PART!r}")

if not result.get("part_datasheet"):
    fail("no datasheet named; the criterion requires the I2C peripherals "
         "be recorded from the datasheet rather than a product page")

# --- the two I2C masters --------------------------------------------------

i2c = result.get("i2c_peripherals", [])

if len(i2c) != 2:
    fail(f"{len(i2c)} I2C peripherals recorded, expected 2")
else:
    instances = [bus.get("instance") for bus in i2c]
    if len(set(instances)) != 2:
        fail(f"the two I2C records name the same instance: {instances}")

    # "Two I2C masters on separate pins" is the binding constraint from
    # the part ADR. Parts listing two instances often mux them onto
    # overlapping pin groups, so the pins are checked, not the count.
    pins: list[str] = []
    for bus in i2c:
        for line in ("scl", "sda"):
            entry = bus.get(line)
            if not isinstance(entry, dict) or not entry.get("pin"):
                fail(f"{bus.get('instance')} has no {line} pin recorded")
                continue
            if entry.get("af") is None:
                fail(f"{bus.get('instance')} {line} has no alternate "
                     f"function recorded")
            pins.append(entry["pin"])

    if len(pins) != len(set(pins)):
        fail(f"the two I2C instances share a pin: {pins}")

# --- report ---------------------------------------------------------------

if failures:
    for message in failures:
        print(message)
    sys.exit(1)

sys.exit(0)
