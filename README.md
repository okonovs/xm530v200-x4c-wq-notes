# XM530V200 X4C-WQ camera research

Field notes from two personally owned Xiongmai/XMEye cameras in the `XM530V200_X4C-WQ_8M` family (firmware ID `000807A8`). This repository records what worked, what failed, and what remains unverified. It is not a firmware distribution or a universal unlock tool.

## Key findings

- A FAT32 microSD boot hook (`sd_run.sh`) gave us a **temporary root shell** on one camera. We later used a short-lived custom-partition maintenance hook, then removed it. See [root access and recovery](docs/root-access.md).
- The stock firmware advertises H.265 only. Forcing H.264 into its real encoder profile made the camera request H.264, but Sofia failed channel creation with `x264_param_default_preset is not supported`. Config changes alone did **not** produce H.264.
- Both cameras currently output H.265 at 2560×1440 on their main streams. That is an encoded frame size, **not proof** of 4 MP optical detail. A same-board vendor profile lists a 1920×1080 `CapWHXY` field; its meaning in the live capture pipeline remains unverified. See [imaging and codec findings](docs/findings.md).
- The stock microphone stream is 8 kHz mono G.711 A-law. A requested LPCM change was accepted by the API but did not change the actual RTSP audio codec.

## Boundaries

These notes concern our own devices. Do not test on systems you do not own or administer. The historic maintenance listener had no authentication and would have exposed root to the LAN; it was removed after the experiment. This repository deliberately does not include that listener, passwords, private IP addresses, MAC addresses, camera footage, flash dumps, or proprietary firmware. See [SECURITY.md](SECURITY.md).

The source projects that made the investigation possible are credited in [REFERENCES.md](REFERENCES.md). Their code and licenses remain with their authors; none is vendored here.

Original text in this repository is released under the [MIT License](LICENSE). No Xiongmai firmware is licensed or redistributed by that statement.
