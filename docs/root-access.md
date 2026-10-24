# Root access: what we actually used

This is a record of an experiment on one of our own `XM530V200_X4C-WQ_8M` cameras, not a claim that every XM530 camera has the same firmware behavior. The access path came from [zhellmann/xm530](https://github.com/zhellmann/xm530), which identified two boot hooks in this SoC family's firmware.

## 1. Reversible microSD boot hook

On the tested firmware, `/usr/bin/dvrHelper` mounts a FAT32 microSD card at `/home` and runs `/home/sd_run.sh` during boot. We prepared an SD card whose script first copied the six MTD partitions and writable configuration to the card, then bind-mounted a modified `Encode.json` **in RAM** and started a temporary maintenance shell. The camera booted with the SD card and we obtained root-level filesystem access. No flash partition was written by this first-stage SD script.

The historic script launched an **unauthenticated** shell and FTP service on the household LAN. This was a risky expedient, not a recommended deployment. We are not publishing the runnable script or listener here. A future reproduction should use an isolated network, authenticated access, a short timeout, and an explicit removal check. A complete off-device flash backup and a tested recovery path should come first.

Removing the SD card and power-cycling reverses the RAM-only overlay. It does **not** reverse any later changes deliberately written to flash.

## 2. Temporary custom-partition hook

We later needed to inspect and change the persistent encoder database. The stock boot sequence runs `/mnt/custom/extapp.sh` when present. A modified copy of the **exact board's** custom partition temporarily launched a maintenance listener before Sofia started. This stage *did* write the custom flash partition and carried a higher recovery risk than the SD overlay. Once the experiment was finished, we reapplied a clean matching custom/stock image and removed the maintenance listener and temporary network export.

The access was real enough to alter the saved encoder profile and observe Sofia attempt H.264 at 1920×1080. That attempt failed in the installed encoder library (`x264_param_default_preset is not supported`), and the camera was restored to its working H.265 stream. See [findings](findings.md).

## What did not work

- The ordinary authenticated DVRIP/ONVIF codec setting was clamped back to H.265.
- Generic `telnetctrl` advice is unreliable on this XM530 firmware; [XM530 research](https://github.com/zhellmann/xm530) explains why.
- Upgrade packages containing `Shell` commands were accepted far enough to test, but this firmware silently dropped those commands. We did not obtain root that way.
- A possible hostname-command-injection route was considered, but **not confirmed** as the method that gave us root.

## Current access state

The shell was temporary. On 2026-10-24, a read-only LAN check found the historic maintenance port closed on both cameras. The main RTSP streams still worked. No root service is included in this repository or currently needed for normal recording.

Do not treat a successful API reply or an encoder setting as proof of the output codec: verify the *actual RTSP bitstream* with `ffprobe` after any change.
