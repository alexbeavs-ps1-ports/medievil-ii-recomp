"""Prevent package provenance from describing a previously built executable."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('package_release',
    Path(__file__).resolve().parents[1] / 'tools/package_release.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


class BuildIdentity(unittest.TestCase):
    def test_accepts_current_identity_across_read_boundary(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / 'game.exe'
            binary.write_bytes(b'\0' * (1024 * 1024 - 8) + b'0.2.1-alpha+abcdef012345\0')
            package.validate_executable_identity(binary, '0.2.1-alpha', 'abcdef012345' + '0' * 28)

    def test_rejects_previous_executable(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / 'game.exe'
            binary.write_bytes(b'MZ\0previous binary\0' + b'0.2.1-alpha+111111111111\0')
            with self.assertRaisesRegex(ValueError, 'Executable is stale'):
                package.validate_executable_identity(binary, '0.2.1-alpha', '2' * 40)

    def test_rejects_wrong_version(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / 'game.exe'
            binary.write_bytes(b'0.2.0-alpha+abcdef012345\0')
            with self.assertRaisesRegex(ValueError, 'Executable is stale'):
                package.validate_executable_identity(binary, '0.2.1-alpha', 'abcdef012345' + '0' * 28)


if __name__ == '__main__':
    unittest.main()
