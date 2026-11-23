# 2026-11-22: QEMU trace locates the exact blocking call in the H.265 path

QEMU-only. No physical camera, router, LAN device, or credential store was
contacted. This note is sanitized: it records observed facts and the next
engineering step, but no raw log, flash dump, device address, credential,
route, local path, or footage. Content hashes below are reproducibility anchors,
not artifacts.

## Question

The durable candidate firmware boots in QEMU and runs a maintained media probe.
The guest console shows `VPSS_CMD_01 -> r=0` and successful opens of
`/dev/h265` and `/dev/venc_info`, but no `PROBE_EXIT` and no media bytes.
Which call blocks, and what input is missing?

## Method (bounded QEMU-only trace)

The maintained safe probe default was left unchanged. A QEMU-only **trace**
variant added flushed enter/return markers around every device operation and
always entered the H.265 path (the default backstop variant skips it). Each
guest ran under a bounded outer `timeout` with standard input closed; the last
`ENTER` without a matching `RET` is the blocking call.

## Result (observed, not inferred)

Every guest console ends identically:

```
[trace] RET open venc_info -> fd 6
[trace] ENTER ioctl h265 H265_CMD_02(0xc0344802)   <- last ENTER, no matching RET
```

`H265_CMD_02` (`0xc0344802`, a 52-byte read/write ioctl on `/dev/h265`) is
entered and **does not return**. This corrects the 2026-11-22 draft, which had
*inferred* the block was at `H265_CMD_02` from source order; the trace now
*shows* it. `VPSS_CMD_01`, the `/dev/h265` open, and the `/dev/venc_info` open
all return; the first H.265 ioctl does not. The earlier belief that the block
was in an `H265_CMD_05` read loop is wrong: the block is at the very first H.265
command, before any `CMD_05` loop.

## What the blocking call does (byte-level static review of `hevc.ko`)

`hevc.ko` (content SHA-256 `c491815a11eeaf69…`, vermagic
`3.10.103+ SMP ARMv7 p2v8`) is a closed vendor module. `hevc_ioctl` dispatches on
the request word; the `R_ARM_CALL` relocations plus disassembly pin the
`H265_CMD_02` path to a kernel wait gated on an encoder-complete interrupt:

- `H265_CMD_02` (`0xc0344802`) reads a global queue field, spins on a
  not-ready comparison, then waits via `schedule()` + `prepare_to_wait()`,
  busy-polls a completion bit, and calls `finish_wait()`.
- The completion bit / wake is set by the module's threaded-IRQ handler,
  registered with `request_threaded_irq` at init and waking the queue with
  `__wake_up`.
- Other requests, for reference: `H265_CMD_03` (`0x40044803`) takes a
  `down_interruptible`; `H265_CMD_05` (`0x40384805`) copies a 56-byte block
  from user space.

This is a code fact, not a device observation: `H265_CMD_02` is the
"wait for the H.265 encoder to produce/complete a block" command. It sleeps in
the kernel until the encoder's threaded interrupt fires and sets the completion
bit. In QEMU there is **no H.265 encoder and no sensor** feeding one, so that
interrupt never arrives and the wait never completes; the ioctl never returns.
This is consistent with the task sitting in an uninterruptible kernel wait that a
user-space `kill`/`SIGKILL` from `/init` cannot break (an earlier backstopped
`/init` hung the same way and hit its wall-clock deadline).

## The missing input

The QEMU `xm530` machine models, as register banks: `xm-gpio`, `xm-i2c`,
`xm-dma` (PL330), `xm-h264` (Hantro H1), `xm-rtc`, `xm-i2s`, `xm-isp`. There is
**no H.265/`hevc`/`venc` encoder model**. The `/dev/h265` misc device is
registered inside `hevc.ko` itself; its behavior depends on the
encoder-complete interrupt, which QEMU does not model. So the read cannot
complete and no NAL bytes can be produced by QEMU alone.

## What is NOT claimed

- No decodable H.265 frame and no video. `VPSS_CMD_01 -> r=0` and the device
  opens are real kernel round-trips, but are **not** a media frame. No
  NAL-looking pattern was produced or claimed. `ffprobe`/`ffmpeg` (both present
  at version 9.0.2 on the host) were not needed this round because no bytes
  appeared; they are the required gate if bytes do appear.
- No physical-camera compatibility is implied; this is QEMU-only.
- This is a QEMU milestone, not an OpenIPC video result. No OpenIPC image has
  booted on a physical camera.

## Blocker and next single task

The missing physical input (an H.265 encoder-complete interrupt, or a video
source feeding the encoder) cannot be modelled within one focused change without
writing a new QEMU device model that synthesizes that interrupt — out of scope,
and not something to fake. The precise dependency to checkpoint: a QEMU model
that delivers the H.265 encoder-complete interrupt to the module's handler (or
an external NAL sample injected at the completion-bit site) so `H265_CMD_02` can
return a non-zero block. Two candidate routes, both requiring new device code:

1. Model the H.265 encoder-complete path in the `xm530` machine (a register bank
   or a `hevc` device that raises the module's interrupt / sets the completion bit
   and copies a NAL into the block), then re-run the trace guest and, if bytes
   appear, decode them with `ffprobe`/`ffmpeg` before any video claim.
2. A user-space mock `hevc.ko` that returns a canned NAL.

Neither is a one-line change.

## Reproducibility anchors

- Trace ARM probe binary, content SHA-256 `98c30a75…` (from source
  `b5f77923…`); its boot log, content SHA-256 `0da94b8d…`.
- Diagnose ARM probe binary, content SHA-256 `62974fea…` (from source
  `7d2862a4…`).
- `hevc.ko`, content SHA-256 `c491815a11eeaf69…`.

Raw logs, the build tree, generated binaries, and the QEMU image remain off
GitHub. These prefixes let a reviewer confirm the same inputs without receiving
the artifacts.

## Device-contact statement

No camera, router, LAN device, or credential store was contacted. All
observation was QEMU-only on the host.
