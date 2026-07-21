# Test 04 — Chat Template Fidelity

**Probes:** whether the port applies each model's chat template
correctly — a common regression site since templates are
model-specific and easy to get subtly wrong (wrong BOS handling,
wrong role tags, missing `add_generation_prompt` equivalent).

## Setup

- Run the same short conversation through at least two model
  families with meaningfully different templates, e.g. a Qwen model
  and a Gemma model (chat template fingerprinting between these two
  is a good pair to use, since their formats diverge noticeably).
- Use `--jinja` (or the port's equivalent) so the template is read
  from the GGUF metadata, not hardcoded.

## Prompt

```
System: You are a terse assistant. Answer in one sentence.
User: What causes tides?
```

## Pass criteria

- [ ] For each model, dump the actual formatted prompt the engine
      sends to the model (most builds have a verbose/debug flag for
      this) and confirm role tags, special tokens, and BOS/EOS
      placement match that model's documented template — not a
      generic fallback template.
- [ ] The system prompt is actually honored (terse, one sentence) —
      if it's ignored, the template likely isn't being applied at
      all and the model is falling back to raw completion.
- [ ] No leaked template control tokens (e.g. `<|im_start|>`,
      `<start_of_turn>`) appearing as visible text in the output.
- [ ] Multi-turn: append a second user turn and confirm the *history*
      is re-wrapped correctly (previous assistant turn gets the
      right role tag, not re-tagged as user or system).

## Stretch variant

- Swap models mid-session (unload model A, load model B, same
  frontend/session state) and confirm the template switches
  correctly rather than sticking with whichever template was
  cached from model A.

## Common failure modes this catches

- Hardcoded/generic template silently overriding the GGUF's actual
  `chat_template` metadata.
- Wrong end-of-turn token for the specific model family, causing run-
  on generation or premature stopping.
- Template caching bugs when switching models within one process.
