#!/usr/bin/env bash
# Stage 2 — compression for force–penetration (paper protocol).
# Restarts from restart/restart_t0p06_settled.vtu with top wall relocated to y=0.03510
# so that at t=0.06 the wall bottom edge sits at the paper's 0.0312 m.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
BIN="${BIN:-$ROOT/build/linux/bin/PeriDEM}"
OUT="$HERE/runs/stage2"
DECK="${DECK:-input_stage2.json}"
VTU="$HERE/restart/restart_t0p06_settled.vtu"

if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — cmake --build build/linux --target PeriDEM" >&2
  exit 1
fi
if [[ ! -f "$VTU" ]]; then
  echo "missing $VTU — run stage 1 and copy the t=0.06 frame, or restore the checked-in IC" >&2
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
d["Restart"]["File"] = "restart/restart_t0p06_settled"
p.write_text(json.dumps(d, indent=2) + "\n")
print("Output.Path =", d["Output"]["Path"])
print("Restart =", d["Restart"])
print("wall 503.y =", d["Particle_Generation"]["Data"]["503"]["y"])
PY
mkdir -p ../out
NP="${NP:-1}"
NTHREADS="${NTHREADS:-8}"
echo "BIN=$BIN NP=$NP NTHREADS=$NTHREADS DECK=$DECK"
MPIEXEC="${MPIEXEC:-mpirun}"
exec "$MPIEXEC" -n "$NP" "$BIN" -i "$DECK" -nThreads "$NTHREADS"
