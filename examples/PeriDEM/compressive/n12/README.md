# Compressive n12 (small pack)

Physics from the Jha et al. JMPS 2021 M1 / contact setup as a **tight packed multi-circle** case
(default 4×3 grains). This is **not** the paper’s 502-grain run.

Deck is independent of MPI mode. Set `Model.MPI_Strategy`:

| Value | Meaning |
|-------|---------|
| `auto` | Multi_Particle → Particle-MPI; Single_Particle → DOF-MPI |
| `none` | No domain decomposition (use `mpirun -n 1`) |
| `particle` | Particle-MPI: whole grains per rank |
| `dof` | DOF-MPI: nodes distributed across ranks (including packs with walls) |

## Run (from a **copy under build/**)

```bash
BIN=../../../../bin/PeriDEM   # adjust
# serial / none
mpirun -n 1 --quiet $BIN -i input_smoke_none.json -nThreads 8
# particle-MPI
mpirun -n 4 --quiet $BIN -i input_smoke_particle.json -nThreads 8
# DOF-MPI
mpirun -n 4 --quiet $BIN -i input_smoke_dof.json -nThreads 8

# long identity (T=0.012, 60k)
mpirun -n 1 --quiet $BIN -i input_long.json -nThreads 8
```

Keep run outputs out of the source tree; copy this folder into `build/linux/examples/...` first.
