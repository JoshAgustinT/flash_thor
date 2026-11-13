---
name: ask-before-rerunning-collectors
description: Mock new plots with fake data first and agree before collecting; never re-run or overwrite data unasked
metadata:
  node_type: memory
  type: feedback
  originSessionId: d26065d3-027b-439f-bfba-acd199291ce8
  modified: 2026-10-19T03:45:54.921Z
---

Don't run anything on the board (not even quick smoke tests) while a new test's plan is still being discussed; agree on the plan first. For a new test or a new plot layout, first write fake data in the exact json format the collector will produce, plot it, and get the user's OK on what the lines / axes / points are. Only then run the collector, once. For plot tweaks on existing data, touch only plot_*.py and re-plot the existing json. Never start a run that overwrites an existing json/log without asking (collectors rewrite thor_*.json and .log from the first result). Always say which png is current; regenerate or flag stale / partial pngs so the user doesn't look at the wrong one.

**Why:** the collect_groups line plot (2026-10-18) took ~6 iterations and several 10-45 min runs because I guessed the design (packed/spread, sparse, categorical, by #groups) instead of agreeing on it; a rejected command had already overwritten the good json; and the user kept viewing stale/partial pngs. A fake-data mock settled it immediately.

**How to apply:** mock -> agree -> run once. If new data seems needed, say so and ask first. See [[per-test-progress-and-styles]].
