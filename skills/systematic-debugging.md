# Systematic Debugging

Use this when investigating a crash, hang, wrong output, or any bug where the cause isn't
already obvious. Do not start editing code until you've completed the steps below.

## 1. Get a reliable repro first

- Find the smallest input/command sequence that triggers the bug every time (or a known
  percentage of the time, if it's non-deterministic).
- If it's intermittent, note that explicitly — intermittent bugs are usually concurrency,
  uninitialized memory, or a race with the model/tool-call loop, not a straightforward logic
  error. Treat them differently from deterministic ones (see Memory Debugging skill for the
  former).
- Never attempt a fix without a repro. A "fix" that isn't validated against a repro is a guess.

## 2. Localize before you hypothesize

- Narrow the bug to a subsystem before theorizing about the cause: TUI/notcurses rendering,
  llama.cpp inference loop, tool-call protocol parsing, KV cache/session state, or build
  config.
- Bisect if the bug is a regression: `git bisect` against a known-good commit is almost always
  faster than reading a diff and guessing.
- Add temporary logging or breakpoints at subsystem boundaries to confirm which side of the
  boundary the corrupted state first appears on, rather than guessing from symptoms alone.

## 3. Use the debugger, not print statements, for anything stateful

- gdb/lldb with a real breakpoint and `bt`/`print`/`watch` beats sprinkling `fprintf(stderr, ...)`
  for anything involving pointers, object lifetime, or cache state — printf debugging shifts the
  bug's timing and can mask it (especially with anything touching KV cache position tracking).
- Set a watchpoint on the specific variable/field that's wrong rather than single-stepping
  through unrelated code.
- For crashes: get a backtrace from the actual fault (core dump or live under gdb), not from
  guessing at the call site.

## 4. Form one hypothesis, test it, then stop

- State the hypothesis explicitly before changing code: "I believe X is null/stale/racing
  because Y."
- Change exactly one variable to test it. If the symptom doesn't move, the hypothesis was
  wrong — revert the change and form a new one. Don't stack unverified changes.
- Resist the urge to fix multiple suspected issues at once "while you're in there." Each fix
  needs its own before/after confirmation against the repro.

## 5. Confirm the fix, then close the loop

- Re-run the original repro and confirm the symptom is gone, not just that the code compiles.
- Check whether the same class of bug exists elsewhere in the codebase (e.g. if it was a stale
  pointer after a reload, check every other reload path).
- Leave a one-line note (commit message or code comment) on *why* the fix works, not just what
  changed — future-you debugging a related issue will need the reasoning, not the diff.

## Anti-patterns to avoid

- Shotgun-fixing: changing several plausible things at once and hoping one works.
- Fixing the symptom at the call site instead of the actual source of corrupted state.
- Declaring victory because the crash didn't repro once — re-run enough times to trust it,
  especially for anything touching cache/session state where timing matters.
