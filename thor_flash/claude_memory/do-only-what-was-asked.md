---
name: do-only-what-was-asked
description: "Stop at the step the user asked for; don't chain on installs/tests/downloads without asking"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 18a53fe7-2be3-4e8f-b700-e1d803c094d6
  modified: 2026-11-12T20:43:05.792Z
---

Do the step the user asked for, then stop and report. Don't chain extra actions (installing packages, smoke tests, model downloads, extra notes) because they seem like the obvious next step.

**Why:** asked "can we just install from the docs?" and I installed vLLM, ran a test model and wrote notes; the user said "u did way more than i wanted".
**How to apply:** for questions, answer (a quick read-only check is fine); propose the next action and wait. Related: [[ask-before-rerunning-collectors]].

2026-11-12 repeat: I offered "should I start the Stage 1 build?", the user replied "yes but <question about crashes>", and I copied sources, patched, edited conf.mk and launched a build. They said "don't rebuild stuff we aren't doing it yet!". A "yes but ..." that carries a question is not a go-ahead: answer the question and wait for an explicit "go". Before builds, copies or source edits, confirm unless the user clearly said to do it now.
