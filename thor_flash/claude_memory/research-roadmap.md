---
name: research-roadmap
description: "User's PhD-scale roadmap: large TA memory on real HW -> TA threads/throughput -> GPU/peripherals in the TEE"
metadata:
  node_type: memory
  type: project
  originSessionId: 42bb9406-d017-440c-9424-da01cbad0dd8
  modified: 2026-11-13T01:18:42.992Z
---

Roadmap (stated 2026-11-12), likely a full PhD:
1. Large memory in OP-TEE TAs, then on real hardware (Jetson Thor) — current first step.
2. Threads and/or scheduler for high-throughput TAs. Goal: run e.g. a ~270 GB/s app in the secure world at ~265-270 GB/s, not toy apps.
3. GPU / peripherals in the secure world (user wants to get there; order vs 2 undecided).

**Why:** user values small, easily-upstreamable, low-TCB changes (cited RT-TEE being a few hundred lines) plus real high-throughput measurements as the gap in TEE papers.
**How to apply:** favor minimal OP-TEE-native designs; measure on Thor with the DRAM perf counters used in cpu_bw/gpu_bw. See [[large-ta-memory-goal]], [[tee-secure-memory-must-be-enforced]].
