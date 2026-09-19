# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

from pathlib import Path

import numpy as np
import peridem

_VTU = (
    Path(__file__).resolve().parents[2]
    / "examples"
    / "PeriDEM"
    / "ellipse_triangle"
    / "runs"
    / "output_40.vtu"
)


def test_read_and_snapshot(tmp_path=None):
    out = Path(tmp_path) if tmp_path is not None else Path("/tmp/peridem_p01")
    out.mkdir(parents=True, exist_ok=True)
    data = peridem.read_vtu(_VTU)
    assert data["n_points"] > 0
    assert data["points"].shape[1] == 3
    assert "Displacement" in data["point_data"]
    u = data["point_data"]["Displacement"]
    assert u.shape[0] == data["n_points"]
    assert np.isfinite(u).all()
    png = peridem.snapshot(data, out / "ellipse_triangle_40.png", color="|Displacement|")
    assert png.is_file() and png.stat().st_size > 1000
    print("ok", data["n_points"], png, png.stat().st_size)
    fields = peridem.fields_from_vtu(_VTU)
    err = peridem.nodal_error(fields, fields)
    assert err["Displacement"]["Linf"] == 0.0


if __name__ == "__main__":
    test_read_and_snapshot()
