---
name: vanilla-optee-change-only-config
description: "For the qemu_vanilla OP-TEE experiment, change only what's allowed (build config); accept whatever limit results, don't ask to patch QEMU/TF-A"
metadata:
  node_type: memory
  type: feedback
  originSessionId: f53ee41d-a7da-4d45-827e-832aeebe5a1d
  modified: 2026-11-12T00:59:38.535Z
---

In scripts/qemu_vanilla, the point is to measure what vanilla OP-TEE gives when we change only what we're allowed to change (OP-TEE build config / carve-out settings). QEMU's board memory map is "hardware" and stays as-is, so we accept whatever limit that gives. Don't offer to patch QEMU or TF-A to get a bigger carve-out.

**Why:** I asked whether to patch QEMU's memory map to grow secure RAM. The user said "regardless we just want whatever the limit it gives us. the idea is we change what we are allowed to change", and asked for a note on how the limit was found and how it maps to a real board (where Linux is placed, how far the carve-out could extend).

**How to apply:** record the limits and where they come from in qemu_vanilla/progress.md. Don't treat them as problems to engineer around. Related: [[tee-secure-memory-must-be-enforced]], [[do-only-what-was-asked]].
