# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Shared helpers for Python example verification."""

from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path
from typing import Any

import numpy as np
import peridem

REPO = Path(__file__).resolve().parents[2]
BIN = REPO / "build/linux/bin/PeriDEM"


def load_deck(src: Path) -> dict[str, Any]:
    return json.loads(src.read_text())


def _abs_mesh_files(deck: dict[str, Any], src_dir: Path) -> None:
    mesh = deck.get("Mesh") or {}
    for key, block in mesh.items():
        if not key.startswith("Set_") or not isinstance(block, dict):
            continue
        f = block.get("File")
        if not f:
            continue
        p = Path(f)
        candidates = []
        if p.is_absolute():
            candidates.append(p)
            candidates.append(src_dir / "meshes" / p.name)
        else:
            candidates.append(src_dir / p)
            candidates.append(src_dir / "meshes" / p.name)
        for cand in candidates:
            if cand.is_file():
                block["File"] = str(cand.resolve())
                break


def prepare_deck(
    src_json: Path,
    out_dir: Path,
    *,
    time_steps: int | None = None,
    mpi_strategy: str | None = None,
    mesh_size: float | None = None,
    debug: int = 0,
) -> Path:
    """Write a runnable input.json under out_dir. Returns that path."""
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    deck = load_deck(src_json)
    _abs_mesh_files(deck, src_json.parent)
    mesh = deck.get("Mesh") or {}
    for key, block in mesh.items():
        if not key.startswith("Set_") or not isinstance(block, dict):
            continue
        f = block.get("File")
        if f and not Path(f).is_file():
            block["File"] = str((out_dir / Path(f).name).resolve())
        cm = block.get("CreateMesh")
        if mesh_size is not None and isinstance(cm, dict) and cm.get("Flag"):
            cm["Mesh_Size"] = mesh_size
            block["File"] = str((out_dir / f"{key}.msh").resolve())
    if mesh_size is not None:
        mat = deck.get("Material") or {}
        for key, block in mat.items():
            if key.startswith("Set_") and isinstance(block, dict) and "Horizon" in block:
                block["Horizon"] = 3.0 * mesh_size
    if time_steps is not None:
        dt = deck["Model"]["Final_Time"] / deck["Model"]["Time_Steps"]
        deck["Model"]["Time_Steps"] = int(time_steps)
        deck["Model"]["Final_Time"] = dt * int(time_steps)
    if mpi_strategy is not None:
        deck["Model"]["MPI_Strategy"] = mpi_strategy
    deck.setdefault("Output", {})
    deck["Output"]["Path"] = str(out_dir / "out") + "/"
    deck["Output"]["Debug"] = debug
    deck["Output"]["Perform_Out"] = True
    if "Output_Interval" in deck["Output"]:
        nt = deck["Model"]["Time_Steps"]
        deck["Output"]["Output_Interval"] = max(1, nt)
    dest = out_dir / "input.json"
    dest.write_text(json.dumps(deck, indent=2))
    return dest


def run_python(
    src_json: Path,
    out_dir: Path,
    *,
    time_steps: int | None = None,
    mpi_strategy: str | None = None,
    mesh_size: float | None = None,
    n_threads: int = 1,
    debug: int = 0,
) -> peridem.Simulation:
    inp = prepare_deck(
        src_json,
        out_dir,
        time_steps=time_steps,
        mpi_strategy=mpi_strategy,
        mesh_size=mesh_size,
        debug=debug,
    )
    peridem.init(n_threads=n_threads)
    sim = peridem.Simulation.from_file(str(inp))
    sim.run()
    return sim


def run_cpp(deck_dir: Path, n_threads: int = 1) -> subprocess.CompletedProcess:
    if not BIN.is_file():
        raise FileNotFoundError(BIN)
    return subprocess.run(
        [str(BIN), "-i", "input.json", "-nThreads", str(n_threads)],
        cwd=deck_dir,
        capture_output=True,
        text=True,
        check=False,
    )


def check_fields(sim: peridem.Simulation) -> None:
    assert sim.n_nodes > 0
    u = sim.displacement
    assert np.isfinite(u).all()
    assert np.allclose(sim.position, sim.reference + u, atol=1e-12)


def vtu_disp(out_dir: Path) -> np.ndarray:
    data = peridem.read_vtu(peridem.last_vtu(out_dir))
    return data["point_data"]["Displacement"]


def load_fields(out_dir: Path) -> dict:
    return peridem.fields_from_vtu(peridem.last_vtu(out_dir))


def linf(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.max(np.abs(a - b)))


def format_nodal(err: dict) -> str:
    parts = []
    for name, e in err.items():
        parts.append(
            f"{name}: L∞={e['Linf']:.6e} rms={e['rms']:.6e} "
            f"(ex={e.get('Linf_x', e['Linf']):.3e} "
            f"ey={e.get('Linf_y', 0):.3e} ez={e.get('Linf_z', 0):.3e} "
            f"i={int(e['argmax'])})"
        )
    return " | ".join(parts) if parts else "no common fields"


def write_snapshot(out_dir: Path, name: str, color: str = "|Displacement|") -> Path:
    png = Path(out_dir) / f"{name}.png"
    return peridem.snapshot(peridem.last_vtu(Path(out_dir) / "out"), png, color=color, title=name)
