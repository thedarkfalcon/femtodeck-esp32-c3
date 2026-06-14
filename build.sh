#!/usr/bin/env bash
set -euo pipefail

FQBN="${1:-esp32:esp32:esp32c3:PartitionScheme=huge_app}"
SKETCH="${2:-femtodeck-c3}"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
VERSION_PATH="$SCRIPT_DIR/$SKETCH/Version.h"

major="$(sed -nE 's/.*VERSION_MAJOR = ([0-9]+).*/\1/p' "$VERSION_PATH")"
minor="$(sed -nE 's/.*VERSION_MINOR = ([0-9]+).*/\1/p' "$VERSION_PATH")"
build="$(sed -nE 's/.*BUILD_NUMBER = ([0-9]+).*/\1/p' "$VERSION_PATH")"

if [[ -z "$major" || -z "$minor" || -z "$build" ]]; then
  echo "Could not read version fields from $VERSION_PATH" >&2
  exit 1
fi

next_build="$((build + 1))"
build_text="v${major}.${minor} b${next_build}"

if sed --version >/dev/null 2>&1; then
  sed -i -E \
    -e "s/(BUILD_NUMBER = )[0-9]+;/\\1${next_build};/" \
    -e "s/(BUILD_TEXT = \")[^\"]+(\";)/\\1${build_text}\\2/" \
    "$VERSION_PATH"
else
  sed -i '' -E \
    -e "s/(BUILD_NUMBER = )[0-9]+;/\\1${next_build};/" \
    -e "s/(BUILD_TEXT = \")[^\"]+(\";)/\\1${build_text}\\2/" \
    "$VERSION_PATH"
fi

echo "Building $build_text"
arduino-cli compile --fqbn "$FQBN" "$SKETCH"
