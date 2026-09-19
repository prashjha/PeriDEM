# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Make ``import peridem`` work from a build tree.

Examples import this first so they run straight from a checkout without
``pip install``. If peridem is already importable (installed, or PYTHONPATH is
set) this does nothing.
"""

from __future__ import annotations

import sys
from pathlib import Path

__all__ = ["REPO_ROOT", "ensure_peridem"]

REPO_ROOT = next(
    p for p in Path(__file__).resolve().parents
    if (p / "python" / "peridem" / "__init__.py").is_file()
)

_BUILD_DIRS = ("build/linux/python", "build/python", "build/Release/python",
               "build/Debug/python")


def ensure_peridem() -> None:
    try:
        import peridem  # noqa: F401

        return
    except ImportError:
        pass
    # The staged package under build/ carries the compiled _core; the source
    # python/ directory does not, so it must never shadow it.
    for rel in _BUILD_DIRS:
        cand = REPO_ROOT / rel
        if (cand / "peridem" / "__init__.py").is_file():
            sys.path.insert(0, str(cand))
            break
    else:
        sys.path.insert(0, str(REPO_ROOT / "python"))
    try:
        import peridem  # noqa: F401
    except ImportError as exc:  # pragma: no cover - build guidance
        raise SystemExit(
            "cannot import peridem. Build the extension first:\n"
            f"  cmake -S {REPO_ROOT} -B {REPO_ROOT}/build -DEnable_Python=ON\n"
            f"  cmake --build {REPO_ROOT}/build --target peridem_core PeriDEM -j\n"
            f"({exc})"
        ) from exc


ensure_peridem()
