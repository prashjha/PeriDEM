# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

import json
from pathlib import Path

import numpy as np
import peridem

_CIRCLE_QUICK = (
    Path(__file__).resolve().parents[2]
    / "examples"
    / "Peridynamics"
    / "circle"
    / "input_quick.json"
)
_CIRCLE_MESH = _CIRCLE_QUICK.parent / "mesh_cir_1_0.msh"


def test_version_and_init():
    peridem.init(n_threads=1)
    assert peridem.version().count(".") == 2
    assert peridem.mpi_size() >= 1


def test_from_file_missing():
    try:
        peridem.Simulation.from_file("/tmp/no_such_peridem.json")
    except RuntimeError as e:
        assert "does not exist" in str(e)
    else:
        raise AssertionError("expected missing-file error")


def test_from_json_not_peridem():
    try:
        peridem.Simulation.from_json("{}")
    except RuntimeError as e:
        assert "not a PeriDEM" in str(e)
    else:
        raise AssertionError("expected non-PeriDEM error")


def test_setup_and_fields():
    peridem.init(n_threads=1)
    sim = peridem.Simulation.from_file(str(_CIRCLE_QUICK))
    sim.setup()
    n = sim.n_nodes
    assert n > 0
    u = sim.displacement
    assert u.shape == (n, 3)
    assert sim.damage.shape == (n,)
    assert np.allclose(sim.position, sim.reference + u)
    u[0, 0] = 1e9
    assert sim.displacement[0, 0] != 1e9
    sim.step()
    assert sim.step_index == 1
    assert sim.time > 0.0


def test_from_dict_and_run_matches_step(tmp_path):
    peridem.init(n_threads=1)
    deck = json.loads(_CIRCLE_QUICK.read_text())
    deck["Mesh"]["Set_1"]["File"] = str(_CIRCLE_MESH.resolve())
    deck["Output"]["Path"] = str(tmp_path / "out") + "/"
    deck["Output"]["Debug"] = 0
    sim_run = peridem.Simulation.from_dict(deck)
    sim_run.run()
    assert sim_run.step_index == 400
    sim_step = peridem.Simulation.from_dict(deck)
    sim_step.setup()
    for _ in range(400):
        sim_step.step()
    assert np.allclose(sim_run.displacement, sim_step.displacement)
