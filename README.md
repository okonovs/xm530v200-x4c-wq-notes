# XM530V200 X4C-WQ camera research

Field notes from two personally owned Xiongmai/XMEye `XM530V200_X4C-WQ_8M` cameras (firmware ID `000807A8`), called **cels** and **pagalms**. Observations are dated, not a live status page. We distinguish actual stream/driver evidence from firmware analysis and hypotheses. No vendor firmware, executable root payload, flash dump, credentials, private network addresses, or footage is distributed.

## Results through 2026-10-28

| Question | Finding |
| --- | --- |
| Is 2560×1440 native? | **Cels:** no. Live VI reported 1920×1080 capture enlarged to 2560×1440 destination; ISP called it `2M->4M`. After a custom-partition change, both capture and destination were 1920×1080. **Pagalms:** after a matching July 2023 firmware upgrade and the same single-entry scaling change, RTSP also produces 1920×1080. Its live VI/ISP and sensor have not been measured, so native *sensor* capture is not independently proved there. |
| Which sensor? | Cels is strongly identified as SmartSens SC223A by live ID 41, 1928×1088 active area, and the build-matched ISP probe. Pagalms' sensor is unverified. |
| Is 25 fps real? | Not in the tested mode. Cels ISP and actual stream stayed at 20 fps even when an exposed sensor-FPS setting was changed to 25; that test was rolled back. |
| H.264? | No working camera-side H.264 from tested API settings, Sofia patches, generic XM530 H.264 driver, or full-HD x264 software test. Some attempts advertised H.264 but sent **no frames**. H.265 works. |
| PCM audio? | No higher-quality camera-side PCM path demonstrated. Actual RTSP remained 8 kHz mono G.711 A-law after an LPCM request. |

Details: [imaging and sensor](docs/findings.md), [encoder/frame rate](docs/encoder-and-frame-rate.md), [H.264 and audio](docs/h264-investigation.md), [camera controls](docs/camera-controls.md), [root access and recovery](docs/root-access.md), [experiment log](docs/experiment-log.md), and [sources](REFERENCES.md).

## Last tested configuration

On 2026-10-28, both main RTSP streams produced 1920×1080 HEVC frames after the pagalms upgrade; pagalms' 800×448 HEVC substream also remained available. Its DVRIP encoder settings remained CBR 8192 kbit/s, quality 6, FPS 20, GOP 1 (main) and CBR 2048 kbit/s, quality 6, FPS 10, GOP 2 (sub). Frigate reported a live pagalms FFmpeg process and zero skipped detection frames at the check. Cels' last measured main settings were VBR 8192 kbit/s, quality 6, FPS 20, GOP 1; sub VBR 2048 kbit/s, quality 6, FPS 12, GOP 2. A 1080p output and a configured 20 fps do not independently prove the physical sensor mode or long-term frame stability.

Cels' temporary, server-source-restricted but **unauthenticated** root maintenance listener remained installed at last check with the owner's approval. It is not a secure permanent design. See [security](SECURITY.md).

The work concerns only devices we own/administer. Original prose is [MIT-licensed](LICENSE); that does not license vendor firmware or third-party code.
