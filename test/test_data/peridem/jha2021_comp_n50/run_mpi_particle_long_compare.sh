#!/usr/bin/env bash
# Physics identity: particle-MPI vs serial on jha2021_comp long setup.
set -euo pipefail
BIN_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$BIN_DIR/Test_PeriDEM_jha2021_comp_n50"
NTHREADS="${NTHREADS:-8}"
RANKS="${RANKS:-4}"
ARGS=(-nThreads "$NTHREADS" -finalTime 0.012 -numSteps 60000 -noRequireContact)

SER_OUT=mpi_long_ser_out
SER_INP=mpi_long_ser_inp
PAR_OUT="mpi_long_r${RANKS}_out"
PAR_INP="mpi_long_r${RANKS}_inp"

rm -rf "$BIN_DIR/$SER_OUT" "$BIN_DIR/$SER_INP" "$BIN_DIR/$PAR_OUT" "$BIN_DIR/$PAR_INP"

echo "=== serial (1 rank) ==="
/usr/bin/time -f 'wall_s=%e' -o "$BIN_DIR/${SER_OUT}_time.txt" \
  mpirun -n 1 --quiet "$BIN" "${ARGS[@]}" \
    -outputDir "$SER_OUT" -inputDir "$SER_INP"

echo "=== particle-MPI ($RANKS ranks) ==="
/usr/bin/time -f 'wall_s=%e' -o "$BIN_DIR/${PAR_OUT}_time.txt" \
  mpirun -n "$RANKS" --quiet "$BIN" "${ARGS[@]}" \
    -outputDir "$PAR_OUT" -inputDir "$PAR_INP"

python3 - "$SER_OUT" "$PAR_OUT" "$RANKS" <<'PY'
import sys
from pathlib import Path
ser_out, par_out, ranks = sys.argv[1], sys.argv[2], sys.argv[3]
ser = [float(x) for x in Path(f"{ser_out}/mpi_metric.txt").read_text().split()]
par = [float(x) for x in Path(f"{par_out}/mpi_metric.txt").read_text().split()]
tol = 1e-9
diffs = [abs(a - b) for a, b in zip(ser, par)]
print("serial metric:", ser)
print(f"mpi-{ranks} metric:", par)
print("abs diffs:", diffs)
print("PRIMARY wall:", Path(f"{ser_out}_time.txt").read_text().strip(), "|",
      Path(f"{par_out}_time.txt").read_text().strip())
if any(d > tol for d in diffs):
    raise SystemExit(f"MPI particle-parallel metric mismatch (tol={tol})")
print("OK: serial and mpirun metrics match")
PY
