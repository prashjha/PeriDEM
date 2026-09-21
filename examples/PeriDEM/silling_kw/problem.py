#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Silling 2003 Kalthoff-Winkler notched-plate impact, 2D and 3D, from Python.

A 200 x 100 mm maraging-steel plate with two open 1.5 mm notches is struck
edge-on by a rigid 1.57 kg cylinder at 32 m/s. Silling reports cracks running
from the notch tips at roughly 900 m/s and about 70 degrees to the notch.

This example uses the parts of the interface a pure JSON deck cannot reach:

* ``sim.setup()`` builds the model, then ``sim.break_bonds_in_slots(...)``
  seeds the pre-notch by cutting the peridynamic bonds that span each slot --
  the same operation the C++ driver does between ``init()`` and the time loop;
* ``sim.integrate()`` then runs the loop *without* re-initialising, or
  :func:`run_with_arrival_times` drives the loop step by step from Python to
  record when each node first becomes damaged, which gives the crack speed.

``Test_PeriDEM_notched_impact_inbuilt`` builds the same deck in C++.
``python/tests/test_example_parity.py`` compares the two decks key by key.
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

import numpy as np  # noqa: E402
import peridem  # noqa: E402
from peridem import Deck, Geometry  # noqa: E402
from peridem.deck import MeshSpec, contact_stiffness  # noqa: E402

# Silling Fig. 2 geometry.
W, H = 0.200, 0.100
NOTCH_DEPTH = 0.050
NOTCH_HALF = 0.025          # notch centrelines at x = +/- 25 mm
NOTCH_W = 0.0015            # 1.5 mm open gap
THICKNESS = 0.009           # 3D only

# Impactor: 1.57 kg cylinder, as wide as the 50 mm ligament.
IW, IH = 0.050, 0.100
IMPACT_V = 32.0
IMPACTOR_MASS = 1.57

MESH_SIZE = 0.001           # Silling's 200 x 100 x 9 grid
RC_FACTOR = 0.95

# Table 2 M1 = maraging steel; Gc from KIc ~ 90 MPa sqrt(m).
RHO = 8000.0
E = 191.0e9
K_BULK = 159.2e9
GC = 42408.0

DT = 2.5e-9
FINAL_TIME = 1.7e-4         # 170 us

TAGS = ["Displacement", "Velocity", "Force", "Damage", "Damage_Bond",
        "Damage_Z", "Particle_ID"]

# The --quick settings: the same problem on a smaller, softer plate.
QUICK = dict(W=0.040, H=0.020, notch_half=0.005, mesh_size=0.020 / 16.0,
             Iw=0.008, Ih=0.004, rho=1200.0, E=1.23e9, K_bulk=2.0e9, Gc=424.0,
             dt=1.0e-8, final_time=1.0e-4)


def _nu(E_: float, K_: float) -> float:
    return 0.5 * (1.0 - E_ / (3.0 * K_))


def notch_void_boxes(h: float, notch_half: float, notch_w: float,
                     notch_depth: float, z_lo: float,
                     z_hi: float) -> list[list[float]]:
    """The two notch slots as boxes, open at the top edge."""
    hw = 0.5 * notch_w
    y_tip = 0.5 * h - notch_depth
    y_hi = 0.5 * h + 1.0e-9
    return [[-notch_half - hw, y_tip, z_lo, -notch_half + hw, y_hi, z_hi],
            [notch_half - hw, y_tip, z_lo, notch_half + hw, y_hi, z_hi]]


def build_deck(output_path: str | os.PathLike[str] = "runs/out/", *,
               dim3: bool = False, quick: bool = False,
               final_time: float | None = None, dt: float | None = None,
               num_steps: int | None = None,
               mesh_dir: str | os.PathLike[str] | None = None,
               v_impact: float = IMPACT_V) -> Deck:
    cfg = dict(W=W, H=H, notch_half=NOTCH_HALF, mesh_size=MESH_SIZE, Iw=IW,
               Ih=IH, rho=RHO, E=E, K_bulk=K_BULK, Gc=GC, dt=DT,
               final_time=FINAL_TIME)
    if quick:
        cfg.update(QUICK)
        cfg["notch_depth"] = 0.5 * cfg["H"]
        cfg["notch_w"] = max(cfg["mesh_size"], 0.1 * cfg["notch_half"])
    else:
        cfg["notch_depth"] = NOTCH_DEPTH
        cfg["notch_w"] = NOTCH_W

    if final_time is not None:
        cfg["final_time"] = final_time
    if dt is not None:
        cfg["dt"] = dt
    n_steps = (int(round(cfg["final_time"] / cfg["dt"])) if num_steps is None
               else num_steps)

    w, h = cfg["W"], cfg["H"]
    mesh_size = cfg["mesh_size"]
    horizon = 3.0 * mesh_size
    notch_half, notch_w, notch_depth = (cfg["notch_half"], cfg["notch_w"],
                                        cfg["notch_depth"])
    iw, ih = cfg["Iw"], cfg["Ih"]
    gap = 1.5 * RC_FACTOR * mesh_size
    rho, e_mod, k_bulk, gc = cfg["rho"], cfg["E"], cfg["K_bulk"], cfg["Gc"]
    nu = _nu(e_mod, k_bulk)
    g_mod = e_mod / (2.0 * (1.0 + nu))

    d = Deck(dim=3 if dim3 else 2, t_final=cfg["final_time"], n_steps=n_steps,
             # Element-node connectivity is only used for strain output and
             # does not handle the hexahedra the 3D structured grid produces.
             populate_element_node_connectivity=not dim3,
             self_contact="none", bond_break="tension", wall_contact="meshed")

    d.set_output(output_path, tags=TAGS, interval=max(1, n_steps // 10),
                 debug=1, perform_fe_out=False, dt_test_out=n_steps,
                 pvd_collection=False)
    d.set_neighbor(update_criteria="simple_all", s_factor=5.0, update_interval=1,
                   near_bd_nodes_tol=0.5)

    # --- bodies
    if dim3:
        plate = Geometry("cuboid", [-0.5 * w, -0.5 * h, -0.5 * THICKNESS,
                                    0.5 * w, 0.5 * h, 0.5 * THICKNESS])
        # Cylinder axis along the impact direction, flat face striking the edge.
        impactor = Geometry("cylinder", [0.5 * iw, 0.0, -0.5 * ih, 0.0,
                                         0.0, ih, 0.0])
        z_lo, z_hi = -0.5 * THICKNESS - 1.0e-9, 0.5 * THICKNESS + 1.0e-9
    else:
        plate = Geometry("rectangle",
                         [-0.5 * w, -0.5 * h, 0.0, 0.5 * w, 0.5 * h, 0.0])
        impactor = Geometry("rectangle",
                            [-0.5 * iw, -0.5 * ih, 0.0, 0.5 * iw, 0.5 * ih, 0.0])
        z_lo, z_hi = -1.0e-9, 1.0e-9

    voids = notch_void_boxes(h, notch_half, notch_w, notch_depth, z_lo, z_hi)
    base = Path(mesh_dir) if mesh_dir is not None else HERE / "runs/inp"
    # The plate sits on Silling's equally spaced structured grid with the notch
    # slots carved out, so they are real gaps rather than cut material.
    g_plate = d.add_particle_type(
        plate, MeshSpec(file=base / "mesh_plate.msh", size=mesh_size,
                        info="uniform", voids=voids))
    # In 2D the impactor is a rectangle, so it goes on the same grid; in 3D it
    # is a cylinder, which a uniform grid cannot represent.
    g_imp = d.add_particle_type(
        impactor,
        MeshSpec(file=base / "mesh_impactor.msh", size=mesh_size,
                 info="gmsh_builtin_mesh" if dim3 else "uniform"))

    # The impactor has the plate material with Gc = 0. Its nodal forces are
    # replaced by the rigid-body acceleration, so its bonds carry no load.
    m_plate = d.add_material(material_type="PMBBond", horizon=horizon,
                             density=rho, K=k_bulk, G=g_mod, Gc=gc, E=e_mod,
                             influence_fn_type=0, influence_fn_params=[1.0])
    m_imp = d.add_material(material_type="PDElasticBond", horizon=horizon,
                           density=rho, K=k_bulk, G=g_mod, Gc=0.0, E=e_mod,
                           influence_fn_type=0, influence_fn_params=[1.0])

    # Contact force density is Kn * V_j * overlap, so Kn carries one power of
    # the horizon per spatial dimension of the nodal weight: 5 in 3D, 4 in 2D.
    kn = contact_stiffness(k_bulk, k_bulk, horizon,
                           horizon_power=5 if dim3 else 4)
    for i, j in ((0, 0), (0, 1), (1, 1)):
        d.add_contact_pair(i, j, contact_radius=RC_FACTOR * mesh_size,
                           Kn=kn, eps=1.0, damping_on=False, friction_on=False,
                           beta_n_factor=0.0, K=k_bulk)
    d.set_contact_laws(damping_law="off", friction_law="coulomb_simple")

    # The plate carries no load on its boundary. The displacement condition
    # constrains the impactor to move along y.
    d.add_displacement_bc(particles=[1], direction=[1],
                          time_fn_type="constant", time_fn_params=[0.0],
                          spatial_fn_type="constant", zero_displacement=True)
    d.add_initial_velocity([0.0, -v_impact, 0.0], particles=[1])

    # In two dimensions the mass is per unit thickness of the 9 mm plate.
    d.add_rigid_particle(1, IMPACTOR_MASS if dim3 else IMPACTOR_MASS / THICKNESS)

    d.place(0.0, 0.0, 0.0, geometry=g_plate, material=m_plate, contact=0)
    d.place(0.0, 0.5 * h + 0.5 * ih + gap, 0.0, geometry=g_imp,
            material=m_imp, contact=1)

    # Used by seed_prenotch and crack_speed below.
    d.extra_info = {"H": h, "notch_half": notch_half, "notch_w": notch_w,
                    "notch_depth": notch_depth, "mesh_size": mesh_size,
                    "horizon": horizon, "n_steps": n_steps,
                    "dt": cfg["dt"], "final_time": cfg["final_time"]}
    return d


def seed_prenotch(sim, info: dict[str, float]) -> int:
    """Cut the peridynamic bonds that span the two notch slots.

    The slots are already absent from the mesh, but the horizon is twice the
    slot width, so bonds still reach across them. Must run after ``setup()``.
    """
    y_top = 0.5 * info["H"]
    y_tip = y_top - info["notch_depth"]
    n = sim.break_bonds_in_slots(
        [-info["notch_half"], info["notch_half"]], info["notch_w"],
        y_tip, y_top + 0.01 * info["H"], 0)
    if n < 10:
        raise RuntimeError(f"expected pre-notch bonds to cut, got {n}")
    return n


def run_with_arrival_times(sim, info: dict[str, float], *,
                           threshold: float = 0.30) -> np.ndarray:
    """Drive the time loop from Python, recording first-damage time per node.

    The arrival-time field is what gives the crack speed Silling reports.
    Returns an array of length n_nodes, -1 where the node never damaged.
    """
    n_nodes = sim.n_nodes
    arrival = np.full(n_nodes, -1.0)
    plate = np.asarray(sim.particle_id) == 0
    sample_every = max(1, int(info["n_steps"]) // 400)

    def sample() -> None:
        fresh = (arrival < 0.0) & plate & (sim.damage >= threshold)
        arrival[fresh] = sim.time

    sim.apply_initial_condition()
    if sim.perform_output:
        sim.write_output()
    sim.set_current_dt(sim.dt)
    sim.apply_displacement_bc()
    sim.compute_forces()
    sim.apply_rigid_body_constraint()
    while sim.step_index < sim.n_steps:
        sim.step()
        if sim.should_output:
            sim.write_output()
        if sim.step_index % sample_every == 0:
            sample()
        sim.check_stop()
        if sim.stopped:
            break
    sample()
    return arrival


def crack_speed(sim, arrival: np.ndarray,
                info: dict[str, float]) -> tuple[float, int, float] | None:
    """Least-squares crack-tip speed from the arrival-time field.

    Fits distance-from-the-right-notch-tip against arrival time over damaged
    plate nodes below the tip (where the crack runs) and within half the plate
    height of it. Returns (m/s, points used, correlation).

    The returned correlation states how well the fit holds. If damage spreads
    through the plate instead of advancing as a front, the fit has no meaning
    and the correlation is low. The ``--quick`` settings produce that, on a
    plate of 40 by 20 mm with E = 1.23 GPa.
    """
    y_tip = 0.5 * info["H"] - info["notch_depth"]
    tip = np.array([info["notch_half"], y_tip])
    x = np.asarray(sim.reference)
    seen = (arrival >= 0.0) & (np.asarray(sim.particle_id) == 0)
    dist = np.linalg.norm(x[:, :2] - tip, axis=1)
    # The crack runs into the plate, below the notch tip.
    band = seen & (dist < 0.5 * info["H"]) & (x[:, 1] < y_tip)
    n = int(band.sum())
    if n < 20:
        return None
    t = arrival[band]
    r_dist = dist[band]
    if np.ptp(t) <= 0.0:
        return None
    slope, _ = np.polyfit(t, r_dist, 1)
    r = float(np.corrcoef(t, r_dist)[0, 1])
    return float(slope), n, r


def main(argv: list[str] | None = None) -> int:
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("-o", "--output", default=None)
    p.add_argument("--dim3", action="store_true", help="Silling's 3D plate")
    p.add_argument("--quick", action="store_true",
                   help="small, soft, short variant for a smoke run")
    p.add_argument("--final-time", type=float, default=None)
    p.add_argument("--num-steps", type=int, default=None)
    p.add_argument("--arrival-times", action="store_true",
                   help="drive the loop from Python and report crack speed")
    p.add_argument("--nthreads", type=int,
                   default=int(os.environ.get("NTHREADS", "8")))
    p.add_argument("--snapshot", metavar="PNG", default=None)
    p.add_argument("--write-deck", metavar="PATH", default=None)
    args = p.parse_args(argv)

    tag = "silling3d" if args.dim3 else ("quick" if args.quick else "silling2d")
    base = Path(args.output or (HERE / "runs_py" / tag)).resolve()
    out, inp = base / "out", base / "inp"

    d = build_deck(str(out) + "/", dim3=args.dim3, quick=args.quick,
                   final_time=args.final_time, num_steps=args.num_steps,
                   mesh_dir=inp)
    if args.write_deck:
        print(d.write(args.write_deck))
        return 0

    out.mkdir(parents=True, exist_ok=True)
    inp.mkdir(parents=True, exist_ok=True)
    info = d.extra_info

    peridem.init(n_threads=args.nthreads)
    sim = d.simulation(workdir=HERE)
    sim.setup()
    n_pre = seed_prenotch(sim, info)
    print(f"{tag}: nodes={sim.n_nodes} prenotch bonds cut={n_pre} "
          f"h={info['mesh_size']:.3e} horizon={info['horizon']:.3e} "
          f"Nt={info['n_steps']}")

    if args.arrival_times:
        arrival = run_with_arrival_times(sim, info)
        n_damaged = int((arrival >= 0.0).sum())
        print(f"damaged nodes: {n_damaged} of {sim.n_nodes}")
        fit = crack_speed(sim, arrival, info)
        if fit is None:
            print("crack speed: too few damaged nodes ahead of the tip to fit")
        else:
            v, n_fit, r = fit
            note = "" if abs(r) > 0.9 else "   (poor fit; damage is not " \
                                          "running as a front)"
            print(f"crack speed: {v:.1f} m/s  "
                  f"[{n_fit} nodes, r = {r:.2f}]{note}")
    else:
        sim.integrate()
    sim.close()

    print(f"final: step={sim.step_index} t={sim.time:g} "
          f"max damage={float(sim.damage.max()):.4f}")
    if args.snapshot:
        print(peridem.snapshot(peridem.last_vtu(out), args.snapshot,
                               color="Damage_Z", title=f"{tag}: damage"))
    print(f"VTU under {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
