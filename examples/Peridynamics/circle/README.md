# Circle — single-particle Peridynamics

2D circle (R = 3 mm) with a fixed left patch and linear displacement on the
right. Material is `PDState` with classical K/G → PD conversion.

| File | Role |
|------|------|
| `input.json` | Full run (T = 0.01, 20k steps) |
| `input_quick.json` | Short run (default for `./run.sh`) |
| `mesh_cir_1_0.msh` | File mesh |
| `view.png` | Reference ParaView view |

```bash
./run.sh                          # short deck
DECK=input.json NP=2 ./run.sh     # full; auto → DOF-MPI on multi-rank

# same deck in-process (needs -DEnable_Python=ON)
./run.py
DECK=input.json NP=2 ./run.py
```

C++: `bin/PeriDEM`. Python: `run.py` / `python -m peridem -i input.json`.
Outputs go under `runs/` (gitignored).
