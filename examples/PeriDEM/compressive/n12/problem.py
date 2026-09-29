#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Compression of a small circular-grain pack, set up in Python.

Parameters from Jha et al., J. Mech. Phys. Solids 151 (2021) 104376, section
4.3: material M1, lc = R/5, horizon = 3 lc, Rc = 0.95 h, C-bar = 100 and plate
velocity -0.06 m/s. The pack here is 4 by 3 grains and not the 502 of the
paper. The grain positions, the open channel that contains them and the moving
plate are derived from R and the pack size, and no deck file is read.

``test/test_data/peridem/jha2021_comp_n50/main.cpp`` builds the same deck in
C++. ``python/tests/test_example_parity.py`` compares the two decks key by
key.
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
from peridem.deck import MeshSpec, contact_stiffness, to_E, to_G  # noqa: E402

R = 0.001
MESH_SIZE = R / 5.0
DENSITY = 1200.0
K = 2.16e7
NU = 0.25
GC = 50.0
WALL_VY = -0.06
GRAVITY = 10.0

NCOLS, NROWS = 4, 3
FINAL_TIME = 0.004
NUM_STEPS = 20000
SEARCH_INTERVAL = 40

TAGS = ["Displacement", "Velocity", "Force", "Damage_Z", "Damage",
        "Particle_ID", "Contact_Nodes"]

MESH_FILES = ("mesh_cir.msh", "mesh_fixed_container.msh",
              "mesh_moving_container.msh")


def pack_geometry(ncols: int = NCOLS, nrows: int = NROWS, r: float = R,
                  mesh_size: float = MESH_SIZE) -> dict[str, object]:
    """Pack spacing and container dimensions, all derived from R and the mesh.

    The gap between grains is slightly larger than the contact radius, so that
    no pair is in contact at t = 0. Gravity and the plate bring pairs into
    contact.
    """
    horizon = 3.0 * mesh_size
    h_est = 0.7 * mesh_size          # realized hmin on a Gmsh disc is ~0.7 lc
    rc_est = 0.95 * h_est
    padding = 1.15 * rc_est
    rwp = horizon + padding
    wall_t = rwp - padding

    step = 2.0 * r + padding
    lin = 2.0 * padding + 2.0 * r + (ncols - 1) * step
    win = 2.0 * padding + 2.0 * r + (nrows - 1) * step

    sites = [(padding + r + i * step, padding + r + j * step)
             for j in range(nrows) for i in range(ncols)]
    return {
        "horizon": horizon, "padding": padding, "rwp": rwp, "wall_t": wall_t,
        "Lin": lin, "Win": win, "sites": sites,
        # The fixed open channel and the plate that compresses the pack.
        "cup": [-rwp, -rwp, lin + rwp, win + wall_t, wall_t, 0.0],
        "plate": [-padding, win, 0.0, lin + padding, win + wall_t, 0.0],
    }


def build_deck(output_path: str | os.PathLike[str] = "runs/", *,
               ncols: int = NCOLS, nrows: int = NROWS,
               final_time: float = FINAL_TIME, num_steps: int = NUM_STEPS,
               mesh_size: float = MESH_SIZE,
               search_interval: int = SEARCH_INTERVAL,
               mesh_dir: str | os.PathLike[str] | None = None,
               write_mesh: bool = True,
               in_process_mesh: bool = False) -> Deck:
    g = pack_geometry(ncols, nrows, R, mesh_size)
    horizon = float(g["horizon"])
    sites = g["sites"]
    n_pack = len(sites)

    E = to_E(K, NU)
    G = to_G(E=E, nu=NU)
    Kn = contact_stiffness(K, K, horizon)

    d = Deck(dim=2, t_final=final_time, n_steps=num_steps)
    d.set_comment("jha2021_comp_contact")

    dt_out = max(1, num_steps // 4)
    d.set_output(output_path, tags=TAGS, interval=dt_out, debug=1,
                 dt_test_out=max(1, dt_out // 10), tag_pp="0",
                 pvd_collection=True)
    d.set_gravity(0.0, -GRAVITY)
    d.set_neighbor(update_criteria="simple_all", s_factor=5.0, update_interval=search_interval,
                   near_bd_nodes_tol=0.5)

    grain = Geometry("circle", [R, 0.0, 0.0, 0.0])
    cup = Geometry("open_rect_channel_2d", list(g["cup"]))
    plate = Geometry("rectangle", list(g["plate"]))

    if in_process_mesh:
        meshes = [MeshSpec(size=mesh_size) for _ in range(3)]
    else:
        base = Path(mesh_dir) if mesh_dir is not None else HERE
        meshes = [MeshSpec(file=base / name, size=mesh_size, write=write_mesh)
                  for name in MESH_FILES]

    g_grain = d.add_particle_type(grain, meshes[0])
    g_cup = d.add_particle_type(cup, meshes[1])
    g_plate = d.add_particle_type(plate, meshes[2])

    m_grain = d.add_material(horizon=horizon, density=DENSITY, K=K, G=G, Gc=GC,
                             influence_fn_type=1)
    m_wall = d.add_material(horizon=horizon, density=DENSITY, K=K, G=G, Gc=GC,
                            influence_fn_type=1)

    for i, j in ((0, 0), (0, 1), (1, 1)):
        d.add_contact_pair(i, j, contact_radius_factor=0.95, Kn=Kn, eps=0.95,
                           beta_n_factor=100.0, K=K)

    # Particle ids: 0..n_pack-1 grains, then the cup, then the plate.
    id_cup, id_plate = n_pack, n_pack + 1
    d.add_displacement_bc(particles=[id_cup], direction=[1, 2],
                          zero_displacement=True)
    d.add_displacement_bc(particles=[id_plate], direction=[2],
                          time_fn_type="linear", time_fn_params=[WALL_VY],
                          spatial_fn_type="constant")

    for x, y in sites:
        d.place(x, y, geometry=g_grain, material=m_grain, contact=0)
    cx, cy, _ = cup.center
    d.place(cx, cy, geometry=g_cup, material=m_wall, contact=1, wall=True)
    px, py, _ = plate.center
    d.place(px, py, geometry=g_plate, material=m_wall, contact=1, wall=True)

    d.set_test("compressive_test", wall_id=id_plate, wall_force_direction=2)
    return d


def main(argv: list[str] | None = None) -> int:
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("-o", "--output", default=str(HERE / "runs_py"))
    p.add_argument("--ncols", type=int, default=NCOLS)
    p.add_argument("--nrows", type=int, default=NROWS)
    p.add_argument("--final-time", type=float, default=FINAL_TIME)
    p.add_argument("--num-steps", type=int, default=NUM_STEPS)
    p.add_argument("--mesh-size", type=float, default=MESH_SIZE)
    p.add_argument("--in-process-mesh", action="store_true",
                   help="build the three meshes with Gmsh in memory")
    p.add_argument("--nthreads", type=int,
                   default=int(os.environ.get("NTHREADS", "4")))
    p.add_argument("--snapshot", metavar="PNG", default=None)
    p.add_argument("--write-deck", metavar="PATH", default=None)
    args = p.parse_args(argv)

    out = Path(args.output).resolve()
    d = build_deck(str(out) + "/", ncols=args.ncols, nrows=args.nrows,
                   final_time=args.final_time, num_steps=args.num_steps,
                   mesh_size=args.mesh_size,
                   in_process_mesh=args.in_process_mesh,
                   mesh_dir=out.parent / "meshes_py")
    if args.write_deck:
        print(d.write(args.write_deck))
        return 0

    out.mkdir(parents=True, exist_ok=True)
    (out.parent / "meshes_py").mkdir(parents=True, exist_ok=True)
    peridem.init(n_threads=args.nthreads)
    sim = d.run(workdir=HERE)
    if peridem.mpi_rank() != 0:
        return 0
    print(f"done: grains={sim.n_particles} walls={sim.n_walls} "
          f"nodes={sim.n_nodes} t={sim.time:g}")
    ys = [p_.center_of_mass[1] for p_ in sim.particles]
    print(f"grain centroid y: min={min(ys):.6e} max={max(ys):.6e}")
    if args.snapshot:
        print(peridem.snapshot(peridem.last_vtu(out), args.snapshot,
                               color="|Displacement|",
                               title="n12 compression: |u|"))
    print(f"VTU under {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
