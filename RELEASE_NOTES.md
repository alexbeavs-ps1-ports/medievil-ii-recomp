# MediEvil II Recompiled release notes

## 0.2.5-alpha — shared optimization release

Fresh settings enable Internal Resolution at 4x. The Mods selector offers 1x through 32x; saved choices are preserved, and GPU texture and memory limits still apply. Disabling the mod selects native resolution.

This build includes the combined qualified engine optimizations and uses the shared release pipeline with selective runtime PGO. CD Speed and Fast Loading packages are excluded. Existing memory cards remain supported.

Automated gameplay validation for this release batch is limited to Ape Escape and Tomba 2. The other titles use build and package checks followed by owner visual approval; no per-title performance gain is claimed.

# MediEvil II Recompiled v0.2.4-alpha

Release refresh for Windows x64 and Linux x86-64.
Supply your USA disc (SCUS-94564); OpenBIOS is included.

- Playtest Diagnostics is now off by default. It remains available in Mods;
  enable it before reproducing a problem you want to report.
- The README links the Halloween blog post.

Gameplay, enhancements and the framework are unchanged from v0.2.3-alpha.

# MediEvil II Recompiled v0.2.3-alpha

Playtest fixes and feedback tooling for Windows x64 and Linux x86-64.
Supply your USA disc (SCUS-94564); OpenBIOS is included.

- Mouse look now follows mouse direction by default, smooths event velocity,
  and uses a single movement path for WASD while turning. Menus retain D-pad
  navigation. Old mouse-axis inversion settings reset to the corrected defaults.
- Mouse capture stays through native pause/inventory, cutscenes and room loads;
  Escape, focus loss and host UI still release it. Transition motion is discarded.
- All five mouse buttons have native action choices in the mod options. Middle
  click opens inventory; side buttons switch weapons and targets by default.
  The mod description and `docs/MODERN_CONTROLS.md` explain all defaults.
- Right-stick Camera is enabled by default, including with digital launcher
  configuration; controller and mouse reversal settings are independent.
- HD Texture Packs is temporarily hidden and disabled, including saved enabled
  choices and leftover bundled/installed copies. Pack investigation is deferred.
- Feedback analysis now locates simultaneous movement/look and duplicate
  movement input. The new human pass reports smoother gameplay through the first
  boss; remaining camera feel and specific combat incidents still need playtesting.

- Resident Loading and Adaptive View accept both supported USA disc layouts,
  including the canonical split-track Track 1 image.
- Paused XA dialogue resumes from the drive position instead of restarting
  from the last seek target. New saves retain unconsumed Setloc state too.
- Optional Mouse and Keyboard controls add WASD, mouse look, Space jump,
  combat/item shortcuts and camera direction options. See `docs/MODERN_CONTROLS.md`.
- Diagnostics v2 records pad presence. Exact replay still requires the original
  build; developer input-only routes are separately labelled and cannot report
  successful RAM verification.
- Playtests now capture timing, loading and nearby-actor evidence independently
  of replay. L3 + R3 (F4 with modern controls) marks an incident. The packaged
  analysis tool produces a timing chart and event report; see `docs/PLAYTEST_FEEDBACK.md`.

Validation includes exact split-disc targeting, live dialogue resume, controller
hotplug replay, and a fresh 777-frame combat replay. The same combat inputs
produce matching health writes with Smooth Presentation enabled and disabled.
Windows checks and Linux SDL2/input/diagnostics fixtures pass. Linux package
qualification is under WSL; native Linux/Steam Deck and full-game playthroughs
remain unqualified. Linux requires glibc 2.39 or newer.

General slowdown, brief object-interaction stalls, camera feel and specific
missed attacks remain open for the next human feedback pass. HD texture-pack
slowdown is deferred until the pack is identified. No guest cycle-scale or
combat-balance change is included: the Ape Escape scale accelerated this game.
Mark incidents with L3 + R3, or F4 with Mouse and Keyboard enabled, then quit
normally and send the newest complete diagnostics session folder.

# MediEvil II Recompiled v0.2.1-alpha

Precompiled Windows and Linux player builds with the complete enhancement set.
Supply your MediEvil II USA (SCUS-94564) disc in the launcher and play.
OpenBIOS is bundled, its startup shell is skipped, and no compiler or Generate
step is required. Disc images, decoded assets and saves are not included.

Default-on improvements:

- Adaptive widescreen follows the window, including ultrawide views, with
  extended terrain visibility, subdivision bypass and expanded render memory.
- Weapon and ammo HUD groups follow the left edge; chalice and gold follow
  the right edge. Health stays centered and text keeps its proportions.
- Higher internal resolution, PGXP precision and stable world texture filtering,
  including the Museum doorway texture-jitter fix.
- Full-width cutscene bars through interpolated drawing and centered subtitles.
- Draw interpolation for smoother presentation, with Display as the default
  target and 60/120/144/240/360 options. Original simulation timing is preserved.
- Resident Loading preloads files and prepares verified PP20 decompression
  results from your disc, reducing supported loading waits.
- Audited native engine/level variants and a recomp-ui launcher with box art.
- Playtest Diagnostics (default on, new in this build) records each frame's
  controller state and a memory checksum on a background thread, plus your
  settings and starting memory cards, so a bug can be replayed exactly. Zip the
  newest `saves/diagnostics/session-*` folder when reporting a problem; it never
  contains the disc or BIOS. An on-screen input line can be turned off in Mods.
- Framework updated to current PSXRecomp and recomp-ui masters.

On Windows, extract the complete ZIP and run `MediEvilIIRecomp.exe`. On Linux,
make the AppImage executable and run it; `--appimage-extract-and-run` works
without FUSE. Linux requires glibc 2.39 or newer, such as Ubuntu 24.04.

This alpha has bounded Museum/intro and sampled level validation, not a full
playthrough. HUD movement was checked at 4:3, 16:9, 21:9 and 32:9, including
50 replay checks with no state mismatches. Later-level special HUDs and native
Linux/Steam Deck hardware need further playtesting. Display is a requested
presentation rate, not a guarantee of that many distinct rendered frames.
See the packaged enhancement notes for detailed evidence and limitations.
