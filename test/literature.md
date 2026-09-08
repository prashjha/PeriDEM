# Literature tests

Quantitative checks against published PeriDEM results. Local PDFs (see `.agent/GOAL.md`):

`Work/Documents/Papers/granular_media/`

## Jha et al., JMPS 2021, 151:104376

Two-particle (Section 4.1, Fig. 5, Table 2). Bottom circle fixed; top particle. \(R_1=R_2=1\,\mathrm{mm}\), material M1, \(g=10\,\mathrm{m/s}^2\). \(H_0=1\,\mathrm{mm}\) is the paper drop height. Coefficient of restitution \(\mathrm{CR}=\sqrt{H_1/H_0}\) where \(H_1\) is the first rebound peak of the surface gap.

Paper discretization: \(h=0.1423\,\mathrm{mm}\) (min node spacing), horizon \(\epsilon=0.6\,\mathrm{mm}\), \(\Delta t=0.2\,\mu\mathrm{s}\), \(T=0.04\,\mathrm{s}\). Contact radius \(R_c=0.95h\). Damping is center-to-center with \(\bar C=100\); \(\bar\varepsilon_n\) is deck `Epsilon`. Pair bulk modulus \(\kappa_{\mathrm{eff}}\) is filled in `Contact::setup` from the two materials (same as v0.1.0 `equivalentMass` of the bulks).

The problem is the v0.1.0 `circ_damp` energy, not a 1 mm rest drop. Surface gap at \(t=0\) is the horizon (0.6 mm); the leftover 0.4 mm of the paper \(H_0=1\,\mathrm{mm}\) is already in \(v_y=-\sqrt{2g\cdot 0.4\,\mathrm{mm}}\). CR still uses \(H_0=1\,\mathrm{mm}\) because that is the energy-equivalent drop. Frozen mesh and `-inbuiltMesh` share this pair (log: `Table 2 kinematics: … surface gap = 0.0006, v_y = -0.08944`). Asking CR \(\le 1\) on a 1 mm gap **plus** that velocity would be a different (over-energized) problem.

Frozen meshes: `test/test_data/peridem/jha2021_table2/mesh_cir_{1,2}.msh`, 125 nodes, \(h_{\min}=0.1423\,\mathrm{mm}\). `Friction_On` is false and \(\mu=0\) (v0.1.0 did not read `Friction_Coeff` when friction was off). Neighbor search every step, `Search_Factor=5`, `Test_Name=two_particle` (force off on the fixed particle).

Binary: `Test_PeriDEM_twop_circ_inbuilt`. Flags: `-jha2021Table2` (test 1) or `-jha2021Table2Test n` (n = 1..5). `-inbuiltMesh` uses in-process Gmsh. ~7 min each at 4 threads.

| Test | \(\bar\varepsilon_n\) | Paper CR | Frozen CR | Inbuilt CR | ctest |
|------|----------------------|----------|-----------|------------|-------|
| 1 | 1 | 1 | 0.997 | 0.997 | `Test_PeriDEM_twop_jha2021_table2_1`, `_inbuilt_1` |
| 2 | 0.95 | 0.946 | 0.946 | 0.946 | `Test_PeriDEM_twop_jha2021_table2_2`, `_inbuilt_2` |
| 3 | 0.9 | 0.893 | 0.894 | — | `Test_PeriDEM_twop_jha2021_table2_3` |
| 4 | 0.85 | 0.845 | 0.844 | — | `Test_PeriDEM_twop_jha2021_table2_4` |
| 5 | 0.8 | 0.796 | 0.797 | — | `Test_PeriDEM_twop_jha2021_table2_5` |

Do not generate this mesh with Gmsh `Mesh_Size=0.1423\,\mathrm{mm}`: that value is the **realized min nodal spacing** of the frozen mesh, not the geo characteristic length. v0.1.0 used \(l_c=R/5=0.2\,\mathrm{mm}\); setting `Mesh_Size` to the paper \(h\) forces a finer mesh (\(h_{\min}\approx 0.091\,\mathrm{mm}\), 232 nodes) and an unstable bounce.

In-process Gmsh with \(l_c=R/5\) (Frontal-Delaunay, algorithm 6) gives 123 nodes and \(h_{\min}=0.141\,\mathrm{mm}\). Same kinematics as the frozen mesh: tests 1 and 2 give CR \(0.997\) and \(0.946\), both below 1. The earlier inbuilt CR \(\approx 1.12\) was **not** a timestep or damping-sign bug: `-inbuiltMesh` had been placed at a 1 mm gap **and** given the 0.4 mm impact velocity, i.e. 1.4 mm of drop energy while CR still divided by \(H_0=1\,\mathrm{mm}\).

Damping sweep on that inbuilt mesh (\(\bar\varepsilon_n=0.95\), paper \(\Delta t\)):

| \(\bar C\) | CR |
|-----------:|---:|
| 0 (and \(\bar\varepsilon_n=1\)) | 0.997 |
| 50 | 0.971 |
| 100 (paper) | 0.946 |
| 200 | 0.899 |

CR falls as \(\bar C\) rises; zero damping is not above 1. Flag `-inbuiltMesh` and `-betaNFactor`. Frozen tests 1–5 remain the paper-mesh pin; inbuilt 1–2 check that Gmsh does not change the energy.

| Case | Where | Metric | Reference | In repo |
|------|--------|--------|-----------|---------|
| Table 3 | mixed radius / M1–M2 | CR | 0.716 … 1 | not yet |
| Table 4 | mesh refinement, \(\bar\varepsilon_n=0.95\) | CR vs \(h\) | 0.946 … 0.977 | not yet |
| Fig. 7–10 | impact \(v_0\) | damage \(Z\) | qualitative | not yet |
| Section 4.2 | two-particle + wall | damage | qualitative | not yet |
| Compression | 500 mixed grains (paper) | wall reaction vs δ | Fig. 12-ish | reduced n50 below |

Paper §4.3 is 502 mixed circles/hexes, \(T=0.06\,\mathrm{s}\), \(\Delta t=0.1\,\mu\mathrm{s}\). Too slow as a ctest. The in-repo case is a **short contacting example**: 12 equal circles (\(4\times 3\)), same M1 / \(l_c=R/5\) / horizon \(0.6\,\mathrm{mm}\) / \(R_c=0.95h\) / \(\bar C=100\) / plate \(v_y=-0.06\). Cup is an `open_rect_channel_2d` (not a boolean annulus). Container and plate are tagged `is_wall` so they are not evolved as PD bodies. Surface gap starts at \(1.15 R_c\) (outside contact). \(\Delta t=0.2\,\mu\mathrm{s}\) (Table 2; \(\approx 0.25\,h/c\)). \(T=0.004\,\mathrm{s}\), 20000 steps. Driver `Test_PeriDEM_jha2021_comp_n50` fails unless grain–grain contact occurs. This does not reproduce the paper’s 502-grain reaction curve.

Materials (Table 1): M1 \(\rho=1200\), \(K=0.0216\,\mathrm{GPa}\), \(G=0.01296\,\mathrm{GPa}\), \(G_c=50\,\mathrm{J/m}^2\). M2 (PMMA) \(K=2\,\mathrm{GPa}\), \(G=1.2\,\mathrm{GPa}\), \(G_c=500\,\mathrm{J/m}^2\).

Run: `ctest --test-dir build/linux -R 'Test_PeriDEM_twop_jha2021_(table2|inbuilt)|Test_PeriDEM_jha2021_comp_n50' --output-on-failure`.

## Bhattacharya and Lipton, SISC 2023

Grain shape and topology vs bulk response in PeriDEM. No CR table. Next: pick a two-grain or small-pack figure and a numeric bulk-stress or damage metric.

## Bhattacharya, Damircheli, Lipton, 2025 (arXiv:2506.05362)

3D crushing / Kalthoff–Winkler / CT sand aggregates. Later; needs 3D grains.

## Bhattacharya and Lipton, 2025b (vehicle mobility)

Wheel on gravel: slip, torque, displacement. Later; not a two-particle case.
