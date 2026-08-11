#!/usr/bin/env bash
#
# toolchain.sh - the command behind `mise run toolchain`.
#
# Gate: "the capture engine part satisfies the timing budget",
# todos/stage0_todo.md, Tier 0. Passes when a blink builds and flashes on
# the selected part, and the two I2C peripherals are recorded from the
# datasheet rather than from a product page.
#
# Writes harness/results/stage0_toolchain.json from the real outcome of
# each step. Every exit path writes a result, including the ones this
# script did not anticipate: the ERR trap below is what makes that true,
# and it replaces an earlier arrangement in which a failure during cmake
# configure exited under `set -e` and left no file at all, while the
# header claimed results came from "the real exit codes of each step".
#
# What this script will not do:
#
#   - It does not decide whether a prediction held. The prediction lives
#     in the gate's issue, recorded before the run, and whether it held is
#     a judgement made by a person reading this file afterwards. An
#     earlier revision wrote "prediction_held": true as a constant on the
#     pass path, which is the one field in the record that cannot be
#     recovered if it is wrong.
#   - It does not infer that the blink was seen. Flashing successfully and
#     a visible LED are different claims; the second is confirmed by the
#     operator at the prompt below. Prompting belongs in scripts/ and is
#     the reason scripts/ and tests/ are separate directories.
#
# Requires: arm-none-eabi-gcc, cmake, ninja, st-flash, and an ST-Link
# attached to an STM32F411CEU6 board.

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
HARNESS_DIR="${REPO_ROOT}/harness"
FW_DIR="${HARNESS_DIR}/firmware/capture"
BUILD_DIR="${FW_DIR}/build"
RESULTS_DIR="${HARNESS_DIR}/results"
RESULTS_FILE="${RESULTS_DIR}/stage0_toolchain.json"

PART="STM32F411CEU6"
PART_DATASHEET="DocID026289 Rev 4"

stage="preflight"
doctor_status="not run"
blink_observed="not reached"

mkdir -p "${RESULTS_DIR}"

# --- result record -------------------------------------------------------

# The two I2C peripherals, as required by the gate's criterion and by
# results/README.md. Recorded from Table 9 of the datasheet named above,
# not from a product page or a selector table. The alternate function
# numbers differ between the two buses and that is not a typo: Table 9
# heads AF04 "I2C1/I2C2/I2C3" and AF09 "I2C2/I2C3", so which column
# applies is a per-pin fact.
read -r -d '' I2C_RECORD <<'JSON' || true
    {
      "instance": "I2C1",
      "scl": { "pin": "PB6", "af": 4, "package_pin": 42 },
      "sda": { "pin": "PB7", "af": 4, "package_pin": 43 }
    },
    {
      "instance": "I2C3",
      "scl": { "pin": "PA8", "af": 4, "package_pin": 29 },
      "sda": { "pin": "PB4", "af": 9, "package_pin": 40 }
    }
JSON

write_result() {
    local status="$1"
    local failed_stage="$2"

    local git_commit git_dirty
    git_commit="$(git -C "${REPO_ROOT}" rev-parse HEAD)"
    if [ -n "$(git -C "${REPO_ROOT}" status --porcelain)" ]; then
        git_dirty="true"
    else
        git_dirty="false"
    fi

    cat > "${RESULTS_FILE}" <<EOF
{
  "gate": "toolchain",
  "stage": 0,
  "tier": 0,
  "specification": "todos/stage0_todo.md",
  "timestamp_utc": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "git_commit": "${git_commit}",
  "git_dirty": ${git_dirty},
  "doctor": "${doctor_status}",
  "status": "${status}",
  "failed_stage": "${failed_stage}",
  "part": "${PART}",
  "part_datasheet": "${PART_DATASHEET}",
  "package": "UFQFPN48",
  "i2c_peripherals": [
${I2C_RECORD}
  ],
  "blink_observed": "${blink_observed}",
  "toolchain": {
    "arm_none_eabi_gcc": "${gcc_version:-unavailable}",
    "cmake": "${cmake_version:-unavailable}",
    "st_flash": "${stflash_version:-unavailable}"
  }
}
EOF
}

on_error() {
    write_result "fail" "${stage}"
    echo "toolchain: failed during ${stage}; result written to ${RESULTS_FILE}" >&2
}
trap on_error ERR

# --- preflight -----------------------------------------------------------

stage="preflight"

for tool in arm-none-eabi-gcc cmake ninja st-flash; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "toolchain: ${tool} not found on PATH" >&2
        false
    fi
done

gcc_version="$(arm-none-eabi-gcc -dumpversion)"
cmake_version="$(cmake --version | head -1 | awk '{print $3}')"
stflash_version="$(st-flash --version 2>&1 | head -1)"

# The interpreter and environment a measurement ran under are part of the
# measurement. doctor is named in the record rather than assumed, because
# the diff cannot show which environment was live.
stage="doctor"
if (cd "${HARNESS_DIR}" && mise run doctor >/dev/null 2>&1); then
    doctor_status="clean"
else
    doctor_status="not clean"
    echo "toolchain: mise run doctor is not clean; refusing to capture" >&2
    false
fi

# --- build ---------------------------------------------------------------

stage="configure"
cmake -S "${FW_DIR}" -B "${BUILD_DIR}" -GNinja

stage="build"
cmake --build "${BUILD_DIR}"

# --- flash ---------------------------------------------------------------

stage="flash"
st-flash write "${BUILD_DIR}/capture_engine.bin" 0x08000000

# --- observation ---------------------------------------------------------
#
# The criterion is that a blink flashes, which no exit code can establish.

stage="observe"
printf 'toolchain: is the LED blinking at about 1 Hz? [y/N] ' >&2
read -r answer
case "${answer}" in
    [yY]|[yY][eE][sS])
        blink_observed="yes"
        ;;
    *)
        blink_observed="no"
        echo "toolchain: blink not observed; the gate does not pass" >&2
        false
        ;;
esac

# --- pass ----------------------------------------------------------------

trap - ERR
write_result "pass" "none"
echo "toolchain: build, flash and blink confirmed; result written to ${RESULTS_FILE}"
