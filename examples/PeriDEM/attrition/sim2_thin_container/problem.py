#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Attrition sim2: thin rotating container with an off-centre spin axis.

Circles, triangles, drums and hexagons in two sizes inside a thin-walled
cylinder with an inward bar. The container spins twice as fast as sim1 and
about a point offset from the origin, which throws the pack against the bar.

The deck is assembled through :class:`peridem.Deck`. Every parameter whose
default changes the result is set here. ``INPUT_DEFAULTS.md`` in this folder
records which those are.
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

R_SMALL, R_LARGE = 0.001, 0.003
MESH_SIZE = R_SMALL / 5.0
HORIZON = 2.0 * MESH_SIZE
R_IN = 0.02
R_OUT = R_IN + 1.5 * MESH_SIZE      # thin wall
L_BAR = 0.005                       # inward protrusion length
W_BAR = 1.5 * MESH_SIZE
DENSITY = 1200.0
GRAVITY = 10.0

OMEGA = -40.0 * math.pi
ROT_CENTER = (-0.2 * R_IN, 0.2 * R_IN, 0.0)

R_REF = {0: R_SMALL, 1: R_SMALL, 2: R_SMALL, 3: R_SMALL,
         4: R_LARGE, 5: R_LARGE, 6: R_LARGE, 7: R_LARGE}

MESH_FILES = [
    "mesh_cir_small_0.msh",
    "mesh_tri_small_0.msh",
    "mesh_drum2d_small_0.msh",
    "mesh_hex_small_0.msh",
    "mesh_cir_large_0.msh",
    "mesh_tri_large_0.msh",
    "mesh_drum2d_large_0.msh",
    "mesh_hex_large_0.msh",
    "mesh_wall_0.msh",
]

WALL_PARAMS = [R_OUT, 0.0, 0.0, 0.0,
               R_IN, 0.0, 0.0, 0.0,
               R_IN - L_BAR, -0.5 * W_BAR, 0.0, R_IN, 0.5 * W_BAR, 0.0]

KN = {
    (0, 0): 5.595291e21,
    (0, 1): 1.017326e22,
    (0, 2): 1.017326e22,
    (1, 1): 5.595291e22,
    (1, 2): 5.595291e22,
    (2, 2): 5.595291e22,
}
K_MAT = {0: 1.0e4, 1: 1.0e5, 2: 1.0e5}

KN_FACTOR = 1.0
BETA_N_FACTOR = 100.0
EPSILON = 0.95
CONTACT_RADIUS_FACTOR = 0.95
FRICTION_COEFF = 0.5
SEARCH_INTERVAL = 40
SEARCH_FACTOR = 10.0

GRAIN_SHAPES = [
    ("circle", [R_SMALL, 0.0, 0.0, 0.0]),
    ("triangle", [R_SMALL, 0.0, 0.0, 0.0]),
    ("drum2d", [R_SMALL, R_SMALL * 0.4, 0.0, 0.0, 0.0]),
    ("hexagon", [R_SMALL, 0.0, 0.0, 0.0]),
    ("circle", [R_LARGE, 0.0, 0.0, 0.0]),
    ("triangle", [R_LARGE, 0.0, 0.0, 0.0]),
    ("drum2d", [R_LARGE, R_LARGE * 0.4, 0.0, 0.0, 0.0]),
    ("hexagon", [R_LARGE, 0.0, 0.0, 0.0]),
]

PRESETS = {
    "short": (0.01, 100_000, 2000),
    "medium": (0.03, 300_000, 3000),
    "paper": (0.1, 1_000_000, 2500),
}

TAGS = ["Displacement", "Velocity", "Force", "Damage_Z", "Damage",
        "Particle_ID", "Fixity", "Contact_Nodes"]


def read_sites(csv: str | os.PathLike[str] = CSV) -> list[dict[str, float]]:
    rows: list[dict[str, float]] = []
    with Path(csv).open() as f:
        next(f)
        for line in f:
            parts = [p.strip() for p in line.split(",")]
            if len(parts) < 6:
                continue
            zone = int(float(parts[0]))
            x, y, z, r, theta = (float(v) for v in parts[1:6])
            rows.append({"zone": zone, "x": x, "y": y, "z": z, "r": r,
                         "theta": theta})
    validate_sites(rows)
    return rows


def validate_sites(rows: list[dict[str, float]]) -> None:
    """Reject a pack that starts inside the wall, the bar, or another grain."""
    bar = (R_IN - L_BAR, -0.5 * W_BAR, R_IN, 0.5 * W_BAR)
    errors: list[str] = []
    for i, a in enumerate(rows):
        if a["zone"] not in R_REF:
            errors.append(f"row {i}: unknown shape id {a['zone']}")
            continue
        if math.hypot(a["x"], a["y"]) + a["r"] > R_IN - 1.0e-9:
            errors.append(f"row {i}: reaches past R_in")
        if not (a["x"] + a["r"] < bar[0] or a["x"] - a["r"] > bar[2]
                or a["y"] + a["r"] < bar[1] or a["y"] - a["r"] > bar[3]):
            errors.append(f"row {i}: overlaps the protrusion")
    for i in range(len(rows)):
        for j in range(i + 1, len(rows)):
            a, b = rows[i], rows[j]
            if math.hypot(a["x"] - b["x"], a["y"] - b["y"]) < \
                    a["r"] + b["r"] - 1.0e-9:
                errors.append(f"rows {i},{j} overlap")
    if errors:
        raise ValueError("initial pack invalid:\n  " + "\n  ".join(errors[:40]))


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
             bond_break="tension",
             # sim2 keeps broken-bond self-contact inside each grain.
             self_contact="broken_bond_kn", wall_contact="meshed")
    d.set_output(output_path, tags=TAGS, interval=output_interval, debug=3,
                 perform_fe_out=False,
                 dt_test_out=max(1, output_interval // 100), tag_pp="0",
                 pvd_collection=True)
    d.set_gravity(0.0, -GRAVITY)
    d.set_neighbor("simple_all", s_factor=SEARCH_FACTOR,
                   update_interval=SEARCH_INTERVAL, near_bd_nodes_tol=0.5)

    base = Path(mesh_dir)
    for i, (name, params) in enumerate(GRAIN_SHAPES):
        d.add_particle_type(Geometry(name, params),
                            MeshSpec(file=(base / MESH_FILES[i]).resolve()))
    wall = Geometry("complex", WALL_PARAMS,
                    vec_type=["circle", "circle", "rectangle"],
                    vec_flag=["plus", "minus", "plus"])
    g_wall = d.add_particle_type(
        wall, MeshSpec(file=(base / MESH_FILES[8]).resolve()))

    m_small = d.add_material(horizon=HORIZON, density=DENSITY, K=1.0e4, G=6.0e3,
                             Gc=50.0, influence_fn_type=1)
    m_large = d.add_material(horizon=HORIZON, density=DENSITY, K=1.0e5, G=6.0e4,
                             Gc=100.0, influence_fn_type=1)

    for (i, j), kn in KN.items():
        d.add_contact_pair(
            i, j, contact_radius_factor=CONTACT_RADIUS_FACTOR, Kn=kn,
            K=peridem.harmonic_mean(K_MAT[i], K_MAT[j]),
            damping_on=False, friction_on=False, eps=EPSILON,
            mu=FRICTION_COEFF, Kn_factor=KN_FACTOR,
            raw={"Beta_n_Factor": BETA_N_FACTOR})
    d.set_contact_laws(damping_law="off", friction_law="coulomb_simple",
                       correct_volume=False)

    sites = read_sites(csv)
    for s in sites:
        zone = int(s["zone"])
        large = zone >= 4
        d.place(s["x"], s["y"], s["z"], geometry=zone,
                material=m_large if large else m_small,
                contact=1 if large else 0,
                theta=s["theta"], scale=s["r"] / R_REF[zone])

    wcx, wcy, wcz = wall.center
    wall_id = len(sites)
    d.place(wcx, wcy, wcz, geometry=g_wall, material=m_large, contact=2,
            wall=True)

    d.add_displacement_bc(particles=[wall_id], direction=[1, 2],
                          time_fn_type="rotation",
                          time_fn_params=[omega, *ROT_CENTER],
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
                               title="attrition sim2: damage"))
    print(f"VTU under {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
