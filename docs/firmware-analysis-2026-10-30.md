# Per-channel codec field — 2026-10-30 offline pass

Continuation of [the 2026-10-29 offline analysis](firmware-analysis-2026-10-29.md).
This pass addresses roadmap item #1 — *where the archived `Sofia` per-channel
codec field gets its value* — using copied files only. It did not contact or
alter either camera. A finding about an archived binary is not automatically a
finding about the binary currently installed on either device.

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

## Provenance

- Static corpus: the archived TB `Sofia` binary, SHA-256 verified before analysis.
  A private verifier reconstructs the field address, the `.bss` section
  membership, the PC-relative references, and the bulk-initializer, and prints
  the constant-write scan.
- Firmware, dumps, executable maintenance payloads, credentials, and device-
  specific backups remain outside this repository.
- Work tracking and acceptance criteria: [research roadmap](research-roadmap.md).
