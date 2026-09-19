# PeriDEM examples

JSON decks for `bin/PeriDEM`. Optional C++ drivers need `-DEnable_Examples=ON`.

## JSON / script decks

| Path | Role |
|------|------|
| `compressive/n12/` | Small compressive pack |
| `compressive/n500/` | Paper N≈502 two-stage settle + compress |
| `attrition/sim1_rotating_cylinder/` | Thick drum |
| `attrition/sim2_thin_container/` | Thin drum, offset ω |
| `silling_kw/` | Silling KW via notched-impact driver |

## C++ drivers (`-DEnable_Examples=ON`)

| Path | Role |
|------|------|
| `ellipse_triangle/` | Hollow ellipse on a tip |
| `twop_circ_contact/` | Two-circle contact |

```bash
cmake -S . -B build -DEnable_Examples=ON
cmake --build build --target example_twop_circ_contact
cmake --build build --target example_ellipse_triangle
```

Attrition: commit setup scripts + meshes/CSV only; `runs/` and generated
`input*.json` are gitignored.
