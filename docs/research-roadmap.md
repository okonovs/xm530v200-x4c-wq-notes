# XM530V200 research roadmap

This is the public work queue for the owner-controlled X4C-WQ firmware investigation. The private research workspace holds archived firmware, full disassembly, device backups, verifier output, and `outputs/STATUS.md`. This repository holds reviewed prose and safe reproduction notes only.

## Outcome

Explain the actual sensor-to-stream pipeline and decide whether the main stream can produce real H.264 on this board. Close a question only with cited binary or device evidence and a reproducible check. Mark unresolved hardware and runtime behavior explicitly. A menu entry, configuration readback, or SDP is not a frame test.

## Work queue

| Priority | Question | Done when |
| --- | --- | --- |
| 1 | Where does the archived `Sofia` per-channel codec field get its value? | A type-aware call and data-flow trace identifies the writer, source configuration or constant, relevant branch, binary offsets, and a repeatable verifier. Offset-only scans across unrelated structs do not count. |
| 2 | What precisely blocks H.264 on the tested configuration? | Reconcile the user-space branch, loaded drivers, generic-driver mismatch, and previous live failure into one evidence ledger. State separately what is known about the shipped firmware and what remains unknown about physical silicon. |
| 3 | What do the three archived sensor profiles actually configure? | Compare their register tables and callbacks with hashes and decoded values. Distinguish the profiles supported by firmware from sensors measured in each camera. |
| 4 | Which ISP controls affect image detail? | Trace denoise, defog, sharpening, and scaling parameter paths to their actual call sites and value sources. Report safe ranges only when the relevant structure and behavior are established. |
| 5 | What does the audio path capture before transport encoding? | Map the source rate, processing stages, and output codec from archived code and earlier measurements; state which improvements require a live measurement. |
| 6 | What remains for a reproducible handoff? | Review the firmware inventory, boot/media map, H.264 feasibility report, sensor/ISP report, security/persistence map, and verifier instructions against the private goal. List every open item and its evidence gate. |

## Progress (as of 2026-10-30, 13 offline passes)

- **#1 (codec field):** the main-stream codec field in archived `Sofia`
   (vma `0x53c3bc`) lives in `.bss` and is written at runtime. Its source is
   unresolved: the writer could copy a parsed setting or a constant supplied by
   code. A type-aware trace must establish which.
- **#3 (sensor profiles) — reframed.** The firmware does **not** have “three
   profiles.” Its sensor cascade (`isp`, `0x3b018–0x3b340`) has **16 chip-ID
   comparison values leading to 15 branch destinations** (two values converge),
   including a **dynamic I2C-probe fallback** (`0x3b8a4`, for chip `0xdc42`/
   unmatched, matching register signatures `0x841`/`0x507`), and rodata **names 33
   sensors** (SC223A, SC223AP, SC1346, SC401AI, SC430AI, IMX307, IMX335, GC3003,
   MIS2009, …). The earlier open `0xda4d` path maps to **SC1346**
   (chip `0x9a4d`/`0xda4d`, sensor id 36, init `0x3e0e8`); `0xdc42` uses the
   fallback. The compared profile tables are I2C writes via `ioctl(/dev/i2c, …)`
   (value constant per pass). The dynamic fallback, decoded at 05:06 UTC, builds a 16-bit I2C
    signature `(reg 0x0b<<8)|reg 0x0a` and maps six signatures to sensors
     (`0x841→F37P`, `0x507→Q03`, `0x843→Q03P`, `0x848→H63P`, `0xa63→H63`,
      `0xf37→F37`, verifier `verify-sensor-fallback.py`). The cascade and fallback
     are mapped; comparison of every profile's register table and callbacks remains
     open, as does *which sensor is fitted in each camera* (needs live readback).
- #4 is confirmed by `verify-isp-detail-controls.py`: a closed 9-key user-tunable
     registry (`VideoColorParam` + 7 leaves incl. `Acutance`); the 11 hidden ISP
     detail families are absent from the config parser. #2, #3, #5, #6 retain
     offline work.

Full detail and reproducible checks are in the dated analysis page
`firmware-analysis-2026-10-30.md` and the private workspace.

## Hourly research pass

1. Read the private goal, handoff, latest status, and this queue. Choose the highest-priority unfinished question that can advance with the copied corpus.
2. Make one bounded, evidence-backed change. Save the command, hash, offset or symbol, result, failed hypothesis, and next action in the private report and `outputs/STATUS.md`. Check arithmetic and disassembly claims independently. Finish with a truthful handoff before the run deadline.
3. Update this public repository only when a finding has a reproducible check and has been reviewed for uncertainty and disclosure. Keep the dated analysis page concise; change the queue as questions close. Commit and push only the intended Markdown files. If there is no new verified finding, leave the public repo unchanged.
4. Keep work offline. A live camera test, reboot, flash, or network probe requires a separate, specific owner instruction. Do not publish firmware, dumps, executable maintenance payloads, credentials, private addresses, footage, or device-specific backup contents.

The owner runs this queue hourly with a local Qwen 3.8 model in OpenClaw. The schedule is a work cadence, not proof of completion. Timeouts and failed checks remain visible in the private status; they do not turn hypotheses into findings. Stop the automation when the meaningful offline queue is complete or the remaining work requires owner action.
