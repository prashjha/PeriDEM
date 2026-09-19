#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Hollow ellipse dropped onto a short tip-up triangle, set up in Python.

``main.cpp`` in this folder builds the same deck in C++, with the same
geometry, materials and contact parameters.
``python/tests/test_example_parity.py`` compares the two decks key by key, so
that the comparison of the two runs is a comparison of the interfaces and not
of two problems.

Contact at the tip opens a crack. The ring separates into two pieces.
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

# --- geometry ---------------------------------------------------------------
# A narrow tip concentrates the contact load.
W = 0.0008
H = 0.0012

# A thin ring, so that a crack started at the tip separates it.
A_OUT, B_OUT = 0.0020, 0.0014
A_IN, B_IN = 0.00180, 0.00122
ELL_THETA = 0.0
TIP_GAP = 0.00008

# --- material ---------------------------------------------------------------
RHO_T, K_T, NU_T, GC_T = 1200.0, 2.16e7, 0.25, 200.0
# Gc on the ellipse is low, so that damage at the tip runs across it.
RHO_E, K_E, NU_E, GC_E = 1200.0, 2.16e7, 0.25, 1.0

R_CONTACT_FACTOR = 0.95
GRAVITY = 10.0
IC_VY = -2.5

MESH_SIZE = 0.00010
HORIZON = 3.0 * MESH_SIZE
FINAL_TIME = 0.0030
NUM_STEPS = 30000

MESH_TRI = "mesh_triangle.msh"
MESH_ELL = "mesh_hollow_ellipse.msh"


def build_deck(output_path: str | os.PathLike[str] = "runs/",
               *, final_time: float = FINAL_TIME,
               num_steps: int = NUM_STEPS,
               mesh_size: float = MESH_SIZE,
               horizon: float | None = None,
               mesh_dir: str | os.PathLike[str] | None = None,
               mesh_files: tuple[str, str] | None = None,
               write_mesh: bool = True,
               in_process_mesh: bool = False) -> Deck:
    """Assemble the deck.

    ``in_process_mesh`` keeps Gmsh output in memory and writes no ``.msh``,
    which is what the README demo uses. The parity runs instead point both the
    Python and the C++ side at the same ``.msh`` files under ``mesh_dir``.
    """
    horizon = 3.0 * mesh_size if horizon is None else horizon

    E_t = to_E(K_T, NU_T)
    G_t = to_G(E=E_t, nu=NU_T)
    E_e = to_E(K_E, NU_E)
    G_e = to_G(E=E_e, nu=NU_E)

    d = Deck(dim=2, t_final=final_time, n_steps=num_steps,
             bond_break="tension",
             # Without this, self-contact across broken bonds closes the
             # crack.
             self_contact="none")

    dt_out_n = max(1, num_steps // 40)
    d.set_output(output_path,
                 tags=["Displacement", "Velocity", "Force", "Damage_Z",
                       "Damage", "Particle_ID", "Fixity"],
                 interval=dt_out_n, debug=1, pvd_collection=True)

    d.set_gravity(0.0, -GRAVITY)
    d.set_neighbor("simple_all", s_factor=8.0, update_interval=5,
                   near_bd_nodes_tol=0.5)

    # --- particle types
    tri = Geometry("triangle", [-0.5 * W, 0.0, 0.0,
                                0.5 * W, 0.0, 0.0,
                                0.0, H, 0.0])
    ell = Geometry("ellipse_minus_ellipse",
                   [A_OUT, B_OUT, A_IN, B_IN, ELL_THETA, 0.0, 0.0, 0.0])

    if in_process_mesh:
        tri_mesh = MeshSpec(size=mesh_size)
        ell_mesh = MeshSpec(size=mesh_size)
    else:
        base = Path(mesh_dir) if mesh_dir is not None else HERE / "inp"
        names = mesh_files or (MESH_TRI, MESH_ELL)
        tri_mesh = MeshSpec(file=base / names[0], size=mesh_size,
                            write=write_mesh)
        ell_mesh = MeshSpec(file=base / names[1], size=mesh_size,
                            write=write_mesh)

    g_tri = d.add_particle_type(tri, tri_mesh)
    g_ell = d.add_particle_type(ell, ell_mesh)

    # --- materials
    m_tri = d.add_material(horizon=horizon, density=RHO_T, K=K_T, G=G_t,
                           Gc=GC_T, influence_fn_type=1)
    m_ell = d.add_material(horizon=horizon, density=RHO_E, K=K_E, G=G_e,
                           Gc=GC_E, influence_fn_type=1)

    # --- contact: group 0 is the triangle tip (wall), group 1 the ellipse
    d.add_contact_pair(0, 0, contact_radius_factor=R_CONTACT_FACTOR,
                       Kn=contact_stiffness(K_T, K_T, horizon), eps=0.95,
                       beta_n_factor=100.0, K=K_T)
    # Beta_n at the tip is large enough to start a crack, and damping is
    # on to limit the node velocities after contact.
    d.add_contact_pair(0, 1, contact_radius_factor=0.90,
                       Kn=contact_stiffness(K_T, K_E, horizon), eps=0.4,
                       beta_n_factor=8.0,
                       K=peridem.harmonic_mean(K_T, K_E))
    d.add_contact_pair(1, 1, contact_radius_factor=R_CONTACT_FACTOR,
                       Kn=contact_stiffness(K_E, K_E, horizon), eps=0.95,
                       beta_n_factor=100.0, K=K_E)
    d.set_contact_laws(damping_law="com_and_node",
                       friction_law="coulomb_simple")

    # --- boundary and initial conditions
    # The triangle is fixed in x and y at every node.
    d.add_displacement_bc(particles=[0], direction=[1, 2],
                          zero_displacement=True)
    d.add_initial_velocity([0.0, IC_VY, 0.0], particles=[1])

    # The triangle is placed at its centroid and the ellipse above the tip.
    tip_y = H
    ell_cy = tip_y + TIP_GAP + B_OUT
    cx, cy, cz = tri.center
    d.place(cx, cy, 0.0, geometry=g_tri, material=m_tri, contact=0, wall=True)
    d.place(0.0, ell_cy, 0.0, geometry=g_ell, material=m_ell, contact=1)

    return d


def main(argv: list[str] | None = None) -> int:
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("-o", "--output", default=str(HERE / "runs_py"))
    p.add_argument("--final-time", type=float, default=FINAL_TIME)
    p.add_argument("--num-steps", type=int, default=NUM_STEPS)
    p.add_argument("--mesh-size", type=float, default=MESH_SIZE)
    p.add_argument("--nthreads", type=int,
                   default=int(os.environ.get("NTHREADS", "4")))
    p.add_argument("--in-process-mesh", action="store_true",
                   help="generate the meshes with Gmsh in memory, write no .msh")
    p.add_argument("--write-deck", metavar="PATH",
                   help="write the assembled JSON deck and exit")
    args = p.parse_args(argv)

    out = Path(args.output).resolve()
    d = build_deck(str(out) + "/", final_time=args.final_time,
                   num_steps=args.num_steps, mesh_size=args.mesh_size,
                   in_process_mesh=args.in_process_mesh,
                   mesh_dir=out.parent / "inp_py")
    if args.write_deck:
        print(d.write(args.write_deck))
        return 0

    out.mkdir(parents=True, exist_ok=True)
    (out.parent / "inp_py").mkdir(parents=True, exist_ok=True)
    peridem.init(n_threads=args.nthreads)
    sim = d.run(workdir=HERE)
    if peridem.mpi_rank() == 0:
        print(f"done: nodes={sim.n_nodes} step={sim.step_index} t={sim.time}")
        for p_ in sim.particles:
            print(f"  {p_!r} com={[round(c, 6) for c in p_.center_of_mass]}")
        print(f"VTU under {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
