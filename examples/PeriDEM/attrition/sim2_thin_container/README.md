# Attrition sim2 — thin container + offset rotation

Modular port of PeriDEM main
`sim2_...thin_container_and_change_rotation_rate`
(GIF: `docs/assets/attrition_test_sim2.gif`).

See **`INPUT_DEFAULTS.md`** for explicit keys (avoid modular defaults).

| Item | Value |
|------|-------|
| Container | `R_in=0.02`, `R_out=0.0203`, protrusion |
| Horizon | `2·h = 4e-4` |
| Contact | Kn_Factor=1, Damping_Law=off, Correct_Volume=false |
| Motion | ω=`-40π` about (−0.004, 0.004) |

```bash
./run.sh                          # default paper if DECK unset in this copy: see run.sh
DECK=input_short.json ./run.sh
CLEAN=1 ./run.sh
```

Python (`-DEnable_Python=ON`), same problem through the `peridem` API:

```bash
./run.py                          # preset "short"
./problem.py --preset paper --snapshot damage.png
./problem.py --write-deck /tmp/input.json
```

`problem.py` checks the initial pack before building the deck. No grain may
start inside the wall, inside the protrusion, or overlapping another grain.
