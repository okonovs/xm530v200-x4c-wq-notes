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

- **#1 (codec field) — done.** The main-stream codec field in archived `Sofia`
    (vma `0x53c3bc`) lives in `.bss` and is written at runtime; it is
    **config-driven**, not hardcoded (verifiers `verify-h264-bss-codec`,
    `verify-h264-codec-driven`).
- **#2 (H.264 blocker) — revised 2026-10-30 13:30 UTC.** The *driver/hardware*
    layer fact holds: the loaded encoder is H.265-only (`hevc.ko`, `/dev/h265`),
    no `/dev/h264`, and `XM_MPI_VENC_CreateChn` fails with `0xa0078007`.
        `verify-h264-polarity` fixes the `movne r3,#0`->H.265 polarity; the
    user-space `0x2b94c4` patch selects the H.264 branch but no H.264 driver is
    loaded. However, the earlier "blocked, hardware unknown" framing was
      **too strong** and is corrected (`verify-h264-userpath.py`, dual-decoder
    PASS): the firmware's **user-space H.264 path is real** (`_h264_create_chn`/
      `H264_PARSER`/`CEncode`/`AVEnc.SmartH264`/RTSP H.264), and a **complete H.264
    driver exists in the corpus** (`libxmvenc`/`hx280enc` `open_h264.ko`, a
    drop-in for `xm530_h264.ko`, Hantro 8290/VEPU1, `xmvenc-test` VI harness).
    So the physical H.264 question is **OPEN — test by binding a known H.264
    driver (E1/E2/E3)**, not a closed negative (corroborates editorial note #4).
- **#3 (sensor profiles) — done offline.** The `isp` sensor cascade
    (`0x3b018-0x3b340`) dispatches **15 fixed chip-IDs**, rodata **names 33
    sensors**, and a **dynamic I2C-signature fallback** (`0x3b8a4`) builds
    `(reg 0x0b<<8)|reg 0x0a` and maps six signatures to sensors
    (`0x841->F37P`, `0x507->Q03`, `0x843->Q03P`, `0x848->H63P`, `0xa63->H63`,
    `0xf37->F37`). The earlier open "third profile" is **SC1346**. `cels` is
    strongly identified as **SC223A** (id 41, 1928x1088/1920x1080).
- **#4 (ISP detail) — mostly done.** The user-tunable surface is a closed 9-key
    registry (`VideoColorParam` + 7 leaves incl. `Acutance`); the 11 hidden ISP
    detail families are not config keys (`verify-isp-detail-controls.py`). The
    enlarge path is a hardware VPSS resampler, not a software kernel.
- **#5 (audio) — done.** 8 kHz, 16-bit PCM source; transcoding adds no bandwidth.
- **#6 (handoff) — done this pass.** A single open-item + evidence-gate ledger
    (`reproducible-handoff-2026-10-30.md`, private) plus this page map every
    GOAL deliverable to its evidence and the 15 verifiers (all PASS).

**Open items, each live- or external-doc-gated (none closeable offline):**

| ID | Open item | Evidence gate |
| --- | --- | --- |
| O1 | `pagalms` on the TB image or a distinct build? | live media dump + SHA-256 |
| O2 | V200 physical H.264 capability | **Revised 13:30 UTC:** a complete H.264 driver exists in the corpus (`libxmvenc`/`hx280enc` `open_h264.ko`, `xmvenc-test` VI harness). Gate: **live bind + encode** (`xmvenc-test -v 0` under a reboot guard) or map V200 encoder MMIO — not external docs. |
| O3 | Which sensor is fitted in each camera? | installed-ISP hash + live I2C signature readback |
| O4 | Safe ranges of the hidden ISP families + `Acutance` | **Ranges established** (SDK `xm_comm_isp.h`: 8-bit `[0,0xFF]`, DRC `[0,0xffff]`, 16 levels, DeFog 65-entry LUT; safe get/set pattern from `extCfg.c`). `Acutance` (`0xF08`) is not one of the 8-bit fields. **Live A/B still pending** to confirm image effect. |
| O5 | Spoofability of the maintenance-shell peer check | live network test (out of scope) |
| O6 | Per-camera active ISP values | live ISP readback |
| O7 | What change actually improves optical detail | daylight resolution chart |

> **Recommendation (revised 13:30 UTC).** O2 was reframed this pass: a working
> H.264 driver path now exists in the corpus (`libxmvenc`/`hx280enc`), so the
> V200 physical H.264 question is **open and testable by binding that driver and
> encoding from the VI channel** (`xmvenc-test -v 0`), not merely a missing
> document. The remaining live-gated items are O1/O3/O5/O6/O7 and the O2 bind+
> encode test. O4 (safe ISP ranges) is closed. No offline item remains
> closeable without a live camera or the O2 live driver-bind test; the hourly
> job was retired at 05:15 UTC and can be re-enabled for that live test.

Full detail and reproducible checks are in the dated analysis page
`firmware-analysis-2026-10-30.md` and the private workspace.


## Hourly research pass

1. Read the private goal, handoff, latest status, and this queue. Choose the highest-priority unfinished question that can advance with the copied corpus.
2. Make one bounded, evidence-backed change. Save the command, hash, offset or symbol, result, failed hypothesis, and next action in the private report and `outputs/STATUS.md`. Check arithmetic and disassembly claims independently. Finish with a truthful handoff before the run deadline.
3. Update this public repository only when a finding has a reproducible check and has been reviewed for uncertainty and disclosure. Keep the dated analysis page concise; change the queue as questions close. Commit and push only the intended Markdown files. If there is no new verified finding, leave the public repo unchanged.
4. Keep work offline. A live camera test, reboot, flash, or network probe requires a separate, specific owner instruction. Do not publish firmware, dumps, executable maintenance payloads, credentials, private addresses, footage, or device-specific backup contents.

The owner runs this queue hourly with a local Qwen 3.8 model in OpenClaw. The schedule is a work cadence, not proof of completion. Timeouts and failed checks remain visible in the private status; they do not turn hypotheses into findings. Stop the automation when the meaningful offline queue is complete or the remaining work requires owner action.
