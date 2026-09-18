#!/usr/bin/env bash
# Stage 1 — gravity settle (T=0.06, top wall high at y=0.04152).
# Produces stage1_out/; copy final frame to restart/restart_t0p06_settled.vtu for stage 2.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
BIN="${BIN:-$ROOT/build/linux/bin/PeriDEM}"
OUT="$HERE/runs/stage1"
DECK="${DECK:-input_stage1.json}"

if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — cmake --build build/linux --target PeriDEM" >&2
  exit 1
fi

if [[ "${CLEAN:-0}" == "1" ]]; then
  rm -rf "$OUT"
fi
mkdir -p "$OUT"
rsync -a --delete \
  --exclude runs --exclude README.md --exclude '*.sh' --exclude .gitignore \
  "$HERE/" "$OUT/inp/"
cd "$OUT/inp"
python3 - <<PY
import json
from pathlib import Path
p = Path("$DECK")
d = json.loads(p.read_text())
d["Output"] = d.get("Output", {})
d["Output"]["Path"] = str(Path("..").resolve() / "out") + "/"
p.write_text(json.dumps(d, indent=2) + "\n")
print("Output.Path =", d["Output"]["Path"])
PY
mkdir -p ../out
NP="${NP:-4}"
NTHREADS="${NTHREADS:-1}"
echo "BIN=$BIN NP=$NP NTHREADS=$NTHREADS DECK=$DECK"
exec mpirun -n "$NP" --quiet "$BIN" -i "$DECK" -nThreads "$NTHREADS"
