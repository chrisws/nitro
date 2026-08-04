# Spike (Throwaway Prototype)

Use this when the goal is to answer a question — "does this approach even work," "what does
this API actually return," "is this fast enough" — not to ship production code. A spike is
explicitly disposable.

## Before starting

- State the question the spike is meant to answer in one sentence. If you can't state it, it's
  not a spike, it's just unplanned coding — use the Plan skill instead.
- Set a rough time/scope box. If you're not close to an answer well past that, stop and
  reassess rather than continuing to dig.

## While spiking

- No test coverage required. No error handling beyond what's needed to see the result. No
  concern for code style, naming, or structure.
- Isolate it: a scratch file, a throwaway branch, or a directory clearly marked temporary (e.g.
  `spike/` or a `_scratch.cpp`) — never spike directly on top of code you intend to keep in the
  same diff.
- It's fine to hardcode paths, skip CLI arg parsing, comment out unrelated code, or use global
  state to get to an answer faster. None of this is meant to survive.
- Stub out anything not needed to answer the question — e.g. if you're spiking a new tool-call
  format, don't wire it through the full notcurses render path if a stderr dump answers the
  question just as well.

## After spiking

- Write down what you learned in a sentence or two (in a commit message, a comment, or wherever
  the team keeps such notes) — the answer to the question is the actual deliverable, not the
  code.
- Then throw the spike code away and reimplement properly: real error handling, tests per the
  TDD skill, and code review. Do not extend spike code into production by incrementally cleaning
  it up in place — the shortcuts taken to move fast tend to survive that process invisibly.
- If the spike reveals the approach doesn't work, that's a successful spike. Document why, and
  move to the next hypothesis with the wasted implementation avoided.

## What NOT to do

- Don't merge spike code into main "just this once because it mostly works."
- Don't spend spike time polishing an approach you already have doubts about — the whole point
  is a fast, cheap answer, not a demo.
