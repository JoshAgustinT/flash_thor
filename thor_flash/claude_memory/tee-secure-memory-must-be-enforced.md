---
name: tee-secure-memory-must-be-enforced
description: "In TEE/OP-TEE work, \"secure\" memory must be hardware/emulator-enforced, never just hidden from Linux via DT; ask before any security trade-off"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 65165388-b333-42fb-bdf5-c73bb253c0ed
  modified: 2026-11-07T23:12:46.861Z
---

Secure-world memory must really be secure-only: enforced by TZASC on hardware, or on QEMU
by a region that exists only in the Secure address space (e.g. extra RAM added to
secure_sysmem in hw/arm/virt.c). Carving TA RAM out of normal DRAM and hiding it from
Linux with a reserved-memory DT node is NOT acceptable.

**Why:** the user rejected the TEEm-on-QEMU first version for exactly this — normal-world
`devmem` could read TA memory, which defeats the TEE.

**How to apply:** treat any isolation/security weakening as a blocking decision for the
user, not a caveat to mention at the end; add a negative isolation test (normal world
read must fault). Related: [[ask-before-rerunning-collectors]], [[do-only-what-was-asked]].
