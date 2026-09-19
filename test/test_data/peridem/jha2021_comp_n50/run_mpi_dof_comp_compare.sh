#!/usr/bin/env bash
# Three-way MPI identity on compressive pack (default 4×3=12 grains):
# serial (none) vs particle@2 vs dof@2.
# Override pack: NCOLS=5 NROWS=5 ./run_mpi_dof_comp_compare.sh
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
BIN="${BIN:-$HERE/Test_PeriDEM_jha2021_comp_n50}"
if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — build Test_PeriDEM_jha2021_comp_n50" >&2
  exit 1
fi

MPIEXEC="${MPIEXEC:-mpirun}"

NCOLS="${NCOLS:-4}"
NROWS="${NROWS:-3}"
# Short contact window (existing particle compare used 200 steps / 4e-5 s).
# Slightly longer so dof path exercises contact + plate motion.
NUM_STEPS="${NUM_STEPS:-500}"
FINAL_TIME="${FINAL_TIME:-1.0e-4}"
TOL="${TOL:-1e-8}"

ARGS=(-nThreads 1 -nCols "$NCOLS" -nRows "$NROWS"
      -numSteps "$NUM_STEPS" -finalTime "$FINAL_TIME" -noRequireContact)

rm -rf "$HERE/cmp_none_out" "$HERE/cmp_none_inp"
rm -rf "$HERE/cmp_particle_out" "$HERE/cmp_particle_inp"
rm -rf "$HERE/cmp_dof_out" "$HERE/cmp_dof_inp"

echo "=== pack ${NCOLS}x${NROWS}  steps=$NUM_STEPS T=$FINAL_TIME ==="
echo "=== serial / none ==="
"$MPIEXEC" -n 1 "$BIN" "${ARGS[@]}" -mpiStrategy none \
  -outputDir cmp_none_out -inputDir cmp_none_inp

echo "=== particle-MPI @2 ==="
"$MPIEXEC" -n 2 "$BIN" "${ARGS[@]}" -mpiStrategy particle \
  -outputDir cmp_particle_out -inputDir cmp_particle_inp

echo "=== DOF-MPI @2 ==="
"$MPIEXEC" -n 2 "$BIN" "${ARGS[@]}" -mpiStrategy dof \
  -outputDir cmp_dof_out -inputDir cmp_dof_inp

python3 - <<PY
from pathlib import Path
import struct
import numpy as np
tol = float("$TOL")
tol_u, tol_v = 1e-12, 1e-9
def load(p):
    a = Path(p).read_text().split()
    return [float(x) for x in a]
none = load("cmp_none_out/mpi_metric.txt")
part = load("cmp_particle_out/mpi_metric.txt")
dof = load("cmp_dof_out/mpi_metric.txt")
print(f"none:     {none}")
print(f"particle: {part}")
print(f"dof:      {dof}")
ok = True
for name, a in (("particle", part), ("dof", dof)):
    for i, lab in enumerate(("max|u|", "com0x", "com0y")):
        d = abs(a[i] - none[i])
        print(f"  |{name}-{lab} - none| = {d:.3e}")
        if d > tol:
            ok = False
            print(f"FAIL: {name} {lab} mismatch (tol={tol})")

def load_ts(p):
    rows = []
    for line in Path(p).read_text().splitlines()[1:]:
        if not line.strip():
            continue
        step, t, mu, cx, cy = line.split(",")
        rows.append((int(step), float(t), float(mu), float(cx), float(cy)))
    return rows
ts_none = load_ts("cmp_none_out/mpi_metric_ts.csv")
ts_part = load_ts("cmp_particle_out/mpi_metric_ts.csv")
ts_dof = load_ts("cmp_dof_out/mpi_metric_ts.csv")
print(f"timeseries samples: none={len(ts_none)} particle={len(ts_part)} dof={len(ts_dof)}")
if not (len(ts_none) == len(ts_part) == len(ts_dof) and len(ts_none) > 5):
    raise SystemExit(f"FAIL: timeseries length mismatch or too short")
max_du = 0.0
n_bad = 0
for a, b, c in zip(ts_none, ts_part, ts_dof):
    if a[0] != b[0] or a[0] != c[0]:
        raise SystemExit(f"FAIL: step mismatch {a[0]} {b[0]} {c[0]}")
    for name, row in (("particle", b), ("dof", c)):
        for i, lab in enumerate(("max_u", "com0x", "com0y"), start=2):
            d = abs(row[i] - a[i])
            max_du = max(max_du, d)
            if d > tol:
                n_bad += 1
                if n_bad <= 5:
                    print(f"FAIL ts step={a[0]} {name} {lab}: |d|={d:.3e}")
                    ok = False
print(f"timeseries scalar max|diff| = {max_du:.3e}")

def load_xref(path):
    raw = Path(path).read_bytes()
    n, = struct.unpack_from("<I", raw, 0)
    arr = np.frombuffer(raw, dtype="<f8", offset=4)
    if arr.size != 3 * n:
        raise SystemExit(f"x_ref size mismatch in {path}")
    return arr.reshape(n, 3)

def load_uv(path):
    raw = Path(path).read_bytes()
    if raw[:4] != b"PDUV":
        raise SystemExit(f"bad magic in {path}")
    step, n = struct.unpack_from("<II", raw, 4)
    arr = np.frombuffer(raw, dtype="<f8", offset=12)
    if arr.size != 6 * n:
        raise SystemExit(f"uv size mismatch in {path}")
    return step, arr.reshape(n, 6)

xr_none = load_xref("cmp_none_out/nodal/x_ref.bin")
xr_part = load_xref("cmp_particle_out/nodal/x_ref.bin")
xr_dof = load_xref("cmp_dof_out/nodal/x_ref.bin")
dxr = max(float(np.max(np.abs(xr_part - xr_none))),
          float(np.max(np.abs(xr_dof - xr_none))))
print(f"x_ref max|diff| vs none = {dxr:.3e} (n={xr_none.shape[0]})")
if dxr > 1e-14:
    raise SystemExit("FAIL: reference coordinates differ")

files_none = sorted(Path("cmp_none_out/nodal").glob("uv_*.bin"))
files_part = sorted(Path("cmp_particle_out/nodal").glob("uv_*.bin"))
files_dof = sorted(Path("cmp_dof_out/nodal").glob("uv_*.bin"))
if not (len(files_none) == len(files_part) == len(files_dof) and len(files_none) > 5):
    raise SystemExit(
        f"FAIL: nodal dump count none={len(files_none)} "
        f"particle={len(files_part)} dof={len(files_dof)}")

max_u_linf = max_v_linf = 0.0
for fn, fp, fd in zip(files_none, files_part, files_dof):
    s0, a = load_uv(fn)
    s1, b = load_uv(fp)
    s2, c = load_uv(fd)
    if s0 != s1 or s0 != s2:
        raise SystemExit(f"FAIL: nodal step mismatch {s0} {s1} {s2}")
    for arr in (b, c):
        du = arr[:, 0:3] - a[:, 0:3]
        dv = arr[:, 3:6] - a[:, 3:6]
        max_u_linf = max(max_u_linf, float(np.max(np.linalg.norm(du, axis=1))))
        max_v_linf = max(max_v_linf, float(np.max(np.linalg.norm(dv, axis=1))))

print(f"nodal over {len(files_none)} samples:")
print(f"  max L∞(|Δu|)={max_u_linf:.6e} (tol={tol_u})")
print(f"  max L∞(|Δv|)={max_v_linf:.6e} (tol={tol_v})")
fail_u = max_u_linf > tol_u
fail_v = max_v_linf > tol_v
if not ok or n_bad or fail_u or fail_v:
    print(f"FAIL: nodal identity (fail_u={fail_u} fail_v={fail_v})")
    raise SystemExit(1)
print("OK: serial, particle-MPI@2, DOF-MPI@2 match (scalars + full nodal u,v) "
      f"(pack {int('$NCOLS')}x{int('$NROWS')})")
PY
