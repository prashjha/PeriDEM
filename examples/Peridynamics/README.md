# Peridynamics examples (single particle)

JSON decks for `bin/PeriDEM` (`Particle_Sim_Type: Single_Particle`). The driver
parses JSON only.

| Path | Mesh | Notes |
|------|------|-------|
| `circle/` | `mesh_cir_1_0.msh` | Fixed left / pull right |
| `rectangle/` | `CreateMesh` uniform | Fixed SW / pull NE |

Both accept `Model.MPI_Strategy`: `auto` (→ DOF-MPI for single particle),
`none`, or `dof`. Use `NP>1` with `mpirun` for nodal partition.

```bash
cd circle && ./run.sh                 # short deck (input_quick.json)
cd rectangle && DECK=input.json ./run.sh
```

Each folder also has a `problem.py` that builds the same problem through the
`peridem` interface (`-DEnable_Python=ON`), and a `run.py` that runs it:

```bash
cd circle && ./run.py
./problem.py --mesh-size 3e-4         # mesh the disc in-process instead
cd ../rectangle && ./problem.py --snapshot pull.png
```
