# H.264, raw frames, and audio

## H.264 experiments

The stock DVRIP capability mask advertised H.265 (`0x100`), and ordinary H.264 requests were rejected or clamped. Adding H.264 to the custom profile changed the mask to `0x180` and let `Simplify.Encode` persist H.264, but the live driver and actual RTSP frames remained HEVC. Changing detailed `AVEnc.Encode` profiles likewise failed and could remove the substream. The H.265 profiles were restored.

A first **RAM-only** `Sofia` patch changed a main-channel H.265 VENC type (`0x109`) to H.264 (`0x60`) but left the later H.265 channel marker (`8`, not H.264's `7`). It produced an H.264 SDP with **no video frames**. Its exact runtime error was not captured; the inconsistent marker is a static-analysis hypothesis. A second one-byte RAM patch at July 2023 TB `Sofia` file offset `0x2b94c4` selected its complete H.264 branch. Sofia requested H.264 (`enType=96`), but `XM_MPI_VENC_CreateChn [0]` failed with `0xa0078007` (`XM_ERR_VENC_NOT_CONFIG` in the OpenIPC headers); `/proc/umap/venc` then listed only JPEG. Timed reboot restored real HEVC frames. SDP and API readback are not success criteria; *actual H.264 packets/frames* are.

Vendor `venc.ko` contains generic H.264 strings, but stock firmware loads `hevc.ko` and exposes `/dev/h265`, with no H.264 module/device. A matching-kernel-vermagic OpenIPC `xm530_h264.ko` was tried **from RAM** after stopping users of `/dev/h265` and unloading `hevc.ko`. It logged `HW not found at 0x20070000`. That generic driver expected a Hantro ID at its base; a read-only register check returned `0x0055D4A8` there. The stock HEVC block at `0x21000000` returned `0x20211019`. Thus the ordinary XM530 H.264 driver is a poor match for XM530**V200**. The stock HEVC driver's ioctls handle register arrays/start/reset/wait, not an evident codec selector. These facts do not conclusively exclude an undocumented V200 H.264 implementation.

The exact-board Sofia binary includes `x264_encoder_*` names, but Ghidra showed its weak `x264_encoder_open_164` prints and returns null; `x264_encoder_encode` returns `-1`. No real linked `libx264.so` was found. This is a stub, not a fallback. Separately, a real static ARMv7/NEON x264 CLI built from VideoLAN commit `0480cb0` ran from camera RAM. At 1920×1080, `ultrafast`, `zerolatency`, one thread, 7-fps black input, it was OOM-killed before one frame while Sofia ran. With Sofia stopped it encoded **one** easy black frame at about 2.88 fps, then was OOM-killed. The kernel managed about 30 MB normal RAM with video buffers in reserved MMZ. This argues against off-the-shelf full-HD x264 on the current memory layout; it does not test every possible low-memory encoder.

No H.264 module, patched Sofia, or x264 binary was flashed. The working camera-side streams remained H.265. A future H.264 route needs V200-specific hardware evidence or a demonstrably low-memory implementation, not another UI toggle.

## Raw video to a server

1920×1080 YUV420 is roughly 3.11 MB/frame. At 7 fps that is about 174 Mbit/s before overhead, beyond the camera's 100-Mbit Ethernet; at 20 fps it is about 498 Mbit/s. Sending H.265/JPEG to a server and transcoding there is a different architecture: the camera would still emit H.265/JPEG. It may solve downstream compatibility but not camera-origin H.264.

## Audio

RTSP audio was mono 8 kHz G.711 A-law (`pcm_alaw`). ONVIF apparently accepted an LPCM request, but subsequent readback and `ffprobe` still showed G.711. No higher-rate or higher-bit-depth camera-side PCM capture was demonstrated. Server-side PCM transcoding cannot restore detail already lost at capture/encoding.
