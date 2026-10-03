# MediEvil II enhancement workbench

The 2026-10-02 workbench starts from Alex's
[MediEvil II repository](https://github.com/alexbeavs-ps1-ports/medievil-ii-recomp)
at `9837687` on `feat/medievil-ii-enhancements`. The intent is the same
OpenBIOS, ahead-of-time recompilation and default-on visual enhancement work
as MediEvil, with independent verification of this game's loader and renderer.
This is setup and assessment evidence, not a gameplay or release receipt.

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

The workbench bumps PSXRecomp from `40ce478` to the canonical upstream master
observed during setup, `6b9a2d49ac76c2dbe791dfa9863467ceff4e119f`. The existing
recomp-ui pin is retained. Framework and project manifests describe this exact
source, but no build or gameplay qualification of the new pin is claimed.

OpenBIOS shell skipping, the shared PGXP plugin, internal resolution controls
and FLIP-sourced motion-adaptive presentation can reuse framework facilities.
The intended presentation choices are Display by default and 60, 120, 144,
240 and 360 targets, retaining original guest timing. Temporal blending does
not imply new simulation frames or guaranteed sustained target throughput.
The reusable renderer/PGXP startup fixes from the first title are available in
framework PRs [483](https://github.com/RetroPortingToolKit/psxrecomp/pull/483)
and [484](https://github.com/RetroPortingToolKit/psxrecomp/pull/484).

## Game-specific work before visual defaults

The disc contains 24 `RELOCS/*.LVB` modules. Each sampled inventory header
starts with word 1, and all 24 are rejected by the existing two-bit tagged
relocation parser. This establishes that the existing method cannot simply
be selected. It does not establish the compression or relocation algorithm,
image destinations, or executable/data boundaries. Recover those facts from
the original loader and verify them against live memory before declaring
guarded AOT producers. The first title's fixed overlay base is not evidence
for this game.

Adaptive Fit, extended terrain distance, larger capture/primitive arenas and
subdivision bypass also need this game's transform, culling and ownership
paths identified. Do not copy the original MediEvil instruction addresses,
heap layout, capture-cell format or subdivision patches into this title.

The immediate qualification route is OpenBIOS startup, intro, title menu and
the starting level with byte-verified native engine dispatch. Then qualify
aspect changes, near-plane/side-boundary geometry, mod toggles and sustained
presentation throughput before describing the visual work as playable.
Audio, saves, later levels and multi-platform package coverage need separate
evidence. Work is tracked under the central game epic `beads-eio.19`.
