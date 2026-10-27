# Security and disclosure

This repository describes research on personally owned cameras. It does not grant permission to access anyone else's device. The temporary cels maintenance shell was **still installed at the last check on 2026-10-25**, with the owner's approval. It restricts source IP but has no authentication; that is inadequate as a permanent security boundary. Never expose an equivalent listener to guests or the Internet. Remove it when experimentation ends or replace it only with tested authenticated access.

Do not attach passwords, RTSP URLs containing credentials, MAC addresses, cloud IDs, flash dumps, or private footage to a public issue or pull request. A flash backup can contain all of those. Reports about these notes can use GitHub issues; discuss a newly found exploitable defect privately with the affected maintainer/vendor before posting a working exploit.

The repository intentionally contains **no executable root-shell payload** or vendor firmware. If you adapt the research for your own device, keep a recoverable off-device backup, a power/serial recovery option, and independently verify the active service state after each change. Do not infer that a successful upgrade or reboot API response means the requested action took effect.
