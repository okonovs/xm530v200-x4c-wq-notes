# Imaging pipeline and sensor evidence

## Provenance

Both units identify as `XM530V200_X4C-WQ_8M`, device ID `000807A8`. Pagalms originally reported a 2023-05-30 build; cels reported 2023-07-13 14:55:21. After the 2026-10-28 upgrade, pagalms also reported 2023-07-13 14:55:21. Static analysis used archived 2022 `standard` and July 2023 `TB` firmware for the same board family, **not** a dump of the currently installed user partition. Runtime `/proc/umap` observations below came from cels. Identical board names and firmware do not prove identical sensors.

## Measured enlargement on cels

Initially both main RTSP streams emitted 2560×1440 HEVC. On cels, `/proc/umap/vi` reported 1920×1080 capture and 2560×1440 destination; `/proc/umap/isp` explicitly described a `2M->4M` enlargement. Its ISP active area was 1920×1088. Thus the 1440p encoded frame was **not** 1440p native capture. The interpolation method is unknown.

Archived `CustomConfig/Encode.json` contained:

```json
"CaptureSizes": {"4M": [2560, 1440, 1440]},
"Vi": {"CapWHXY": [1920, 1080, 0, 0], "ViX": [{"Channel": 0, "CapSize": "4M"}]}
```

Ghidra traced `CapWHXY` to the VI capture rectangle and `CaptureSizes` to the destination. In 2022 `Sofia`, the relevant functions are `FUN_00290fa4`, `FUN_0029109c`, `FUN_0028c530`, and the VI ioctl wrapper `FUN_00296cc8`; in July 2023 TB they are `FUN_002c7f60`, `FUN_002c8058`, `FUN_002c0260`, and `FUN_002cf2cc`. TB `vi.ko` `vi_set_chn_attr` compares capture and destination dimensions and calls `vpss_isp_set_attr` for the enlarged path. Its default `cut` parameter is 1. This static path matches the live VI/ISP evidence. The [public VI header](https://github.com/OpenIPC/openxiongmai/blob/30b8e3eb11fca81994321e1d7a1ad1fc01dece81/libraries/include/re_comm_vi.h) names `stCapRect` and `stDestSize`, but SDK comments do not override live driver behavior.

Changing **only** cels' custom-partition `CaptureSizes.4M` to `[1920,1080,1080]` made VI capture and destination both 1920×1080. `ffprobe` received real 1920×1080 HEVC main frames and the substream kept working. The `4M` UI label remained but no longer meant a 4-megapixel output. This removes the measured VI enlargement, **not** optical blur or all hidden ISP processing.

Pagalms' normal DVRIP capability mask offered only `4M` main (`ResolutionMask=0x00100000`); `1080P` requests through both simplified and detailed encoder objects returned `Ret=606`. A shell-only upgrade did not establish root on the older build. On 2026-10-28, we upgraded its camera filesystems to the hardware-matched stock July 2023 TB build, **excluding U-Boot and U-Boot environment**. Bootloader environment in the vendor package included placeholder network/MAC values, so preserving the existing one mattered. DVRIP login and both HEVC streams returned on the July build. We then applied a separate custom-partition-only package whose extracted tree differed from TB stock in just `CustomConfig/Encode.json`, changing `CaptureSizes.4M` to `[1920,1080,1080]`. `ffprobe` subsequently received 1920×1080 main and 800×448 sub HEVC frames. Network and encoder readbacks were retained.

This achieves the requested **native-sized 1080p output** on pagalms without the known 1080→1440 destination setting. It does **not** establish that its physical sensor, live VI capture rectangle, or ISP path is identical to cels; no pagalms root/ISP readout was available. A backup of its readable DVRIP configuration was kept privately on the recording server, but its original May 2023 raw custom partition was not backed up before the upgrade. The stock July TB custom image and a verified stock-custom restore package remain available for recovery while DVRIP works.

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
