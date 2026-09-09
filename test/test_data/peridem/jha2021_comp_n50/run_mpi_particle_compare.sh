#!/usr/bin/env bash
# Compare short 12-grain run: 1 rank vs 2 ranks (particle-parallel owners).
set -euo pipefail
BIN_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$BIN_DIR/Test_PeriDEM_jha2021_comp_n50"
ARGS=(-nThreads 2 -numSteps 200 -finalTime 4.0e-5 -noRequireContact)

rm -rf "$BIN_DIR/mpi_cmp_ser_out" "$BIN_DIR/mpi_cmp_ser_inp"
rm -rf "$BIN_DIR/mpi_cmp_2_out" "$BIN_DIR/mpi_cmp_2_inp"

mpirun -n 1 --quiet "$BIN" "${ARGS[@]}" \
  -outputDir mpi_cmp_ser_out -inputDir mpi_cmp_ser_inp

mpirun -n 2 --quiet "$BIN" "${ARGS[@]}" \
  -outputDir mpi_cmp_2_out -inputDir mpi_cmp_2_inp

python3 - <<'PY'
from pathlib import Path
ser = Path("mpi_cmp_ser_out/mpi_metric.txt").read_text().split()
par = Path("mpi_cmp_2_out/mpi_metric.txt").read_text().split()
ser_f = [float(x) for x in ser]
par_f = [float(x) for x in par]
tol = 1e-9
diffs = [abs(a - b) for a, b in zip(ser_f, par_f)]
print("serial metric:", ser_f)
print("mpi-2 metric:", par_f)
print("abs diffs:", diffs)
if any(d > tol for d in diffs):
    raise SystemExit(f"MPI particle-parallel metric mismatch (tol={tol})")
print("OK: serial and mpirun -n 2 metrics match")
PY
