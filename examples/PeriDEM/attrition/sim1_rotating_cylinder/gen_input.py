#!/usr/bin/env python3
"""Build modular JSON for attrition sim1 — match main GIF deck.

Reference (main README links attrition_test_sim1.gif here):
  examples/PeriDEM/attrition_tests/sim1_multi_particle_circ_tri_drum_with_rotating_cylinder_with_protrusion

Physics from that problem_setup.py: Kn_Factor=1, Gc=50/100, Bond_Break unset
(tension default). Wall site = ComplexGeom centroid so mesh at origin is not shifted.
"""

from __future__ import annotations

import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
MESH = HERE / "meshes"
CSV = HERE / "particle_locations_0.csv"

R_REF = {0: 0.001, 1: 0.001, 2: 0.001, 3: 0.003, 4: 0.003, 5: 0.003}
MESH_FILES = [
    "mesh_cir_small_0.msh",
    "mesh_tri_small_0.msh",
    "mesh_drum2d_small_0.msh",
    "mesh_cir_large_0.msh",
    "mesh_tri_large_0.msh",
    "mesh_drum2d_large_0.msh",
    "mesh_wall_0.msh",
]

# outer circle, inner circle (minus), protrusion rectangle (plus) — same as yaml
WALL_PARAMS = [
    0.021,
    0.0,
    0.0,
    0.0,
    0.02,
    0.0,
    0.0,
    0.0,
    0.014,
    -0.0015,
    0.0,
    0.02,
    0.0015,
    0.0,
]

# Kn from sim1 (h=6e-4, K_small=1e4, K_large=K_wall=1e5)
KN = {
    (0, 0): 7.368284e20,
    (0, 1): 1.339688e21,
    (0, 2): 1.339688e21,
    (1, 1): 7.368284e21,
    (1, 2): 7.368284e21,
    (2, 2): 7.368284e21,
}

K_MAT = {0: 1.0e4, 1: 1.0e5, 2: 1.0e5}
KN_FACTOR = 1.0  # GIF / main sim1 (not increased_Kn_factor)
R_IN = 0.02


def wall_geom_center(params: list[float]) -> tuple[float, float, float]:
    """Signed-volume centroid of circle(+)-circle(-)-rectangle(+) wall."""
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
        "Contact_Radius_Factor": 0.95,
        "Damping_On": False,
        "Friction_On": False,
        "Kn": KN[(a, b)],
        "K": 2.0 * K_MAT[a] * K_MAT[b] / (K_MAT[a] + K_MAT[b]),
        "Epsilon": 0.95,
        "Friction_Coeff": 0.5,
        "Kn_Factor": KN_FACTOR,
        "Beta_n_Factor": 100.0,
    }


def material(horizon: float, K: float, G: float, Gc: float) -> dict:
    return {
        "Type": "PDState",
        "Horizon": horizon,
        "Density": 1200.0,
        "Compute_From_Classical": True,
        "K": K,
        "G": G,
        "Gc": Gc,
        "Influence_Function": {"Type": 1},
    }


def build(
    final_time: float,
    time_steps: int,
    out_path: str,
    output_interval: int,
    omega: float = -20.0 * math.pi,
) -> dict:
    horizon = 6.0e-4
    particles = []
    with CSV.open() as f:
        next(f)
        for line in f:
            parts = [p.strip() for p in line.split(",")]
            if len(parts) < 6:
                continue
            zi = int(float(parts[0]))
            x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
            r, o = float(parts[4]), float(parts[5])
            large = zi >= 3
            if math.hypot(x, y) + r > R_IN - 1.0e-6:
                raise SystemExit(
                    f"IC overlap: particle zone={zi} at ({x},{y}) r={r} "
                    f"extends past R_in={R_IN}"
                )
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

    wall_id = len(particles)
    # MUST equal ComplexGeom centroid so mesh (axis at origin) is not shifted.
    wcx, wcy, wcz = wall_geom_center(WALL_PARAMS)
    particles.append(
        {
            "x": wcx,
            "y": wcy,
            "z": wcz,
            "theta": 0.0,
            "s": 1.0,
            "geom_id": 6,
            "mat_id": 1,
            "contact_id": 2,
            "is_wall": True,
        }
    )

    gen = {"Method": "From_File", "Random_Rotation": False, "Data": {"N": len(particles)}}
    for i, p in enumerate(particles):
        gen["Data"][str(i)] = p

    particle_geom = {
        "Sets": 7,
        "Set_1": {"Type": "circle", "Parameters": [0.001, 0.0, 0.0, 0.0]},
        "Set_2": {"Type": "triangle", "Parameters": [0.001, 0.0, 0.0, 0.0]},
        "Set_3": {"Type": "drum2d", "Parameters": [0.001, 0.0004, 0.0, 0.0, 0.0]},
        "Set_4": {"Type": "circle", "Parameters": [0.003, 0.0, 0.0, 0.0]},
        "Set_5": {"Type": "triangle", "Parameters": [0.003, 0.0, 0.0, 0.0]},
        "Set_6": {"Type": "drum2d", "Parameters": [0.003, 0.0012, 0.0, 0.0, 0.0]},
        "Set_7": {
            "Type": "complex",
            "Vec_type": ["circle", "circle", "rectangle"],
            "Vec_flag": ["plus", "minus", "plus"],
            "Parameters": WALL_PARAMS,
        },
    }

    mesh = {"Sets": 7}
    for i, name in enumerate(MESH_FILES):
        mesh[f"Set_{i + 1}"] = {"File": str((MESH / name).resolve())}

    contact = {
        "Sets": 3,
        "Damping_Law": "off",
        "Friction_Law": "coulomb_simple",
        "Correct_Volume": False,  # main DEM contact used full Vj
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
            "Seed": 0,
            # main yaml omitted Bond_Break → tension (PMB default)
            "Bond_Break": "tension",
            "Self_Contact": "none",
            "Wall_Contact": "meshed",
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
            "Debug": 1,
            "Perform_FE_Out": False,
            "Compress_Type": "zlib",
            "File_Format": "vtu",
            "Test_Output_Interval": max(1, output_interval // 10),
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
                    "Parameters": [omega, 0.0, 0.0, 0.0],
                },
                "Spatial_Function": {"Type": "rotation"},
                "Zero_Displacement": False,
            },
        },
        "Particle": particle_geom,
        "Mesh": mesh,
        "Material": {
            "Sets": 2,
            # paper Gc: small=50, large/wall=100
            "Set_1": material(horizon, 1.0e4, 6.0e3, 50.0),
            "Set_2": material(horizon, 1.0e5, 6.0e4, 100.0),
        },
        "Contact": contact,
        "Neighbor": {
            "Update_Criteria": "simple_all",
            "Search_Factor": 10.0,
            "Search_Interval": 40,
            "Near_Bd_Nodes_Tol": 0.5,
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
    wp = short["Particle_Generation"]["Data"][str(wid)]
    print(
        f"wrote input_short/medium/json (N={n}, wall_id={wid}, KnF={KN_FACTOR}, "
        f"Gc=50/100, Bond_Break=tension, Damp=off, "
        f"wall_site=({wp['x']:.6f},{wp['y']:.6f}))"
    )


if __name__ == "__main__":
    main()
