# Plan

Use this before starting any change that touches more than one or two files, or where the
right approach isn't already obvious. Produce a plan, not code, as the first output.

## What a good plan contains

- **Scope statement**: one or two sentences on what's changing and, just as importantly, what's
  explicitly out of scope for this change.
- **File-by-file breakdown**: exact paths, and for each, what changes there (new function,
  modified signature, new file entirely). Avoid vague steps like "update the tool protocol" —
  say which file, which function, and what the new behavior is.
- **Ordering**: the sequence steps need to happen in, especially where one step depends on
  another compiling/working first (e.g. a new tool type needs its parser before the dispatch
  table can reference it).
- **Bite-sized steps**: each step should be small enough to be its own reviewable commit. If a
  step description needs "and" more than once, it's probably two steps.
- **Open questions**: anything genuinely ambiguous (e.g. "should this replace the existing
  `run_allowed` allowlist path or sit alongside it?") called out explicitly rather than silently
  decided.

## Process

1. Read enough of the existing code to ground the plan in what's actually there — don't plan
   against an assumed structure.
2. Write the plan before touching any implementation file.
3. Get confirmation on the plan (or at minimum, state it clearly enough that a reader could
   object) before executing — don't quietly start implementing partway through planning.
4. Execute step by step, checking off each one. If reality diverges from the plan mid-execution
   (a file doesn't have the structure you assumed), stop and revise the plan rather than
   improvising silently.

## What NOT to do

- Don't write a plan so vague it could describe any change ("improve the tool protocol").
  Specificity is the entire value of doing this step.
- Don't skip planning for "quick" changes that turn out to touch build config, a header used in
  three places, and a runtime code path — if it's not obviously confined to one function, plan
  it.
- Don't let the plan include actual code snippets beyond a short illustrative signature — the
  plan is about shape and sequencing, not implementation.
