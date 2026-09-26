#!/usr/bin/env bash
#
# Generate labeled placeholder coin images for the sample collection.
#
# These are synthetic placeholders (a coin-like disc with the coin name and
# face written on it) — NOT real coin photography. They exist so the demo/
# sample data has something to show in the web UI and CLI.
#
# Output: PNG files under samples/images/. Requires `rsvg-convert`
# (e.g. `brew install librsvg`).
#
# Usage: scripts/gen-sample-images.sh
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root_dir="$(cd "$script_dir/.." && pwd)"
outdir="$root_dir/samples/images"

if ! command -v rsvg-convert >/dev/null 2>&1; then
  echo "rsvg-convert not found. Install librsvg (e.g. 'brew install librsvg')." >&2
  exit 1
fi

mkdir -p "$outdir"

# emit_png NAME FACE HEX_BG HEX_RING OUTFILE
emit_png() {
  local name="$1" face="$2" bg="$3" ring="$4" out="$5"
  local tmp
  tmp="$(mktemp -t coin-svg.XXXXXX)"
  cat > "$tmp" <<SVG
<svg xmlns="http://www.w3.org/2000/svg" width="480" height="480" viewBox="0 0 480 480">
  <rect width="480" height="480" fill="#f2f2f4"/>
  <circle cx="240" cy="240" r="210" fill="${bg}" stroke="${ring}" stroke-width="10"/>
  <circle cx="240" cy="240" r="188" fill="none" stroke="${ring}" stroke-width="3" stroke-dasharray="6 8" opacity="0.7"/>
  <text x="240" y="150" text-anchor="middle" font-family="Helvetica, Arial, sans-serif"
        font-size="30" font-weight="700" fill="#ffffff" opacity="0.95">PLACEHOLDER</text>
  <text x="240" y="248" text-anchor="middle" font-family="Helvetica, Arial, sans-serif"
        font-size="40" font-weight="700" fill="#ffffff">${name}</text>
  <text x="240" y="330" text-anchor="middle" font-family="Helvetica, Arial, sans-serif"
        font-size="34" font-weight="600" letter-spacing="4" fill="#ffffff" opacity="0.95">${face}</text>
</svg>
SVG
  rsvg-convert -w 480 -h 480 "$tmp" -o "$out"
  rm -f "$tmp"
  echo "wrote ${out#"$root_dir/"}"
}

emit_png "Norway 50 Ore"  "OBVERSE" "#2f5aa0" "#e8eef7" "$outdir/coin1-obverse.png"
emit_png "Norway 50 Ore"  "REVERSE" "#2f5aa0" "#e8eef7" "$outdir/coin1-reverse.png"
emit_png "USA \$1 Morgan" "OBVERSE" "#b08828" "#fff6e0" "$outdir/coin2-obverse.png"
emit_png "USA \$1 Morgan" "REVERSE" "#b08828" "#fff6e0" "$outdir/coin2-reverse.png"
emit_png "Sweden 1 Krona" "OBVERSE" "#2a8f82" "#e2f5f1" "$outdir/coin3-obverse.png"

echo "Done. Generated 5 placeholder images in samples/images/."
