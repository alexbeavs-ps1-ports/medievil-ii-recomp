"""Summarize a playtest session without replay, a disc, BIOS or extra packages.

Usage: python analyze_medievil2_feedback.py SESSION --output REPORT_DIRECTORY
The original session is read only. Output includes JSON, a Markdown summary
and an SVG timing plot. Counts describe observations, never prove collision
correctness or identify an actor as an enemy. Old sessions lacking telemetry
remain useful as input evidence and receive an explicit availability note.
"""
import argparse
import csv
from collections import Counter
from html import escape
import json
from pathlib import Path
import statistics


def percentile(values, fraction):
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, int((len(ordered) - 1) * fraction))] if ordered else 0


def summarize(session):
    session = Path(session)
    metadata = json.loads((session / 'session.json').read_text(encoding='utf-8'))
    inputs, attacks, presence, previous = {}, [], [], None
    camera = dict(look_rows=0, moving_look_rows=0, duplicate_movement_rows=0, moving_look_windows=[])
    window = None
    with (session / 'inputs.csv').open(encoding='utf-8') as source:
        for line in source:
            if not line.startswith('N ') or not line.endswith('\n'):
                continue  # a torn final row is not a complete input boundary
            fields = [int(value) for value in line[2:].strip().split(',')]
            if len(fields) not in (14, 16):
                raise ValueError('Unsupported input row width')
            sequence, buttons = fields[:2]
            inputs[sequence] = buttons
            analog_move = fields[3] and max(abs(fields[4]-128), abs(fields[5]-128)) > 36
            digital_move = (buttons & 0xF0) != 0xF0
            looking = fields[3] and max(abs(fields[6]-128), abs(fields[7]-128)) > 36
            camera['look_rows'] += bool(looking)
            camera['duplicate_movement_rows'] += bool(analog_move and digital_move)
            if looking and (analog_move or digital_move):
                camera['moving_look_rows'] += 1
                if window is None:
                    window = dict(start=sequence, end=sequence, frames=0)
                    camera['moving_look_windows'].append(window)
                window['end'] = sequence
                window['frames'] += 1
            else:
                window = None
            if previous is not None:
                for name, mask in [('primary', 0x4000), ('secondary', 0x8000)]:
                    if previous[1] & mask and not buttons & mask:
                        attacks.append(dict(sequence=sequence, action=name))
                if len(fields) == len(previous) == 16 and fields[14:16] != previous[14:16]:
                    presence.append(dict(sequence=sequence, connected=fields[14:16]))
            previous = fields
    result = dict(build=metadata.get('build'), execution=metadata.get('execution'),
                  input_rows=len(inputs), attack_edges=attacks, presence_changes=presence, camera_input=camera,
                  telemetry_available=False, incidents=[], damage=[], actor_health_changes=[],
                  host_gaps=[], update_gaps=[], notes=[
                      'Observational evidence; no successful replay or retail collision equivalence is implied.',
                      'A nearby actor can be scenery/NPC/object; proximity alone is not a hit test.',
                      'Host VBlank intervals include pacing, focus pauses, rendering and diagnostic work.'])
    path = session / 'telemetry.csv'
    if not path.exists():
        result['notes'].append('This older session has no telemetry.csv. Capture a new session for timing/combat evidence.')
        return result, []
    with path.open(encoding='utf-8', newline='') as source:
        reader = csv.DictReader(source)
        required = {'sequence', 'host_interval_us', 'host_elapsed_us', 'game_vblank', 'game_updates',
                    'retail_layout', 'player', 'player_state', 'game_state', 'game_flags', 'health', 'grace',
                    'near_actor', 'near_health', 'incident', 'render_verify_mismatch'}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError('Unsupported or incomplete telemetry schema')
        rows = []
        for raw in reader:
            if None in raw or any(value is None for value in raw.values()):
                result['notes'].append('Ignored an incomplete telemetry tail.'); break
            try:
                rows.append({key: int(value) for key, value in raw.items()})
            except ValueError:
                result['notes'].append('Ignored an incomplete telemetry tail.'); break
    result['telemetry_available'] = True
    result['telemetry_rows'] = len(rows)
    result['paired_rows'] = sum(row['sequence'] in inputs for row in rows)
    intervals = [row['host_interval_us'] for row in rows if row['host_interval_us'] > 0]
    result['host_interval_ms'] = dict(median=statistics.median(intervals)/1000 if intervals else 0,
                                    p95=percentile(intervals,.95)/1000, p99=percentile(intervals,.99)/1000,
                                    maximum=max(intervals, default=0)/1000)
    result['render_verify_mismatch_max'] = max((row['render_verify_mismatch'] for row in rows), default=0)
    result['render_verify_checks'] = max((row.get('render_verify_checks',0) for row in rows), default=0)
    result['actor_scan_capped_rows'] = sum(bool(row.get('actor_scan_capped',0)) for row in rows)
    previous, last_update = None, None
    cadence = Counter()
    for row in rows:
        sequence = row['sequence']
        if row['host_interval_us'] >= 50000:
            result['host_gaps'].append(dict(sequence=sequence, ms=row['host_interval_us']/1000,
                                           cd_data_read=row.get('cd_data_read'), cd_xa=row.get('cd_xa')))
        if row['incident'] > (previous['incident'] if previous else 0):
            result['incidents'].append(dict(sequence=sequence, incident=row['incident']))
        active = row['retail_layout'] and row['player'] and row['player_state'] >= 0 and not row['game_state'] and not row['game_flags'] & 0x3ff
        same_player = previous and row['player'] and row['player'] == previous['player']
        if active and same_player and row['health'] < previous['health']:
            result['damage'].append(dict(sequence=sequence, lost=previous['health']-row['health'],
                                         health=row['health'], grace=row['grace'], prior_grace=previous['grace'],
                                         near_actor=row['near_actor'], near_health=row['near_health']))
        if previous and row['near_actor'] and row['near_actor'] == previous['near_actor'] and row['near_health'] != previous['near_health']:
            result['actor_health_changes'].append(dict(sequence=sequence, actor=row['near_actor'],
                                                       old=previous['near_health'], new=row['near_health']))
        if not active or not same_player:
            last_update = row if active else None
        elif last_update and row['game_updates'] != last_update['game_updates']:
            updates = (row['game_updates'] - last_update['game_updates']) & 0xffffffff
            vblanks = (row['game_vblank'] - last_update['game_vblank']) & 0xffffffff
            # Exclude restore/reset discontinuities; preserve raw rows for investigation.
            if updates == 1 and 0 < vblanks < 600:
                cadence[vblanks] += 1
                if vblanks > 2:
                    result['update_gaps'].append(dict(sequence=sequence, vblanks=vblanks,
                                                     host_interval_us=row['host_interval_us']))
            last_update = row
        previous = row
    result['vblanks_per_update'] = dict(sorted(cadence.items()))
    lookup = {row['sequence']: row for row in rows}
    for event in attacks:
        row = lookup.get(event['sequence'])
        if row:
            event.update(near_actor=row['near_actor'], near_health=row['near_health'],
                         distance_squared=row.get('distance_squared'), player_grace=row['grace'])
    return result, rows


def write_report(session, output):
    result, rows = summarize(session)
    output = Path(output); output.mkdir(parents=True, exist_ok=False)
    (output / 'summary.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    lines = ['# MediEvil II playtest evidence', '', f"Build: `{result['build']}`; {result['input_rows']} complete input rows.", '']
    if result['telemetry_available']:
        timing = result['host_interval_ms']
        lines += [f"Host VBlank interval: median {timing['median']:.2f} ms, p95 {timing['p95']:.2f}, p99 {timing['p99']:.2f}, max {timing['maximum']:.2f}.", '',
                  f"{len(result['host_gaps'])} intervals ≥50 ms; {len(result['update_gaps'])} gameplay update gaps >2 VBlanks; "
                  f"{len(result['damage'])} health losses; {len(result['actor_health_changes'])} nearby-actor health changes.", '',
                  f"Render state verification: {result['render_verify_checks']} checks, {result['render_verify_mismatch_max']} mismatches. A zero check count is not a pass.", '',
                  '![Host interval in milliseconds by input sequence](timing.svg)', '',
                  '| Incident | Input sequence | Suggested inspection window |', '| --- | ---: | --- |']
        for incident in result['incidents']:
            seq=incident['sequence']
            lines.append(f"| {incident['incident']} | {seq} | {max(0,seq-120)}–{seq+120} |")
    camera = result['camera_input']
    lines += ['', f"Camera input: {camera['look_rows']} look rows; {camera['moving_look_rows']} simultaneous movement/look rows; "
              f"{camera['duplicate_movement_rows']} rows asserting both D-pad and left-stick movement.",
              'Moving/look windows are listed in summary.json. These are accepted pad states; they cannot establish visual camera shake or identify the physical input device.', '',
              f"Primary/secondary attack edges: {len(result['attack_edges'])}; controller presence changes: {len(result['presence_changes'])}.", '', *result['notes'], '',
              'See summary.json for event sequences and telemetry.csv in the original session for the surrounding rows.']
    (output / 'REPORT.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    if rows:
        # Per-bin maxima preserve short stalls when plotting long recordings.
        bins = {}
        for row in rows:
            x = int(row['sequence'] * 880 / max(1,rows[-1]['sequence']))
            bins[x] = max(bins.get(x,0),row['host_interval_us']/1000)
        ceiling = max(40, max(bins.values()))
        points = ' '.join(f'{50+x:.1f},{260-min(ceiling,y)/ceiling*210:.1f}' for x,y in sorted(bins.items()))
        marks = ''.join(f'<path d="M{50+e["sequence"]*880/max(1,rows[-1]["sequence"]):.1f} 50V260" stroke="#ffca6b"/>' for e in result['incidents'])
        svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="960" height="310" viewBox="0 0 960 310" role="img" aria-label="Host VBlank intervals"><rect width="960" height="310" fill="#0e1420"/><g font-family="sans-serif" fill="#e6edf7"><text x="30" y="25">Host VBlank interval · bin maxima · incident markers in gold</text><text x="10" y="52">{ceiling:.0f}</text><text x="20" y="260">0</text><text x="400" y="295">Input sequence 0–{rows[-1]["sequence"]}</text><polyline points="{escape(points)}" fill="none" stroke="#79b2ff"/>{marks}</g></svg>\n'
        (output / 'timing.svg').write_text(svg, encoding='utf-8')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('session',type=Path)
    parser.add_argument('--output',type=Path,required=True,help='New report directory; never overwrites a session')
    args=parser.parse_args()
    result=write_report(args.session,args.output)
    print(f"Report: {args.output / 'REPORT.md'}; telemetry available: {result['telemetry_available']}")


if __name__ == '__main__':
    main()
