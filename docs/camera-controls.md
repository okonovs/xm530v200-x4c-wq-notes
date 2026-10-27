# Exposed camera controls and earlier image tests

This page preserves the earlier authenticated DVRIP/ONVIF observations. Some settings were measured before the later root/ISP investigation; where we did not reconfirm a final value, it is labeled accordingly. `OpenIPC/python-dvr` supplied the authenticated DVRIP interface. A successful `Ret=100` or setting readback is **not** proof of a sensor/stream change.

## Available configuration surfaces

| Interface/object | Observed fields and limits |
| --- | --- |
| `get_encode_capabilities()` | Main `ResolutionMask=0x00100000`, `CompressionMask=0x00000100` (HEVC only); secondary resolution mask `0x00000009`; reported `MaxBitrate=131072` and `MaxEncodePower=277585920` are generic capabilities, not measured stable operating limits. `AVEnc.EncodeAbility` returned `Ret=607`. |
| `Simplify.Encode` / `AVEnc.Encode` | Main/sub resolution, codec, configured FPS, bitrate, CBR/VBR, quality, GOP and virtual GOP. The active stream needs separate verification; changing an inactive detailed profile is not equivalent to changing the live channel. |
| `Camera` / `Camera.ParamEx` | Day/night noise-filter levels, `LowLuxMode`, image `Style`, gain/exposure-related controls, `Dis` and `Ldc`. Vendor-specific semantics are not fully known. |
| `AVEnc.VideoColor.[0]` | Time-based color profiles including `Acutance`, brightness, contrast, hue, saturation, gain and white balance; only the enabled section is active. |
| `AVEnc.VideoWidget`, `fVideo.OSDInfo` | Timestamp/channel-name/custom-text encode and preview overlays. |
| `snapshot()` | 800×448 JPEG from the configured low-resolution JPEG channel; not a native-sensor measurement. |

## What was tested

- Both cameras' time/channel overlays were disabled and read back as `EncodeBlend=false` and `PreviewBlend=false`; custom OSD text was empty on the tested camera. Observed privacy-cover count was zero, and covers were not deliberately changed.
- `Day_nfLevel` and `Night_nfLevel` began at 2, were tried at 1, then at 0. Later checks found 0 on both. The operator saw no convincing gain in fine detail. These sliders may not control the dominant ISP denoiser.
- `LowLuxMode` began at 1; 0 was tested on cels with little visible difference. Restoration of the old value was **not explicitly verified** in the early handoff. Do not present its current value as known without a new readback.
- Pagalms' enabled `Acutance` originally read 3848 (`0x0F08`); cels' enabled section read 0. An inactive section on both read 3848. A later live read/change set pagalms' *active* Acutance to 0 and verified readback. The image still had a processed look. The 16-bit value is not safely interpretable as a simple 0–15 sharpness slider.
- Typical color readbacks were brightness, contrast, hue, saturation 50; gain 0; white balance 128. `Dis=0`, `Ldc=0`, `PreventOverExpo=0`, and `SoftPhotosensitivecontrol=0` were observed. The initial styles differed (`type1` on pagalms, `type2` on cels); their effects were not compared under controlled identical scenes. No evidence supports a single objectively best style.
- A historical 20-second HEVC recording from pagalms contained 300 frames (~15 fps by that capture's duration) despite nominal 25-fps RTSP metadata. Missing/unset packet timestamps and network path could affect that estimate. Later short direct tests were closer to 20 fps. Neither test alone proves distinct sensor exposures.
- Downscaling one soft frame to 720p and re-enlarging it looked nearly unchanged to the operator. This is useful subjective evidence of poor effective detail, **not** proof of a 720p sensor: optics, focus, low-light/IR shift, denoising, and compression can give the same appearance. Use original main-stream daylight frames and a resolution chart for the next optical test.

For video tests, prefer copying the main RTSP video without re-encoding and probing the resulting bitstream. The built-in DVRIP JPEG snapshot is only 800×448 and should not be used to assess main-stream detail. Keep credentials out of shell history and logs.
