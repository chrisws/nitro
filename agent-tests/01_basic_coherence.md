# Test 01 — Basic Coherence

**Probes:** the build loads the model, runs a forward pass, samples
tokens, and stops cleanly. This is the smoke test — if it fails,
nothing else in the suite is worth running.

## Setup

- Model: baseline (see README)
- Context: 4096
- Temp: 0.7, top-p 0.9 (or the port's defaults)
- `n_predict`: 256

## Prompt

```
Explain, in three short paragraphs, how a hash table resolves
collisions. Use plain English, no code.
```

## Pass criteria

- [ ] Output is grammatical, on-topic English (or the model's expected
      language) throughout — no garbled tokens, no repeated-token
      loops, no mid-word truncation.
- [ ] Generation stops at a natural end (EOS / stop token), not by
      hitting `n_predict` mid-sentence.
- [ ] No crash, no VRAM allocation error, no silent fallback to CPU.
- [ ] Load time and time-to-first-token are logged for later
      comparison (not pass/fail, just record it).

## Stretch variant

- `n_predict`: 1024, temp 1.0.
- Watch specifically for degeneration late in the generation
  (repetition loops are more likely to surface at length/temp
  extremes and are a common site for sampler bugs in new ports).

## Common failure modes this catches

- Broken tokenizer detokenization (mangled/missing bytes, especially
  around punctuation or multi-byte UTF-8).
- Sampler off-by-one or wrong default causing runaway repetition.
- Missing/incorrect EOS handling for the model's chat template,
  causing the model to keep talking past its turn.
