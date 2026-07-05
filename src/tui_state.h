// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include "llama-sb.h"
#include "llama-sb-rag.h"
#include "input_history.h"

//
// icons
//
constexpr std::string ICON_ERR   = " ⚡ ▏";
constexpr std::string ICON_THINK = " 🤔 ▏";
constexpr std::string ICON_TOOL  = " 🔧 ▏";
constexpr std::string ICON_SYS   = " ✨ ▏";

//
// Notcurses TUI
//
//
//  ┌──────────────────── header (1 row) ─────────────────────────────────┐
//  │ ✦ NITRO  model: …  tok/s: …  KV: …%  VRAM: …%                       │
//  ├─────────────────────────────────────────────────────────────────────┤
//  │                                                                     │
//  │  chat pane  (rows 1 … term_rows-3)                                  │
//  │                                                                     │
//  ├─────────────────────────────────────────────────────────────────────┤
//  │ ─────────────────────────────────────  (separator)                  │
//  │ ❯ input                                                             │
//  └─────────────────────────────────────────────────────────────────────┘
struct TuiState {
  // ── notcurses handles ──────────────────────────────────────────────
  struct notcurses *nc      = nullptr;
  struct ncplane   *stdpl   = nullptr;
  struct ncplane   *header  = nullptr;
  struct ncplane   *chatpl  = nullptr;
  struct ncplane   *inputpl = nullptr;
  // ── chat buffer ───────────────────────────────────────────────────
  std::vector<std::string> chat_lines;
  int scroll_offset = 0;
  std::mutex lines_mutex;
  // ── streaming accumulator ─────────────────────────────────────────
  std::string token_acc;
  // ── input ─────────────────────────────────────────────────────────
  std::string input_buf;
  size_t      cursor_pos = 0;
  bool        mouse_mode = true;
  // ── status bar values ─────────────────────────────────────────────
  std::string current_model  = "none";
  float       tokens_per_sec = 0.0f;
  int         kv_used        = 0;
  int         kv_total       = 1;
  int         kv_percent     = 0;
  size_t      vram_used      = 0;
  size_t      vram_total     = 1;
  int term_rows = 0;
  int term_cols = 0;
  // ── thinking spinner ──────────────────────────────────────────────
  bool    thinking      = false;
  int     spinner_frame = 0;
  // ── input history ─────────────────────────────────────────────────
  InputHistory history;
  // Advance spinner by one frame and redraw the header.
  void tick_spinner();

  // Toggle thinking mode; redraws header immediately.
  void set_thinking(bool on);
  void update_usage(int tokens_sec, const LlamaMemoryInfo &mem);

  // ── lifecycle ─────────────────────────────────────────────────────
  void init();
  void destroy();
  void resize();
  // ── draw ──────────────────────────────────────────────────────────
  void redraw_header() const;
  void redraw_chat();
  void redraw_input() const;
  void redraw_all();
  // ── content helpers ───────────────────────────────────────────────
  void append_line(const std::string &line);
  void append_token(const std::string &token);
  void flush_token_acc();
  // ── interaction ───────────────────────────────────────────────────
  bool confirm_dialog(const std::string &prompt) const;
  // Blocking readline with history navigation, cursor, arrow-key scrolling.
  std::string readline_blocking();
  // Modal popup overlay while a long operation runs.
  // Call show_modal_popup to display; dismiss_modal_popup to remove.
  // The popup plane is stored in modal_plane; callers hold it as an opaque
  // handle — or just use the paired helpers below.
  struct ncplane *modal_plane = nullptr;
  void show_modal_popup(const std::string &message);
  void show_help();
  void dismiss_modal_popup();
  // ── folder picker popup ───────────────────────────────────────
  // Presents an interactive directory browser to let the user choose a
  // folder (or file) to index.  Returns the selected path, or empty string
  // if the user cancelled.
  // ── file browser popup ─────────────────────────────────────
  // Used by /rag, /model, and /embed to pick a path interactively.
  // Pass a hint string shown in the title bar (e.g. "RAG Folder",
  // "Model File", "Embedding Model").
  // Returns the selected path, or empty string if the user cancelled.
  std::string file_picker(const std::string &start_dir,
                          const std::string &title_hint = "File") const;
  // Legacy alias kept for callers that used the old name.
  std::string rag_folder_picker(const std::string &start_dir) const {
    return file_picker(start_dir, "RAG Folder");
  }
};
