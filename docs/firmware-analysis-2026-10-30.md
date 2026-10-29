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
| Codec field origin | The struct at vma `0x53c340` lives in the **`.bss`** section (`SHT_NOBITS`, zero-initialized). By loader definition it has no initial data, so the codec field **cannot be a compiled-in constant**; it is written at runtime. | This proves "not hardcoded"; it does **not** yet prove *which* configuration key sets bit 0. |
| Writer | The field is set through calls: a bulk struct initializer copies one source value into `+0x68/+0x6c/+0x14/+0x78/+0x7c/+0x80` before returning. A scan for a constant store to the field found 11 constant writes to the shared `+0x7c` offset, but **0** of them target this struct (their base registers belong to other structs). | The config-key → bit 0 mapping still needs a type-aware (Ghidra) trace; a flat offset scan is insufficient because `+0x7c` is an overloaded struct offset. |
| H.264 capability | A codec field that is runtime/config-populated is **not** evidence of a working encoder. | The hardware/driver blocker is unchanged: no decodable H.264 frame has been produced, and a correct configuration still needs a driver that exists on this board. |

The per-channel codec field is therefore **runtime-populated, not hardcoded**.
The exact configuration key that sets its bit 0 remains open and needs a
type-aware trace of the struct and its callers. This does not establish a working
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
| Configuration tables | Each profile's `+0x28` callback iterates a distinct `.rodata` table: `SC223A` at vma `0x67eb0` (852 bytes) and `SC223AP` at vma `0x7532c` (744 bytes). The tables differ by SHA-256 and share 0 leading 16-bit entries. | The tables are **not** a flat register/value I2C list. |
| Table semantics | The loop reads one *advancing* halfword (`ldrh r1, [r3, r6]!`, `r6 += 4`) and one *constant-offset* halfword (`ldrh r2, [r3, #2]`), then calls the writer `0x3c7f4` (a per-sensor struct writer, stride 56). | The meaning of each halfword is **uncertain**; a prior "flat register table" framing is **not** confirmed and is withdrawn. Exact semantics need the `0x3c7f4` decompilation. |

The two primary sensor profiles are therefore **separate configuration sets**, not
one table with a parameter. Which sensor each branch binds to, and what each
table halfword means, remain open. The `SC223A` identification for *cels* still
relies on earlier live evidence; the installed `isp` binary has not been hashed
against this archive.

## Provenance

- Static corpus: the archived TB `Sofia` and `isp` binaries, SHA-256 verified
  before analysis.
  A private verifier reconstructs the field address, the `.bss` section
  membership, the PC-relative references, and the bulk-initializer, and prints
  the constant-write scan.
- Firmware, dumps, executable maintenance payloads, credentials, and device-
  specific backups remain outside this repository.
- Work tracking and acceptance criteria: [research roadmap](research-roadmap.md).
