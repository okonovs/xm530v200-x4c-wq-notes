# OpenIPC Firmware Build for XM530V200 — 2026-11-12

## Summary

A complete OpenIPC firmware image was built for the Xiongmai **XM530V200** SoC
(X4C-WQ camera, device ID `000807A8`) using the `xm530v200_lite` Buildroot
defconfig. The build ran in a Colima/Ubuntu 22.04 amd64 Docker container to
bypass macOS/BSD toolchain incompatibilities. The image builds cleanly but
**excludes the `majestic` video-capture backend**, so it provides boot + network
+ SSH but **no video capture**.

> **Not tested on hardware.** This image has not been flashed to a physical
> camera. Boot and network behavior are unverified. Flashing is an owner
> decision that requires a UART + TFTP recovery setup (see [Recovery](#recovery)).

## Image artifacts

The firmware binary artifacts are **not published** to this repository (the
`.gitignore` blocks `*.bin`, `*.img`, `*.tar`, `*.tgz`, `*.log`). They reside
in the private build directory on the research host.

| File | Size | Description |
|------|------|-------------|
| `openipc.xm530v200-nor-lite.tgz` | 3.8 MB | Packaged firmware (uImage + rootfs.squashfs + md5sums) |
| `uImage.xm530v200` | 1.2 MB | U-Boot kernel image (1175 / 2048 KB) |
| `rootfs.squashfs.xm530v200` | 2.6 MB | Compressed root filesystem (2540 / 5120 KB) |
| `rootfs.cpio` | 5.5 MB | Uncompressed root filesystem |
| `rootfs.xm530v200.tar` | 6.3 MB | Uncompressed root filesystem (tar) |

## What and how the image was made

### 1. Source

- **OpenIPC firmware repo**: cloned (shallow) from
  `https://github.com/OpenIPC/firmware` to the research host build directory.
- **Defconfig**: `br-ext-chip-xiongmai/configs/xm530v200_lite_defconfig` — a
  dedicated Xiongmai defconfig with a sensor driver
  (`BR2_PACKAGE_XIONGMAI_OSDRV_XM530V200=y`) but **no** `majestic` (video
  backend), matching the upstream status from
  [issue #1927](https://github.com/OpenIPC/firmware/issues/1927).
- **Buildroot**: 2024.02.10
- **Target**: `arm-openipc-linux-musleabi`

### 2. Prebuilt toolchain (no host gcc bootstrap)

The OpenIPC build uses a **prebuilt ARM toolchain**, not a host-compiled gcc:

- Tarball: `toolchain.xiongmai-xm530.tgz` (~101 MB), downloaded during the build
  to `output-xm530v200/buildroot-2024.02.10/dl/toolchain-external-custom/`.
- Extracted to:
  `output-xm530v200/per-package/toolchain-external-custom/host/opt/ext-toolchain/arm-openipc-linux-musleabi/`
- Contains: `bin`, `lib`, `include`, `libexec`, `sdk`, `relocate-sdk.sh`, etc.
- This eliminated the need to cross-compile a gcc toolchain from source on the
  build host, which was the original macOS blocker.

### 3. Build environment

macOS (BSD userland) cannot run the OpenIPC/Buildroot build natively — BSD
`tar`, `sed`, `make`, and the lack of GNU `install`/`flock`/`gpatch`/`gcc` cause
failures. The build was therefore run inside a **Colima Linux VM** with Docker:

- **VM**: Colima, Ubuntu 22.04, amd64, glibc 2.35
- **Docker image**: `openipc/chaos_calmer_toolchain:200806` (788 MB, OpenIPC's
  official build image)
- **Host-side shims** (for any host-side tool invocations):
  - `sed` → `gnu-sed` (`gsed`)
  - `gcc` / `g++` → Homebrew `gcc-16`
  - `patch` → `gpatch`
  - `flock` → `util-linux`
  - `bash` → Homebrew `bash` 5.3 (macOS ships 3.2, which lacks `mapfile`)

### 4. Build invocation

```sh
export DOCKER_HOST=unix:///path/to/.colima/rosetta/docker.sock

docker run --rm -it \
  -v <build-dir>/firmware:/work/firmware \
  -v <build-dir>/output-xm530v200:/work/output \
  -v <build-dir>/images:/work/images \
  openipc/chaos_calmer_toolchain:200806 \
  bash -c 'cd /work/firmware && \
    export OPENIPC_SOC_VENDOR=xiongmai \
          OPENIPC_SOC_FAMILY=xm530 \
          OPENIPC_SOC_MODEL=xm530v200 && \
    make BOARD=xm530v200_lite \
         TARGET=/work/output && \
    cp output/build/openipc/*.tgz /work/images/ && \
    cp output/build/openipc/*.squashfs /work/images/ && \
    cp output/build/openipc/uImage* /work/images/ && \
    cp output/images/rootfs* /work/images/ 2>/dev/null; \
    echo "done"' > build-ubuntu22.log 2>&1
```

### 5. Build result

- **Exit code**: 0 (clean)
- **Build time**: 51 minutes 58 seconds
- **Packages built**: 62 (busybox, dropbear, libcurl, atbm603x Wi-Fi driver, etc.)
- **Last lines of log**:
  ```
  - uImage: [1175KB/2048KB]
  - rootfs.squashfs: [2540KB/5120KB]
  - Build time: 51:58
  ```

## Known limitations

### No video capture

The `xm530v200_lite_defconfig` explicitly excludes `BR2_PACKAGE_XIONGMAI_MAJESTIC`.
This means the firmware **cannot capture video**. The V200's `sys/vb/vpss/venc`
media interface is not driven. Boot, network, and SSH (dropbear) should work,
but the camera will produce no video stream.

This matches the upstream status: [OpenIPC issue
#1927](https://github.com/OpenIPC/firmware/issues/1927) was closed on 2026-10-04
by a maintainer stating that XM530V200 is a different chip and the XM530 image is
not expected to boot on it. The `lite` defconfig is a better match than the raw
XM530 image, but it is **not tested on actual X4C-WQ hardware**.

### Identity mismatch

The camera is **X4C-WQ, device ID `000807A8`**, but OpenIPC's published Xiongmai
support list names **XM510, XM530, XM550** — not XM530V200. The device ID
`000807A8` (or `000807CF`) is not in OpenIPC's catalogue.

## Recovery

The camera's stock flash layout:

```
256 KiB  boot    (U-Boot)
1536 KiB kernel
1280 KiB cramfs  (romfs)
4544 KiB user
256 KiB  custom
320 KiB  mtd
```

OpenIPC writes to the kernel + romfs + user partitions. The **boot** partition
(U-Boot) is typically left untouched, enabling the standard Xiongmai recovery:

1. **UART + TFTP** (primary recovery): Connect a 3.3V TTL serial adapter,
   interrupt U-Boot at the serial prompt, and TFTP-boot or TFTP-flash a
   known-good stock image. See [OpenIPC Coupler
   docs](https://github.com/OpenIPC/coupler).
2. **SPI flash reprogrammer** (last resort): if UART fails.

### Recommended safe path before any flash

1. **Full hardware backup**: `ipctool` backup over NFS before flashing:
   ```sh
   python3 telnet_opener.py <camera-ip> -b --nfs <nfs-server>:/srv/ipctool
   ```
2. **Non-persistent TFTP boot test**: TFTP-boot the OpenIPC image *without*
   writing to flash. If it works, you've verified bootability with zero risk.
   If it fails, just power-cycle back to stock.
3. **UART connected** before writing anything to flash, so recovery is possible.

> **Warning**: A power cycle does **not** recover an incompatible kernel or
> partition image. If the OpenIPC image fails to boot on this hardware and
> overwrites the kernel/romfs partitions, the camera can only be recovered via
> UART/TFTP or a SPI flash reprogrammer.

## Reproducing the build

1. Clone the OpenIPC firmware repo:
   ```sh
   git clone --depth=1 https://github.com/OpenIPC/firmware
   ```
2. Set up a Colima VM with Ubuntu 22.04:
   ```sh
   colima start --arch x86_64 --image ubuntu:22.04
   ```
3. Pull the OpenIPC build image:
   ```sh
   docker pull openipc/chaos_calmer_toolchain:200806
   ```
4. Run the build (see [Build invocation](#4-build-invocation) above).
5. Copy the resulting `openipc.xm530v200-nor-lite.tgz` from the images directory.

## References

- [OpenIPC firmware issue #1927](https://github.com/OpenIPC/firmware/issues/1927)
  — XM530V200 boot failure report
- [OpenIPC Coupler](https://github.com/OpenIPC/coupler) — UART/TFTP recovery
- [OpenIPC Xiongmai vendor page](https://openipc.org/cameras/vendors/xiongmai)
  — published support list (XM510, XM530, XM550)
- [IPG-X4C-WER board page](https://openipc.org/cameras/boards/xiongmai-ipg-x4c-wer)
  — board catalogue (device ID `000807CF`, not our `000807A8`)
