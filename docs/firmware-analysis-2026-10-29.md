# Offline firmware analysis — 2026-10-29

This update summarizes analysis of archived XM530V200/X4C-WQ firmware and earlier owner measurements. The scheduled analysis on 2026-10-29 used copied files only; it did not contact or alter either camera. A finding about an archived binary is not automatically a finding about the binary currently installed on either device.

## Findings and evidence limits

| Area | Finding | Limit |
| --- | --- | --- |
| Main-stream H.264 | Earlier live tests produced no decodable H.264 frames. The archived stock image loads an H.265 driver and contains no shipped H.264 driver. The generic Hantro driver tried earlier did not recognize the probed register value. | This rules out the tested routes. It does **not** prove that the board lacks every possible H.264 hardware path. A working path requires actual decodable frames, not an API readback or SDP. |
| Sensor selection | The archived ISP binary distinguishes SC223A and SC223AP using a sensor register after a shared chip-ID check. It also has a third recognized profile. All three traced profiles configure a 1920×1080 video mode. | The code proves supported profiles, not which sensor is fitted in *pagalms*. The SC223A identification for *cels* also uses earlier live evidence; its installed ISP binary has not been hashed against the archive. |
| 1080p to 1440p enlargement | The archived VI and VPSS code selects an enlargement path when destination dimensions exceed capture dimensions, allocates the destination buffer, and programs ISP/VPSS registers. A previous live VI/ISP readout on *cels* showed 1920×1080 capture enlarged to 2560×1440. | The interpolation tap kernel is inside the hardware path; the static code does not reveal its exact filter. The older live observation does not by itself establish *pagalms*' sensor mode. |
| Image controls | The archived ISP contains denoise, defog, dynamic-range, and sharpening APIs. The shipped configuration exposes color controls but does not expose those internal stage parameters. | The presence of an API does not establish active values or safe tuning ranges. Do not change hidden parameters from strings alone. |
| Persistence and recovery | Archived boot scripts contain an optional custom-partition startup hook. The owner-added maintenance service uses a source-IP restriction without authentication. The research also matched an archived stock image to an earlier backup by hash. | This is a security concern, not a recommended permanent access method. A matching archived image does not guarantee that reflashing a particular live device is safe; device-specific state and recovery must be checked first. |

The analysis produced local disassembly reports and verifier scripts. Checks independently reconstructed the ISP constants `0x4bda12f7 / 2^32 ≈ 8/27` and `0xaaaaaaab / 2^32 ≈ 2/3`. A prior report mislabeled the first value as `10/11`; that claim was corrected. The verifiers also traced the archived sensor profile branches and the 1920×1080 setters. These checks strengthen the static interpretation; they are not substitutes for a live stream test.

## Current H.264 conclusion

The stock software path and tested generic driver do not provide camera-side H.264 on these units. The next useful offline step is a type-aware trace of the per-channel codec field in `Sofia`: identify exactly where its value comes from and verify each caller and branch. Whether a compatible encoder block exists on this V200 board remains open. A camera-side success claim needs captured H.264 packets and decodable frames from a specifically authorized live test.

## Provenance

- Static corpus: archived 2022 standard and July 2023 TB firmware for this board family. Private reports record SHA-256 values, binary offsets, disassembly, commands, failed hypotheses, and verifier output. The firmware, dumps, executable maintenance payload, credentials, and device-specific backups remain outside this repository.
- Earlier live observations: the owner measurements summarized in [imaging and sensor](findings.md) and [H.264 and audio](h264-investigation.md). They predate this offline pass.
- Work tracking and acceptance criteria: [research roadmap](research-roadmap.md).
