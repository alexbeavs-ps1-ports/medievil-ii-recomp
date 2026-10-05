# Resident Loading

`medievil2.enhancement.seamless-loading` is enabled by default for the verified
USA SCUS-94564 disc. Disable **Resident Loading** in Mods to restore the original
loading path. This feature minimizes asset waits without increasing guest CD
speed, advancing gameplay clocks, or changing movie/audio pacing.

## Preparation

The native preparation step reads the effective mounted `PROJFILE.MWD` archive
and 24 `RELOCS/*.LVB` files. The archive contains 66 PP20-compressed parent
containers; the level modules themselves are uncompressed. All file sizes,
sector tails and hashes are verified before enabling the adapter. The effective
disc is checked again on warm launches so sector patches cannot be hidden by a
stock cache. Changed assets select the original loader.

The mod retains 46,634,340 bytes (about 44.5 MiB) in host memory, including the
original data and decompressed containers. It stores verified decoded bytes in
`MediEvilIIRecomp/seamless` under the platform cache directory (`LOCALAPPDATA`
on Windows). The cache key includes the resolved mod-plan fingerprint.
Truncated, corrupt or extended cache files are rebuilt. Cache publication uses
a process-specific temporary file and atomic replacement. An unwritable cache
still permits resident loading from memory for that launch.

Cache files are prepared from the player's mounted disc at runtime. No asset
bytes, disc images, initialized gameplay states or RAM captures are distributed.
The checked-in catalog contains only original extents, sizes, decoder scratch
metadata and SHA-256 hashes.

## Guest contracts

The adapter chains the shared dispatch hook and guards the original function
words, callers, RAM ranges and asset bounds. It runs with OpenBIOS's LLE kernel.
This uses the dispatch extension point to replace game asset services; it does
not require enabling BIOS-call HLE.

| Service | Accelerated behavior | Original behavior retained |
| --- | --- | --- |
| `800AA244` | Copy a verified resident level file and finish its synchronous read | File lookup, heap allocation, relocation, callback parsing and heap shrinking |
| `800A0654` / `800A06F0` | Copy the bounded archive extent; return one successful completion | Original asset queue, allocation, postprocessing and type callbacks |
| `800ADFB4`, archive caller only | Copy the predecoded PP20 output and reproduce final bit-reader scratch | In-place destination/prefix, allocation ownership and later asset initialization |

The archive completion record is stored in snapshot-backed Expansion 1 memory.
It is consumed once, matches its descriptor and GP, and survives an in-flight
save/restore. Asset writes invalidate executable pages and publish page changes
so native overlay matching sees the new bytes. The game owns GPU uploads, sound
bank initialization, teardown and module relocation.

Loading polls no longer wait for supported resident reads. Authored fades and
cutscenes remain in the game. The disc-error display/retry path is preserved;
it is not treated as a loading screen to skip. Movies, XA and CD audio remain
on their ordinary paths. Unrecognized code, callers or buffers execute the
original routines.

## Validation

- All 66 original PP20 streams match the original MIPS decoder both with a
  separate destination and with the game's in-place prefix: 132 comparisons.
  Output bytes, return value and final GP bit-reader scratch match.
- Loader fixture covers successful completion, consuming it once, restored
  guest metadata, malformed locations, code/destination bounds and native fallback.
- Store fixture covers cold/warm preparation, truncation, payload corruption,
  extra cache tails, changed mod plans, changed effective assets and extent bounds.

Two isolated builds of the same full visual-enhancement stack, with this mod on
and off, reached the title, New Game, Museum intro and controllable Museum scene.
The mod served 33 archive reads, two module reads and five PP20 decompressions
through Museum entry, with matching read/completion counts and no adapter
fallbacks. Save/restore was checked in the modded instance.

In the first side-by-side New Game comparison, the measured transition window
from the first loading edge to the last CD setup edge before the Museum intro
was 8,304 ms with the original loader and 1,382 ms with resident loading. The
corresponding guest-frame spans were 539 and 109. Debug turbo was off during
these windows. These are one-run observations with two private instances,
not a repeated benchmark or a promise for later levels. Authored fades remain.
This does not claim a complete playthrough or the removal of every transition.

Developer validation (requires your owned USA disc and native compilers):

```sh
python -m unittest discover -s tests -p 'test_*contract.py'
c++ -std=c++17 -O2 tests/pp20_driver.cpp src/mods/medievil2_pp20.cpp -o pp20-driver
python tools/validate_medievil2_pp20.py --disc 'disc/MediEvil II (USA).cue' \
  --decoder ./pp20-driver --receipt /tmp/medievil2-pp20.json
python tools/validate_medievil2_store.py --disc 'disc/MediEvil II (USA).cue' \
  --cxx c++ --cc cc
```

The MIPS oracle needs the Python `unicorn` package. `MEDIEVIL2_SEAMLESS_TRACE=1`
logs supported reads for private QA; `mod_counters` exposes read, decode,
completion and fallback counts without enabling trace. `MEDIEVIL2_SEAMLESS_CACHE`
can select an isolated cache directory. These are diagnostic overrides, not
player settings.
