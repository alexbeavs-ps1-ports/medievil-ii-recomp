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

The enhancement workbench uses OpenBIOS, skips its startup shell, and enables
higher internal resolution, PGXP precision, adaptive world rendering and
display-rate presentation by default. It also supplies game-specific terrain
capture, expanded render buffers, selectable draw distance and subdivision
bypass. This is an enhancement workbench; full gameplay and release
qualification are still in progress. See [ENHANCEMENTS.md](docs/ENHANCEMENTS.md)
for the exact source pins, evidence and known limitations.

| Enhancement | Default / choices |
| --- | --- |
| Internal resolution | 1080p preset, rounded to a whole native-resolution multiplier |
| PGXP | Geometry, perspective textures and CPU propagation enabled |
| World view | Fit to window; 4:3, 16:9, 21:9 and 32:9 also available |
| Cutscene layout | Full-width letterbox bars; centered subtitles retain their original proportions |
| Terrain distance | 3x; Original and 2x also available |
| Terrain subdivision | Bypassed; original subdivision remains selectable |
| Presentation rate | Display; 60, 120, 144, 240 and 360 also available |
| Resident loading | On; preloaded archive/level files and predecoded PP20 assets |

Presentation interpolates camera/model transforms and replays game drawing
while retaining the original game timing. Stable world filtering defaults on,
with nearest and bilinear alternatives; untracked UI stays nearest. It does not create additional simulation frames or guarantee sustained
throughput at every target. Movies retain their authored proportions.

Resident Loading prepares assets from your own disc, keeps about 44.5 MiB in
host memory, and completes supported file reads and decompression immediately.
The game still allocates, relocates and initializes those assets itself. Movies,
streamed audio and gameplay keep their original timing. Turn it off in Mods to
use the original loader; changed or unsupported assets also fall back. See
[the loading mod notes](docs/RESIDENT_LOADING.md) for coverage and validation.

Known qualification work includes intro flicker, circular fade coverage,
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
