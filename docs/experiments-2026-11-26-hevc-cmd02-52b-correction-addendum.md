# Addendum (2026-11-26): correct two material errors in the 2026-11-25 correction

This is a **scoped addendum** to `experiments-2026-11-25-hevc-cmd02-52b-correction.md`
(`1e0bac5`). It corrects two stack-liveness errors that `1e0bac5` still repeats.
All claims below are verified against `Sofia` SHA-256 `71137674dfbbeb45…`
(ELF32 ARM EABI5, stripped; `off = vma - 0x8000`) and `hevc.ko`
SHA-256 `c491815a11eeaf69…`. Ground truth: `objdump`/`capstone` over raw
re-extracted 4-byte words. No camera/router/LAN/credential contact.

## Error 1: the 52-byte buffer is NOT fully pre-zeroed

`1e0bac5` says the buffer `sp+0x1c` (52 B) is "pre-zeroed at
`0x39a0c0`–`0x39a0dc`". That is wrong.

- **Twelve** zero-word stores `0x39a0c0`–`0x39a0ec` (`str r6,[sp,#imm]`,
  `r6=0`) clear **48 of the 52 bytes**, at `sp+0x1c/24/28/2c/30/34/38/3c/40/44/
  48/4c`.
- The one non-zeroed word is **`sp+0x20`** (buffer `+0x04`): the store sequence
  jumps `0x39a0bc → 0x39a0c0`, skipping `sp+0x20`. `0x39a0bc str r3,[sp,#0x20]`
  writes **`*(arg1+0x4)`** into `sp+0x20` (`r3` loaded at `0x39a0b4 ldr r3,[sl,#4]`).
- `sp+0x24` (buffer `+0x08`) **is** among the 12 zero-stores (`0x39a0c4 str
  r6,[sp,#0x24]`).

So `*(arg1+0x4)` is the single residual input word; the other 48 bytes are
zeroed. The earlier statement "pre-zeroed" must read "48 of 52 bytes zeroed;
`+0x04` holds the input".

## Error 2: `*(arg1+0x1c)` stashed at `sp+0x24` is dead, and `0x39a350` is an add

`1e0bac5` ties the ring handle to `sp+0x24`. Two corrections:

- `0x39a06c str sb,[sp,#0x24]` stashes `*(arg1+0x1c)` (read `0x39a03c ldr
  sb,[sl,#0x1c]`) into `sp+0x24`, **but** the zero-store at `0x39a0c4`
  overwrites it before the `ioctl`. The stash is **dead** before the call and
  cannot be identified with any post-call read.
- `0x39a350` is `add r3, sp, #0x18` (raw `18308DE2`; `#0x18` = decimal 24),
  computing `sp+0x18` (four bytes before the buffer at `sp+0x1c`). It is **not** a load of the `sp+0x24` pointer. The genuine
  load of `sp+0x24` is a separate site, `0x39a370 ldr r1,[sp,#0x24]`.

## What is correct in `1e0bac5` (unchanged)

- The 52-byte output buffer is the per-call stack temp at `sp+0x1c`; it is
  passed to wrapper `0x39af4c` via `r9 = sp+0x1c` (`0x39a098 add sb,sp,#0x1c`;
  `0x39a0b8 mov r0,sb`; `0x39a0f0 bl 0x39af4c`).
- It is re-read field-by-field after the call (success path `0x39a320`;
  immediate branch `0x39a19c`) at `+0x00/08/0c/10/14/18/2c`.
- The kernel source is the global `now` (`.bss+0xbd0`, `0x34` = 52 B).

## New fact: the 52-byte block is kernel write-only for `H265_CMD_02`

On `hevc.ko`'s `0xc0344802` branch (`0x584`–`0x924`), the only user-copy is
`0x914 __copy_to_user(dst=arg, src=sp-mirror, 0x34)` (sym#97). A full
`.rel.text` scan shows the three `__copy_from_user` sites (sym#98) are at
`0x674` (56 B, in the `0x40384805` branch) and `0x6c0` (56 B, same branch)
and `0x7b0` (4 B, in the `0x40044806` branch) — **none is reachable from the
`0xc0344802` path**. The kernel **does not read** the caller's buffer on this
command; input `+0x04` (`*(arg1+0x4)`) is not kernel-consumed, and the seven
post-call re-reads read only kernel-written bytes.

## What remains unresolved (unchanged stop edge)

Per-field semantics of the 52-byte `now`/descriptor remain undecoded (raw
189-entry register-table image, no struct symbols). Binding `arg1+0x1c` to a
specific ring buffer is an indirect edge and is **not** inferred from the dead
`sp+0x24` stash. No ABI invented; no decodable frame, NAL, or video.

## Reproducibility

`Sofia` `71137674dfbbeb45…`; `hevc.ko` `c491815a11eeaf69…`. Sites:
`0x39a03c` `ldr sb,[sl,#0x1c]`; `0x39a06c` `str sb,[sp,#0x24]` (dead);
`0x39a098` `add sb,sp,#0x1c`; `0x39a0b4` `ldr r3,[sl,#4]`; `0x39a0b8`
`mov r0,sb`; `0x39a0bc` `str r3,[sp,#0x20]`; `0x39a0c0`–`0x39a0ec` 12×
`str r6,[sp,#…]` (zero-run, incl. `sp+0x24` @ `0x39a0c4`); `0x39a0f0`
`bl 0x39af4c`; `0x39a320`/`0x39a370`/`0x39a324`/`0x39a338`/`0x39a380`/
`0x39a360`/`0x39a19c` post-call reads; `0x39a350` `add r3,sp,#0x18`;
`0x39a370` `ldr r1,[sp,#0x24]`. Kernel: `0x914 __copy_to_user` (sym#97);
no `__copy_from_user` (sym#98) on the `0xc0344802` path; `handle_bgn`
`0x744`. No camera/router/LAN device or credential store contacted.
