# Security and disclosure

This repository describes research on personally owned cameras. It does not grant permission to access anyone else's device. Its historic maintenance shell was unauthenticated, network-exposed, and deliberately removed. Do not leave an equivalent service running, particularly on a network reachable from guests or the Internet.

Do not attach passwords, RTSP URLs containing credentials, MAC addresses, cloud IDs, flash dumps, or private footage to a public issue or pull request. A flash backup can contain all of those. Reports about these notes can use GitHub issues; discuss a newly found exploitable defect privately with the affected maintainer/vendor before posting a working exploit.

The repository intentionally contains **no executable root-shell payload** or vendor firmware. If you adapt the research for your own device, keep a recoverable off-device backup and independently verify that any temporary access service is gone afterward.
