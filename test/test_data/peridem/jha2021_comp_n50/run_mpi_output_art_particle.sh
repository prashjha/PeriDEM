#!/usr/bin/env bash
# Artificial particle-MPI output check: tiny steps, verify per-rank VTU + PVTU + PVD.
# Does NOT claim physics identity — only that parallel I/O layout is sane.
set -euo pipefail
BIN_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$BIN_DIR/Test_PeriDEM_jha2021_comp_n50"
OUT="$BIN_DIR/mpi_out_art_particle"
INP="$BIN_DIR/mpi_out_art_particle_inp"
rm -rf "$OUT" "$INP"

MPIEXEC="${MPIEXEC:-mpirun}"
"$MPIEXEC" -n 4 "$BIN" \
  -nThreads 1 -nCols 4 -nRows 3 \
  -numSteps 40 -finalTime 8.0e-6 -noRequireContact \
  -outputDir "$(basename "$OUT")" -inputDir "$(basename "$INP")"

python3 - <<PY
from pathlib import Path
out = Path("$OUT")
pvd = out / "output.pvd"
assert pvd.is_file(), f"missing {pvd}"
text = pvd.read_text()
assert ".pvtu" in text, "PVD should reference .pvtu for multi-rank runs"
# one frame expected (dtOut from deck); check first pvtu + pieces
pvtus = sorted(out.glob("output_*.pvtu"))
assert pvtus, "no .pvtu files"
pvtu = pvtus[0]
body = pvtu.read_text()
pieces = [ln for ln in body.splitlines() if "Piece Source=" in ln]
assert len(pieces) == 4, f"expected 4 pieces, got {len(pieces)}"
for r in range(4):
    name = f"{pvtu.stem}_r{r}.vtu"
    assert (out / name).is_file(), f"missing piece {name}"
    # pieces must not be huge corrupted multi-root files
    raw = (out / name).read_bytes()
    assert raw.count(b"<VTKFile") == 1, f"{name} has multiple VTKFile roots"
print(f"OK particle-MPI output: {pvtu.name} + 4 pieces; PVD lists .pvtu")
PY
