# Test 07 — Quantization Regression

**Probes:** whether a port's quantization/dequantization kernels
produce sane output — relevant when a fork claims new or optimized
quant formats, or after upstream changes touch the CUDA quant
kernels.

## Setup

- Model: same base weights, quantized to at least three levels you
  can compare: `Q4_K_M` (baseline), `Q8_0` (near-lossless reference),
  and one lower-bit format the port supports or claims to have
  improved (e.g. `IQ3_XS`, `IQ2_M`).
- Same prompt, same seed, temp 0 (greedy) across all three, so
  differences are attributable to quantization, not sampling noise.

## Prompt

```
Solve step by step: A train leaves station A at 60 km/h. Two hours
later, a second train leaves the same station on the same track at
90 km/h. How far from station A does the second train catch the
first?
```

(A math word problem is useful here — quantization damage tends to
show up as arithmetic slips or lost reasoning steps before it shows
up as fluent-sounding nonsense.)

## Pass criteria

- [ ] `Q8_0` gets the correct answer (360 km) — this is the
      reference; if this fails, the bug is in the engine, not the
      quant format.
- [ ] `Q4_K_M` gets the correct answer or a reasoning path that's
      clearly still coherent even if it slips numerically — full
      incoherence at Q4_K_M is a red flag for the quant/dequant path,
      since this quant level is normally solid.
- [ ] Lower-bit format may legitimately struggle with the arithmetic,
      but output should still be fluent, on-topic text — garbled
      tokens or repetition loops specifically at lower quant (and
      not at Q8_0) point to a dequant kernel bug rather than expected
      quality loss.
- [ ] File sizes and reported perplexity (if the port has a
      `--perplexity` mode) roughly match published figures for that
      quant type — a build that's silently mis-dequantizing often
      still runs, just with degraded/garbage-adjacent output and
      normal-looking perplexity is a useful independent check.

## Stretch variant

- Run the port's `--perplexity` (or equivalent) mode against a
  standard text sample for each quant level and compare against
  known-good numbers for that model/quant combo from upstream docs
  or your own upstream baseline run.

## Common failure modes this catches

- New/optimized quant kernels that are faster but numerically wrong
  (common when a fork hand-writes new CUDA kernels for a quant
  format).
- Regressions introduced when upstream changes touch shared
  dequantization code paths used by multiple quant types.
