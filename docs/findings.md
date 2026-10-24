# Hardware, imaging, video, and audio findings

## Tested family

Two Xiongmai/XMEye cameras identify as `XM530V200_X4C-WQ_8M`, device ID `000807A8`. The observed installed firmware was `V5.00.R02.000807A8.10010.342917.0000010` (2023-05-30 build). One camera underwent the root/encoder experiment; the other was used as a comparison. Board-family identity does not prove that their physical image sensors are identical.

## Observed output versus native capture

On 2026-10-24, `ffprobe` reported H.265/HEVC video at 2560×1440, nominal 25 fps, on both main RTSP streams. Earlier observations found an 800×448 H.265 substream and 8 kHz mono G.711 A-law audio. This is the output format, not a sensor specification or a count of unique exposures per second.

An archived **2022 vendor image for the same X4C-WQ board family** has these encoder-profile values: `CaptureSizes.4M = [2560, 1440, 1440]`, `Vi.CapWHXY = [1920, 1080, 0, 0]`, and `ViX[0].CapSize = 4M`. The later `_TB` variant has the same dimensions. This mismatch makes internal 1080p-to-1440p scaling plausible, but the role of `CapWHXY` and the live firmware's sensor readout have not been verified. It would be incorrect to call the camera “native 1080p” or “fake 4 MP” on this evidence alone.

The X4C-WQ product listing associates the board with a SmartSens SC401AI, a sensor whose stated array is 2568×1448. That is **family-level evidence only**; we have not read the actual sensor ID from these two units. Strong sharpening, denoising, compression, optics/focus, and sensor-mode selection can all reduce *effective* detail despite a genuine 4 MP sensor.

### Decisive next measurements

1. From a securely obtained root session, read the loaded sensor driver/boot log and use `ipctool --sensor-name` where supported.
2. Inspect the active VI/ISP input dimensions and scaling stages, rather than only `Encode.json` defaults.
3. Photograph a known resolution chart in good light and compare resolved line pairs in the original stream against 1080p and 720p down/upscaled versions. Noise or sharpening alone must not be counted as scene detail.

## H.264 experiment

The ordinary DVRIP API advertised H.265 only and reverted an H.264 request. The SD/config experiment changed the real profile sufficiently for Sofia to attempt H.264 channel creation at 1920×1080. Sofia logged `x264_param_default_preset is not supported` followed by encoder-channel errors. The stock encoder implementation therefore did not produce a usable H.264 stream in this test. We restored the exact-board H.265 profile and verified the RTSP output, rather than trusting the configuration label.

## PCM experiment

ONVIF accepted a request for LPCM but immediately reported the old G.711 profile; the actual RTSP stream remained G.711 A-law at 8 kHz. We have not demonstrated a higher-quality camera-side PCM capture path. Transcoding the existing audio to PCM elsewhere would not restore lost microphone bandwidth or detail.
