# Test 09 — Determinism & Reproducibility

**Probes:** whether the same seed and greedy sampling produce
identical output run-to-run and build-to-build — important for
debugging (a nondeterministic engine makes every other test in this
suite noisier than it needs to be) and for catching sampler bugs.

## Setup

- Model: baseline, temp 0 (pure greedy), fixed seed, single-threaded
  if the port allows it (multi-threaded reductions can otherwise
  introduce float nondeterminism even at temp 0).

## Prompt

```
List the first 12 Fibonacci numbers, one per line, with no other
text.
```

(Deterministic, easily diffable, and long enough to catch a
divergence partway through generation.)

## Procedure

1. Run the prompt 3 times in fresh processes, same build, same seed,
   temp 0.
2. Diff the three outputs byte-for-byte.
3. If the port supports prompt caching / session save-restore, save
   a session after the prompt, restart, restore, and generate again
   — compare against the fresh run.

## Pass criteria

- [ ] All 3 fresh runs produce byte-identical output. Any divergence
      at temp 0 with a fixed seed indicates either a real
      nondeterminism bug (uninitialized memory, race condition in a
      CUDA kernel, thread-count-dependent reduction order) or that
      the seed/temp flags aren't being honored.
- [ ] Session save/restore reproduces the same continuation as the
      uninterrupted run — divergence here points specifically at the
      KV cache (de)serialization path.
- [ ] If the port exposes a `--no-cont-batching` / single-vs-batched
      mode, confirm output is identical between them for a single
      sequence (batching should never change output for one request
      in isolation).

## Stretch variant

- Repeat with a non-trivial batch size (multiple concurrent
  sequences) and confirm each individual sequence's output still
  matches its single-sequence run — cross-sequence contamination in
  batched decoding is a classic and subtle bug class.

## Common failure modes this catches

- CUDA kernels with nondeterministic reduction order (common after
  performance-focused kernel rewrites).
- Seed handling that's ignored or applied inconsistently across CPU/
  GPU backends.
- KV cache serialization bugs that don't manifest in normal use but
  break save/restore workflows.
