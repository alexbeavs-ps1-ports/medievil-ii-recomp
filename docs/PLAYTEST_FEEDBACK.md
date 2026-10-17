# Next MediEvil II playtest

This candidate fixes split-track USA disc targeting and dialogue audio resume,
adds optional mouse/WASD controls, and records controller presence. The next
pass should concentrate on the camera follow-up and remaining gameplay reports.
HD Texture Packs is temporarily absent from the bundled mod catalog; saved
HD selections remain inactive without that package. No pack selection is needed.

1. Leave **Playtest Diagnostics / Input Recording** enabled. Enable **Mouse
   and Keyboard** if testing the new controls; use native button layout A.
2. When a stall, missed attack or camera problem happens, press both stick
   buttons together (**L3 + R3**), or **F4** with Mouse and Keyboard enabled.
   A toast gives the incident number. Mark just after the problem; keep playing
   for a few seconds. Holding the chord records one mark.
3. In the feedback, pair each incident number with a brief description:
   object/enemy, weapon, intended action, what happened, and whether you paused.
   A short screen recording with sound is helpful for audio and camera feel.
4. Quit normally, then zip the entire newest `saves/diagnostics/session-*`
   folder. `diagnostics-location.txt` beside the game names it. Include the
   folder even if an earlier recording did not replay exactly.

The folder includes starting memory cards, input/presence, settings and mod
choices, plus a `telemetry.csv` sidecar. It contains no disc or BIOS. Audio is
enabled normally in the player candidate. Python is optional for analysis;
playing and recording require no Python or debug server.

Suggested short checks:

| Report | Useful check |
| --- | --- |
| Slowdown / interaction | Repeat the same pickup, door or switch; mark the stall. Report whether sound stutters too. If convenient, repeat with Smooth Presentation disabled and the same graphics settings. |
| Missed attack | Name the weapon and enemy. Mark a miss, then try after stepping back and facing the enemy. Test primary and secondary attacks separately. |
| Camera | With the default Right-stick Camera feature, try a controller with the launcher set to digital. With Mouse and Keyboard, walk with W/A/S/D while turning both ways, then reverse direction. Open inventory/pause and cross a doorway/cutscene: capture should stay until Escape or focus loss. Try changing a middle/side-button action. Mark any shake with F4. Alt + mouse invokes native stationary first-person look. |
| Voice resume | Pause and resume the opening ghost dialogue twice. Voice should continue with captions. |

`telemetry.csv` pairs each row with the input sequence. It records elapsed
host time and VBlank interval, retail update/VBlank counters, game state,
player position/facing/health/grace, the nearest active actor's identity,
position/health/grace, incident count, CD activity, and render-pass counters.
The nearest actor can be an object or NPC; proximity does not prove a hit.
The actor walk is capped at 64 entries. Boot/unknown layouts are flagged.
The sidecar runs through a bounded background queue; if it fails, input
recording continues. Formatting and disk writes run outside the game thread.
It is observational data and does not change strict input/RAM replay checks.

For developers or testers who want a report, with Python 3 installed:

```powershell
python tools/analyze_medievil2_feedback.py saves/diagnostics/session-123 --output feedback-report-123
```

Use a new output directory. The tool reads the session without changing it
and writes `REPORT.md`, `summary.json` and a timing SVG. It flags host gaps
of at least 50 ms separately from gameplay update gaps over two VBlanks,
lists damage and nearby-actor health changes, and locates incident windows.
Old sessions without telemetry still produce an input summary. Host intervals
include pacing/focus pauses and do not alone identify a CPU/GPU bottleneck.
Zero render verification checks means unmeasured, not verified.
