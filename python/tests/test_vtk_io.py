# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""VTU reading, node matching and snapshot rendering.

The VTU is produced here rather than read from a committed run, so the test
works from a clean checkout.
"""

from pathlib import Path
from tempfile import TemporaryDirectory

import numpy as np
import peridem
from peridem import Deck, Geometry
from peridem.deck import MeshSpec


def _make_vtu(out_dir: Path) -> Path:
    """Run ten steps of a tiny two-grain problem and return its last VTU."""
    peridem.init(n_threads=1)
    d = Deck(dim=2, t_final=1.0e-5, n_steps=10)
    d.set_output(out_dir, interval=5,
                 tags=["Displacement", "Velocity", "Force", "Damage_Z",
                       "Damage", "Particle_ID"])
    d.set_gravity(0.0, -10.0)
    g = d.add_particle_type(Geometry("circle", [0.001, 0.0, 0.0, 0.0]),
                            MeshSpec(size=2.5e-4))
    m = d.add_material(horizon=7.5e-4, density=1200.0, K=2.16e7, nu=0.25,
                       Gc=50.0, influence_fn_type=1)
    d.add_contact_pair(0, 0, K=2.16e7, horizon=7.5e-4)
    d.place(0.0, 0.0, geometry=g, material=m)
    d.place(0.0, 0.0035, geometry=g, material=m)
    d.run(workdir=out_dir.parent)
    return peridem.last_vtu(out_dir)


def test_read_snapshot_and_compare(tmp_path=None):
    ctx = TemporaryDirectory() if tmp_path is None else None
    root = Path(tmp_path) if tmp_path is not None else Path(ctx.name)
    out = root / "out"
    out.mkdir(parents=True, exist_ok=True)
    vtu = _make_vtu(out)

    data = peridem.read_vtu(vtu)
    assert data["n_points"] > 0
    assert data["points"].shape[1] == 3
    assert "Displacement" in data["point_data"]
    u = data["point_data"]["Displacement"]
    assert u.shape[0] == data["n_points"]
    assert np.isfinite(u).all()

    png = peridem.snapshot(data, root / "snap.png", color="|Displacement|")
    assert png.is_file() and png.stat().st_size > 1000

    fields = peridem.fields_from_vtu(vtu)
    assert "x_ref" in fields
    err = peridem.nodal_error(fields, fields)
    for name in ("Displacement", "Velocity", "Force"):
        assert err[name]["Linf"] == 0.0
        assert err[name]["rms"] == 0.0

    if ctx is not None:
        ctx.cleanup()


def test_snapshot_rejects_unknown_field(tmp_path=None):
    ctx = TemporaryDirectory() if tmp_path is None else None
    root = Path(tmp_path) if tmp_path is not None else Path(ctx.name)
    out = root / "out"
    out.mkdir(parents=True, exist_ok=True)
    data = peridem.read_vtu(_make_vtu(out))
    try:
        peridem.snapshot(data, root / "bad.png", color="Not_A_Field")
    except KeyError:
        pass
    else:
        raise AssertionError("expected a KeyError for the unknown field")
    if ctx is not None:
        ctx.cleanup()


if __name__ == "__main__":
    test_read_snapshot_and_compare()
    test_snapshot_rejects_unknown_field()
    print("ok")
