---
name: large-ta-memory-goal
description: qemu_dev goal is large memory for OP-TEE TAs with minimal changes; TEEm paper fidelity does not matter
metadata:
  node_type: memory
  type: project
  originSessionId: 42bb9406-d017-440c-9424-da01cbad0dd8
  modified: 2026-11-12T23:03:16.896Z
---

scripts/qemu_dev: the goal is large memory in OP-TEE TAs, as minimal/non-invasive as possible. Faithfully reproducing the TEEm paper does NOT matter (user: "the paper wasn't that good anyways").

**Why:** user dislikes invasive changes (e.g. the PbMA syscall 71); TEEm was just a starting point.
**How to apply:** prefer the smallest change that gives large TA memory (e.g. plain .bss heap if it works) over paper-faithful mechanisms. Related: [[tee-secure-memory-must-be-enforced]].
