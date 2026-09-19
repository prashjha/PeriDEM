#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Run this example in-process from Python.

``problem.py`` next to this file sets up the problem: geometry, materials,
contact, boundary conditions and particle placement, through the ``peridem``
interface. Run ``./problem.py --help`` for its arguments.

To run the same problem through the C++ executable instead, hand it the deck:

    ./problem.py --write-deck /tmp/input.json
    <build>/bin/PeriDEM -i /tmp/input.json -nThreads 4
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from problem import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
