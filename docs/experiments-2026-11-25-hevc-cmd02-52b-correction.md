# 2026-11-25 (correction): H265_CMD_02 52-byte output is a stack buffer that IS re-read

QEMU-only + offline static RE. No camera, router, LAN device, or credential store
was contacted. Sanitized: offsets, call sites, content hashes only. This is a
**correction** to the wrapper note `5a6a792`
(`docs/experiments-2026-11-25-hevc-cmd02-wrapper.md`) and to the private
`HEVC-ENCODER-INPUT-TRACE-20261124.md`; the correction was found and verified in
the same pass, so it is published here rather than left as a known error. The
52-byte **field semantics** remain undecoded; this note does **not** claim a
field map is solved.

## What was wrong

The wrapper note said the 52-byte `H265_CMD_02` output "is **not** re-read
field-by-field by this caller; it is the `ioctl` scratch/ack buffer," and that
the handle passed to the wrapper was `*(arg1+0x1c)`. Both are imprecise.

## What is grounded instead (`Sofia`, SHA-256 `71137674dfbbeb45…`;
`hevc.ko`, SHA-256 `c491815a11eeaf69…`)

- **The 52-byte output buffer is a per-call stack temp at `sp+0x1c`** (52 B,
  `sp+0x1c`…`sp+0x4f`), **pre-zeroed** at `0x39a0c0`–`0x39a0dc`, and passed to
  the wrapper `0x39af4c` as `r0` via `r9 = sp+0x1c` (`0x39a098`, then `mov
  r0,r9` `0x39a0b8`, `bl 0x39af4c` `0x39a0f0`).
- **It IS re-read field-by-field after the call** (success path) at buffer
  offsets `{+0x00, +0x08, +0x0c, +0x10, +0x14, +0x18, +0x2c}` — reads at
  `0x39a320/0x39a370/0x39a324/0x39a338/0x39a380/0x39a360/0x39a19c`
  (`sp+0x1c/0x24/0x28/0x2c/0x30/0x34/0x48`) — and its fields are rearranged
  into a local output descriptor (`0x39a388`–`0x39a3b0`).
- **`*(arg1+0x1c)` is a separate pointer** (a ring/dimension base): read at
  `0x39a03c`, stashed to `sp+0x24` at `0x39a06c`, and used as the ring base at
  `0x39a350`. The earlier text missed the `r9` reassignment at `0x39a098`.
- **The 52-byte kernel source is the global `now`** (`.bss+0xbd0`, size `0x34`
  = 52 B), a 189-entry hardware-register snapshot; the copy-out is
  `__copy_to_user(handle, now-mirror, 52)` @ `0x914`, sourced by
  `add r12, r12, #3024` @ `0x5c0`; `handle_bgn` @ `0x744` is the frame-ready
  worker, woken on IRQ 77 (`hevc_interrupt` → `hw_info+0xc04/0xc08`).

## What remains unresolved (stop edge — do not invent the ABI)

The 52-byte **field semantics** are UNDECODED: `now` is a raw register-table
snapshot with no struct symbols, so offsets `+0x00/0x08/0x0c/0x10/0x14/0x18/
0x2c` show that fields are consumed but not what they mean (frame size,
timestamp, buffer handle, flags, …). Binding the per-stream handle `arg1+0x1c`
to the specific ring buffer is also an unresolved indirect edge. Resolving either
needs the vendor userspace struct header or a grounded live readback — not
available offline. The map stops here.

## What is NOT claimed

No decodable H.265 frame, no NAL, no video; no bytes were decoded, so
`ffprobe`/`ffmpeg` were not run (they remain the required gate). No per-field
meaning of the 52-byte block is asserted. No physical-camera compatibility;
QEMU-only + offline RE. No encoder ABI was invented; the maintained safe
Buildroot probe default and the durable QEMU machine are unchanged.

## Next single task

Trace the per-stream ring-base init (the `str [arg1+0x1c]!` config-copy writers,
e.g. `0x3937b4`, and the 648-byte `memcpy` at `sp+0x220`) to identify the ring
struct and its head/tail/offset layout, correlate the seven read offsets of the
52-byte block against that ring's bookkeeping, then attempt an independent
`ffprobe`/`ffmpeg` decode of a captured ring. No ABI is invented.

## Reproducibility anchors

- `Sofia` `71137674dfbbeb45…`; `hevc.ko` `c491815a11eeaf69…`; `libdvr.so`
  `b1eef0b80e736f…`; scanner `tools/const_scan_capstone.py`
  `1abfda93c2ac16…` (capstone cross-check; `objdump` is ground truth).
- Wrapper `0x39af4c`; `0xc0344802` `0x39af6c/0x39af70`; `ioctl@plt 0x10980`
  (`GOT 0x496780`); `open@plt 0x11604`; device struct `0x4c5cd8` (fd
  `+0x4 = 0x4c5cdc`, init `-1`, `.data`); `/dev/h265` `0x44b7ac`
  (PC-pool `0x39afdc → 0x000b0814`); open `0x39af9c`; fd store `0x39afa8`.
- Caller `0x399bdc` (arg1 `r10`, `arg0+0x21878`→`r4` @ `0x399c0c`); 52-byte
  buffer `sp+0x1c` (zeroed `0x39a0c0`–`0x39a0dc`); `r9=sp+0x1c` `0x39a098`;
  `mov r0,r9` `0x39a0b8`; `bl 0x39af4c` `0x39a0f0`; stashed ring handle
  `sp+0x24` (`*(arg1+0x1c)` `0x39a03c`→`0x39a06c`); post-call reads
  `sp+0x1c/0x24/0x28/0x2c/0x30/0x34/0x48`; ring `r4+0x60` `0x39a1ec`;
  dims `r4+0x3c/0x40` `0x39a1d4/0x39a1d8`; consumer `0x39c580`; ring-init edge
  `0x3937b4`.
- Kernel: `now` `.bss+0xbd0` size `0x34`; `add r12,r12,#3024` `0x5c0`;
  `__copy_to_user` `0x914`; `handle_bgn` `0x744`; wake `hw_info+0xc04/0xc08`
  from `hevc_interrupt` (`0x324`).

Raw logs, build trees, generated binaries, and the QEMU image remain off GitHub.
These prefixes let a reviewer confirm the same inputs without receiving the
artifacts. No camera, router, LAN device, or credential store was contacted.
