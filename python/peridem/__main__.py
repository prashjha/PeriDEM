# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Run a PeriDEM JSON deck in-process.

    python -m peridem -i examples/Peridynamics/circle/input.json -nThreads 4
    mpirun -n 2 python -m peridem -i input.json -nThreads 4

Does not change Time_Steps, Horizon, or mesh size.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


def _ensure_import() -> None:
    try:
        import peridem  # noqa: F401

        return
    except ImportError:
        pass
    here = Path(__file__).resolve()
    root = here.parents[2] if here.parent.name == "peridem" else here.parents[1]
    for cand in (root / "build/linux/python", root / "build/python"):
        if (cand / "peridem").is_dir():
            sys.path.insert(0, str(cand))
            break
    sys.path.insert(0, str(root / "python"))
    import peridem  # noqa: F401


def _abs_meshes(deck: dict[str, Any], src_dir: Path) -> None:
    mesh = deck.get("Mesh") or {}
    for key, block in mesh.items():
        if not key.startswith("Set_") or not isinstance(block, dict):
            continue
        f = block.get("File")
        if not f:
            continue
        p = Path(f)
        candidates = [p, src_dir / "meshes" / p.name, src_dir / p.name]
        if not p.is_absolute():
            candidates.insert(0, src_dir / p)
        for cand in candidates:
            if cand.is_file():
                block["File"] = str(cand.resolve())
                break


def stage_deck(src: Path, out_dir: Path) -> Path:
    """Write a runnable copy: mesh paths resolved, Output.Path → out_dir.

    Physics keys (Time_Steps, Horizon, mesh size) are left unchanged.
    """
    src = src.resolve()
    out_dir = out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    deck = json.loads(src.read_text())
    _abs_meshes(deck, src.parent)
    mesh = deck.get("Mesh") or {}
    for key, block in mesh.items():
        if not key.startswith("Set_") or not isinstance(block, dict):
            continue
        cm = block.get("CreateMesh")
        if isinstance(cm, dict) and cm.get("Flag"):
            name = Path(block.get("File") or f"{key}.msh").name
            block["File"] = str(out_dir.parent / name)
    deck.setdefault("Output", {})
    deck["Output"]["Path"] = str(out_dir) + "/"
    dest = out_dir.parent / "python_input.json"
    dest.write_text(json.dumps(deck, indent=2) + "\n")
    return dest


def _already_under_mpi() -> bool:
    return any(
        k in os.environ
        for k in (
            "OMPI_COMM_WORLD_SIZE",
            "PMIX_RANK",
            "PMI_RANK",
            "MPI_LOCALNRANKS",
        )
    )


def run_deck(deck: Path, n_threads: int) -> None:
    _ensure_import()
    import peridem

    peridem.init(n_threads=n_threads)
    sim = peridem.Simulation.from_file(str(deck))
    sim.run()
    if peridem.mpi_rank() == 0:
        out = Path(json.loads(deck.read_text())["Output"]["Path"])
        print(f"done n={sim.n_nodes} step={sim.step_index} t={sim.time}", flush=True)
        print(f"VTU/PVD under {out}", flush=True)


def example_script(
    here: Path,
    *,
    default_deck: str,
    default_np: int,
    default_nthreads: int,
) -> int:
    """Entry used by examples/*/run.py. Honours DECK, NP, NTHREADS, CLEAN."""
    here = here.resolve()
    deck_name = os.environ.get("DECK", default_deck)
    np = int(os.environ.get("NP", str(default_np)))
    nthreads = int(os.environ.get("NTHREADS", str(default_nthreads)))
    src = here / deck_name
    if not src.is_file():
        raise SystemExit(f"missing deck {src}")
    out_dir = here / "runs" / "out"
    if os.environ.get("CLEAN", "0") == "1" and out_dir.exists():
        shutil.rmtree(out_dir)
    staged = stage_deck(src, out_dir)

    if np > 1 and not _already_under_mpi():
        mpiexec = os.environ.get("MPIEXEC") or shutil.which("mpirun") or "mpirun"
        cmd = [
            mpiexec,
            "-n",
            str(np),
            sys.executable,
            "-m",
            "peridem",
            "-i",
            str(staged),
            "-nThreads",
            str(nthreads),
        ]
        print(f"MPIEXEC={mpiexec} NP={np} DECK={deck_name} nThreads={nthreads}", flush=True)
        return subprocess.call(cmd, env=os.environ.copy())

    print(f"DECK={deck_name} NP={np} nThreads={nthreads}", flush=True)
    run_deck(staged, nthreads)
    return 0


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Run a PeriDEM JSON deck in-process")
    p.add_argument("-i", "--input", required=True, help="input JSON")
    p.add_argument("-nThreads", type=int, default=int(os.environ.get("NTHREADS", "4")))
    args = p.parse_args(argv)
    src = Path(args.input)
    if not src.is_file():
        raise SystemExit(f"missing deck {src}")
    # Direct -i: run that file as-is (no restage) so a staged python_input.json works.
    run_deck(src.resolve(), args.nThreads)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
