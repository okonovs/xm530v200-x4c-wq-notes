# Upstream reproduction review — 2026-11-27

The built pinned SDK reaches diagnostic guest execution, but the reported X6C-WEQ40M successful decode has not been reproduced locally.

Independent raw-log review found:

-40M diagnostic: kernel cmdline contains mem=40M. The diagnostic reports `bpu_offline_deal` on channel1. No decoded output is established. Log SHA256 `21d15a92b226b5eb98bac61b1571c1934aff1a76d8be223cdbcfe5c2d863b46c`.
- Earlier40M probe log reports MMZ88MiB starting0x82800000, consistent with reserving40MiB for Linux on128MiB emulated RAM.
- The earlier purported64M test has an empty kernel command line and MMZ128MiB at0x80000000, overlapping Linux RAM. Its overrun warnings cannot reproduce upstream's insufficient-MMZ64M case. Log SHA256 `40c51f6e12bbe1316e87f70abdb3a97134ab214fa9abcd07b9bad27215ee0689`.

The40M error does not establish a QEMU model defect or exclude other harness problems. The next test corrects memory configuration and checks actual runtime bounds before comparing64M/40M outcomes. A one-sensor control may isolate behavior but cannot count as an X6C dual-sensor pass.

No physical camera/router contact, initialization or flash change. Sensor identity on owned X4C-WQ remains unverified. Physical upstream video and the local upstream QEMU decode baseline remain pending. Build checkpoint9836c48 remains valid.
