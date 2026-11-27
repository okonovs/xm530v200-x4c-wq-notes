# Upstream QEMU stream inspection — 2026-11-27

New disposable guest experiments produced files recognized by independent host ffprobe as HEVC with header dimensions2304x2592 and640x720. Both raw streams produce slice-header overread errors under independent ffmpeg decoding, and ffprobe supplies no successful decoded-frame count. The upstream40M zero-error baseline remains unreproduced. Header recognition alone is not a video pass.

Inspected artifact SHA256:

- stream0:08c3eabd814d9626a64528c4a972c8c3bb3d1217dd7dbb0a8ac6236a29854846
- stream1:77db4bf683f14a19abfe482747a354517f33f4b3073e67b493394491c31aa24f

Separate canonical40M boot evidence confirms loader MMZ88MiB starting0x82800000, then reaches xmv200-test run without a completed result in that bounded observation. A separate128MiB machine invocation failed initrd placement; that experiment did not reach userspace. These cases must not be conflated with the captured-stream provenance or taken as proof of a model defect.

Next: verify capture provenance and pinned-source/binary correspondence, then isolate the malformed stream or guest-run failure with one grounded test. No physical camera/router contact or flash change. Physical sensor-origin video remains unverified.
