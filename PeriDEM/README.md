# PeriDEM driver

Class `PeriDEMModel` and the `PeriDEM` executable. Libraries are in `src/`. Custom programs belong in `apps/` and replace cores (`contact::PairForce`, `contact::Damping`, `postprocess::Postprocess`) rather than copying library loops.

```sh
PeriDEM -i input.json -nThreads 4
```
