# Examples

Every example here can be run two ways, and they are the same problem:

* **JSON and `bin/PeriDEM`**: `./run.sh`, or `bin/PeriDEM -i input.json`.
* **Python**: `./run.py`, which calls the `problem.py` next to it. That file
  builds the deck through the `peridem` interface: geometry, mesh, material,
  contact, boundary conditions and particle placement. Requires
  `-DEnable_Python=ON`.

Some examples also have a C++ driver (`-DEnable_Examples=ON`) that builds the
same deck in C++. Those are the examples whose decks are compared key by key.

## What each folder holds

| File | Role |
|------|------|
| `problem.py` | the problem, set up in Python (`build_deck()` plus a CLI) |
| `run.py` | runs `problem.py` in the calling process |
| `run.sh` | runs `bin/PeriDEM` on the JSON deck |
| `input*.json` | the same problem as a deck, for `bin/PeriDEM` |
| `main.cpp` | C++ driver, where one exists |
| `runs/` | output (gitignored) |

## The examples

| Path | Problem | C++ driver |
|------|---------|------------|
| `PeriDEM/twop_circ_contact/` | Two circles, one dropped on the other | yes |
| `PeriDEM/ellipse_triangle/` | Hollow ellipse split on a tip-up triangle | yes |
| `PeriDEM/compressive/n12/` | 4×3 grain pack compressed by a plate | yes (`jha2021_comp_n50`) |
| `PeriDEM/compressive/n500/` | Paper N≈502 pack, two-stage settle → compress | — |
| `PeriDEM/attrition/sim1_rotating_cylinder/` | Thick rotating drum with a protrusion | — |
| `PeriDEM/attrition/sim2_thin_container/` | Thin drum, offset spin axis | — |
| `PeriDEM/silling_kw/` | Silling 2003 Kalthoff–Winkler impact, 2D and 3D | yes (`notched_impact_inbuilt`) |
| `Peridynamics/circle/` | Single disc, fixed patch and linear pull | — |
| `Peridynamics/rectangle/` | Single plate on a uniform in-process grid | — |

## Running them

```sh
cmake -S . -B build -DEnable_Python=ON -DEnable_Examples=ON -DEnable_Tests=ON
cmake --build build --target peridem_core PeriDEM -j
export PYTHONPATH="$PWD/build/python:$PYTHONPATH"

cd examples/Peridynamics/rectangle
./run.py                             # Python, mesh built in memory
./problem.py --help                  # per-example options
./problem.py --write-deck /tmp/input.json
../../../build/bin/PeriDEM -i /tmp/input.json -nThreads 4
```

`run.sh` still honours `DECK` / `NP` / `NTHREADS` for the JSON path.

## Meshes

Where the mesh does not need to be a specific one, `problem.py` has Gmsh
generate it in the calling process and writes no file. That is what
`--in-process-mesh` and `--mesh-size` select.

Where a specific mesh is needed, which is the case for the attrition packs and
for runs compared against archived output, `problem.py` reads the committed
`.msh`. Those files, the particle-location CSVs and the setup scripts are what
is checked in. `runs/` and generated `input*.json` are gitignored.

## Parity

`python/tests/test_example_parity.py` rebuilds each example's deck in Python,
compares it against the deck its C++ driver writes, then runs that deck through
`bin/PeriDEM` and through the interface and compares the nodes. See
[python/README.md](../python/README.md#parity-with-the-c-executable).
