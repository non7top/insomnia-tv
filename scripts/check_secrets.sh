#!/bin/bash
# Runs ripsecrets in a throwaway container: the repo is mounted read-only and
# the default pattern set is extended with MAC addresses (a BSSID places a home
# to within tens of metres). Pass file paths to scan only those; none scans the
# whole tree. Allowlisted values live in .secretsignore.
set -euo pipefail

cd "$(dirname "$0")/.."

readonly IMAGE=insomniatv-ripsecrets

if ! docker image inspect "$IMAGE" &>/dev/null; then
  docker build --quiet --tag "$IMAGE" scripts/ripsecrets >/dev/null
fi

exec docker run --rm \
  --user "$(id -u):$(id -g)" \
  --volume "$PWD:/repo:ro" \
  --workdir /repo \
  "$IMAGE" \
  --additional-pattern '(?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' \
  "$@"
