#!/usr/bin/env bash
# Attrition sim2 — thin rotating container, offset rotation center (main GIF sim2).
# Regenerates decks then runs PeriDEM. Does not wipe runs/ unless CLEAN=1.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
BIN="${BIN:-}"
if [[ -z "$BIN" ]]; then
  for c in "$ROOT/build/linux/bin/PeriDEM" "$ROOT/build/bin/PeriDEM"; do
    [[ -x "$c" ]] && BIN="$c" && break
  done
fi
OUT="$HERE/runs"
mkdir -p "$OUT/out"
if [[ -z "${BIN}" || ! -x "$BIN" ]]; then
  echo "missing PeriDEM binary — cmake --build <build> --target PeriDEM" >&2
  exit 1
fi
python3 "$HERE/gen_input.py"
python3 - <<PY
import json
from pathlib import Path
here = Path("$HERE")
out = (here / "runs" / "out").resolve()
for name in ("input_short.json", "input_medium.json", "input.json"):
    p = here / name
    if not p.is_file():
        continue
    d = json.loads(p.read_text())
    d["Output"]["Path"] = str(out) + "/"
    for k, v in list(d.get("Mesh", {}).items()):
        if k.startswith("Set_") and isinstance(v, dict) and "File" in v:
            fp = Path(v["File"])
            if fp.is_file():
                v["File"] = str((here / "meshes" / fp.name).resolve())
    p.write_text(json.dumps(d, indent=2) + "\n")
print("Output.Path =", out)
PY
DECK="${DECK:-input.json}"
NP="${NP:-4}"
cd "$HERE"
if [[ "${CLEAN:-0}" == "1" ]]; then
  rm -rf "$OUT/out"/output_* "$OUT/out"/*.pvd 2>/dev/null || true
fi
echo "BIN=$BIN NP=$NP DECK=$DECK"
MPIEXEC="${MPIEXEC:-mpirun}"
"$MPIEXEC" -n "$NP" "$BIN" -i "$DECK" -nThreads "${NTHREADS:-1}"
python3 "$HERE/check_health.py" "$OUT/out" "$DECK"
