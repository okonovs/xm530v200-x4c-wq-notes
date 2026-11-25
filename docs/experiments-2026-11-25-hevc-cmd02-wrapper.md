# 2026-11-25: H265_CMD_02 userspace wrapper grounded; 52-byte field layout still undecoded

QEMU-only + offline static RE of the copied TB-user firmware. No physical camera,
router, LAN device, or credential store was contacted. This note is sanitized:
observed facts, call sites, and content hashes only — no raw binary, log, flash
dump, device address, credential, or footage. The content hashes below are
reproducibility anchors, not artifacts.

This note records the userspace side the 2026-11-24 IRQ-77 correction pointed at,
and the one gap that still blocks a decodable stream.

## Wrapper `0xc0344802` call in `Sofia` (re-verified this pass, `objdump`)

The hash-matched userspace front-end `Sofia` (SHA-256 `71137674dfbbeb45…`,
ELF32 ARM EABI5, stripped; `off = vma - 0x8000`) issues `H265_CMD_02`
(`0xc0344802`, an `_IOC` READ|WRITE, 52-byte, type `'H'`, nr 2) through one
small wrapper at vma `0x39af4c`, with a single static caller at `0x39a0f0`.
Re-reading the bytes this pass confirms:

- **`0x39af4c` takes an output-handle pointer** (`r0`), which it keeps in `r2`
  as the `ioctl` third argument.
- **Lazy open, cached fd:** on the first call (cached fd `< 0`) it opens
  `"/dev/h265"` with `O_RDWR` at `0x39af9c` (filename grounded through the
  wrapper's own PC-pool, `0x39afdc -> 0x000b0814 -> vma 0x44b7ac`), and stores
  the returned fd into the `.data` device-struct slot `+0x4` (vma `0x4c5cdc`)
  with a register-base store `str r0,[r4,#0x4]` at `0x39afa8`. Subsequent calls
  reuse the cached fd.
- **The 52-byte call** is `ioctl(fd, 0xc0344802, handle)` via `ioctl@plt`
  (`0x10980`) at `0x39af74`; the constant is built with
  `movw r1,#0x4802` / `movt r1,#0xc034` at `0x39af6c`/`0x39af70`.
- **Return normalization:** any non-zero `ioctl` return is normalized to `-1`
  (`adds r0,r0,#0` / `mvnne r0,#0` at `0x39af78`/`0x39af7c`), so the wrapper
  does not preserve the kernel's original error value; the caller compares the
  result against `0` (`0x39a100`).

PLT/GOT anchors (from `objdump -d`/`-R`): `ioctl@plt 0x10980`
(`GOT 0x496780`), `open@plt 0x11604`, `close@plt 0x11874`.

## The 52-byte output — grounded at the edges, field layout still UNDECODED

The kernel side is hash-matched `hevc.ko` (SHA-256 `c491815a11eeaf69…`, 8,456 B,
ELF32 ARM EABI5 relocatable, unstripped `.text`). For `0xc0344802`, `hevc_ioctl`
wakes from a `schedule`/`prepare_to_wait`/`finish_wait` wait on `hw_info+0xc04`
and then does a **bulk** 52-byte copy to the caller:

- **Source is a fixed kernel global, not a per-field struct.** `hevc.ko` `.bss`
  has an object **`now` at `.bss+0xbd0`, size `0x34` (exactly 52 bytes)**
  (between `regTbaleCfg` and `hevc_finsh` at `+0xc04`). The copy source pointer
  is computed by `add r12, r12, #3024` (`0x5c0`, `3024 = 0xBD0`) into that same
  `now`, and the copy is `__copy_to_user(handle, now, 52)` at
  `0x914` (`mov r1,r5 ; mov r0,r6 ; mov r2,#52 ; bl __copy_to_user`).
- **`now` is filled, not field-assigned.** A 189-entry hardware register-table
  copy loop (`0x5c0`–`0x744`) runs into `now` and then calls `handle_bgn`
  (`0x744`); the frame-ready wake comes from `hevc_interrupt` on IRQ 77, which
  sets `hw_info+0xc04`/`+0xc08`. So the 52 bytes are a raw image of the engine's
  register snapshot, copied as a block.
- **On the userspace side the frame is not read out of the 52 bytes.** The
  single caller's success branch consumes the actual picture from a **separate
  per-stream ring** (`r4+0x60`, with head/tail/offset at `+0x8/0`) and reads
  width/height from `r4+0x3c`/`r4+0x40`; a `0xFF`-scan over the ring (`0x39a284`
  loop) counts NAL start-codes. The 52-byte `now` copy and the ring are different
  buffers. The 52-byte handle itself is **not re-read field-by-field** by this
  caller; it is the `ioctl` scratch/ack buffer.

### Unresolved edge (stop here — do not invent the ABI)

The 52-byte **field-by-field layout is UNDECODED.** What is grounded is that the
52 bytes are a bulk copy of the kernel `now` register snapshot and that the
userspace frame path uses a separate ring; what is not grounded is which offset of
those 52 bytes is which field (frame size, PTS, buffer handle, status, …). That
requires the vendor userspace struct header or a grounded live readback, neither
of which is available offline. Per scope, the trace stops at this indirect edge
rather than inventing the layout. This is the same gap the 11-24 kernel note
flagged; it remains open.

## What is NOT claimed

- No decodable H.265 frame and no video. No NAL bytes were produced or decoded,
  so `ffprobe`/`ffmpeg` were not run; they remain the required gate the moment
  any bytes appear. The `r = -1` observed in QEMU is an error return, not a
  populated 52-byte block.
- No per-field semantics of the 52-byte block are asserted; only the block size,
  its kernel source (`now`, 0x34 B), and the userspace consumer structure are
  grounded.
- No physical-camera compatibility; this is QEMU-only + offline RE. No OpenIPC
  image booted on a physical camera. No encoder ABI was invented.
- The maintained safe Buildroot probe default and the durable QEMU machine are
  unchanged.

## Next single task (one step)

Trace where the per-stream struct's `r4+0x60` ring and `r4+0x3c/0x40` W/H are
allocated/initialized, and how the 52-byte `now` snapshot relates to the ring
head/tail the caller advances — i.e., bind the kernel `now` register fields to
the userspace ring consumer, then attempt an independent `ffprobe`/`ffmpeg`
decode of a captured ring. Until the 52-byte layout and the ring binding are
grounded, the verified milestone is "the `H265_CMD_02` wrapper and its kernel
52-byte copy-out are fully traced; the field layout is the open gap." No H.265
video is produced or claimed.

## Reproducibility anchors

- `Sofia` (TB-user), content SHA-256 `71137674dfbbeb45…`.
- `hevc.ko` (TB modules), content SHA-256 `c491815a11eeaf69…`.
- `libdvr.so` (TB-user), content SHA-256 `b1eef0b80e736f…` — holds none of the
  five `hevc.ko` constants; the constants are built in `Sofia .text`.
- Constant scanner `tools/const_scan_capstone.py`, content SHA-256
  `1abfda93c2ac16…` (capstone cross-check; `objdump` is the ground truth).
- Key vma (`.text`, `off = vma - 0x8000`): wrapper `0x39af4c`;
  `movw/movt 0xc0344802` `0x39af6c`/`0x39af70`; `ioctl@plt 0x10980`;
  `open@plt 0x11604`; device struct `0x4c5cd8` (fd slot `+0x4 = 0x4c5cdc`,
  init `-1`, `.data`); `/dev/h265` `0x44b7ac` (PC-pool `0x39afdc -> 0x000b0814`);
  open site `0x39af9c`; fd store `0x39afa8`; caller `0x39a0f0`;
  per-stream struct `r4 = *(arg0+0x21878)` @ `0x399c0c`; ring `r4+0x60` @
  `0x39a1ec`; dims `r4+0x3c/0x40` @ `0x39a1d4`/`0x39a1d8`.
- Kernel: `now` `.bss+0xbd0` size `0x34`; copy source `add r12,r12,#3024` @
  `0x5c0`; `__copy_to_user(handle, now, 52)` @ `0x914`; `handle_bgn` @ `0x744`;
  wake source `hw_info+0xc04`/`+0xc08` from `hevc_interrupt`.

Raw logs, build trees, generated binaries, and the QEMU image remain off GitHub.
These prefixes let a reviewer confirm the same inputs without receiving the
artifacts.

## Device-contact statement

No camera, router, LAN device, or credential store was contacted. All observation
was QEMU-only + offline static RE on the copied corpus.
