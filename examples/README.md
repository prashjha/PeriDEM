# Examples

Run cases with `bin/PeriDEM` and the JSON decks in this tree. That is the
setup path.

Optional C++ programs (`-DEnable_Examples=ON`) wrap a few of the same cases
as in-process drivers. They are not required to run a deck.

## JSON decks (`bin/PeriDEM`)

| Path | Role |
|------|------|
| `PeriDEM/compressive/n12/` | Small compressive pack; MPI-strategy decks |
| `PeriDEM/compressive/n500/` | Paper N≈502 pack + restart |
| `PeriDEM/attrition/sim1_rotating_cylinder/` | Thick rotating drum |
| `PeriDEM/attrition/sim2_thin_container/` | Thin drum, offset rotation |
| `PeriDEM/silling_kw/` | Silling KW 2D/3D (scripts → notched-impact driver) |
| `Peridynamics/circle/` | Single particle; file mesh; fixed / pull BC |
| `Peridynamics/rectangle/` | Single particle; `CreateMesh`; fixed / pull BC |

`./run.sh` defaults to `input_quick.json`. Set `DECK=input.json` for the full run.

## C++ drivers (`-DEnable_Examples=ON`)

| Path | Role |
|------|------|
| `PeriDEM/twop_circ_contact/` | Two circles; shares the twop inbuilt test |
| `PeriDEM/ellipse_triangle/` | Hollow ellipse dropped on a tip |

See `PeriDEM/README.md` and `Peridynamics/README.md`.
