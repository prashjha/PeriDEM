# Compressive n12 (small pack)

Physics from the Jha et al. JMPS 2021 M1 / contact setup as a **tight packed multi-circle** case
(default 4×3 particles). This is **not** the paper’s 502-particle run.

Deck is independent of MPI mode. Set `Model.MPI_Strategy`:

| Value | Meaning |
|-------|---------|
| `auto` | Multi_Particle → Particle-MPI; Single_Particle → DOF-MPI |
| `none` | No domain decomposition (use `mpirun -n 1`) |
| `particle` | Particle-MPI: whole particles per rank |
| `dof` | DOF-MPI: nodes distributed across ranks |

## Run (from a **copy under build/**)

```bash
BIN=../../../../bin/PeriDEM   # adjust
# serial / none
mpirun -n 1 $BIN -i input_quick_none.json -nThreads 8
# Particle-MPI
mpirun -n 4 $BIN -i input_quick_particle.json -nThreads 8
# DOF-MPI
mpirun -n 4 $BIN -i input_quick_dof.json -nThreads 8

# longer identity run (T=0.012, 60k)
mpirun -n 1 $BIN -i input_long.json -nThreads 8
```

Keep run outputs out of the source tree; copy this folder into `build/linux/examples/...` first.
