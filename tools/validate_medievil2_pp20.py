"""Compare every archive PP20 stream with its original MIPS decoder (owned disc)."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc',type=Path,required=True)
    parser.add_argument('--decoder',type=Path,required=True)
    parser.add_argument('--receipt',type=Path,required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    sys.path.insert(0,str(root/'psxrecomp/tools'))
    from aot_overlay_pipeline import Disc
    from unicorn import Uc,UC_ARCH_MIPS,UC_MODE_MIPS32,UC_MODE_LITTLE_ENDIAN
    from unicorn.mips_const import UC_MIPS_REG_A0,UC_MIPS_REG_A1,UC_MIPS_REG_A2
    from unicorn.mips_const import UC_MIPS_REG_SP,UC_MIPS_REG_GP,UC_MIPS_REG_RA,UC_MIPS_REG_V0
    from unicorn.mips_const import UC_MIPS_REG_PC,UC_MIPS_REG_S0,UC_MIPS_REG_S1,UC_MIPS_REG_S2,UC_MIPS_REG_S3,UC_MIPS_REG_S4
    disc=Disc(args.disc);engine=disc.read('MED2.EXE')[0x800:];archive=disc.read('PROJFILE.MWD')
    receipt=[]
    with tempfile.TemporaryDirectory(prefix='medievil2-oracle-') as directory:
        source_path=Path(directory)/'encoded';output_path=Path(directory)/'decoded'
        for index in range(0x51a):
            e=struct.unpack_from('<9I',engine,0xbf6a4-0x10000+36*index)
            if not e[1]&16: continue
            source=archive[e[3]*2048:e[3]*2048+e[6]];source_path.write_bytes(source)
            subprocess.run([str(args.decoder),str(source_path),str(output_path)],check=True)
            native=output_path.read_bytes();cursor,remaining,bits=struct.unpack_from('<3I',native)
            decoded=native[12:]
            for inplace in (False,True):
                uc=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN)
                uc.mem_map(0,0x800000);uc.mem_write(0x10000,engine)
                uc.mem_write(0xf46b8,bytes(int(f'{i:08b}'[::-1],2) for i in range(256)))
                src=0x80300000;dst=src+4*(e[7]>>24) if inplace else 0x80500000;gp=0x800e0000
                uc.mem_write(src&0x1fffffff,source)
                for reg,value in ((UC_MIPS_REG_A0,src),(UC_MIPS_REG_A1,dst),(UC_MIPS_REG_A2,len(source)),
                                  (UC_MIPS_REG_SP,0x802fff00),(UC_MIPS_REG_GP,gp),(UC_MIPS_REG_RA,0x80008000)):
                    uc.reg_write(reg,value)
                preserved=(UC_MIPS_REG_S0,UC_MIPS_REG_S1,UC_MIPS_REG_S2,UC_MIPS_REG_S3,UC_MIPS_REG_S4)
                for i,reg in enumerate(preserved):uc.reg_write(reg,0x12345670+i)
                uc.emu_start(0x800adfb4,0x80008000,count=250000000)
                assert uc.reg_read(UC_MIPS_REG_PC)==0x80008000,(index,'unfinished decoder')
                assert uc.reg_read(UC_MIPS_REG_SP)==0x802fff00,(index,'stack')
                assert all(uc.reg_read(reg)==0x12345670+i for i,reg in enumerate(preserved)),(index,'saved registers')
                assert uc.reg_read(UC_MIPS_REG_V0)==1,(index,'return')
                actual=bytes(uc.mem_read(dst&0x1fffffff,len(decoded)))
                assert actual==decoded,(index,inplace,'output')
                state=struct.unpack('<3I',bytes(uc.mem_read((gp&0x1fffffff)+0x86c,12)))
                assert state==(src+cursor,remaining,bits),(index,inplace,'scratch',state,(src+cursor,remaining,bits))
                del uc
            receipt.append({'index':index,'encoded':len(source),'decoded':len(decoded),
                            'sha256':hashlib.sha256(decoded).hexdigest(),'comparisons':2})
            print(f'PP20 {index}: separate and in-place match ({len(decoded)} bytes)',flush=True)
    args.receipt.parent.mkdir(parents=True,exist_ok=True)
    args.receipt.write_text(json.dumps({'streams':receipt,'comparisons':2*len(receipt)},indent=2)+'\n')

if __name__=='__main__':main()
