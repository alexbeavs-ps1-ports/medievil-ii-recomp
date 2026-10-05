"""Regenerate and audit built-in native code from the supported owned disc."""
import argparse
from pathlib import Path
import subprocess
import sys

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--recompiler', required=True)
    p.add_argument('--gcc', default='gcc')
    p.add_argument('--cmake', default='cmake')
    p.add_argument('--workers', type=int, default=2)
    args = p.parse_args()
    root = Path(__file__).resolve().parents[1]
    recompiler = str(Path(args.recompiler).resolve())
    subprocess.run([recompiler, '--config', str(root/'game.toml')], cwd=root, check=True)
    subprocess.run([sys.executable, str(root/'psxrecomp/tools/aot_overlay_pipeline.py'),
        'static', '--profile', str(root/'aot/overlays.json'), '--game-toml', str(root/'game.toml'),
        '--recompiler', recompiler, '--work-dir', str(root/'build-release-aot'),
        '--gcc', args.gcc, '--cmake', args.cmake, '--workers', str(args.workers),
        '--cps', '--reuse'], cwd=root, check=True)

if __name__ == '__main__':
    main()
