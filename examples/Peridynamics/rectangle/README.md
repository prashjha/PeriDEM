# Rectangle — single-particle Peridynamics

2D square plate (10 mm) with uniform `CreateMesh`, fixed SW corner, and linear
pull on the NE corner. Material is `PDState`.

| File | Role |
|------|------|
| `input.json` | Full run (T = 0.01, 20k steps, h = 0.2 mm) |
| `input_quick.json` | Short run (default for `./run.sh`) |
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
