# Compressive N≈502 (Jha JMPS 2021 §4.4)

Two-stage paper protocol. Force–penetration needs **stage 2** (settled IC + top wall relocated). Stage 1 alone never contacts the pack.

## Paper parameters
- N=502 (366 circle + 136 hex), R≈1 mm ±10%, M1, horizon 0.6 mm, Rc=0.95 h, C̄=100
- dt=0.1 μs, plate vy=−0.06 m/s, g=−10

## Stages

| Deck | Role | Wall `503.y` | Time | Restart |
|------|------|--------------|------|---------|
| `input_stage1.json` (alias: `input_paper.json`) | Gravity settle | **0.04152** | T=0.06 | none |
| `input_stage2.json` | Compression / Fig. 23 | **0.03510** | T=0.15 | `restart/restart_t0p06_settled` @ step 600000 |

Wall move is a **reference** shift in `Particle_Generation`, not a DispBC change. BC stays `u = −0.06 t`. At restart t=0.06 that places the wall bottom edge at the paper’s **0.0312 m**.

Short decks: `input_quick.json` (2k steps), `input_short.json` (T=0.003).

## Run

```bash
BIN=build/linux/bin/PeriDEM
cd examples/PeriDEM/compressive/n500

# optional bring-up
mpirun -n 4 --quiet $BIN -i input_quick.json -nThreads 1

# stage 1 settle (long) — or skip if using the checked-in settled VTU
NP=4 NTHREADS=1 ./run_stage1.sh

# stage 2 compress (force–penetration CSV)
NP=1 NTHREADS=8 ./run_stage2.sh
```

Reaction CSV: `runs/stage2/out/pp_compressive_test_0.csv` (force per unit area: divide by wall length 0.06894 m).

Outputs under `runs/` are gitignored. Wipe a run dir with `CLEAN=1 ./run_stage*.sh`.

## Restart IC
- Checked-in: `restart/restart_t0p06_settled.vtu` (settled pack at t=0.06).
- To regenerate: finish stage 1, copy the final output frame to that path (Ascii/zlib VTU readable as Restart).
