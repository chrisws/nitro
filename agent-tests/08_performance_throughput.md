# Test 08 — Performance & Throughput (8GB Stress)

**Probes:** the actual numbers a fork's speed claims live or die on —
prompt processing tok/s, generation tok/s, and VRAM footprint under
realistic 8GB pressure. This is the one test that's inherently
comparative rather than pass/fail.

## Setup

- Model: baseline, then repeat at stretch settings (see below).
- Use the port's built-in benchmark tool if it has one (llama.cpp
  upstream ships `llama-bench`); otherwise time a fixed prompt +
  generation manually with the same model/quant/context across
  builds.
- Record: prompt-eval tok/s (pp), generation tok/s (tg), peak VRAM
  (`nvidia-smi` during the run), and load time.

## Procedure

1. Cold-load the model, record load time.
2. Run a fixed 512-token prompt with `n_predict 256`, record pp
   tok/s and tg tok/s.
3. With the model still loaded, immediately run a second identical
   generation (cache-cold prompt, i.e. don't reuse the previous
   prompt) and confirm tok/s is stable, not degrading run-over-run.
4. Record peak VRAM via `nvidia-smi --query-gpu=memory.used
   --format=csv -l 1` running alongside step 2-3.

## Pass criteria

- [ ] No crash or OOM at baseline settings (this should never fail
      on an 8GB card — if it does, that's the headline result).
- [ ] tg tok/s is stable across repeated runs (±5%), not degrading —
      a fork with a memory leak or fragmentation bug often shows
      declining tok/s over successive generations before it OOMs.
- [ ] Peak VRAM stays within a predictable margin of
      `model size + KV cache size` for the configured context — a
      much larger-than-expected footprint suggests a fork isn't
      freeing intermediate buffers correctly.
- [ ] Numbers are directionally consistent with what you'd expect
      for the GPU (a good reference point: roughly what upstream
      llama.cpp achieves on the same hardware/model/quant — a fork
      claiming a large speedup is either right, in which case say by
      how much, or has cut a corner somewhere else in this suite).

## Stretch variant

- Push context to the stretch setting (16-32K, see README) and/or
  increase `n_predict` to 1024+, and re-run the full procedure. This
  is where 8GB cards actually get pressured — watch specifically for
  OOM, silent CPU fallback (tok/s cratering with no error), or
  swapping.
- If the port supports batched/parallel decoding, run 2-4 concurrent
  sequences and confirm aggregate throughput scales reasonably
  rather than collapsing.

## Common failure modes this catches

- Speed gains that come from skipping bounds/safety checks that
  matter under real memory pressure (fast until it silently
  corrupts or OOMs).
- VRAM fragmentation causing throughput to degrade over a long
  session even without a hard OOM.
- Claimed speedups that don't reproduce, or only apply to a
  cherry-picked config not representative of normal use.
