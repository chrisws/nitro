# Requesting Code Review

Use this as a self-review pass before opening a PR or asking someone else to review — catch
what a reviewer would catch, before they have to.

## Before opening the PR

- **Re-read your own diff top to bottom**, not just the files you remember changing. Look for:
  leftover debug prints, commented-out code, TODOs you meant to resolve, and unrelated
  formatting churn mixed into a focused change.
- **Run the full test suite**, not just the tests near your change.
- **Build clean** — no new compiler warnings introduced. Check the actual build output, don't
  assume; a change that compiles isn't the same as a change that compiles warning-free.
- **Run sanitizers if the change touches memory/lifetime** (pointers, buffers, object teardown,
  anything in the model-loading or context-reload paths) — see Memory Debugging skill.
- **Check the diff size** — if it's sprawling across unrelated concerns, split it into separate
  PRs/commits along those concern lines rather than asking a reviewer to hold multiple unrelated
  changes in their head at once.

## Writing the description

- State *what* changed and *why*, not just what — the diff already shows what; the description's
  job is the reasoning a diff can't carry (why this approach over an alternative, what
  constraint drove it).
- Call out anything you're deliberately not confident about — a workaround, a section you
  weren't sure how to test, an assumption about how another part of the system behaves. Flagging
  it yourself is far more useful to a reviewer than letting them find it and wonder if it was
  intentional.
- If the change fixes a bug, link or describe the repro so the reviewer can verify the fix
  against it, not just read the diff and trust it.

## What NOT to do

- Don't open a PR you haven't personally re-read end to end — reviewers should be checking your
  reasoning, not doing the first pass of proofreading for you.
- Don't bundle a refactor and a behavior change in the same diff (see Simplify Code skill) —
  it forces the reviewer to untangle which lines are "safe" restructuring and which actually
  need scrutiny.
- Don't respond to review feedback by fixing only the exact line flagged — check if the same
  issue exists elsewhere in the diff before re-requesting review.
