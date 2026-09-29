# Rectangle — single-particle Peridynamics

2D square plate (10 mm) with uniform `CreateMesh`, fixed SW corner, and linear
pull on the NE corner. Material is `PDState`.

| File | Role |
|------|------|
| `problem.py` | The problem set up in Python (`build_deck()` + CLI) |
| `run.py` | Runs `problem.py` in-process |
| `input.json` | Same problem as a deck (T = 0.01, 20k steps, h = 0.2 mm) |
| `input_quick.json` | Short deck (default for `./run.sh`) |
| `view.png` | Reference ParaView view |

```bash
./run.sh                          # JSON deck via bin/PeriDEM
DECK=input.json NP=2 ./run.sh     # full; auto → DOF-MPI on multi-rank
```

Python (`-DEnable_Python=ON`). Nothing is read from disk here: the uniform grid
is built in memory, so this is the shortest complete example of the interface.

```bash
./run.py
./problem.py --mesh-size 1e-4 --snapshot pull.png
./problem.py --write-deck /tmp/input.json
```

Outputs go under `runs/` (`runs_py/` for the Python path); both are
gitignored.
