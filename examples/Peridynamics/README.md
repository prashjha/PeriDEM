# Peridynamics examples (single particle)

JSON decks for `bin/PeriDEM` (`Particle_Sim_Type: Single_Particle`). Legacy YAML
is removed — the modular driver parses JSON only.

| Path | Mesh | Notes |
|------|------|-------|
| `circle/` | `mesh_cir_1_0.msh` | Fixed left / pull right |
| `rectangle/` | `CreateMesh` uniform | Fixed SW / pull NE |

Both accept `Model.MPI_Strategy`: `auto` (→ DOF-MPI for single particle),
`none`, or `dof`. Use `NP>1` with `mpirun` for nodal partition.

```bash
cd circle && ./run.sh                 # short deck (input_smoke.json)
cd rectangle && DECK=input.json ./run.sh
```

Same physics also lives under `apps/peridynamics/example/` (app-local copies).
