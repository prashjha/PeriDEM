# Circle — single-particle Peridynamics

2D circle (R = 3 mm) with a fixed left patch and linear displacement on the
right. Material is `PDState` with classical K/G → PD conversion.

| File | Role |
|------|------|
| `input.json` | Full run (T = 0.01, 20k steps) |
| `input_smoke.json` | Short run (default for `./run.sh`) |
| `mesh_cir_1_0.msh` | File mesh |
| `view.png` | Reference ParaView view |

```bash
./run.sh                          # short deck
DECK=input.json NP=2 ./run.sh     # full; auto → DOF-MPI on multi-rank
```

Requires `bin/PeriDEM`. Outputs go under `runs/` (gitignored).
