#!/usr/bin/env python3
"""Health gates for attrition sim2 (thin container, R_out=0.0203).

Containment about drum axis (0,0). Wall rotates about an offset, but the
cavity geometry is still centered at the origin.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

import meshio
import numpy as np

R_IN = 0.02
R_OUT = 0.0203
PEN_TOL = 8.0e-5
MAX_V = 40.0
EARLY_MEAN_DAMAGE_MAX = 0.08
MIN_FINAL_DAMAGE = 0.1
MIN_DAMAGED_NODES = 10
MIN_DAMAGED_PARTICLES = 2
MIN_FRAC_DAMAGE = 0.001
AXIS = np.array([0.0, 0.0])


def frame_key(p: Path) -> int:
    m = re.search(r"output_0_(\d+)_r", p.name)
    return int(m.group(1)) if m else -1


def wall_id_and_T(here: Path, deck_name: str | None) -> tuple[int, float]:
    """Pick wall id / Final_Time from the deck that was run."""
    names = []
    if deck_name:
        names.append(Path(deck_name).name)
    names.extend(["input_short.json", "input_medium.json", "input.json"])
    seen = set()
    for name in names:
        if name in seen:
            continue
        seen.add(name)
        p = here / name
        if not p.is_file():
            continue
        j = json.loads(p.read_text())
        wid = int(j["Displacement_BC"]["Set_1"]["Particle_List"][0])
        T = float(j["Model"]["Final_Time"])
        return wid, T
    return 58, 0.1


def load_frame(out: Path, fr: int, wall_id: int):
    xs, ids, dm, vs = [], [], [], []
    for r in sorted(out.glob(f"output_0_{fr}_r*.vtu")):
        m = meshio.read(r)
        xs.append(m.points[:, :2])
        ids.append(m.point_data["Particle_ID"].ravel())
        if "Damage" in m.point_data:
            dm.append(m.point_data["Damage"].ravel())
        if "Velocity" in m.point_data:
            vs.append(m.point_data["Velocity"])
    X = np.concatenate(xs)
    I = np.concatenate(ids).astype(int)
    wall = I == wall_id
    grain = ~wall
    R = np.hypot(X[grain, 0] - AXIS[0], X[grain, 1] - AXIS[1])
    Rw = np.hypot(X[wall, 0] - AXIS[0], X[wall, 1] - AXIS[1])
    D = np.concatenate(dm)[grain] if dm else np.zeros(grain.sum())
    V = np.concatenate(vs)[grain] if vs else np.zeros((grain.sum(), 3))
    Ig = I[grain]
    vmax = float(np.linalg.norm(V, axis=1).max()) if len(V) else 0.0
    n_part_dmg = 0
    for pid in set(Ig.tolist()):
        if D[Ig == pid].max() > 0.1:
            n_part_dmg += 1
    n_com_out = 0
    for pid in set(Ig.tolist()):
        com = X[grain][Ig == pid].mean(axis=0)
        if np.hypot(com[0] - AXIS[0], com[1] - AXIS[1]) > R_IN:
            n_com_out += 1
    return R, D, vmax, Rw, n_part_dmg, n_com_out


def main() -> int:
    here = Path(__file__).resolve().parent
    out = Path(sys.argv[1] if len(sys.argv) > 1 else here / "runs" / "out").resolve()
    deck_arg = sys.argv[2] if len(sys.argv) > 2 and not sys.argv[2].startswith("-") else None
    wall_id, T = wall_id_and_T(here, deck_arg)
    require_wear = T >= 0.03 and "--ic-only" not in sys.argv

    ranks0 = sorted(out.glob("output_0_*_r0.vtu"), key=frame_key)
    frames = [frame_key(p) for p in ranks0 if frame_key(p) >= 0]
    if len(frames) < 2:
        print(f"FAIL: need ≥2 frames in {out}, got {len(frames)}")
        return 1

    print(
        f"wall_id={wall_id}  T={T}  require_wear={require_wear}  "
        f"R_in={R_IN}  R_out={R_OUT}  (radius about drum axis (0,0))"
    )
    print(
        "fr   maxR_ax  n_pen  n_thru  n_com_out  DamMax  DamMean  n_part_dmg  |v|max  wallR"
    )
    series = []
    fails = []
    for fr in frames:
        R, D, vmax, Rw, n_part, n_com = load_frame(out, fr, wall_id)
        n_pen = int((R > R_IN + PEN_TOL).sum())
        n_thru = int((R > R_OUT).sum())
        series.append(
            (
                fr,
                float(R.max()),
                n_pen,
                n_thru,
                n_com,
                float(D.max()),
                float(D.mean()),
                n_part,
                vmax,
                float(Rw.min()),
                float(Rw.max()),
            )
        )
        s = series[-1]
        print(
            f"{fr:4d}  {s[1]:.5f}  {n_pen:5d}  {n_thru:5d}  {n_com:5d}  "
            f"{s[5]:.4f}  {s[6]:.4f}  {n_part:5d}  {vmax:.3f}  "
            f"[{s[9]:.4f},{s[10]:.4f}]"
        )
        if fr == frames[0]:
            if s[9] < 0.014 or s[10] > 0.021:
                fails.append(
                    f"IC wall mesh shifted: wall R=[{s[9]:.5f},{s[10]:.5f}] "
                    f"(expect ~[0.015,0.0203])"
                )
            if n_pen > 0 or n_thru > 0:
                fails.append(f"IC grain/wall overlap: n_pen={n_pen} n_thru={n_thru}")
        if n_thru > 0:
            fails.append(f"fr={fr}: {n_thru} nodes through wall (R>{R_OUT})")
        elif n_pen > 0:
            fails.append(f"fr={fr}: {n_pen} nodes past R_in+tol (maxR={R.max():.6f})")
        if n_com > 0:
            fails.append(f"fr={fr}: {n_com} particle COMs outside R_in")

    early = series[1] if len(series) > 1 else series[0]
    late = series[-1]
    peak_v = max(s[8] for s in series)

    if early[8] > MAX_V:
        fails.append(f"early |v|={early[8]:.1f} > {MAX_V}")
    if peak_v > MAX_V:
        fails.append(f"peak |v|={peak_v:.1f} > {MAX_V}")
    if early[6] > EARLY_MEAN_DAMAGE_MAX:
        fails.append(f"early DamMean={early[6]:.4f} > {EARLY_MEAN_DAMAGE_MAX}")

    if require_wear:
        if late[5] < MIN_FINAL_DAMAGE:
            fails.append(f"final DamMax={late[5]:.4f} < {MIN_FINAL_DAMAGE}")
        n_dmg = int((load_frame(out, late[0], wall_id)[1] > 0.05).sum())
        if n_dmg < MIN_DAMAGED_NODES:
            fails.append(f"damaged nodes={n_dmg} < {MIN_DAMAGED_NODES}")
        if late[7] < MIN_DAMAGED_PARTICLES:
            fails.append(f"damaged particles={late[7]} < {MIN_DAMAGED_PARTICLES}")
        if late[6] < MIN_FRAC_DAMAGE * 0.5 and n_dmg < MIN_DAMAGED_NODES:
            fails.append("insufficient attrition damage")
        if early[5] > 0.95 and early[6] > 0.1:
            fails.append("early frame already mass-ruptured")
    else:
        n_dmg = int((load_frame(out, late[0], wall_id)[1] > 0.05).sum())
        print(f"(IC/short mode: skip wear gates; DamMax={late[5]:.4f} n_dmg={n_dmg})")

    if fails:
        seen = set()
        print("FAIL:")
        for f in fails:
            if f not in seen:
                print(" ", f)
                seen.add(f)
        return 1
    print(
        f"PASS: zero escape maxR={late[1]:.5f}, DamMax {early[5]:.3f}→{late[5]:.3f}, "
        f"n_part_dmg={late[7]}, peak|v|={peak_v:.2f}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
