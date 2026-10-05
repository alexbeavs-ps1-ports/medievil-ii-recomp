# Player releases

Branch `release/medievil2-enhancements-20261005` combines all enhancement PRs
#1–#11 plus the final HUD anchoring update. `packaging/included-prs.json`
records their exact heads, the HUD commit and the Tomba packaging reference.
The later framework pin supersedes the pin-only updates on PRs #1/#2;
their feature changes and all subsequent fixes are retained.
The local packagers are the release entry points, following Tomba. They build
precompiled player packages; no disc data or credentials are needed in CI.
The obsolete setup-host publishing workflow has been retired so pushing a
version tag cannot publish packages requiring players to generate code.

The player package follows Tomba's precompiled release layout. The executable
contains the audited original engine, its enhanced variant and known relocated
level-module placements. A separate overlay compiler/cache is unnecessary for
these built-in variants. Unknown placements retain runtime fallback; full-game
native coverage is not claimed. Resident Loading prepares assets from the
player's disc. Disc images, retail BIOS files, decoded caches and saves are
excluded. OpenBIOS and its MIT notice are included. Player builds link only the
OpenBIOS backend, so first-run launcher discovery cannot silently select a
retail BIOS found elsewhere on the machine.

From a recursive checkout, put the supported USA disc in `disc/` as declared
by `game.toml`, build the pinned framework's `psxrecomp-game`, then run:

```sh
python tools/prepare_release.py --recompiler /path/to/psxrecomp-game --workers 2
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DPSX_SETUP_WIZARD=OFF -DPSXRECOMP_FORCE_SETUP_HOST=OFF \
  -DPSXRECOMP_REQUIRE_GAME_C=ON -DPSX_DEBUG_TOOLS=OFF \
  -DPSX_SDL_BACKEND=SDL3 -DPSX_ENABLE_VULKAN=OFF -DPSX_NETPLAY=ON
cmake --build build-release --target psx-runtime --parallel 4
```

Use native Windows executable paths for GCC/G++, Ninja and CMake on Windows.
`prepare_release.py` also accepts `--gcc` and `--cmake` paths. Source generation
audits all 24 profile images and publishes `generated/AOT_STATIC_AUDIT.json`.
Both platforms compile the same audited sources; no generated C is committed.
The framework provides recompiled OpenBIOS. Build dependencies include SDL3,
OpenGL, a C/C++ toolchain and Python 3.11+. If the host SDL3 development package
is unsuitable, `-DCMAKE_DISABLE_FIND_PACKAGE_SDL3=TRUE` builds its pinned source.

Windows packaging (use the full commit from `git rev-parse HEAD`):

```powershell
python tools/package_release.py --build-dir build-release --platform windows-x64 `
  --source-commit <full-commit> --release --stage dist/windows-stage `
  --objdump C:/msys64/mingw64/bin/objdump.exe `
  --zip dist/MediEvilIIRecomp-v0.2.0-alpha-windows-x64.zip
```

The ZIP checks system-only DLL imports, native code/audit correspondence, the
seven-package mod catalog, OpenBIOS, release flags and excluded user data.
Extract the complete ZIP into a writable directory and run `MediEvilIIRecomp.exe`.

Linux packaging needs ImageMagick, patchelf, curl and the usual ELF inspection
tools. It downloads checksum-pinned linuxdeploy/appimagetool releases:

```sh
SOURCE_COMMIT=$(git rev-parse HEAD) PUBLIC_RELEASE=1 BUILD_DIR="$PWD/build-release" \
  sh tools/package_appimage.sh
```

Output is under `dist/`, with a SHA-256 sidecar. The AppImage keeps persistent
data under `$XDG_DATA_HOME/MediEvilIIRecomp` (default
`~/.local/share/MediEvilIIRecomp`), following Tomba's writable-data launcher.
`MEDIEVIL_II_RECOMP_DATA_DIR` overrides that directory. Upgrades refresh bundled
resources while preserving settings, installed mods and saves. Bundled library
paths are not exported into host Browse-dialog processes.

Build flags disable the developer setup wizard and debug tooling. Omit
`--release` / `PUBLIC_RELEASE=1` for a local simulated release; neither mode
uploads anything. The manifest records source, pins, version and payload hashes.
Native Linux/Steam Deck and whole-game testing remain separate qualification
work. Current Linux builds require glibc 2.39 or newer (Ubuntu 24.04, for example);
AppImage packaging does not lower that requirement.

For publication, merge the reviewed enhancement PRs in order with merge commits,
then build and package the resulting `main` revision with `VERSION=0.2.0-alpha`.
Verify all prior PR heads are ancestors, regenerate native code after hook/config
changes, run the game contract suites, and smoke-test the extracted artifacts.
Both package manifests must name that same merged revision and bare version.
Create `v0.2.0-alpha` at that revision, then publish a GitHub prerelease with the
Windows ZIP, Linux AppImage, their SHA-256 sidecars and `RELEASE_NOTES.md`.
Keep the existing stable release as the Latest stable release. Publication is
an explicit maintainer step; the packaging scripts never push tags or upload.
