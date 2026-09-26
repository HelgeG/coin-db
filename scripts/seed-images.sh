#!/usr/bin/env bash
#
# Attach the sample placeholder images (samples/images/) to coins in a seeded
# data directory, using the CLI's `image add` (which copies each file into the
# data dir's managed image store and records the DB row).
#
# Run this AFTER scripts/seed.sh has imported the sample collection, so coins
# 1, 2 and 3 exist. Safe to run against the demo/ data dir.
#
# Usage: scripts/seed-images.sh [DATA_DIR]
#   DATA_DIR   target data directory (default: coins-data)
#   COINS_CLI  override the coins_cli binary path
set -euo pipefail

data_dir="${1:-coins-data}"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root_dir="$(cd "$script_dir/.." && pwd)"
cli="${COINS_CLI:-$root_dir/build/build/Debug/cli/coins_cli}"
img_dir="$root_dir/samples/images"

if [ ! -x "$cli" ]; then
  echo "coins_cli not found at: $cli" >&2
  echo "Build the project first, or set COINS_CLI to the binary path." >&2
  exit 1
fi
if [ ! -d "$img_dir" ]; then
  echo "Sample images not found at: $img_dir" >&2
  echo "Generate them first: scripts/gen-sample-images.sh" >&2
  exit 1
fi

# coin_id | kind | file | caption
add() {
  "$cli" --data-dir "$data_dir" image add "$1" --file "$img_dir/$3" --kind "$2" --caption "$4"
}

add 1 obverse coin1-obverse.png "Norway 50 Øre — obverse (placeholder)"
add 1 reverse coin1-reverse.png "Norway 50 Øre — reverse (placeholder)"
add 2 obverse coin2-obverse.png "1889-O Morgan Dollar — obverse (placeholder)"
add 2 reverse coin2-reverse.png "1889-O Morgan Dollar — reverse (placeholder)"
add 3 obverse coin3-obverse.png "Sweden 1 Krona — obverse (placeholder)"

echo "Attached 5 sample images to coins in '$data_dir'."
