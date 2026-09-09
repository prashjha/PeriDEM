#!/usr/bin/env bash
# Compare short Single_Particle PD run: 1 rank vs 2 ranks (DOF-MPI).
set -euo pipefail
BIN_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$BIN_DIR/Peridynamics"
ARGS=(-i ./example/input_dof_mpi.json -nThreads 1)

rm -rf "$BIN_DIR/dof_cmp_ser_out" "$BIN_DIR/dof_cmp_2_out"
mkdir -p "$BIN_DIR/dof_cmp_ser_out" "$BIN_DIR/dof_cmp_2_out"

mpirun -n 1 --quiet "$BIN" "${ARGS[@]}" -outputDir "$BIN_DIR/dof_cmp_ser_out"
mpirun -n 2 --quiet "$BIN" "${ARGS[@]}" -outputDir "$BIN_DIR/dof_cmp_2_out"

python3 - <<PY
from pathlib import Path
ser = float(Path("$BIN_DIR/dof_cmp_ser_out/pd_metric.txt").read_text().split()[0])
par = float(Path("$BIN_DIR/dof_cmp_2_out/pd_metric.txt").read_text().split()[0])
tol = 1e-9
diff = abs(ser - par)
print(f"serial max|u|={ser:.12e}")
print(f"mpi-2  max|u|={par:.12e}")
print(f"abs diff={diff:.3e}")
if diff > tol:
    raise SystemExit(f"PD DOF-MPI metric mismatch (tol={tol})")
print("OK: serial and mpirun -n 2 PD metrics match")
PY
