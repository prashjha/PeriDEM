# Attrition sim1 — rotating cylinder + protrusion

Modular port of PeriDEM main
`sim1_multi_particle_circ_tri_drum_with_rotating_cylinder_with_protrusion`
(GIF: `docs/assets/attrition_test_sim1.gif`).

| Item | Value |
|------|-------|
| Container | `R_in=0.02`, `R_out=0.021`, protrusion |
| Grains | cir / tri / drum × small / large |
| Contact | Kn Silling, Kn_Factor=1, Damping_On=false, Correct_Volume=false |
| Fracture | Bond_Break=tension, Gc=50 / 100 |
| Motion | ω=`-20π` about origin |

```bash
./run.sh                          # paper T=0.1 (keeps existing runs/ unless CLEAN=1)
DECK=input_medium.json ./run.sh
DECK=input_short.json ./run.sh
CLEAN=1 ./run.sh                  # wipe VTUs then run
```
