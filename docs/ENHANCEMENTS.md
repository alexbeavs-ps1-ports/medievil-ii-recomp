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

The framework gitlink is `bf5555c6ae84908a8522e9c411442ab6dbaf4877` (tree `3b92f133cd0aa6f3716060169bba935d31451c26`), a published
`feat/medievil-native-quality-20261003` integration branch on the canonical repository.
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
Separate expanded-RAM arenas hold up to 4096 capture records, 16384 polygon
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
360 targets. It replays the drawing span 0x80050044..0x80050114 with interpolated
camera/model matrices, guarded against unrelated vertices and scene cuts.
Submission is bound to the engine's call of 0x800A101C. CPU/RAM/scratch, GPU/DMA
state and precision shadows are restored after each draw. Guest simulation,
input and audio retain their original cadence; draw cost limits actual throughput.

World Texture Filtering defaults to stable minification on proven OpenGL world
polygons, with nearest and bilinear choices. It decodes current palette colors
before averaging and preserves texture windows, primitive bounds, cutouts and
STP classification. Untracked UI stays nearest. Other backends use bilinear;
disabling the feature restores the Display filter setting.

## Native-quality update (2026-10-03)

Smooth Presentation now interpolates camera/model transforms and replays only
the engine's drawing section, from `0x80050044` to `0x80050114`, followed by
guarded SDK submission inside the render-pass sandbox. Gameplay, audio and
input retain their original timing. The Museum route passed 227 CPU, RAM,
device and VRAM sandbox comparisons with zero mismatches or watchdogs.

Texture Filtering offers Nearest, Bilinear and Stable. Stable uses bounded
derivative-based sampling for tracked world geometry, decodes live palettes
before averaging, preserves cutout/STP classes and respects texture windows
and primitive UV bounds. Untracked sprites/UI remain nearest. The shared real
OpenGL fixture passed 325 checks.

Ten-second comparisons of the same Museum gameplay checkpoint, with
compilation stopped, a 1280x720 window and 5x internal scale (1200 lines):

| Mode | Guest VBlanks/s | Additional geometry draws/s | Static phase residency |
| --- | ---: | ---: | ---: |
| Native / nearest | 59.84 | 0 | 98.93% |
| Interpolated / nearest | 59.05 | 25.32 | 97.90% |
| Interpolated / stable | 59.47 | 25.80 | 98.05% |

The figures are event-counter deltas and host wall-time samples; they do not
establish unique displayed FPS or complete native instruction coverage.
Presentation rate is a target, constrained by replay cost. The shared
`tools/measure_render_quality.py` collects these counters and settings.
New snapshots preserve the BIOS/game handoff latch and invalidate interpolation
histories, including when a cold launch restores directly into gameplay.

The hands-off movie sequence exposed stale pixels around successive movies.
Depth24 presentation consumes CPU VRAM; its clears and copies must update that
same copy while movie framebuffer uploads are deferred. The shared renderer now
preserves that authority and command order through entry, consecutive movies
and return to geometry. The real OpenGL regression fixture failed five checks
before the fix and passes all 167 checks at both 1x and 4x after it. Actual
presentation captures show black movie borders, including the Dan/dragon scene.

The conservative 3x title terrain capture also exhausted its old 8192-polygon
candidate budget and rejected 21 whole cells. The 16384-pointer reservation
retains 10062 candidates across 363 cells with no skips in the observed title
route. The regression fixture retains and cleans up two cells totaling 8300
polygons. Emitted primitive buffers remain 1 MiB each. Close-camera clipping and
later levels still require visual qualification beyond these capacity checks.

The quad funnel computes both triangle winding results before testing either.
Its first branch at `0x8007FCE0` consumes the preceding NCLIP command; the next
quad branch and single-triangle branch consume the latest command. The shared
guarded recovery now retains both results, and this adapter binds that first
consumer explicitly. The real GTE and interpreter tests cover opposite exact
signs even when both native results collapse to zero, together with stale,
near-depth, replay and 4:3 rejection. This preserves the original guest MAC0.
Fresh-boot captures reach the owner's hands-off torch hallway and New Game.
A controlled comparison restores the same complete checkpoint and changes only
the subdivision instruction. Both choices reach that doorway; the small
close-camera edge report still needs a precisely identified visual comparison.

## Native code and level-module relocation

The boot executable and main `MED2.EXE` engine are compiled offline. Named
strings, jump/data tables and padding after `0x800BCA2C` are explicitly excluded
from native producers by an owned-byte hash. The final transform's return and
stack restore are at `0x800BCA24/28`. The latest audit covers seven original/mod
engine views plus title, Museum, Museum boss and Professor lab modules,
40,284 guarded static variants in
thirteen generated files, with all
guards matching their known input bytes. It does not claim full static coverage.

All 24 `RELOCS/*.LVB` files contain plain counted image containers with separate
two-bit relocation streams. The original loader at `0x80070764` allocates the
container, relocates each image through `0x80078328`, then discards the streams.
This path needs relocation, not decompression. Image placement is the allocated
container base plus its header/image offset; it is not the first title's fixed
overlay base. The generic reader preserves opcode fields and accepts relocation
offset zero. All 24 modules matched execution of the original routine at two
different RAM bases, byte for byte: 48 comparisons passed.

The counted-relocated producer compiles original disc images at loader-verified
heap placements, with byte guards and explicit header/data exclusions. Museum
at `0x80131190` and title at `0x80109BA4` are now declared. Live dispatch confirms
the former Museum fallbacks `0x80131444/FC` and title callback `0x8010A804` run
natively. Stock Complete Level also reached Museum boss at `0x8013097C` and
Professor lab at `0x80134150`; their original images, relocation streams and
MIPS/script boundaries now supply two more guarded native producers.
The rebuilt boss and lab modules dispatch their former fallbacks
`0x80130DF4` and `0x80134D94` natively (515 and 566 hits in the sampled arrivals).
Fresh Museum entry still matches the
declared `0x80131190` placement and has no recorded module interpreter fallback
in that route. These observations qualify the sampled placements and entries.
Other placements and modules retain interpreter fallback.
The remaining sampled title fallback `0x8010CD50` is a loop inside the leaf
routine at `0x8010CD04`, whose entry also runs natively (104 observed hits).
It is not evidence of an undiscovered disc function; internal resume/dispatch
coverage must be assessed separately from initial function discovery.
The initial disc discoverer now identifies complete counted containers and
emits relocation metadata instead of guessing a fixed RAM base. Proven loader
callback fields can seed discovery; script/data pointers do not become code
roots merely because they resemble addresses. Level/cutscene coverage
harvesting is tracked by `beads-eio.19.7` and will assess which additional
producers the initial discoverer could have identified automatically.

## Validation and remaining qualification

The latest GCC diagnostic executable cold-restored a newly captured lab state
with the v9 latch intact. Default interpolation, Stable filtering and 5x
internal scale ran at approximately 59.9 guest VBlanks/s over the initial
50 seconds, with 1536 extra render passes and zero pass aborts, watchdogs,
VRAM leaks or span failures. This is a scene-specific smoke check, not a
throughput guarantee for every map or target rate. A separate forced sandbox
verification run passed 292 comparisons with zero mismatches, then hit the
normal starvation watchdog; that diagnostic/restore qualification stays open
under `beads-eio.3.251`. Older checkpoints with a different codegen integrity
key remain rejected; the tests used recaptured checkpoints rather than
weakening compatibility checks.

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
The movie-border flicker is fixed as described above. Gameplay HUD anchoring
and remaining close-camera edge reports remain open. Starting-level movement
and boundary coverage, all view
and mod choices, sustained target throughput, audio, saves, later levels and
exact release packages still require qualification. These findings are tracked
under the central game epic `beads-eio.19`; the intro/masks/HUD task is
`beads-eio.19.4`. This workbench is not a new release qualification receipt.
