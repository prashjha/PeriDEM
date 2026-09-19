#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Attrition sim1: a rotating drum with an inward protrusion.

Circles, triangles and drums in two sizes tumble inside a cylinder that is
spun by a rotation displacement BC; the protrusion grinds them. This is the
deck behind ``attrition_test_sim1.gif`` in the top-level README.

The deck is assembled through :class:`peridem.Deck`. The wall is a complex
geometry, an outer circle with an inner circle removed and a protrusion
rectangle added. It is placed at its signed-volume centroid, computed by the
geometry object. The mesh axis is at the origin, so any other site translates
the mesh.

``gen_input.py`` next to this file writes the same decks as JSON for
``bin/PeriDEM``. ``python/tests/test_example_parity.py`` compares the two.
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
from peridem.deck import MeshSpec  # noqa: E402

MESH_DIR = HERE / "meshes"
CSV = HERE / "particle_locations_0.csv"

HORIZON = 6.0e-4
DENSITY = 1200.0
R_IN = 0.02                 # inner radius of the drum
R_SMALL, R_LARGE = 0.001, 0.003
OMEGA = -20.0 * math.pi     # drum spin rate, rad/s
GRAVITY = 10.0

# Reference radius of each of the six grain shapes (3 small, 3 large).
R_REF = {0: R_SMALL, 1: R_SMALL, 2: R_SMALL, 3: R_LARGE, 4: R_LARGE, 5: R_LARGE}

MESH_FILES = [
    "mesh_cir_small_0.msh",
    "mesh_tri_small_0.msh",
    "mesh_drum2d_small_0.msh",
    "mesh_cir_large_0.msh",
    "mesh_tri_large_0.msh",
    "mesh_drum2d_large_0.msh",
    "mesh_wall_0.msh",
]

# outer circle (+), inner circle (-), protrusion rectangle (+)
WALL_PARAMS = [0.021, 0.0, 0.0, 0.0,
               0.02, 0.0, 0.0, 0.0,
               0.014, -0.0015, 0.0, 0.02, 0.0015, 0.0]

# Normal contact stiffness per pair of contact groups, from the sim1
# calibration. These are not recomputed from K_MAT and HORIZON.
KN = {
    (0, 0): 7.368284e20,
    (0, 1): 1.339688e21,
    (0, 2): 1.339688e21,
    (1, 1): 7.368284e21,
    (1, 2): 7.368284e21,
    (2, 2): 7.368284e21,
}
K_MAT = {0: 1.0e4, 1: 1.0e5, 2: 1.0e5}
KN_FACTOR = 1.0

# Small circle, triangle and drum, then the same three shapes larger.
GRAIN_SHAPES = [
    ("circle", [R_SMALL, 0.0, 0.0, 0.0]),
    ("triangle", [R_SMALL, 0.0, 0.0, 0.0]),
    ("drum2d", [R_SMALL, 0.0004, 0.0, 0.0, 0.0]),
    ("circle", [R_LARGE, 0.0, 0.0, 0.0]),
    ("triangle", [R_LARGE, 0.0, 0.0, 0.0]),
    ("drum2d", [R_LARGE, 0.0012, 0.0, 0.0, 0.0]),
]

PRESETS = {
    "short": (0.01, 100_000, 2000),
    "medium": (0.03, 300_000, 3000),
    "paper": (0.1, 1_000_000, 2500),
}

TAGS = ["Displacement", "Velocity", "Force", "Damage_Z", "Damage",
        "Particle_ID", "Fixity", "Contact_Nodes"]


def read_sites(csv: str | os.PathLike[str] = CSV) -> list[dict[str, float]]:
    """Packed grain sites: ``zone, x, y, z, r, theta`` per row."""
    sites: list[dict[str, float]] = []
    with Path(csv).open() as f:
        next(f)
        for line in f:
            parts = [p.strip() for p in line.split(",")]
            if len(parts) < 6:
                continue
            zone = int(float(parts[0]))
            x, y, z, r, theta = (float(v) for v in parts[1:6])
            if math.hypot(x, y) + r > R_IN - 1.0e-6:
                raise ValueError(
                    f"initial pack overlaps the drum: zone={zone} at "
                    f"({x}, {y}) r={r} reaches past R_in={R_IN}")
            sites.append({"zone": zone, "x": x, "y": y, "z": z, "r": r,
                          "theta": theta})
    return sites


def build_deck(output_path: str | os.PathLike[str] = "runs/out/", *,
               preset: str = "short",
               final_time: float | None = None,
               num_steps: int | None = None,
               output_interval: int | None = None,
               omega: float = OMEGA,
               mesh_dir: str | os.PathLike[str] = MESH_DIR,
               csv: str | os.PathLike[str] = CSV) -> Deck:
    t_default, n_default, out_default = PRESETS[preset]
    final_time = t_default if final_time is None else final_time
    num_steps = n_default if num_steps is None else num_steps
    output_interval = out_default if output_interval is None else output_interval

    d = Deck(dim=2, t_final=final_time, n_steps=num_steps,
             bond_break="tension", self_contact="none", wall_contact="meshed")
    d.set_output(output_path, tags=TAGS, interval=output_interval, debug=1,
                 perform_fe_out=False,
                 dt_test_out=max(1, output_interval // 10), tag_pp="0",
                 pvd_collection=True)
    d.set_gravity(0.0, -GRAVITY)
    d.set_neighbor("simple_all", s_factor=10.0, update_interval=40,
                   near_bd_nodes_tol=0.5)

    base = Path(mesh_dir)
    for i, (name, params) in enumerate(GRAIN_SHAPES):
        d.add_particle_type(Geometry(name, params),
                            MeshSpec(file=(base / MESH_FILES[i]).resolve()))
    wall = Geometry("complex", WALL_PARAMS,
                    vec_type=["circle", "circle", "rectangle"],
                    vec_flag=["plus", "minus", "plus"])
    g_wall = d.add_particle_type(wall,
                                 MeshSpec(file=(base / MESH_FILES[6]).resolve()))

    # Material 0 is the small grains, material 1 the large grains and drum.
    m_small = d.add_material(horizon=HORIZON, density=DENSITY, K=1.0e4, G=6.0e3,
                             Gc=50.0, influence_fn_type=1)
    m_large = d.add_material(horizon=HORIZON, density=DENSITY, K=1.0e5, G=6.0e4,
                             Gc=100.0, influence_fn_type=1)

    # Contact groups: 0 small grains, 1 large grains, 2 the drum.
    for (i, j), kn in KN.items():
        d.add_contact_pair(
            i, j, contact_radius_factor=0.95, Kn=kn,
            K=peridem.harmonic_mean(K_MAT[i], K_MAT[j]),
            damping_on=False, friction_on=False, eps=0.95, mu=0.5,
            Kn_factor=KN_FACTOR,
            # Damping is off, so Beta_n_Factor does not enter the force.
            raw={"Beta_n_Factor": 100.0})
    d.set_contact_laws(damping_law="off", friction_law="coulomb_simple",
                       correct_volume=False)

    sites = read_sites(csv)
    for s in sites:
        zone = int(s["zone"])
        large = zone >= 3
        d.place(s["x"], s["y"], s["z"], geometry=zone,
                material=m_large if large else m_small,
                contact=1 if large else 0,
                theta=s["theta"], scale=s["r"] / R_REF[zone])

    wcx, wcy, wcz = wall.center
    wall_id = len(sites)
    d.place(wcx, wcy, wcz, geometry=g_wall, material=m_large, contact=2,
            wall=True)

    # The rotation centre is the last three time function parameters.
    d.add_displacement_bc(particles=[wall_id], direction=[1, 2],
                          time_fn_type="rotation",
                          time_fn_params=[omega, 0.0, 0.0, 0.0],
                          spatial_fn_type="rotation")
    return d


def main(argv: list[str] | None = None) -> int:
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("-o", "--output", default=str(HERE / "runs_py/out"))
    p.add_argument("--preset", choices=sorted(PRESETS), default="short")
    p.add_argument("--final-time", type=float, default=None)
    p.add_argument("--num-steps", type=int, default=None)
    p.add_argument("--output-interval", type=int, default=None)
    p.add_argument("--nthreads", type=int,
                   default=int(os.environ.get("NTHREADS", "4")))
    p.add_argument("--snapshot", metavar="PNG", default=None)
    p.add_argument("--write-deck", metavar="PATH", default=None)
    args = p.parse_args(argv)

    out = Path(args.output).resolve()
    d = build_deck(str(out) + "/", preset=args.preset,
                   final_time=args.final_time, num_steps=args.num_steps,
                   output_interval=args.output_interval)
    if args.write_deck:
        print(d.write(args.write_deck))
        return 0

    out.mkdir(parents=True, exist_ok=True)
    peridem.init(n_threads=args.nthreads)
    sim = d.run(workdir=HERE)
    if peridem.mpi_rank() != 0:
        return 0
    print(f"done: grains={sim.n_particles} walls={sim.n_walls} "
          f"nodes={sim.n_nodes} t={sim.time:g}")
    worst = max(sim.particles, key=lambda q: float(q.damage.max()), default=None)
    if worst is not None:
        print(f"most damaged grain: id={worst.id} "
              f"max Z={float(worst.damage.max()):.4f} "
              f"shape={worst.geometry_name}")
    if args.snapshot:
        print(peridem.snapshot(peridem.last_vtu(out), args.snapshot,
                               color="Damage_Z",
                               title="attrition sim1: damage"))
    print(f"VTU under {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
