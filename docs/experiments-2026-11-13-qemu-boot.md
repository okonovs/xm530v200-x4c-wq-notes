# EXPERIMENTS.md

## 2026-11-13: QEMU boot of XM530V200 OpenIPC firmware

### Hypothesis
The OpenIPC uImage + initrd can boot in QEMU's custom `xm530v200` machine and
reach a login prompt.

### Command
```bash
QEMU=/tmp/qemu-bench/qemu-src/build/qemu-system-arm-unsigned
KERNEL=/Users/oko/Projects/openipc-build/images/uImage.xm530v200
INITRD=/tmp/initrd-full.cpio  # cpio from /Users/oko/Projects/openipc-build/images/rootfs-extract/

timeout 30 $QEMU \
    -M xm530v200 \
    -kernel $KERNEL \
    -initrd $INITRD \
    -display none \
    -monitor none \
    -nographic \
    -serial file:/tmp/qemu-boot-test/serial.log
```

### QEMU build
- **Base**: QEMU v10.2.0 (commit 698104725efad4b29079d857dfdebbd804e34c99)
- **Machine**: `xm530v200` from OpenIPC/qemu-bench `qemu/hw/arm/xiongmai.c`
- **Host**: macOS 27.2 arm64, built with meson + ninja
- **Missing device types added** (were not in meson.build):
  - `xm-dwmac.c` (hw/net) — Xiongmai DWMAC ethernet
  - `xm-i2c.c` (hw/i2c) — Xiongmai I2C bus
  - `xm-gpio.c` (hw/gpio) — Xiongmai GPIO
  - `arm-sp805.c` (hw/watchdog) — ARM SP805 watchdog
- **Missing HISI_* symbols** added to Kconfig:
  - `HISI_MISC`, `HISI_FEMAC`, `HISI_GMAC`, `HISI_HIETH_SF`, `HISI_I2C`, `HISI_SPI`
- **Missing headers** copied from `qemu/include/`:
  - `hw/i2c/ingenic-sensor.h`
  - `hw/mips/ingenic.h`
- **Binary**: `/tmp/qemu-bench/qemu-src/build/qemu-system-arm-unsigned` (33 MB)
  - Code signing not attempted (macOS); unsigned binary runs locally.

### Boot log (abridged)
```
Uncompressing Linux... done, booting the kernel.
Linux version 3.10.103+ (root@e61d783c847f) (buildroot-gcc-13.3.0) #2 SMP Thu Nov 12 14:01:34 UTC 2026
Machine: xm530v200
Memory: 128MB = 128MB total
...
Serial: AMBA PL011 UART driver
uart:0: ttyAMA0 at MMIO 0x10030000 (irq = 32) is a PL011 rev1
console [ttyAMA0] enabled
...
XMSFCV2: flash name:W25Q64CV size:0x800000
XMMMAC driver: eth0 - (dev. name: xmmaceth - id: -1, IRQ #35)
libphy: XMMMAC MII Bus: probed
eth0: PHY ID 0007c0f1 at 1 IRQ 0 (0:01) active
sp805-wdt sp805-wdt: registration successful
...
Trying to unpack rootfs image as initramfs...
Freeing initrd memory: 5372K
...
Starting syslogd: OK
Starting klogd: OK
udhcpc: lease of 10.0.2.15 obtained from 10.0.2.2, lease time 86400
Starting dropbear: OK
Starting crond: OK
Welcome to OpenIPC
openipc-xm530v200 login:
```

### Result
**PASS**: The OpenIPC firmware boots to a login prompt in QEMU.
- Kernel: 3.10.103+ (buildroot-gcc-13.3.0)
- Machine: xm530v200 (Cortex-A5 stand-in A9, GIC, uMCTL2, 128 MB RAM)
- Ethernet: eth0 (xmmaceth) probed, DHCP lease obtained from QEMU's slirp
- Services: syslogd, klogd, udhcpc, ntpd, dropbear (SSH), crond all started
- No kernel panic; QEMU ran the full 30-second timeout with a login prompt pending

### Known issues in the boot log
- `GIC CPU mask not found - kernel will fail to boot` — printed twice but boot continues
- `dma-pl330: PERIPH_ID 0x0, PCELL_ID 0x0 !` — DMA controller probe fails (-22)
- `mount: mounting /dev/ on /overlay failed: Block device required` — flash block device not mapped
- `pivot_root: Invalid argument` — overlay root mount fails, falls back to initrd
- `modprobe: module exfat not found in modules.dep` — non-critical
- These are expected in a minimal QEMU setup without a full flash image

### Conclusion
QEMU is NOT blocked. The OpenIPC/qemu-bench provides a working `xm530v200` machine
that boots the OpenIPC firmware to userspace. The previous claim that "QEMU has no
XM530 machine" was incorrect — it conflated stock upstream QEMU with OpenIPC's
custom build.

The firmware reaches a login prompt, DHCP lease, and SSH server. What remains:
1. A full flash image (NOR SPI + rootfs + U-Boot) booted via `-M xm530v200,flash-file=...`
2. Vendor kernel modules loaded in QEMU (hevc, vpss, venc, isp)
3. A minimal H.265 pipeline that produces decodable output in QEMU

### Artifacts
- Serial log: `/tmp/qemu-boot-test/serial.log`
- QEMU stderr: `/tmp/qemu-boot-test/qemu.stderr.log`
- Test harness: `qemu-boot/test-xm530v200.sh` (in notes repo)
- QEMU binary: `/tmp/qemu-bench/qemu-src/build/qemu-system-arm-unsigned`
