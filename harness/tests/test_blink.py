"""Grade the Tier 1 blink gate from its results file.

Pure: reads ../results/stage0_blink.json, computes, exits. Silent on
success with exit 0, one line per failure and exit 1 otherwise.

The claim is that the cross toolchain produces an image the selected part
executes. It was half of the Tier 0 gate's criterion until issue #20 split
them: flashing is powering, and Tier 0 is defined as nothing powered.
"""

import json
import sys
from pathlib import Path

HARNESS = Path(__file__).resolve().parent.parent
RESULTS = HARNESS / "results" / "stage0_blink.json"
TOOLCHAIN_RESULTS = HARNESS / "results" / "stage0_toolchain.json"

EXPECTED_PART = "STM32F411CEU6"

failures: list[str] = []


def fail(message: str) -> None:
    failures.append(message)


if not RESULTS.exists():
    print(f"{RESULTS} does not exist; the gate has not been run")
    sys.exit(1)

# Nothing below an open gate gets run, and the gate above this one leaves
# its file behind. Absent it, this result was captured out of order.
if not TOOLCHAIN_RESULTS.exists():
    print(f"{TOOLCHAIN_RESULTS} is absent; the Tier 0 gate above this one "
          f"has not run")
    sys.exit(1)

try:
    result = json.loads(RESULTS.read_text())
except json.JSONDecodeError as exc:
    print(f"{RESULTS} is not valid JSON: {exc}")
    sys.exit(1)

if result.get("status") != "pass":
    fail(f"status is {result.get('status')!r}, failed at stage "
         f"{result.get('failed_stage')!r}")

if result.get("doctor") != "clean":
    fail(f"doctor was {result.get('doctor')!r} for the run that produced "
         f"this file")

if result.get("git_dirty") is not False:
    fail("the working tree was dirty when this result was captured")

if not result.get("git_commit"):
    fail("no git commit recorded")

if result.get("part") != EXPECTED_PART:
    fail(f"part is {result.get('part')!r}, expected {EXPECTED_PART!r}")

# Linking and running are different claims. A build that flashes without
# error still proves nothing about whether the part executed it.
if result.get("blink_observed") != "yes":
    fail(f"blink_observed is {result.get('blink_observed')!r}; the "
         f"criterion requires a blink that runs, not one that links")

# The toolchain that produced an image is part of what the image is.
toolchain = result.get("toolchain", {})
if not toolchain.get("arm_none_eabi_gcc") or \
        toolchain["arm_none_eabi_gcc"] == "unavailable":
    fail("no arm-none-eabi-gcc version recorded")

if failures:
    for message in failures:
        print(message)
    sys.exit(1)

sys.exit(0)
