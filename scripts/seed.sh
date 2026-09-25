#!/usr/bin/env bash
#
# Seed a coins-db data directory with the example collection by importing
# samples/collection.sample.json through the CLI.
#
# Usage: scripts/seed.sh [DATA_DIR]
#   DATA_DIR   target data directory (default: coins-data)
#   COINS_CLI  override the coins_cli binary path
set -euo pipefail

data_dir="${1:-coins-data}"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root_dir="$(cd "$script_dir/.." && pwd)"
cli="${COINS_CLI:-$root_dir/build/build/Debug/cli/coins_cli}"
sample="$root_dir/samples/collection.sample.json"

if [ ! -x "$cli" ]; then
  echo "coins_cli not found at: $cli" >&2
  echo "Build the project first, or set COINS_CLI to the binary path." >&2
  exit 1
fi

"$cli" --data-dir "$data_dir" import --in "$sample"
echo "Seeded '$data_dir' from $sample"
