# Contributing to the XM530V200 OpenIPC port

You can help without being a programmer or owning this exact camera. The best
contribution is one small question answered with evidence another person can
check.

## Pick one bounded task

- Review an [open issue](https://github.com/okonovs/xm530v200-x4c-wq-notes/issues)
  labeled `help wanted`, or open a new issue with one question and a stopping
  point. Check [port status](docs/openipc-port.md) first.
- For documentation, identify one stale claim or broken link and propose its
  exact replacement.
- For code, name the behavior and a host-only or offline test that would prove
  it. `media-probe/` on the public branch is an earlier experimental version;
  label findings with the exact revision. The maintained Buildroot package
  source should be reviewed and published before contributors modify it here.
- For device owners, describe the exact board and firmware *version* and what
  you observed. A different module or userspace binary may have a different ABI.

## Report evidence

Use this compact format in an issue or pull request:

```text
Question:
Hardware / firmware variant (if relevant):
Source revision and file SHA-256 (if relevant):
Command or method (redact private paths and identifiers):
Observed output (short excerpt or safe artifact link):
Conclusion: verified / inferred / unresolved
What would disprove it:
Physical camera or LAN contacted: yes / no; if yes, describe authorization and actions
Next bounded question:
```

Attach a minimal, sanitized log where possible. State what was *actually run*.
Do not turn a successful build into a claim of boot, or a QEMU device open into
a claim of encoded video. If a result corrects an older page, link both.

## Keep public submissions safe and usable

- Do not upload vendor firmware, kernel modules, proprietary executables,
  device backups, footage, credentials, tokens, serial numbers, MAC addresses,
  cloud IDs, private IP addresses, or RTSP URLs with credentials. A hash and
  version identifier can usually support a provenance discussion.
- Do not add prebuilt binaries or generated images. Link source and a build
  command. Check the actual diff, including binary files and logs, before a
  commit or pull request.
- Work offline when possible. Do not contact another person's camera or LAN.
  Any physical test needs the device owner's authorization, a recovery plan,
  and a narrow procedure. Do not run speculative mutating ioctls.
- Follow [SECURITY.md](SECURITY.md) for sensitive findings. Public issues are
  for reproducible engineering results, not credentials or exploit payloads.

## Pull request checklist

1. State one question and the result in the description.
2. Link exact source versions and safe evidence; distinguish observation from
   inference.
3. Update [docs/openipc-port.md](docs/openipc-port.md) only if a gate changed.
4. Correct or mark older conflicting claims rather than adding another
   unqualified status statement.
5. Confirm the diff contains no private or vendor-owned artifacts.

The public repository currently lacks some local build inputs. Say so when a
step cannot be independently reproduced; a precise missing prerequisite is a
useful contribution.
