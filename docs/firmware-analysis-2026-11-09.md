# Full structural reverse-engineering of the X4C-WQ firmware, 2026-11-09

This page is a complete, per-file structural account of the archived Xiongmai/
XMEye `XM530V200_X4C-WQ_8M` firmware image (firmware ID `000807A8`, software
`V5.00.R02.000807A8.10010.342917`, build 2023-07-13). It is synthesized from the
private research workspace, the 45 analysis passes plus the 2026-11-09 closure
passes, and live non-destructive device readback. It distinguishes *observed
facts* (binary layout, live readback) from *inferred behavior* and *honest
residuals* (method limits, not claimed vulnerabilities). No vendor binary,
executable payload, backup, device address, credential, or footage is reproduced
here; only structure, offsets, and behavior are described.

All offsets are ELF-relative unless stated otherwise. The two main user-space
executables (`Sofia`, `isp`) are **fully symbol-stripped** (`nm` finds 0 `T`/`D`/
`B` symbols), so their behavior is derived from section headers, PC-relative
literals, and call graphs, not from symbol tables.

## 1. Image layout and boot chain

The `TB` (and parallel `standard`) family is a multi-partition image. The
`*-custom` partitions are the writable/config overlays; the `*-romfs` /
`-inner` / `-user` partitions are the read-only root filesystem and kernel.

| Partition / file | Role |
| --- | --- |
| `u-boot.bin.img` | U-Boot bootloader. |
| `u-boot.env.img` | U-Boot environment (bootargs, partition table). |
| `uImage.img` | Kernel image (U-Image wrapped). |
| `romfs-x.cramfs.img` | Read-only cramfs root filesystem (bin/lib/sbin/usr/etc). |
| `user-x.cramfs.img` | Read-only user overlay (web UI, music, zoneinfo). |
| `custom-x.cramfs.img` | Writable custom-config overlay (installed to `/mnt/custom`). |
| `InstallDesc` | Install/flash descriptor for the upgrade tool. |
| `000807A8-TB.zip` / `000807A8-standard.zip` | Top-level package zips. |
| `AVEnc-live.b32` | Live encoder parameter blob. |
| `xm530_bind_shell.S` | (Owner artifact) a bind-shell source; not part of the vendor image. |

The boot sequence is `u-boot → kernel → romfs root → linuxrc` (the `standard-
romfs/linuxrc` init) which unpacks `modules.tar.lzma` into `/var/modules`, runs
`loadxm530 -i`, then sources `/usr/etc/loadpublic` (the main init script), then
removes the extracted modules. `loadmod` drives this; `loadpublic` does the
real work.

## 2. `TB-user` — the user-space root

`TB-user` is the primary read-only user root. Structure:

```
TB-user/
  bin/     Sofia, isp, dvrHelper, netinit, debug, iperf, armbenv, app.sh,
           pd_s_d1_xmdpu1_0.dpu, Squirrel/{ptz,rs485}
  sbin/    WiFi + network stack: hostapd, wpa_supplicant, wifiscan,
           wlandaemon, brctl, iw/iwpriv/WifiTest_iw, iperf-arm, routeswitch,
           udhcpd.conf, udhcpc.script, wpa_*.conf, rtl_hostapd.conf
  lib/     libXMcrypto.so, libaf.so, libdvr.so, libiw.so.29,
           libjson.so.0, libnl.so.1, modules.tar.lzma, modules/
  etc/     loadmod, loadpublic
  web/     index.htm, css/, js/ (web UI + client crypto), pluginVersion.js
  share/   music/ (80+ alarm/notification PCM in 6 languages), zoneinfo/
```

### 2.1 `bin/` executables

| File | Size | What it is |
| --- | --- | --- |
| `Sofia` | 4.95 MB | **The main application.** The DVR/IPC control daemon: web/RTSP/DVRIP server, encoder config, motion/PTZ/RS485, upgrade handler, the privileged `system()`/`popen()` sink. Fully stripped. |
| `isp` | 522 KB | **Image-signal-processing daemon.** Loads the sensor profile, drives the ISP pipeline (denoise, defog, sharpen, scaling `2M->4M`), talks to `isp.ko`. Fully stripped. |
| `pd_s_d1_xmdpu1_0.dpu` | 450 KB | Vendor DSP/ISP microcode (`.dpu`) for the `XMDPU1` block. |
| `dvrHelper` / `netinit` | 18 KB / 18 KB | Same size/`nm` fingerprint — the DVR helper and network-init front-end (identical build, different entry role). |
| `debug` | 131 KB | Debug/diagnostic utility. |
| `iperf` | 73 KB | TCP/UDP bandwidth tool. |
| `armbenv` | 5.6 KB | Reads/sets U-Boot environment (`env` access). |
| `app.sh` | 0.3 KB | Boot entry: `exec /usr/bin/Sofia`, then writes `maintain`/`RebootAbnormal` markers and halts-logging to `/mnt/mtd`. |
| `Squirrel/ptz/` | — | PTZ control: `MainPtz`, `PtzBase`, `PELCOP.nut`/`PELCOD.nut` (NUT script engine: PELCO-P/D protocol). |
| `Squirrel/rs485/` | — | RS485 serial control: `MainRS485`, `RS485Base`, `TelPubBase`, plus NUT scripts `GPS.nut`, `InteractCmd.nut`, `Transparent.nut`, `TransparentBase` (GPS polling, transparent serial passthrough). |

`Squirrel` is a **NUT-script runtime** (a small bytecode interpreter, like Lua)
used for PTZ and RS485 co-processor logic — the `.nut` files are the scripts.

### 2.2 `sbin/` — network + WiFi

| File | Role |
| --- | --- |
| `hostapd` / `wpa_supplicant` | 802.11 AP + supplicant (the camera can host an AP via `XM713G`). |
| `wifiscan` | WiFi scan utility. |
| `wlandaemon` | WiFi daemon (start/stop the `XM713G` stack). |
| `brctl` | Bridge control. |
| `iw` / `iwpriv` / `WifiTest_iw` | `iw` (wireless cfg80211) + vendor private ioctls + test. |
| `iperf-arm` | ARM build of iperf. |
| `routeswitch` | Route/UPnP/DNAT switch. |
| `udhcpd.conf` / `udhcpc.script` | DHCP server + client scripts. |
| `wpa_none.conf` / `wpa_wpa2.conf` / `rtl_hostapd.conf` | AP profiles (open / WPA2 / Realtek hostapd). |

### 2.3 `lib/` — shared libraries

| File | Role |
| --- | --- |
| `libdvr.so` | **DVR/web backend.** The web/CGI dispatcher, the `system`/`popen`/`system_cmd` sink, and `LibXmDvr_System_getDeviceSecret` (device secret). 149 `+0x7c` stores / 187 reads in its `.text`; small (`.text` ≈ 0x1c9f0). |
| `libXMcrypto.so` | Vendor crypto (auth, hashing, `PasswordV2`). Contains conditional bit-forcing stores but **not** the codec-field writer. |
| `libjson.so.0` | JSON parser (used by the web API). |
| `libiw.so.29` | `iw` wireless library (conditional non-codec stores; not the codec writer). |
| `libaf.so` | Audio-format / audio-frontend library (G.711 A-law, PCM). |
| `libnl.so.1` | libnl (netlink) library. |
| `modules.tar.lzma` + `modules/` | Kernel module archive (see §3). |

All six `.so` carry `.dynsym` (the older "stripped" label was an `nm`/`.symtab`
artifact — they are not fully stripped). **No codec-family symbol is exported**
by any of them, and **zero `+0x7c` stores** target the main-binary codec struct
`0x53c340` from any `.so` `.text` — so no shared library writes that field.

### 2.4 `etc/` — init

| File | Role |
| --- | --- |
| `loadmod` | Unpacks `modules.tar.lzma` to `/var/modules`, runs `loadxm530 -i`, sources `loadpublic`, cleans up. |
| `loadpublic` | **Main init.** Loads SD/USB modules, selects the WiFi variant (`XM713G`/`XM712G`/`ATBM6062S`/`RTL8192FC`/`HI3861L`) and loads `wifi_pdn.ko`+`cfg80211.ko`+vendor wifi module, starts `wlandaemon`, starts `routeswitch`, and `insmod kmotor.ko` with the PTZ motor GPIO pins `166,11,13,9,8,35,36,33`. |

### 2.5 `web/` — web UI + client crypto

`index.htm` is the admin UI. `web/js/` ships the **client-side auth crypto**:
`rsa.js`, `jsbn.js`, `prng4.js`, `rng.js` (RSA + PRNG), `md5-min.js`, `base64.
js`, `jcookie.js`, plus `jquery`/`LAB.min.js`/`json.js`/`xmlhttp.js`.
`protocolcheck.js` gates the browser feature check; `pluginVersion.js` points at
the vendor ActiveX/download server (`xmsecu.com`) and reports `g_oemName`/
`g_WebStyle` (both blank → no OEM branding here).

### 2.6 `share/` — media

`share/music/` holds **80+ alarm/notification PCM files** in 6 languages
(English, Simplified Chinese, Spanish, Vietnamese, Portuguese, Chinese-English)
— the audio cues for `motiondetect`, `sirenlingyu` (siren), `xiaojiaojingbao`
(siren), `weixianquyu` (danger zone), etc. `share/zoneinfo/` is the timezone
database.

## 3. `modules.tar.lzma` — kernel modules

The module archive (`modules/`) is the imaging/media core of the SoC:

| Module | Role |
| --- | --- |
| `venc.ko` | **Video encoder** (H.264/H.265) — the hardware VENC. |
| `vpss.ko` / `vpss_isp.ko` | **Video pipeline / ISP** front-end. |
| `vo.ko` / `lcd.ko` | Video output / LCD. |
| `vb.ko` | Video buffer manager. |
| `nn.ko` | **Neural-network** accel (the `IA_PED` smart feature — person detection). |
| `i2c.ko` / `i2s.ko` | I2C (sensor) / I2S (audio). |
| `bridge.ko` | MIPI-CSI bridge. |
| `ddr.ko` / `mmz.ko` / `mmz` | DDR controller / memory-zone allocator. |
| `dwc2.ko` / `xm_usb0.ko` | USB (dwc2 host + vendor gadget). |
| `kmotor.ko` | PTZ motor driver (PWM on the GPIOs above). |
| `xm_wdt.ko` | Watchdog. |
| `adc.ko` | ADC. |
| `cfg80211.ko` / `wifi_pdn.ko` / `xm713g.ko` | WiFi (cfg80211 + power-domain + the `XM713G` module). |
| `sdio1_sd.ko` | SD/SDIO card. |
| `sys.ko` / `log.ko` / `var.ko` | System / logging / var. |
| `loadxm530*` | Per-variant module loader (32M/64M/128M/double-cam/ahd/LKP/lowpower). |
| `pll_db.sh` | PLL database. |

This is a **Xiongmai XM530** (SigmaStar/Ingenic-class) SoC: VENC + VPSS/ISP +
NN + bridge + DDR/MMZ is the classic embedded-IPC media stack.

## 4. `TB-custom` / `standard-custom` — config overlays

| File | Role |
| --- | --- |
| `FirmwareInfo` | **The build identity manifest** (see §5). |
| `ProductDefinition` | JSON: Vendor=General, Hardware=`XM530V200_X4C-WQ_8M`, 6 languages, PackSize 3072. |
| `CustomConfig/` | Writable per-device config (encoder, accounts, net, PTZ `motor.init`). |
| `FactoryConfig/` | Factory defaults. |
| `Dot/` | Hidden config dotfiles. |
| `data/` | Runtime data overlay. |

## 5. Build identity (`FirmwareInfo`)

```
PRODUCT_NAME=XM530V200_X4C-WQ_DSSRPSRPS_VIDEOCloudStorageEXETRACKDAS_IA_PED_...
CHIP_ID=XM530V200
DEVICE_ID=X4C-WQ
EXTENFUNC_PRE=DSS,RPS,RPS_VIDEO,CloudStorage,EXE,TRACK,DAS
SMART_ID=IA_PED                 (smart: person detection)
NET_PROTOCOL=NetIP,SimpOnvif    (IP + Simplified-ONVIF)
NET_UPGRADE_FILE=General_IPC_XM530V200_X4C-WQ_WIFIXM713G_TB.713g.Nat.dss.
                       OnvifS_V5.00.R02.20230713_all.bin
BURN_FILE=upall_..._OnvifS.20230713.bin
LOGNAME=zhaochanglong           (developer tag)
```

`IA_PED` = the `nn.ko` neural-network person-detection feature. `EXE`/`TRACK`/
`DAS` = the smart/behavior features. `SimpOnvif` = the simplified ONVIF profile.

## 6. Security findings (evidenced, not claims)

The private workspace tracks a ranked finding set. The headline results:

- **F-1 — owner maintenance root shell (custom partition).** The owner has an
  *approved, temporary* unauthenticated root maintenance listener on the custom
  partition. It is a maintenance hook, **not** a secure permanent design.
- **F-2 — update admission has no cryptographic signature.** Firmware upgrade
  admission uses **structural checks only** (name, size, version, OEM
  binding). There is **no cryptographic signature** — a real weakness. OEM MAC/
  SN binding and the `Mx8Q` integrity blob are present, but an attacker with a
  well-formed image is not stopped by a signature gate.
- **F-3 — privileged `system(2)` sink with a weak blocklist guard.** `Sofia`/
  `libdvr` reach `system`/`popen` through an upgrade path whose guard is a
  **bypassable blocklist** (a denylist of dangerous tokens), not a allowlist.
- **F-4 — serial getty/login is *password-protected*, not passwordless.**
  (An earlier "passwordless serial" hypothesis was **retracted**; the getty
  authenticates.)
- **F-5 — open port surface (live, closed).** Live port enumeration shows only
  `80`/`8085` (web), `554` (RTSP), `34567` (DVRIP), and `2323` (a pre-existing
  `3d-nfsd` NFS export). **No SSH, telnet, or debug backdoor** is exposed.

## 7. S2 — the upgrade RCE chain (exploitability blocked on this build)

The most-studied finding: `Sofia`'s `CUpgrader` upgrade path reaches a
`system()`/`popen()` sink and **bypasses the named per-command authority gate**
(the 76-entry `COperateSet` authority check and the per-command auth sub-check at
`0x130cdc`), plus the web login-state gate. The sink is **disjoint** from the
authority gate, so a crafted `Script`/upgrade payload can reach the shell path.

**However, exploitability is *blocked* on this build.** In owner-authorized,
non-destructive live testing the exploit reached **100% "Uploading/Upgrading"**
but then returned **"Upgrade failed"** three times — **no reboot, no shell, no
persistent state**. The image is rejected by the validity checks (the
structural-admission gate §6/F-2). So S2 is **a real code-path weakness
(disjoint sink + bypassed gate + weak blocklist) but not demonstrated
exploitable** on the X4C-WQ / 2023 build. The exploit scripts are
*not* run without fresh owner authorization, and no root was obtained.

**Web-auth model (resolved live, 2026-11-09).** The web layer **does** enforce
authority: `getUsers` returns a per-account `AuthorityList` (including
`SysUpgrade`, `EncodeConfig`, `Account`, `NetConfig`, …) plus a plaintext
password and a `PasswordV2` hash. So the *primary* privilege gate is the per-
account authority list at the web layer; the §7 bypass is a *secondary* gate.
The "Upgrade failed" result is consistent with the admission/validity checks
rejecting a crafted image even where the named gate is bypassed.

## 8. Codec finding (resolved live, authoritative)

The per-channel codec select field is `0x53c340 + 0x7c` (in `.bss`, zero-
initialized). It is **read** for the bit-0 codec select at `0x2c1380` and has a
conditional bit-forcer at `0x2bf188` (`ldr/tst/orrne/strne`, forcing bits
`0x0a` idempotently) — but that is a **bit-forcer, not the codec-value setter**.
The field's **runtime value is config-driven** (`Media_OP_load_Encodecfg`), not
a hardcoded constant.

**Live readback (2026-11-09) is authoritative:** `get_encode_info` returns
**H.265 (HEVC) on both streams** — Main 4M@20fps/8192 kbit/s and Extra
D1@10fps/2048 kbit/s, plus 8 kHz mono G.711 A-law audio. There is **no working
camera-side H.264** on this configuration (a menu/advertised H.264 sends no
frames); H.265 is what the device actually produces. This closes the roadmap
#1 runtime-value question.

## 9. Honest residuals (method limits, not open vulnerabilities)

- **Out-of-window base-register alias paths.** A single-pass static scan cannot
  exclude a writer that reaches the field through a base register computed
  outside the analyzed window (e.g. a kernel-module or unextracted vendor PLT
  provider). This is a **static-RE method limit**, not a claimed finding.
- **Physical silicon vs. tested config.** A 1080p/4M output and a configured
  20 fps do **not** independently prove the physical sensor mode or long-term
  frame stability. The sensor is strongly identified as **SmartSens SC223A** on
  the cels unit (live ID 41, 1928×1088 active area, build-matched ISP probe);
  the second unit's sensor is unverified.
- **`0x2bf188` is a bit-forcer, not the value setter** — a correction of two
  prior method errors (a PC-relative `LDR` `+4`-vs-`+8` decoding bug had
  produced false "no writer" results; now fixed).

## 10. Method notes

- `Sofia`/`isp` are **fully stripped** (0 symbols), so their behavior is derived
  from section headers, PC-relative literal pools, and call graphs (BFS over the
  reachable call set). The 2026-10-31 "PC-relative `LDR` `+4` vs `+8`" bug is a
  documented method caution: ARM PC-relative loads resolve at `PC+8+imm`, and
  reading the adjacent pool literal caused false negatives across five verifiers.
- **Static verifiers** never contact the camera or network. **Live probes** are
  strictly non-destructive readback (blank/owner password, hard timeouts, no
  flash, no config change, no reboot unless owner-authorized).
- All private artifacts (full disassembly, device backups, verifiers,
  `outputs/STATUS.md`) remain in the private workspace; this page is the
  reviewed prose account only.

*No camera, Frigate, LAN, or network contact is implied by this page. All live
results above were obtained under owner authorization and are dated 2026-11-09.*
