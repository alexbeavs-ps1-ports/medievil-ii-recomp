"""Render the milestone table in docs/PLAYTEST_BURNDOWN.md as a standalone SVG."""
from html import escape
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
document = root / 'docs/PLAYTEST_BURNDOWN.md'
table = document.read_text(encoding='utf-8').split('## Burndown chart', 1)[1].split('![', 1)[0]
points = [(label.strip(), int(remaining), int(resolved)) for label, remaining, resolved in
          re.findall(r'^\| ([^|]+) \| (\d+) \| (\d+) \|$', table, re.M)]
if len(points) < 2:
    raise ValueError('Missing milestone table')
labels = ['Intake', 'Scope added', 'Split disc', 'XA resume', 'Controls', 'Pad presence', 'Retail grace', 'Human cadence']
if len(labels) != len(points):
    labels = [label for label, _, _ in points]
width, height = 960, 440
left, right, top, bottom = 70, 900, 85, 325
maximum = max(remaining + resolved for _, remaining, resolved in points)
coords = [(left + i * (right-left)/(len(points)-1), bottom - n*(bottom-top)/maximum)
          for i, (_, n, _) in enumerate(points)]
svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="960" height="440" viewBox="0 0 960 440" role="img" aria-labelledby="title desc">',
       '<title id="title">MediEvil II playtest burndown</title>',
       '<desc id="desc">Reports remaining: ' + escape(', '.join(f'{label}: {n}' for label,n,_ in points)) + '.</desc>',
       '<rect width="960" height="440" rx="14" fill="#0e1420"/>',
       '<g font-family="Segoe UI,Arial,sans-serif" fill="#e6edf7">',
       '<text x="36" y="37" font-size="24" font-weight="600">MediEvil II · playtest burndown</text>',
       '<text x="36" y="61" font-size="14" fill="#aebdd1">2026-10-17 · completed acceptance gates</text>']
for n in range(maximum, -1, -2):
    y=bottom-n*(bottom-top)/maximum
    svg += [f'<path d="M{left} {y}H{right}" stroke="#29364a"/>',
            f'<text x="52" y="{y+5}" text-anchor="end" font-size="13" fill="#aebdd1">{n}</text>']
svg += ['<polyline points="'+' '.join(f'{x},{y}' for x,y in coords)+'" fill="none" stroke="#5d9dff" stroke-width="3"/>']
for (x,y),label,(_,n,_) in zip(coords,labels,points):
    svg += [f'<circle cx="{x}" cy="{y}" r="5" fill="#79b2ff"/>',
            f'<text x="{x}" y="{y-13}" text-anchor="middle" font-size="17" font-weight="600">{n}</text>',
            f'<text x="{x}" y="352" text-anchor="middle" font-size="13" fill="#aebdd1">{escape(label)}</text>']
svg += [f'<text x="36" y="396" font-size="16">{points[-1][2]} verified · 3 instrumented for human follow-up · 1 HD deferred / hidden</text>',
        '<text x="36" y="420" font-size="13" fill="#aebdd1">Milestones reflect validation, not elapsed time. Source: PLAYTEST_BURNDOWN.md</text>',
        '</g></svg>']
target = document.with_suffix('.svg')
target.write_text('\n'.join(svg)+'\n', encoding='utf-8', newline='\n')
print(target)
