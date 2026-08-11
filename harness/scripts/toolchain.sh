#!/usr/bin/env bash
#
# toolchain.sh - the command behind `mise run toolchain`.
#
# Gate: "the capture engine part satisfies the timing budget",
# todos/stage0_todo.md, Tier 0. Passes when the clock plan holds against
# every limit transcribed from the part's datasheet, and the two I2C
# peripherals are recorded from the datasheet on separate pins with their
# alternate function numbers.
#
# Nothing is powered here and nothing is flashed. The check is a compile:
# scripts/toolchain_record.c includes firmware/capture/timing_budget.h,
# whose static assertions compare the clock plan against limits
# transcribed from DocID026289 Rev 4. A plan that violates a limit does
# not build. Whether the cross toolchain can produce an image the part
# executes is the Tier 1 `blink` gate, and was half of this gate's
# criterion until issue #20 split them.
#
# The record is printed by that program from the same headers the firmware
# compiles against, so it cannot claim an alternate function the firmware
# does not program. No value in the result file is typed by hand.
#
# This script does not decide whether a prediction held. That is recorded
# in the gate's issue before the run and judged by a person afterwards.

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
HARNESS_DIR="${REPO_ROOT}/harness"
FW_DIR="${HARNESS_DIR}/firmware/capture"
SCRIPTS_DIR="${HARNESS_DIR}/scripts"
RESULTS_DIR="${HARNESS_DIR}/results"
RESULTS_FILE="${RESULTS_DIR}/stage0_toolchain.json"

stage="preflight"
doctor_status="not run"
work_dir=""

cleanup() {
    [ -n "${work_dir}" ] && rm -rf "${work_dir}"
}
trap cleanup EXIT

write_failure() {
    local git_commit git_dirty
    git_commit="$(git -C "${REPO_ROOT}" rev-parse HEAD)"
    if [ -n "$(git -C "${REPO_ROOT}" status --porcelain)" ]; then
        git_dirty="true"
    else
        git_dirty="false"
    fi
    mkdir -p "${RESULTS_DIR}"
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
  "status": "fail",
  "failed_stage": "${stage}"
}
EOF
}

on_error() {
    write_failure
    echo "toolchain: failed during ${stage}; result written to ${RESULTS_FILE}" >&2
}
trap 'on_error; cleanup' ERR

mkdir -p "${RESULTS_DIR}"

# --- preflight -----------------------------------------------------------

stage="preflight"

CC="${CC:-cc}"
if ! command -v "${CC}" >/dev/null 2>&1; then
    echo "toolchain: no host C compiler (${CC}) on PATH" >&2
    false
fi

# The interpreter and environment a measurement ran under are part of the
# measurement, so doctor is named in the record rather than assumed.
stage="doctor"
if (cd "${HARNESS_DIR}" && mise run doctor >/dev/null 2>&1); then
    doctor_status="clean"
else
    doctor_status="not clean"
    echo "toolchain: mise run doctor is not clean; refusing to capture" >&2
    false
fi

# --- the check: the datasheet assertions must compile --------------------

stage="assertions"
work_dir="$(mktemp -d)"
"${CC}" -std=c11 -Wall -Wextra -Werror \
    -I "${FW_DIR}" \
    "${SCRIPTS_DIR}/toolchain_record.c" \
    -o "${work_dir}/toolchain_record"

stage="record"
"${work_dir}/toolchain_record" > "${work_dir}/record.json"

# --- assemble ------------------------------------------------------------

stage="assemble"
CC_VERSION="$("${CC}" --version | head -1)"
GIT_COMMIT="$(git -C "${REPO_ROOT}" rev-parse HEAD)"
if [ -n "$(git -C "${REPO_ROOT}" status --porcelain)" ]; then
    GIT_DIRTY="true"
else
    GIT_DIRTY="false"
fi

RECORD_JSON="${work_dir}/record.json" \
RESULTS_FILE="${RESULTS_FILE}" \
DOCTOR_STATUS="${doctor_status}" \
GIT_COMMIT="${GIT_COMMIT}" \
GIT_DIRTY="${GIT_DIRTY}" \
CC_VERSION="${CC_VERSION}" \
"${HARNESS_DIR}/.venv/bin/python" - <<'PY'
import datetime
import json
import os

with open(os.environ["RECORD_JSON"]) as handle:
    record = json.load(handle)

result = {
    "gate": "toolchain",
    "stage": 0,
    "tier": 0,
    "specification": "todos/stage0_todo.md",
    "timestamp_utc": datetime.datetime.now(datetime.timezone.utc)
                     .strftime("%Y-%m-%dT%H:%M:%SZ"),
    "git_commit": os.environ["GIT_COMMIT"],
    "git_dirty": os.environ["GIT_DIRTY"] == "true",
    "doctor": os.environ["DOCTOR_STATUS"],
    "status": "pass",
    "failed_stage": "none",
    "checked_by": {
        "method": "compile-time static assertions, host compiler",
        "compiler": os.environ["CC_VERSION"],
        "source": "harness/scripts/toolchain_record.c",
    },
}
result.update(record)

with open(os.environ["RESULTS_FILE"], "w") as handle:
    json.dump(result, handle, indent=2)
    handle.write("\n")
PY

trap cleanup EXIT
echo "toolchain: datasheet assertions hold; result written to ${RESULTS_FILE}"
