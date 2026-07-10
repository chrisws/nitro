// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <notcurses/notcurses.h>
#include "llama-sb.h"
#include "input_history.h"
#include "tui_context.h"
#include "input.h"

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
class Tui final: TuiContext {
public:
  Tui();
  virtual ~Tui();

  // ── lifecycle ─────────────────────────────────────────────────────
  void init();
  void resize();
  bool is_escape();
  void clear_chat();
  void setup_model(std::string &model_name, const LlamaMemoryInfo &mem);
  void tick_spinner();
  void set_thinking(bool on);
  void update_usage(int tokens_sec, const LlamaMemoryInfo &mem);
  
  // ── draw ──────────────────────────────────────────────────────────
  void redraw_header() const;
  void redraw_chat();
  void redraw_input() const override;
  void redraw_all();

  // ── content helpers ───────────────────────────────────────────────
  void append_line(const std::string &line);
  void append_token(const std::string &token);
  void flush_token_acc();

  // ── interaction ───────────────────────────────────────────────────
  bool confirm_dialog(const std::string &prompt) const;
  std::string readline() { return input_.readline(*this); }

  // Modal popup overlay while a long operation runs.
  // Call show_modal_popup to display; dismiss_modal_popup to remove.
  // The popup plane is stored in modal_plane; callers hold it as an opaque
  // handle — or just use the paired helpers below.
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

  double get_tokens_per_sec() const { return tokens_per_sec_; }
  double get_kv_percent() const { return kv_percent_; }

  // TuiContext
  InputEvent get_event() override { return InputEvent(nc_); }
  void render() override { notcurses_render(nc_); }

  // ── history ────────────────────────────────────────────────────────
  void history_load(const std::string &path) { input_.load(path); }
  void history_save(const std::string &path) const { input_.save(path); }
  
private:
  // ── notcurses handles ──────────────────────────────────────────────
  struct notcurses *nc_;
  struct ncplane   *stdpl_;
  struct ncplane   *header_;
  struct ncplane   *chatpl_;
  struct ncplane   *inputpl_;
  struct ncplane   *modal_plane_;

  // ── status bar values ─────────────────────────────────────────────
  std::string current_model_;
  float tokens_per_sec_;
  int kv_used_;
  int kv_total_;
  int kv_percent_;
  size_t vram_used_;
  size_t vram_total_;

  // ── dimensions ────────────────────────────────────────────────────
  int term_rows_;
  int term_cols_;
  
  // ── chat buffer ───────────────────────────────────────────────────
  std::vector<std::string> chat_lines_;
  std::mutex lines_mutex_;

  // ── streaming accumulator ─────────────────────────────────────────
  std::string token_acc_;
  
  // ── thinking spinner ──────────────────────────────────────────────
  bool thinking_;
  int spinner_frame_;

  // ── input ─────────────────────────────────────────────────────────
  Input input_;
};
