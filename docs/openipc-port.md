# OpenIPC port: evidence and open questions

Last reviewed: 2026-11-19. This page summarizes the local development work in
plain language. The public `main` branch contains an earlier experimental
`media-probe/` source and a dated QEMU smoke script, but not the current
maintained Buildroot package source, all build inputs, vendor modules, or
current live-tracing tools. Treat a result
as reproducible by others only when its source, command, and shareable output are
linked in a public issue or pull request. The owner will review and publish safe
artifacts separately.

## Goal

Boot OpenIPC on the XM530V200 X4C-WQ and independently decode a real H.265
stream from its image sensor. A Buildroot package build, QEMU login, loaded
module, opened device, successful ioctl, or plausible NAL header is an
intermediate observation. None alone establishes video.

## Evidence gates

| Gate | Current assessment | Required proof to close it |
| --- | --- | --- |
| Package build | Verified locally | A specific source revision, build command and log, installed binary hash. |
| Offline boot | Verified locally | QEMU reaches a login prompt. A shareable current image and complete build recipe remain to be published. |
| Module and device access | Partial | All 18 Xiongmai modules loaded in a local smoke; the probe opened exact video device paths. Access alone says nothing about encoding. |
| Encoder interface | Partial | Saved-flash `Sofia` and `hevc.ko` hashes match the analyzed TB variant; the current RAM image is not independently matched. Static request sizes are known, but live payload field meanings remain unknown. |
| Stock sensor and timing | Partial | Runtime readback reports sensor ID 41 and linear MIPI; long stock stream averaged 6.68 fps after an earlier near-20 fps sample. Chip model and physical exposure cadence remain unknown. |
| Live tracing | Partial | A controlled ARM child was traced and explicitly detached on the camera. One vendor-app thread was traced for three seconds with zero ioctl requests observed; other threads and configuration paths were not covered. |
| Video | Open | Capture H.265 elementary-stream bytes from the port and decode frames with an independent decoder. |
| Physical deployment | OpenIPC untested | Read-only flash backup and RAM-only tools on the stock camera are verified; no OpenIPC image has booted on the physical camera. |

The latest local safe-mode QEMU probe opened `/dev/sys`, `/dev/vpss`,
`/dev/h265`, and `/dev/venc_info` without issuing ioctls; it exited successfully.
This proves the source-built program reaches device access in the emulated
environment. It does not prove the encoder works. The read-only six-partition
flash backup was checked by matching sizes and two independent SHA-256 reads.
Saved-flash `Sofia` and `hevc.ko` matched the analyzed TB variant; that does not
prove the currently running RAM code is identical. The vendor stock camera
reports numeric sensor ID 41, linear MIPI, 1928×1088 active area and 1920×1080
VI output. A 538.86-second RTSP sample contained 3,599 parsed frames (6.68 fps),
while an earlier short sample was near 20 fps. The discrepancy is open.

A static ARM tracer ran from camera RAM. Its controlled blocked-child test hit
a three-second deadline, returned `PTRACE_DETACH rc=0` and left the child able
to continue. A three-second passive trace of one `Sofia` thread returned cleanly
with **zero observed ioctl requests** on that thread. This is not a claim about
other threads or a map of encoder payload bytes. Raw traces, flash backups,
private addresses, credentials and footage remain off GitHub.

### Static ABI starting points

| Device | Request | Established boundary | Still unknown |
| --- | --- | --- | --- |
| `/dev/venc_info` | `0x40c04901` | 192-byte input copy | Meaning and userspace origin of most fields |
| `/dev/venc_info` | `0xc0284902` | 40-byte output | How OpenIPC should use the result |
| `/dev/h265` | `0x40384805` | 56-byte copied argument; offset `+0x34` points to a 1,512-byte table | Table contents and setting-to-field mapping |

These are static module observations, not live field semantics. The next useful
evidence is a bounded trace across current vendor-app threads and a payload
diff when **one** normal setting changes. No decodable OpenIPC frame exists yet.

## Ways to help now

1. Review a single claim in an issue: give the exact source revision, file hash,
   command, observed output, and any conflicting evidence.
2. Review the published experimental `media-probe/` source for host-only
   parsing or error-handling issues, labeling any result as version-specific.
   The maintained Buildroot source has not yet been published here. A parser
   test does not prove hardware behavior.
3. Help create a sanitized, repeatable offline build or QEMU recipe that uses
   legally obtained inputs and records hashes without redistributing them.
4. If you own a matching camera, share its board and firmware *identifiers*
   through the issue template. Do not post a dump or run an unreviewed write,
   flash, or ioctl sequence.

See [CONTRIBUTING.md](../CONTRIBUTING.md) for the evidence template and
[AGENTS.md](../AGENTS.md) for bounded agent tasks. The earlier
[firmware research](../README.md#earlier-vendor-firmware-research) explains the
original camera behavior and is not a success report for the OpenIPC port.

## Corrections and version matching

Some dated local analyses were corrected after independent disassembly. In
particular, a 56-byte HEVC ioctl argument previously described as partly
unwritten is now known to be fully written before the call. Do not copy a
field map from an older note without checking the exact binary hash and a
later correction. Different `hevc.ko` hashes exist in the research inputs.
The saved-flash variant was matched, but a request number alone does not
establish compatible payloads on other variants or prove the running RAM image.

When a claim changes, put the correction beside the original public claim,
link the new evidence, and mark the earlier claim superseded. Keep unknown
fields unknown. This page should change only after the evidence gate changes.
