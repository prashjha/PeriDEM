#!/usr/bin/env bash
# Silling KW 2D (full literature path).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
BIN="${ROOT}/build/linux/test/test_data/peridem/notched_impact_inbuilt/Test_PeriDEM_notched_impact_inbuilt"
OUT="$(cd "$(dirname "$0")" && pwd)/runs/silling2d"
mkdir -p "$OUT"
if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — cmake --build build/linux --target Test_PeriDEM_notched_impact_inbuilt" >&2
  exit 1
fi
echo "BIN=$BIN"
echo "OUT=$OUT"
exec "$BIN" -nThreads "${NTHREADS:-8}" -outputDir "$OUT"
