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

Results are written under `runs/` (gitignored).
