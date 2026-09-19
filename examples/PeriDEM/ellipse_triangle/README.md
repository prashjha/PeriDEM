# Ellipse × short tip

**Hollow ellipse** (`ellipse_minus_ellipse`) dropped onto a short tip-up triangle.
`Self_Contact=none` so a tip-seeded crack can open into separate pieces.

| Item | Value |
|------|-------|
| Ellipse | outer a=2 mm, b=1.4 mm; inner a=1.8 mm, b=1.22 mm |
| Tip | W=0.8 mm, H=1.2 mm |
| Gc | tip 200, ellipse 1.0 |
| Impact | `v_y=-2.5`, `T=3 ms`, tip `Beta_n=8` |

`./run.sh` wipes stale meshes/VTUs and **fails unless** mesh+VTU are elliptical (`SHAPE_OK`)
and health gates pass (through-crack → spatially separated pieces, no spray).

`problem.py` builds the same deck as `main.cpp` through the `peridem`
interface, with the same geometry, materials and contact parameters.
`python/tests/test_example_parity.py` compares the two decks key by key, so
that comparing the runs compares the interfaces and not two problems.

```bash
./run.py                                   # default 30k steps
./problem.py --num-steps 2000 --snapshot crack.png
./problem.py --in-process-mesh             # write no .msh at all
./problem.py --write-deck /tmp/input.json  # then bin/PeriDEM -i /tmp/input.json
```

Tip contact and crack opening: [ellipse_triangle_impact.png](../../../docs/assets/ellipse_triangle_impact.png).
