# Jha JMPS 2021 — 502 circle/hex compression (modular)

Restored from `main` `examples/PeriDEM/compressive_test/n500_circ_hex/` (paper pack).
Modular decks: `modular/input_{smoke,short,paper}.json` via `convert_to_modular.py`.

## Paper parameters
- N=502 (366 circle + 136 hex), R≈1 mm ±10%, M1, horizon 0.6 mm, Rc=0.95 h, C̄=100
- dt=0.1 μs, T=0.06 s (600000 steps), plate vy=−0.06 m/s, g=−10
- Note: checked-in legacy YAML had `Time_Steps: 1` (bug); modular paper deck uses 600000

## Run (particle-MPI)
```bash
BIN=build/linux/bin/PeriDEM
cd examples/PeriDEM/compressive_test/n500_circ_hex/modular
mpirun -n 4 --quiet $BIN -i input_smoke.json -nThreads 1   # 2k steps bring-up
mpirun -n 4 --quiet $BIN -i input_short.json -nThreads 1   # T=0.003, reaction start
mpirun -n 4 --quiet $BIN -i input_paper.json -nThreads 1   # full paper (long)
```
Reaction CSV: `*/pp_compressive_test_0.csv` (compare to `assets/compressive_test_reaction_force_n500.jpg`).
