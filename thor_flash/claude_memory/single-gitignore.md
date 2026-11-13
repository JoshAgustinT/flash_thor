---
name: single-gitignore
description: Repo has one root .gitignore; add rules there with full paths, never create per-directory .gitignore files
metadata:
  type: feedback
---

Put all ignore rules in the root `ARMHiT/.gitignore` (with `scripts/<test>/...` paths). Don't create nested `.gitignore` files.

**Why:** user said "we have one .gitignore dont make more" when I added scripts/TEE/.gitignore.
**How to apply:** any time something needs ignoring, append a commented block to the root .gitignore. Related: [[self-contained-test-dirs]].
