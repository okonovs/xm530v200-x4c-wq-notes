# Upstream projects and sources

These are links and attribution, not vendored copies. “Used” means a tool or documented mechanism informed our own camera experiment; “related” means useful context, not a claim that it unlocked this camera.

| Project/source | Role in this investigation |
| --- | --- |
| [zhellmann/xm530](https://github.com/zhellmann/xm530) | **Used:** identified the XM530V200 `dvrHelper` microSD `sd_run.sh` hook and the `/mnt/custom/extapp.sh` boot hook. Its research made the temporary root-access experiment possible. |
| [OpenIPC/python-dvr](https://github.com/OpenIPC/python-dvr) | **Used:** DVRIP library for authenticated configuration reads/writes, encoder-capability testing, and firmware-upgrade/recovery interactions. Its generic telnet-unlock approach did **not** provide root on this firmware. |
| [OpenIPC/ipctool](https://github.com/OpenIPC/ipctool) | **Related diagnostic tool:** documents sensor-name discovery and flash backup. Suggested for the still-open physical-sensor question; not the mechanism that opened our root shell. |
| [OpenIPC/openxiongmai](https://github.com/OpenIPC/openxiongmai) | **Related SDK research:** background on open XM530 platform work; not a replacement firmware we installed on these units. |
| [Xiongmai X4C-WQ product information](https://manuals.plus/ae/1005007535812467) | Board-family marketing specifications, including claimed SC401AI sensor; not proof of the sensor actually fitted to our cameras. |
| [OpenIPC SC401AI database](https://openipc.org/cameras/sensors/sc401ai) | Stated SC401AI array dimensions for comparison with the reported stream size. |

We do not redistribute upstream source, binaries, manufacturer firmware, or camera dumps. Follow each upstream project's license and the manufacturer's terms when obtaining material yourself.
