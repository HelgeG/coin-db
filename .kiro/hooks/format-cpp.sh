#!/usr/bin/env bash
# Kiro postToolUse hook: format C/C++ files after they are written or edited.
# Reads the hook event JSON from stdin, extracts written file paths, and runs
# clang-format in place on any C/C++ source/header files.
#
# Exit codes:
#   0  success (or nothing to do / clang-format not installed)
#   Kiro shows STDERR as a warning for non-zero exit codes; we stay at 0 so a
#   missing clang-format does not interrupt the workflow — it just warns.

set -euo pipefail

event="$(cat)"

# clang-format is required to do anything; warn (via stderr) but do not fail.
if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found on PATH; skipping auto-format." >&2
  exit 0
fi

# Extract candidate file paths from the tool_input JSON.
# Works for both write (path) and any tool that reports operations[].path.
paths="$(printf '%s' "$event" | grep -oE '"path"[[:space:]]*:[[:space:]]*"[^"]+"' \
  | sed -E 's/.*"path"[[:space:]]*:[[:space:]]*"([^"]+)"/\1/' || true)"

[ -z "$paths" ] && exit 0

cpp_re='\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$'

while IFS= read -r file; do
  [ -z "$file" ] && continue
  if [[ "$file" =~ $cpp_re ]] && [ -f "$file" ]; then
    clang-format -i --style=file "$file" && \
      echo "clang-format: formatted $file" >&2
  fi
done <<< "$paths"

exit 0
