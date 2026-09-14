#!/usr/bin/env python3
"""Prove that required player resources fail closed when removed.

This is a bounded package-level probe.  It extracts the candidate archive into
one task-owned temporary directory, removes one required resource category at
a time, and invokes the real package smoke checker against the altered copy.
The original archive and repository are never modified.
"""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile


def load_package_smoke():
    path = Path(__file__).with_name("package_smoke.py")
    spec = importlib.util.spec_from_file_location("writeover_package_smoke", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load package smoke helper: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def fail(message: str) -> "NoReturn":
    print(f"PACKAGE NEGATIVE PROBE ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def write_zip(package_root: Path, archive: Path) -> None:
    with zipfile.ZipFile(
        archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
    ) as output:
        for path in sorted(package_root.rglob("*")):
            if not path.is_file():
                continue
            relative = path.relative_to(package_root).as_posix()
            info = zipfile.ZipInfo(f"{package_root.name}/{relative}")
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            output.writestr(info, path.read_bytes())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=("windows-x64",), required=True)
    parser.add_argument("--archive", required=True, type=Path)
    args = parser.parse_args()

    archive = args.archive.expanduser().absolute()
    if not archive.is_file():
        fail(f"archive is missing: {archive}")

    smoke = load_package_smoke()
    with tempfile.TemporaryDirectory(prefix="writeover-07-package-negative-") as raw:
        workspace = Path(raw)
        extracted = workspace / "extracted"
        extracted.mkdir()
        smoke.extract(archive, extracted)
        source_root = smoke.package_root(extracted)

        probes = (
            ("missing_content", Path("data"), "missing package data directory"),
            (
                "missing_character_art",
                Path("data/characters/b1_character_art.txt"),
                "missing required player runtime resources",
            ),
            (
                "missing_text",
                Path("data/text/recovery_text.txt"),
                "missing required player runtime resources",
            ),
            ("missing_chinese_text", Path("data/text/interface.zh-CN.txt"),
             "missing required player runtime resources"),
            ("missing_english_guide", Path("PLAYER_GUIDE.en.md"),
             "missing package metadata"),
        )
        for name, relative, expected in probes:
            variant = workspace / name / source_root.name
            shutil.copytree(source_root, variant)
            target = variant / relative
            if target.is_dir():
                shutil.rmtree(target)
            elif target.is_file():
                target.unlink()
            else:
                fail(f"probe target was not present: {relative}")

            altered_archive = workspace / f"{name}.zip"
            write_zip(variant, altered_archive)
            result = subprocess.run(
                [
                    sys.executable,
                    str(Path(__file__).with_name("package_smoke.py")),
                    "--platform",
                    args.platform,
                    "--archive",
                    str(altered_archive),
                ],
                cwd=workspace,
                capture_output=True,
                text=True,
                check=False,
            )
            output = result.stdout + result.stderr
            if result.returncode == 0:
                fail(f"{name} unexpectedly passed package smoke")
            if expected not in output:
                fail(
                    f"{name} failed without the expected diagnostic {expected!r}: "
                    f"{output[-2000:]}"
                )
            print(f"PACKAGE_NEGATIVE_{name.upper()}=PASS")

    print("PACKAGE_NEGATIVE_PROBE=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
