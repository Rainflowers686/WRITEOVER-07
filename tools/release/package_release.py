#!/usr/bin/env python3
"""Build one deterministic player staging tree and its archive.

This is deliberately stdlib-only.  The C++ build remains responsible for the
binary; this tool only copies the shipping runtime files, writes release
metadata, and archives the same staging tree that future Steam depots will use.
"""

from __future__ import annotations

import argparse
import datetime as dt
import gzip
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tarfile
import zipfile


PLATFORMS = {
    "windows-x64": {
        "suffix": "win-x64",
        "arch": "x64",
        "entry": "WRITEOVER-07.exe",
        "archive": "zip",
        "start": "Double-click WRITEOVER-07.exe.",
        "save_dir": "%LOCALAPPDATA%\\WRITEOVER-07\\saves\\",
    },
    "linux-x64": {
        "suffix": "linux-x64",
        "arch": "x64",
        "entry": "WRITEOVER-07",
        "archive": "tar.gz",
        "start": "Run ./WRITEOVER-07 from a terminal (the executable bit is included).",
        "save_dir": "$XDG_DATA_HOME/WRITEOVER-07/saves/ (fallback: ~/.local/share/WRITEOVER-07/saves/)",
    },
    "macos-arm64": {
        "suffix": "macos-arm64",
        "arch": "arm64",
        "entry": "WRITEOVER-07.app",
        "archive": "zip",
        "start": "Double-click WRITEOVER-07.app.",
        "save_dir": "~/Library/Application Support/WRITEOVER-07/saves/",
    },
}

RELEASE_FILES = {"manifest.json", "SHA256SUMS.txt"}


def fail(message: str) -> "NoReturn":
    print(f"PACKAGE ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def absolute(path: Path) -> Path:
    return path.expanduser().absolute()


def read_version(source_root: Path, requested: str | None) -> str:
    version_file = source_root / "VERSION"
    if not version_file.is_file():
        fail(f"missing version source of truth: {version_file}")
    version = version_file.read_text(encoding="utf-8").strip()
    if not version or any(ch.isspace() for ch in version):
        fail("VERSION must contain exactly one non-empty line")
    if requested is not None and requested != version:
        fail(f"requested version {requested!r} does not match VERSION {version!r}")
    return version


def git_value(source_root: Path, fmt: str, fallback: str) -> str:
    try:
        return subprocess.check_output(
            ["git", "-C", str(source_root), "show", "-s", f"--format={fmt}", "HEAD"],
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip() or fallback
    except (OSError, subprocess.CalledProcessError):
        return fallback


def epoch_from_iso(value: str) -> int:
    try:
        return int(dt.datetime.fromisoformat(value.replace("Z", "+00:00")).timestamp())
    except ValueError:
        return 0


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def files_under(root: Path, exclude: set[str] | None = None) -> list[Path]:
    excluded = exclude or set()
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.relative_to(root).as_posix() not in excluded
    )


def safe_stage_path(dist_root: Path, platform: str) -> Path:
    stage = absolute(dist_root / "stage" / platform)
    dist = absolute(dist_root)
    if stage == dist or dist not in stage.parents:
        fail(f"stage path escaped dist root: {stage}")
    return stage


def copy_runtime_data(source_root: Path, stage_root: Path) -> None:
    source_data = source_root / "data"
    if not source_data.is_dir():
        fail(f"missing runtime data directory: {source_data}")
    compiled = [
        path
        for path in sorted(source_data.rglob("*"))
        if path.is_file() and path.suffix.lower() in {".bin", ".woc"}
    ]
    if not compiled:
        fail("runtime data has no compiled .bin or .woc files")
    for source in compiled:
        destination = stage_root / "data" / source.relative_to(source_data)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def write_player_readme(source_root: Path, stage_root: Path, info: dict[str, str], version: str) -> None:
    template_path = source_root / "docs" / "release" / "PLAYER_README.txt"
    if not template_path.is_file():
        fail(f"missing player README template: {template_path}")
    text = template_path.read_text(encoding="utf-8")
    replacements = {
        "@VERSION@": version,
        "@PLATFORM@": info["platform_label"],
        "@START@": info["start"],
        "@ENTRY@": info["entry"],
        "@SAVE_DIR@": info["save_dir"],
        "@SETTINGS_DIR@": info["settings_dir"],
    }
    for key, value in replacements.items():
        text = text.replace(key, value)
    (stage_root / "README.txt").write_text(text, encoding="utf-8", newline="\n")


def write_info_plist(app_root: Path, version: str) -> None:
    base_version = version.split("-", 1)[0]
    plist = f'''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>WRITEOVER-07</string>
    <key>CFBundleDisplayName</key>
    <string>WRITEOVER-07</string>
    <key>CFBundleIdentifier</key>
    <string>com.rainflowers.writeover07</string>
    <key>CFBundleVersion</key>
    <string>{base_version}</string>
    <key>CFBundleShortVersionString</key>
    <string>{base_version}</string>
    <key>CFBundleExecutable</key>
    <string>WRITEOVER-07</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
</dict>
</plist>
'''
    (app_root / "Contents" / "Info.plist").write_text(
        plist, encoding="utf-8", newline="\n"
    )


def make_manifest(stage_root: Path, version: str, commit: str, platform: str, info: dict[str, str]) -> None:
    records = []
    for path in files_under(stage_root, RELEASE_FILES):
        relative = path.relative_to(stage_root).as_posix()
        records.append({"path": relative, "size": path.stat().st_size, "sha256": sha256(path)})
    manifest = {
        "manifestVersion": 1,
        "product": "WRITEOVER-07",
        "version": version,
        "gitCommit": commit,
        "buildType": "Release",
        "platform": platform,
        "architecture": info["arch"],
        "files": records,
    }
    (stage_root / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )


def make_checksums(stage_root: Path) -> None:
    lines = []
    for path in files_under(stage_root, {"SHA256SUMS.txt"}):
        lines.append(f"{sha256(path)}  {path.relative_to(stage_root).as_posix()}")
    (stage_root / "SHA256SUMS.txt").write_text(
        "\n".join(lines) + "\n", encoding="utf-8", newline="\n"
    )


def fixed_datetime(epoch: int) -> tuple[int, int, int, int, int, int]:
    minimum = 315532800  # 1980-01-01, required by the ZIP format.
    stamp = dt.datetime.fromtimestamp(max(epoch, minimum), tz=dt.timezone.utc)
    return stamp.year, stamp.month, stamp.day, stamp.hour, stamp.minute, stamp.second


def make_zip(stage_root: Path, archive: Path, package_name: str, epoch: int) -> None:
    archive.parent.mkdir(parents=True, exist_ok=True)
    date_time = fixed_datetime(epoch)
    with zipfile.ZipFile(
        archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
    ) as output:
        for path in files_under(stage_root):
            relative = path.relative_to(stage_root).as_posix()
            info = zipfile.ZipInfo(f"{package_name}/{relative}", date_time=date_time)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            mode = stat.S_IMODE(path.stat().st_mode)
            info.external_attr = (mode & 0xFFFF) << 16
            output.writestr(info, path.read_bytes())


def make_tar_gz(stage_root: Path, archive: Path, package_name: str, epoch: int) -> None:
    archive.parent.mkdir(parents=True, exist_ok=True)
    tar_bytes = io.BytesIO()
    with tarfile.open(fileobj=tar_bytes, mode="w") as output:
        for path in files_under(stage_root):
            relative = path.relative_to(stage_root).as_posix()
            data = path.read_bytes()
            entry = tarfile.TarInfo(f"{package_name}/{relative}")
            entry.size = len(data)
            entry.mtime = epoch
            entry.mode = stat.S_IMODE(path.stat().st_mode)
            entry.uid = 0
            entry.gid = 0
            entry.uname = ""
            entry.gname = ""
            output.addfile(entry, io.BytesIO(data))
    with archive.open("wb") as raw:
        with gzip.GzipFile(fileobj=raw, mode="wb", filename="", mtime=epoch) as compressed:
            compressed.write(tar_bytes.getvalue())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=sorted(PLATFORMS), required=True)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--dist-root", type=Path, default=Path("dist"))
    parser.add_argument("--archive", type=Path)
    parser.add_argument("--version")
    parser.add_argument("--commit")
    parser.add_argument("--ci-run", default=os.environ.get("GITHUB_RUN_ID", ""))
    parser.add_argument("--build-timestamp")
    args = parser.parse_args()

    source_root = absolute(args.source_root)
    info = dict(PLATFORMS[args.platform])
    info["platform_label"] = args.platform
    if args.platform == "windows-x64":
        info["settings_dir"] = "%LOCALAPPDATA%\\WRITEOVER-07\\settings.cfg"
    elif args.platform == "linux-x64":
        info["settings_dir"] = "$XDG_DATA_HOME/WRITEOVER-07/settings.cfg (fallback: ~/.local/share/WRITEOVER-07/settings.cfg)"
    else:
        info["settings_dir"] = "~/Library/Application Support/WRITEOVER-07/settings.cfg"

    version = read_version(source_root, args.version)
    binary = absolute(args.binary if args.binary.is_absolute() else source_root / args.binary)
    if not binary.is_file():
        fail(f"release binary is missing: {binary}")
    commit = args.commit or git_value(source_root, "%H", "unknown")
    timestamp = args.build_timestamp or git_value(source_root, "%cI", "")
    if not timestamp:
        timestamp = dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    epoch = epoch_from_iso(timestamp) or int(git_value(source_root, "%ct", "0") or 0)
    if epoch <= 0:
        epoch = int(dt.datetime.now(dt.timezone.utc).timestamp())

    dist_root = absolute(args.dist_root if args.dist_root.is_absolute() else source_root / args.dist_root)
    stage_root = safe_stage_path(dist_root, args.platform)
    package_name = f"WRITEOVER-07-v{version}-{info['suffix']}"
    archive_suffix = ".zip" if info["archive"] == "zip" else ".tar.gz"
    archive = args.archive or (dist_root / f"{package_name}{archive_suffix}")
    archive = absolute(archive if archive.is_absolute() else source_root / archive)
    if dist_root not in archive.parents:
        fail(f"archive path must stay below dist root: {archive}")

    if stage_root.exists():
        shutil.rmtree(stage_root)
    if archive.exists():
        archive.unlink()
    stage_root.mkdir(parents=True, exist_ok=True)

    if args.platform == "macos-arm64":
        app_binary = stage_root / "WRITEOVER-07.app" / "Contents" / "MacOS" / "WRITEOVER-07"
        app_binary.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(binary, app_binary)
        app_binary.chmod(app_binary.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        write_info_plist(stage_root / "WRITEOVER-07.app", version)
        copy_runtime_data(source_root, stage_root / "WRITEOVER-07.app" / "Contents" / "Resources")
    else:
        entry = stage_root / info["entry"]
        shutil.copy2(binary, entry)
        if args.platform == "linux-x64":
            entry.chmod(entry.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        copy_runtime_data(source_root, stage_root)

    notice = source_root / "docs" / "release" / "THIRD_PARTY_NOTICES.txt"
    if not notice.is_file():
        fail(f"missing third-party notice: {notice}")
    shutil.copy2(notice, stage_root / "THIRD_PARTY_NOTICES.txt")
    write_player_readme(source_root, stage_root, info, version)
    version_json = {
        "product": "WRITEOVER-07",
        "version": version,
        "gitCommit": commit,
        "buildType": "Release",
        "platform": args.platform,
        "architecture": info["arch"],
        "buildTimestamp": timestamp,
    }
    if args.ci_run:
        version_json["ciRun"] = args.ci_run
    (stage_root / "version.json").write_text(
        json.dumps(version_json, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    make_manifest(stage_root, version, commit, args.platform, info)
    make_checksums(stage_root)

    if info["archive"] == "zip":
        make_zip(stage_root, archive, package_name, epoch)
    else:
        make_tar_gz(stage_root, archive, package_name, epoch)

    print(f"PACKAGE_PLATFORM={args.platform}")
    print(f"PACKAGE_VERSION={version}")
    print(f"PACKAGE_COMMIT={commit}")
    print(f"PACKAGE_STAGE={stage_root}")
    print(f"PACKAGE_ARCHIVE={archive}")
    print(f"PACKAGE_ARCHIVE_SIZE={archive.stat().st_size}")
    print("PACKAGE_FILESET_DETERMINISTIC=YES")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
