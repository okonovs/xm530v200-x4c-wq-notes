# Ioctl constant audit — 2026-11-13

## Method

The V200 kernel modules (`venc.ko`, `hevc.ko`, `vpss.ko`, `isp.ko`, `sys.ko`) are
stripped of symbols but retain their `.text` sections. The ioctl request constants
are embedded in the code as ARM `MOVW`/`MOVT` instruction pairs that build 32-bit
values. These were decoded by scanning the `.text` section for `MOVW`/`MOVT` pairs
whose assembled value has a known ioctl type byte (`0x48` for hevc, `0x49` for
venc, `0x50` for vpss, `0x10` for isp).

## Results

### VENC (venc.ko, type=0x49)

| Constant | Direction | Size | Nr | Description |
|----------|-----------|------|----|-------------|
| `0x40284903` | WRITE | 0x028 | 0x03 | GET_INFO? |
| `0x40104904` | WRITE | 0x010 | 0x04 | GET_BUF? |
| `0x41044901` | WRITE | 0x104 | 0x01 | RET_BUF? |
| `0xc0284902` | READ\|WRITE | 0x028 | 0x02 | RELEASE? |

### H265 (hevc.ko, type=0x48)

| Constant | Direction | Size | Nr | Description |
|----------|-----------|------|----|-------------|
| `0x40044806` | WRITE | 0x004 | 0x06 | GET? |
| `0x40384805` | WRITE | 0x038 | 0x05 | READ_BLOCK |
| `0xc0344802` | READ\|WRITE | 0x034 | 0x02 | WRITE_BLOCK |

### VPSS (vpss.ko, type=0x50)

| Constant | Direction | Size | Nr | Description |
|----------|-----------|------|----|-------------|
| `0x40805001` | WRITE | 0x080 | 0x01 | READ_01 |
| `0x40805002` | WRITE | 0x080 | 0x02 | READ_02 |
| `0x40805004` | WRITE | 0x080 | 0x04 | READ_04 |
| `0x40805006` | WRITE | 0x080 | 0x06 | READ_06 |
| `0x40805007` | WRITE | 0x080 | 0x07 | READ_07 |
| `0x4080500a` | WRITE | 0x080 | 0x0a | READ_0A |
| `0x4080500c` | WRITE | 0x080 | 0x0c | READ_0C |
| `0x4080500d` | WRITE | 0x080 | 0x0d | READ_0D |
| `0x4080500e` | WRITE | 0x080 | 0x0e | READ_0E |
| `0x4080500f` | WRITE | 0x080 | 0x0f | READ_0F |
| `0x40805012` | WRITE | 0x080 | 0x12 | READ_12 |
| `0x40045008` | WRITE | 0x004 | 0x08 | READ_08 |
| `0x40045009` | WRITE | 0x004 | 0x09 | READ_09 |
| `0x420c5011` | WRITE | 0x20c | 0x11 | READ_11 |
| `0xc0805003` | READ\|WRITE | 0x080 | 0x03 | SET_03 |
| `0xc0805005` | READ\|WRITE | 0x080 | 0x05 | SET_05 |

### ISP (isp.ko, type=0x10)

| Constant | Direction | Size | Nr | Description |
|----------|-----------|------|----|-------------|
| `0x340010b4` | WRITE | 0x400 | 0xb4 | ? |
| `0x34081014` | WRITE | 0x408 | 0x14 | ? |
| `0x3408101c` | WRITE | 0x408 | 0x1c | ? |
| `0x34081010` | WRITE | 0x408 | 0x10 | ? |
| `0x34081018` | WRITE | 0x408 | 0x18 | ? |

## Caveats

- The direction field (bits 31-30) describes the encoded userspace/kernel transfer
  convention, **not** necessarily every side effect of the driver. A request with
  `direction=WRITE` may still have read side effects.
- The struct layouts are **not confirmed** — only the sizes are known from the
  ioctl encoding. The actual field offsets and types require on-device testing
  or deeper disassembly.
- `sys.ko` has **no ioctl constants** in its `.text` section — it may use a
  different mechanism (e.g., `write()`/`read()` on the char device).

## QEMU boot blocker

The kernel is built for the XM530 platform (`CONFIG_XM530_MACHINE_NAME="xm530v200"`).
QEMU does not have an XM530 machine type, so the kernel cannot be booted without
either writing a custom QEMU machine or using physical hardware.
