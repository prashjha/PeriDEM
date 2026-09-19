#!/usr/bin/env bash
# Three-way MPI identity: serial (none) vs particle@2 vs dof@2 on twop impact
# with bottom-patch DispBC (deformable base).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
BIN="${BIN:-$HERE/Test_PeriDEM_twop_circ_inbuilt}"
if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — build Test_PeriDEM_twop_circ_inbuilt" >&2
  exit 1
fi

# Prefer system OpenMPI (--quiet) when available.
export PATH="/usr/bin:${PATH:-}"
MPI_EXTRA=()
if mpirun --help 2>&1 | grep -q -- '--quiet'; then
  MPI_EXTRA+=(--quiet)
fi

ARGS=(-nThreads 1 -bottomPatchBC -writeMpiMetric -epsN 1.0
      -finalTime 0.008 -numSteps 24000 -requireContact)

rm -rf "$HERE/mpi_cmp_none_out" "$HERE/mpi_cmp_particle_out" "$HERE/mpi_cmp_dof_out"
mkdir -p "$HERE/mpi_cmp_none_out" "$HERE/mpi_cmp_particle_out" "$HERE/mpi_cmp_dof_out"

echo "=== serial / none ==="
mpirun -n 1 "${MPI_EXTRA[@]}" "$BIN" "${ARGS[@]}" -mpiStrategy none \
  -outputDir "$HERE/mpi_cmp_none_out"

echo "=== particle-MPI @2 ==="
mpirun -n 2 "${MPI_EXTRA[@]}" "$BIN" "${ARGS[@]}" -mpiStrategy particle \
  -outputDir "$HERE/mpi_cmp_particle_out"

echo "=== DOF-MPI @2 ==="
mpirun -n 2 "${MPI_EXTRA[@]}" "$BIN" "${ARGS[@]}" -mpiStrategy dof \
  -outputDir "$HERE/mpi_cmp_dof_out"

python3 - <<PY
from pathlib import Path
import struct
import numpy as np

tol_scalar = 1e-8
# Absolute L∞ on nodal vector magnitude |Δu|, |Δv| (gathered global fields).
tol_u = 1e-12
tol_v = 1e-9   # contact/CD accumulates ~1e-11 noise; still << physical |v|


def load_end(p):
    a = Path(p).read_text().split()
    return float(a[0]), float(a[1]), float(a[2])
none = load_end("$HERE/mpi_cmp_none_out/mpi_metric.txt")
part = load_end("$HERE/mpi_cmp_particle_out/mpi_metric.txt")
dof = load_end("$HERE/mpi_cmp_dof_out/mpi_metric.txt")
print(f"none:     max|u|={none[0]:.12e} com1=({none[1]:.12e},{none[2]:.12e})")
print(f"particle: max|u|={part[0]:.12e} com1=({part[1]:.12e},{part[2]:.12e})")
print(f"dof:      max|u|={dof[0]:.12e} com1=({dof[1]:.12e},{dof[2]:.12e})")
ok = True
for name, a in (("particle", part), ("dof", dof)):
    for i, lab in enumerate(("max|u|", "com1x", "com1y")):
        d = abs(a[i] - none[i])
        print(f"  |{name}-{lab} - none| = {d:.3e}")
        if d > tol_scalar:
            ok = False
            print(f"FAIL: {name} {lab} mismatch (tol={tol_scalar})")

def load_ts(p):
    rows = []
    for line in Path(p).read_text().splitlines()[1:]:
        if not line.strip():
            continue
        step, t, mu, cx, cy = line.split(",")
        rows.append((int(step), float(t), float(mu), float(cx), float(cy)))
    return rows
ts_none = load_ts("$HERE/mpi_cmp_none_out/mpi_metric_ts.csv")
ts_part = load_ts("$HERE/mpi_cmp_particle_out/mpi_metric_ts.csv")
ts_dof = load_ts("$HERE/mpi_cmp_dof_out/mpi_metric_ts.csv")
print(f"timeseries samples: none={len(ts_none)} particle={len(ts_part)} dof={len(ts_dof)}")
if not (len(ts_none) == len(ts_part) == len(ts_dof) and len(ts_none) > 5):
    raise SystemExit(f"FAIL: timeseries length mismatch or too short")
max_du = 0.0
n_bad = 0
for a, b, c in zip(ts_none, ts_part, ts_dof):
    if a[0] != b[0] or a[0] != c[0]:
        raise SystemExit(f"FAIL: step mismatch {a[0]} {b[0]} {c[0]}")
    for name, row in (("particle", b), ("dof", c)):
        for i, lab in enumerate(("max_u", "com1x", "com1y"), start=2):
            d = abs(row[i] - a[i])
            max_du = max(max_du, d)
            if d > tol_scalar:
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
        raise SystemExit(f"uv size mismatch in {path}: {arr.size} vs {6*n}")
    return step, arr.reshape(n, 6)

# Reference coords must match (same node order)
xr_none = load_xref("$HERE/mpi_cmp_none_out/nodal/x_ref.bin")
xr_part = load_xref("$HERE/mpi_cmp_particle_out/nodal/x_ref.bin")
xr_dof = load_xref("$HERE/mpi_cmp_dof_out/nodal/x_ref.bin")
if xr_none.shape != xr_part.shape or xr_none.shape != xr_dof.shape:
    raise SystemExit("FAIL: x_ref node count mismatch")
dxr = max(np.max(np.abs(xr_part - xr_none)), np.max(np.abs(xr_dof - xr_none)))
print(f"x_ref max|diff| vs none = {dxr:.3e} (n={xr_none.shape[0]})")
if dxr > 1e-14:
    raise SystemExit("FAIL: reference coordinates differ — node order/mesh mismatch")

def nodal_dir(tag):
    return Path(f"$HERE/mpi_cmp_{tag}_out/nodal")

files_none = sorted(nodal_dir("none").glob("uv_*.bin"))
files_part = sorted(nodal_dir("particle").glob("uv_*.bin"))
files_dof = sorted(nodal_dir("dof").glob("uv_*.bin"))
if not (len(files_none) == len(files_part) == len(files_dof) and len(files_none) > 5):
    raise SystemExit(
        f"FAIL: nodal dump count mismatch none={len(files_none)} "
        f"particle={len(files_part)} dof={len(files_dof)}")

max_u_linf = max_u_l2 = max_v_linf = max_v_l2 = 0.0
worst = None
for fn, fp, fd in zip(files_none, files_part, files_dof):
    s0, a = load_uv(fn)
    s1, b = load_uv(fp)
    s2, c = load_uv(fd)
    if s0 != s1 or s0 != s2:
        raise SystemExit(f"FAIL: nodal step mismatch {s0} {s1} {s2}")
    for name, arr in (("particle", b), ("dof", c)):
        du = arr[:, 0:3] - a[:, 0:3]
        dv = arr[:, 3:6] - a[:, 3:6]
        u_linf = float(np.max(np.linalg.norm(du, axis=1)))
        v_linf = float(np.max(np.linalg.norm(dv, axis=1)))
        u_l2 = float(np.linalg.norm(du) / np.sqrt(max(du.shape[0], 1)))
        v_l2 = float(np.linalg.norm(dv) / np.sqrt(max(dv.shape[0], 1)))
        max_u_linf = max(max_u_linf, u_linf)
        max_v_linf = max(max_v_linf, v_linf)
        max_u_l2 = max(max_u_l2, u_l2)
        max_v_l2 = max(max_v_l2, v_l2)
        if u_linf > tol_u or v_linf > tol_v:
            ok = False
        if worst is None or u_linf + v_linf > worst[0]:
            iu = int(np.argmax(np.linalg.norm(du, axis=1)))
            iv = int(np.argmax(np.linalg.norm(dv, axis=1)))
            worst = (u_linf + v_linf, s0, name, iu, u_linf, iv, v_linf,
                     a[iu, 0:3], arr[iu, 0:3], a[iv, 3:6], arr[iv, 3:6])

print(f"nodal over {len(files_none)} samples (n={xr_none.shape[0]} nodes):")
print(f"  max L∞(|Δu|)={max_u_linf:.6e}  max L2_rms(|Δu|)={max_u_l2:.6e}  (tol={tol_u})")
print(f"  max L∞(|Δv|)={max_v_linf:.6e}  max L2_rms(|Δv|)={max_v_l2:.6e}  (tol={tol_v})")
if worst is not None:
    _, s0, name, iu, u_linf, iv, v_linf, u0, u1, v0, v1 = worst
    print(f"WORST sample: step={s0} vs={name} node_u={iu} |Δu|∞={u_linf:.6e} "
          f"node_v={iv} |Δv|∞={v_linf:.6e}")
    print(f"  u_none={u0} u_mpi={u1}")
    print(f"  v_none={v0} v_mpi={v1}")
fail_u = max_u_linf > tol_u
fail_v = max_v_linf > tol_v
if not ok or n_bad or fail_u or fail_v:
    print(f"FAIL: nodal identity (fail_u={fail_u} fail_v={fail_v})")
    raise SystemExit(1)
print("OK: serial, particle-MPI@2, DOF-MPI@2 match (scalars + full nodal u,v)")
PY
