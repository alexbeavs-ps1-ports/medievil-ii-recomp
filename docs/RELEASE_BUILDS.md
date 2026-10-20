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

Releases are cut with the framework's one release command. This repository
keeps only `packaging/release.toml` (see psxrecomp
`docs/RELEASING_A_TITLE.md`). From a recursive checkout, put the supported USA
disc in `disc/` as declared by `game.toml`, then run from the repository root:

```powershell
psxrecomp/tools/package_title_release.ps1 --channel public   --platform all
psxrecomp/tools/package_title_release.ps1 --channel playtest --platform all
```

On native Linux, run `psxrecomp/tools/package_title_release.sh --channel ...`.
`--channel` is required and has no default. `public` builds with
`PSX_RELEASE_CHANNEL=public`, so Playtest Diagnostics ship default off.
`playtest` ships them default on and adds `-playtest` to every artifact name.
On Windows, `--platform all` writes the Windows ZIP and then, through WSL, the
Linux AppImage.

The command builds the emitters and regenerates the game C. It then
regenerates and audits all 24 static AOT profile images and publishes
`generated/AOT_STATIC_AUDIT.json`. Both platforms compile those same audited
sources; no generated C is committed. The build uses Release mode, no debug
tools, no setup wizard and the OpenBIOS backend only. The command checks that
the executable carries the `<version>+<commit>` build identity and that every
audited unit is linked. It then stages and checks the package:

- Windows executable imports only system DLLs
- mod catalog, checked against the build's own catalog list
- OpenBIOS included
- Playtest Diagnostics defaults match the channel
- no user data

Finally it writes the ZIP or AppImage and its SHA-256 file, then extracts the
artifact and verifies it. The ZIP holds one `MediEvilIIRecomp/` folder. Extract
the whole ZIP into a writable directory and run `MediEvilIIRecomp.exe`.

Linux packaging needs ImageMagick, curl and the usual ELF tools. It downloads
checksum-pinned linuxdeploy and appimagetool releases. Unreferenced bundled
libraries are pruned.

Output is under `dist/`, with a SHA-256 sidecar. The AppImage keeps persistent
data under `$XDG_DATA_HOME/MediEvilIIRecomp` (default
`~/.local/share/MediEvilIIRecomp`), following Tomba's writable-data launcher.
`MEDIEVIL_II_RECOMP_DATA_DIR` overrides that directory. Upgrades refresh bundled
resources while preserving settings, installed mods and saves. Bundled library
paths are not exported into host Browse-dialog processes.

Neither channel uploads anything. `RELEASE_MANIFEST.json` records the channel,
source, framework and recomp-ui commits, version, mod list and payload hashes.
Native Linux/Steam Deck and whole-game testing remain separate qualification
work. Current Linux builds require glibc 2.39 or newer (Ubuntu 24.04, for example);
AppImage packaging does not lower that requirement.

For publication, merge the reviewed enhancement PRs in order with merge commits,
then build and package the resulting `main` revision with `VERSION=0.2.4-alpha`.
Verify all prior PR heads are ancestors, regenerate native code after hook/config
changes, run the game contract suites, and smoke-test the extracted artifacts.
Both package manifests must name that same merged revision and bare version.
Create `v0.2.4-alpha` at that revision, then publish a GitHub prerelease with the
Windows ZIP, Linux AppImage, their SHA-256 sidecars and `RELEASE_NOTES.md`.
Keep the existing stable release as the Latest stable release. Publication is
an explicit maintainer step; the packaging scripts never push tags or upload.
