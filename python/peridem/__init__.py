# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Python interface to PeriDEM."""

from __future__ import annotations

import atexit
import json
from typing import Any

from ._core import Simulation, finalize, init, mpi_rank, mpi_size, version
from .io import fields_from_vtu, last_vtu, nodal_error, read_vtu, snapshot


def _from_dict(cls, deck: dict[str, Any]) -> Simulation:
    return cls.from_json(json.dumps(deck))


Simulation.from_dict = classmethod(_from_dict)
atexit.register(finalize)

__all__ = [
    "Simulation",
    "finalize",
    "init",
    "fields_from_vtu",
    "last_vtu",
    "mpi_rank",
    "mpi_size",
    "nodal_error",
    "read_vtu",
    "snapshot",
    "version",
]
