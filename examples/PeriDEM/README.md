# PeriDEM examples

## Build (C++ drivers only)

```bash
cmake -S . -B build -DEnable_Examples=ON
cmake --build build --target example_twop_circ_contact
cmake --build build --target example_ellipse_triangle
```

## JSON / script demos (`bin/PeriDEM`)

| Path | Role |
|------|------|
| `compressive/n12/` | Small compressive pack |
| `compressive/n500/` | Paper-scale pack + restart |
| `attrition/sim1_rotating_cylinder/` | Thick drum, GIF sim1 |
| `attrition/sim2_thin_container/` | Thin drum, offset ω, GIF sim2 |
| `silling_kw/` | Silling KW via notched_impact driver |
| `ellipse_triangle/` | Hollow ellipse on tip-up triangle (C++ example) |
| `twop_circ_contact/` | Two-circle contact (C++ example) |

Attrition: commit setup scripts + meshes/CSV only; `runs/` and generated
`input*.json` are gitignored.
