# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Run an existing PeriDEM JSON deck in-process.

    python -m peridem -i examples/Peridynamics/circle/input.json -nThreads 4
    mpirun -n 2 python -m peridem -i input.json -nThreads 4

This performs the same run as ``bin/PeriDEM -i``, in the calling process. The
deck is used as written, including Time_Steps, Horizon and the mesh size.

To build a problem in Python instead of reading a deck, see
:class:`peridem.Deck` and the ``problem.py`` in each example folder.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

__all__ = ["main", "run_deck", "stage_deck"]


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
    """Resolve relative Mesh.Set_*.File entries against the deck's folder."""
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
    """Write a runnable copy of ``src``: mesh paths resolved, output redirected.

    Physics keys (Time_Steps, Horizon, mesh size) are left untouched. Returns
    the path of the staged deck.
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


def run_deck(deck: Path, n_threads: int) -> None:
    _ensure_import()
    import peridem

    peridem.init(n_threads=n_threads)
    sim = peridem.Simulation.from_file(str(deck))
    sim.run()
    if peridem.mpi_rank() == 0:
        out = Path(json.loads(deck.read_text())["Output"]["Path"])
        print(f"done n={sim.n_nodes} step={sim.step_index} t={sim.time}",
              flush=True)
        print(f"VTU/PVD under {out}", flush=True)


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("-i", "--input", required=True, help="input JSON deck")
    p.add_argument("-nThreads", type=int,
                   default=int(os.environ.get("NTHREADS", "4")))
    args = p.parse_args(argv)
    src = Path(args.input)
    if not src.is_file():
        raise SystemExit(f"missing deck {src}")
    run_deck(src.resolve(), args.nThreads)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
