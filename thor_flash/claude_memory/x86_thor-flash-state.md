---
name: thor-flash-state
description: "Jetson Thor OP-TEE carve-out flashing from this x86 host — where the runbook is, what's on the board, hard limits found"
metadata:
  node_type: memory
  type: project
  originSessionId: 800d3511-40db-4aef-991a-b44cb371b8b0
  modified: 2026-11-13T22:56:19.697Z
---

Runbook = `~/Desktop/thor_flash/progress.md` (read it first; history in `notes/x86_flash_log_2026-11-13.md`, serial logs in `logs/`).
State 2026-11-13: slot A = 4GB-2MB OP-TEE set (S=0xFFE00000), boots, free -g 118; xtest/bigmem for 4gb not yet recorded.
Slot B never written (stock). Saved signed packages: `bsp/Linux_for_Tegra/unified_flash/out_{2gb,4gb}` (local only, gitignored).

Hard facts learned (not in NVIDIA docs):
- Thor flashing: `l4t_initrd_flash.sh --no-flash ...` then `flash_bsp_images.py ... -u A_MB1_BCT A_secure-hv A_sec-partition` with ABSOLUTE paths. `-k` fails (UNIFIED_FLASH). NEVER `--flash-only` (package includes NVMe system.img).
- MB1: TZDRAM >~2GB -> "Timeout while polling for CO 49 scrub" unless BCT `enable_dram_staged_scrubbing = <0>` ("option B", non-stock, currently active).
- MB2: SP mem_size read as 32-bit -> OP-TEE region must be < 4GB (36GB failed: "SP: Accessing SP1 ... after boundary").
- OP-TEE cross-builds fine on x86 (needs build-essential + gcc-aarch64-linux-gnu etc.).

**Why:** research goal is large TA memory on Thor ([[large-ta-memory-goal]] in ~/Desktop/thor_flash/claude_memory); >4GB needs a second FF-A memory region or an NVIDIA fix.
**How to apply:** start any Thor flash/build task from progress.md; verify packages with tools/check_package.py before flashing.
