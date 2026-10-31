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

- **#1 (codec field) - open (advanced 2026-10-31).** The field at vma `0x53c3bc`
    is in `.bss` and starts at zero. REA/Ghidra refuted the earlier `0x61120`
    writer candidate: that store belongs to another structure. **The 6 direct-
    internal call sites that receive `&0x53c340` were analyzed body-wise**
    (dual-decoder capstone+objdump, non-vacuous): none of the 5 distinct callees
    writes `+0x7c`, the 2 `memset` 'base' candidates are false positives (r0=
    stack), and 0 of the `+0x7c` stores in `.text` use base `0x53c340` — so the
    direct-callee coverage gap is **closed**. The field's **source/runtime value
    remains unknown**: the 5 stripped `.so` (a setter could live in a stripped
    shared library) and indexed/indirect stores through a reused base register
    are not fully modeled, and the runtime value needs a type-aware Ghidra trace
    plus a live test. Verifier `verify-codec-callees-0x7c.py` (gates PASS);
    `verify-codec-writer-repaired.py` remains OPEN by construction (no
    unconditional PASS).
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


## Firmware security track

The owner has also requested a backdoor and vulnerability review. The
archived owner-modified image contains a maintenance root listener with
only a source-address check; its presence on currently running cameras
is unverified. This is distinct from stock vendor behavior. An initial
stock-filesystem review found a serial login entry and a legacy root
password-hash format, but has not established passwordless login or a
vendor network backdoor.

The offline queue is to map active startup and network services; trace
authentication to privileged operations in the web, RTSP, and cloud/P2P
paths; inspect firmware-update authorization and integrity checks; verify
serial login behavior; then rank only reproducible findings with
mitigations. As of 2026-10-31, S1 (boot/startup -> services), S3 (update
admission), and S4 (serial getty/login) are closed offline; S2 (auth ->
privileged sink) is traced structurally with its upstream authorization left
live-gated; S5 is the ranked-finding table. A string match or bundled software version alone does not
count as a vulnerability. Live camera tests require separate owner
authorization, and public notes will omit credentials and operational
exploit details.

**S1 progress (2026-10-30).** The stock boot chain was mapped:
`inittab ::sysinit -> rcS` launches (in order) `dnode`, the filesystem
mounts, `loadmod` (-> `loadpublic`, conditional `wlandaemon`/`routeswitch`),
`netinit`, the owner `extapp.sh` hook when present, and `dvrHelper` over a
loopback control channel, which supervises `app.sh` -> `Sofia` (the single
long-lived application). Listening-service classification (stripped-aware
dynamic-symbol import of `bind`+`listen`): **active at boot** = `netinit`,
`Sofia`, `dvrHelper`; **dormant** (present in the image but referenced by no
boot script in the archived corpus) = a debug-listener binary
(`accept`/`bind`/`listen`/`popen`, a "listening at port" string, a debug-open
symbol) and `iperf`; Wi-Fi/bridge tooling is conditional on matched `WIFI=`/
`UVC=`/`NVR_FLAG` profile branches. The owner-added `extapp.sh` hook is
present in the owner custom roots and absent in the stock root(-clean); it
loops a small statically-linked shell that serves one connection to a
single configured owner peer with no cryptographic authentication. "Dormant"
means "no boot-script reference in the archived corpus" and is not a proof the
live device never starts the binary; live port exposure is owner-gated. The
peer address, the device MAC, and the maintenance-shell payload are kept
private. Reproducible check: `verify-boot-services.py`.

**S3 progress (2026-10-30).** Update admission was traced. Both archived
"all.bin" images are ZIP/PKZIP containers whose `InstallDesc.UpgradeCommand`
lists six `Burn` steps including `custom-x.cramfs.img` — so an admitted update
**overwrites the custom partition** (`/mnt/custom`), which wipes the owner
maintenance hook found in S1 (`extapp.sh`) and its stock-root rollback; the
update is thus the persistence-rollback vector. The admission gate is
**structural** — container CRC, an incompatible-version check, and a
per-partition size check against the MTD layout — plus an OEM MAC/serial-number
binding (`CUpgrader::checkOEMinfoSum` / an `Mx8Q` per-device blob). **No
cryptographic signature is verified by the `upgrader`** (no crypto routines
present), and a per-device-integrity skip path ("skip check Mx8Q") plus a
"cracked image" path exist in the code as a *documented-as-code* bypass whose
reachability on this build is **unverified** (needs a caller-address trace or a
live test). Submitters are the authenticated web/ONVIF operator
(`upgrade.cgi`, password-serial auth) and the vendor cloud/P2P auto-upgrade.
Reachability of the `upgrade.cgi` auth gate specifically is left to S2. The
`Mx8Q` value, device ID, and any exploit steps are kept private. Reproducible
check: `verify-update-admission.py`.

**S2 progress (2026-10-30, offline-static; not published as a vulnerability).**
A privileged sink was located: `system(2)` in `Sofia` (the CUpgrader command
dispatcher) executes a network/CGI-supplied "Script" field guarded only by a
3-token substring blocklist (`telnetd`/`sleep`/`/debug`) plus a length check,
with no allowlist or argument sanitization — a weak, bypassable-by-construction
guard. The exact upstream authorization for this dispatch (which CGO/ONVIF
command registers the message, and whether the authority gate covers it) is
traced structurally, not by a full CGO->auth cross-reference, so it is marked
**partly-resolved, not a confirmed exploitable defect**; a live bypass needs an
owner-authorized test. No exploit, payload, credential, or device address is
published; the detailed trace stays private. Reproducible check (sink + weak
guard only): `verify-upgrade-rce-guard.py`.

**S4 progress (2026-10-31).** Serial getty/login semantics were verified in the
archived stock BusyBox (`v1.33.1`, uClibc, stripped ARM; `etc/inittab` sha
`3818bb9f...`, `etc/passwd` sha `2f0f285c...`, matching the triage). The getty
entry is
`::respawn:/sbin/getty -L ttyS000 115200 vt100 -n root -I "Auto login as root ..."`.
**Passwordless auto-login is NOT established:** the "Auto login as root" text is
the `-I` issue banner — it is **absent from the BusyBox binary** and present
only in the inittab (cosmetic). The inittab uses `-n root`, **not** the
force-login flag `-f` (the getty option string is `^I:LH:f:hil:mt:+wn`, in which
`f:` is the separate force option), and getty executes the login program
(`execlp`), which authenticates against the root MD5-crypt hash `$1$...` in
`/etc/passwd` (there is no `/etc/shadow`). This corroborates the editorial
correction that the banner must not be read as a bypass. The root hash is not
cracked or published. Coverage limit (stated): the exact runtime meaning of `-n`
in this stripped build is not fully resolved; the finding is the negative
(force flag not used; login program invoked). Live serial behavior on a camera
is out of scope. Reproducible check: `verify-serial-login.py`.

**S5 progress (2026-10-31).** The findings were ranked by impact × reachability ×
confidence, each row backed by a component verifier (aggregated by private
`verify-security-rankings.py`; all component verifiers PASS). Ranked, **safe-to-
publish** findings: **(F-1)** the owner-added custom-partition root-shell hook
(`extapp.sh`, source-address-gated, no cryptographic auth; **owner-added, not a
vendor backdoor**; live presence unverified); **(F-2)** update admission is
**structural only** (container CRC + version + per-partition size + an OEM
MAC/serial binding), **no cryptographic signature is verified**, and an admitted
update **overwrites the custom partition** — with two documented-as-code bypass
paths ("skip check Mx8Q", "cracked image") whose reachability is **unverified**;
**(F-4)** the serial getty "auto login as root" is a **cosmetic banner, not a
passwordless bypass** (see S4); **(F-5)** a debug-listener binary and `iperf`
are **dormant** (no boot-script reference in the archived corpus). **Not
established:** no hidden vendor network backdoor, default remote password,
unauthenticated vendor API, or confirmed exploitable parser flaw. The
exploit-adjacent finding **(F-3)** — a privileged `system(2)` sink in `Sofia`
guarded only by a 3-token blocklist (`telnetd`/`sleep`/`/debug`) plus a length
check (weak / bypassable-by-construction, upstream authorization only
**partly-resolved**) — is **withheld** from public notes pending a proven auth
gate; it is kept private with no exploit or credential material.

## Hourly research pass

1. Read the private goal, handoff, latest status, and this queue. Choose the highest-priority unfinished question that can advance with the copied corpus.
2. Make one bounded, evidence-backed change. Save the command, hash, offset or symbol, result, failed hypothesis, and next action in the private report and `outputs/STATUS.md`. Check arithmetic and disassembly claims independently. Finish with a truthful handoff before the run deadline.
3. Update this public repository only when a finding has a reproducible check and has been reviewed for uncertainty and disclosure. Keep the dated analysis page concise; change the queue as questions close. Commit and push only the intended Markdown files. If there is no new verified finding, leave the public repo unchanged.
4. Keep work offline. A live camera test, reboot, flash, or network probe requires a separate, specific owner instruction. Do not publish firmware, dumps, executable maintenance payloads, credentials, private addresses, footage, or device-specific backup contents.

The owner runs this offline queue hourly with a local Qwen 3.8 model in OpenClaw. The schedule is a work cadence, not proof of completion. Timeouts and failed checks remain visible in the private status; they do not turn hypotheses into findings. Stop the automation when the meaningful offline queue is complete or the remaining work requires owner action.
