#!/usr/bin/env bash
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo"

if [[ "${1:-}" != "--skip-build" ]]; then
  python3 tools/contentc/contentc.py --data-dir data --out-dir data --check
  python3 tools/contentc/test_contentc.py
  python3 tools/systemic/systemic_schema_check.py --data-dir data
  python3 tools/systemic/test_systemic_schema.py
  python3 tools/systemic/compile_systemic_seed.py --src data/systemic/systemic_seed.json --out data/systemic/systemic_seed.bin
  cmake --preset linux-release
  cmake --build --preset linux-release --parallel 2
  ./out/build/linux-release/writeover_tests
  ctest --test-dir out/build/linux-release --output-on-failure
  ./out/build/linux-release/writeover_bench
fi

commit="$(git rev-parse HEAD)"
version="$(tr -d '\r\n' < PRODUCT_VERSION)"
python3 tools/release/package_release.py \
  --platform linux-x64 \
  --binary out/build/linux-release/writeover_app \
  --source-root "$repo" \
  --dist-root dist \
  --version "$version" \
  --commit "$commit"
