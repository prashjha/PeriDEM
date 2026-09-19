# Tutorial

**Current input format is JSON** (`bin/PeriDEM -i input.json`). Use the checked-in
decks under [examples/](../examples/README.md) and the root
[README - Running simulations](../README.md#Running-simulations).

This folder has Python helpers and notebooks that write `input.yaml`, `.geo`, and
particle-location CSVs. They explain geometry and BC construction. Their YAML is
not accepted by current `bin/PeriDEM` (JSON only). Updating these helpers to emit
JSON is open work.

## Contents

- [two_particles.ipynb](two_particles.ipynb) - two-particle walkthrough (writes YAML)
- [particle_wall.py](particle_wall.py) - particle + wall helper (writes YAML)
- [setup_two_particles_wall.py](setup_two_particles_wall.py) - concave particles + wall (writes YAML)
- Compressive notebook path was never finished (`TBA` below)

## Compressive test

> TBA (use [examples/PeriDEM/compressive](../examples/PeriDEM/compressive) instead)
