---
name: self-contained-test-dirs
description: "Each scripts/<test>/ dir must stand alone with its own logic; don't factor shared helpers into scripts/common.py"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 9d9123ce-d372-4ea5-84da-bc3caa03a974
  modified: 2026-10-09T07:33:23.220Z
---

Each test directory under `scripts/` (e.g. cpu_bw, gpu_bw) keeps its own collector, perf call, clock checks and plotter. Don't move shared logic into `scripts/common.py` for them, even if that means some duplication. A dir that combines others (mixed_bw) is the exception and may use their binaries.

**Why:** the user wants each test easy to rerun and to read/verify on its own later. They rejected a refactor that shared helpers across dirs ("hard disagree").

**How to apply:** when adding a new test dir, write it self-contained. Favour simplicity and readability over DRY; the user reviews and explains this code themselves.
