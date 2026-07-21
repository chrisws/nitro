# Test 06 — Grammar-Constrained (GBNF) Decoding

**Probes:** whether GBNF grammar constraints are enforced correctly
during sampling — used for anything that needs guaranteed-valid JSON
output (structured tool args, form filling, etc).

## Setup

- Model: baseline.
- Grammar (GBNF), constrain to a fixed JSON schema:

```gbnf
root   ::= "{" ws "\"name\"" ws ":" ws string "," ws "\"age\"" ws ":" ws number "," ws "\"tags\"" ws ":" ws array "}"
string ::= "\"" [^"]* "\""
number ::= [0-9]+
array  ::= "[" ws (string (ws "," ws string)*)? ws "]"
ws     ::= [ \t\n]*
```

## Prompt

```
Generate a fictional user record. It should have a name, age, and a
list of interest tags.
```

Run with the grammar attached (`--grammar-file` or the port's
equivalent).

## Pass criteria

- [ ] Output parses as valid JSON matching the schema on every run —
      no exceptions, no fields out of order (order should be
      guaranteed by the grammar), no trailing garbage after the
      closing brace.
- [ ] Content is still sensible (an actual name, a plausible age,
      real-looking tags) — the grammar shouldn't force the model
      into degenerate output like empty strings or age `0` on every
      run.
- [ ] Run 5-10 times; 100% schema validity is the bar (that's the
      point of grammar constraints — partial compliance means the
      constrained sampler itself is buggy).
- [ ] Generation doesn't hang or infinite-loop trying to satisfy an
      unsatisfiable grammar state.

## Stretch variant

- Nest the schema one level deeper (an object inside the array
  instead of plain strings) and increase grammar complexity
  (alternation, repetition ranges like `{2,4}`) to exercise more of
  the GBNF parser.

## Common failure modes this catches

- Grammar sampler applied only loosely (biases logits but doesn't
  hard-constrain), producing occasional schema violations.
- Performance cliff or hang on more complex grammars due to
  inefficient grammar-state stack management.
- Silent fallback to unconstrained sampling if the grammar file fails
  to parse (worth checking the port actually errors instead of
  ignoring it).
