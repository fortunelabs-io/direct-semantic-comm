#!/bin/sh
# Export the schematic PDF stamped with the release tag and commit hash.
# Usage:       sh scripts/export-sch.sh
# Vendor copy: OWNER="<registered name>" sh scripts/export-sch.sh
# Overrides:   PRO=path/to/project.kicad_pro  KICAD_CLI=path/to/kicad-cli
set -eu

cd "$(git rev-parse --show-toplevel)"

# Locate kicad-cli. Fall back to the default Windows install path.
KICAD_CLI="${KICAD_CLI:-kicad-cli}"
if ! command -v "$KICAD_CLI" >/dev/null 2>&1; then
  KICAD_CLI="/c/Program Files/KiCad/9.0/bin/kicad-cli.exe"
fi
if [ ! -x "$KICAD_CLI" ] && ! command -v "$KICAD_CLI" >/dev/null 2>&1; then
  echo "export-sch: kicad-cli not found, set KICAD_CLI" >&2
  exit 1
fi

# Locate the project. The root schematic has the same stem as the .kicad_pro.
PRO="${PRO:-$(git ls-files '*.kicad_pro' | head -n 1)}"
if [ -z "$PRO" ]; then
  echo "export-sch: no .kicad_pro found in the repository" >&2
  exit 1
fi
SCH="${PRO%.kicad_pro}.kicad_sch"
NAME=$(basename "${PRO%.kicad_pro}")

# Release: nearest v* tag, plus the number of commits after it.
TAG=$(git describe --tags --abbrev=0 --match '[vV]*' 2>/dev/null || true)
if [ -n "$TAG" ]; then
  AHEAD=$(git rev-list --count "$TAG"..HEAD)
  if [ "$AHEAD" -eq 0 ]; then RELEASE="$TAG"; else RELEASE="$TAG+$AHEAD"; fi
else
  RELEASE="untagged"
fi

# Hash: short HEAD, marked dirty when tracked files have uncommitted changes.
HASH=$(git rev-parse --short HEAD)
if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
  HASH="$HASH-dirty"
fi

set -- -D "RELEASE=$RELEASE" -D "GIT_HASH=$HASH"
if [ -n "${OWNER:-}" ]; then
  set -- "$@" -D "OWNER=$OWNER"
fi

mkdir -p out
OUT="out/$NAME-$RELEASE-$HASH.pdf"
"$KICAD_CLI" sch export pdf "$@" -o "$OUT" "$SCH"
echo "$OUT"
