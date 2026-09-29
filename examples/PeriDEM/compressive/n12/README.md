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

## Python (`-DEnable_Python=ON`)

`problem.py` derives the whole setup from `R` and the pack size rather than
reading a deck. The grain spacing is slightly larger than the contact radius, and
the open U-channel cup and the moving plate follow from the resulting pack
bounding box. `test/test_data/peridem/jha2021_comp_n50/main.cpp` builds the
same deck in C++, and `python/tests/test_example_parity.py` compares the two key
by key.

```bash
./run.py                                  # 4x3, default 20k steps
./problem.py --ncols 6 --nrows 4          # a different pack
./problem.py --in-process-mesh            # write no .msh at all
./problem.py --num-steps 2000 --snapshot pack.png
./problem.py --write-deck /tmp/input.json # then bin/PeriDEM -i /tmp/input.json
```

To run an existing MPI-strategy deck in-process instead:

```bash
mpirun -n 4 python3 -m peridem -i input_quick_particle.json -nThreads 8
```

VTUs: `runs/out/output.pvd`.
