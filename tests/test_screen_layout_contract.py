"""Verify mask/HUD producer ownership and bounded submission without disc assets."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

class ScreenLayoutContractTest(unittest.TestCase):
    def test_owned_packets_and_reuse(self):
        root = Path(__file__).resolve().parents[1]
        compiler = os.environ.get('CC') or shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(compiler, 'Set CC to a native GCC or Clang executable')
        with tempfile.TemporaryDirectory(prefix='medievil2-screen-layout-') as temporary:
            binary = Path(temporary) / ('test.exe' if os.name == 'nt' else 'test')
            subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-I', str(root/'psxrecomp/runtime/include'),
                            str(root/'tests/test_screen_layout_contract.c'),
                            '-lm', '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

if __name__ == '__main__':
    unittest.main()
