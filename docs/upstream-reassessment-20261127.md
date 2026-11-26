# Upstream reassessment — 2026-11-27

The project goal is to run upstream libxmv200 and OpenIPC on X4C-WQ. Independent reconstruction of implemented multimedia interfaces is retired unless an observed ABI mismatch requires it.

## Reproducible source baseline

All five requested repositories are cloned under the sibling `xm530v200-upstream` directory. Exact revisions are in `evidence/revisions.json` there. `openxiongmai-1d6c9eb` is a detached baseline checkout. `firmware-pr2939` is firmware merge `63d87fee`, with the single package VERSION deliberately changed to `1d6c9eb`. Important: PR prose names that pin, but the actual merge tree pins `a4686613de4309ab24d946147cbbd5bb442aa8c7`. Current firmware pins `14eb857e1b2645e4243c4fd2c61bc8890a5b3aa7`. These are distinct experiments; do not silently mix them. The baseline-to-current source diff spans 49 files in the inspected media/tool paths; newer results require separate revision records.

Sources: https://github.com/OpenIPC/openxiongmai/pull/73 and https://github.com/OpenIPC/firmware/pull/2939 . Upstream's reported hardware tests are QEMU-only, explicitly stated in PR2939.

## What supersedes our work

| Existing research gap | Upstream baseline implementation | Project decision |
|---|---|---|
| venc_info 40-byte frame request | libxmv200/venc.c VENC_GET_FRAME=0xc0284902; paired 0x40284903 release | Adopt upstream frame acquisition/release |
| 56-byte HEVC start payload / 52-byte completion | venc.c v3 engine: 0x40384805, 0xc0344802, 189 register pairs, 13 returned words | Retire independent CMD05 reconstruction absent measured mismatch |
| Encoder ring/reference handling and H.265 headers | venc.c v3 register map, ring offsets, parameter sets and slice header generation | Use upstream encoder; stop ad hoc completion-only model development |
| Sensor/ISP/VPSS initialization | libxmv200 board.c, sns.c, tuning registry, isp.c; open xm530v200_isp and xm530v200_vpss | Port layout and tuning selection rather than invent SDK |
| Kernel encoder/frame modules | drivers/xm_hevc and xm_v200_venc | Build supported open modules with matching kernel; evaluate vendor fallback separately |
| Capture and tests | tools/xmv200-test; xmv200-capture tuning extraction; xmv200-sns sensor extraction/registry tooling | Follow their intended roles; extraction tools are not proof of physical sensor identity |
| Emulated sensor/encoder/IRQ | current qemu-bench xm-sensor, xm-hevc and HEVC encoder model | Reproduce current upstream before changing QEMU |
| Boot support | OpenIPC/u-boot openipc-xm branch, XM530V200 profile | Emulate first; never infer X4C DDR/flash safety from X6C |
| Stock service alternative | Vittuu/icsee-localmod uses stock multimedia stack on another board family | Useful stock service/recovery reference; does not replace this pipeline or prove X4C compatibility |

Matching ioctl constants establish overlap, not complete binary ABI compatibility. Compare actual module/version and observed errors before diagnosing differences.

## Explicit evidence classes

Original findings retained: verified six-partition stock backup with two matching reads; stock RTSP measurements; live stock /proc sensor/ISP/VI observations; static vendor module findings and their corrections. Historical raw material stays private.
Superseded: independent SDK implementation queue, unresolved ioctl-interface claims already implemented upstream, old custom QEMU completion-model strategy. Preserve historical logs without repeating those tests.
Emulation verified locally before this reassessment: old candidate delivered IRQ77 and returned CMD02 error with zero bytes. This is not an upstream SDK pass. Upstream reports X6C 64M MMZ failure and 40M successful decode; local reproduction is pending.
Physically verified: stock camera ARMv7 hardware, stock sensor ID41, linear MIPI 1928x1088 active / 1920x1080 VI, stock HEVC and ~6.7fps in latest measurement. Replacement pipeline physical video remains unverified.

## Board compatibility matrix

| Property | Upstream X6C-WEQ baseline | Owned X4C-WQ evidence | Unresolved / required test |
|---|---|---|---|
| SoC | XM530V200 | live /proc/cpuinfo XM530V200, two ARMv7 CPUs | kernel/module build compatibility |
| Sensor identity | board.c two SC2336 | numeric stock ID41; SC223A hypothesis only | review upstream ID reads; SC223A/AP share 0xcb3e, distinguish register 0x801b |
| Sensor count | two sensors | one active stock VI channel observed | physical inspection/stock mapping; one channel does not prove one physical sensor |
| MIPI | second sensor uses receiver/index 1; board.c layout | linear MIPI, active 1928x1088 | lane count/order, receiver, polarity, reset and power pins |
| Sensor clock | upstream family clock setup/tunings | not physically verified | stock register evidence and oscillator measurement if available |
| Image mode | 2304x1296 sensor pair; streams 2304x1296 + 640x360, fps12 | VI1920x1080; nominal20, latest flow~6.7 | selected tuning and actual frame cadence |
| DRAM / MMZ | reported osmem64M failure, 40M pass | board total capacity not re-established in this audit | boot DDR evidence and Linux/MMZ map; do not copy 40M blindly |
| Flash | upstream U-Boot/firmware layout | six backed-up partitions verified | exact offsets/sizes/type and boot ROM recovery from private manifest |
| Ethernet | upstream platform support | stock network functional | PHY type/reset, interface wiring and link test |
| Wi-Fi | upstream atbm603x X6C profile | chipset not verified here | USB IDs/module/board evidence; avoid assuming same device |
| IR-cut / LEDs / motors / audio | board-specific profiles | GPIO polarity/pins unverified | inspect stock peripheral config and wiring, then individually reviewed tests |

## Recoverable physical validation plan

1. Finish baseline build and QEMU 64M/40M tests; save source/config/artifact hashes, console status, frame counts and independent ffprobe plus ffmpeg decode exits. QEMU images must be disposable; synthetic sensor frames never count as physical frames.
2. Re-establish authorized read-only transport and verify router cleanup. Recheck backup manifest privately, stock stream health, uptime, memory and current module/kernel versions. No new hardware initialization yet.
3. Review upstream sensor detection code against board evidence. Auto detection is not read-only: board.c sensor_clocks_on writes clocks, and some identification paths write streaming registers. Prepare exact proposed register/pin actions for owner approval before executing unreviewed initialization.
4. Prepare a RAM-only test bundle matching the stock kernel, or a reviewed RAM-boot kernel/initramfs with verified return-to-stock path. Review all init scripts for flash mounts/writes, environment saves and watchdog behavior. Establish serial access/recovery before stopping services or RAM boot. Do not save bootloader environment.
5. After explicit approval of the concrete hardware initialization sequence, stop stock multimedia in controlled order, start the confirmed upstream layout, capture a bounded elementary stream to RAM, copy privately, independently decode and inspect sensor-origin evidence. Restore stock app or reboot into unchanged flash, confirm stream/network health and router forwarding=no.
6. Accept first physical milestone only when sensor-origin H.265 decodes with the upstream replacement pipeline, with exact module/config hashes and frame dimensions/counts. No flash deployment is part of this milestone.

## Contribution preparation

Keep X4C layout non-executable until sensor count, chip, I2C, clocks, MIPI, GPIO and memory are established. No guessed board.c entry will be submitted. Prepare a board evidence worksheet now, then a minimal layout/config patch plus sanitized reproduction commands and decode results. Submit no footage, credentials, addresses, dumps or private mappings. Owner review precedes upstream submission.

## Research iteration rule

Each iteration records input revisions, one hypothesis, exact command/config, actual result, hashes and resulting decision. Do not repeat failed old experiments without a changed input or new evidence. Next execution work: finish official baseline build, build current qemu-bench, reproduce both MMZ cases and decode captured streams. Until then status is BUILD/REPRODUCTION PENDING, never PASS.
