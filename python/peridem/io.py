# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Read PeriDEM VTU/PVTU and write a snapshot PNG."""

from __future__ import annotations

from pathlib import Path
from typing import Any

import numpy as np


def _reader_for(path: Path):
    import vtk

    suffix = path.suffix.lower()
    if suffix == ".pvtu":
        return vtk.vtkXMLPUnstructuredGridReader()
    if suffix == ".vtu":
        return vtk.vtkXMLUnstructuredGridReader()
    raise ValueError(f"unsupported mesh file: {path}")


def read_vtu(path: str | Path) -> dict[str, Any]:
    """Load a PeriDEM .vtu or .pvtu.

    Returns
    -------
    dict
        ``points`` (N, 3), ``point_data`` name → array, ``n_points``.
    """
    from vtk.util.numpy_support import vtk_to_numpy

    p = Path(path)
    if not p.is_file():
        raise FileNotFoundError(p)
    reader = _reader_for(p)
    reader.SetFileName(str(p))
    reader.Update()
    grid = reader.GetOutput()
    points = np.array(vtk_to_numpy(grid.GetPoints().GetData()), dtype=np.float64)
    pdata: dict[str, np.ndarray] = {}
    pd = grid.GetPointData()
    for i in range(pd.GetNumberOfArrays()):
        arr = pd.GetArray(i)
        name = arr.GetName()
        val = np.array(vtk_to_numpy(arr), dtype=np.float64)
        pdata[name] = val
    return {"points": points, "point_data": pdata, "n_points": int(points.shape[0])}


def _frame_index(path: Path) -> int:
    stem = path.stem
    digits = ""
    for ch in reversed(stem):
        if ch.isdigit():
            digits = ch + digits
        elif digits:
            break
    return int(digits) if digits else -1


def last_vtu(out_dir: str | Path) -> Path:
    """Latest assembled output under ``out_dir``.

    Prefers ``.pvtu`` (all MPI ranks) over a single rank piece.
    """
    root = Path(out_dir)
    pvtus = list(root.rglob("*.pvtu"))
    if pvtus:
        return max(pvtus, key=lambda f: (_frame_index(f), str(f)))
    vtus = [
        f
        for f in root.rglob("*.vtu")
        if "_r" not in f.stem.split("output_")[-1]
    ]
    if not vtus:
        vtus = list(root.rglob("*.vtu"))
    if not vtus:
        raise FileNotFoundError(f"no vtu/pvtu under {root}")
    return max(vtus, key=lambda f: (_frame_index(f), str(f)))


def _xref_key(xref: np.ndarray, decimals: int = 12) -> np.ndarray:
    """Integer key per node from rounded reference coordinates."""
    r = np.round(xref, decimals)
    scale = 10**decimals
    return (
        np.rint(r[:, 0] * scale).astype(np.int64) * 1_000_003
        + np.rint(r[:, 1] * scale).astype(np.int64) * 1009
        + np.rint(r[:, 2] * scale).astype(np.int64)
    )


def unique_by_xref(fields: dict[str, np.ndarray]) -> dict[str, np.ndarray]:
    """Drop duplicate nodes from MPI piece assembly (shared interface nodes)."""
    xref = np.asarray(fields["x_ref"], dtype=np.float64)
    keys = _xref_key(xref)
    _, first = np.unique(keys, return_index=True)
    first = np.sort(first)
    out = {}
    for name, arr in fields.items():
        a = np.asarray(arr)
        out[name] = a[first] if a.shape[0] == xref.shape[0] else a
    return out


def fields_from_vtu(path: str | Path) -> dict[str, np.ndarray]:
    """Point coordinates and named arrays, plus ``x_ref = x - u`` when possible."""
    data = read_vtu(path)
    out: dict[str, np.ndarray] = {"points": data["points"]}
    out.update(data["point_data"])
    if "Displacement" in out:
        u = np.atleast_2d(out["Displacement"])
        if u.shape[1] == 1:
            u = u.reshape(-1, 1)
        out["x_ref"] = data["points"][:, : u.shape[1]] - u
    else:
        out["x_ref"] = data["points"].copy()
    return unique_by_xref(out)


def _order_by_xref(xref: np.ndarray) -> np.ndarray:
    return np.lexsort((xref[:, 2], xref[:, 1], xref[:, 0]))


def _align(a: dict[str, np.ndarray], b: dict[str, np.ndarray]) -> tuple[np.ndarray, np.ndarray]:
    """Permutation of ``b`` so ``b_xref[perm]`` matches ``a_xref``."""
    xa = np.asarray(a["x_ref"], dtype=np.float64)
    xb = np.asarray(b["x_ref"], dtype=np.float64)
    if xa.shape != xb.shape:
        raise ValueError(f"node count mismatch {xa.shape} vs {xb.shape}")
    ia = _order_by_xref(xa)
    ib = _order_by_xref(xb)
    if np.max(np.abs(xa[ia] - xb[ib])) > 1e-12:
        raise ValueError(
            f"reference coordinates do not match (max |Δx_ref|="
            f"{np.max(np.abs(xa[ia] - xb[ib])):g})"
        )
    # perm such that xb[perm] == xa in original a-order
    inv_ib = np.empty_like(ib)
    inv_ib[ib] = np.arange(ib.size)
    # after lexsort both are the same physical order; map a-index → b-index
    # xa[i] == xb[perm[i]]
    perm = np.empty(xa.shape[0], dtype=int)
    perm[ia] = ib
    return ia, perm


def nodal_error(
    a: dict[str, np.ndarray],
    b: dict[str, np.ndarray],
    names: tuple[str, ...] = ("Displacement", "Velocity", "Force", "Damage_Z", "Damage"),
) -> dict[str, dict[str, float]]:
    """Per-node error of matching fields.

    Vector fields: ``e_i = ||a_i - b_i||_2``. Scalar: ``e_i = |a_i - b_i|``.
    Returns L∞ (max over nodes) and RMS for each name present in both.
    """
    _, perm = _align(a, b)
    report: dict[str, dict[str, float]] = {}
    n = a["x_ref"].shape[0]
    for name in names:
        if name not in a or name not in b:
            continue
        va = np.asarray(a[name], dtype=np.float64)
        vb = np.asarray(b[name], dtype=np.float64)[perm]
        if va.shape != vb.shape:
            raise ValueError(f"{name} shape {va.shape} vs {vb.shape}")
        if va.ndim == 1 or (va.ndim == 2 and va.shape[1] == 1):
            e = np.abs(va.reshape(-1) - vb.reshape(-1))
            comp = {
                "Linf_x": float(np.max(e)),
            }
        else:
            d = va - vb
            e = np.linalg.norm(d, axis=1)
            comp = {
                "Linf_x": float(np.max(np.abs(d[:, 0]))),
                "Linf_y": float(np.max(np.abs(d[:, 1]))),
                "Linf_z": float(np.max(np.abs(d[:, 2]))) if d.shape[1] > 2 else 0.0,
            }
        report[name] = {
            "n": float(n),
            "Linf": float(np.max(e)),
            "rms": float(np.sqrt(np.mean(e * e))),
            "argmax": float(int(np.argmax(e))),
            **comp,
        }
    return report


def snapshot(
    source: str | Path | dict[str, Any],
    png: str | Path,
    *,
    color: str = "Displacement",
    title: str | None = None,
) -> Path:
    """Write a 2D scatter PNG. ``color`` is a point-data name, or ``|name|``."""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    data = source if isinstance(source, dict) else read_vtu(source)
    xy = data["points"][:, :2]
    c = _color_values(data["point_data"], color)
    out = Path(png)
    out.parent.mkdir(parents=True, exist_ok=True)
    fig, ax = plt.subplots(figsize=(6, 5), dpi=120)
    sc = ax.scatter(xy[:, 0], xy[:, 1], c=c, s=8, cmap="viridis", linewidths=0)
    fig.colorbar(sc, ax=ax, label=color)
    ax.set_aspect("equal", adjustable="box")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_title(title or color)
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)
    if out.stat().st_size < 100:
        raise RuntimeError(f"snapshot looks empty: {out}")
    return out


def _color_values(point_data: dict[str, np.ndarray], color: str) -> np.ndarray:
    mag = color.startswith("|") and color.endswith("|")
    name = color[1:-1] if mag else color
    if name not in point_data:
        raise KeyError(f"{name} not in point_data {sorted(point_data)}")
    a = point_data[name]
    if a.ndim == 1:
        return a
    return np.linalg.norm(a, axis=1) if mag or a.shape[-1] > 1 else a.reshape(-1)
