# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Run a problem through both interfaces and compare the node fields.

Two comparisons are provided.

``compare_decks`` compares a deck assembled by :class:`peridem.Deck` with the
deck a C++ driver writes, key by key. It measures whether the Python setup
reproduces the C++ setup.

``run_pair`` runs one deck through ``bin/PeriDEM`` in a separate process and
through the in-process interface, then compares Displacement, Velocity, Force
and Damage node by node. Each side writes to its own directory and builds its
own mesh unless the deck names a mesh file.

Neither function reads a deck produced by the other side. A deck compared with
itself yields no information.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable, Sequence

from .io import fields_from_vtu, last_vtu, nodal_error

__all__ = [
    "DeckDiff",
    "ParityResult",
    "compare_decks",
    "compare_outputs",
    "find_peridem_binary",
    "repo_root",
    "run_cpp",
    "run_python",
    "run_pair",
]

DEFAULT_FIELDS = ("Displacement", "Velocity", "Force", "Damage_Z", "Damage")


def _is_repo_root(p: Path) -> bool:
    # The staged copy under build/<cfg>/python also has python/peridem, so
    # require the source markers too.
    return ((p / "python" / "peridem" / "__init__.py").is_file()
            and (p / "CMakeLists.txt").is_file()
            and (p / "src").is_dir())


def repo_root(start: str | os.PathLike[str] | None = None) -> Path:
    here = Path(start or __file__).resolve()
    for p in [here, *here.parents]:
        if _is_repo_root(p):
            return p
    raise FileNotFoundError("PeriDEM repo root not found")


def find_peridem_binary(explicit: str | os.PathLike[str] | None = None) -> Path:
    """Locate ``bin/PeriDEM``. Honours ``PERIDEM_BIN``."""
    cands: list[Path] = []
    if explicit:
        cands.append(Path(explicit))
    env = os.environ.get("PERIDEM_BIN")
    if env:
        cands.append(Path(env))
    # When imported from a staged build tree, bin/ is a sibling a level or two up.
    for p in Path(__file__).resolve().parents:
        cands.append(p / "bin" / "PeriDEM")
    try:
        root = repo_root()
    except FileNotFoundError:
        root = None
    if root is not None:
        for build in ("build/linux", "build", "build/Release", "build/Debug"):
            cands.append(root / build / "bin" / "PeriDEM")
    which = shutil.which("PeriDEM")
    if which:
        cands.append(Path(which))
    for c in cands:
        if c.is_file() and os.access(c, os.X_OK):
            return c.resolve()
    raise FileNotFoundError(
        "bin/PeriDEM not found; build it or set PERIDEM_BIN "
        "(cmake --build <build> --target PeriDEM)")


# ---------------------------------------------------------------------------
# Deck comparison
# ---------------------------------------------------------------------------


@dataclass
class DeckDiff:
    """Result of comparing two decks."""

    differences: list[str] = field(default_factory=list)

    @property
    def equal(self) -> bool:
        return not self.differences

    def __bool__(self) -> bool:  # `assert diff` reads wrong; force explicit use
        return self.equal

    def __str__(self) -> str:
        if self.equal:
            return "decks identical"
        lines = [f"{len(self.differences)} deck difference(s):"]
        lines += [f"  {d}" for d in self.differences]
        return "\n".join(lines)


def compare_decks(a: dict[str, Any] | str | os.PathLike[str],
                  b: dict[str, Any] | str | os.PathLike[str],
                  *, ignore: Iterable[str] = (),
                  rtol: float = 0.0) -> DeckDiff:
    """Compare two decks key by key.

    ``ignore`` lists dotted paths to skip (for example ``Output.Path``, which
    necessarily differs between the two runs). ``rtol`` allows a relative
    tolerance on numbers; the default demands exact equality.
    """

    def load(x: Any) -> dict[str, Any]:
        if isinstance(x, dict):
            return x
        return json.loads(Path(x).read_text())

    skip = set(ignore)
    out: list[str] = []

    def num_equal(x: float, y: float) -> bool:
        if x == y:
            return True
        if rtol <= 0.0:
            return False
        scale = max(abs(x), abs(y))
        return abs(x - y) <= rtol * scale

    def walk(x: Any, y: Any, path: str) -> None:
        if path.lstrip(".") in skip:
            return
        if isinstance(x, dict) and isinstance(y, dict):
            for k in sorted(set(x) | set(y)):
                sub = f"{path}.{k}" if path else k
                if k not in x:
                    if sub.lstrip(".") not in skip:
                        out.append(f"{sub}: only in second ({y[k]!r})")
                elif k not in y:
                    if sub.lstrip(".") not in skip:
                        out.append(f"{sub}: only in first ({x[k]!r})")
                else:
                    walk(x[k], y[k], sub)
        elif isinstance(x, list) and isinstance(y, list):
            if len(x) != len(y):
                out.append(f"{path}: length {len(x)} vs {len(y)}")
            else:
                for i, (u, v) in enumerate(zip(x, y)):
                    walk(u, v, f"{path}[{i}]")
        elif isinstance(x, bool) or isinstance(y, bool):
            if x is not y:
                out.append(f"{path}: {x!r} vs {y!r}")
        elif isinstance(x, (int, float)) and isinstance(y, (int, float)):
            if not num_equal(float(x), float(y)):
                out.append(f"{path}: {x!r} vs {y!r}")
        elif x != y:
            out.append(f"{path}: {x!r} vs {y!r}")

    walk(load(a), load(b), "")
    return DeckDiff(out)


# ---------------------------------------------------------------------------
# Running
# ---------------------------------------------------------------------------


def _staged(deck: Any, out_dir: Path, name: str) -> tuple[dict[str, Any], Path]:
    """Copy of ``deck`` with Output.Path redirected to ``out_dir``."""
    d = deck.to_dict() if hasattr(deck, "to_dict") else json.loads(json.dumps(deck))
    out_dir.mkdir(parents=True, exist_ok=True)
    d.setdefault("Output", {})["Path"] = str(out_dir.resolve()) + "/"
    path = out_dir.parent / name
    path.write_text(json.dumps(d, indent=2) + "\n")
    return d, path


def run_cpp(deck: Any, work_dir: str | os.PathLike[str], *, n_threads: int = 1,
            n_ranks: int = 1, binary: str | os.PathLike[str] | None = None,
            timeout: float | None = None) -> Path:
    """Run ``bin/PeriDEM`` on ``deck`` in its own directory. Returns the out dir."""
    work = Path(work_dir).resolve()
    out = work / "out"
    _, deck_path = _staged(deck, out, "input.json")
    exe = find_peridem_binary(binary)
    cmd: list[str] = []
    if n_ranks > 1:
        cmd += [os.environ.get("MPIEXEC") or shutil.which("mpirun") or "mpirun",
                "-n", str(n_ranks)]
    cmd += [str(exe), "-i", str(deck_path), "-nThreads", str(n_threads)]
    proc = subprocess.run(cmd, cwd=work, capture_output=True, text=True,
                          timeout=timeout)
    if proc.returncode != 0:
        tail = "\n".join((proc.stdout + proc.stderr).splitlines()[-25:])
        raise RuntimeError(f"PeriDEM failed ({proc.returncode}):\n{tail}")
    return out


def run_python(deck: Any, work_dir: str | os.PathLike[str], *,
               n_threads: int = 1):
    """Run ``deck`` through the in-process interface. Returns (out_dir, Simulation)."""
    from . import init
    from ._core import Simulation

    work = Path(work_dir).resolve()
    out = work / "out"
    d, _ = _staged(deck, out, "input.json")
    init(n_threads=n_threads)
    sim = Simulation.from_json(json.dumps(d), str(work))
    sim.run()
    return out, sim


@dataclass
class ParityResult:
    cpp_out: Path
    py_out: Path
    cpp_vtu: Path
    py_vtu: Path
    report: dict[str, dict[str, float]]

    @property
    def max_linf(self) -> float:
        return max((v["Linf"] for v in self.report.values()), default=0.0)

    def table(self, label: str = "") -> str:
        head = f"{label}  ({self.cpp_vtu.name} vs {self.py_vtu.name})" if label \
            else f"{self.cpp_vtu.name} vs {self.py_vtu.name}"
        rows = [head,
                f"  {'field':<14}{'nodes':>8}{'Linf':>14}{'rms':>14}"]
        for name, v in self.report.items():
            rows.append(f"  {name:<14}{int(v['n']):>8}{v['Linf']:>14.3e}"
                        f"{v['rms']:>14.3e}")
        return "\n".join(rows)


def compare_outputs(cpp_out: str | os.PathLike[str],
                    py_out: str | os.PathLike[str], *,
                    fields: Sequence[str] = DEFAULT_FIELDS) -> ParityResult:
    cpp_vtu = last_vtu(cpp_out)
    py_vtu = last_vtu(py_out)
    a = fields_from_vtu(cpp_vtu)
    b = fields_from_vtu(py_vtu)
    return ParityResult(Path(cpp_out), Path(py_out), cpp_vtu, py_vtu,
                        nodal_error(a, b, tuple(fields)))


def run_pair(deck: Any, tmp_dir: str | os.PathLike[str], *, n_threads: int = 1,
             n_ranks: int = 1, binary: str | os.PathLike[str] | None = None,
             fields: Sequence[str] = DEFAULT_FIELDS,
             timeout: float | None = None) -> ParityResult:
    """Run ``deck`` both ways under ``tmp_dir`` and compare the last frame.

    The two runs never share a directory, so each builds its own mesh unless the
    deck points at a ``.msh`` on disk.
    """
    root = Path(tmp_dir)
    cpp_out = run_cpp(deck, root / "cpp", n_threads=n_threads, n_ranks=n_ranks,
                      binary=binary, timeout=timeout)
    py_out, _ = run_python(deck, root / "py", n_threads=n_threads)
    return compare_outputs(cpp_out, py_out, fields=fields)
