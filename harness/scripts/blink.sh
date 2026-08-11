#!/usr/bin/env bash
#
# blink.sh - the command behind `mise run blink`.
#
# Gate: "code built for the selected part runs on it",
# todos/stage0_todo.md, Tier 1. Passes when a blink builds with
# arm-none-eabi-gcc, flashes over SWD, and is observed running.
#
# This was half of the Tier 0 gate's criterion until issue #20 split them:
# flashing is powering, and Tier 0 is defined as nothing powered. It is
# the first item in the specification that puts a part under power.
#
# Writes harness/results/stage0_blink.json from the real outcome of each
# step. Every exit path writes a record, including the unanticipated ones;
# that is what the ERR trap is for, and it replaces an arrangement in
# which a failure during cmake configure exited under `set -e` and left no
# file at all.
#
# What this script will not do:
#
#   - It does not decide whether a prediction held. That is recorded in
#     the gate's issue before the run and judged by a person afterwards.
#   - It does not infer that the blink was seen. Flashing successfully and
#     a visible LED are different claims; the second is confirmed by the
#     operator at the prompt below. Prompting belongs in scripts/ and is
#     why scripts/ and tests/ are separate directories.
#
# Requires: arm-none-eabi-gcc, cmake, ninja, st-flash, and an ST-Link
# attached over SWD to an STM32F411CEU6 board.

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
HARNESS_DIR="${REPO_ROOT}/harness"
FW_DIR="${HARNESS_DIR}/firmware/capture"
BUILD_DIR="${FW_DIR}/build"
RESULTS_DIR="${HARNESS_DIR}/results"
RESULTS_FILE="${RESULTS_DIR}/stage0_blink.json"
TOOLCHAIN_RESULT="${RESULTS_DIR}/stage0_toolchain.json"

PART="STM32F411CEU6"

stage="preflight"
doctor_status="not run"
blink_observed="not reached"

mkdir -p "${RESULTS_DIR}"

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
  "gate": "blink",
  "stage": 0,
  "tier": 1,
  "specification": "todos/stage0_todo.md",
  "timestamp_utc": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "git_commit": "${git_commit}",
  "git_dirty": ${git_dirty},
  "doctor": "${doctor_status}",
  "status": "${status}",
  "failed_stage": "${failed_stage}",
  "part": "${PART}",
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
    echo "blink: failed during ${stage}; result written to ${RESULTS_FILE}" >&2
}
trap on_error ERR

# --- preflight -----------------------------------------------------------

stage="preflight"

# Order is enforced by data dependency: this gate does not start until the
# gate above it has left a passing file behind.
if [ ! -f "${TOOLCHAIN_RESULT}" ]; then
    echo "blink: ${TOOLCHAIN_RESULT} is absent; the Tier 0 gate above this one has not run" >&2
    false
fi

for tool in arm-none-eabi-gcc cmake ninja st-flash; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "blink: ${tool} not found on PATH" >&2
        false
    fi
done

gcc_version="$(arm-none-eabi-gcc -dumpversion)"
cmake_version="$(cmake --version | head -1 | awk '{print $3}')"
stflash_version="$(st-flash --version 2>&1 | head -1)"

stage="doctor"
if (cd "${HARNESS_DIR}" && mise run doctor >/dev/null 2>&1); then
    doctor_status="clean"
else
    doctor_status="not clean"
    echo "blink: mise run doctor is not clean; refusing to capture" >&2
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
# The criterion is that a blink runs, which no exit code establishes.

stage="observe"
printf 'blink: is the LED blinking at about 1 Hz? [y/N] ' >&2
read -r answer
case "${answer}" in
    [yY]|[yY][eE][sS])
        blink_observed="yes"
        ;;
    *)
        blink_observed="no"
        echo "blink: not observed; the gate does not pass" >&2
        false
        ;;
esac

# --- pass ----------------------------------------------------------------

trap - ERR
write_result "pass" "none"
echo "blink: build, flash and blink confirmed; result written to ${RESULTS_FILE}"
