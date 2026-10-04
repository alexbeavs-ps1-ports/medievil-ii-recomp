# MediEvil II enhancement workbench

Current framework: canonical upstream `master` at `a916ed52e00f364a8615b97cd597858aa7f385dc`
(tree `7293a6e386ffdcf81ac74adc6d26a5505f7319d0`). The shared PRs 485, 486 and 498-501
are merged. The combined framework passed the bounded MMX6, Tomba, Tomba 2
and Ape Escape regression suite plus focused runtime, codegen and real-GL tests.
Historical integration pins below describe earlier work; the gitlink and
`project-manifest.toml` identify the current dependency. This does not expand
the gameplay coverage claims or qualify a release package.

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

The earlier framework integration was `084719fc56a606f9aca9222ad30066f525b7b123` (tree `c701ccd201597271965e4cd448a03e72a7ee4a2d`), a published
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
the subdivision instruction. The owner identified premature disappearance of
the hallway side walls. Captured GP0 packets then exposed projected wall
triangles taller than the PS1's 511-pixel limit, rejected before mirroring into
the additional view. The shared frontend admits these only with exact packet
provenance, positive depth, unsaturated Y and bounded projected X. Admitted
faces use the same normal OpenGL geometry in the canonical and wide passes,
with coherent GPU readback. The first margin-only implementation left a wall
split at the canonical center-copy boundary; owner feedback exposed that
remaining defect, and the revised path removes its special clipping/batching.
Ordinary mod-generated triangles keep their existing backend behavior.
The real GL regression passes 182 checks at both 1x and 4x, including a wall
crossing the center-copy boundary, painter order, readback coherence and
ordinary lines/textured draws after recovered faces. The previous path fails
five targeted checks at each scale. At 32:11 the exact hallway sample counted
775 recovered triangles with Stable filtering, interpolation and 5x resolution.
The extended diagnostic passed 835 sandbox state comparisons with no mismatch,
abort, watchdog, VRAM leak or span failure. The owner still observed folding
at the furthest edges after this change.

Submission-time tracing then isolated a separate near-camera failure. A wall
had signed corner depths 243, -158, 530 and 129 with H=300. The negative corner
had already become SZ=0; the 129-depth corner used the hardware divider cap.
Both effects destroy the projected wall shape before horizontal recovery.
The adapter now explicitly enables signed homogeneous projection transport
through PGXP. The GL path clips camera-crossing textured faces and their UVs
before dividing by depth, retaining the visible part in both canonical and
wide passes. The source words must all have exact intact provenance; partial
writes, stale words, 4:3 and software rendering retain their existing paths.
The guest GTE registers and its original culling branches remain unchanged.

The GL fixture now passes 197 checks at both 1x and 4x, including a crossing
wall, an entirely behind-camera face, painter order and CPU/GPU readback.
Six focused runtime tests pass. The diagnostic build passed 440 sandbox
comparisons with no mismatch, abort, watchdog, VRAM leak or span failure.
At 32:11, two repeated captures retain the walls where the prior sequence
revealed triangular holes and the purple scene beyond. The owner validated
the repeated hallway sequence: "Walls stay intact." The default run completed
4573 additional render passes with no abort, watchdog, VRAM leak or span
failure. This resolves the reported hallway defect; later-level qualification
stays open.

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

## Projector coverage sweep (2026-10-03)

The expanded sweep reached all 22 level modules at observed heap placements:
the previous title, Museum, Museum boss and Professor lab, plus Kensington,
Freakshow, both Greenwich maps, Kew Gardens, Dankenstein, Iron Slugger, Wulfrun
Hall, the Count, Whitechapel, Sewers, all three Time Machine maps, both
Cathedral Spires maps, the Demon, and the second Kensington map. This is
module-load and bounded arrival/input coverage, not a completed playthrough.
Puzzle branches, every boss phase, every cutscene, and other allocation
histories remain outside this receipt. `CREDITS.LVB` and `FLAG.LVB` were not
reached by the level sweep.

The profile now declares 29 original-disc images (seven engine views plus
22 level modules). Generation publishes 75,043 guarded variants in 31 files,
up from 40,284 variants for the initial four-module profile. All generated
guards match their original inputs. The live copies of each new declared text
range match the original relocated disc image exactly; mutable data outside
text is not used as a native input. See [the source-only coverage receipt](../aot/coverage-projector.json).

The rebuilt-native sweep passed all 18 new module samples: declared text
matched the original, native dispatch was observed, and no module-address
interpreter fallbacks or extra-pass aborts/watchdogs/VRAM leaks/span failures
were recorded after the sewer callback correction. The correction changed
only the sewer translation unit and dispatch table; the other generated
module units were byte-identical to the swept build. The source receipt
records per-module results and the targeted callback retest. This verifies
these arrival/input samples, not a complete playthrough.

A limited follow-up entered Kensington from the hub and used the stock
Pause > Exit Level flow back to the Professor lab. Both loaded at their
already declared addresses with exact original text and zero module
interpreter fallbacks during the post-arrival samples. Death/retry and
actual memory-card save/reload remain unqualified; further sampling was
deferred when the owner requested upstream delivery.

For repeatable QA, use a private runtime/save directory, enable the built-in
Cheats menu, select Invulnerability and Open All Levels, finish the first
Professor dialogue, and stand on the projector plate. Save a runtime checkpoint
with Kensington selected. The [USA cheat reference](https://gamehacking.org/game/89269)
documents the menu-enable byte at `0x800D36BD`; these are diagnostic changes,
not enabled product defaults. Then, for example:

```powershell
python tools/harvest_projector_coverage.py --port 46421 --slot 0 --output analysis/native-coverage 0 1 2 level:26
```

Visible routes `0..12` advance from Kensington to the Demon. The stock
projector filters out records whose flags contain bit `0x2`, even after Open
All Levels. Routes `level:26..level:30` replace only the target byte of the
selected Kensington row (`0x800E175A`, originally 18) with an existing hidden
map id. The original selection, teardown, loader, and relocation code still
run. No opcode patch or replacement asset is involved. The helper verifies
the USA engine signature, selected row, and checkpoint completion, and refuses
to overwrite a finished sample. Captures contain game data and belong only in
ignored `analysis/` or a private external directory. Longer loading movies
may require a larger `--arrival-frames` sample.

Initial discovery can identify all counted containers and relocation streams
before play; the shared discoverer now does that. Proven loader callback
fields also expose initialization functions that have no stack prologue.
The new profiles retain leading and trailing frameless functions, including
instructions before a later stack decrement. A first/last-prologue heuristic
would incorrectly exclude real code. The discoverer already supports those
leaves; the fix here is accurate source boundaries and producer declarations.
The sewer sweep also exposed a callback whose `LUI`/`LW` prefix appears
before its stack decrement at the start of the text region. The generic
prelude scan requires a preceding return, and the dense pointer-table scan
requires adjacent callbacks; this entry has neither. Its original relocation
field at image `+0x10D80` points to `+0x4F0`, and engine `0x8004471C` calls it.
Declaring that callback field supplies the true entry without guessing or
using captured RAM as a producer. Relocation-backed callback discovery could
recover this case earlier; that broader inference remains future work.

The remaining structural limitation is that native overlay code is tied to
verified load addresses. Relocation-independent native overlays would avoid
declaring unseen heap placements, but are not implemented by this change.

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
The movie-border flicker and reported title-hallway wall folding are fixed as
described above. Gameplay HUD anchoring remains open. Starting-level movement
and boundary coverage, all view
and mod choices, sustained target throughput, audio, saves, later levels and
exact release packages still require qualification. These findings are tracked
under the central game epic `beads-eio.19`; the intro/masks/HUD task is
`beads-eio.19.4`. This workbench is not a new release qualification receipt.

## Upstream review series

Review game PRs [1](https://github.com/alexbeavs-ps1-ports/medievil-ii-recomp/pull/1),
[2](https://github.com/alexbeavs-ps1-ports/medievil-ii-recomp/pull/2) and
[3](https://github.com/alexbeavs-ps1-ports/medievil-ii-recomp/pull/3) in that order.
The shared framework changes are stacked as
[485](https://github.com/RetroPortingToolKit/psxrecomp/pull/485),
[486](https://github.com/RetroPortingToolKit/psxrecomp/pull/486),
[498](https://github.com/RetroPortingToolKit/psxrecomp/pull/498),
[499](https://github.com/RetroPortingToolKit/psxrecomp/pull/499),
[500](https://github.com/RetroPortingToolKit/psxrecomp/pull/500) and
[501](https://github.com/RetroPortingToolKit/psxrecomp/pull/501).
The game now pins the merged canonical master recorded above, including newer
upstream PGXP session and projection behavior.
The bounded coverage and remaining qualification limits above still apply.
