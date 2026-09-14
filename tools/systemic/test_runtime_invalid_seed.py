#!/usr/bin/env python3
"""Integration test: an invalid systemic seed must prevent app startup."""
import pathlib
import argparse
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]

CANDIDATES = [
    ROOT / "out/build/debug/Debug/writeover_app.exe",
    ROOT / "out/build/release/Release/writeover_app.exe",
    ROOT / "out/build/ci/Debug/writeover_app.exe",
]


def find_app():
    for p in CANDIDATES:
        if p.exists():
            return p
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=pathlib.Path)
    args = parser.parse_args()
    app = args.executable.resolve() if args.executable else find_app()
    if app is None:
        print("FAIL: writeover_app.exe not built")
        return 1

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = pathlib.Path(tmp)
        # Keep every other startup prerequisite valid. Missing locale files
        # must not mask the systemic-seed rejection this probe is testing.
        data_root = tmp_path / "data"
        shutil.copytree(ROOT / "data", data_root)
        systemic_dir = data_root / "systemic"
        (systemic_dir / "systemic_seed.bin").write_bytes(b"NOTAVALIDSEED")
        cp = subprocess.run(
            [str(app), "--smoke", "--data-dir", str(data_root),
             "--user-data-dir", str(tmp_path / "user")],
            capture_output=True,
            text=True,
            timeout=30,
        )
        if cp.returncode == 0:
            print("FAIL: invalid seed did not stop startup")
            return 1
        if "systemic seed startup failed" not in cp.stderr:
            print(f"FAIL: unexpected error output: {cp.stderr[:200]}")
            return 1
        print("PASS: invalid systemic seed prevented startup")
        return 0


if __name__ == "__main__":
    sys.exit(main())
