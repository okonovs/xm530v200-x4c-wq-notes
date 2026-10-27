# Upstream projects and sources

These are links and attribution, not vendored copies. “Used” means a tool or documented mechanism informed our own camera experiment; “related” means useful context, not a claim that it unlocked this camera.

| Project/source | Role in this investigation |
| --- | --- |
| [zhellmann/xm530](https://github.com/zhellmann/xm530) | **Used:** identified the XM530V200 `dvrHelper` microSD `sd_run.sh` hook and the `/mnt/custom/extapp.sh` boot hook. Its research made the temporary root-access experiment possible. |
| [OpenIPC/python-dvr](https://github.com/OpenIPC/python-dvr) | **Used:** DVRIP library for authenticated configuration reads/writes, encoder-capability testing, and firmware-upgrade/recovery interactions. Its generic telnet-unlock approach did **not** provide root on this firmware. |
| [OpenIPC/ipctool](https://github.com/OpenIPC/ipctool) | **Related diagnostic tool:** sensor discovery/flash backup references. Not the mechanism that opened our root shell. |
| [OpenIPC/openxiongmai](https://github.com/OpenIPC/openxiongmai) | **Used for static-analysis context:** VI structures, ISP sensor registration, MPI error codes, and the generic XM530 H.264 driver comparison. Its SDK-generation sensor enum does not map directly to the tested TB firmware. No OpenIPC replacement firmware was installed. |
| [SmartSens SC223A flyer](https://smartsens.oss-cn-beijing.aliyuncs.com/web/img/1624345889092341238.pdf) | Manufacturer pixel-array and maximum-video-mode specification used to cross-check the cels live sensor evidence. |
| [Xiongmai XM530V200WX1 product information](https://www.xmsilicon.com/productinfo/2145662.html) | Manufacturer family-level codec/rate-control specification; not proof that our exact assembled cameras implement H.264. |
| [Xiongmai X4C-WQ product information](https://manuals.plus/ae/1005007535812467) | Marketing claim of SC401AI for a board-family variant; contradicted for cels by stronger live/build-matched evidence. |
| [OpenIPC SC401AI database](https://openipc.org/cameras/sensors/sc401ai) | Alternative sensor's dimensions; archived firmware support does not prove installation. |
| [VideoLAN x264](https://code.videolan.org/videolan/x264) | Source of the real ARMv7/NEON software-encoder CLI used for a RAM-only feasibility test. |
| [OpenIPC firmware issue #1927](https://github.com/OpenIPC/firmware/issues/1927) | Related XM530V200 driver incompatibility report; context, not proof about these units. |
| [XM530V200 IPCamTalk discussion](https://ipcamtalk.com/threads/xm530-4mp-wifi-camera-h-265-h-264-possible-maybe-flash-fw.72173/) | Early lead on the exact board family and `Encode.json`; its claims were tested against archived firmware and live state rather than assumed true. |
| [XMeye.help firmware catalog](https://xmeye.help/firmware) | Catalog considered for stock/modified XMH firmware. No XMH image was flashed; alleged modifications and security benefits were not independently verified for our units. |

We do not redistribute upstream source, binaries, manufacturer firmware, or camera dumps. Follow each upstream project's license and the manufacturer's terms when obtaining material yourself.
