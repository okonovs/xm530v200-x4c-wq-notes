# Imaging pipeline and sensor evidence

## Provenance

Both units identify as `XM530V200_X4C-WQ_8M`, device ID `000807A8`. Pagalms reported a 2023-05-30 build; cels a 2023-07-13 14:55:21 build. Static analysis used archived 2022 `standard` and July 2023 `TB` firmware for the same board family, **not** a dump of the currently installed user partition. Runtime `/proc/umap` observations below came from cels. Identical board names do not prove identical sensors.

## Measured enlargement on cels

Initially both main RTSP streams emitted 2560×1440 HEVC. On cels, `/proc/umap/vi` reported 1920×1080 capture and 2560×1440 destination; `/proc/umap/isp` explicitly described a `2M->4M` enlargement. Its ISP active area was 1920×1088. Thus the 1440p encoded frame was **not** 1440p native capture. The interpolation method is unknown.

Archived `CustomConfig/Encode.json` contained:

```json
"CaptureSizes": {"4M": [2560, 1440, 1440]},
"Vi": {"CapWHXY": [1920, 1080, 0, 0], "ViX": [{"Channel": 0, "CapSize": "4M"}]}
```

Ghidra traced `CapWHXY` to the VI capture rectangle and `CaptureSizes` to the destination. In 2022 `Sofia`, the relevant functions are `FUN_00290fa4`, `FUN_0029109c`, `FUN_0028c530`, and the VI ioctl wrapper `FUN_00296cc8`; in July 2023 TB they are `FUN_002c7f60`, `FUN_002c8058`, `FUN_002c0260`, and `FUN_002cf2cc`. TB `vi.ko` `vi_set_chn_attr` compares capture and destination dimensions and calls `vpss_isp_set_attr` for the enlarged path. Its default `cut` parameter is 1. This static path matches the live VI/ISP evidence. The [public VI header](https://github.com/OpenIPC/openxiongmai/blob/30b8e3eb11fca81994321e1d7a1ad1fc01dece81/libraries/include/re_comm_vi.h) names `stCapRect` and `stDestSize`, but SDK comments do not override live driver behavior.

Changing **only** cels' custom-partition `CaptureSizes.4M` to `[1920,1080,1080]` made VI capture and destination both 1920×1080. `ffprobe` received real 1920×1080 HEVC main frames and the substream kept working. The `4M` UI label remained but no longer meant a 4-megapixel output. This removes the measured VI enlargement, **not** optical blur or all hidden ISP processing.

Pagalms did **not** receive that flash change. Its normal DVRIP capability mask offered only `4M` main (`ResolutionMask=0x00100000`); a `1080P` request returned `Ret=606` and readback stayed `4M`. A shell-only upgrade did not establish root on its older firmware. Its own custom-partition backup and live VI/sensor readout are needed before attempting a board-specific change; copying cels' image could overwrite calibration or other differences. Pagalms' internal enlargement is plausible by analogy, not proven.

## Sensor identity

Cels' running ISP reported sensor ID 41 and 1928×1088 input. In the build-matched archived TB `isp`, `FUN_0003ad80` reads registers `0x3107/0x3108`; chip ID `0xcb3e` with register `0x801b` zero prints `SC223A!` and assigns ID `0x29` (41). The alternative `0x801b` value selects SC223AP/ID `0x2e`. The [SmartSens SC223A flyer](https://smartsens.oss-cn-beijing.aliyuncs.com/web/img/1624345889092341238.pdf) lists a 1928×1088 pixel array and maximum 1920×1080 video mode. These matches strongly identify **cels** as SC223A; a direct live register read or installed-ISP hash would close the final provenance gap. Pagalms' live sensor ID remains unknown.

The archived firmware also has an SC401AI probe (`0xCD2E`) and a 2568×1448 mode. That proves firmware **support**, not the fitted hardware. The public OpenXiongmai sensor enum is from a different SDK generation and maps numeric IDs differently; its ID 41 must not be applied to this build's live ID.

## Image-processing limits

Exposed `Day_nfLevel` and `Night_nfLevel` were already 0 on both units. Cels' active `Acutance` was already 0; pagalms' was changed from 3848 to 0 and read back. Digital stabilization and lens-distortion correction were off. Vendor ISP code nevertheless has separate sharpen, spatial/temporal NR, BPU 3DNR, and VPSS 3DNR stages. Zero exposed sliders do **not** prove the whole pipeline is unprocessed. Focus, optics, light, demosaicing, quantization, and hidden ISP stages can all reduce effective detail. A daylight resolution chart using original encoded frames would be the next optical check.

## Archived binary SHA-256 values

| Binary | SHA-256 |
| --- | --- |
| 2022 standard `user/bin/Sofia` | `2ce99938d97c96eea59844cf1ba06bfc4aa8ba91d3f2a27d00a285d93208bf26` |
| 2022 standard `user/bin/isp` | `ffa5c1d89fbecb4f55b7996f2c83c13ab341fea34b420a18471a149eeb01ade5` |
| July 2023 TB `user/bin/Sofia` | `71137674dfbbeb456ec5d7239f09563022a36e30802c1815c536131afe634d5a` |
| July 2023 TB `user/bin/isp` | `9bb3a6604a36e8334b25f08ebc85e01157c07deaf9783cc1473ebc6ded093ca5` |
| July 2023 TB `vi.ko` | `6ae713ca3fd2c218de947d7577f8d19948c652edf1459db28dfa97799038b152` |
