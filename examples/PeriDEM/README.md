# PeriDEM examples

Multi-particle problems. Each folder has a `problem.py` that sets the problem
up through the `peridem` API (`-DEnable_Python=ON`) and, where one exists, a
JSON deck for `bin/PeriDEM`.

| Path | Problem | C++ driver |
|------|---------|------------|
| `twop_circ_contact/` | Two circles, one dropped on the other | `example_twop_circ_contact` |
| `ellipse_triangle/` | Hollow ellipse split on a tip-up triangle | `example_ellipse_triangle` |
| `compressive/n12/` | 4×3 grain pack compressed by a plate | `Test_PeriDEM_jha2021_comp_n50` |
| `compressive/n500/` | Paper N≈502 two-stage settle → compress | — |
| `attrition/sim1_rotating_cylinder/` | Thick drum with an inward protrusion | — |
| `attrition/sim2_thin_container/` | Thin drum, offset spin axis | — |
| `silling_kw/` | Silling 2003 Kalthoff–Winkler impact, 2D and 3D | `Test_PeriDEM_notched_impact_inbuilt` |

```sh
cmake -S . -B build -DEnable_Python=ON -DEnable_Examples=ON -DEnable_Tests=ON
cmake --build build --target peridem_core PeriDEM \
      example_twop_circ_contact example_ellipse_triangle -j
export PYTHONPATH="$PWD/build/python:$PYTHONPATH"
```

## Python

```sh
cd ellipse_triangle
./run.py                                  # default run
./problem.py --num-steps 2000 --snapshot crack.png
./problem.py --in-process-mesh            # no .msh written at all
./problem.py --write-deck /tmp/input.json # hand the same deck to bin/PeriDEM
```

Two of these need operations a deck file has no field for:

* `compressive/n12/problem.py` derives the pack spacing, the open U-channel cup
  and the moving plate from `R` and the pack size, and places the walls at the
  centroids the geometry objects report.
* `silling_kw/problem.py` calls `sim.setup()`, cuts the peridynamic bonds that
  span each notch slot (`sim.break_bonds_in_slots`), then drives the time loop
  itself to record first-damage times and fit a crack speed.

## JSON

`./run.sh` where present, or `bin/PeriDEM -i input.json -nThreads 4`.
`DECK`, `NP` and `NTHREADS` select deck, ranks and threads.

`-deckOnly` makes a C++ driver write its deck and exit, so that the deck
comparison does not run the simulation.

## What is checked in

Setup scripts, meshes and particle-location CSVs only. `runs/` and generated
`input*.json` are gitignored.
