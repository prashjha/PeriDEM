# Attrition examples (main GIF suite)

| Dir | Main reference | Notes |
|-----|----------------|-------|
| [`sim1_rotating_cylinder/`](sim1_rotating_cylinder/) | `sim1_..._with_protrusion` | Thick drum `R_out=0.021`, ω=−20π about origin |
| [`sim2_thin_container/`](sim2_thin_container/) | `sim2_..._thin_container_and_change_rotation_rate` | Thin drum `R_out=0.0203`, ω=−40π about (−0.004, 0.004) |

Each folder: `gen_input.py` + `run.sh` + meshes/CSV. Generated `input*.json` and
`runs/` stay local (see `.gitignore`).

```bash
cd sim1_rotating_cylinder && ./run.sh          # paper T=0.1
cd ../sim2_thin_container && DECK=input_short.json ./run.sh
# preserve existing VTUs: omit CLEAN=1 (default). Fresh out: CLEAN=1 ./run.sh
```
