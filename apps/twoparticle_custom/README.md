# Two-particle custom app

Shows how to replace **the pair law**, **damping**, and **extra postprocessing** without editing `src/` or copying `contact.cpp`.

The library still supplies neighbor search, node–node assembly (`contact::PairForce`), and the usual two-particle metrics. This app:

- keeps the default pair law
- subclasses `contact::Damping` for extra center-to-center viscous damping (`appDamping.cpp`)
- writes kinetic energy and that extra force to `app_pp.csv` (`appPostprocess.cpp`)

A different node-node relation is a `contact::PairForce` subclass with its own data; set it with `Contact::setPairForce`. Do not override `Contact::computeForces`.

## Copy this

1. Copy `apps/twoparticle_custom/` to a new directory under `apps/`.
2. Add `add_subdirectory(...)` in `apps/CMakeLists.txt`.
3. Subclass `contact::PairForce` and/or `contact::Damping` and `postprocess::Postprocess` in the **app**.
4. After `PeriDEMModel dem(deck);` construct `contact::Contact`, call `setPairForce` / `setDamping`, then `dem.setContact(...)` and `dem.setPostprocess(...)`, then `dem.run(deck)`.

Do not change contact or postprocess internals in `src/` for a user-specific model.

## Run

```sh
TwoParticle_Custom
TwoParticle_Custom -i input.json -nThreads 4
```

With no `-i`, the app builds a short two-circle collision in process (Gmsh). With `-i`, it uses the same JSON path as `PeriDEM`.
