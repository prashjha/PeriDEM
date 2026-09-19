# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Unit tests for the Python interface: runtime, geometry, decks, particles."""

import json
import math
from pathlib import Path

import numpy as np
import peridem
from peridem import Deck, Geometry
from peridem.deck import MeshSpec, contact_stiffness, to_G

REPO = Path(__file__).resolve().parents[2]
_CIRCLE_QUICK = REPO / "examples/Peridynamics/circle/input_quick.json"
_CIRCLE_MESH = _CIRCLE_QUICK.parent / "mesh_cir_1_0.msh"


# ---------------------------------------------------------------------------
# runtime
# ---------------------------------------------------------------------------


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


# ---------------------------------------------------------------------------
# elastic helpers agree with the C++ conversions
# ---------------------------------------------------------------------------


def test_elastic_conversions():
    K, nu = 2.16e7, 0.25
    E = peridem.to_E(K, nu)
    assert math.isclose(E, K * 3.0 * (1.0 - 2.0 * nu), rel_tol=0, abs_tol=0)
    assert math.isclose(peridem.to_K(E, nu), K, rel_tol=1e-15)
    G = to_G(E=E, nu=nu)
    assert math.isclose(G, E / (2.0 * (1.0 + nu)), rel_tol=0, abs_tol=0)
    assert math.isclose(peridem.harmonic_mean(2.0, 4.0), 8.0 / 3.0,
                        rel_tol=1e-15)
    h = 6.0e-4
    assert math.isclose(contact_stiffness(K, K, h),
                        18.0 * K / (math.pi * h**5), rel_tol=1e-15)


# ---------------------------------------------------------------------------
# geometry
# ---------------------------------------------------------------------------


def test_geometry_queries():
    g = Geometry("circle", [0.001, 0.0, 0.0, 0.0])
    assert g.name == "circle"
    assert np.allclose(g.center, [0.0, 0.0, 0.0])
    assert math.isclose(g.volume, math.pi * 0.001**2, rel_tol=1e-9)
    assert math.isclose(g.bounding_radius, 0.001, rel_tol=1e-12)
    lo, hi = g.box
    assert np.allclose(lo, [-0.001, -0.001, 0.0])
    assert np.allclose(hi, [0.001, 0.001, 0.0])
    assert g.is_inside([0.0, 0.0, 0.0])
    assert not g.is_inside([0.01, 0.0, 0.0])
    assert json.loads(g.to_json()) == {
        "Type": "circle", "Parameters": [0.001, 0.0, 0.0, 0.0]}


def test_geometry_complex_centroid():
    # Annulus with an added bar: the centroid is not the outer circle's centre.
    params = [0.021, 0.0, 0.0, 0.0,
              0.02, 0.0, 0.0, 0.0,
              0.014, -0.0015, 0.0, 0.02, 0.0015, 0.0]
    g = Geometry("complex", params,
                 vec_type=["circle", "circle", "rectangle"],
                 vec_flag=["plus", "minus", "plus"])
    a_out = math.pi * 0.021**2
    a_in = math.pi * 0.02**2
    a_rect = abs(0.02 - 0.014) * abs(0.0015 - -0.0015)
    cx = a_rect * 0.5 * (0.014 + 0.02) / (a_out - a_in + a_rect)
    assert math.isclose(g.center[0], cx, rel_tol=1e-9)
    assert json.loads(g.to_json())["Vec_flag"] == ["plus", "minus", "plus"]


def test_geometry_bad_name():
    try:
        Geometry("not_a_shape", [1.0])
    except Exception:
        pass
    else:
        raise AssertionError("expected a failure for an unknown geometry")


def test_acceptable_geometries():
    names = peridem.acceptable_geometries()
    assert "circle" in names and "complex" in names


# ---------------------------------------------------------------------------
# deck building
# ---------------------------------------------------------------------------


def _two_grain_deck(tmp_path, **kw) -> Deck:
    horizon = 6.0e-4
    d = Deck(dim=2, t_final=1.0e-5, n_steps=10, **kw)
    d.set_output(tmp_path / "out", tags=["Displacement"], interval=10)
    d.set_gravity(0.0, -10.0)
    g = d.add_particle_type(Geometry("circle", [0.001, 0.0, 0.0, 0.0]),
                            MeshSpec(size=2.0e-4))
    m = d.add_material(horizon=horizon, density=1200.0, K=2.16e7, nu=0.25,
                       Gc=50.0, influence_fn_type=1)
    d.add_contact_pair(0, 0, K=2.16e7, horizon=horizon)
    d.place(0.0, 0.0, geometry=g, material=m)
    d.place(0.0, 0.0035, geometry=g, material=m)
    return d


def test_deck_shape(tmp_path):
    d = _two_grain_deck(tmp_path)
    j = d.to_dict()
    assert j["Model"]["Dimension"] == 2
    assert j["Particle"]["Sets"] == 1
    assert j["Mesh"]["Sets"] == 1
    assert j["Material"]["Sets"] == 1
    assert j["Contact"]["Sets"] == 1
    assert j["Particle_Generation"]["Data"]["N"] == 2
    assert j["Output"]["Path"].endswith("/")
    assert j["Force_BC"]["Gravity"] == [0.0, -10.0, 0.0]
    # No .msh requested, so nothing is written to disk.
    assert "File" not in j["Mesh"]["Set_1"]
    assert j["Mesh"]["Set_1"]["CreateMesh"]["Write_Mesh_File"] is False


def test_deck_is_reusable(tmp_path):
    d = _two_grain_deck(tmp_path)
    assert d.to_dict() == d.to_dict()


def test_deck_rejects_missing_contact_pair(tmp_path):
    d = _two_grain_deck(tmp_path)
    d.place(0.0, 0.007, geometry=0, material=0, contact=1)
    try:
        d.to_dict()
    except ValueError as e:
        assert "Set_1_2" in str(e)
    else:
        raise AssertionError("expected a missing contact pair error")


def test_deck_rejects_empty(tmp_path):
    d = Deck(dim=2, t_final=1.0, n_steps=10)
    try:
        d.to_dict()
    except ValueError as e:
        assert "particle types" in str(e)
    else:
        raise AssertionError("expected an empty-deck error")


def test_deck_material_needs_elastic_constants():
    d = Deck(dim=2)
    try:
        d.add_material(horizon=1.0e-4, density=1.0)
    except ValueError as e:
        assert "K" in str(e)
    else:
        raise AssertionError("expected a missing-modulus error")


def test_single_particle_deck_has_no_contact(tmp_path):
    d = Deck(dim=2, t_final=1.0e-4, n_steps=10,
             particle_sim_type="Single_Particle")
    d.set_output(tmp_path / "out", tags=["Displacement"], interval=10)
    d.add_particle_type(Geometry("rectangle", [0, 0, 0, 0.01, 0.01, 0]),
                        MeshSpec(size=1.0e-3, info="uniform"))
    d.add_material(horizon=3.2e-4, density=1200.0, K=216000.0, nu=0.25,
                   Gc=500.0, influence_fn_type=1)
    d.set_test("test_peridynamics")
    j = d.to_dict()
    assert "Contact" not in j and "Particle_Generation" not in j
    assert j["Test"] == {"Test_Name": "test_peridynamics"}


def test_deck_write_and_reload(tmp_path):
    d = _two_grain_deck(tmp_path)
    p = d.write(tmp_path / "deck/input.json")
    assert json.loads(p.read_text()) == d.to_dict()


def test_deck_region_bc_is_an_object():
    d = Deck(dim=2)
    region = Geometry("rectangle", [0, 0, 0, 1e-3, 1e-3, 0])
    d.add_displacement_bc(region=region, direction=[1, 2],
                          time_fn_type="linear", time_fn_params=[0.5],
                          spatial_fn_type="constant")
    s = d.displacement_bc[0]
    assert s["Region"]["Geometry"]["Type"] == "rectangle"
    assert s["Time_Function"] == {"Type": "linear", "Parameters": [0.5]}
    assert s["Spatial_Function"] == {"Type": "constant"}


def test_deck_rejects_unknown_bc_argument():
    d = Deck(dim=2)
    try:
        d.add_displacement_bc(direction=[1], typo=True)
    except TypeError as e:
        assert "typo" in str(e)
    else:
        raise AssertionError("expected a TypeError for the unknown argument")


# ---------------------------------------------------------------------------
# running and reading state
# ---------------------------------------------------------------------------


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
    sim.step()
    assert sim.step_index == 1
    assert sim.time > 0.0


def test_fields_are_writable_views():
    """Node fields are views into the model, not copies: writes land."""
    peridem.init(n_threads=1)
    sim = peridem.Simulation.from_file(str(_CIRCLE_QUICK))
    sim.setup()
    v = sim.velocity
    v[0, 0] = 1.25
    assert sim.velocity[0, 0] == 1.25
    # ... so take a copy when you want a snapshot.
    snap = sim.velocity.copy()
    v[0, 0] = -3.0
    assert snap[0, 0] == 1.25


def test_particle_views(tmp_path):
    peridem.init(n_threads=1)
    sim = _two_grain_deck(tmp_path).simulation(workdir=tmp_path)
    sim.setup()
    assert sim.n_particles == 2
    assert sim.n_walls == 0
    assert sim.n_particles_all == 2
    p0, p1 = sim.particles
    assert p0.id == 0 and p1.id == 1
    assert not p0.is_wall
    assert p0.geometry_name == "circle"
    assert p0.n_nodes + p1.n_nodes == sim.n_nodes
    assert p1.node_start == p0.n_nodes
    assert p0.reference.shape == (p0.n_nodes, 3)
    assert math.isclose(p0.horizon, 6.0e-4, rel_tol=1e-12)
    assert math.isclose(p0.density, 1200.0, rel_tol=1e-12)
    # The second grain sits 3.5 mm above the first.
    assert p1.center_of_mass[1] - p0.center_of_mass[1] > 0.003
    # Per-particle views are slices of the global arrays.
    p0.velocity[0, 1] = -7.0
    assert sim.velocity[p0.node_start, 1] == -7.0


def test_particle_index_out_of_range(tmp_path):
    peridem.init(n_threads=1)
    sim = _two_grain_deck(tmp_path).simulation(workdir=tmp_path)
    sim.setup()
    try:
        sim.particle(5)
    except IndexError:
        pass
    else:
        raise AssertionError("expected IndexError")


def test_bonds_need_setup(tmp_path):
    peridem.init(n_threads=1)
    sim = _two_grain_deck(tmp_path).simulation(workdir=tmp_path)
    try:
        sim.pd_neighbors(0)
    except RuntimeError as e:
        assert "setup()" in str(e)
    else:
        raise AssertionError("expected a not-initialised error")


def test_bond_breaking(tmp_path):
    peridem.init(n_threads=1)
    sim = _two_grain_deck(tmp_path).simulation(workdir=tmp_path)
    sim.setup()
    n = sim.n_pd_neighbors(0)
    assert n > 0
    assert len(sim.pd_neighbors(0)) == n
    assert not sim.bond_broken(0, 0)
    sim.set_bond_broken(0, 0, True)
    assert sim.bond_broken(0, 0)
    sim.set_bond_broken(0, 0, False)
    assert not sim.bond_broken(0, 0)
    # A cut through the middle of grain 0 must break some bonds.
    cut = sim.break_bonds_crossing_vertical([0.0], -1.0, 1.0, 0)
    assert cut > 0


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


def test_deck_runs_end_to_end(tmp_path):
    """A deck built entirely in Python runs and produces finite fields."""
    peridem.init(n_threads=1)
    d = _two_grain_deck(tmp_path)
    sim = d.run(workdir=tmp_path)
    assert sim.step_index == 10
    assert np.isfinite(sim.displacement).all()
    assert np.isfinite(sim.velocity).all()
    assert peridem.last_vtu(tmp_path / "out").is_file()
