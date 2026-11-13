---
name: per-test-progress-and-styles
description: "Test run recaps go in scripts/<test>/progress.md, not the root progress.md; one-off plot styling stays in the test's plotter, not styling/"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 9fc2373a-fa52-4d00-bd26-7d16f678487d
  modified: 2026-10-09T20:44:31.982Z
---

Write a test's run notes/results to `scripts/<test>/progress.md` (create it if missing), never to the root `progress.md`. progress.md is append-only: add new dated entries at the end of the file; don't rewrite, replace or delete existing lines (even outdated ones). When I explain findings or methodology in chat (e.g. how something was discovered), append that explanation to progress.md too. progress.md is a status log of what has been done/found, not a plan (no speculative "next" items) — but DO record the commands the user is going to run (setup, download, benchmark), so they can copy them from there. Don't edit `styling/paper_style.py` / `PLOTTING.md` for a single figure's needs (colours, display names, variants); put them in that test's `plot_data.py`.

**Why:** the user wants each test dir to carry its own history (a running log, so nothing earlier gets lost), and the shared style kept stable; a one-off figure isn't a reason to change it.

**How to apply:** goes with [[self-contained-test-dirs]]; when a test needs custom colours or labels, define them locally in its plotter.
