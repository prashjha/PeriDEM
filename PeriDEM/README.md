# PeriDEM driver

Class `PeriDEMModel` and the `PeriDEM` executable. Libraries are in `src/`.
Run decks with `bin/PeriDEM -i input.json`. Cases live under `examples/`.

A custom program can link `PeriDEMModel` and replace `contact::PairForce`,
`contact::Damping`, or `postprocess::Postprocess`. Do not copy the time loop.

Contact / wall / self-contact policy: `src/contact/README.md`.

```sh
PeriDEM -i input.json -nThreads 4
```
