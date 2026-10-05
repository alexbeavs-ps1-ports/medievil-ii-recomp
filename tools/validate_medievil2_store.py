"""Compile and exercise cold/warm/corrupt/modified-disc resident-store contracts."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc',type=Path,required=True)
    parser.add_argument('--cxx',required=True)
    parser.add_argument('--cc',required=True)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    sys.path.insert(0,str(root/'psxrecomp/tools'))
    from aot_overlay_pipeline import Disc
    disc=Disc(args.disc)
    with tempfile.TemporaryDirectory(prefix='medievil2-store-') as temporary:
        directory=Path(temporary);assets=directory/'assets';cache=directory/'cache';assets.mkdir();cache.mkdir()
        for name in disc.files:
            if name=='PROJFILE.MWD' or name=='MED2.EXE' or name.endswith('.LVB'):
                path=assets/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(disc.read(name))
        obj=directory/'sha.o';binary=directory/('test.exe' if os.name=='nt' else 'test')
        includes=['-I',str(root/'psxrecomp/runtime/include'),'-I',str(root/'psxrecomp/lib/recomp-net/include')]
        subprocess.run([args.cc,'-std=c11','-O2',*includes,'-c',str(root/'psxrecomp/runtime/src/psx_sha256.c'),'-o',str(obj)],check=True)
        subprocess.run([args.cxx,'-std=c++17','-O2','-Wall','-Wextra','-Werror',*includes,
                        str(root/'tests/test_seamless_store.cpp'),str(root/'src/mods/medievil2_seamless_store.cpp'),
                        str(root/'src/mods/medievil2_pp20.cpp'),str(obj),'-o',str(binary)],check=True)
        subprocess.run([str(binary),str(assets),str(cache)],env=dict(os.environ,MEDIEVIL2_SEAMLESS_CACHE=str(cache)),check=True)
    print('Resident store: cold, warm, corrupt, modified plan/disc and bounds passed')

if __name__=='__main__':main()
