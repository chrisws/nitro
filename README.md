# Nitro

**A standalone, agentic LLM shell for your terminal.**

Nitro is a local-first agentic coding/chat shell built on [llama.cpp](https://github.com/ggml-org/llama.cpp), rendered with [notcurses](https://github.com/dankamongmen/notcurses). No server, no browser tab, no cloud dependency — just a fast terminal UI driving a local model with tool use, RAG, and careful context management.

```
┌─ nitro ──────────────────────────────────────────────────────┐
│ > summarize the diff in src/kv_cache.cpp                     │
│                                                                │
│ [TOOL:CURL] ...                                               │
│ The change replaces per-sequence llama_memory_seq_rm calls    │
│ with a full_flush_except_system() recovery path to avoid...   │
│                                                                │
│ >                                                              │
└────────────────────────────────────────────────────────────────┘
```

## Why

Most agentic shells assume a hosted API and treat context as free. Nitro assumes neither: it's built for consumer GPUs (developed against an 8GB RTX 5060) where the KV cache is a scarce resource and every tool call has to earn its place in the context window.

## Features

- **notcurses TUI** — plane-based rendering, modal popups, persistent input history (Up/Down navigation), live `/set` commands for generation parameters.
- **Fragmentation-aware KV cache management** — `full_flush_except_system()` as a graceful recovery path when sequence removal fragments the cache instead of compacting it; a `KVCachePreset` enum (`F16` / `Balanced` / `Compact`) coupled to flash-attention settings.
- **Dynamic tool-result budgeting** — `max_tool_result_size()` targets ~75% of remaining context so a single large tool result can't blow the budget.
- **Unambiguous tool-call protocol** — an explicit `NITRO_END_TOOL` terminator so tool boundaries never get confused with model chatter.
- **Built-in tools** — `TOOL:CURL` (via libcurl, with HTML stripping) and an allowlisted `run_allowed` command runner.
- **Pure C++ RAG pipeline** — semantic chunker, binary `.db` index, deduplicating `RagSession`, with a folder picker for building indexes on the fly.
- **Persistent settings** — configuration lives in `~/.config/nitro.settings.json`.

## Building

Nitro vendors llama.cpp as a submodule and links everything statically.

```bash
git clone --recurse-submodules https://github.com/<you>/nitro.git
cd nitro
cmake -B build -DLLAMA_BACKEND=AUTO
cmake --build build -j
./build/bin/nitro
```

### Backend selection

| `-DLLAMA_BACKEND=` | Behavior |
|---|---|
| `AUTO` (default) | Probes for `nvcc`; falls back to CPU + OpenMP if CUDA isn't found |
| `CPU` | Forces CPU with native SIMD optimizations |
| `GPU` | Non-CUDA GPU path, CPU/OpenMP fallback |
| `CUDA` | Forces CUDA; fails the configure step if `nvcc` isn't found |

### Dependencies

| Library | Required | Notes |
|---|---|---|
| notcurses | Yes | `apt install libnotcurses-dev`, or `-DNOTCURSES_DIR=<prefix>` |
| libcurl | No | Enables `TOOL:CURL`; `apt install libcurl4-openssl-dev`, or `-DCURL_DIR=<prefix>` |
| CUDA toolkit | No | Only for `-DLLAMA_BACKEND=CUDA`/`AUTO` with an NVIDIA GPU: `apt install nvidia-open cuda-toolkit` |

## Project layout

```
nitro/
├── CMakeLists.txt
├── src/
│   ├── nitro.cpp              # entry point, TUI event loop
│   ├── llama-sb.h/.cpp        # llama.cpp wrapper
│   └── llama-sb-rag.cpp       # RAG session, chunker, indexer
├── include/                   # vendored utility sources (param, hashmap, apiexec)
└── third_party/
    └── llama.cpp               # submodule
```

## Status

Nitro started life as a component inside a larger SmallBASIC-adjacent monorepo and is being split out here as a standalone project. Expect some rough edges from the extraction (see `MIGRATION.md`) while paths and includes settle.

## Roadmap

- [ ] Drop the `include/` vendor copies in favor of a proper shared-utils dependency (or absorb them outright)
- [ ] CI build matrix (CPU / CUDA)
- [ ] Package a release binary

## License

MIT — see `LICENSE`. *(Swap this out if you want something else; not yet confirmed.)*
