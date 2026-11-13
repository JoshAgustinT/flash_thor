---
name: no-commit-attribution
description: Never add Co-Authored-By Claude or other attribution lines to commit messages
metadata:
  node_type: memory
  type: feedback
  originSessionId: 6d08c231-5c1d-4c25-b9f7-3ef364acbbcf
  modified: 2026-10-02T19:52:58.547Z
---

Don't add "Co-Authored-By: Claude" (or any Claude attribution) to git commit messages. Keep messages plain and short, e.g. "new repo".

**Why:** User explicitly asked for this.
**How to apply:** Every commit/PR in this user's repos. This overrides the default attribution reminder.
