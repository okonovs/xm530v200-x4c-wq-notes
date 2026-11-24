# 2026-11-24: QEMU IRQ-77 delivery corrected (SPI input 45); H265_CMD_02 now returns, still no video

QEMU-only. No physical camera, router, LAN device, or credential store was
contacted. This note is sanitized: it records observed facts and the next
engineering step, but no raw log, flash dump, device address, credential,
route, local path, or footage. Content hashes below are reproducibility
anchors, not artifacts.

## Question

The 2026-11-22 trace (see the 11-22 page) showed `H265_CMD_02` blocks in QEMU,
gated on an encoder-complete interrupt that QEMU did not model. A 2026-11-23
model note proposed a QEMU-only completion stub and reported that the guest
could not deliver Linux IRQ 77 (its `pic[]` GIC-input array was assumed to be
filled only for the first 64 SPI inputs). Was that conclusion correct, and does
delivering IRQ 77 let `H265_CMD_02` return — and if so, does that produce any
media?

## Result (observed, not inferred): the B1 "cannot deliver IRQ 77" claim was wrong

The 11-23 "cannot deliver IRQ 77" conclusion was a **wrong index, not a missing
GIC capability.** In this guest, kernel IRQ 77 is GIC SPI input **45**
(`77 - 32`; kernel IRQ = GIC input + 32, as the UARTs show: GIC inputs 0/1/2 map
to kernel IRQs 32/33/34). The model had wired the encoder line to the
unpopulated slot `pic[77]`; the populated slot is `pic[45]`, well inside
`num_spi = 64`. Connecting the line to `pic[45]` — no GIC/PPI rewrite — delivers
IRQ 77.

An isolated candidate was built from the durable QEMU machine with only the
existing completion stub enabled and the line re-pointed to `pic[45]` plus a
**read-clear** completion register (a naive raise/lower pulse is lost because the
GIC level tracks the input; a sustained level livelocks the guest in the module's
handler, which never deasserts the line). The durable machine and the maintained
safe probe default were left unchanged; the change lives in a separate build.

Observed, in a bounded disposable guest (outer timeout, no network, no reboot):

- Guest `/proc/interrupts`: `77: 0 0 GIC h265` **→** `77: 1 0 GIC h265` — the
  counter goes **0 → 1**, so IRQ 77 **is delivered** within `num_spi = 64`. The
  GIC needs no rewrite; the 11-23 blocker was the `pic[77]` index.
- `H265_CMD_02` (`0xc0344802`) **returns** instead of blocking forever: it comes
  back as an **error, `r = -1` (EPERM)**. The probe then reaches its exit code
  and a shell — no hang.
- **Zero content blocks and zero NAL start-codes: no output bytes.** The
  follow-up `H265_CMD_05` also returns `-1` after a user-space copy failure and a
  register-push failure.

So the IRQ-delivery half of the 11-23 blocker is resolved and evidence-corrected.
The data half is not: the call returns an *error*, not a populated block, so no
independent H.265 decode is even applicable.

## What is grounded vs ungrounded

- **Grounded (from `hevc.ko` bytes):** the call can be made to return. IRQ 77
  (= SPI 45) delivers; the threaded-IRQ handler runs; `H265_CMD_02` leaves its
  kernel wait and returns. The read-clear register models IRQ-delivery mechanics
  only.
- **Ungrounded (no vendor evidence in the corpus):** the 52-byte output block that
  `H265_CMD_02` is meant to copy to the caller, and the NAL payload a real frame
  would put in the output ring. No vendor userspace struct or device observation
  for that block exists here, so producing a decodable stream would require
  *inventing* the encoder ABI — exactly the ungrounded design that is checkpointed,
  not delivered.

## What is NOT claimed

- No decodable H.265 frame and no video. Zero NAL bytes appeared, so
  `ffprobe`/`ffmpeg` (present on the host at version 9.0.2) were not run; they
  remain the required gate the moment any bytes do appear.
- The `r = -1` is an **error return**, not a populated 52-byte block. It is not a
  frame and not a round-trip of media.
- No physical-camera compatibility is implied; this is QEMU-only. No OpenIPC
  image booted on a physical camera.
- No encoder ABI was invented to make the number come out as a frame.

## The next single task (one step)

Obtain the vendor **userspace** encoder control sequence and the 52-byte output-
block layout from the actual userspace front-end (the stripped `Sofia`/`isp`
binaries and their shared library, or the physical camera path), then populate the
modelled ring from that *real* layout, and only then attempt an independent
`ffprobe`/`ffmpeg` decode. Until that ABI is grounded, the verified milestone is
"IRQ 77 delivers and `H265_CMD_02` returns"; the data path stays checkpointed. No
H.265 video is produced or claimed.

## Reproducibility anchors

- Baseline QEMU binary (completion stub disabled), content SHA-256
  `905ff7e6fcb8…`.
- Candidate QEMU binary (completion stub enabled, line on `pic[45]`, read-clear),
  content SHA-256 `3e876b358b9b…`.
- Guest kernel image, content SHA-256 `752f3ae73537…`.
- `hevc.ko` (the closed vendor module that registers `/dev/h265` and IRQ 77),
  content SHA-256 `c491815a11ee…`.
- Evidence initrd used for the bounded run, content SHA-256 `acc8f4c6bd5d…`.

The candidate machine source is a two-hunk change on the durable source: enable
the existing completion stub and re-point the encoder line from `pic[enc_irq]` to
`pic[enc_irq - 32]` (= `pic[45]`). The durable machine and the maintained safe
probe default were left byte-for-byte unchanged and re-verified.

Raw logs, the build tree, generated binaries, and the QEMU image remain off
GitHub. These prefixes let a reviewer confirm the same inputs without receiving
the artifacts.

## Device-contact statement

No camera, router, LAN device, or credential store was contacted. All observation
was QEMU-only on the host.
