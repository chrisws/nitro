# Test 05 — KV Cache Stress

**Probes:** correctness of KV cache management under context shifts,
sequence reuse, and eviction — the area most likely to silently break
in a fork optimizing for speed. If you've worked on `KVCachePreset`-
style logic yourself, this is the test most likely to catch the bugs
you already know the shape of.

## Setup

- Model: baseline, context set deliberately small (e.g. 2048) to
  force shifts quickly and cheaply.
- Have a way to observe KV cache state: prompt-eval token counts
  per turn (should be small on a cache hit, large on a cache miss/
  full reprocess) and/or the port's own cache debug output if it has
  one.

## Prompt sequence

1. Fill context to ~90% with a long instruction + filler, ending
   with: `Remember this number: 4471. Now continue the story below.`
   plus ~1500 tokens of unrelated filler text.
2. Ask a short follow-up that pushes past the context limit, forcing
   a shift: `Summarize what just happened in one sentence.`
3. Ask: `What was the number I told you to remember?`
4. Immediately re-ask the exact same question again (no new
   information) to test pure cache-hit behavior.

## Pass criteria

- [ ] Turn 2 doesn't crash or silently truncate to garbage — the
      shift happens (via reprocessing, sliding window, or eviction)
      without corrupting the sequence.
- [ ] Turn 3: if 4471 was evicted by the shift, the model should say
      so or answer from best available context — not confidently
      hallucinate a different number. If it *wasn't* evicted, it
      should answer correctly. Either is fine; a confidently wrong
      number is the failure.
- [ ] Turn 4's prompt-eval token count is near-zero / matches a cache
      hit — if it reprocesses the full prompt again, cache reuse
      across identical repeated turns is broken.
- [ ] No VRAM growth across the sequence beyond what the configured
      context size implies (watch for a KV cache leak where old
      sequences aren't actually freed on shift).

## Stretch variant

- Repeat the whole sequence three times with `--cache-type-k q8_0`,
  `f16`, and `q4_0` (or the port's equivalent cache quantization
  flags) and confirm behavior is consistent across cache dtypes —
  this is exactly the kind of thing a `KVCachePreset`-style enum is
  meant to guard, so it's worth checking a fork hasn't broken one
  cache type while "improving" another.

## Common failure modes this catches

- Context-shift logic that corrupts or drops the wrong tokens (e.g.
  shifting system tokens instead of oldest conversation tokens).
- KV cache not actually being reused on identical repeated prompts,
  silently costing full reprocessing every turn.
- Cache dtype (F16/Q8/Q4) changes that work in isolation but break
  when combined with context shift or sequence removal.
