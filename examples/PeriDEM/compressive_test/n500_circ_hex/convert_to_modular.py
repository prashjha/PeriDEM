#!/usr/bin/env python3
"""Convert restored n500_circ_hex init_config (YAML/CSV/msh) to modular JSON decks.

Reads particle_locations_0.csv + paper meshes under init_config/inp/.
Writes modular/input_{smoke,short,paper}.json with absolute mesh paths.
"""
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

R_REF = 1e-3
HORIZON = 6e-4
RHO, K, G, GC = 1200.0, 2.16e7, 1.296e7, 50.0
KN = 1.591549e24
FIXED_RECT = [-8.2e-4, -8.2e-4, 0.0, 6.932e-2, 4.182e-2, 0.0]
MOVING_RECT = [-2.2e-4, 4.122e-2, 0.0, 6.872e-2, 4.182e-2, 0.0]


def rect_center(r):
    return [(r[0] + r[3]) / 2, (r[1] + r[4]) / 2]


def build(root: Path, final_t: float, steps: int, out_dir: Path, tag: str,
          out_interval: int | None = None, test_interval: int | None = None) -> dict:
    inp = root / "init_config" / "inp"
    mod = root / "modular"
    mod.mkdir(parents=True, exist_ok=True)
    for name in (
        "mesh_particle_1_0.msh",
        "mesh_particle_2_0.msh",
        "mesh_rigid_wall_0.msh",
        "mesh_moving_wall_0.msh",
    ):
        src, dst = inp / name, mod / name
        if not dst.exists():
            dst.write_bytes(src.read_bytes())

    grains = []
    with open(inp / "particle_locations_0.csv") as f:
        next(f)
        for row in csv.reader(f):
            if not row:
                continue
            zone = int(row[0])
            x, y, z, r, o = map(float, row[1:6])
            grains.append((zone, x, y, z, r, o))
    n_pack = len(grains)
    n_fixed, n_moving = n_pack, n_pack + 1
    n_total = n_pack + 2
    cfix, cmov = rect_center(FIXED_RECT), rect_center(MOVING_RECT)

    def mesh(name: str) -> str:
        return name  # relative; run from modular/

    contact = {
        "Contact_Radius_Factor": 0.95,
        "Kn": KN,
        "Damping_On": True,
        "Epsilon": 0.95,
        "Friction_On": False,
        "Friction_Coeff": 0.5,
        "Kn_Factor": 1.0,
        "Beta_n_Factor": 100.0,
    }
    mat = {
        "Type": "PDState",
        "Horizon": HORIZON,
        "Density": RHO,
        "Compute_From_Classical": True,
        "K": K,
        "G": G,
        "Gc": GC,
        "Influence_Function": {"Type": 1},
    }
    data = {"N": n_total}
    for i, (zone, x, y, z, r, o) in enumerate(grains):
        data[str(i)] = {
            "x": x,
            "y": y,
            "z": z,
            "theta": o,
            "s": r / R_REF,
            "geom_id": zone,
            "mat_id": 0,
            "contact_id": 0,
        }
    data[str(n_fixed)] = {
        "x": cfix[0],
        "y": cfix[1],
        "z": 0.0,
        "theta": 0.0,
        "s": 1.0,
        "geom_id": 2,
        "mat_id": 1,
        "contact_id": 1,
        "is_wall": True,
    }
    data[str(n_moving)] = {
        "x": cmov[0],
        "y": cmov[1],
        "z": 0.0,
        "theta": 0.0,
        "s": 1.0,
        "geom_id": 3,
        "mat_id": 1,
        "contact_id": 1,
        "is_wall": True,
    }

    if out_interval is None:
        out_interval = max(1, steps // 40)
    if test_interval is None:
        test_interval = max(1, out_interval // 10)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir.name + "/"

    return {
        "Comment": f"Jha JMPS 2021 n500_circ_hex ({tag}); 502 mixed circle/hex; paper M1",
        "Model": {
            "Dimension": 2,
            "Final_Time": final_t,
            "Time_Steps": steps,
            "Discretization_Type": {
                "Spatial": "finite_difference",
                "Time": "central_difference",
            },
            "Populate_ElementNodeConnectivity": True,
            "Quad_Approximation_Order": 2,
            "Particle_Sim_Type": "Multi_Particle",
            "Seed": 0,
        },
        "Output": {
            "Path": out_path,
            "Perform_Out": True,
            "Tags": [
                "Displacement",
                "Velocity",
                "Force",
                "Damage_Z",
                "Damage",
                "Particle_ID",
                "Contact_Nodes",
            ],
            "Output_Interval": out_interval,
            "Debug": 1,
            "Perform_FE_Out": True,
            "Compress_Type": "zlib",
            "File_Format": "vtu",
            "Test_Output_Interval": test_interval,
            "Tag_PP": "0",
            "PVD_Collection": True,
        },
        "Force_BC": {"Gravity": [0.0, -10.0, 0.0]},
        "Displacement_BC": {
            "Sets": 2,
            "Set_1": {
                "Particle_List": [n_fixed],
                "Direction": [1, 2],
                "Zero_Displacement": True,
            },
            "Set_2": {
                "Particle_List": [n_moving],
                "Direction": [2],
                "Time_Function": {"Type": "linear", "Parameters": [-0.06]},
                "Spatial_Function": {"Type": "constant"},
            },
        },
        "Particle": {
            "Sets": 4,
            "Set_1": {"Type": "circle", "Parameters": [R_REF, 0.0, 0.0, 0.0]},
            "Set_2": {"Type": "hexagon", "Parameters": [R_REF, 0.0, 0.0, 0.0]},
            "Set_3": {"Type": "rectangle", "Parameters": FIXED_RECT},
            "Set_4": {"Type": "rectangle", "Parameters": MOVING_RECT},
        },
        "Mesh": {
            "Sets": 4,
            "Set_1": {"File": mesh("mesh_particle_1_0.msh")},
            "Set_2": {"File": mesh("mesh_particle_2_0.msh")},
            "Set_3": {"File": mesh("mesh_rigid_wall_0.msh")},
            "Set_4": {"File": mesh("mesh_moving_wall_0.msh")},
        },
        "Material": {"Sets": 2, "Set_1": mat, "Set_2": mat},
        "Contact": {
            "Sets": 2,
            "Set_1_1": contact,
            "Set_1_2": contact,
            "Set_2_2": contact,
        },
        "Neighbor": {
            "Update_Criteria": "simple_all",
            "Search_Factor": 5.0,
            "Search_Interval": 40,
            "Near_Bd_Nodes_Tol": 0.5,
        },
        "Particle_Generation": {
            "Method": "From_File",
            "Random_Rotation": False,
            "Data": data,
        },
        "Test": {
            "Test_Name": "compressive_test",
            "Compressive_Test": {
                "Wall_Id": n_moving,
                "Wall_Force_Direction": 2,
            },
        },
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parent,
    )
    args = ap.parse_args()
    root = args.root
    mod = root / "modular"
    decks = [
        ("input_smoke.json", 2e-4, 2000, mod / "smoke_out", "smoke_2k", 500, 50),
        ("input_short.json", 0.003, 30000, mod / "short_out", "short_30k", 1500, 15),
        ("input_paper.json", 0.06, 600000, mod / "paper_out", "paper_600k", 1500, 15),
    ]
    for name, T, n, outd, tag, oi, ti in decks:
        j = build(root, T, n, outd, tag, oi, ti)
        (mod / name).write_text(json.dumps(j, indent=2))
        print("wrote", mod / name, "N=", j["Particle_Generation"]["Data"]["N"])


if __name__ == "__main__":
    main()
