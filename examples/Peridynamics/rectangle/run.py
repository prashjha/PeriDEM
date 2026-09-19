#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""In-process Python run. Same DECK / NP / NTHREADS as ./run.sh."""

from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def _root() -> Path:
    for p in [HERE, *HERE.parents]:
        if (p / "python" / "peridem" / "__init__.py").is_file():
            return p
    raise SystemExit("PeriDEM repo root not found")


ROOT = _root()
sys.path.insert(0, str(ROOT / "python"))
for _b in (ROOT / "build/linux/python", ROOT / "build/python"):
    if (_b / "peridem").is_dir():
        sys.path.insert(0, str(_b))
        break

from peridem.__main__ import example_script  # noqa: E402

raise SystemExit(
    example_script(
        HERE,
        default_deck="input_quick.json",
        default_np=1,
        default_nthreads=4,
    )
)
