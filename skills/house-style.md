# House Style

A persistent style/conventions skill, distinct from the task skills (debugging, TDD, etc.) —
this governs *how* code and prose get produced, not what task is being done. Curated from the
broader "AGENT.md" convention used across coding harnesses, filtered against known failure
modes rather than adopted wholesale — some common AGENT.md advice is better enforced by a
linter than repeated in a prompt, and some of it is actively counterproductive. Where a rule
below has a mechanical equivalent (clang-format, clang-tidy), prefer configuring that over
relying on this file to catch it every time — this file is for judgment calls a linter can't
make, not a substitute for one.

## Code style

- **Magic numbers/strings** → extract into named `const`/`enum` when the value recurs or carries
  meaning. A one-off, self-explanatory value can stay inline. A value from an external spec
  (an HTTP status code, a protocol constant) gets a name even if used once — the name documents
  the spec reference.
- **Comments explain why, not what.** The code already says what it does; a comment restating
  that is noise. Explain the reasoning, the constraint, or the non-obvious consequence instead.
  No decorative separator comments (`// ---- Section ----`). No ASCII diagrams in comments —
  they're fragile to maintain and error-prone to generate accurately; link to real documentation
  instead if a system needs a diagram.
- **Reduce nesting** with early returns/continues over deep conditional pyramids — see the
  `simplify-code` skill for the fuller treatment of this.
- **Enums over bare booleans** for function parameters where the call site would otherwise read
  as an unlabeled `true`/`false` — `Logging::Enabled` beats a naked `true` three arguments deep.
- **Blank lines between logical blocks** — let a function breathe rather than reading as one
  unbroken wall.
- **Member visibility is a design decision, not a formatting one** — treat any change from
  `private` to `protected`/`public` as something to flag and get confirmation on, not something
  to do silently while implementing something else.
- **Respect layered boundaries** — a layer talks to its immediate neighbor, not two levels down
  (a UI/TUI component doesn't reach past a service layer straight into a driver or raw socket).
  Punching through layers for a quick fix creates exactly the coupling the layering existed to
  prevent.
- **Minimize the diff footprint for the actual task** — don't reformat, re-comment, or "fix"
  unrelated code while implementing a feature; note anything unrelated you noticed instead of
  silently changing it. Apply this with judgment, though: don't let it push toward multiplying
  near-duplicate functions (`get_total_rounded`, `get_total_float`) just to avoid touching an
  existing signature — sometimes the right-sized change legitimately touches a shared function
  or its call sites, and that's not a violation of this rule.
- **On a bug fix specifically**: write the failing test first, observe it fail, then fix — see
  `test-driven-development` for the full workflow; this is just the reminder that "fix" and
  "write the reproducing test" are not optional to reorder.

## When to stop

Every substantial task should end in one of three states, not trail off indefinitely:

- **Done** — the intended behavior works on the real path, verified, not just "should work."
- **Real progress, honestly bounded** — not complete, but a genuine blocker got removed and the
  next one is now identified with actual evidence, not just "seemed hard."
- **Honest stop** — continuing would require scope creep, a brittle workaround, or tangled logic
  to force it through. Stop and report the specific reason with evidence, rather than
  producing more patches that don't converge.

Don't confuse activity with progress — a stalled attempt is only acceptable if it leaves the
problem narrower, the evidence stronger, or a clearly justified stopping point, not just more
diff.

## Voice, for prose (replies, commit messages, comments)

Write plainly. Say the thing directly rather than performing confidence or empathy about it.

Avoid stock AI-prose tics specifically: "honestly"/"to be clear"/"the honest answer",
"you're absolutely right", "not just X, it's Y" and its cousins, "here's the thing", "in other
words", "at the end of the day", reflexive hedges ("largely", "roughly", "tends to") stacked for
no reason, the aphoristic mic-drop closing line, and raising an objection solely to knock it
down. Vary sentence rhythm and paragraph shape rather than defaulting to a uniform
three-sentences-every-time structure. One em dash per paragraph, at most — a comma usually does
the job.

## Commit messages

1. Blank line between subject and body.
2. Subject line ≤ 50 characters (72 is the hard ceiling).
3. Capitalize the first letter of the subject.
4. No period at the end of the subject line.
5. Imperative mood ("Fix bug", "Add feature") — the subject should complete "If applied, this
   commit will ___."
6. Wrap the body at 72 characters.
7. Body explains *what and why*, not *how* — the diff already shows how; the message is for
   context a diff can't carry.

## What NOT to do

- Don't treat every rule in this file as equally load-bearing — if something here has a linter
  or formatter equivalent, configure that instead of relying on re-deriving it each session.
- Don't force short function names at the cost of clarity — a longer, exact name beats a
  cryptic abbreviation chosen to hit a character budget.
- Don't add "what it does" comments or decorative section banners — both add noise without
  adding information the code doesn't already carry.
- Don't keep grinding on a stuck task past an honest-stop point — report the blocker plainly
  instead of generating more unconverging patches.
