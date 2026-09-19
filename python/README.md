# PeriDEM Python interface

Setup, run, and read fields from Python. The time loop stays in C++.

## What other codes do

| Code | Binding | User API |
|------|---------|----------|
| HOOMD-blue | pybind11 | `Simulation`, numpy/GPU arrays |
| LAMMPS | C ABI + ctypes | `lammps()`, `command()`, extract arrays |
| FEniCSx / DOLFINx | pybind11 | mesh / form / solve in Python |
| Yade | Boost.Python | script the DEM loop in Python |
| MFEM | SWIG | older C++ wrap |

A subprocess that writes JSON and calls `bin/PeriDEM` is not a library API.
A new C ABI plus ctypes (LAMMPS) is extra work we do not need: we already have
`PeriDEMModel`. SWIG is the old path.

**Choice:** [nanobind](https://nanobind.readthedocs.io/) (2026 default for new
C++17+ bindings; same author as pybind11, smaller/faster) plus
[scikit-build-core](https://scikit-build-core.readthedocs.io/) so `pip install`
drives the existing CMake tree. The user object is `Simulation`, like HOOMD.

## Install

Needs the same C++ deps as PeriDEM (MPI, VTK, Metis, Gmsh, BLAS/LAPACK).

```sh
# from a configured CMake build
cmake -S . -B build -DEnable_Python=ON
cmake --build build --target peridem_core
export PYTHONPATH="$PWD/build/python:$PYTHONPATH"

# or
pip install -e . --no-build-isolation
```

## Use

```python
import peridem

peridem.init(n_threads=4)
sim = peridem.Simulation.from_file("examples/Peridynamics/circle/input_quick.json")
sim.run()

print(peridem.version(), sim.time, sim.n_nodes)
u = sim.displacement   # numpy (N, 3)
z = sim.damage         # numpy (N,)
```

JSON examples also have `run.py` next to `run.sh` (same `DECK` / `NP` / `NTHREADS`):

```sh
cd examples/Peridynamics/circle
./run.py
DECK=input.json NP=2 ./run.py
# or
python -m peridem -i input.json -nThreads 4
```

`from_file` resolves mesh and output paths from the deck folder (the Python
process working directory is left unchanged). `from_dict` / `from_json` take a
deck in memory and use the current working directory.

```python
sim.setup()   # init particles / mesh
sim.step()    # one integrator step
```

## Output

```python
data = peridem.read_vtu("runs/output_10.vtu")
u = data["point_data"]["Displacement"]
peridem.snapshot(data, "snap.png", color="|Displacement|")
```
