# PeriDEM examples

Optional C++ drivers live under subfolders (for example `compression_large_set_inbuilt/` with `main.cpp`).

## Build

Examples are **not** built by default. Configure with:

```bash
cmake -S . -B build -DEnable_Examples=ON
cmake --build build --target example_compression_large_set_inbuilt
```

The executable is written next to that example’s build directory, for example:

`build/examples/PeriDEM/compression_large_set_inbuilt/example_compression_large_set_inbuilt`

(Add `-DCMAKE_BUILD_TYPE=Debug` if you use a multi-config tree.)

## Relation to tests

The same pattern exists under `test/test_data/peridem/*/`: those targets are enabled when **`Enable_Tests=ON`** (see `test/CMakeLists.txt`). The copies here are for local edits without touching the test tree.

`twop_circ_contact` shares `test/test_data/peridem/twop_circ_inbuilt/main.cpp`. The short ctest `Test_PeriDEM_twop_circ_inbuilt` stays 0.002 s / 6000 steps. `Test_PeriDEM_twop_circ_inbuilt_contact` uses 0.012 s / 36000 steps and fails if max `Damage_Z` is 0. The example defaults to that contact window. Override with `-finalTime` / `-numSteps`.
