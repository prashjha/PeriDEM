# Examples

## PeriDEM (multi-particle)

**C++ drivers** (`-DEnable_Examples=ON`): see `PeriDEM/README.md`.

| Path | Role |
|------|------|
| `PeriDEM/twop_circ_contact/` | Two circles; shares the twop inbuilt test driver |
| `PeriDEM/silling_kw/` | Silling KW 2D/3D (scripts → notched_impact driver) |
| `PeriDEM/compressive/n12/` | Small compressive pack — JSON for `bin/PeriDEM` |
| `PeriDEM/compressive/n500/` | Paper N≈502 pack + restart — JSON for `bin/PeriDEM` |
| `PeriDEM/attrition/sim1_rotating_cylinder/` | Thick rotating drum (GIF sim1) |
| `PeriDEM/attrition/sim2_thin_container/` | Thin drum, offset rotation (GIF sim2) |
| `PeriDEM/ellipse_triangle/` | Hollow ellipse dropped on tip-up triangle |

## Peridynamics (single particle)

Manual YAML demos (not Enable_Examples): `Peridynamics/circle/`, `Peridynamics/rectangle/`.

A curated showcase suite (Silling KW, compressive restart, attrition, ellipse–triangle)
will live under `../ShowcaseExamples/` once runs are packaged.
