---
name: progress-command-format
description: "How to write shell commands in progress.md notes — one per line, \"> \" prefix, ~-based paths, terse prose"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 147e5941-58cc-4dce-ae26-1ee73591b709
  modified: 2026-10-26T23:08:42.095Z
---

In progress.md notes, write each shell command on its own single line prefixed with `> ` (no `\` continuations, no code fence), using paths from `~` so they work from any directory (no `cd`, no shorthand vars like `O=`). Keep the surrounding prose terse.

**Why:** user found multi-line/relative commands error-prone (ran them from the wrong dir) and said the notes were "too chatty".
Exception to append-only: when a model/version/flag changes, update the earlier command lines in place too, so every command in the file is runnable as-is (user complained when old commands still said Qwen3.6 after switching to 3.8).

**How to apply:** any command recorded in scripts/<test>/progress.md. See [[per-test-progress-and-styles]].
