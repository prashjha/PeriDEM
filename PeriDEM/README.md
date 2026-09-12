# PeriDEM driver

Class `PeriDEMModel` and the `PeriDEM` executable. Libraries are in `src/`. Custom programs belong in `apps/` and replace cores (`contact::PairForce`, `contact::Damping`, `postprocess::Postprocess`) rather than copying library loops.

Contact / wall / self-contact policy selection: see `src/contact/README.md`.

```sh
PeriDEM -i input.json -nThreads 4
```
