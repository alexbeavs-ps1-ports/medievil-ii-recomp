"""Generate metadata only from an owned SCUS-94564 disc; never emit asset bytes."""
import argparse
import hashlib
from pathlib import Path
import struct
import sys
import subprocess
import tempfile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc',type=Path,required=True)
    parser.add_argument('--framework',type=Path,default=Path(__file__).resolve().parents[1]/'psxrecomp')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--decoder',type=Path,required=True,help='Compiled tests/pp20_driver.cpp')
    args=parser.parse_args()
    sys.path.insert(0,str(args.framework/'tools'))
    from aot_overlay_pipeline import Disc
    disc=Disc(args.disc)
    engine=disc.read('MED2.EXE')[0x800:]
    archive=disc.read('PROJFILE.MWD')
    entries=[]
    for index in range(0x51a):
        words=struct.unpack_from('<9I',engine,0xbf6a4-0x10000+index*36)
        flags,offset,size,decoded=words[1],words[3],words[6],words[7]
        if flags&16:
            block=archive[offset*2048:offset*2048+size]
            assert block[:4]==b'PP20' and len(block)==size
            assert int.from_bytes(block[-4:-1],'big')==decoded&0xffffff
            with tempfile.TemporaryDirectory(prefix='medievil2-pp20-') as temporary:
                encoded_path=Path(temporary)/'source'; decoded_path=Path(temporary)/'decoded'
                encoded_path.write_bytes(block)
                subprocess.run([str(args.decoder),str(encoded_path),str(decoded_path)],check=True)
                output=decoded_path.read_bytes()
            assert len(output)-12==decoded&0xffffff
            cursor,remaining,bits=struct.unpack_from('<3I',output)
            entries.append((index,flags,offset,size,decoded,hashlib.sha256(block).hexdigest(),
                            hashlib.sha256(output[12:]).hexdigest(),cursor,remaining,bits))
    files=[]
    for name,(lba,size) in sorted(disc.files.items()):
        if name=='PROJFILE.MWD' or name.endswith('.LVB'):
            padded=(size+2047)&~2047
            raw=disc.reader.read_file_bytes(lba,padded)
            if any(raw[size:]): raise ValueError(f'{name}: nonzero final-sector tail')
            files.append((name,lba,size,hashlib.sha256(raw).hexdigest()))
    lines=['// Generated stock metadata and hashes; no disc content.',
           'static const FileEntry files[] = {']
    lines += [f'    {{"{name}",{lba}u,{size}u,"{digest}"}},' for name,lba,size,digest in files]
    lines += ['};','static const CompressedEntry compressed[] = {']
    lines += [f'    {{{i}u,{flags}u,{off}u,{size}u,0x{decoded:08X}u,"{digest}","{dh}",{cursor}u,{remaining}u,0x{bits:08X}u}},'
              for i,flags,off,size,decoded,digest,dh,cursor,remaining,bits in entries]
    lines += ['};','']
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text('\n'.join(lines),encoding='utf-8',newline='\n')
    print(f'{len(files)} files, {len(entries)} PP20 containers')

if __name__=='__main__': main()
