#!/usr/bin/env python3
"""
Helpers for a 2D PeriDEM case with many particle types in a workspace.

This script writes JSON *fragments* you can merge into a full input deck produced
elsewhere (e.g. from apps/example-modular or test/twop_general_inbuilt patterns):

  - Five gmsh_builtin_mesh reference groups: circle, triangle, hexagon, rectangle, ellipse.
  - Twenty particle placements with geom_id cycling 0..4 (four particles per shape).


  - open_rect_channel_2d: U-channel domain (open +y) for a bulk reference body; see
    geom::OpenRectChannel2D and mesh_gen::physicalGroupsWallOpenFromY2D.
  - A separate *top platen* is usually an extra rectangle reference particle with
    displacement BC (downward); wire that in Model/BC decks like other examples.

Usage:
  python3 scripts/gen_peridem_container_demo.py [--out-dir path]
Writes: demo_particle_mesh_pgen.json (merge into your full input.json).
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", type=Path, default=Path("."), help="Directory for output JSON")
    args = ap.parse_args()

    s = 0.001
    mesh_size = s / 5.0
    z0 = 0.0

    geoms = [
        ("circle", [s, 0.0, 0.0, z0]),
        ("triangle", [s, 0.0, 0.0, z0, 1.0, 0.0, 0.0]),
        ("hexagon", [s, 0.0, 0.0, z0, 1.0, 0.0, 0.0]),
        ("rectangle", [-1.2 * s, -0.8 * s, z0, 1.2 * s, 0.8 * s, z0]),
        ("ellipse", [1.2 * s, 0.85 * s, 0.25, 0.0, 0.0, z0]),
    ]
    n_sets = len(geoms)
    particle_block: dict = {"Sets": n_sets}
    mesh_block: dict = {"Sets": n_sets}
    for i, (name, params) in enumerate(geoms):
        k = i + 1
        particle_block[f"Set_{k}"] = {"Type": name, "Parameters": params}
        mesh_block[f"Set_{k}"] = {
            "Mesh_Size": mesh_size,
            "File": f"mesh_ref_{k}_{name}.msh",
            "CreateMesh": {
                "Flag": True,
                "Info": "gmsh_builtin_mesh",
                "Write_Mesh_File": True,
            },
        }

    pgen_data: dict = {"N": 20}
    cols, rows = 5, 4
    dx, dy = 2.8 * s, 2.8 * s
    x0, y0 = 2.0 * s, 2.0 * s
    idx = 0
    for r in range(rows):
        for c in range(cols):
            geom_id = c % n_sets
            pgen_data[str(idx)] = {
                "x": x0 + c * dx,
                "y": y0 + r * dy,
                "z": z0,
                "theta": (idx % 7) * 0.2,
                "s": 1.0,
                "ax": 0.0,
                "ay": 0.0,
                "az": 1.0,
                "geom_id": geom_id,
                "mat_id": 0,
                "contact_id": 0,
            }
            idx += 1

    out = {
        "_readme": (
            "Merge these keys into your full input: Particle, Mesh, and Particle_Generation.Data. "
            "Keep Material/Contact/Neighbor/Model/Output/BC consistent with the rest of your deck."
        ),
        "Particle": particle_block,
        "Mesh": mesh_block,
        "Particle_Generation": {
            "Method": "From_File",
            "Random_Rotation": False,
            "Data": pgen_data,
        },
    }

    args.out_dir.mkdir(parents=True, exist_ok=True)
    path = args.out_dir / "demo_particle_mesh_pgen.json"
    path.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {path.resolve()}")


if __name__ == "__main__":
    main()
