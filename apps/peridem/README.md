# PeriDEM executable

Wires the `src/` libraries and produces `bin/PeriDEM`.

```sh
PeriDEM -i input.json -nThreads 4
```

`src/` builds libraries only. To change contact or extra postprocessing, copy this app (or add another under `apps/`) and call `DEMModel::setContact` / `setPostprocess` before `run()` — do not edit library internals.
