---
name: runbook-style-and-pacing
description: "How the user wants flash/build docs and steps handled — runbook sections with command blocks, explicit undo notes, one confirmed step at a time"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 800d3511-40db-4aef-991a-b44cb371b8b0
  modified: 2026-11-13T22:56:19.732Z
---

Docs for reflashing/building must be sections of the EXACT commands that worked (literal paths, not templates with variables), not FAQ/Q&A prose and not a long chronological log; history goes in a separate notes file.
Every non-stock change must be marked explicitly in the doc with how to undo it, and old artifacts kept side by side (never overwrite a working set).
The user runs sudo/flash commands themselves; give the exact single command, confirm board state (lsusb 0955:7026) before each, and check results before the next step.

**Why:** user asked to rewrite progress.md "so it's easy to reflash if I or you read it", rejected FAQ format; asked "make it explicit we did it so we know what to undo"; kept asking to confirm the exact command before running.
**How to apply:** Thor flashing work in ~/Desktop/thor_flash; also see the Thor-side preferences in ~/Desktop/thor_flash/claude_memory/ (e.g. do-only-what-was-asked, flag-non-obvious-steps). Related: [[thor-flash-state]].
