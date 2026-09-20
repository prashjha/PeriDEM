#!/usr/bin/env python3
"""Build modular JSON for attrition sim2 — explicit keys, match main GIF deck.

Reference:
  examples/PeriDEM/attrition_tests/sim2_multi_particle_circ_tri_drum_hex_with_rotating_cylinder_with_protrusion_thin_container_and_change_rotation_rate

Every field that has a dangerous modular default is set explicitly (see
INPUT_DEFAULTS.md). No reliance on omitted-key defaults.
"""

from __future__ import annotations

import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
MESH = HERE / "meshes"
CSV = HERE / "particle_locations_0.csv"

R_SMALL = 0.001
R_LARGE = 0.003
MESH_SIZE = R_SMALL / 5.0
HORIZON = 2.0 * MESH_SIZE
R_IN = 0.02
R_OUT = R_IN + 1.5 * MESH_SIZE
L_BAR = 0.005
W_BAR = 1.5 * MESH_SIZE
R_REF = {
    0: R_SMALL,
    1: R_SMALL,
    2: R_SMALL,
    3: R_SMALL,
    4: R_LARGE,
    5: R_LARGE,
    6: R_LARGE,
    7: R_LARGE,
}

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

WALL_PARAMS = [
    R_OUT,
    0.0,
    0.0,
    0.0,
    R_IN,
    0.0,
    0.0,
    0.0,
    R_IN - L_BAR,
    -0.5 * W_BAR,
    0.0,
    R_IN,
    0.5 * W_BAR,
    0.0,
]

KN = {
    (0, 0): 5.595291e21,
    (0, 1): 1.017326e22,
    (0, 2): 1.017326e22,
    (1, 1): 5.595291e22,
    (1, 2): 5.595291e22,
    (2, 2): 5.595291e22,
}
K_MAT = {0: 1.0e4, 1: 1.0e5, 2: 1.0e5}

# Explicit — never omit (modular defaults differ from main)
KN_FACTOR = 1.0
BETA_N_FACTOR = 100.0
EPSILON = 0.95
CONTACT_RADIUS_FACTOR = 0.95
FRICTION_COEFF = 0.5
DAMPING_ON = False
FRICTION_ON = False
DAMPING_LAW = "off"
FRICTION_LAW = "coulomb_simple"
CORRECT_VOLUME = False  # main DEM contact used full Vj
SEARCH_INTERVAL = 40
SEARCH_FACTOR = 10.0
NEAR_BD_TOL = 0.5
BOND_BREAK = "tension"
# Main applies broken-bond self-contact inside PD; modular name:
SELF_CONTACT = "broken_bond_kn"
WALL_CONTACT = "meshed"
RANDOM_ROTATION = False
OMEGA = -40.0 * math.pi
ROT_CENTER = (-0.2 * R_IN, 0.2 * R_IN, 0.0)


def wall_geom_center(params: list[float]) -> tuple[float, float, float]:
    r_out, cx0, cy0, cz0 = params[0], params[1], params[2], params[3]
    r_in, cx1, cy1, cz1 = params[4], params[5], params[6], params[7]
    x0, y0, z0, x1, y1, z1 = params[8:14]
    a_out = math.pi * r_out * r_out
    a_in = math.pi * r_in * r_in
    a_rect = abs(x1 - x0) * abs(y1 - y0)
    vol = a_out - a_in + a_rect
    cx = (a_out * cx0 - a_in * cx1 + a_rect * 0.5 * (x0 + x1)) / vol
    cy = (a_out * cy0 - a_in * cy1 + a_rect * 0.5 * (y0 + y1)) / vol
    cz = (a_out * cz0 - a_in * cz1 + a_rect * 0.5 * (z0 + z1)) / vol
    return cx, cy, cz


def contact_pair(i: int, j: int) -> dict:
    a, b = min(i, j), max(i, j)
    return {
        "Contact_Radius_Factor": CONTACT_RADIUS_FACTOR,
        "Damping_On": DAMPING_ON,
        "Friction_On": FRICTION_ON,
        "Kn": KN[(a, b)],
        "K": 2.0 * K_MAT[a] * K_MAT[b] / (K_MAT[a] + K_MAT[b]),
        "Epsilon": EPSILON,
        "Friction_Coeff": FRICTION_COEFF,
        "Kn_Factor": KN_FACTOR,
        "Beta_n_Factor": BETA_N_FACTOR,
    }


def material(horizon: float, K: float, G: float, Gc: float) -> dict:
    return {
        "Type": "PDState",
        "Horizon": horizon,
        "Density": 1200.0,
        "Compute_From_Classical": True,
        "Is_Plain_Strain": False,
        "K": K,
        "G": G,
        "Gc": Gc,
        "Influence_Function": {"Type": 1},
    }


def validate_ic(rows: list[tuple]) -> None:
    bar = (R_IN - L_BAR, -0.5 * W_BAR, R_IN, 0.5 * W_BAR)
    errors: list[str] = []
    for i, (zi, x, y, z, r, o) in enumerate(rows):
        if zi not in R_REF:
            errors.append(f"row {i}: bad zone {zi}")
            continue
        if math.hypot(x, y) + r > R_IN - 1.0e-9:
            errors.append(
                f"row {i} zone={zi}: past R_in (hyp+r={math.hypot(x, y) + r:.6e})"
            )
        if not (x + r < bar[0] or x - r > bar[2] or y + r < bar[1] or y - r > bar[3]):
            errors.append(f"row {i} zone={zi}: overlaps protrusion AABB")
    for i in range(len(rows)):
        for j in range(i + 1, len(rows)):
            dx = rows[i][1] - rows[j][1]
            dy = rows[i][2] - rows[j][2]
            if math.hypot(dx, dy) < rows[i][4] + rows[j][4] - 1.0e-9:
                errors.append(f"overlap rows {i},{j}")
    if errors:
        raise SystemExit("IC invalid:\n  " + "\n  ".join(errors[:40]))


def build(
    final_time: float,
    time_steps: int,
    out_path: str,
    output_interval: int,
) -> dict:
    particles = []
    rows = []
    with CSV.open() as f:
        next(f)
        for line in f:
            parts = [p.strip() for p in line.split(",")]
            if len(parts) < 6:
                continue
            zi = int(float(parts[0]))
            x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
            r, o = float(parts[4]), float(parts[5])
            rows.append((zi, x, y, z, r, o))
            large = zi >= 4
            particles.append(
                {
                    "x": x,
                    "y": y,
                    "z": z,
                    "theta": o,
                    "s": r / R_REF[zi],
                    "geom_id": zi,
                    "mat_id": 1 if large else 0,
                    "contact_id": 1 if large else 0,
                }
            )

    validate_ic(rows)

    wall_id = len(particles)
    wcx, wcy, wcz = wall_geom_center(WALL_PARAMS)
    particles.append(
        {
            "x": wcx,
            "y": wcy,
            "z": wcz,
            "theta": 0.0,
            "s": 1.0,
            "geom_id": 8,
            "mat_id": 1,
            "contact_id": 2,
            "is_wall": True,  # no PD on wall; DOFs locked by Displacement_BC
        }
    )

    gen = {
        "Method": "From_File",
        "Random_Rotation": RANDOM_ROTATION,
        "Data": {"N": len(particles)},
    }
    for i, p in enumerate(particles):
        gen["Data"][str(i)] = p

    particle_geom = {
        "Sets": 9,
        "Set_1": {"Type": "circle", "Parameters": [R_SMALL, 0.0, 0.0, 0.0]},
        "Set_2": {"Type": "triangle", "Parameters": [R_SMALL, 0.0, 0.0, 0.0]},
        "Set_3": {
            "Type": "drum2d",
            "Parameters": [R_SMALL, R_SMALL * 0.4, 0.0, 0.0, 0.0],
        },
        "Set_4": {"Type": "hexagon", "Parameters": [R_SMALL, 0.0, 0.0, 0.0]},
        "Set_5": {"Type": "circle", "Parameters": [R_LARGE, 0.0, 0.0, 0.0]},
        "Set_6": {"Type": "triangle", "Parameters": [R_LARGE, 0.0, 0.0, 0.0]},
        "Set_7": {
            "Type": "drum2d",
            "Parameters": [R_LARGE, R_LARGE * 0.4, 0.0, 0.0, 0.0],
        },
        "Set_8": {"Type": "hexagon", "Parameters": [R_LARGE, 0.0, 0.0, 0.0]},
        "Set_9": {
            "Type": "complex",
            "Vec_type": ["circle", "circle", "rectangle"],
            "Vec_flag": ["plus", "minus", "plus"],
            "Parameters": WALL_PARAMS,
        },
    }

    mesh = {"Sets": 9}
    for i, name in enumerate(MESH_FILES):
        mesh[f"Set_{i + 1}"] = {"File": str((MESH / name).resolve())}

    contact = {
        "Sets": 3,
        "Damping_Law": DAMPING_LAW,
        "Friction_Law": FRICTION_LAW,
        "Correct_Volume": CORRECT_VOLUME,
    }
    for i in range(3):
        for j in range(i, 3):
            contact[f"Set_{i + 1}_{j + 1}"] = contact_pair(i, j)

    return {
        "Model": {
            "Dimension": 2,
            "Final_Time": final_time,
            "Time_Steps": time_steps,
            "Discretization_Type": {
                "Spatial": "finite_difference",
                "Time": "central_difference",
            },
            "Populate_ElementNodeConnectivity": True,
            "Quad_Approximation_Order": 2,
            "Particle_Sim_Type": "Multi_Particle",
            "MPI_Strategy": "auto",
            "Seed": 0,
            "Bond_Break": BOND_BREAK,
            "Self_Contact": SELF_CONTACT,
            "Wall_Contact": WALL_CONTACT,
        },
        "Output": {
            "Path": out_path if out_path.endswith("/") else out_path + "/",
            "Perform_Out": True,
            "Tags": [
                "Displacement",
                "Velocity",
                "Force",
                "Damage_Z",
                "Damage",
                "Particle_ID",
                "Fixity",
                "Contact_Nodes",
            ],
            "Output_Interval": output_interval,
            "Debug": 3,
            "Perform_FE_Out": False,
            "Compress_Type": "zlib",
            "File_Format": "vtu",
            "Test_Output_Interval": max(1, output_interval // 100),
            "Tag_PP": "0",
            "PVD_Collection": True,
        },
        "Force_BC": {"Gravity": [0.0, -10.0, 0.0]},
        "Displacement_BC": {
            "Sets": 1,
            "Set_1": {
                "Particle_List": [wall_id],
                "Direction": [1, 2],
                "Time_Function": {
                    "Type": "rotation",
                    "Parameters": [OMEGA, ROT_CENTER[0], ROT_CENTER[1], ROT_CENTER[2]],
                },
                "Spatial_Function": {"Type": "rotation"},
                "Zero_Displacement": False,
            },
        },
        "Particle": particle_geom,
        "Mesh": mesh,
        "Material": {
            "Sets": 2,
            "Set_1": material(HORIZON, 1.0e4, 6.0e3, 50.0),
            "Set_2": material(HORIZON, 1.0e5, 6.0e4, 100.0),
        },
        "Contact": contact,
        "Neighbor": {
            "Update_Criteria": "simple_all",
            "Search_Factor": SEARCH_FACTOR,
            "Search_Interval": SEARCH_INTERVAL,
            "Near_Bd_Nodes_Tol": NEAR_BD_TOL,
        },
        "Particle_Generation": gen,
    }


def main() -> None:
    for mesh in MESH_FILES:
        if not (MESH / mesh).is_file():
            raise SystemExit(f"missing mesh {MESH / mesh}")
    if not CSV.is_file():
        raise SystemExit(f"missing {CSV}")

    short = build(0.01, 100000, "runs/out", 2000)
    medium = build(0.03, 300000, "runs/out", 3000)
    paper = build(0.1, 1000000, "runs/out", 2500)
    (HERE / "input_short.json").write_text(json.dumps(short, indent=2) + "\n")
    (HERE / "input_medium.json").write_text(json.dumps(medium, indent=2) + "\n")
    (HERE / "input.json").write_text(json.dumps(paper, indent=2) + "\n")
    n = short["Particle_Generation"]["Data"]["N"]
    wid = short["Displacement_BC"]["Set_1"]["Particle_List"][0]
    print(
        f"wrote decks N={n} wall_id={wid} Self_Contact={SELF_CONTACT} "
        f"DampLaw={DAMPING_LAW} DampOn={DAMPING_ON} SearchInt={SEARCH_INTERVAL} "
        f"KnF={KN_FACTOR} BetaN={BETA_N_FACTOR} Bond={BOND_BREAK}"
    )
    print("Wall DOFs via Displacement_BC; see INPUT_DEFAULTS.md for contact gaps.")


if __name__ == "__main__":
    main()
