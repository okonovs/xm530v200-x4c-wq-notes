# Upstream baseline build checkpoint — 2026-11-27

The official XM530V200 lite firmware build completed with EXIT=0. Independent review found the actual build-volume package VERSION set to `1d6c9eb`, its matching package directory, and `.stamp_built`. This is the requested explicitly pinned experiment, distinct from firmware PR2939 merge's original package pin.

Firmware log reports kernel1175KiB/2048KiB and rootfs2656KiB/5120KiB. Exported artifact SHA256:

- uImage.xm530v200: `31d351a1e67151e5c36bb6e85d29585ff7fa5938fbf1adf62bb8f22aac55892f`
- rootfs.squashfs.xm530v200: `2c89b4c979d28ce25103fd4e59d450dc1d3f63a267fb7e8ced45ca1dd49404ca`
- OpenIPC U-Boot XM530V200 NOR: `43dd60b1f97ff1b84551755208cf84f6d9b431a1b32149a0dbec88b5962dc5e1`

The U-Boot build log also reports EXIT=0. The current upstream QEMU build executable runs, reports version10.2.0, and lists the xm530v200 machine. This confirms build prerequisites only. Neither a boot result nor H.265 decode is claimed here.

Next: bounded disposable X6C-WEQ guest tests with osmem64M and40M, record MMZ and actual xmv200-test outputs, then independent ffprobe/ffmpeg decode. Both reproduction cases remain pending. No physical camera/router contact or flash change in this checkpoint. Physical sensor identity and upstream sensor-origin video remain unresolved.
