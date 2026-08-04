# Memory Debugging (C++)

Use this for crashes, corruption, leaks, or "works sometimes" bugs that smell like a memory
safety issue — use-after-free, double-free, buffer overrun, uninitialized read, or a dangling
pointer surviving a reload/teardown path.

## Reach for the right tool first

- **AddressSanitizer (ASan)**: first choice for use-after-free, buffer overflow, double-free.
  Build with `-fsanitize=address -g -O1` (or a dedicated CMake sanitizer build type) and re-run
  the repro. ASan's report gives the exact free/alloc site — read both stack traces before
  touching code.
- **UndefinedBehaviorSanitizer (UBSan)**: `-fsanitize=undefined` catches signed overflow,
  misaligned access, null-pointer arithmetic, and similar UB that doesn't always crash but
  silently corrupts state. Worth running even when ASan comes back clean.
- **Valgrind (memcheck)**: slower than ASan but catches some uninitialized-read cases ASan
  misses, and doesn't require a special build — useful when you can't easily rebuild with
  sanitizer flags (e.g. a release binary in the field).
- **valgrind --tool=helgrind or ASan's ThreadSanitizer**: if the bug is intermittent and
  timing-sensitive, suspect a race before anything else — this is a different bug class from
  a deterministic memory error and the tool choice reflects that.

## Where to look first in this kind of codebase

- **Object lifetime across reload paths** — anywhere a resource is torn down and rebuilt (model
  reload, context reset, TUI plane recreation), check that every raw pointer referencing the old
  instance is either updated or invalidated, not left dangling with stale contents that happen
  to still look plausible.
- **`unique_ptr`/`shared_ptr` ownership crossing a boundary** — a raw pointer or reference
  extracted from a smart pointer and stored somewhere that outlives the smart pointer's scope is
  the single most common source of use-after-free in code that otherwise "uses RAII."
- **Buffers sized from one assumption used with another** — e.g. a buffer sized for a fixed
  token count reused after a config change that increases it.

## Process

1. Get ASan/UBSan output first — it's the fastest path to an exact fault site and is usually
   enough on its own.
2. Read the *free* stack trace as carefully as the *use* stack trace — the bug is almost always
   about why the object was freed when it was, not just that it was used after.
3. Fix the ownership/lifetime issue at its source (who should own this, and for how long) rather
   than patching the crash site with a null check that hides the deeper problem.
4. Re-run under the sanitizer that caught it, plus the others, before considering it resolved —
   a fix for one class of memory bug can introduce another.

## What NOT to do

- Don't add a null check or a `try/catch` around a crash without understanding why the pointer
  was invalid — that treats the symptom and leaves the actual lifetime bug in place.
- Don't disable a sanitizer finding as a "false positive" without being certain — sanitizers are
  precise about what they report even when the fix isn't obvious.
- Don't skip re-testing under the sanitizer after a fix; a lifetime fix that "looks right" but
  wasn't re-verified under ASan is not confirmed.
