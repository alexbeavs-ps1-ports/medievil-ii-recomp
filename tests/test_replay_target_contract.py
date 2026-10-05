"""Validate the replay-only SDK framebuffer descriptors without disc data."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class ReplayTargetContractTest(unittest.TestCase):
    def test_framebuffer_handoff(self):
        root = Path(__file__).resolve().parents[1]
        compiler = os.environ.get('CC') or shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(compiler, 'Set CC to a native GCC or Clang executable')
        with tempfile.TemporaryDirectory(prefix='medievil2-replay-target-') as temporary:
            binary = Path(temporary) / ('test.exe' if os.name == 'nt' else 'test')
            subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-I', str(root/'psxrecomp/runtime/include'),
                            str(root/'tests/test_replay_target_contract.c'), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    unittest.main()
