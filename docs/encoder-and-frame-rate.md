# Encoder quality and frame-rate experiments

## Last short stream tests (2026-10-25)

| Stream | Actual RTSP output | Frame observation | Configured encoder |
| --- | --- | --- | --- |
| cels main | HEVC 1920×1080 | 100 frames in a five-second `ffprobe` interval; live ISP/VI 20 fps | VBR 8192 kbit/s, quality 6, FPS 20, GOP 1 (20-frame interval) |
| cels sub | HEVC 800×448 | 60 frames in five seconds | VBR 2048 kbit/s, quality 6, FPS 12, GOP 2 |
| pagalms main | HEVC 2560×1440 | 88 frames in one five-second sample; nominal `r_frame_rate=20/1`, possible dropped frames | CBR 8192 kbit/s, quality 6, FPS 20, GOP 1 |
| pagalms sub | HEVC 800×448 | 49 frames in five seconds | CBR 2048 kbit/s, quality 6, FPS 10, GOP 2 |

These are short probes, not long-term drop statistics. RTSP metadata also reported `avg_frame_rate=25/1` after the FPS setting was corrected, so that field is not a count of delivered or distinct frames. Both substreams were retained; Frigate roles were not changed by these camera-side experiments.

On 2026-10-28, following the pagalms stock-TB and custom-scaling upgrades, a new short `ffprobe` run received **38 1920×1080 HEVC main frames in a two-second interval** and **14 800×448 HEVC sub frames** in another two-second interval. The configured main and sub encoder values in the table remained unchanged. Frigate reported an active pagalms FFmpeg process; this is not a long-term frame-rate or physical-sensor proof.

## Why 25 fps was rolled back

The archived July 2023 TB `Encode.json` declared `Vi.SensorFPS_PAL=20` and `SensorFPS_NTSC=20`. On cels, `/proc/umap/isp` reported 20.0 fps and VI delivered 20 fps. A custom-partition trial changed **only** these two JSON values to 25, with the rest of the tree diffed against the working image before flash. After reboot, configuration readback, VI requested rate, and H26X `frameRate` said 25, **but ISP still reported 20.0 and real RTSP video remained 20 fps**. We restored the known-good custom image. The setting does not change the sensor timing/PLL by itself.

The SC223A sensor datasheet specifies up to 1080p30, but this does not prove this board/clock/firmware can run it at 25. A genuine change needs a verified sensor mode and actual distinct frame/ISP timing measurements. We set the exposed main FPS to the observed 20 rather than leave a false 25 label.

## Quality, rate control, keyframes

Cels initially used VBR 8192 kbit/s, quality 3, GOP 2. Raising main quality to 6 persisted and changed live H26X `QPMax` from 45 to 40 while `QPMin` stayed 30: direct evidence of less aggressive worst-case quantization. GOP 1 changed live `GOPLen` from 40 to 20 at the true 20 fps. A configured main bitrate of 16384 kbit/s persisted through reboot but the live driver still reported 8388608 bit/s; the misleading setting was reverted to 8192. The true main-rate ceiling is unresolved.

Cels' substream quality rose from 3 to 6 and bitrate from 1280 to 2048 kbit/s; live H26X then showed `QPMax=40`, 2097152 bit/s, 12 fps, 24-frame GOP. Pagalms' main was already quality 6, CBR 8192, GOP 1; its sub bitrate rose from 817 to 2048 kbit/s. Because pagalms lacked root, its live driver QP/rate was not read; its DVRIP readback and actual RTSP were checked.

Higher configuration values are not necessarily higher *delivered* bitrate or visible quality. A shorter GOP costs compression efficiency but gives more frequent recovery frames. Long recordings and controlled daylight/night samples are needed to judge visual benefit. The lower-resolution substreams are intentional and do not change the main native-size goal.
