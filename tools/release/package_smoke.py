#!/usr/bin/env python3
"""Extract and run one player archive from a clean directory.

The subprocess working directory is deliberately different from the package
directory.  This proves executable-relative content lookup and user-data
separation rather than merely running a build-tree binary.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tarfile
import tempfile
import zipfile


PLATFORMS = {
    "windows-x64": ("WRITEOVER-07.exe", "zip"),
    "linux-x64": ("WRITEOVER-07", "tar.gz"),
    "macos-arm64": ("WRITEOVER-07.app/Contents/MacOS/WRITEOVER-07", "zip"),
}
FORBIDDEN_NAMES = {
    ".git",
    ".env",
    ".pdb",
    "cmakecache.txt",
    "build.ninja",
    "cachesetup.cmake",
    "writeover_tests",
    "writeover_tests.exe",
    "writeover_bench",
    "writeover_bench.exe",
    "mapc",
    "mapc.exe",
}
PATH_MARKERS = (
    b"D:\\Edge Download",
    b"C:\\Users\\Rain",
    b"/mnt/d/Edge Download",
    b"out/build",
    b"CMakeCache.txt",
)
SECRET_MARKERS = (
    b"BEGIN PRIVATE KEY",
    b"github_pat_",
    b"ghp_",
    b"xoxb-",
    b"api_key=",
    b"password=",
)


def fail(message: str) -> "NoReturn":
    print(f"PACKAGE SMOKE ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def safe_member(name: str) -> None:
    path = Path(name)
    if path.is_absolute() or ".." in path.parts:
        fail(f"archive traversal member: {name}")


def extract(archive: Path, destination: Path) -> None:
    if archive.name.endswith(".zip"):
        with zipfile.ZipFile(archive) as source:
            for member in source.infolist():
                safe_member(member.filename)
            source.extractall(destination)
    elif archive.name.endswith(".tar.gz"):
        with tarfile.open(archive, "r:gz") as source:
            members = source.getmembers()
            for member in members:
                safe_member(member.name)
            source.extractall(destination)
    else:
        fail(f"unsupported archive type: {archive}")


def package_root(extracted: Path) -> Path:
    entries = list(extracted.iterdir())
    directories = [entry for entry in entries if entry.is_dir()]
    if len(directories) != 1 or len(entries) != 1:
        fail("archive must contain exactly one versioned package root")
    root = directories[0]
    if not root.name.startswith("WRITEOVER-07-v"):
        fail(f"unexpected package root: {root.name}")
    return root


def check_inventory(root: Path, platform: str) -> dict:
    entry, archive_type = PLATFORMS[platform]
    entry_path = root / entry
    if not entry_path.is_file():
        fail(f"missing player entry: {entry_path.relative_to(root)}")
    if archive_type == "tar.gz" and not (entry_path.stat().st_mode & stat.S_IXUSR):
        fail("Linux player entry is not executable")
    required = {"README.txt", "THIRD_PARTY_NOTICES.txt", "version.json", "manifest.json", "SHA256SUMS.txt"}
    actual = {path.relative_to(root).as_posix() for path in root.rglob("*") if path.is_file()}
    missing = required - actual
    if missing:
        fail(f"missing package metadata: {sorted(missing)}")
    if not (root / "data").is_dir() and platform != "macos-arm64":
        fail("missing package data directory")
    if platform == "macos-arm64" and not (root / "WRITEOVER-07.app" / "Contents" / "Resources" / "data").is_dir():
        fail("missing app-bundle Resources/data")

    for path in root.rglob("*"):
        name = path.name.lower()
        relative = path.relative_to(root).as_posix().lower()
        if name in FORBIDDEN_NAMES or "/.git" in f"/{relative}" or name.startswith(".env"):
            fail(f"developer artifact in package: {relative}")
        path_parts = relative.split("/")
        if "data" in path_parts[:-1] and relative.endswith(".json"):
            fail(f"authoring JSON leaked into package: {relative}")
        if path.is_file():
            payload = path.read_bytes()
            if any(marker in payload for marker in PATH_MARKERS):
                fail(f"developer path/build marker in package: {relative}")
            if any(marker in payload for marker in SECRET_MARKERS):
                fail(f"possible secret marker in package: {relative}")
    return {"entry": entry, "actual": actual}


def check_metadata(root: Path, platform: str) -> None:
    version = json.loads((root / "version.json").read_text(encoding="utf-8"))
    for key in ("product", "version", "gitCommit", "buildType", "platform", "architecture", "buildTimestamp"):
        if key not in version:
            fail(f"version.json missing {key}")
    if version["product"] != "WRITEOVER-07" or version["platform"] != platform:
        fail("version.json product/platform mismatch")
    if version["buildType"] != "Release":
        fail("player package is not marked Release")
    if len(version["gitCommit"]) != 40 or any(c not in "0123456789abcdef" for c in version["gitCommit"].lower()):
        fail("version.json gitCommit is not a full SHA")

    manifest = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
    records = {record["path"]: record for record in manifest.get("files", [])}
    actual = {
        path.relative_to(root).as_posix()
        for path in root.rglob("*")
        if path.is_file() and path.name not in {"manifest.json", "SHA256SUMS.txt"}
    }
    if set(records) != actual:
        fail(f"manifest file set mismatch: records={sorted(records)} actual={sorted(actual)}")
    for relative, record in records.items():
        path = root / Path(relative)
        if path.stat().st_size != record["size"] or digest(path) != record["sha256"]:
            fail(f"manifest hash/size mismatch: {relative}")

    checksum_lines = (root / "SHA256SUMS.txt").read_text(encoding="utf-8").splitlines()
    checksums = {}
    for line in checksum_lines:
        if not line.strip():
            continue
        parts = line.split("  ", 1)
        if len(parts) != 2:
            fail(f"invalid checksum line: {line}")
        checksums[parts[1]] = parts[0]
    expected_checksums = {
        path.relative_to(root).as_posix()
        for path in root.rglob("*")
        if path.is_file() and path.name != "SHA256SUMS.txt"
    }
    if set(checksums) != expected_checksums:
        fail("package SHA256SUMS file set mismatch")
    for relative, expected in checksums.items():
        if digest(root / Path(relative)) != expected:
            fail(f"SHA256SUMS mismatch: {relative}")


def run_smoke(root: Path, platform: str, clean_root: Path) -> None:
    entry = root / PLATFORMS[platform][0]
    if platform in {"linux-x64", "macos-arm64"}:
        # Python's zipfile extractor does not restore POSIX executable bits.
        # The archive still carries the mode in its central directory; restore
        # it here so the clean-package launch tests the extracted player,
        # rather than an extractor implementation detail.
        entry.chmod(entry.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    workdir = clean_root / "unrelated-working-directory"
    user_data = clean_root / "user-data"
    workdir.mkdir()
    user_data.mkdir()
    if entry.suffix == "" or platform == "macos-arm64":
        command_entry = entry
    else:
        command_entry = entry
    command = [
        str(command_entry),
        "--smoke",
        "--frames",
        "61",
        "--width",
        "80",
        "--height",
        "30",
        "--user-data-dir",
        str(user_data),
    ]
    try:
        result = subprocess.run(
            command,
            cwd=workdir,
            capture_output=True,
            timeout=60,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        fail(f"clean-package launch failed: {exc}")
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    if result.returncode != 0:
        fail(f"clean-package smoke exit={result.returncode}\n{output[-4000:]}")
    if "WRITEOVER-07 v" not in output or "writeover_game exit=0" not in output:
        fail("startup identity or clean smoke receipt missing")
    if "Missing game data" in output:
        fail("package could not find executable-relative data")
    save_path = user_data / "saves" / "smoke.wo07"
    if not save_path.is_file() or save_path.stat().st_size == 0:
        fail("package smoke did not write user data outside the package")
    print("PACKAGE_SMOKE=PASS")
    print("PACKAGE_RESOURCE_ROOT=PASS")
    print("PACKAGE_USER_DATA_SEPARATION=PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=sorted(PLATFORMS), required=True)
    parser.add_argument("--archive", required=True, type=Path)
    args = parser.parse_args()
    archive = args.archive.expanduser().absolute()
    if not archive.is_file():
        fail(f"archive is missing: {archive}")
    with tempfile.TemporaryDirectory(prefix="writeover-07-package-smoke-") as temporary:
        clean_root = Path(temporary)
        extracted = clean_root / "extracted"
        extracted.mkdir()
        extract(archive, extracted)
        root = package_root(extracted)
        inventory = check_inventory(root, args.platform)
        check_metadata(root, args.platform)
        print("PACKAGE_SECRET_SCAN=PASS")
        print("PACKAGE_DEV_GARBAGE=0")
        print(f"PACKAGE_ENTRY={inventory['entry']}")
        run_smoke(root, args.platform, clean_root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
