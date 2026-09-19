#!/usr/bin/env bash
# Quick identity: serial vs Particle-MPI@2 vs DOF-MPI@2
# (two grains + one fixed wall; BC + PP + wall contact; final nodal u,v).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
# Prefer sibling binary; ignore a stale BIN from the environment.
BIN="$HERE/Test_PeriDEM_mpi_identity_twop_wall"
if [[ ! -x "$BIN" ]]; then
  echo "missing $BIN — build Test_PeriDEM_mpi_identity_twop_wall" >&2
  exit 1
fi

MPIEXEC="${MPIEXEC:-mpirun}"

rm -rf "$HERE/id_none_out" "$HERE/id_none_inp"
rm -rf "$HERE/id_particle_out" "$HERE/id_particle_inp"
rm -rf "$HERE/id_dof_out" "$HERE/id_dof_inp"

echo "=== serial / none ==="
"$MPIEXEC" -n 1 "$BIN" -nThreads 1 -mpiStrategy none \
  -outputDir "$HERE/id_none_out" -inputDir "$HERE/id_none_inp"

echo "=== Particle-MPI @2 ==="
"$MPIEXEC" -n 2 "$BIN" -nThreads 1 -mpiStrategy particle \
  -outputDir "$HERE/id_particle_out" -inputDir "$HERE/id_particle_inp"

echo "=== DOF-MPI @2 ==="
"$MPIEXEC" -n 2 "$BIN" -nThreads 1 -mpiStrategy dof \
  -outputDir "$HERE/id_dof_out" -inputDir "$HERE/id_dof_inp"

python3 - <<PY
from pathlib import Path
import struct
import numpy as np

HERE = Path("$HERE")
tol_u = 1e-12
tol_v = 1e-9

def load_xref(p):
    raw = Path(p).read_bytes()
    n = struct.unpack_from("<I", raw, 0)[0]
    return np.frombuffer(raw, dtype="<f8", count=3 * n, offset=4).reshape(n, 3)

def load_uv(p):
    raw = Path(p).read_bytes()
    assert raw[:4] == b"PDUV"
    step, n = struct.unpack_from("<II", raw, 4)
    arr = np.frombuffer(raw, dtype="<f8", count=6 * n, offset=12).reshape(n, 6)
    return step, arr

xr_none = load_xref(HERE / "id_none_out/nodal/x_ref.bin")
xr_part = load_xref(HERE / "id_particle_out/nodal/x_ref.bin")
xr_dof = load_xref(HERE / "id_dof_out/nodal/x_ref.bin")
if xr_none.shape != xr_part.shape or xr_none.shape != xr_dof.shape:
    raise SystemExit("FAIL: x_ref node count mismatch")
dxr = max(float(np.max(np.abs(xr_part - xr_none))),
          float(np.max(np.abs(xr_dof - xr_none))))
print(f"x_ref max|diff| vs none = {dxr:.3e} (n={xr_none.shape[0]})")
if dxr > 0.:
    raise SystemExit("FAIL: reference coordinates differ")

s0, a0 = load_uv(HERE / "id_none_out/nodal/final_uv.bin")
s1, a1 = load_uv(HERE / "id_particle_out/nodal/final_uv.bin")
s2, a2 = load_uv(HERE / "id_dof_out/nodal/final_uv.bin")
if not (s0 == s1 == s2):
    raise SystemExit(f"FAIL: final step mismatch {s0} {s1} {s2}")

fail_u = fail_v = 0
max_du = max_dv = 0.
worst = None
for name, arr in (("particle", a1), ("dof", a2)):
    du = np.linalg.norm(arr[:, 0:3] - a0[:, 0:3], axis=1)
    dv = np.linalg.norm(arr[:, 3:6] - a0[:, 3:6], axis=1)
    iu = int(np.argmax(du))
    iv = int(np.argmax(dv))
    mdu = float(du[iu])
    mdv = float(dv[iv])
    max_du = max(max_du, mdu)
    max_dv = max(max_dv, mdv)
    if mdu > tol_u:
        fail_u += 1
    if mdv > tol_v:
        fail_v += 1
    if worst is None or mdv > worst[0]:
        worst = (mdv, name, iu, iv, arr)

print(f"final step={s0} nodes={a0.shape[0]}")
print(f"  max L∞(|Δu|)={max_du:.6e} (tol={tol_u})")
print(f"  max L∞(|Δv|)={max_dv:.6e} (tol={tol_v})")
if worst is not None:
    mdv, name, iu, iv, arr = worst
    print(f"WORST vs={name} node_u={iu} |Δu|∞={np.linalg.norm(arr[iu,0:3]-a0[iu,0:3]):.6e} "
          f"node_v={iv} |Δv|∞={mdv:.6e}")
if fail_u or fail_v:
    raise SystemExit(f"FAIL: nodal identity (fail_u={fail_u} fail_v={fail_v})")
print("OK: serial, Particle-MPI@2, DOF-MPI@2 match (final nodal u,v)")
PY
