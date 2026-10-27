# Root access, maintenance, and recovery

This describes personally owned hardware, **not** a generic unlock procedure. [zhellmann/xm530](https://github.com/zhellmann/xm530) identified the boot hooks. This repo intentionally contains no runnable payload or flash image.

## Mechanisms used

1. On the tested firmware, `/usr/bin/dvrHelper` mounts a FAT32 microSD at `/home` and executes `/home/sd_run.sh`. Our initial script copied six MTD partitions and writable config to the card, bind-mounted a test `Encode.json` in RAM, and launched temporary root access. This first stage did **not** write flash. Removing the card and power-cycling reverses RAM changes only.
2. Stock boot also executes `/mnt/custom/extapp.sh` from the `custom` CramFS partition before `Sofia`. A board-matched, custom-partition-only upgrade later started a tiny maintenance listener independent of Sofia. It wrote the 256-KiB **custom** partition, not `user` or `romfs`. Stock CramFS used about 192 KiB. A bad partition is harder to recover than an SD overlay.

The original temporary shell/FTP had no authentication. One later listener accepted only one useful session and had a 15-minute limit. A corrected `SO_REUSEADDR`/syscall variant accepted three independent UID/GID-0 shell sessions. That server-source-restricted **but unauthenticated** listener was still installed on **cels** at the last check (2026-10-25), with owner approval. It was not installed on pagalms. The source-IP check is not cryptographic authentication; same-LAN compromise/spoofing remains a risk. Exact addresses, code, credentials, and images are not published.

## Recovery evidence

Known-good native-1080p/maintenance and stock-custom restore packages were kept off-device. The known-good package SHA-256 was `b57aa70bd692e6f242a145645acaa2f5e52a9e999c2d48500d5a5179eb354b7c`. Its descriptor wrote only `custom`. After one physical power cycle, cels returned real 1920×1080 HEVC frames and DVRIP login. The corrected maintenance image then allowed repeated shells while video continued.

One software reboot command returned success yet did not restore video; physical power-cycling was needed. Risky RAM tests used a timed reboot guard, with actual uptime and RTSP video verified afterward. A backup of cels' live custom partition was extracted and diffed before the 25-fps test; when ISP stayed at 20 fps, we rolled back to the known-good custom image. No equivalent pagalms custom-partition backup was obtained; its flash was not changed. API acknowledgement, config readback, and RTSP SDP alone were repeatedly insufficient to establish a working camera.

## Routes that did not establish root

- Generic DVRIP `telnetctrl` did not work on this firmware generation.
- Pagalms `OPSystemUpgrade` packages containing only `Shell` commands returned an error or replied `Done` without opening the requested listener.
- Hostname command injection was considered but not confirmed as the route used.

## Better long-term design (not installed)

The current IP-filtered listener is a laboratory expedient. Persistent access should be key-authenticated, LAN-only, without Internet forwarding, with a stable host key and a verified removal/recovery path. Stripped ARM Dropbear might fit, but free space, writable key storage, BusyBox utilities, and reboot behavior were not established. SSH is **not** installed. Serial/physical recovery remains important for future flash work.
