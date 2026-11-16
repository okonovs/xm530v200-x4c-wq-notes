# Instructions for coding agents

Read `README.md`, `docs/openipc-port.md`, and `CONTRIBUTING.md` before taking a
camera-port task. This public repository is a reviewed entry point, not a dump
of the owner's local workspace. Do not assume local paths, private firmware,
credentials, or QEMU images are available to contributors.

## One run, one question

Write down one confirm-or-reject question, the exact source version or hash,
the planned offline command, and the observation that would answer it. Work
for a bounded period; stop if evidence is missing or two attempts repeat the
same failure. Do not fill a context window by redoing a settled calculation.
Leave a file or issue checkpoint that another agent can use without transcript
history: question, commands, raw evidence location, verified facts, inferences,
failed hypotheses, unresolved point, next single task, and device-contact
statement. Keep a correction adjacent to any superseded claim.

## Evidence and safety

- Use the [evidence gates](docs/openipc-port.md). Do not claim a working
  OpenIPC camera until a real H.265 stream is independently decoded.
- Match exact firmware and module variants before combining user-space and
  kernel ABI conclusions. Keep unknown structure fields unknown.
- Prefer static review and host-only tests. Do not run a mutating ioctl,
  flash, reboot, or contact a camera, Raspberry Pi, or LAN device from a
  public task. A device owner must separately authorize a specific procedure.
- Do not commit or upload vendor binaries, images, backups, secrets, private
  addresses, footage, or generated executables. Review the whole diff before
  proposing publication.
- If an input is private or unavailable, state the exact missing artifact and
  stop. Do not invent a reproduction claim.

This file guides contributors. The owner's local `CAMERA-PORT-WORKFLOW-20261114.md`
may contain additional instructions for active private runs; it is not a
public invitation to execute device tests.
