# Silling 2003 Kalthoff–Winkler (KW)

Driver: `Test_PeriDEM_notched_impact_inbuilt` (built with `-DEnable_Tests=ON`).

| Case | Flag | Plate |
|------|------|-------|
| 2D | (default) | 200×100 mm structured plate with U-slot voids |
| 3D | `-dim3` | 200×100×9 mm |

Common settings: rigid impactor, PMB tension, `Bond_Break=tension`, Δt = 2.5 ns, T = 170 µs.

```bash
./run_2d.sh
./run_3d.sh
```

## Python

`problem.py` builds the same case through the `peridem` API. This example is
the one that needs more than a deck: the notch slots are carved out of the
structured mesh, but the horizon is twice the slot width, so after the model is
built the bonds still spanning each slot have to be cut.

```python
sim = deck.simulation()
sim.setup()
sim.break_bonds_in_slots([-notch_half, notch_half], notch_w, y_tip, y_top)
sim.integrate()          # the loop, without re-initialising
```

The time loop can also be driven step by step to record when each node first
becomes damaged. That arrival-time field is how a crack speed is measured
(Silling reports ~900 m/s for this case). The report includes the correlation
of the fit — on the `--quick` variant the plate is small and soft enough that
damage does not run as a front, and the fit says so rather than quoting a
number as if it meant something:

```bash
./run.py                          # 2D, default 170 µs
./problem.py --quick              # smaller, softer plate, shorter window
./problem.py --arrival-times      # drive the loop, fit the crack speed
./problem.py --dim3               # Silling's 200×100×9 mm plate
./problem.py --write-deck /tmp/input.json
```

`python/tests/test_example_parity.py` compares the Python deck with the one
`Test_PeriDEM_notched_impact_inbuilt` writes, key by key, and compares the
number of pre-notch bonds broken from Python with the number the C++ driver
reports.

Results are written under `runs/` (`runs_py/` for the Python path); both are
gitignored.
