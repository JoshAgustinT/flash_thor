---
name: flag-non-obvious-steps
description: "In plans/summaries, explicitly call out steps that differ from what the user would assume (missing tools, manual packaging), not just bury them in progress.md"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 18a53fe7-2be3-4e8f-b700-e1d803c094d6
  modified: 2026-11-12T20:33:27.393Z
---

When summarizing a plan in chat, explicitly call out any step or prerequisite the user likely doesn't expect: e.g. that NVIDIA's optee_src_build.sh does NOT produce tos.img (manual dtc + sptool step), or that the flash script only exists in the BSP download (not in public_sources). Don't compress these into "build X" or leave them only in progress.md.

**Why:** 2026-11-12 the user only learned late in the conversation that they had never had the flash script and that the OP-TEE build doesn't output a flashable image. They said "see thats important info your just telling me now".

**How to apply:** In any plan or summary, add a short "things you might not expect" list: what's missing, which steps are manual, and where each tool comes from.
