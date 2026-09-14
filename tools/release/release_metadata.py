#!/usr/bin/env python3
"""Resolve release names from PRODUCT_VERSION; never publish or create tags."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from package_release import PLATFORMS

ROOT = Path(__file__).resolve().parents[2]


def metadata(root: Path, requested_tag: str | None = None) -> dict[str, str]:
    version = (root / "PRODUCT_VERSION").read_text(encoding="utf-8").strip()
    if not re.fullmatch(r"(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-[A-Za-z0-9]+(?:[.-][A-Za-z0-9]+)*)?", version):
        raise ValueError("PRODUCT_VERSION is not a supported semantic version")
    tag = "v" + version
    if requested_tag is not None and requested_tag != tag:
        raise ValueError(f"Tag mismatch: expected {tag}, got {requested_tag}")
    notes = f"docs/release/RELEASE_NOTES_v{version}.md"
    if not (root / notes).is_file():
        raise ValueError(f"Missing release notes: {notes}")
    result = {
        "version": version, "tag": tag, "title": f"WRITEOVER-07 {version} — Candidate",
        "notes": notes, "prerelease": "true",
    }
    for platform, key in (("windows-x64", "windows_archive"), ("linux-x64", "linux_archive"),
                          ("macos-arm64", "macos_archive")):
        spec = PLATFORMS[platform]
        extension = ".zip" if spec["archive"] == "zip" else ".tar.gz"
        result[key] = f"WRITEOVER-07-v{version}-{spec['suffix']}{extension}"
    return result


def verify_tag(root: Path, tag: str) -> str:
    head = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
    target = subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "--verify", f"refs/tags/{tag}^{{commit}}"],
        text=True, stderr=subprocess.DEVNULL).strip()
    if target != head:
        raise ValueError("Release tag must identify the checked-out commit exactly")
    return head


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--tag", default=os.environ.get("WRITEOVER_RELEASE_TAG"))
    parser.add_argument("--platform", choices=PLATFORMS)
    parser.add_argument("--require-tag", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--github-output", type=Path)
    parser.add_argument("--github-output-env", action="store_true")
    args = parser.parse_args()
    result = metadata(args.source_root, args.tag)
    if args.platform:
        result["archive"] = result[args.platform.split("-")[0] + "_archive"]
    if args.require_tag:
        result["commit"] = verify_tag(args.source_root, result["tag"])
    output_path = args.github_output
    if args.github_output_env:
        output_path = Path(os.environ["GITHUB_OUTPUT"])
    if output_path:
        with output_path.open("a", encoding="utf-8", newline="\n") as stream:
            for key, value in result.items():
                stream.write(f"{key}={value}\n")
    print(json.dumps({"mode": "DRY_RUN" if args.dry_run else "METADATA_ONLY",
                      "publishes": False, **result}, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
