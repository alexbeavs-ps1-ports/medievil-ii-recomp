# MediEvil II enhancement workbench

The 2026-10-02 workbench starts from Alex's
[MediEvil II repository](https://github.com/alexbeavs-ps1-ports/medievil-ii-recomp)
at `9837687` on `feat/medievil-ii-enhancements`. The intent is the same
OpenBIOS, ahead-of-time recompilation and default-on visual enhancement work
as MediEvil, with independent verification of this game's loader and renderer.
This records the enhancement workbench and diagnostic evidence. Full gameplay
and release qualification remain in progress.

## Owned disc verification

The supplied USA CUE/BIN was copied into the ignored `disc/` directory. It is
a merged two-track image; the original CUE retains both the MODE2 data track
and AUDIO track. No conversion, splitting or source modification was needed.

| Input | Verified value |
| --- | --- |
| Serial | SCUS-94564 |
| Merged BIN bytes | 483874608 |
| SHA-256, matching source and copied BIN | dfcb8602274c70f59551e820d0f4026b0213fd68b7d2a385189d37eb767b2d97 |
| SHA-1, matching existing compatibility identity | 15d13c5ac5d9314500310ccef44619f5ac08f736 |
| MD5, matching existing compatibility identity | c64e9bde313a5bfa8b38bea0802977bd |
| Boot executable SHA-256, matching existing disc probe | b26ec73e4a770b04990b8b62b3362d19b9075b2f0845e3b00f228b2cd4f4fbe9 |

`SCUS_945.64` and `MED2.EXE` were extracted without modifying their bytes.
Disc files and the private inventory remain ignored and are not committed.

## Baseline and reusable framework work

The existing source selects OpenGL at 4:3, disables OpenBIOS, and supplies
199 boot-executable seeds. It has no game-owned enhancement plugins or AOT
level-module profile. The boot executable loads at `0x801B0000`, enters at
`0x801B5220`, and declares a `0x23800`-byte body. The separate `MED2.EXE`
loads at `0x80010000`, enters at `0x800A757C`, and declares a `0xDF000`-byte
body. Its file SHA-256 is
`d1005710982394f469dcf4786af682fd9c49896dce387fb76cb1631366ffa1c1`.

The framework gitlink is `3a718531cd77183beb7520312ed03497eae11e9b` (tree `197e430598b2c32baa7f46523c86ec8652fb8d4b`), a published
`feat/medievil-ii-framework` integration branch on the canonical repository.
It combines upstream master `6b9a2d49ac76c2dbe791dfa9863467ceff4e119f` with the
shared guarded projection and PGXP startup fixes from framework PRs
[483](https://github.com/RetroPortingToolKit/psxrecomp/pull/483) and
[484](https://github.com/RetroPortingToolKit/psxrecomp/pull/484), plus the counted
relocation reader from [485](https://github.com/RetroPortingToolKit/psxrecomp/pull/485).
Owned cutscene panels, radial masks and guarded centered text are proposed
separately in [486](https://github.com/RetroPortingToolKit/psxrecomp/pull/486).
The recomp-ui gitlink is `03d58aa098fc4c2ca944ee43659cb45ce586362a`, which supplies the API fields required by
this runtime. Submodule gitlinks remain authoritative; the manifests record
those exact pins.

The source-kit wrapper stages the adapter sources, AOT profile, source mod
catalog, assets and enhancement notes. It uses the shared OpenBIOS staging
policy and removes obsolete retail-only arguments unsupported by this pin.
Generated engine C remains a local Generate output. The wrapper explicitly
records the static-engine/runtime-module contract instead of requiring an
unrelated dynamic overlay cache. These source-kit changes do not replace
the pending exact-package qualification.

## Default enhancements

OpenBIOS replaces the default retail BIOS selection and skips the startup
shell while retaining its LLE kernel. The PGXP package owns geometry,
perspective-texture and CPU-precision defaults so its off switch still works.
The 1080p preset uses the next whole multiplier of the 240-line reference:
the diagnostic renderer reports 5x / 1200 internal lines.

Fit uses the shared native-wide renderer and follows the window aspect with
a 4:3 minimum. Fixed 4:3, 16:9, 21:9 and 32:9 views are available. The game
adapter binds this title's NCLIP consumers and packed-coordinate reject by
full instruction words. The generic corner heuristic remains disabled:
world polygons must not be mistaken for HUD.

The terrain adapter emits this game's twelve-byte capture records, retains
byte-6 source-cell marks and cleanup, and selects a conservative horizontal
footprint that does not reject tall walls using ground-plane intersections.
Separate expanded-RAM arenas hold up to 4096 capture records, 8192 polygon
marks and two 1 MiB primitive buffers. The original terrain and primitive heap
allocations remain owned by their guest teardown routines. Near, depth,
winding and vertical rejection remain in the original polygon funnel.

Draw distance defaults to 3x, with Original and 2x choices. The adapter retains
the original 128-entry fog tables and their allocations: the renderer copies
them into fixed stack buffers. Guarded disc variants instead enlarge depth
steps and the OT reach/shift while keeping its bucket count. Both depth and
brightness tables are rebuilt. Authored values are restored before the guest
advances transitions, avoiding repeated multiplication across frames or states.
This contract is bound to the verified 8192-unit retail terrain viewport.

Subdivision bypass defaults on. A guarded CD instruction replacement disables
area-selected subdivision; turning it off selects the original instruction.
All distance/subdivision combinations have independent audited AOT input
views. Executable RAM is not rewritten on each launch.

The frame-rate package defaults to Display and offers 60, 120, 144, 240 and
360 targets. It uses FLIP-sourced motion-adaptive temporal presentation while
retaining original guest timing. This is image interpolation, not additional
simulation frames or motion vectors; target choices do not promise throughput.

## Native code and level-module relocation

The boot executable and main `MED2.EXE` engine are compiled offline. Named
strings, jump/data tables and padding after `0x800BCA2C` are explicitly excluded
from native producers by an owned-byte hash. The final transform's return and
stack restore are at `0x800BCA24/28`. The latest audit covers seven original/mod
engine views, 32,880 guarded static variants in nine generated files, with all
guards matching their known input bytes. It does not claim full static coverage.

All 24 `RELOCS/*.LVB` files contain plain counted image containers with separate
two-bit relocation streams. The original loader at `0x80070764` allocates the
container, relocates each image through `0x80078328`, then discards the streams.
This path needs relocation, not decompression. Image placement is the allocated
container base plus its header/image offset; it is not the first title's fixed
overlay base. The generic reader preserves opcode fields and accepts relocation
offset zero. All 24 modules matched execution of the original routine at two
different RAM bases, byte for byte: 48 comparisons passed.

Heap placement and executable/data boundaries still need native producer and
dispatch integration. The decoder alone does not make those modules AOT-native;
live module fallback remains part of the runtime.

## Validation and remaining qualification

The Clang diagnostic build linked successfully with OpenBIOS, PGXP and the
shared presentation service. Live engine dispatch, precision hits, adaptive
projection and guarded winding recovery were observed. Expanded title capture
recorded 254 cells / 5359 candidate polygons with zero budget skips in the
distance build, exceeding the original 100-cell / 2000-polygon limits. The
retail 8192-unit reach/shift 1 became 32768/shift 3 while preserving the
128-entry fog allocations. Starting-level visual qualification remains open.

The isolated C contract fixture verifies capture record layout, tall-cell
retention, shared-cell deduplication, budget limits, cleanup, original heap
ownership, bounded fallback, primitive-cursor preservation, fixed fog table
ownership and noncompounding distance transitions:

```sh
python -m unittest discover -s tests -p test_terrain_contract.py
```

Owner playtesting reports intro flickering and cutscene bars/circular fade
masks confined to the 4:3 center. The screen-layout adapter binds the original
bar and 32-segment iris producers and tags completed packets at DMA submission.
Panels extend into the reveal; both iris rings scale together around the
display centre. The original transition phase and guest packet bytes stay
unchanged. The subtitle glyph producer is bound to its font object and tags
text as centred screen space, retaining its original proportions. The rebuilt
OpenBIOS diagnostic reached the museum intro through the title's New Game
selection. Its letterbox bars cover the full 16:9, 21:9 and 32:9 reveal.
Restoring the same caption checkpoint at 4:3 and all three wide views produced
identical source glyph XY/UV/size packets. Measured text bounds remained
294 by 28 native pixels, with the original one-pixel centre offset, at every
aspect. This qualifies that subtitle and panel path; it does not qualify all
fonts or gameplay HUD widgets. Circular fade transitions still need live
qualification. Both asset-free contract tests pass, including line breaks,
extended font characters, font ownership and recycled-command rejection.
Gameplay HUD anchoring and the cause of the reported flicker remain open. Starting-level movement and boundary coverage, all view
and mod choices, sustained target throughput, audio, saves, later levels and
exact release packages still require qualification. These findings are tracked
under the central game epic `beads-eio.19`; the intro/masks/HUD task is
`beads-eio.19.4`. This workbench is not a new release qualification receipt.
