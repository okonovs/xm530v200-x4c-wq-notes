# XM530V200 X4C-WQ: OpenIPC port and camera research

This project is working toward a reproducible [OpenIPC](https://openipc.org/)
port for the Xiongmai XM530V200 X4C-WQ camera. The target is a camera that boots
OpenIPC and produces a **real, independently decodable H.265 stream**. It is a
work in progress, and no working OpenIPC video stream has been demonstrated.

## OpenIPC port: current state

Last reviewed: **2026-11-19**. These are separate evidence gates, not a
percentage of completion.

| Gate | State | What the evidence supports |
| --- | --- | --- |
| Build | Verified locally | A maintained media probe builds and installs through a Buildroot package. A complete distributable firmware image is not established. |
| Offline boot | Verified locally | The XM530V200 QEMU setup reaches a login prompt. This does not emulate a working image sensor or video encoder. |
| Modules and device access | Partial | All 18 Xiongmai modules loaded in a local QEMU smoke; the source-built probe opens `/dev/sys`, `/dev/vpss`, `/dev/h265`, and `/dev/venc_info` in safe mode, without issuing ioctls. |
| Encoder ABI | In progress | Saved-flash `Sofia` and `hevc.ko` match the analyzed variant. A bounded tracer now runs on the stock camera, but encoder payload fields remain unmapped. |
| Stock sensor and frame timing | Partial | Live readback reports numeric sensor ID 41, linear MIPI and 1928×1088 active area. A long stock-stream sample averaged 6.68 fps after an earlier near-20 fps sample; cause and chip model are unknown. |
| Video | Unverified | No real H.265 elementary stream has been produced and independently decoded from this OpenIPC port. |
| Physical deployment | OpenIPC not tested | Owner-authorized RAM-only tools have run on the stock camera after a verified read-only flash backup. No OpenIPC image has been booted on that camera. |

Read the [port status and evidence guide](docs/openipc-port.md) before using an
older experiment. The dated research below describes the *vendor firmware*;
its working H.265 streams do not prove that the OpenIPC port streams video.

## Help move the port forward

Start with [CONTRIBUTING.md](CONTRIBUTING.md). It has small tasks, an evidence
format, and instructions for submitting results without posting vendor binaries,
camera backups, credentials, or private network details. Agents should also read
[AGENTS.md](AGENTS.md). [Open an issue](https://github.com/okonovs/xm530v200-x4c-wq-notes/issues/new/choose)
for a bounded question or a reproducible result. A contributor can help review
code and evidence without owning this camera; hardware tests require a device
they own or are authorized to use.

## Earlier vendor-firmware research

Field notes and a **structural reverse-engineering account** of the Xiongmai/
XMEye `XM530V200_X4C-WQ_8M` firmware image (firmware ID `000807A8`, software
`V5.00.R02.000807A8.10010.342917`, build 2023-07-13), from two personally owned
cameras the owner calls **cels** and **pagalms**.

> Observations are **dated, not a live status page.** We separate *observed
> facts* (binary layout, live non-destructive readback) from *inferred behavior*
> and *honest residuals* (method limits — not claimed vulnerabilities). No vendor
> firmware, executable root payload, flash dump, credentials, private network
> addresses, or footage is distributed. The work concerns only devices we
> own/administer.

## Table of contents

- [What this is](#what-this-is)
- [Headline results](#headline-results)
- [Firmware map at a glance](#firmware-map-at-a-glance)
- [Security findings](#security-findings)
- [The S2 upgrade chain (weakness, not an exploit)](#the-s2-upgrade-chain-weakness-not-an-exploit)
- [Codec finding (live, authoritative)](#codec-finding-live-authoritative)
- [Imaging & sensor](#imaging--sensor)
- [Honest residuals](#honest-residuals)
- [Reading guide](#reading-guide)
- [Method, safety, licensing](#method-safety--licensing)

## What this is

A porting and research effort around the X4C-WQ image and physical hardware:

1. **OpenIPC porting** — build and boot a replacement image, identify encoder
   and sensor interfaces, and ultimately verify a real stream.
2. **Live device evidence** — owner-authorized readback and temporary RAM-only
   tools on the stock camera, with private backups and bounded experiments.
3. **Static reverse-engineering** of the archived firmware — a complete per-file
   structural map plus the security finding set. The two main user-space
   executables, `Sofia` (4.95 MB) and `isp` (522 KB), are **fully symbol-
   stripped**, so their behavior is derived from section headers, PC-relative
   literal pools, and call graphs — not symbol tables. `libdvr.so`, by contrast,
   keeps its full `.dynsym` (424 exported functions) and is readable at the
   symbol level.

The authoritative structural account is the
[**2026-11-09 full analysis**](docs/firmware-analysis-2026-11-09.md) (whole-image
map + `libdvr.so` deep-dive). Earlier dated pages
([2026-10-29](docs/firmware-analysis-2026-10-29.md),
[2026-10-30](docs/firmware-analysis-2026-10-30.md)) document the codec-field
investigation and a documented **decoding-bug correction** (a PC-relative `LDR`
`+4`-vs-`+8` error that produced false negatives).

## Headline results

| Question | Finding |
| --- | --- |
| **Codec?** | **H.265 (HEVC)** on *both* streams — live `get_encode_info`: Main 4M@20 fps/8192 kbit/s, Extra D1@10 fps/2048 kbit/s, +8 kHz mono G.711 A-law audio. **No working camera-side H.264** (a menu/advertised H.264 sends *no frames*). |
| **Native 2560×1440?** | **Cels:** no — 1920×1080 capture enlarged to a 2560×1440 destination (`2M->4M`). **Pagalms:** 1920×1080 after the matching upgrade; sensor not independently measured. |
| **Sensor?** | Current cels readback reports numeric ID **41**, linear MIPI and a 1928×1088 active area. The chip model is **not yet verified** from the live device; the earlier SC223A attribution is a hypothesis. Pagalms' sensor is unverified. |
| **25 fps real?** | An earlier tested mode held about 20 fps despite a 25 fps request. A later long stock-stream sample averaged about **6.7 fps**. The change is unexplained, and neither proves sensor exposure cadence. |
| **PCM audio?** | No higher-quality camera-side PCM path — RTSP stayed 8 kHz mono G.711 A-law after an LPCM request. |
| **Backdoor ports?** | None. Only `80`/`8085` (web), `554` (RTSP), `34567` (DVRIP), `2323` (pre-existing NFS) — **no SSH/telnet/debug** (live, closed). |
| **Upgrade RCE?** | A real **code-path weakness** (disjoint `system()` sink + bypassed authority gate + weak blocklist), **but exploitability is blocked** on this build — not demonstrated. |

## Firmware map at a glance

The `TB` family is a multi-partition image. Boot chain:

```
u-boot.bin.img  ->  uImage.img (kernel)  ->  romfs-x.cramfs.img (root)
   ->  linuxrc  ->  loadmod  ->  loadpublic  ->  Sofia (main daemon)
```

| Partition / dir | Role |
| --- | --- |
| `u-boot.bin.img` / `u-boot.env.img` | Bootloader + environment (bootargs, partition table). |
| `uImage.img` | Kernel (U-Image wrapped). |
| `romfs-x.cramfs.img` | Read-only root filesystem (`bin`/`lib`/`sbin`/`usr`/`etc`). |
| `user-x.cramfs.img` | Read-only user overlay (web UI, music, zoneinfo). |
| `custom-x.cramfs.img` | Writable config overlay (`/mnt/custom`). |
| `TB-user/bin` | `Sofia` (control daemon), `isp` (ISP daemon), `dvrHelper`, `netinit`, `debug`, `iperf`, `armbenv`, `app.sh`, `Squirrel/` (NUT-script PTZ/RS485 runtime). |
| `TB-user/sbin` | WiFi/network stack: `hostapd`, `wpa_supplicant`, `wifiscan`, `wlandaemon`, `brctl`, `iw`/`iwpriv`, `iperf-arm`, `routeswitch`, `udhcpd.conf`. |
| `TB-user/lib` | `libdvr.so` (DVR/web backend), `libXMcrypto.so`, `libaf.so`, `libiw.so.29`, `libjson.so.0`, `libnl.so.1`, `modules.tar.lzma`. |
| `modules/` | `venc`/`vpss`/`vpss_isp`/`vo`/`vb`/`nn`/`bridge`/`ddr`/`mmz`/`kmotor`/`cfg80211`+`xm713g` — the XM530 media stack. |
| `TB-custom` | Config overlay: `FirmwareInfo` (build identity), `ProductDefinition`, `CustomConfig/`, `FactoryConfig/`. |

`FirmwareInfo` declares `CHIP_ID=XM530V200`, `DEVICE_ID=X4C-WQ`, `SMART_ID=IA_
PED` (neural-net person detection, the `nn.ko` block), `NET_PROTOCOL=NetIP,
SimpOnvif` (IP + simplified-ONVIF). See the
[full per-file account](docs/firmware-analysis-2026-11-09.md).

## Security findings

The private workspace tracks a ranked finding set. Headline results:

- **F-1 — owner maintenance root shell (custom partition).** An *approved,
  temporary* unauthenticated root maintenance listener on the custom partition.
  A maintenance hook, **not** a secure permanent design.
- **F-2 — upgrade admission has no cryptographic signature.** Firmware upgrade
  admission uses **structural checks only** (name, size, version, OEM binding).
  OEM MAC/SN binding and the `Mx8Q` integrity blob are present, but a well-
  formed image is not stopped by a signature gate. **Real weakness.**
- **F-3 — privileged `system(2)` sink behind a *bypassable* guard.** The
  `libdvr.so` `system_cmd` sink is an unguarded `vfork → execl → wait` wrapper;
  protection is a denylist on the command string plus the web-layer authority
  check, **not** a check *at* the sink.
- **F-4 — serial getty/login is *password-protected*, not passwordless.** (An
  earlier "passwordless serial" hypothesis was **retracted**.)
- **F-5 — open port surface, closed live.** Only `80`/`8085`/`554`/`34567`/
  `2323` (pre-existing NFS); **no SSH/telnet/debug backdoor.**

See [security](SECURITY.md) for the maintenance-listener posture.

## The S2 upgrade chain (weakness, not an exploit)

The `Sofia` `CUpgrader` upgrade path reaches a `system()`/`popen()` sink and
**bypasses the named authority gate** (the 76-entry `COperateSet` authority
check and the per-command `0x130cdc` sub-check, plus the web login-state gate).
The sink — `libdvr.so` `system_cmd` at `0xf8e4` — is literally:

```
bl  vfork@plt   ->  execl@plt(<command>)   ->  wait@plt
```

with **no authorization of its own** (the gate is upstream in the `XMJSO`/
`LibXmDvr_*` caller and the web-layer per-account `AuthorityList`). **But it is
not exploitable on this build:** owner-authorized non-destructive live testing
reached **100% "Uploading/Upgrading"** then returned **"Upgrade failed"** ×3 —
**no reboot, no shell, no persistent state.** The image is rejected by the
structural admission gate (§ F-2). So S2 is a **real code-path weakness, not a
demonstrated exploit.** The web layer *does* enforce authority — `getUsers`
returns a per-account `AuthorityList` (`SysUpgrade`, `EncodeConfig`, `Account`,
`NetConfig`, …) plus a `PasswordV2` hash. Details in
[firmware-analysis-2026-11-09.md, Appendix A](docs/firmware-analysis-2026-11-09.md).

## Codec finding (live, authoritative)

The per-channel codec select field is `0x53c340 + 0x7c` (in `.bss`). It is
**read** for the bit-0 codec select at `0x2c1380`; `0x2bf188` is a *conditional
bit-forcer* (forcing bits `0x0a` idempotently), **not** the codec-value setter.
The field's value is **config-driven** (`Media_OP_load_Encodecfg`), not a fixed
constant. **Live readback is authoritative: H.265 on both streams.** This closes
the roadmap #1 runtime-value question.

## Imaging & sensor

- **Cels** — earlier configuration showed 1920×1080 capture enlarged to a
  2560×1440 destination. Current readback reports numeric sensor ID 41,
  linear MIPI and a 1928×1088 active area; chip model remains unverified.
  An earlier tested mode held 20 fps after a 25 fps request, while a later
  long stock-stream sample averaged about 6.7 fps. The cause is unknown.
- **Pagalms** — 1920×1080 main + 800×448 substream HEVC after the matching
  upgrade; sensor unverified.

A 1080p output and a configured 20 fps do **not** independently prove the
physical sensor mode or long-term frame stability. Details in
[imaging & sensor](docs/findings.md),
[encoder/frame rate](docs/encoder-and-frame-rate.md), and
[H.264 and audio](docs/h264-investigation.md).

## Honest residuals

These are **method limits, not claimed vulnerabilities:**

- **Out-of-window base-register alias paths.** A single-pass static scan cannot
  exclude a writer that reaches a global through a base register computed outside
  the analyzed window — e.g. a kernel module or the `libdvr.so`
  `dvr_info_init`/`get_dvr_hwinfo` **dynamically-resolved vendor entry point**.
- **Physical silicon vs. tested config.** A 1080p/4M output and a configured 20
  fps do not independently prove the physical sensor mode.
- **`0x2bf188` is a bit-forcer, not the value setter** — a correction of two
  prior method errors (a PC-relative `LDR` `+4`-vs-`+8` decoding bug had produced
  false "no writer" results; now fixed).

## Reading guide

| Start here | For |
| --- | --- |
| [docs/firmware-analysis-2026-11-09.md](docs/firmware-analysis-2026-11-09.md) | **The whole picture** — full per-file map + `libdvr.so` deep-dive + S2 + codec. |
| [docs/research-roadmap.md](docs/research-roadmap.md) | The open questions and their evidence gates. |
| [docs/firmware-analysis-2026-10-30.md](docs/firmware-analysis-2026-10-30.md) | The codec-field investigation and the decoding-bug correction. |
| [docs/firmware-analysis-2026-10-29.md](docs/firmware-analysis-2026-10-29.md) | The first static-analysis pass and its limits. |
- [docs/firmware-analysis-2026-11-13.md](docs/firmware-analysis-2026-11-13.md) | **H.265 pipeline RE** — capstone ioctl scan (35 constants, 5 modules) + diagnostic probe. |
| [docs/firmware-analysis-2026-11-12.md](docs/firmware-analysis-2026-11-12.md) | OpenIPC firmware build for V200 (boot + net, no video). |
| [docs/findings.md](docs/findings.md) / [docs/encoder-and-frame-rate.md](docs/encoder-and-frame-rate.md) / [docs/h264-investigation.md](docs/h264-investigation.md) | Live imaging, sensor, encoder, audio. |
| [docs/camera-controls.md](docs/camera-controls.md) / [docs/root-access.md](docs/root-access.md) | Camera controls; root access & recovery. |
| [docs/experiment-log.md](docs/experiment-log.md) / [REFERENCES.md](REFERENCES.md) / [SECURITY.md](SECURITY.md) | The dated experiment log, sources, and the security posture. |

## Method, safety, licensing

- **Static verifiers** never contact the camera or network. Owner-authorized
  live work now includes readback and temporary RAM-only tools; flash and
  persistent camera storage remain untouched. No vendor binary, payload,
  backup, address, credential, or footage is published.
- `Sofia`/`isp` are fully stripped (0 symbols); their behavior is derived from
  section headers, PC-relative pools, and call graphs. `libdvr.so` is **not**
  stripped of exports (424 functions in `.dynsym`) and is symbol-readable.
- A documented method caution: ARM PC-relative `LDR` resolves at `PC+8+imm`;
  reading the adjacent pool literal (`+4`) caused false negatives across five
  verifiers (see the 2026-10-30 page). The correct VMA→file constant is per-
  binary (`libdvr.so` `.text` is `off == vma`; `Sofia` is `0x8000`).
- Last tested live configuration (2026-10-28) and the owner's temporary
  maintenance listener are described in the dated pages; see
  [SECURITY.md](SECURITY.md).

Original prose is [MIT-licensed](LICENSE); that does **not** license vendor
firmware or third-party code.
