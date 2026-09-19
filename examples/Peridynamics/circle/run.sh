#!/usr/bin/env bash
# Single-particle Peridynamics demo (JSON → bin/PeriDEM).
# Default deck is input_quick.json (short run); set DECK=input.json for the full run.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
BIN="${BIN:-}"
if [[ -z "$BIN" ]]; then
  for c in "$ROOT/build/linux/bin/PeriDEM" "$ROOT/build/bin/PeriDEM"; do
    [[ -x "$c" ]] && BIN="$c" && break
  done
fi
if [[ -z "${BIN}" || ! -x "$BIN" ]]; then
  echo "missing PeriDEM binary — cmake --build <build> --target PeriDEM" >&2
  exit 1
fi
DECK="${DECK:-input_quick.json}"
NP="${NP:-1}"
OUT_KEY=$(python3 - <<PY
import json
from pathlib import Path
p = Path("$HERE") / "$DECK"
d = json.loads(p.read_text())
print(d["Output"]["Path"].rstrip("/"))
PY
)
mkdir -p "$HERE/$OUT_KEY"
cd "$HERE"
if [[ "${CLEAN:-0}" == "1" ]]; then
  rm -rf "$HERE/$OUT_KEY"/output_* "$HERE/$OUT_KEY"/*.pvd 2>/dev/null || true
fi
echo "BIN=$BIN NP=$NP DECK=$DECK"
MPIEXEC="${MPIEXEC:-mpirun}"
"$MPIEXEC" -n "$NP" "$BIN" -i "$DECK" -nThreads "${NTHREADS:-4}"
ls -1 "$HERE/$OUT_KEY"/output_*.vtu 2>/dev/null | head -3
echo "OK: wrote under $HERE/$OUT_KEY"
