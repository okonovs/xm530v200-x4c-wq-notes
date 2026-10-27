# Experiment chronology and open work

This log highlights failures easily mistaken for success from API readback. Dates refer to research, not product releases.

| Date | Experiment | Observation and resulting state |
| --- | --- | --- |
| Before 2026-10-24 | API image/codec/audio changes | H.264 and LPCM labels did not change actual RTSP codecs; exposed noise/acuity controls were reduced. |
| 2026-10-24 | Ghidra on archived 2022 standard and July 2023 TB builds | `CapWHXY` supplied 1920×1080 capture; `CaptureSizes.4M` supplied 2560×1440 destination; driver enlargement path found. No flash from analysis. |
| 2026-10-24 | Temporary root diagnostic on cels | Live VI showed 1920×1080 → 2560×1440; ISP `2M->4M`; sensor ID 41, 1928×1088 active area. |
| 2026-10-24 | Cels `CaptureSizes.4M` set to `[1920,1080,1080]` | VI capture = destination 1080p; RTSP produced real 1080p HEVC, substream still worked. Retained. |
| 2026-10-24 | H.264 capability/detailed profiles | H.264 readback but actual main HEVC; substream could disappear. H.265 profiles restored. |
| 2026-10-25 | RAM-only Sofia H.264 tests | H.264 SDP with no frames; complete branch failed VENC creation `0xa0078007`. Timed reboot restored HEVC. |
| 2026-10-25 | Generic XM530 H.264 driver from RAM | `HW not found` at expected Hantro base; stock HEVC uses another base. Restored on reboot. |
| 2026-10-25 | Real x264 from RAM | Full-HD 7-fps black test OOM-killed; one frame only with Sofia stopped. Not flashed. |
| 2026-10-25 | Corrected cels custom maintenance image | Three separate UID/GID-0 sessions and working RTSP. Source-restricted but unauthenticated listener left installed with owner approval. |
| 2026-10-25 | Cels `SensorFPS_PAL/NTSC` 20→25-only custom test | VI/H26X config said 25; ISP and real frames remained 20. Rolled back to known-good custom image. |
| 2026-10-25 | Encoder changes | Cels quality 6 made `QPMax` 45→40; GOP 1 made a 20-frame interval. A 16-Mbit setting remained 8 Mbit live and was reverted. Both substreams' bitrate increased; cels sub quality increased. |
| 2026-10-25 | Pagalms direct 1080P and shell-only upgrade | `1080P` rejected (`Ret=606`); shell commands rejected/ignored. No custom flash; output remained 1440p. |
| 2026-10-25 | Final short RTSP probes | Both cameras' main and substreams yielded real HEVC frames; cels main 1080p, pagalms main 1440p. |

## Priority next measurements

1. Back up **pagalms' own** custom partition and read its live VI/ISP/sensor state before any native-size flash change. Do not assume cels' image fits the older build.
2. Find a *verified* SC223A 25-fps sensor mode for this exact board/clock/driver. JSON FPS alone demonstrably does not work; count distinct frames and inspect live ISP timing.
3. Compare controlled daylight/night scenes and long-running RTSP/Frigate drop statistics after quality/GOP changes. Investigate hidden ISP sharpening/NR without blind register writes.
4. Remove cels' unauthenticated maintenance listener when no longer needed, or replace it only after designing and testing key-authenticated access and recovery.
5. For H.264, seek V200-specific hardware documentation/driver or a proven low-memory implementation. UI toggles and SDP alone are not progress.

Preserve known-good real HEVC video and test one variable at a time. A physical power-cycle was needed once despite a successful reboot response, so remote-only recovery is not guaranteed.
