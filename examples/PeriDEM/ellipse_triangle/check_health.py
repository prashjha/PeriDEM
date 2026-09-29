#!/usr/bin/env python3
"""Health gates for ellipse × tip-up triangle.

Desired: tip-seeded crack that grows into two spatially separated pieces
(not a glued tip tether, not fragment spray).
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

import meshio
import numpy as np

MAX_V = 25.0
EARLY_DAMAGE_CAP = 0.98
EARLY_MEAN_DAMAGE_CAP = 0.25
MIN_FINAL_DAMAGE = 0.85
MAX_FINAL_MEAN_DAMAGE = 0.50
MIN_DAMAGE_RISE_FRAMES = 1
MAX_COM_RADIUS = 0.012
MAX_COM_DRIFT = 0.012
# Tip tether: fully damaged ellipse node stuck on tip while COM has lifted.
TETHER_DIST = 1.5e-4  # 0.15 mm
TETHER_COM_LIFT = 5.0e-4
TETHER_DAMAGE = 0.9
# Two-piece: spatial clusters after fracture.
CLUSTER_LINK = 1.9e-4  # ~1.9 * mesh_size
MIN_CLUSTER_FRAC = 0.15
MIN_CLUSTER_SEP = 6.0e-4  # 0.6 mm between piece COMs
MESH_SIZE = 1.0e-4


def frame_index(p: Path) -> int:
    m = re.search(r"output_(\d+)\.vtu$", p.name)
    return int(m.group(1)) if m else -1


def spatial_clusters(
    X: np.ndarray, D: np.ndarray | None = None, link: float = CLUSTER_LINK, dam_cut: float = 0.75
) -> list[tuple[np.ndarray, int]]:
    """Union-find clusters. Highly damaged nodes are removed so a crack is a cut."""
    if len(X) == 0:
        return []
    if D is not None:
        alive = D < dam_cut
        if alive.sum() < 4:
            alive = np.ones(len(X), dtype=bool)
        X = X[alive]
    n = len(X)
    parent = np.arange(n)

    def find(a: int) -> int:
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    def union(a: int, b: int) -> None:
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[rb] = ra

    for i in range(n):
        for j in range(i + 1, n):
            if np.hypot(X[i, 0] - X[j, 0], X[i, 1] - X[j, 1]) < link:
                union(i, j)

    groups: dict[int, list[int]] = {}
    for i in range(n):
        r = find(i)
        groups.setdefault(r, []).append(i)
    out = []
    for idxs in groups.values():
        xi = X[np.asarray(idxs)]
        out.append((xi.mean(axis=0), len(idxs)))
    out.sort(key=lambda t: t[1], reverse=True)
    return out


def load_frames(out: Path):
    files = sorted(out.glob("output_*.vtu"), key=frame_index)
    files = [p for p in files if frame_index(p) >= 0]
    rows = []
    for p in files:
        m = meshio.read(p)
        pid = m.point_data["Particle_ID"].ravel().astype(int)
        X = m.points[:, :2]
        D_all = np.asarray(m.point_data["Damage"]).ravel()
        V_all = np.asarray(m.point_data["Velocity"])
        ell = pid == 1
        tri = pid == 0
        tip = X[tri][np.argmax(X[tri, 1])]
        D = D_all[ell]
        V = V_all[ell]
        Xe = X[ell]
        com = Xe.mean(axis=0)
        R = np.hypot(Xe[:, 0] - com[0], Xe[:, 1] - com[1])
        rmax = float(np.percentile(R, 99)) if len(R) else 0.0
        d_tip = np.linalg.norm(Xe - tip, axis=1)
        j = int(np.argmin(d_tip))
        clusters = spatial_clusters(Xe, D, CLUSTER_LINK)
        n_ref = max(1, int((D < 0.75).sum()) if len(D) else len(Xe))
        n_big = sum(1 for _, sz in clusters if sz >= MIN_CLUSTER_FRAC * n_ref)
        sep = 0.0
        if len(clusters) >= 2 and clusters[1][1] >= MIN_CLUSTER_FRAC * n_ref:
            sep = float(np.linalg.norm(clusters[0][0] - clusters[1][0]))
        rows.append(
            {
                "t_idx": frame_index(p),
                "dmax": float(D.max()) if len(D) else 0.0,
                "dmean": float(D.mean()) if len(D) else 0.0,
                "nd05": int((D > 0.05).sum()),
                "nd50": int((D > 0.5).sum()),
                "vmax": float(np.linalg.norm(V, axis=1).max()) if len(V) else 0.0,
                "rmax": rmax,
                "com": com,
                "tip": tip,
                "min_d_tip": float(d_tip[j]),
                "stuck_dam": float(D[j]),
                "n": int(ell.sum()),
                "n_big_clusters": n_big,
                "cluster_sep": sep,
                "cluster_sizes": [sz for _, sz in clusters[:4]],
            }
        )
    return rows


def main() -> int:
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "runs").resolve()
    rows = load_frames(out)
    if len(rows) < 6:
        print(f"FAIL: need ≥6 VTU frames in {out}, got {len(rows)}")
        return 1

    print(
        "fr  DamMax  DamMean  n>0.05  n>0.5  |v|max  Rcom  d_tip  stuckDam  "
        "nClus  sep_mm  sizes"
    )
    for i, r in enumerate(rows):
        print(
            f"{i:3d}  {r['dmax']:.4f}  {r['dmean']:.4f}  {r['nd05']:5d}  {r['nd50']:5d}  "
            f"{r['vmax']:6.2f}  {r['rmax']:.4f}  {r['min_d_tip']*1e3:5.2f}mm  "
            f"{r['stuck_dam']:.3f}  {r['n_big_clusters']:3d}  "
            f"{r['cluster_sep']*1e3:5.2f}  {r['cluster_sizes']}"
        )

    fails = []
    early = rows[1]
    late = rows[-1]
    peak_v = max(r["vmax"] for r in rows)
    peak_r = max(r["rmax"] for r in rows)
    com0 = rows[0]["com"]
    com_drift = float(np.linalg.norm(late["com"] - com0))

    rise = sum(1 for i in range(1, len(rows)) if rows[i]["dmax"] > rows[i - 1]["dmax"] + 1e-6)

    if early["dmax"] >= EARLY_DAMAGE_CAP and early["dmean"] >= EARLY_MEAN_DAMAGE_CAP:
        fails.append(
            f"early DamMax={early['dmax']:.3f} DamMean={early['dmean']:.3f} "
            f"(mass rupture at impact)"
        )
    if late["dmax"] < MIN_FINAL_DAMAGE:
        fails.append(f"final DamMax={late['dmax']:.3f} < {MIN_FINAL_DAMAGE} (no through crack)")
    if late["dmean"] > MAX_FINAL_MEAN_DAMAGE:
        fails.append(
            f"final DamMean={late['dmean']:.3f} > {MAX_FINAL_MEAN_DAMAGE} (ring pulverized)"
        )
    if rise < MIN_DAMAGE_RISE_FRAMES:
        fails.append(f"damage rise frames={rise} < {MIN_DAMAGE_RISE_FRAMES}")
    if peak_v > MAX_V:
        fails.append(f"peak |v|={peak_v:.1f} > {MAX_V} (fragment spray)")
    if peak_r > MAX_COM_RADIUS:
        fails.append(f"peak node-COM radius={peak_r:.4f} > {MAX_COM_RADIUS} (blown apart)")
    if com_drift > MAX_COM_DRIFT:
        fails.append(f"ellipse COM drift={com_drift:.4f} > {MAX_COM_DRIFT}")

    # Abrupt full-field jump is bad; tip-seeded DamMax jump with low mean is OK.
    for i in range(1, len(rows)):
        if (
            rows[i - 1]["dmax"] < 0.05
            and rows[i]["dmax"] > 0.95
            and rows[i]["dmean"] > 0.3
        ):
            fails.append(
                f"Damage jump {rows[i-1]['dmax']:.2f}→{rows[i]['dmax']:.2f} "
                f"DamMean={rows[i]['dmean']:.2f} at frame {i}"
            )
            break

    # Two-piece bipartition in the last third of the run.
    split_ok = False
    best_sep = 0.0
    for r in rows[len(rows) // 2 :]:
        best_sep = max(best_sep, r["cluster_sep"])
        if r["n_big_clusters"] >= 2 and r["cluster_sep"] >= MIN_CLUSTER_SEP:
            split_ok = True
            break
    if not split_ok:
        fails.append(
            f"no two-piece split (need ≥2 clusters ≥{MIN_CLUSTER_FRAC*100:.0f}% nodes "
            f"with COM sep≥{MIN_CLUSTER_SEP*1e3:.1f}mm; best_sep={best_sep*1e3:.2f}mm, "
            f"late sizes={late['cluster_sizes']})"
        )

    tether_frames = 0
    for r in rows[len(rows) // 3 :]:
        lift = r["com"][1] - r["tip"][1]
        if (
            r["min_d_tip"] < TETHER_DIST
            and r["stuck_dam"] >= TETHER_DAMAGE
            and lift > TETHER_COM_LIFT
        ):
            tether_frames += 1
    if tether_frames >= 3:
        fails.append(
            f"tip tether: {tether_frames} frames with Dam≥{TETHER_DAMAGE} node "
            f"stuck within {TETHER_DIST*1e3:.2f}mm of tip while COM lifted"
        )

    if fails:
        print("FAIL:")
        for f in fails:
            print(" ", f)
        return 1
    print(
        f"PASS: two-piece DamMax {early['dmax']:.3f}→{late['dmax']:.3f}, "
        f"DamMean={late['dmean']:.3f}, peak|v|={peak_v:.2f}, "
        f"cluster_sep={late['cluster_sep']*1e3:.2f}mm, sizes={late['cluster_sizes']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
