# llama.cpp Fork/Port Aptitude Tests

A suite of markdown "test cartridges" for evaluating a llama.cpp build —
whether it's a third-party port, a fork claiming improvements, or just
today's `git pull` on upstream. Each test is a self-contained prompt (or
short prompt sequence) plus a pass/fail rubric you can eyeball in under a
minute. They're written to be run *by hand* (paste into the CLI/server of
whatever build you're testing) or wired into a script that pipes the
prompt in and greps the transcript.

## Why these and not a standard benchmark

Standard benchmarks (MMLU, HumanEval, etc.) measure the *model*. These
tests are aimed at the **inference engine** — the stuff that's specific
to a llama.cpp build rather than the weights: KV cache handling, chat
template application, grammar-constrained decoding, tool-call framing,
tokenizer edge cases, batching, and sampler correctness. A port can nail
MMLU and still corrupt the KV cache on a context shift, so these are
meant to catch that class of bug.

## Hardware baseline

Tuned for an 8GB-class consumer GPU (e.g. RTX 5060 8GB) as the floor.
"Stretch" tags in each file mark configurations that push past
comfortable 8GB headroom — bigger model, longer context, or more
concurrent sequences — so you can tell whether a port genuinely handles
memory pressure better, or just runs the easy case.

Suggested models (adjust to what you have pulled):
- **Baseline**: a ~7-9B Q4_K_M model (e.g. Qwen3-8B/9B Q4_K_M) — should
  leave headroom on 8GB at 4-8K context.
- **Stretch**: the same family at Q4_K_M with context pushed to
  16-32K, or a ~12-14B Q4_K_M model — this is where OOM handling,
  KV cache eviction, and offload logic actually get exercised.
- Keep the model and quant **identical** across builds you're
  comparing — the test is the engine, not the weights.

## How to use this as a regression suite

1. Pick a reference build (e.g. last known-good upstream commit) and
   run all tests once, saving outputs to `baseline/`.
2. After a `git pull` or when trying a new fork, run the same tests
   with the same model/quant/seed, saving to `candidate/`.
3. Diff behaviorally, not textually — most of these tests care about
   *properties* (did it stay coherent, did the JSON parse, did VRAM
   stay bounded) not exact wording. Exact-match only matters for
   03 (tool calling), 06 (grammar), and 09 (determinism).
4. A single failure isn't necessarily disqualifying — note it and
   move on. The value is in seeing *which* tests regress between
   builds, since that tells you what actually changed.

## Test index

| # | File | What it probes | Stretch variant |
|---|------|----------------|------------------|
| 01 | `01_basic_coherence.md` | Sanity check: model loads, generates, stops cleanly | Longer generation, higher temp |
| 02 | `02_long_context_retrieval.md` | Needle-in-haystack across full context window | Push to max context / multi-needle |
| 03 | `03_tool_calling_agentic.md` | Structured tool-call emission and multi-turn agent loop | Nested/chained tool calls |
| 04 | `04_chat_template_fidelity.md` | Correct chat template application across model families | Mixed-family swap mid-session |
| 05 | `05_kv_cache_stress.md` | KV cache correctness under context shifts, reuse, eviction | Cache type variation (F16/Q8/Q4) |
| 06 | `06_grammar_constrained_json.md` | GBNF-constrained decoding | Nested schema, large grammar |
| 07 | `07_quantization_regression.md` | Output quality/coherence sanity across quant levels | Low-bit quant (IQ2/IQ3) |
| 08 | `08_performance_throughput.md` | Prompt processing & generation tok/s, VRAM footprint | Sustained load / batch size |
| 09 | `09_determinism_reproducibility.md` | Seeded output reproducibility, `--temp 0` greedy match | Cross-run with cache reuse |
| 10 | `10_unicode_tokenizer_edge_cases.md` | Tokenizer round-trip on unicode, emoji, special tokens | CJK + emoji mixed streams |

## Recording results

A plain table works fine — one row per (build, test):

```
| build          | commit/tag | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | notes |
|----------------|-----------|----|----|----|----|----|----|----|----|----|----|-------|
| upstream       | abcd123   | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | 42 tok/s | ✅ | ✅ | baseline |
| fork-x         | v0.3.1    | ✅ | ❌ | ✅ | ✅ | ❌ | ✅ | ✅ | 51 tok/s | ✅ | ✅ | KV cache regression on context shift |
```
