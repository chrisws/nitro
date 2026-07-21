# Test 03 — Tool Calling & Agentic Loop

**Probes:** structured tool-call emission and whether a multi-turn
agent loop stays coherent across several tool round-trips. Directly
relevant if you're driving the build from something like Nitro's tool
protocol rather than chatting with it directly.

## Setup

- Model: a model with a known tool-calling chat template (Qwen3,
  Hermes, etc.) — use whatever `--jinja`/chat-template path the port
  supports.
- Define one trivial tool in the system/tool spec:

```json
{
  "name": "get_weather",
  "description": "Get current weather for a city",
  "parameters": {
    "type": "object",
    "properties": { "city": { "type": "string" } },
    "required": ["city"]
  }
}
```

- Have a stub tool executor that returns a fixed fake response, e.g.
  `{"city": "Adelaide", "temp_c": 19, "condition": "cloudy"}`.

## Prompt sequence

1. User: `What's the weather like in Adelaide right now?`
2. Feed the tool result back into the conversation as the tool
   response turn.
3. User (follow-up, same session): `And how does that compare to
   what you'd expect for this time of year?`

## Pass criteria

- [ ] Turn 1 produces a well-formed tool call (parseable JSON args,
      correct tool name) rather than a hallucinated free-text answer.
- [ ] The engine correctly frames the tool-call/tool-result boundary
      per the chat template — no leaked control tokens, no missing
      terminator, no bleed between the tool call and the next turn.
- [ ] Turn 3 correctly uses the tool result from turn 1 (proves the
      KV cache / conversation state survived the tool round-trip
      intact) without needing to re-call the tool.
- [ ] No infinite tool-call loop (model calling the same tool
      repeatedly instead of answering).

## Stretch variant

- Chain two dependent tool calls (e.g. `get_weather` then a second
  tool that needs the first tool's output as an argument) in one
  turn, and confirm the engine handles back-to-back tool-call/
  tool-result cycles without losing position or re-processing the
  entire prompt from scratch each time (watch prompt-eval tok/s on
  turn 2 — it should be fast if KV cache reuse is working).

## Common failure modes this catches

- Chat template mismatch producing malformed or unparseable tool-call
  JSON.
- Terminator/stop-token handling that lets the model keep generating
  past the tool call into a hallucinated tool result.
- KV cache not being correctly extended/reused across the tool
  round-trip, causing the follow-up turn to "forget" earlier context
  or silently reprocess everything.
