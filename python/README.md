# PeriDEM Python interface

Set a problem up, run it, and read the fields back without leaving Python. The
time loop stays in C++.

```python
import peridem
from peridem import Deck, Geometry
from peridem.deck import MeshSpec, contact_stiffness

peridem.init(n_threads=4)

R, h = 0.001, 0.0002            # grain radius, mesh size
horizon = 3.0 * h

d = Deck(dim=2, t_final=0.012, n_steps=36000)
d.set_output("runs/out", tags=["Displacement", "Velocity", "Damage_Z"],
             interval=3600)
d.set_gravity(0.0, -10.0)

grain = d.add_particle_type(Geometry("circle", [R, 0, 0, 0]),
                            MeshSpec(size=h))          # Gmsh, in memory
steel = d.add_material(horizon=horizon, density=1200.0, K=2.16e7, nu=0.25,
                       Gc=50.0, influence_fn_type=1)
d.add_contact_pair(0, 0, Kn=contact_stiffness(2.16e7, 2.16e7, horizon),
                   K=2.16e7, eps=0.9)

d.add_displacement_bc(particles=[0], direction=[1, 2], zero_displacement=True)
d.add_initial_velocity([0.0, -0.089, 0.0], particles=[1])  # free fall, 0.4 mm

d.place(R, R, geometry=grain, material=steel)          # fixed grain
d.place(R, 4 * R, geometry=grain, material=steel)      # falling grain

sim = d.run()

print(sim.n_particles, "grains,", sim.n_nodes, "nodes at t =", sim.time)
u = sim.displacement                    # numpy (N, 3) view into the model
for p in sim.particles:
    print(p.id, p.geometry_name, p.center_of_mass, float(p.damage.max()))
```

No JSON file is written and no `.msh` touches disk. `d.write("input.json")`
gives you the deck if you want to hand the same problem to `bin/PeriDEM`.

## Install

Needs the same C++ dependencies as PeriDEM (MPI, VTK, Metis, Gmsh,
BLAS/LAPACK), plus numpy; `snapshot()` also needs matplotlib.

```sh
# from a configured CMake build
cmake -S . -B build -DEnable_Python=ON
cmake --build build --target peridem_core PeriDEM -j
export PYTHONPATH="$PWD/build/python:$PYTHONPATH"

# or
pip install -e . --no-build-isolation
```

## This is a binding, not a second implementation

Nothing here re-implements the library. Specifically:

| Python | Calls |
|---|---|
| `Deck.*` block builders | `inp::*Deck::getExampleJson` (via `peridem.decks.*`) |
| `to_E`, `to_K`, `to_G`, `to_Gc`, `to_KIc` | `material::to*` |
| `harmonic_mean`, `contact_stiffness` | `util::harmonicMean`, `util::normalContactStiffness` |
| `Geometry` | `geom::createGeomObject`, `geom::writeGeometry` |
| `sim.break_bonds_in_slots`, `break_bonds_crossing_vertical` | `geometry::breakBondsInSlots`, `breakBondsCrossingVerticalLines` |
| `sim.setup/step/integrate/...` | `PeriDEMModel` and `time_int::` |

There is one set of defaults and one copy of each formula. Where a helper did
not exist on the C++ side it was added there rather than written here:
`util::normalContactStiffness` replaced the same expression copy-pasted at nine
sites, and `src/fracture/prenotch.h` replaced three near-duplicate notch
routines in the notched-impact driver, so the driver and the Python interface
break the same bonds.

`test/test_exec/inp/testDeckRoundTrip.cpp` keeps the deck layer honest: for
each deck it writes a block with values other than the defaults, reads it back,
and compares the values. That closes the gap that had let three writer/reader
mismatches through.

That is what makes the parity numbers below mean something: the Python deck is
built independently, then checked against the deck the C++ driver writes.

## API

### Setting up

| | |
|---|---|
| `Deck(dim, t_final, n_steps, ...)` | new deck; `particle_sim_type="Single_Particle"` for one body |
| `d.set_output(path, tags, interval, ...)` | output block |
| `d.set_gravity(gx, gy, gz)` | body force |
| `d.set_neighbor(criteria, s_factor, interval)` | contact neighbour search |
| `d.add_particle_type(geometry, mesh)` → `geom_id` | a reference shape and its mesh |
| `d.add_material(horizon=, density=, K=, nu=, Gc=, ...)` → `mat_id` | material |
| `d.add_contact_pair(i, j, Kn=, K=, eps=, ...)` | contact between two groups |
| `d.set_contact_laws(damping_law=, friction_law=, correct_volume=)` | |
| `d.add_displacement_bc(...)`, `d.add_force_bc(...)` | by particle list or by region |
| `d.add_initial_velocity(v, particles=[...])` | |
| `d.add_rigid_particle(id, mass)` | rigid body of finite mass |
| `d.place(x, y, z, geometry=, material=, contact=, theta=, scale=, wall=)` | one instance |
| `d.place_many(sites, ...)` | many instances sharing options |
| `d.set_test(name, ...)` | post-processing test block |
| `d.to_dict()`, `d.to_json()`, `d.write(path)` | the assembled deck |
| `d.simulation(workdir)`, `d.run(workdir)` | build / build and run |

`d.model`, `d.output`, `d.neighbor`, `d.materials`, `d.contact_pairs` … stay
plain Python objects, so anything the helpers do not cover you can set
directly before `to_dict()`.

### Geometry

```python
g = Geometry("ellipse_minus_ellipse", [a_out, b_out, a_in, b_in, 0, 0, 0, 0])
g.center, g.box, g.volume, g.bounding_radius, g.inscribed_radius
g.is_inside([x, y, z]), g.is_near([x, y, z], tol)

# composites: signed union of sub-shapes, and a centroid that accounts for them
wall = Geometry("complex", params,
                vec_type=["circle", "circle", "rectangle"],
                vec_flag=["plus", "minus", "plus"])
wall.center            # signed-volume centroid, not the outer circle's centre
peridem.acceptable_geometries()
```

### Meshes

```python
MeshSpec(size=1e-4)                        # Gmsh in memory, nothing on disk
MeshSpec(size=1e-4, info="uniform")        # structured grid
MeshSpec(size=1e-4, file="mesh.msh")       # also write the .msh
MeshSpec(file="mesh.msh")                  # read an existing mesh
MeshSpec(size=1e-4, info="uniform", voids=[[x0, y0, z0, x1, y1, z1]])
```

### Running and reading state

```python
sim = d.simulation()
sim.setup()                 # build particles, meshes, neighbour lists
sim.step()                  # one integrator step
sim.integrate()             # the rest of the loop, without re-initialising
sim.run()                   # init + loop + close, in one call
sim.close()

sim.n_nodes, sim.n_particles, sim.n_walls, sim.step_index, sim.time, sim.dt
sim.reference, sim.position, sim.displacement, sim.velocity, sim.force
sim.volume, sim.fixity, sim.particle_id, sim.damage
```

Node fields are **writable views into the model**, not copies: assigning into
`sim.velocity` changes the simulation, and `.copy()` is how you take a
snapshot. Re-fetch them after `setup()`, which resizes the arrays.

### Particles

```python
p = sim.particle(0)                 # grains; sim.wall(i), sim.particle_all(i)
p.id, p.is_wall, p.n_nodes, p.node_start, p.geometry_name, p.box
p.density, p.horizon, p.mesh_size, p.radius, p.center_of_mass
p.reference, p.position, p.displacement, p.velocity, p.force, p.damage
p.group_id("mat_id")
p.compute_force = False             # freeze one grain
```

Each particle's arrays are slices of the global ones, so writes through either
view land in the same place.

### Bonds

After `setup()` the peridynamic bonds can be read and broken. This is how a
pre-notch is applied, and a JSON deck has no field for it.

```python
sim.n_pd_neighbors(node), sim.pd_neighbors(node)
sim.bond_broken(node, k), sim.set_bond_broken(node, k, True)
sim.break_bonds_in_slots([-0.025, 0.025], 0.0015, y_lo, y_hi)
sim.break_bonds_crossing_vertical([0.0], y_lo, y_hi)
```

### Driving the loop yourself

```python
sim.apply_initial_condition()
sim.set_current_dt(sim.dt)
sim.apply_displacement_bc()
sim.compute_forces()
sim.apply_rigid_body_constraint()
while sim.step_index < sim.n_steps:
    sim.step()
    if sim.should_output:
        sim.write_output()
    sim.check_stop()
    if sim.stopped:
        break
```

`examples/PeriDEM/silling_kw/problem.py` uses this to record when each node
first becomes damaged, which is what gives the crack speed.

### Output

```python
data = peridem.read_vtu("runs/out/output_10.vtu")
u = data["point_data"]["Displacement"]
peridem.snapshot(data, "snap.png", color="|Displacement|")

fields = peridem.fields_from_vtu(peridem.last_vtu("runs/out"))
peridem.nodal_error(fields_a, fields_b)      # per-node Linf and rms
```

### Running an existing deck

```python
sim = peridem.Simulation.from_file("examples/Peridynamics/circle/input.json")
sim.run()
```

or from the shell:

```sh
python -m peridem -i input.json -nThreads 4
mpirun -n 2 python -m peridem -i input.json -nThreads 4
```

`from_file` resolves mesh and output paths relative to the deck's folder and
leaves the process working directory alone. `from_dict` / `from_json` take an
in-memory deck and an optional `workdir`.

## MPI

`peridem.init()` initialises MPI if it is available, and
`peridem.mpi_rank()` / `peridem.mpi_size()` report the layout. Run a script
under `mpirun` as you would the executable; both `MPI_Strategy: "particle"`
and `"dof"` work. Node fields are the calling rank's local data.

## Parity with the C++ executable

`python/tests/test_example_parity.py` checks two separate things for each
example, and neither reuses a deck the other side produced:

* **deck**: the Python deck against the deck the corresponding C++ driver
  writes, key by key, including every floating-point value;
* **run**: that deck run through `bin/PeriDEM` in a separate process and
  through the in-process interface, into separate directories, compared node by
  node.

```sh
PYTHONPATH=build/python python3 python/tests/test_example_parity.py
PYTHONPATH=build/python python3 python/tests/test_example_parity.py -k attrition
PERIDEM_PARITY_FULL=1 ... # include the full-size Silling plate
```

Current result on Linux, 1–8 threads. The last two are skipped by default
(`PERIDEM_PARITY_FULL=1` includes them) only because the C++ driver's neighbour
search on the full plate is slow, not because they are unverified:

| case | deck vs C++ | nodes | max L∞ |
|------|-------------|-------|--------|
| `twop_circ_contact` | identical | 246 | 0 |
| `ellipse_triangle` | identical | 402 | 0 |
| `compressive_n12` | identical | 2274 | 0 |
| `peridynamics_circle` | no C++ driver | 123 | 0 |
| `peridynamics_rectangle` | no C++ driver | 2601 | 0 |
| `attrition_sim1` | no C++ driver | 13553 | 0 |
| `attrition_sim2` | no C++ driver | 15367 | 0 |
| `silling_kw_quick` | identical (+170 pre-notch bonds) | 571 | 0 |
| `silling_kw_2d` | identical (+1012 pre-notch bonds) | 25350 | 0 |
| `silling_kw_3d` | identical (+26064 pre-notch bonds) | 354330 | 0 |

"no C++ driver" means the example ships a hand-written JSON deck rather than a
C++ program, so only the run comparison applies.

## Tests

```sh
python3 python/tests/run_tests.py            # unit tests
ctest --test-dir build -R Test_Python -V     # same, plus the parity suite
```

`run_tests.py` uses pytest when it is installed and a built-in runner
otherwise, so pytest is not a build dependency.

## Why nanobind

| Code | Binding | User API |
|------|---------|----------|
| HOOMD-blue | pybind11 | `Simulation`, numpy/GPU arrays |
| LAMMPS | C ABI + ctypes | `lammps()`, `command()`, extract arrays |
| FEniCSx / DOLFINx | pybind11 | mesh / form / solve in Python |
| Yade | Boost.Python | script the DEM loop in Python |
| MFEM | SWIG | older C++ wrap |

A subprocess that writes JSON and calls `bin/PeriDEM` is not a library API. A
new C ABI plus ctypes (LAMMPS) is work we do not need: `PeriDEMModel` is
already there. SWIG is the old path.

[nanobind](https://nanobind.readthedocs.io/) is the 2026 default for new
C++17 and later bindings, from the author of pybind11, with
[scikit-build-core](https://scikit-build-core.readthedocs.io/) so `pip install`
drives the existing CMake tree. The user object is `Simulation`, like HOOMD.
