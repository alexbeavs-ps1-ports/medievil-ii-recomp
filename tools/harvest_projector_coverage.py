"""Harvest bounded native coverage from a prepared MediEvil II projector state.

Use a private QA runtime and save directory. Prepare an unlocked Kensington
projector checkpoint after the first Professor dialogue. This script restores
that state for each route, selects a level, and captures dispatch counters,
arrival/input screenshots and 2 MiB RAM for local analysis. It never writes
memory cards or changes source/disc files. Captures contain owned-game data;
keep the output under ignored analysis/ or outside the repository.

Routes 0..12 select the visible slides to the right of Kensington. Routes
level:26..level:30 temporarily replace that row's target byte with an original
hidden secondary-map id. Both paths use the original level loader. This is
QA coverage, not a gameplay cheat enabled by a shipped mod.
"""
from pathlib import Path
import argparse,sys,json,time,struct
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'psxrecomp/tools'))
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--port',type=int,required=True)
parser.add_argument('--slot',type=int,choices=range(12),required=True)
parser.add_argument('--output',type=Path,default=root/'analysis/native-coverage')
parser.add_argument('--arrival-frames',type=int,default=900)
parser.add_argument('routes',nargs='+',help='0..12 or level:26..level:30')
args=parser.parse_args()
if not 120<=args.arrival_frames<=7200:parser.error('arrival frames must be 120..7200')
from debug_client import query
out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
def ask(cmd,**kw):
    r=query('127.0.0.1',args.port,dict(cmd=cmd,**kw),timeout=20)
    if not r.get('ok'):raise RuntimeError((cmd,r))
    return r
def press(mask,frames=8):
    ask('press',buttons=mask,frames=frames)
    deadline=time.monotonic()+max(20,frames/15)
    while ask('pad_status')['override_frames']>0:
        if time.monotonic()>deadline:raise RuntimeError('Input timed out')
        time.sleep(.2)
    time.sleep(.15)
def restore():
    before=ask('savestate_status')['generation']
    ask('savestate',op='load',slot=args.slot)
    deadline=time.monotonic()+25
    while True:
        r=ask('savestate_status')
        if not r['pending'] and r['generation']!=before:
            if not r['last_ok'] or r['last_slot']!=args.slot:
                raise RuntimeError(('Checkpoint restore failed',r))
            break
        if time.monotonic()>deadline:raise RuntimeError('Restore timed out')
        time.sleep(.3)
    ask('clear_input');press(0xffff,30)
    # USA MED2.EXE Cheat menu entry, independent of renderer patches.
    code=ask('read_ram',addr='0x8006f960',len=16)['hex']
    if code!='e0ffbd270d80043c1c61842421280000':
        raise RuntimeError('Restored image is not the supported USA MED2.EXE')
    pointer=struct.unpack('<I',bytes.fromhex(ask('read_ram',addr='0x800efe80',len=4)['hex']))[0]
    if not 0x80010000<=pointer<0x80200000:
        raise RuntimeError('Projector state pointer is invalid')
    selected=struct.unpack('<I',bytes.fromhex(ask('read_ram',addr=hex(pointer),len=4)['hex']))[0]
    if selected!=2:
        raise RuntimeError('Checkpoint must have Kensington selected')
def metrics():
    rows=[];skip=0
    while True:
        page=ask('dirty_ram_stats',skip=skip)
        rows.extend(page['per_pc']);skip+=page['per_pc_emitted']
        if skip>=page['per_pc_matching']:break
        if not page['per_pc_emitted']:raise RuntimeError('Empty coverage page')
    page['per_pc']=rows
    return dict(dirty_ram_stats=page,registers=ask('get_registers'),
        native=ask('overlay_static_entries',addr_lo='0x80100000',addr_hi='0x80200000',limit=1024),
        render_pass_stats=ask('render_pass_stats'),video_info=ask('video_info'))
def shot(path):
    result=query('127.0.0.1',args.port,dict(cmd='wide_shot',path=path.as_posix()),timeout=20)
    if not result.get('ok'):
        # FMVs switch off native-wide; the canonical capture remains valid.
        if 'native-wide not engaged' not in result.get('error',''):raise RuntimeError(result)
        ask('screenshot',path=path.as_posix())
for item in args.routes:
    level_id=int(item.split(':')[1],0) if item.startswith('level:') else None
    n=0 if level_id is not None else int(item)
    label=f'level-{level_id:02x}' if level_id is not None else f'right-{n:02d}'
    if not 0<=n<=12:raise ValueError('Visible routes are 0..12')
    if level_id is not None and level_id not in (26,27,28,29,30):
        raise ValueError('Secondary map ids are 26..30')
    if (out/(label+'.json')).exists():raise FileExistsError(out/(label+'.json'))
    restore()
    if level_id is not None:
        # The original selected Kensington row has level id 18. Replace only
        # its target byte with a secondary map id already present in the
        # engine's level table. Stock selection/teardown/loading still runs.
        if ask('read_ram',addr='0x800e175a',len=2)['hex']!='1200':
            raise RuntimeError('Kensington target field does not match the original')
        ask('write_ram',addr='0x800e175a',val=hex(level_id))
    for i in range(n):
        press(0xffdf)
        press(0xffff,45) # Let the stock projector finish its slide transition.
    press(0xffff,24)
    shot(out/(label+'-selector.png'))
    before=metrics()
    (out/(label+'-before.json')).write_text(json.dumps(before,indent=2)+'\n')
    press(0xbfff);press(0xffff,45);press(0xbfff);press(0xffff,args.arrival_frames)
    shot(out/(label+'-arrival.png'))
    press(0xffef,40);press(0xbfff,12);press(0xffff,120)
    after=metrics()
    old={r['pc']:r for r in before['dirty_ram_stats']['per_pc']}
    delta=[]
    for r in after['dirty_ram_stats']['per_pc']:
        d={**r}
        for k in ('hits','insns','entries'):d[k]=r[k]-old.get(r['pc'],{}).get(k,0)
        if d['hits']>0:delta.append(d)
    delta.sort(key=lambda r:r['insns'],reverse=True)
    ram=b''.join(bytes.fromhex(ask('read_ram',addr=hex(a),len=0x40000)['hex']) for a in range(0x80000000,0x80200000,0x40000))
    (out/(label+'.ram')).write_bytes(ram)
    shot(out/(label+'-sample.png'))
    result=dict(selector_right_presses=n,level_id_override=level_id,source_slot=args.slot,observation='Stock projector loader, arrival, 40 frames forward and attack; bounded smoke coverage only. Secondary ids use the documented data-only selector target override.',before=before,after=after,fallback_delta=delta,ram_bytes=len(ram))
    (out/(label+'.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(label,'new fallback PCs',len(delta),'module top',[(r['pc'],r['hits']) for r in delta if int(r['pc'],16)>=0x100000][:6],flush=True)
