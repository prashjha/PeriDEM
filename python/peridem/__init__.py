# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Python interface to PeriDEM.

Set a problem up, run it, and read the fields back without leaving Python::

    import peridem
    from peridem import Deck, Geometry

    peridem.init(n_threads=4)

    d = Deck(dim=2, t_final=1.0e-4, n_steps=2000)
    d.set_output("runs/out", tags=["Displacement", "Damage_Z"])
    ...
    sim = d.run()
    u = sim.displacement          # numpy (N, 3) view into the model

``Simulation.from_file`` runs an existing JSON deck, which is what the
comparison against ``bin/PeriDEM`` uses.
"""

from __future__ import annotations

import atexit
import json
import os
from typing import Any

from ._core import (
    Geometry,
    Particle,
    Simulation,
    acceptable_geometries,
    decks,
    finalize,
    harmonic_mean,
    init,
    mpi_rank,
    mpi_size,
    to_E,
    to_Gc,
    to_K,
    to_KIc,
    version,
)
from .deck import Deck, MeshSpec, contact_stiffness, example_geometry, to_G
from .io import fields_from_vtu, last_vtu, nodal_error, read_vtu, snapshot


def _from_dict(cls, deck: dict[str, Any], workdir: str | os.PathLike[str] | None = None):
    """Build a Simulation from a deck already assembled as a dict."""
    return cls.from_json(json.dumps(deck), "" if workdir is None else str(workdir))


Simulation.from_dict = classmethod(_from_dict)
atexit.register(finalize)

__all__ = [
    "Deck",
    "Geometry",
    "MeshSpec",
    "Particle",
    "Simulation",
    "acceptable_geometries",
    "contact_stiffness",
    "decks",
    "example_geometry",
    "fields_from_vtu",
    "finalize",
    "harmonic_mean",
    "init",
    "last_vtu",
    "mpi_rank",
    "mpi_size",
    "nodal_error",
    "read_vtu",
    "snapshot",
    "to_E",
    "to_G",
    "to_Gc",
    "to_K",
    "to_KIc",
    "version",
]
