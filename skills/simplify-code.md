# Simplify Code

Use this for a refactor pass: improving clarity, reducing duplication, or cutting complexity
without changing observable behavior. This is not the skill for adding features.

## Ground rule

- Behavior must be identical before and after. If tests exist, they must pass unchanged before
  and after. If they don't exist for the code you're touching, that's a signal to write a quick
  characterization test first (see Test-Driven Development skill) rather than refactor blind.

## What to look for

- **Duplication** — repeated logic across functions/files that should be one function. In this
  codebase specifically: check for duplicated tool-parsing, notcurses plane setup, or
  llama.cpp context-management boilerplate that's been copy-pasted across call sites.
- **Deep nesting** — prefer early returns / guard clauses over pyramids of `if`. Three or more
  levels of nested conditionals is usually a sign the function is doing too much.
- **Manual resource management** — raw `new`/`delete`, manual `free()`, or hand-rolled cleanup
  paths that should be RAII (`unique_ptr`, `shared_ptr`, a destructor, or a scope guard).
  This matters more than usual here given how much of the codebase touches model/context
  lifetime and TUI plane teardown.
- **Long functions doing multiple things** — split along natural seams (parse / validate /
  execute), not arbitrarily. A function should be extractable in one clean cut, not requiring
  new shared mutable state to split.
- **Premature abstraction** — the opposite failure mode. A one-call-site interface, a template
  with a single instantiation, or a config flag nothing ever varies is complexity that isn't
  earning its keep. Collapse it back down.

## Process

1. Confirm current behavior first — run the existing tests, or manually exercise the path if
   none exist.
2. Make one category of change at a time (e.g. "extract duplication" as one pass, "convert to
   RAII" as a separate pass). Don't mix unrelated refactors in one diff — it makes review and
   bisection harder later.
3. Re-verify behavior after each pass, not just at the end.
4. Prefer smaller, reviewable diffs over one large rewrite, even if the end state is the same.

## What NOT to do

- Don't refactor and add functionality in the same change — separate them, even if it means two
  commits back to back.
- Don't "simplify" by removing error handling or edge-case checks that look unnecessary but
  were added for a reason you haven't confirmed (check git blame/commit message before deleting).
- Don't chase style preferences with no functional payoff (e.g. reformatting untouched code)
  inside an otherwise-focused simplification diff.
