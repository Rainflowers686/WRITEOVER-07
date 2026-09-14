import tempfile
import unittest
from pathlib import Path

from release_metadata import metadata


class ReleaseMetadataTests(unittest.TestCase):
    def test_versions_change_every_asset_name(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "docs/release").mkdir(parents=True)
            for version in ("0.2.0-candidate.1", "0.3.0-preview.2"):
                (root / "PRODUCT_VERSION").write_text(version, encoding="utf-8")
                (root / f"docs/release/RELEASE_NOTES_v{version}.md").write_text("Candidate notes", encoding="utf-8")
                result = metadata(root, "v" + version)
                for key in ("windows_archive", "linux_archive", "macos_archive", "notes", "title"):
                    self.assertIn(version, result[key])
                    self.assertNotIn("pvs01", result[key])
                self.assertEqual(result["prerelease"], "true")
                with self.assertRaises(ValueError):
                    metadata(root, "v0.1.0-pvs01-gold")

    def test_missing_notes_and_invalid_versions_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for version in ("0.2.0-candidate.1", "../other", "0.2.0\nextra", "01.2.0", "0.2", ""):
                (root / "PRODUCT_VERSION").write_text(version, encoding="utf-8")
                with self.assertRaises(ValueError):
                    metadata(root)


if __name__ == "__main__":
    unittest.main()
