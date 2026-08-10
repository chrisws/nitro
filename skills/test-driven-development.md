# Test-Driven Development (C++)

Use this when implementing new behavior or fixing a bug in a way that should stay fixed.
Enforce RED → GREEN → REFACTOR — do not write implementation code before a failing test exists.

## RED: write a failing test first

- Write the smallest test that exercises the new behavior or reproduces the bug. It must fail
  for the right reason (compile-and-fail on missing behavior, not fail because of a typo).
- Use whatever test framework the project already uses (Catch2/GoogleTest); don't introduce a
  second framework for one test.
- If there's no test harness wired up for the code you're touching yet, that's the first task:
  get one file's worth of infrastructure working before writing the substantive test.
- For a bug fix specifically: the failing test *is* the reproduction of the bug. If you can't
  write a test that fails against current behavior, you don't understand the bug yet — go back
  to the debugging skill.

## GREEN: minimal code to pass

- Write the least code that makes the test pass. Resist adding generality, extra parameters, or
  handling for cases nothing has asked for yet.
- Don't touch other tests or unrelated code in this step. If you notice something else broken,
  note it and move on — fix it in its own RED/GREEN cycle.

## REFACTOR: clean up with the safety net in place

- Now improve names, extract duplication, simplify control flow — with the test suite green
  throughout. Re-run tests after each refactor step, not just at the end.
- Refactoring should never change the test's assertions. If you find yourself wanting to change
  what the test checks, that's a new RED step, not a refactor.

## For legacy/untested code

- Don't try to add full coverage to an untouched file before making a change to it. Write a
  narrow characterization test that pins down current behavior for the specific path you're
  about to modify, then proceed with RED/GREEN/REFACTOR on top of that.

## What NOT to do

- Don't write the implementation first and backfill tests after — this produces tests that
  confirm what the code does, not what it should do, and misses edge cases the implementation
  author wasn't thinking about.
- Don't skip RED because "I already know this will fail" — actually run it and see it fail.
  Tests that were never observed to fail sometimes turn out to not test anything.
- Don't commit with any test in a skipped/disabled state without an explicit tracked reason.
