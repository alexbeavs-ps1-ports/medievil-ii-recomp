# MediEvil II Recompiled v0.2.0-alpha

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

On Windows, extract the complete ZIP and run `MediEvilIIRecomp.exe`. On Linux,
make the AppImage executable and run it; `--appimage-extract-and-run` works
without FUSE. Linux requires glibc 2.39 or newer, such as Ubuntu 24.04.

This alpha has bounded Museum/intro and sampled level validation, not a full
playthrough. HUD movement was checked at 4:3, 16:9, 21:9 and 32:9, including
50 replay checks with no state mismatches. Later-level special HUDs and native
Linux/Steam Deck hardware need further playtesting. Display is a requested
presentation rate, not a guarantee of that many distinct rendered frames.
See the packaged enhancement notes for detailed evidence and limitations.
