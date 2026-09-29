#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Two deformable circles: one fixed, one dropped onto it.

Two grains, one contact pair, gravity and an initial velocity. Both meshes are
built by Gmsh in the calling process and nothing is read from disk.

``test/test_data/peridem/twop_circ_inbuilt/main.cpp``, compiled with
TWOP_CONTACT_EXAMPLE as the target example_twop_circ_contact, builds the same
deck in C++.
"""

from __future__ import annotations

import math
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

R1 = R2 = 0.001
PARTICLE_DIST = 0.001       # drop height
DENSITY = 1200.0
K = 2.16e7
NU = 0.25
GC = 50.0

R_CONTACT_FACTOR = 0.95
EPSILON = 0.9
BETA_N_FACTOR = 100.0
FRICTION_COEFF = 0.5
GRAVITY = 10.0

FINAL_TIME = 0.012
NUM_STEPS = 36000

TAGS = ["Displacement", "Velocity", "Force", "Damage_Z", "Damage",
        "Particle_ID"]


def build_deck(output_path: str | os.PathLike[str] = "out/", *,
               final_time: float = FINAL_TIME, num_steps: int = NUM_STEPS,
               mesh_size: float | None = None, horizon: float | None = None,
               damping_on: bool = False, eps: float = EPSILON,
               zero_ic: bool = False,
               mesh_dir: str | os.PathLike[str] | None = None,
               write_mesh: bool = True,
               in_process_mesh: bool = False) -> Deck:
    mesh_size = min(R1, R2) / 5.0 if mesh_size is None else mesh_size
    horizon = 3.0 * mesh_size if horizon is None else horizon

    E = to_E(K, NU)
    G = to_G(E=E, nu=NU)

    d = Deck(dim=2, t_final=final_time, n_steps=num_steps)
    d.set_output(output_path, tags=TAGS, interval=num_steps // 10, debug=2,
                 pvd_collection=True)
    d.set_gravity(0.0, -GRAVITY)
    d.set_neighbor(update_criteria="simple_all", s_factor=10.0, update_interval=40,
                   near_bd_nodes_tol=0.5)

    if in_process_mesh:
        meshes = [MeshSpec(size=mesh_size), MeshSpec(size=mesh_size)]
    else:
        base = Path(mesh_dir) if mesh_dir is not None else HERE / "inp"
        meshes = [MeshSpec(file=base / f"mesh_cir_{i}.msh", size=mesh_size,
                           write=write_mesh) for i in (1, 2)]

    g1 = d.add_particle_type(Geometry("circle", [R1, 0.0, 0.0, 0.0]), meshes[0])
    g2 = d.add_particle_type(Geometry("circle", [R2, 0.0, 0.0, 0.0]), meshes[1])

    m1 = d.add_material(horizon=horizon, density=DENSITY, K=K, G=G, Gc=GC,
                        influence_fn_type=1)
    m2 = d.add_material(horizon=horizon, density=DENSITY, K=K, G=G, Gc=GC,
                        influence_fn_type=1)

    kn = contact_stiffness(K, K, horizon)
    for i, j in ((0, 0), (0, 1), (1, 1)):
        d.add_contact_pair(i, j, contact_radius_factor=R_CONTACT_FACTOR, Kn=kn,
                           eps=eps, mu=FRICTION_COEFF, damping_on=damping_on,
                           friction_on=False, beta_n_factor=BETA_N_FACTOR,
                           K=K)
    d.set_contact_laws(damping_law="com_and_node",
                       friction_law="coulomb_simple")

    # Particle 0 is fixed and particle 1 falls onto it.
    d.add_displacement_bc(particles=[0], direction=[1, 2],
                          zero_displacement=True)
    vy = 0.0
    if not zero_ic:
        fallen = PARTICLE_DIST - horizon
        if fallen > 0.0:
            vy = -math.sqrt(2.0 * GRAVITY * fallen)
    d.add_initial_velocity([0.0, vy, 0.0], particles=[1])

    top_gap = PARTICLE_DIST
    d.place(R1, R1, 0.0, geometry=g1, material=m1, contact=0)
    d.place(R1, 2.0 * R1 + R2 + top_gap, 0.0, geometry=g2, material=m2,
            contact=1, theta=math.pi)
    return d


def main(argv: list[str] | None = None) -> int:
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("-o", "--output", default=str(HERE / "runs_py"))
    p.add_argument("--final-time", type=float, default=FINAL_TIME)
    p.add_argument("--num-steps", type=int, default=NUM_STEPS)
    p.add_argument("--in-process-mesh", action="store_true",
                   help="build both discs with Gmsh in memory, write no .msh")
    p.add_argument("--nthreads", type=int,
                   default=int(os.environ.get("NTHREADS", "4")))
    p.add_argument("--snapshot", metavar="PNG", default=None)
    p.add_argument("--write-deck", metavar="PATH", default=None)
    args = p.parse_args(argv)

    out = Path(args.output).resolve()
    d = build_deck(str(out / "out") + "/", final_time=args.final_time,
                   num_steps=args.num_steps,
                   in_process_mesh=args.in_process_mesh,
                   mesh_dir=out / "inp")
    if args.write_deck:
        print(d.write(args.write_deck))
        return 0

    (out / "out").mkdir(parents=True, exist_ok=True)
    (out / "inp").mkdir(parents=True, exist_ok=True)
    peridem.init(n_threads=args.nthreads)
    sim = d.run(workdir=HERE)
    if peridem.mpi_rank() != 0:
        return 0

    fixed, falling = sim.particle(0), sim.particle(1)
    # Same measure as the C++ RestitutionProbe: centre-node separation minus
    # the two bounding radii.
    separation = math.dist(falling.x_center, fixed.x_center)
    gap = separation - fixed.bounding_radius - falling.bounding_radius
    print(f"done: nodes={sim.n_nodes} t={sim.time:g}")
    print(f"centre-to-centre gap at final time: {gap:.6e} m")
    print(f"max damage: {float(sim.damage.max()):.4f}")
    if args.snapshot:
        print(peridem.snapshot(peridem.last_vtu(out / "out"), args.snapshot,
                               color="|Velocity|",
                               title="two circles: |v| at final time"))
    print(f"VTU under {out / 'out'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
