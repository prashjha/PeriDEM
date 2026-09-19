# Rectangle — single-particle Peridynamics

2D square plate (10 mm) with uniform `CreateMesh`, fixed SW corner, and linear
pull on the NE corner. Material is `PDState`.

| File | Role |
|------|------|
| `input.json` | Full run (T = 0.01, 20k steps, h = 0.2 mm) |
| `input_smoke.json` | Short run (default for `./run.sh`) |
| `view.png` | Reference ParaView view |

```bash
./run.sh                          # short deck
DECK=input.json NP=2 ./run.sh     # full; auto → DOF-MPI on multi-rank
```

Requires `bin/PeriDEM`. Outputs go under `runs/` (gitignored).
