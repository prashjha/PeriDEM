# Circle — single-particle Peridynamics

2D circle (R = 3 mm) with a fixed left patch and linear displacement on the
right. Material is `PDState` with classical K/G → PD conversion.

| File | Role |
|------|------|
| `problem.py` | The problem set up in Python (`build_deck()` + CLI) |
| `run.py` | Runs `problem.py` in-process |
| `input.json` | Same problem as a deck (T = 0.01, 20k steps) |
| `input_quick.json` | Short deck (default for `./run.sh`) |
| `mesh_cir_1_0.msh` | File mesh |
| `view.png` | Reference ParaView view |

```bash
./run.sh                          # JSON deck via bin/PeriDEM
DECK=input.json NP=2 ./run.sh     # full; auto → DOF-MPI on multi-rank
```

Python (`-DEnable_Python=ON`):

```bash
./run.py                          # Python-authored deck, reads mesh_cir_1_0.msh
./problem.py --mesh-size 3e-4     # ... or mesh the disc in-process
./problem.py --snapshot pull.png
./problem.py --write-deck /tmp/input.json
```

`python -m peridem -i input.json` runs an existing deck in-process.
Outputs go under `runs/` (`runs_py/` for the Python path); both are
gitignored.
