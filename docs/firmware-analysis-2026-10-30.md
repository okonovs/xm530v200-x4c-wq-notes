# Offline analysis — 2026-10-30 pass (codec field + sensor profiles)

Continuation of [the 2026-10-29 offline analysis](firmware-analysis-2026-10-29.md).
This pass addresses roadmap item #1 — *where the archived `Sofia` per-channel
codec field gets its value* — and roadmap item #3 — *what the archived sensor
profiles configure* — using copied files only. It did not contact or alter either
camera. A finding about an archived binary is not automatically a finding about
the binary currently installed on either device.

## What was checked

The main-stream codec select in the archived `Sofia` binary reads bit 0 of a
per-channel configuration field. The field address was established at
`0x53c340 + 0x7c` (vma `0x53c3bc`): the codec branch loads it with `ldr r3,
[r3, #0x7c]` where `r3 = 0x53c340` (a global struct captured on the stack and
loaded from a PC-relative literal). Bit 0 selects H.264 when set and H.265 when
clear for the main channel.

## Findings and limits

| Area | Finding | Limit |
| --- | --- | --- |
| Codec field origin | The struct at vma `0x53c340` lives in the **`.bss`** section (`SHT_NOBITS`, zero-initialized), so the field has no value stored in the file at that address. It is written at runtime. | The source of the written value is open. It could come from parsed configuration or from a constant in code; `.bss` placement alone does not distinguish them. |
| Writer | The field is set through calls: a bulk struct initializer copies one source value into `+0x68/+0x6c/+0x14/+0x78/+0x7c/+0x80` before returning. A scan for a constant store to the field found 11 constant writes to the shared `+0x7c` offset, but **0** of them target this struct (their base registers belong to other structs). | The config-key → bit 0 mapping still needs a type-aware (Ghidra) trace; a flat offset scan is insufficient because `+0x7c` is an overloaded struct offset. |
| H.264 capability | A codec field that is runtime/config-populated is **not** evidence of a working encoder. The firmware's **user-space H.264 path is real** (`_h264_create_chn`/`H264_PARSER`/`CEncode`/`AVEnc.SmartH264`; 55 distinct `h264` strings), and a **complete H.264 driver exists in the corpus** (`libxmvenc`/`hx280enc` `open_h264.ko`, drop-in for `xm530_h264.ko`, Hantro 8290/VEPU1, `xmvenc-test` VI harness) — see `h264-investigation.md` and `verify-h264-userpath.py` (dual-decoder PASS). | The *driver* fact is unchanged (no decodable H.264 frame; `hevc.ko` is H.265-only, no `/dev/h264`), but the physical H.264 question is **OPEN**: test by binding a known H.264 driver and encoding from the VI channel, not a closed negative. |

The per-channel codec field is **runtime-populated**. Whether its value comes
from configuration or a constant in code remains open and needs a type-aware
trace of the struct and its callers. This does not establish a working
H.264 path: that still requires captured, decodable H.264 frames from a
specifically authorized live test.

## Sensor profiles (roadmap #3)

The archived `isp` binary (SHA-256 verified before analysis) selects a sensor
from a chip-ID cascade and stores per-sensor callbacks in a registry. The two
primary profiles were compared with a reproducible dual-decoder check.

| Area | Finding | Limit |
| --- | --- | --- |
| Chip-ID cascade | The probe compares chip IDs `0xda4d` (branch `0x3b77c`) and `0xdc42` (branch `0x3b8a4`), plus a `0xcb3e`/`0x801b` discriminator branch that separates `SC223A` from `SC223AP`. | Confirms the *branch points*; the sensor name bound to each branch is not all confirmed. |
| Profile separation | `SC223A` (sensor id 41, init `0x42d00`) and `SC223AP` (sensor id 46, init `0x48700`) store **four** callbacks at registry offsets `+0x28/+0x2c/+0x30/+0x34`. The two profiles share **zero** callbacks (`SC223A {0x42e38,0x42d08,0x42d84,0x42e10}` vs `SC223AP {0x48814,0x48708,0x48778,0x487ec}`). | Confirms the profiles are distinct code, not that each callback's effect is fully understood. |
| Configuration tables | Each profile's `+0x28` callback iterates a distinct `.rodata` table: `SC223A` at vma `0x67eb0` (852 bytes) and `SC223AP` at vma `0x7532c` (744 bytes). The tables differ by SHA-256 and share 0 leading 16-bit entries. | Tables confirmed distinct; see the resolution below on what they contain. |
| Table semantics (resolved) | The helper `0x3c7f4` is an **I2C register-write**: it builds a 16-byte struct and calls `ioctl(/dev/i2c, cmd, &struct)`. The loop feeds the table's two halfwords as `reg` and `value`; the **value halfword is constant per pass** (`ldrh r2, [r3, #2]`, fixed offset), so every register a pass enumerates is written the same value `table[2]`, while the register halfword advances (`ldrh r1, [r3, r6]!`). A sibling `0x3c748` uses `ioctl` command 3. | The tables **are** register/value programming tables (a prior "not a flat reg/val list" wording in this page was an over-correction and is withdrawn). The meaning of `ioctl` commands 1 vs 3 needs the out-of-corpus `/dev/i2c` kernel driver. |

> **Correction (2026-10-30 02:17 UTC):** an earlier sentence in this page that said
> the tables are *not* a flat register/value list was an over-correction. The
> per-profile `.rodata` tables **are** sensor register/value writes delivered
> through `ioctl(/dev/i2c, …)` with the value halfword constant per pass. The
> detail is in the private report `sensor-i2c-writer-2026-10-30.md` and verifier
> `verify-sensor-i2c-writer.py` (10 checks pass in both decoders).

The two primary sensor profiles are therefore **separate configuration sets**, not
one table with a parameter, and each set programs the sensor's registers over I2C
with a uniform value per pass. The kernel semantics of `ioctl` commands 1 vs 3
remain open. The `SC223A` identification for *cels* still relies on earlier live
evidence; the installed `isp` binary has not been hashed against this archive.

> **Correction (2026-10-30 03:24 UTC): the "three profiles" framing is too
> narrow.** A later pass (`sensor-catalog-2026-10-30.md`, verifier
> `verify-sensor-catalog.py`, 13 checks) showed the `isp` sensor cascade
> `0x3b018–0x3b340` compares **16 chip-ID values leading to 15 branch destinations**
> (`0xcb3e 0x9b3a 0x4210 0x5235 0x9a4d 0xcb14 0xca3f 0xcb1c 0xce1f 0xcc1a
> 0xcb5c 0xcc41 0xda2a 0xce39 0xda4d 0xdc42`; `0x9a4d` and `0xda4d` share
> a destination), including a **dynamic I2C-probe fallback** (`0x3b8a4` for chip `0xdc42`/unmatched, matching register
> signatures `0x841`/`0x507`), and rodata **names 33 sensors**
> (SC223A, SC223AP, SC1346, SC401AI, SC430AI, SC4336, SC307E, SC307C, IMX307,
> IMX335, GC3003, MIS2009, MIS4001, F37/F37P, H63/H63P, Q03/Q03P, SC2331,
> SC2335, SC2336/SC2336P, SC3335/SC3338, SC3235, SC120AT, SC1336, SC1345,
> SC500AI, SC5239AI, SC4210, SC200AI, SC035GS, …). `SC223A/SC223AP` are two
> entries in this catalog, not the whole. The earlier open `0xda4d` path maps to
> **SC1346**: chip `0x9a4d`/`0xda4d` (branch `0x3b77c`), id 36, init `0x3e0e8`,
> name ptr `0x3bef4` → vma `0x5b5d5`. The `0xdc42` path uses the dynamic
> fallback, decoded below. Full
> register-table and callback comparisons across the catalog remain open, as
> does which sensor is *fitted* in each camera (needs live readback).
>
> **Completion (2026-10-30 05:06 UTC): the dynamic fallback is decoded.** The
> fallback at `0x3b8a4` reads the sensor's own I2C registers and builds a 16-bit
> signature `signature = (reg 0x0b << 8) | reg 0x0a` (`0x3baa8`/`0x3bab4` read
> regs `0x0a`/`0x0b` via `0x3c748`; `0x3bac8` `orr r0, r0, r11, lsl #8`), then
> maps six signatures to sensor-id and name (verifier `verify-sensor-fallback.py`,
> 4 checks, raw ARM words + format strings):
>
> | signature | sensor id | sensor name |
> | --- | --- | --- |
> | `0x841` | 57 | F37P |
> | `0x507` | 133 | Q03 |
> | `0x843` | 134 | Q03P |
> | `0x848` | 32 | H63P |
> | `0xa63` | 31 | H63 |
> | `0xf37` | — (name-only) | F37 |
>
> So the firmware recognizes **F37/F37P/Q03/Q03P/H63/H63P** by I2C self-ID
> signature — a broader catalog than the two primary profiles first analyzed.
> The cascade and six fallback mappings are decoded; the other profiles'
> register tables and callbacks still need comparison. Which sensor is *fitted*
> in each camera also needs owner-approved live readback.

## ISP detail ranges (roadmap #4 / O4)

The 11 hidden ISP detail families (3D-NR/YcNR/Bpu3DNr/Vpss3DNr/DeFog/VpssDeFog/
DRC/Sharpen/Sharpen1/Ltm/LDC) are not user-tunable config keys, so they are
applied through the firmware's own `XM_MPI_ISP_Set*Attr` API. Their *safe*
parameter ranges are the firmware's own SDK, which ships in the corpus
(`references/openxiongmai/.../xm_comm_isp.h`, plus `extCfg.c`/`com.c` sample
code):

- 8-bit auto strength arrays are `Range [0, 0xFF]`: Sharpen D/Ud/Kd, 2DNR/YCNR
  `Thresh`, DyDPC/DRC auto `Sth`, and `u8GlobalSth`.
- DRC `au16ToneMappingValue[DRC_IDX_NUM]` is `Range [0x0, 0xffff]` (16-bit).
- `ISP_AUTO_STENGTH_NUM = 16` (each auto family is a 16-entry ISO/strength
  array); DeFog uses a 65-entry `au8DefogLut` plus `bUserLutEnable` `[0,1]`.
- The firmware's safe application pattern (from `extCfg.c`) is: `Get` the attr,
  overwrite only the 16-entry auto array, copy level 7 into indices 8–15, then
  `Set`, treating a non-`XM_SUCCESS` return as failure.
- The `TB isp` binary uses the same SDK API generation (7/8 checked families' error
  strings match the header's structs).
- The live config `Acutance` = 3848 (`0xF08`) is **> 0xFF**, so it is *not* one of
  these 8-bit sharpen/DNR fields; it is a separate color control whose own range
  is still unestablished.

> **Gate.** These are the SDK's documented *field* ranges, i.e. a crash/safety
> bound — not a measured image-quality recommendation. Applying any value still
> needs the owner-authorized A/B test (O4), and a value in range can still
> degrade the image. Verifier `verify-isp-safe-ranges.py`.

## Handoff (roadmap #6)

A private open-item + evidence-gate ledger (`reproducible-handoff-2026-10-30.md`)
closes roadmap #6: it maps each GOAL deliverable to its evidence and the 17
corpus verifiers (all PASS), and lists every open item with the evidence gate
that closes it. This run (18th pass) **corrected** the earlier "offline queue
empty / H.264 blocked" verdict: the firmware's user-space H.264 path is real and
a complete H.264 driver exists in the reference corpus, so the V200 physical
H.264 question (O2) is **open and testable by binding that driver and encoding
from the VI channel** (`xmvenc-test -v 0`), not merely a missing document. This
corroborates the standing editorial note that a compatible H.264 block cannot be
excluded from the driver facts alone. O4 (safe ISP ranges) is closed; O1/O3/O5/
O6/O7 and the O2 bind+encode test remain live-gated. See `h264-investigation.md`
and the roadmap `Progress` section for the O1–O7 gate table and the (revised)
recommendation to re-enable the hourly job for that live test.

## Provenance

- Static corpus: the archived TB `Sofia` and `isp` binaries, SHA-256 verified
  before analysis.
  A private verifier reconstructs the field address, the `.bss` section
  membership, the PC-relative references, and the bulk-initializer, and prints
  the constant-write scan.
- Firmware, dumps, executable maintenance payloads, credentials, and device-
  specific backups remain outside this repository.
- Work tracking and acceptance criteria: [research roadmap](research-roadmap.md).
