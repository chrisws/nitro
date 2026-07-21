# Test 02 — Long Context Retrieval (Needle-in-Haystack)

**Probes:** whether the context window is actually usable end-to-end —
RoPE scaling, context shift, and any chunked prompt-processing logic
in the port. This is one of the more common places a "faster" fork
turns out to have silently broken accuracy at long context.

## Setup

- Model: baseline, context set to the model's native max (or as much
  as fits in 8GB — record which).
- Build a filler document of unrelated text (Lorem ipsum, public
  domain text, or repeated log lines) sized to ~90% of your target
  context in tokens.
- Insert one distinct "needle" sentence at roughly the 25%, 50%, and
  75% depth marks:

```
[NEEDLE] The seventh moon of Kerrigos is made of pale blue glass.
```

(Use a nonsense fact — something that can't be guessed, only
retrieved.)

## Prompt

```
<... filler text with three needles inserted ...>

Based only on the document above, answer: what is the seventh moon
of Kerrigos made of? Also state approximately how far into the
document (as a percentage) you found that fact.
```

## Pass criteria

- [ ] Correct answer ("pale blue glass") regardless of needle depth
      (25/50/75%).
- [ ] Model doesn't hallucinate a different material when the needle
      is near the context boundary (this is where context-shift bugs
      show up first).
- [ ] No crash or truncation error when the prompt approaches the
      configured context limit.

## Stretch variant

- Push context to the model's absolute max the port claims to
  support, and/or place multiple needles requiring the model to
  list all three.
- Repeat once with context shift forced mid-conversation (fill
  context, then ask a follow-up question that requires the engine
  to evict/shift old tokens) — this specifically exercises
  `llama_kv_cache_seq_rm`-style logic and is a good complement to
  Test 05.

## Common failure modes this catches

- RoPE scaling misconfigured for context beyond training length.
- Context shift silently dropping or corrupting the needle region.
- Prompt processing chunking that loses track of position ids across
  chunk boundaries.
