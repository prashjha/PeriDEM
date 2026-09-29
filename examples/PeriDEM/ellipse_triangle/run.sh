#!/usr/bin/env bash
# Hollow ellipse × short tip + shape gate + health gates.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
BIN="${ROOT}/build/linux/examples/PeriDEM/ellipse_triangle/example_ellipse_triangle"
OUT="$HERE/runs"
INP="$HERE/inp"
mkdir -p "$OUT" "$INP"
if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — cmake --build build/linux --target example_ellipse_triangle" >&2
  exit 1
fi
rm -rf "$OUT"/output_*.vtu "$OUT"/*.pvd "$OUT"/fe 2>/dev/null || true
rm -f "$INP"/mesh_*.msh "$INP"/input.json 2>/dev/null || true
"$BIN" -nThreads "${NTHREADS:-4}" -outputDir "$OUT" -inputDir "$INP" \
  ${FINAL_TIME:+-finalTime "$FINAL_TIME"} \
  ${NUM_STEPS:+-numSteps "$NUM_STEPS"} \
  "$@"
python3 - "$INP" "$OUT" <<'PY'
import json, meshio, numpy as np, re, sys
from pathlib import Path
inp, out = Path(sys.argv[1]), Path(sys.argv[2])
j = json.loads((inp / "input.json").read_text())
t = j["Particle"]["Set_2"]["Type"]
print("Particle Set_2 Type =", t)
if t != "ellipse_minus_ellipse":
    raise SystemExit(f"FAIL: expected ellipse_minus_ellipse, got {t}")
m = meshio.read(inp / "mesh_hollow_ellipse.msh")
X = m.points[:, :2]
span = X.max(0) - X.min(0)
print(f"ellipse mesh span x={span[0]:.4e} y={span[1]:.4e} nn={len(X)}")
if span[0] <= span[1] * 1.05:
    raise SystemExit("FAIL: mesh is not a wider-than-tall ellipse")
outs = sorted(out.glob("output_*.vtu"),
              key=lambda p: int(re.search(r"output_(\d+)", p.name).group(1)))
if not outs:
    raise SystemExit("FAIL: no VTU output")
m0 = meshio.read(outs[0])
pid = m0.point_data["Particle_ID"].ravel().astype(int)
n_ell = int((pid == 1).sum())
print(f"VTU pid=1 nodes = {n_ell} (mesh nn={len(X)})")
if abs(n_ell - len(X)) > 2:
    raise SystemExit(f"FAIL: VTU ellipse nodes {n_ell} != mesh {len(X)}")
Xe = m0.points[pid == 1, :2]
span2 = Xe.max(0) - Xe.min(0)
print(f"VTU ellipse span x={span2[0]:.4e} y={span2[1]:.4e}")
if span2[0] <= span2[1] * 1.05:
    raise SystemExit("FAIL: VTU particle 1 is not elliptical")
print("SHAPE_OK: hollow ellipse in mesh and VTU")
PY
python3 "$HERE/check_health.py" "$OUT"
