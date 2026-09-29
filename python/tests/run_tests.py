#!/usr/bin/env python3
# -------------------------------------------
# Copyright (c) 2021 - 2026 Prashant K. Jha
# -------------------------------------------
# PeriDEM https://github.com/prashjha/PeriDEM
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE)

"""Run the peridem test modules with or without pytest.

pytest is not a dependency of PeriDEM, so ctest goes through here. If pytest is
installed it is used; otherwise the ``test_*`` functions are called directly,
with a ``tmp_path`` supplied to those that ask for one.

    python3 python/tests/run_tests.py                 # everything
    python3 python/tests/run_tests.py test_api.py     # one module
    python3 python/tests/run_tests.py -k geometry     # matching names
"""

from __future__ import annotations

import argparse
import importlib.util
import inspect
import shutil
import sys
import tempfile
import traceback
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]

DEFAULT_MODULES = ["test_api.py", "test_vtk_io.py"]


def _ensure_peridem() -> None:
    try:
        import peridem  # noqa: F401

        return
    except ImportError:
        pass
    for rel in ("build/linux/python", "build/python"):
        if (REPO / rel / "peridem").is_dir():
            sys.path.insert(0, str(REPO / rel))
            break
    sys.path.insert(0, str(REPO / "python"))


def load(path: Path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load {path}")
    mod = importlib.util.module_from_spec(spec)
    sys.modules[path.stem] = mod
    spec.loader.exec_module(mod)
    return mod


def run_module(path: Path, select: list[str]) -> tuple[int, int, list[str]]:
    mod = load(path)
    passed = failed = 0
    failures: list[str] = []
    for name, fn in sorted(vars(mod).items()):
        if not name.startswith("test_") or not inspect.isfunction(fn):
            continue
        if fn.__module__ != mod.__name__:
            continue
        if select and not any(s in name for s in select):
            continue
        params = inspect.signature(fn).parameters
        tmp = Path(tempfile.mkdtemp(prefix="peridem_test_"))
        try:
            fn(tmp) if "tmp_path" in params else fn()
            passed += 1
            print(f"  ok    {path.stem}::{name}", flush=True)
        except Exception:  # noqa: BLE001 - report every failure, keep going
            failed += 1
            failures.append(f"{path.stem}::{name}\n" +
                            textwrap_indent(traceback.format_exc()))
            print(f"  FAIL  {path.stem}::{name}", flush=True)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
    return passed, failed, failures


def textwrap_indent(text: str) -> str:
    return "\n".join("    " + line for line in text.rstrip().splitlines())


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("modules", nargs="*", default=None,
                   help="test modules (default: the unit-test modules)")
    p.add_argument("-k", "--select", action="append", default=[],
                   help="only tests whose name contains this")
    p.add_argument("--no-pytest", action="store_true",
                   help="use the built-in runner even if pytest is installed")
    args = p.parse_args(argv)

    _ensure_peridem()
    paths = [HERE / m if not Path(m).is_absolute() else Path(m)
             for m in (args.modules or DEFAULT_MODULES)]
    missing = [p_ for p_ in paths if not p_.is_file()]
    if missing:
        print("no such test module: " + ", ".join(map(str, missing)),
              file=sys.stderr)
        return 2

    if not args.no_pytest:
        try:
            import pytest  # noqa: F401

            extra = []
            for s in args.select:
                extra += ["-k", s]
            return int(pytest.main(["-q", *map(str, paths), *extra]))
        except ImportError:
            pass

    total_pass = total_fail = 0
    all_failures: list[str] = []
    for path in paths:
        print(f"{path.name}:", flush=True)
        ok, bad, failures = run_module(path, args.select)
        total_pass += ok
        total_fail += bad
        all_failures += failures

    if all_failures:
        print("\n" + "=" * 70)
        for f in all_failures:
            print(f)
            print("-" * 70)
    print(f"\n{total_pass} passed, {total_fail} failed")

    # Collecting nothing is a failure, not a pass. A module whose tests this
    # runner cannot see, such as one that states them through pytest, would
    # otherwise report success without having run anything.
    if total_pass == 0 and total_fail == 0:
        what = "no test ran"
        if args.select:
            what += f" (nothing matched {', '.join(args.select)})"
        print(f"{what}: {', '.join(p_.name for p_ in paths)}", file=sys.stderr)
        return 2

    return 0 if total_fail == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
