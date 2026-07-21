# Test 10 — Unicode & Tokenizer Edge Cases

**Probes:** tokenizer/detokenizer round-trip correctness — a
surprisingly common regression site since tokenizer code is often
reimplemented or hand-optimized per-port rather than shared with
upstream.

## Setup

- Model: baseline (or any model; this test is mostly
  engine/tokenizer-side, not model-quality-side).

## Prompt

```
Repeat exactly, on separate lines, each of the following:
1. café
2. naïve résumé
3. 日本語のテスト
4. 🎉🚀✨ emoji test 🐍
5. "quotes" and 'apostrophes' and — em-dashes
6. a mix: héllo世界🌍
```

## Pass criteria

- [ ] Each line is reproduced correctly — no mangled diacritics, no
      broken multi-byte UTF-8 sequences (shows up as `�` or missing
      characters), no dropped/duplicated emoji.
- [ ] CJK text (line 3) isn't silently transliterated, dropped, or
      replaced with spaces.
- [ ] Emoji (line 4, 6) render as the correct emoji, not a
      replacement character or an incorrect but plausible-looking
      emoji.
- [ ] Streaming output (if the port streams tokens) doesn't split a
      multi-byte UTF-8 character across two stream chunks in a way
      that corrupts display — this is a very common bug in ports that
      handle streaming detokenization themselves rather than
      buffering incomplete sequences.

## Stretch variant

- Feed a long mixed CJK/Latin/emoji document (a few hundred tokens)
  and ask for verbatim repetition of a specific line in the middle —
  combines tokenizer correctness with the retrieval check from
  Test 02, and is a good stress case for anything that indexes
  context by byte offset instead of by token/codepoint.

## Common failure modes this catches

- Byte-pair encoding edge cases mishandled by a reimplemented
  tokenizer (as opposed to using the model's actual `tokenizer.json`/
  GGUF vocab).
- Streaming detokenization splitting multi-byte UTF-8 sequences
  across chunk boundaries.
- Special/control tokens in the vocab colliding with legitimate text
  containing similar byte sequences.
