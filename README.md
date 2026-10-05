<p align="center"><a href="https://alexbeavs-ps1-ports.github.io/psxrecomp-ports/"><img src="https://raw.githubusercontent.com/alexbeavs-ps1-ports/psxrecomp-ports/main/docs/assets/alexbeav-ps1-recomps-banner.png" alt="Alexbeav's PS1 Recomps" width="100%"></a></p>

# MediEvil II  Recompiled

<!-- retcomm-readme-metrics -->
[![GitHub downloads (all assets, all releases)](https://img.shields.io/github/downloads/Alexbeav/medievil-ii-recomp/total)](https://github.com/Alexbeav/medievil-ii-recomp/releases)
[![GitHub downloads (latest release)](https://img.shields.io/github/downloads/Alexbeav/medievil-ii-recomp/latest/total)](https://github.com/Alexbeav/medievil-ii-recomp/releases/latest)
[![GitHub release](https://img.shields.io/github/v/release/Alexbeav/medievil-ii-recomp)](https://github.com/Alexbeav/medievil-ii-recomp/releases/latest)
<!-- /retcomm-readme-metrics -->

Static recompilation of **MediEvil II** built on
[psxrecomp](https://github.com/mstan/psxrecomp) and
[recomp-ui](https://github.com/mstan/recomp-ui).

MediEvil II recompiled for modern systems using psxrecomp.

This branch consolidates enhancement PRs **#1–#11** and the final HUD anchoring
update for the `0.2.0-alpha` player release.
Windows ZIP and Linux x86-64 AppImage
packages contain precompiled game/overlay code, OpenBIOS, recomp-ui box art
and all bundled enhancements. Supply your USA disc and play; there is no
compiler or Generate step. Resident assets are prepared from your disc on
first run. No disc, decoded assets or user saves are distributed.

See [release build instructions](docs/RELEASE_BUILDS.md) and the packaged
`START_HERE.txt`. Each package records its source/pins and file hashes.
The release scripts produce Windows and Linux player packages from an audited
checkout. Linux requires glibc 2.39 or newer (for example, Ubuntu 24.04);
testing so far is under WSL, with native Linux hardware qualification pending.

The enhancement workbench uses OpenBIOS, skips its startup shell, and enables
higher internal resolution, PGXP precision, adaptive world rendering and
display-rate presentation by default. It also supplies game-specific terrain
capture, expanded render buffers, extended draw distance and subdivision
bypass. This is an enhancement workbench; full gameplay and release
qualification are still in progress. See [ENHANCEMENTS.md](docs/ENHANCEMENTS.md)
for the exact source pins, evidence and known limitations.

The recomp-ui launcher displays the USA front cover. Its artwork is bundled
locally, so opening the launcher does not need an image download.

| Enhancement | Default / choices |
| --- | --- |
| Internal resolution | 1080p preset, rounded to a whole native-resolution multiplier |
| PGXP | Geometry, perspective textures and CPU propagation enabled |
| World view | Automatically fit the window, with a 4:3 minimum |
| Cutscene layout | Full-width letterbox bars; centered subtitles retain their original proportions |
| Gameplay HUD | Weapon/ammo groups anchor left and money/chalice groups anchor right; health and centered messages stay centered |
| Terrain distance | Extended to 3x as part of Adaptive View |
| Terrain subdivision | Bypassed as part of Adaptive View |
| World texture filtering | Stable filtering enabled; text and HUD sprites stay sharp |
| Presentation rate | Display; 60, 120, 144, 240 and 360 also available |
| Resident loading | On; preloaded archive/level files and predecoded PP20 assets |

The framework pin includes the shared PGXP fixes for in-place arithmetic,
vertex fields carried through bitwise operations, the GTE vertex FIFO and
precision restoration after interpolated drawing. Generate regenerates the
game and AOT code with the corresponding instruction hooks. See
[the PGXP uptake notes](docs/ENHANCEMENTS.md#pgxp-propagation-uptake-2026-10-04).

The Museum doorway's diagonal trim breaks and camera-dependent texture jitter
are fixed in the shared renderer. Perspective world polygons retain their
authored UVs; mirrored 2D sprites keep their original sampling correction.
Validated from the reported save with movement and camera settling, including
interpolated drawing and wide aspect ratios. PGXP and perspective textures
remain default-on. See [the doorway validation](docs/ENHANCEMENTS.md#museum-doorway-texture-stability-2026-10-04).

Cutscene bars keep their full adaptive width during interpolated drawing,
including the Museum intro flashes. Normal frames and render passes retain
separate mask/subtitle submission metadata; subtitle proportions and authored
bar heights are preserved. See [the replay validation](docs/ENHANCEMENTS.md#cutscene-mask-replay-fix-2026-10-04).

Adaptive View also anchors the status-panel HUD to the visible edges. Icons
and their numbers move together without stretching, while the central health
bar keeps its original alignment. This is enabled with Adaptive View and
needs no additional switch. The game retains its original HUD fade timing.
See [HUD ownership and validation](docs/ENHANCEMENTS.md#gameplay-hud-anchoring-2026-10-05).

Presentation interpolates camera/model transforms and replays game drawing
while retaining the original game timing. Adaptive View and World Texture
Filtering each have one default-on behavior and an on/off switch in Mods;
there are no separate distance, subdivision or filtering-mode controls.
PGXP remains enabled for precise geometry and perspective-correct textures;
stable filtering and the doorway sampling fix complement it. Presentation
does not create additional simulation frames or guarantee sustained
throughput at every target. Movies retain their authored proportions.

Interpolated drawing now targets the framebuffer that presentation actually
captures, including each viewport's draw area and offset. Movement captures
verify distinct intermediate terrain, camera and character poses. See
[the replay handoff validation](docs/ENHANCEMENTS.md#interpolation-framebuffer-handoff-2026-10-05).

Adaptive View also removes terrain that is outside both camera endpoints of
an interpolated draw, using the actual polygon bounds. This reduces redraw
cost without reducing the view width, draw distance or visual defaults.
Display selects the monitor's refresh rate as a target; it is not an achieved
FPS reading. See [presentation measurements](docs/ENHANCEMENTS.md#terrain-redraw-performance-2026-10-05)
for the measured improvement and remaining high-refresh limits.

Resident Loading prepares assets from your own disc, keeps about 44.5 MiB in
host memory, and completes supported file reads and decompression immediately.
The game still allocates, relocates and initializes those assets itself. Movies,
streamed audio and gameplay keep their original timing. Turn it off in Mods to
use the original loader; changed or unsupported assets also fall back. See
[the loading mod notes](docs/RESIDENT_LOADING.md) for coverage and validation.
The generic CD Speed and Fast Loading packages are excluded from this game's
catalog; Resident Loading is the single loading enhancement.

Known qualification work includes other intro effects, circular fade coverage,
gameplay HUD anchoring, and native dispatch
for dynamically allocated level modules. Owned disc images, extracted modules
and generated C are local inputs and are never committed.

| | |
|---|---|
| Players | 1 |
| Region | USA |
| Publisher | Sony |
| Year | 2000 |

Scaffolded with the New Project Layout. See
`psxrecomp/docs/GAME_PROJECT_SETUP.md` for the full flow.

<!-- retcomm-readme-launcher -->
## RetComM Launcher

You can run this title **standalone** (release zip + the built-in recomp-ui
Generate & Build flow), or manage installs, updates, ROM/BIOS wiring, and queued
builds more intuitively with
**[RetComM Launcher](https://github.com/TechnicallyComputers/RetComM-Launcher)** â€”
the Retro Compilation Manager hub for self-compiling recomps.

[Downloads](https://github.com/TechnicallyComputers/RetComM-Launcher/releases) Â·
[Full README & features](https://github.com/TechnicallyComputers/RetComM-Launcher#readme)

<p align="center">
  <img src="https://raw.githubusercontent.com/TechnicallyComputers/RetComM-Launcher/main/docs/screenshots/hub-and-game-launcher.png" alt="RetComM hub with a background build, next to a titleâ€™s recomp-ui launcher" width="720">
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/TechnicallyComputers/RetComM-Launcher/main/docs/screenshots/queue-and-background-build.png" alt="Background cmake build with titles queued" width="720">
</p>

RetComM checks for updates, rebuilds with existing build data when possible,
uses the same platform build tools as per-title launchers, and automates
BIOS/ROM/save plumbing so you are not stuck repeating each gameâ€™s wizard by hand.
<!-- /retcomm-readme-launcher -->

## Legal

You must own the original game. Disc images under `disc/` are gitignored and
must never be committed. Retail BIOS dumps are not redistributed; OpenBIOS is
used for Generate unless you supply your own SCPH locally.

Default app icon: `assets/psxrecomp.ico` (and `.png` / `.svg`) â€” RetComM-themed controller mark from `psxrecomp/assets/`. Windows builds embed it via `APP_ICON`.

Optional box art under `launcher_assets/img/` may come from
[libretro-thumbnails](https://github.com/libretro-thumbnails/libretro-thumbnails)
(`Named_Boxarts`); see `BOXART_SOURCE.txt` when present.

## Quick start (dev)

```bash
git submodule update --init --recursive
./psxrecomp/tools/ci/build_emitters.sh
python3 psxrecomp/psxrecomp_cli.py generate \
  --config game.toml --project-root . --disc disc/<your>.cue
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target psx-runtime
```

Zip prefix for CI artifacts: `medievil-ii-recomp`.

## Symbols

Progressive map: `symbols.toml` â†’ `python3 tools/sync_symbols.py` â†’
`psx_symbols.h` (`PSX_FN_*`). See `psxrecomp/docs/SYMBOLS.md`.

## Framework pins

Submodule gitlinks (`psxrecomp`, optional `recomp-ui`, nested `recomp-net`)
are authoritative. `framework_pins.txt` is an optional scaffold snapshot;
release CI logs SHAs with `record_pins.sh` but builds whatever the gitlinks
resolve to. Bump submodules deliberately â€” do not float on `main`/`master`
in release CI.

+## About this project

These ports are developed by a hobbyist (a DevSecOps engineer, not a game
programmer) with substantial AI assistance. What keeps that honest: every
change is validated before it ships - boot gates, hardware-oracle A/B
comparisons (Beetle/DuckStation), deterministic replay probes, and a shared
findings registry that documents failures as carefully as successes. AI
writes most of the code; the evidence discipline decides what survives.
Bug reports welcome - expect them to be investigated the same way.

tl;dr AI writes the code, but I always test it myself before pushing


<!-- retcomm-readme-raid -->
---

<p align="center">
  <sub><b>R.A.I.D. â€” Retro AI Development</b> Â· a Discord for AI-assisted retro reverse-engineering, decomp &amp; recomp</sub>
</p>

<p align="center">
  <a href="https://discord.gg/Ad9BwSzctP"><img src=".github/raid-discord.png" alt="Join the Retro AI Development (R.A.I.D.) Discord" width="200"></a>
</p>
<!-- /retcomm-readme-raid -->

## v0.1.1 three-platform candidate

This candidate targets Windows x64, Linux x64, macOS ARM64, and macOS x64.
The enhancement source kit includes the game adapters, mod catalogs and AOT
profile. Generate uses your legally owned game disc and the bundled OpenBIOS;
a retail BIOS dump is optional. Packages remain unpublished until their exact package tests
and release authorization pass.
