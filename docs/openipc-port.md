# OpenIPC port: evidence and open questions

Last reviewed: 2026-11-16. This page summarizes the local development work in
plain language. The public `main` branch contains an earlier experimental
`media-probe/` source, but not the current maintained Buildroot package source,
all build inputs, vendor modules, or QEMU artifacts. Treat a result
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
| Offline boot | Verified locally | Reproducible QEMU command and serial log reaching usable userspace. |
| Module and device access | Partial | Module-load results and exact device paths; access alone says nothing about encoding. |
| Encoder interface | In progress | Match user-space and kernel-module variants, then trace each proposed ioctl and payload field to its handler. |
| Video | Open | Capture H.265 elementary-stream bytes from the port and decode frames with an independent decoder. |
| Physical deployment | Open | Owner-approved test procedure, recovery method, recorded results. |

The latest local safe-mode QEMU probe opened `/dev/sys`, `/dev/vpss`,
`/dev/h265`, and `/dev/venc_info` without issuing ioctls; it exited successfully.
This proves the source-built program reaches device access in the emulated
environment. It does not prove the encoder works. The next offline question is
whether the `Sofia` user-space binary and the task-target `hevc.ko` used for
static analysis came from the same firmware variant. Until extraction provenance
is established, their structure compatibility remains unverified.

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
later correction. Different `hevc.ko` hashes exist in the research inputs;
matching an ioctl number alone does not establish compatible payloads.

When a claim changes, put the correction beside the original public claim,
link the new evidence, and mark the earlier claim superseded. Keep unknown
fields unknown. This page should change only after the evidence gate changes.
