# Compressive examples

JSON decks for `bin/PeriDEM` (not Enable_Examples C++ drivers).

| Directory | What it is |
|-----------|------------|
| `n12/` | Small packed circles (Jha-style M1); short / MPI-strategy decks |
| `n500/` | Paper N≈502 circle+hex pack — **two-stage** settle then compress (see `n500/README.md`) |

Related **ctests** (under `test/test_data/peridem/`):

- `jha2021_comp_n50` — in-process 4×3 pack with contact assert (and MPI variants)
- `compressive_mixed_inbuilt` — short in-process mixed-shape pack
- `compressive_mixed` — `PeriDEM -i input.json` with checked-in meshes
