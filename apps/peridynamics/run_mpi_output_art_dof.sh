#!/usr/bin/env bash
# Artificial DOF-MPI output check: short Single_Particle PD, verify per-rank VTU + PVTU + PVD.
set -euo pipefail
BIN_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$BIN_DIR/Peridynamics"
EX="$BIN_DIR/example"
OUT="$BIN_DIR/mpi_out_art_dof"
rm -rf "$OUT"
mkdir -p "$OUT"

# tiny deck copy with frequent output
python3 - <<PY
import json
from pathlib import Path
src = Path("$EX/input_dof_mpi.json")
j = json.loads(src.read_text())
j["Model"]["Time_Steps"] = 20
j["Model"]["Final_Time"] = 1.0e-5
j["Output"]["Path"] = "$OUT/"
j["Output"]["Output_Interval"] = 10
j["Output"]["Test_Output_Interval"] = 10
j["Output"]["PVD_Collection"] = True
# mesh path relative to cwd (= BIN_DIR)
j["Mesh"]["Set_1"]["File"] = "./example/mesh_cir_1_0.msh"
Path("$OUT/input.json").write_text(json.dumps(j, indent=2))
print("wrote", "$OUT/input.json")
PY

cd "$BIN_DIR"
MPIEXEC="${MPIEXEC:-mpirun}"
"$MPIEXEC" -n 4 "$BIN" -i "$OUT/input.json" -nThreads 1 -outputDir "$OUT"

python3 - <<PY
from pathlib import Path
out = Path("$OUT")
pvd = out / "output.pvd"
assert pvd.is_file(), f"missing {pvd}"
assert ".pvtu" in pvd.read_text(), "PVD should reference .pvtu"
pvtus = sorted(out.glob("output_*.pvtu"))
assert pvtus, "no .pvtu"
pvtu = pvtus[0]
body = pvtu.read_text()
pieces = [ln for ln in body.splitlines() if "Piece Source=" in ln]
assert len(pieces) == 4, f"expected 4 pieces, got {len(pieces)}"
for r in range(4):
    name = f"{pvtu.stem}_r{r}.vtu"
    assert (out / name).is_file(), f"missing {name}"
    raw = (out / name).read_bytes()
    assert raw.count(b"<VTKFile") == 1, f"{name} multi-root"
print(f"OK DOF-MPI output: {pvtu.name} + 4 pieces; PVD lists .pvtu")
PY
