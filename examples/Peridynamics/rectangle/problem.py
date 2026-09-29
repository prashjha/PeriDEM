#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Single rectangular particle pulled diagonally.

The south-west corner is fixed and the north-east corner is pulled at a
constant rate. The deck is built in Python and the mesh is generated in the
calling process, so no ``.msh`` file is read or written.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
for _p in HERE.parents:
    if (_p / "examples" / "peridem_env.py").is_file():
        sys.path.insert(0, str(_p / "examples"))
        break
import peridem_env  # noqa: F401,E402

import peridem  # noqa: E402
from peridem import Deck, Geometry  # noqa: E402
from peridem.deck import MeshSpec, to_G  # noqa: E402

SIDE = 0.01
MESH_SIZE = 0.0002
HORIZON = 0.00032
DENSITY = 1200.0
K = 216000.0
NU = 0.25
GC = 500.0

FINAL_TIME = 0.01
NUM_STEPS = 20000
PULL_RATE = 0.01

TAGS = ["Displacement", "Velocity", "Force", "Force_Density", "Damage_Z",
        "Damage", "Nodal_Volume", "Zone_ID", "Particle_ID", "Fixity",
        "Force_Fixity", "Theta"]


def build_deck(output_path: str | os.PathLike[str] = "runs/", *,
               final_time: float = FINAL_TIME, num_steps: int = NUM_STEPS,
               output_interval: int = 2000, mesh_size: float = MESH_SIZE,
               horizon: float = HORIZON,
               tags: list[str] | None = None) -> Deck:
    d = Deck(dim=2, t_final=final_time, n_steps=num_steps,
             particle_sim_type="Single_Particle")
    d.set_comment("Single-particle rectangle: uniform in-process mesh; fixed "
                  "SW corner, linear pull on NE, set up in Python")
    d.set_output(output_path, tags=tags or TAGS, interval=output_interval,
                 debug=1, tag_pp="1", pvd_collection=True)

    # The uniform mesh is generated in memory and no file is read.
    d.add_particle_type(Geometry("rectangle", [0.0, 0.0, 0.0, SIDE, SIDE, 0.0]),
                        MeshSpec(size=mesh_size, info="uniform"))
    d.add_material(horizon=horizon, density=DENSITY, K=K,
                   G=to_G(E=peridem.to_E(K, NU), nu=NU), Gc=GC,
                   influence_fn_type=1)

    corner = 0.2 * SIDE
    hold = Geometry("rectangle", [0.0, 0.0, 0.0, corner, corner, 0.0])
    pull = Geometry("rectangle",
                    [SIDE - corner, SIDE - corner, 0.0, SIDE, SIDE, 0.0])
    d.add_displacement_bc(region=hold, direction=[1, 2],
                          time_fn_type="constant", time_fn_params=[0.0],
                          spatial_fn_type="constant", zero_displacement=True)
    d.add_displacement_bc(region=pull, direction=[1, 2],
                          time_fn_type="linear", time_fn_params=[PULL_RATE],
                          spatial_fn_type="constant")

    d.set_test("test_peridynamics")
    return d


def main(argv: list[str] | None = None) -> int:
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("-o", "--output", default=str(HERE / "runs_py"))
    p.add_argument("--final-time", type=float, default=FINAL_TIME)
    p.add_argument("--num-steps", type=int, default=NUM_STEPS)
    p.add_argument("--output-interval", type=int, default=2000)
    p.add_argument("--mesh-size", type=float, default=MESH_SIZE)
    p.add_argument("--nthreads", type=int,
                   default=int(os.environ.get("NTHREADS", "4")))
    p.add_argument("--snapshot", metavar="PNG", default=None)
    p.add_argument("--write-deck", metavar="PATH", default=None)
    args = p.parse_args(argv)

    out = Path(args.output).resolve()
    d = build_deck(str(out) + "/", final_time=args.final_time,
                   num_steps=args.num_steps,
                   output_interval=args.output_interval,
                   mesh_size=args.mesh_size)
    if args.write_deck:
        print(d.write(args.write_deck))
        return 0

    out.mkdir(parents=True, exist_ok=True)
    peridem.init(n_threads=args.nthreads)
    sim = d.run(workdir=HERE)
    if peridem.mpi_rank() != 0:
        return 0
    u = sim.displacement
    print(f"done: nodes={sim.n_nodes} step={sim.step_index} t={sim.time:g}")
    print(f"max |u| = {float(abs(u).max()):.6e}")
    if args.snapshot:
        print(peridem.snapshot(peridem.last_vtu(out), args.snapshot,
                               color="|Displacement|",
                               title="rectangle: |u| at final time"))
    print(f"VTU under {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
