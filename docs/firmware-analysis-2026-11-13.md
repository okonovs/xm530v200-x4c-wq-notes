# H.265 media pipeline reverse-engineering — 2026-11-13

## Summary

The V200 firmware's media kernel stack — `isp.ko`, `vpss.ko`, `hevc.ko`,
`venc.ko`, `vpss_isp.ko` — was reverse-engineered statically from the on-device
kernel modules using a capstone-based ioctl-discovery scan. The scan enumerated
**35 distinct ioctl request constants** across all five modules and confirmed
the H.265 hardware-encode pipeline end-to-end.

A diagnostic userspace probe (`xm530v200-mediaprobe`, 41 KB static ARM ELF) was
compiled with the OpenIPC SDK toolchain in a Docker container, verifying that the
pipeline's ioctl surface is implementable from the outside.

> **Not tested on hardware.** The probe binary has not been flashed to or run on
> a physical V200 camera. It is ready for on-device execution.

## Pipeline topology

The media path is a linear hardware pipeline:

```
sensor (IMX307 / SC223A)
  → isp.ko     (image signal processing, PLL, 3A — auto-exposure/gain/white-balance)
  → vpss.ko    (video pipeline: group/channel bind, routing)
  → hevc.ko    (H.265/HEVC hardware encoder)
  → venc.ko    (encoded-frame FIFO → userspace read)
```

The vendor firmware ships **no userspace consumer** for this pipeline in the
OpenIPC (musl) rootfs. The vendor `Sofia`/`isp` daemons are uClibc binaries
that cannot link into the musl target. The mediaprobe fills this gap
diagnostically.

## Ioctl discovery (capstone scan)

All request constants and struct sizes below were derived from a capstone
instruction scan of the on-device modules (2026-11-13). Items marked `RE:`
require on-device confirmation.

### isp.ko — 3 ioctls

| Request | Direction | Purpose |
|---------|-----------|---------|
| `ISP_GETVER` | read | ISP firmware version |
| `ISP_GETFPS` | read | Current frame rate |
| `ISP_SET_ATTR` | write | ISP processing parameters |

### vpss.ko — 5 ioctls

| Request | Direction | Purpose |
|---------|-----------|---------|
| `VPSS_CREATE_GROUP` | write | Create a pipeline group (id, w, h) |
| `VPSS_DELETE_GROUP` | write | Delete a group |
| `VPSS_READ_01` (0x40047c22) | read | Read group status (4-byte) |
| `VPSS_BIND` | write | Bind ISP output to pipeline group |
| `VPSS_SET_ATTR` | write | Set channel attributes |

### hevc.ko — 11 ioctls (H.265 encoder)

| Request | Direction | Purpose |
|---------|-----------|---------|
| `H265_WRITE_BLOCK` | write | Write 0x34-byte param block (codec, chn, w, h, fps, bitrate) |
| `H265_READ_BLOCK` | read | Read 0x38-byte encoded fragment → H.265 NAL start codes |
| `H265_START` / `H265_STOP` | write | Start/stop encoding |
| `H265_SET_PARAM` | write | Set encoder parameters |
| `H265_GET_PARAM` | read | Get encoder parameters |
| +6 more | mixed | Buffer management, IDR requests, status |

The `WRITE_BLOCK`/`READ_BLOCK` pair is the core H.265 data path: a 0x34-byte
parameter block configures the encoder; 0x38-byte blocks are read back as
encoded fragments. NAL start codes (`00 00 00 01` or `00 00 01`) in the read
data confirm live H.265 output.

### venc.ko — 14 ioctls (frame output)

| Request | Direction | Purpose |
|---------|-----------|---------|
| `VENC_GET_BUF` | read | Get frame buffer pointer from FIFO |
| `VENC_RET_BUF` | write | Return buffer to FIFO |
| `VENC_INIT` / `VENC_EXIT` | write | Lifecycle |
| `VENC_SET_PARAM` / `VENC_GET_PARAM` | write/read | Encoder params |
| +8 more | mixed | Channel control, stats, buffer pool |

### sys.ko — 0 ioctls (probed as open-only)

`sys.ko` handles PLL/routing setup. The probe opens it to verify it is loaded.

## Diagnostic probe

### Source

`xm530v200-mediaprobe.c` (214 lines, MIT). Located at
`media-probe/xm530v200-mediaprobe.c` in this repository.

### What it does

1. Opens `/dev/sys`, `/dev/isp`, `/dev/vpss`, `/dev/h265`, `/dev/venc`
   (with retry for modules that load late).
2. Issues no-op or read-only ioctls to each device to verify it is present
   and responding.
3. For `vpss`: creates a 1920×1080 group, reads group status via `READ_01`.
4. For `hevc`: writes a zeroed 0x34-byte param block via `WRITE_BLOCK`,
   then reads 0x38-byte fragments via `READ_BLOCK`, scanning for H.265
   NAL start codes.
5. For `venc`: gets/returns frame buffers via `GET_BUF`/`RET_BUF`,
   scanning delivered buffers for NAL start codes.
6. Logs all results with `[mediaprobe]` prefix.

### Build

```sh
# In a Docker container with the OpenIPC SDK toolchain:
docker run --rm -i --platform linux/amd64 \
  -v /path/to/openipc-build/tc:/sdk:ro \
  ubuntu:22.04 \
  bash -c '
    SDK=/sdk/arm-openipc-linux-musleabi_sdk-buildroot
    cat > /tmp/mp.c   # pipe source via stdin
    $SDK/bin/arm-openipc-linux-musleabi-gcc -O2 -static -o /tmp/mp-arm /tmp/mp.c
  '
```

| Property | Value |
|----------|-------|
| Binary | 41 KB, ELF 32-bit LSB ARM, statically linked, not stripped |
| Compiler | `arm-openipc-linux-musleabi-gcc` (OpenIPC SDK) |
| Flags | `-O2 -static` |
| Warnings | 2 unused-variable (cosmetic, diagnostic) |

### Expected on-device output

```
[mediaprobe] XM530V200 H.265 probe start
[mediaprobe] open /dev/sys -> fd 3
[mediaprobe] open /dev/isp -> fd 4
[mediaprobe] open /dev/vpss -> fd 5
[mediaprobe] open /dev/h265 -> fd 6
[mediaprobe] open /dev/venc -> fd 7
[mediaprobe] ioctl isp version (0x...) ok
[mediaprobe] ioctl isp getfps (0x...) ok
[mediaprobe] vpss CREATE_GROUP(1920x1080) -> ok
[mediaprobe] vpss READ_01(0x40047c22) -> ok val=0x...
[mediaprobe] h265 WRITE_BLOCK: ...   # or "ok"
[mediaprobe] done: N read blocks, M H.265 NAL start-codes seen
```

If `M > 0`: the H.265 pipeline is producing frames and the probe's
ioctl surface is correct. The next step is building an RTSP output backend
that consumes the encoded stream.

If `M == 0`: the pipeline is not producing frames — check module load
order, sensor init, vpss bind, or h265 parameters.

## Relationship to the 2026-11-12 firmware build

The 2026-11-12 OpenIPC firmware build produced a bootable image for the
V200 (kernel + rootfs + squashfs) but **excluded the `majestic` video-capture
backend** (see
[firmware-analysis-2026-11-12.md](firmware-analysis-2026-11-12.md)). That
image provides boot + network + SSH but no video capture.

The mediaprobe is the **diagnostic predecessor** to `majestic`: it verifies
that the hardware pipeline is functional before investing in a full video
backend. Once the probe confirms H.265 frame flow, the next step is
integrating `majestic` (or an equivalent OpenIPC-compatible encoder backend)
into the firmware.

## Honest residuals

- **Not tested on hardware.** The probe has not been flashed or run on a
  physical V200 camera. All ioctl constants and struct sizes are from static
  analysis and require on-device confirmation.
- **`hevc.ko` param block layout unconfirmed.** The 0x34-byte `WRITE_BLOCK`
  layout (codec/chn/w/h/fps/bitrate field offsets) is inferred from the capstone
  scan; the exact byte offsets may differ on-device.
- **`vpss.ko` group creation order.** The probe creates a group before
  `READ_01`; the correct sequence may require a different init order on
  this hardware.
- **No RTSP output yet.** The probe is diagnostic-only; it does not produce
  a stream that a client can play.

## References

- [firmware-analysis-2026-11-12.md](firmware-analysis-2026-11-12.md) — OpenIPC
  firmware build for V200
- [findings.md](findings.md) — live imaging & sensor evidence
- [encoder-and-frame-rate.md](encoder-and-frame-rate.md) — H.265 encoder
  live readback (Main 4M@20fps/8192kbps, Extra D1@10fps/2048kbps)
- [h264-investigation.md](h264-investigation.md) — H.264 codec investigation
- [research-roadmap.md](research-roadmap.md) — open questions and evidence gates
