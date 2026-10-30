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

## Progress (as of 2026-10-30, 16 offline passes)

- **#1 (codec field) - open.** The field at vma `0x53c3bc` is in `.bss`
    and starts at zero. REA/Ghidra refuted the earlier `0x61120` writer
    candidate: that store belongs to another structure. The later negative
    verifiers search limited literal-load and nearby-call patterns; they do
    not establish that the field lacks a writer or is config-driven. Its
    runtime source remains unknown.
- **#2 (H.264 blocker) - partly resolved.** The tested firmware loaded
    H.265 support and yielded no decodable H.264 frames. The archived
    user-space binary contains an H.264 path, and reference H.264 driver
    source and a test harness exist. The reference driver has not been
    shown compatible with the V200 board, kernel, register map, or encoder
    block. Physical H.264 capability remains open.
- **#3 (sensor profiles) — partially mapped.** The `isp` sensor cascade
    (`0x3b018-0x3b340`) dispatches **16 comparison values and 15 branch destinations**, rodata **names 33
    sensors**, and a **dynamic I2C-signature fallback** (`0x3b8a4`) builds
    `(reg 0x0b<<8)|reg 0x0a` and maps six signatures to sensors
    (`0x841->F37P`, `0x507->Q03`, `0x843->Q03P`, `0x848->H63P`, `0xa63->H63`,
    `0xf37->F37`). The earlier open "third profile" is **SC1346**. `cels` is
    strongly identified as **SC223A** (id 41, 1928x1088/1920x1080).
    Full register-table and callback comparisons remain open.
- **#4 (ISP detail) — mostly done.** The user-tunable surface is a closed 9-key
    registry (`VideoColorParam` + 7 leaves incl. `Acutance`); the 11 hidden ISP
    detail families are not config keys (`verify-isp-detail-controls.py`). The
    enlarge path is a hardware VPSS resampler, not a software kernel.
- **#5 (audio) — done.** 8 kHz, 16-bit PCM source; transcoding adds no bandwidth.
- **#6 (handoff) — done this pass.** A single open-item + evidence-gate ledger
    (`reproducible-handoff-2026-10-30.md`, private) plus this page map every
    GOAL deliverable to its evidence. Later codec-writer absence checks are
    under review and are not evidence that the runtime field stays zero.

**Open items have separate offline and live evidence gates:**

| ID | Open item | Evidence gate |
| --- | --- | --- |
| O1 | `pagalms` on the TB image or a distinct build? | live media dump + SHA-256 |
| O2 | V200 physical H.264 capability | Offline gate: compare reference driver ABI, MMIO, IRQ, and device interface with the archived V200 firmware. Live gate: separately authorized bind plus captured decodable frames and a recovery plan. |
| O3 | Which sensor is fitted in each camera? | installed-ISP hash + live I2C signature readback |
| O4 | Safe ranges of the hidden ISP families + `Acutance` | **Ranges established** (SDK `xm_comm_isp.h`: 8-bit `[0,0xFF]`, DRC `[0,0xffff]`, 16 levels, DeFog 65-entry LUT; safe get/set pattern from `extCfg.c`). `Acutance` (`0xF08`) is not one of the 8-bit fields. **Live A/B still pending** to confirm image effect. |
| O5 | Spoofability of the maintenance-shell peer check | live network test (out of scope) |
| O6 | Per-camera active ISP values | live ISP readback |
| O7 | What change actually improves optical detail | daylight resolution chart |

> **Correction (2026-10-30).** The REA pass corrected the `0x61120`
> writer candidate, but the follow-up negative verifiers overstate their
> coverage. One parser does not resolve its reported call targets, one
> constructor array exists despite the printed conclusion, and a final
> conclusion is marked PASS without a check. The codec-field source and the
> V200 compatibility of reference H.264 code remain open.

## New offline goals

1. **Repair the evidence gate.** Rework the codec-writer verifiers so they
   report the patterns and call targets actually covered. Test them against
   known direct, alias, indirect-call, and constructor examples. Remove any
   unconditional PASS for a research conclusion.
2. **Trace the codec field source.** Use REA/Ghidra to follow the global
   `0x53c340` and the pointer copied into the per-channel slot through
   callers, indirect calls, and bulk operations. Close this only with a
   reproducible writer/source trace, or leave it unresolved with an explicit
   list of paths not analyzed.
3. **Compare sensor programming.** Decode and compare the archived SC223A,
   SC223AP, and SC1346 register tables and callbacks with hashes, lengths,
   representative values, and an independent byte-level verifier. Do not
   infer which profile is installed on a physical camera.
4. **Assess H.264 driver compatibility offline.** Compare reference driver
   source and harness with the archived V200 kernel/module ABI, MMIO and IRQ
   assumptions, and device interface. Produce a compatibility matrix and
   a proposed recovery plan; defer any bind or encode test on a camera.

Each goal needs a bounded report and reproducible check. The hourly OpenClaw
run should stop when these offline goals are complete or explicitly blocked,
rather than repeating the same hypothesis.

Full detail and reproducible checks are in the dated analysis page
`firmware-analysis-2026-10-30.md` and the private workspace.


## Hourly research pass

1. Read the private goal, handoff, latest status, and this queue. Choose the highest-priority unfinished question that can advance with the copied corpus.
2. Make one bounded, evidence-backed change. Save the command, hash, offset or symbol, result, failed hypothesis, and next action in the private report and `outputs/STATUS.md`. Check arithmetic and disassembly claims independently. Finish with a truthful handoff before the run deadline.
3. Update this public repository only when a finding has a reproducible check and has been reviewed for uncertainty and disclosure. Keep the dated analysis page concise; change the queue as questions close. Commit and push only the intended Markdown files. If there is no new verified finding, leave the public repo unchanged.
4. Keep work offline. A live camera test, reboot, flash, or network probe requires a separate, specific owner instruction. Do not publish firmware, dumps, executable maintenance payloads, credentials, private addresses, footage, or device-specific backup contents.

The owner runs this offline queue hourly with a local Qwen 3.8 model in OpenClaw. The schedule is a work cadence, not proof of completion. Timeouts and failed checks remain visible in the private status; they do not turn hypotheses into findings. Stop the automation when the meaningful offline queue is complete or the remaining work requires owner action.
