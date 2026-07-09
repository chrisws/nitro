// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include <notcurses/notcurses.h>
#include "tui.h"

// ─── colour helpers ──────────────────────────────────────────────────────
static constexpr uint32_t BG_CHAT_R = 18,  BG_CHAT_G = 22,  BG_CHAT_B = 30;
static constexpr uint32_t BG_INP_R  = 22,  BG_INP_G  = 28,  BG_INP_B  = 38;
static constexpr uint32_t BG_HDR_R  = 30,  BG_HDR_G  = 40,  BG_HDR_B  = 55;

static inline uint64_t chat_ch(uint32_t r, uint32_t g, uint32_t b) {
  return NCCHANNELS_INITIALIZER(r, g, b, BG_CHAT_R, BG_CHAT_G, BG_CHAT_B);
}

static inline uint64_t inp_ch(uint32_t r, uint32_t g, uint32_t b) {
  return NCCHANNELS_INITIALIZER(r, g, b, BG_INP_R, BG_INP_G, BG_INP_B);
}

static inline uint64_t hdr_ch(uint32_t r, uint32_t g, uint32_t b) {
  return NCCHANNELS_INITIALIZER(r, g, b, BG_HDR_R, BG_HDR_G, BG_HDR_B);
}

//
// Tui::init
//
void Tui::init() {
  notcurses_options opts{};
  opts.flags = NCOPTION_SUPPRESS_BANNERS;
  nc_ = notcurses_init(&opts, nullptr);
  if (!nc_) { std::fputs("notcurses_init failed\n", stderr); std::exit(1); }
  stdpl_ = notcurses_stdplane(nc_);
  notcurses_term_dim_yx(nc_, (unsigned *)&term_rows, (unsigned *)&term_cols);
  uint64_t bg = NCCHANNELS_INITIALIZER(BG_CHAT_R, BG_CHAT_G, BG_CHAT_B,
                                       BG_CHAT_R, BG_CHAT_G, BG_CHAT_B);
  ncplane_set_base(stdpl_, " ", 0, bg);
  ncplane_erase(stdpl_);
  ncplane_options hopt{};
  hopt.y = 0; hopt.x = 0;
  hopt.rows = 1; hopt.cols = (unsigned)term_cols;
  header_ = ncplane_create(stdpl_, &hopt);
  int chat_rows = std::max(1, term_rows - 3);
  ncplane_options copt{};
  copt.y = 1; copt.x = 0;
  copt.rows = (unsigned)chat_rows; copt.cols = (unsigned)term_cols;
  chatpl_ = ncplane_create(stdpl_, &copt);
  ncplane_set_base(chatpl_, " ", 0,
                   NCCHANNELS_INITIALIZER(BG_CHAT_R, BG_CHAT_G, BG_CHAT_B,
                                          BG_CHAT_R, BG_CHAT_G, BG_CHAT_B));
  ncplane_options iopt{};
  iopt.y = term_rows - 2; iopt.x = 0;
  iopt.rows = 2; iopt.cols = (unsigned)term_cols;
  inputpl_ = ncplane_create(stdpl_, &iopt);
  ncplane_set_base(inputpl_, " ", 0,
                   NCCHANNELS_INITIALIZER(BG_INP_R, BG_INP_G, BG_INP_B,
                                          BG_INP_R, BG_INP_G, BG_INP_B));
  notcurses_mice_enable(nc_, NCMICE_BUTTON_EVENT);
  redraw_all();
}

void Tui::destroy() {
  if (nc_) {
    notcurses_stop(nc_);
    nc_ = nullptr;
  }
}

void Tui::resize() {
  unsigned rows = 0, cols = 0;
  notcurses_stddim_yx(nc_, &rows, &cols);
  if ((int)rows != term_rows || (int)cols != term_cols) {
    notcurses_term_dim_yx(nc_, (unsigned *)&term_rows, (unsigned *)&term_cols);
    ncplane_resize_simple(header_, 1, (unsigned)term_cols);
    int cr = std::max(1, term_rows - 3);
    ncplane_resize_simple(chatpl_, (unsigned)cr, (unsigned)term_cols);
    ncplane_move_yx(inputpl_, term_rows - 2, 0);
    ncplane_resize_simple(inputpl_, 2, (unsigned)term_cols);
    redraw_all();
  }
}

//
// Tui::redraw
//
void Tui::redraw_header() const {
  ncplane_erase(header_);
  ncplane_set_base(header_, " ", 0,
                   NCCHANNELS_INITIALIZER(BG_HDR_R, BG_HDR_G, BG_HDR_B,
                                          BG_HDR_R, BG_HDR_G, BG_HDR_B));
  float kv_pct   = kv_total   > 0 ? 100.f * (float)kv_used   / (float)kv_total   : 0.f;
  float vram_pct = vram_total  > 0 ? 100.f * (float)vram_used / (float)vram_total : 0.f;

  static const char *const SPIN[] = { "⣾","⣽","⣻","⢿","⡿","⣟","⣯","⣷" };
  const char *spin_str = thinking ? SPIN[spinner_frame % 8] : " ";
  char buf[512];
  int n = std::snprintf(buf, sizeof(buf),
                        " ✦ NITRO  │ %-32s │ %5.1f tok/s │ KV %4.1f%%  VRAM %4.1f%%  %s",
                        current_model.c_str(), (double)tokens_per_sec,
                        (double)kv_pct, (double)vram_pct, spin_str);
  if (n > term_cols) buf[term_cols] = '\0';
  ncplane_set_channels(header_, hdr_ch(130, 220, 200));
  ncplane_putstr_yx(header_, 0, 0, buf);
}

void Tui::redraw_chat() {
  ncplane_erase(chatpl_);
  unsigned rows, cols;
  ncplane_dim_yx(chatpl_, &rows, &cols);
  std::lock_guard<std::mutex> lk(lines_mutex);

  // Pre-compute wrapped lines so we know total visual rows.
  struct VisualLine {
    std::string text;
    uint64_t    ch;
  };
  std::vector<VisualLine> visual;
  visual.reserve(chat_lines.size());

  for (const std::string &line : chat_lines) {
    uint64_t ch;
    if (line.rfind("[logo_", 0) == 0 && line.size() > 7 && line[7] == ']') {
      int logo_row = line[6] - '0';
      static const uint32_t GRAD_R[] = {  0,  20,  60, 120, 180, 210, 220 };
      static const uint32_t GRAD_G[] = { 230, 255, 255, 255, 200, 130,  80 };
      static const uint32_t GRAD_B[] = { 255, 200, 140,  80, 100, 200, 255 };
      int gi = std::max(0, std::min(logo_row, 6));
      ch = chat_ch(GRAD_R[gi], GRAD_G[gi], GRAD_B[gi]);
    }
    else if (line.rfind("You: ",    0) == 0) ch = chat_ch(100, 200, 255);
    else if (line.rfind("Nitro: ",  0) == 0) ch = chat_ch(180, 255, 180);
    else if (line.rfind(ICON_SYS,   0) == 0) ch = chat_ch(160,  82,  45);
    else if (line.rfind(ICON_TOOL,  0) == 0) ch = chat_ch(255, 180,  80);
    else if (line.rfind(ICON_ERR,   0) == 0) ch = chat_ch(255,  80,  80);
    else if (line.rfind(ICON_THINK, 0) == 0) ch = chat_ch(140, 140, 200);
    else                                     ch = chat_ch(210, 210, 210);

    std::string display = (line.rfind("[logo_", 0) == 0 && line.size() > 8)
      ? line.substr(8) : line;

    // Split into cols-wide chunks, each carrying the same channel.
    if (display.empty()) {
      visual.push_back({"", ch});
    } else {
      for (size_t off = 0; off < display.size(); off += cols) {
        visual.push_back({display.substr(off, cols), ch});
      }
    }
  }

  int total   = static_cast<int>(visual.size());
  int visible = static_cast<int>(rows);
  int start   = std::max(0, total - visible - scroll_offset);
  int end     = std::min(total, start + visible);

  for (int i = start, row = 0; i < end; ++i, ++row) {
    ncplane_set_channels(chatpl_, visual[i].ch);
    ncplane_putstr_yx(chatpl_, row, 0, visual[i].text.c_str());
  }
}

void Tui::redraw_input() const {
  ncplane_erase(inputpl_);

  if (thinking) {
    static constexpr const char *BLOCKS[] = { "-", "~", "≈", "~", "-" };
    static constexpr int    N_BLOCKS = 5;
    static constexpr double FREQ     = 0.25;  // gentler wave
    static constexpr double SPEED    = 0.15;  // slower scroll
    static constexpr int    DELAY    = 12;    // frames before animation starts

    if (spinner_frame < DELAY) {
      // still just a plain separator during the pause
      ncplane_set_channels(inputpl_, inp_ch(80, 120, 160));
      std::string sep(term_cols, '-');
      ncplane_putstr_yx(inputpl_, 0, 0, sep.c_str());
    } else {
      int frame = spinner_frame - DELAY;  // animation frame relative to start
      for (int col = 0; col < term_cols; ++col) {
        double phase = (col * FREQ) - (frame * SPEED);
        int idx = static_cast<int>(((std::sin(phase) + 1.0) * 0.5 * (N_BLOCKS - 1)));
        idx = std::max(0, std::min(idx, N_BLOCKS - 1));
        // subtle brightness shift — blue-grey, not full glow
        int brightness = 80 + idx * 20;
        ncplane_set_channels(inputpl_, NCCHANNELS_INITIALIZER(brightness, brightness + 20, brightness + 40,
                                                             BG_INP_R, BG_INP_G, BG_INP_B));
        ncplane_putstr_yx(inputpl_, 0, col, BLOCKS[idx]);
      }
    }
    ncplane_set_channels(inputpl_, inp_ch(140, 140, 180));
    ncplane_putstr_yx(inputpl_, 1, 2, "thinking…");
  } else {
    ncplane_set_channels(inputpl_, inp_ch(80, 120, 160));
    std::string sep(term_cols, '-');
    ncplane_putstr_yx(inputpl_, 0, 0, sep.c_str());
    const std::string prompt = " ❯ ";
    const int prompt_cols = 4;
    ncplane_set_channels(inputpl_, inp_ch(100, 210, 255));
    ncplane_putstr_yx(inputpl_, 1, 0, prompt.c_str());
    int max_w = std::max(0, term_cols - prompt_cols - 1);
    std::string visible = input_buf;
    int view_offset = 0;
    if (visible.size() > max_w && max_w > 0) {
      view_offset = static_cast<int>(visible.size() - max_w);
      visible = visible.substr(view_offset);
    }
    int cur_in_view = std::max(0, static_cast<int>(cursor_pos - view_offset));
    cur_in_view = std::min(cur_in_view, (int)visible.size());
    std::string before = visible.substr(0, cur_in_view);
    std::string after  = cur_in_view < (int)visible.size()
      ? visible.substr(cur_in_view + 1) : "";
    char cursor_ch_val = cur_in_view < (int)visible.size()
      ? visible[cur_in_view] : ' ';
    ncplane_set_channels(inputpl_, inp_ch(230, 230, 230));
    ncplane_putstr_yx(inputpl_, 1, prompt_cols, before.c_str());
    int cx = prompt_cols + cur_in_view;
    ncplane_set_channels(inputpl_, NCCHANNELS_INITIALIZER(BG_INP_R, BG_INP_G, BG_INP_B, 180, 230, 255));
    char cbuf[2] = { cursor_ch_val, '\0' };
    ncplane_putstr_yx(inputpl_, 1, cx, cbuf);
    ncplane_set_channels(inputpl_, inp_ch(230, 230, 230));
    if (!after.empty()) {
      ncplane_putstr_yx(inputpl_, 1, cx + 1, after.c_str());
    }
  }
}

void Tui::redraw_all() {
  redraw_header();
  redraw_chat();
  redraw_input();
  notcurses_render(nc_);
}

void Tui::tick_spinner() {
  ++spinner_frame;
  redraw_header();
  redraw_input();
  notcurses_render(nc_);
}

void Tui::set_thinking(bool on) {
  thinking = on;
  if (!on) spinner_frame = 0;
  redraw_header();
  redraw_input();
  notcurses_render(nc_);
}

void Tui::update_usage(int tokens_sec, const LlamaMemoryInfo &mem) {
  tokens_per_sec = tokens_sec;
  kv_used    = mem.kv_used;
  kv_total   = mem.kv_total;
  kv_percent = mem.kv_percent;
  vram_used  = mem.vram_used;
  vram_total = mem.vram_total;
}

//
// TuiState content helpers
//
void Tui::append_line(const std::string &line) {
  std::lock_guard<std::mutex> lk(lines_mutex);
  int w = std::max(1, term_cols - 1);
  if ((int)line.size() <= w) {
    chat_lines.push_back(line);
  } else {
    for (int off = 0; off < (int)line.size(); off += w) {
      chat_lines.push_back(line.substr(off, w));
    }
  }
}

void Tui::append_token(const std::string &token) {
  token_acc += token;
  for (;;) {
    auto pos = token_acc.find('\n');
    if (pos == std::string::npos) {
      break;
    }
    append_line(token_acc.substr(0, pos));
    token_acc = token_acc.substr(pos + 1);
  }
  redraw_chat();
  notcurses_render(nc_);
}

void Tui::flush_token_acc() {
  if (!token_acc.empty()) {
    append_line(token_acc);
    token_acc.clear();
    redraw_chat();
    notcurses_render(nc_);
  }
}

//
// Creates a centred floating plane with a border and a status message.
// The popup sits above all other planes and blocks until explicitly dismissed.
//
void Tui::show_modal_popup(const std::string &message) {
  // Dismiss any previous popup first.
  dismiss_modal_popup();

  // Clamp popup size to terminal.
  int popup_w = std::min((int)message.size() + 8, term_cols - 4);
  popup_w = std::max(popup_w, 20);
  int popup_h = 5;
  int py = std::max(0, (term_rows - popup_h) / 2);
  int px = std::max(0, (term_cols - popup_w) / 2);

  ncplane_options opts{};
  opts.y    = py; opts.x    = px;
  opts.rows = (unsigned)popup_h;
  opts.cols = (unsigned)popup_w;
  modal_plane_ = ncplane_create(stdpl_, &opts);
  if (!modal_plane_) return;

  // Background: deep navy.
  static constexpr uint32_t PBG_R = 20, PBG_G = 28, PBG_B = 50;
  ncplane_set_base(modal_plane_, " ", 0,
                   NCCHANNELS_INITIALIZER(PBG_R, PBG_G, PBG_B, PBG_R, PBG_G, PBG_B));
  ncplane_erase(modal_plane_);

  // Border — bright cyan.
  uint64_t border_ch = NCCHANNELS_INITIALIZER(80, 220, 255, PBG_R, PBG_G, PBG_B);
  ncplane_set_channels(modal_plane_, border_ch);

  // Draw corners and edges manually so we don't require nccell border helpers.
  // Top row
  ncplane_putstr_yx(modal_plane_, 0, 0, "╔");
  for (int c = 1; c < popup_w - 1; ++c)
    ncplane_putstr_yx(modal_plane_, 0, c, "═");
  ncplane_putstr_yx(modal_plane_, 0, popup_w - 1, "╗");
  // Middle rows
  for (int r = 1; r < popup_h - 1; ++r) {
    ncplane_putstr_yx(modal_plane_, r, 0, "║");
    ncplane_putstr_yx(modal_plane_, r, popup_w - 1, "║");
  }
  // Bottom row
  ncplane_putstr_yx(modal_plane_, popup_h - 1, 0, "╚");
  for (int c = 1; c < popup_w - 1; ++c)
    ncplane_putstr_yx(modal_plane_, popup_h - 1, c, "═");
  ncplane_putstr_yx(modal_plane_, popup_h - 1, popup_w - 1, "╝");

  // Title bar.
  uint64_t title_ch = NCCHANNELS_INITIALIZER(255, 220, 80, PBG_R, PBG_G, PBG_B);
  ncplane_set_channels(modal_plane_, title_ch);
  ncplane_putstr_yx(modal_plane_, 1, 2, "⏳ Loading…");

  // Message.
  uint64_t msg_ch = NCCHANNELS_INITIALIZER(200, 200, 200, PBG_R, PBG_G, PBG_B);
  ncplane_set_channels(modal_plane_, msg_ch);
  // Truncate message to fit inside border.
  int max_msg = popup_w - 4;
  std::string display = message.size() > (size_t)max_msg
    ? message.substr(0, max_msg)
    : message;
  ncplane_putstr_yx(modal_plane_, 2, 2, display.c_str());

  notcurses_render(nc_);
}

void Tui::show_help() {
  append_line(ICON_SYS + "Commands:");
  append_line(ICON_SYS + "  /model  [path]           load a GGUF model (picker if no path)");
  append_line(ICON_SYS + "  /embed  [path]           load an embedding model (picker if no path)");
  append_line(ICON_SYS + "  /rag    [path]           index file or directory (picker if no path)");
  append_line(ICON_SYS + "  /memory                  KV / VRAM / layer stats");
  append_line(ICON_SYS + "  /clear                   reset conversation");
  append_line(ICON_SYS + "  /settings                show current settings");
  append_line(ICON_SYS + "  /set    <key> <value>    change a setting live");
  append_line(ICON_SYS + "  /help                    this message");
  append_line(ICON_SYS + "  exit / quit              exit Nitro");
  append_line(ICON_SYS + "Settable keys (via /set):");
  append_line(ICON_SYS + "  temperature  top_p  top_k  min_p  penalty_repeat");
  append_line(ICON_SYS + "  penalty_last_n  rag_top_k  n_gpu_layers");
  append_line(ICON_SYS + "  run_allowed  (comma-separated list, e.g. python3,make)");
  redraw_all();
}

void Tui::dismiss_modal_popup() {
  if (modal_plane_) {
    ncplane_destroy(modal_plane_);
    modal_plane_ = nullptr;
    notcurses_render(nc_);
  }
}

//
// ─── Tui::file_picker ────────────────────────────────────────────────
// Interactive directory/file browser popup.
// Keyboard:  ↑/↓ navigate,  Enter select/descend,  Backspace go up,
//            's' select current dir for indexing,   Esc cancel.
// Returns the chosen path or "" on cancel.
// ─── Tui::file_picker ────────────────────────────────────────────────
// Unified interactive directory/file browser used by /rag, /model, /embed.
// title_hint appears in the popup header (e.g. "RAG Folder", "Model File").
//
// Keyboard:
//   ↑/↓        navigate list
//   Enter      descend into directory, or select a file
//   Backspace  go up one directory
//   s          select the current directory itself (useful for /rag)
//   Esc        cancel → returns ""
//
// Returns the chosen path, or "" on cancel.
//
std::string Tui::file_picker(const std::string &start_dir,
                                  const std::string &title_hint) const {
  std::string current_dir = start_dir;
  {
    std::error_code ec;
    auto canon = fs::canonical(start_dir, ec);
    if (!ec) current_dir = canon.string();
  }
  auto load_entries = [](const std::string &dir,
                         std::vector<std::string> &entries)
  {
    entries.clear();
    std::error_code ec;
    if (fs::path(dir).has_parent_path() &&
        fs::path(dir) != fs::path(dir).root_path())
      entries.emplace_back("..");
    std::vector<std::string> dirs, files;
    for (const auto &e : fs::directory_iterator(dir, ec)) {
      if (ec) break;
      std::string name = e.path().filename().string();
      if (name.empty() || name[0] == '.') continue;
      if (e.is_directory()) dirs.push_back(name);
      else                  files.push_back(name);
    }
    ranges::sort(dirs);
    ranges::sort(files);
    for (auto &d : dirs)  entries.push_back(d + "/");
    for (auto &f : files) entries.push_back(f);
  };

  std::vector<std::string> entries;
  int selected = 0;
  int scroll   = 0;

  // Popup dimensions.
  static constexpr int PW = 60;
  static constexpr int PH = 20;
  int py = std::max(0, (term_rows - PH) / 2);
  int px = std::max(0, (term_cols - PW) / 2);

  ncplane_options opts{};
  opts.y = py; opts.x = px;
  opts.rows = (unsigned)PH; opts.cols = (unsigned)PW;
  struct ncplane *picker = ncplane_create(stdpl_, &opts);
  if (!picker) return "";

  static constexpr uint32_t PBG_R = 18, PBG_G = 24, PBG_B = 40;
  ncplane_set_base(picker, " ", 0, NCCHANNELS_INITIALIZER(PBG_R, PBG_G, PBG_B, PBG_R, PBG_G, PBG_B));
  // Build a compact hint line appropriate to the operation.
  // /rag adds 's=select dir'; /model and /embed only need file selection.
  std::string hint_line = "↑↓ navigate  Enter open/select  Esc cancel";
  if (title_hint.find("RAG") != std::string::npos ||
      title_hint.find("Folder") != std::string::npos) {
    hint_line = "↑↓ navigate  Enter open  s=select dir  Esc cancel";
  }
  auto draw_picker = [&]() {
    ncplane_erase(picker);
    uint64_t border_ch = NCCHANNELS_INITIALIZER(100, 180, 255, PBG_R, PBG_G, PBG_B);
    ncplane_set_channels(picker, border_ch);
    ncplane_putstr_yx(picker, 0, 0, "╔");
    for (int c = 1; c < PW - 1; ++c) ncplane_putstr_yx(picker, 0, c, "═");
    ncplane_putstr_yx(picker, 0, PW - 1, "╗");
    for (int r = 1; r < PH - 1; ++r) {
      ncplane_putstr_yx(picker, r, 0,      "║");
      ncplane_putstr_yx(picker, r, PW - 1, "║");
    }
    ncplane_putstr_yx(picker, PH - 1, 0, "╚");
    for (int c = 1; c < PW - 1; ++c) ncplane_putstr_yx(picker, PH - 1, c, "═");
    ncplane_putstr_yx(picker, PH - 1, PW - 1, "╝");

    // Title
    ncplane_set_channels(picker, NCCHANNELS_INITIALIZER(255, 220, 80, PBG_R, PBG_G, PBG_B));
    std::string title_str = " 📂 " + title_hint + " Picker ";
    if ((int)title_str.size() > PW - 4) title_str = title_str.substr(0, PW - 4);
    ncplane_putstr_yx(picker, 0, 2, title_str.c_str());
    // Current path (truncated).
    std::string path_display = current_dir;
    if ((int)path_display.size() > PW - 4)
      path_display = "…" + path_display.substr(path_display.size() - (PW - 5));
    ncplane_set_channels(picker, NCCHANNELS_INITIALIZER(160, 200, 240, PBG_R, PBG_G, PBG_B));
    ncplane_putstr_yx(picker, 1, 2, path_display.c_str());
    // Hint line (bottom interior row).
    ncplane_set_channels(picker, NCCHANNELS_INITIALIZER(120, 120, 160, PBG_R, PBG_G, PBG_B));
    std::string hint_trunc = hint_line;
    if ((int)hint_trunc.size() > PW - 4) hint_trunc = hint_trunc.substr(0, PW - 4);
    ncplane_putstr_yx(picker, PH - 2, 2, hint_trunc.c_str());
    // Entry list.
    int list_rows = PH - 5;
    if (selected < scroll) scroll = selected;
    if (selected >= scroll + list_rows) scroll = selected - list_rows + 1;
    for (int i = 0; i < list_rows; ++i) {
      int idx = scroll + i;
      if (idx >= (int)entries.size()) break;
      bool is_selected = (idx == selected);
      bool is_dir = !entries[idx].empty() && entries[idx].back() == '/';
      uint32_t fr, fg, fb;
      if (is_selected)  { fr = 20;  fg = 20;  fb = 20;  }
      else if (is_dir)   { fr = 120; fg = 200; fb = 255; }
      else               { fr = 200; fg = 200; fb = 200; }
      uint32_t br = is_selected ? 100 : PBG_R;
      uint32_t bg = is_selected ? 180 : PBG_G;
      uint32_t bb = is_selected ? 255 : PBG_B;
      ncplane_set_channels(picker, NCCHANNELS_INITIALIZER(fr, fg, fb, br, bg, bb));
      std::string label = (is_selected ? " ▶ " : "   ") + entries[idx];
      if ((int)label.size() > PW - 2) label = label.substr(0, PW - 2);
      while ((int)label.size() < PW - 2) label += ' ';
      ncplane_putstr_yx(picker, 2 + i, 1, label.c_str());
    }
    notcurses_render(nc_);
  };

  std::string result;
  load_entries(current_dir, entries);
  draw_picker();

  for (;;) {
    ncinput ni{};
    notcurses_get_blocking(nc_, &ni);
    if (ni.id == NCKEY_ESC) {
      break;  // cancelled
    }
    if (ni.id == NCKEY_UP) {
      if (selected > 0) --selected;
      draw_picker();
      continue;
    }
    if (ni.id == NCKEY_DOWN) {
      if (selected + 1 < (int)entries.size()) ++selected;
      draw_picker();
      continue;
    }
    // 's' — select the current directory (useful for /rag, ignored for file pickers).
    if (ni.id == 's' || ni.id == 'S') {
      // Select current directory for RAG indexing.
      result = current_dir;
      break;
    }
    if (ni.id == NCKEY_BACKSPACE || ni.id == 127) {
      // Go up one level.
      fs::path p(current_dir);
      if (p.has_parent_path() && p != p.root_path()) {
        current_dir = p.parent_path().string();
        load_entries(current_dir, entries);
        selected = 0; scroll = 0;
        draw_picker();
      }
      continue;
    }
    if (ni.id == NCKEY_ENTER || ni.id == '\r' || ni.id == '\n') {
      if (entries.empty()) continue;
      const std::string &entry = entries[selected];
      if (entry == "..") {
        fs::path p(current_dir);
        if (p.has_parent_path() && p != p.root_path()) {
          current_dir = p.parent_path().string();
          load_entries(current_dir, entries);
          selected = 0; scroll = 0;
          draw_picker();
        }
      } else if (!entry.empty() && entry.back() == '/') {
        // Descend into directory.
        current_dir = current_dir + "/" + entry.substr(0, entry.size() - 1);
        {
          std::error_code ec;
          auto canon = fs::canonical(current_dir, ec);
          if (!ec) current_dir = canon.string();
        }
        load_entries(current_dir, entries);
        selected = 0; scroll = 0;
        draw_picker();
      } else {
        // Select a specific file.
        // Select the highlighted file.
        result = current_dir + "/" + entry;
        break;
      }
      continue;
    }
  }
  ncplane_destroy(picker);
  notcurses_render(nc_);
  return result;
}

//
// ─── Tui::confirm_dialog ─────────────────────────────────────────────
//
bool Tui::confirm_dialog(const std::string &prompt) const {
  ncplane_erase(inputpl_);
  ncplane_set_channels(inputpl_, inp_ch(255, 200, 80));
  std::string msg = " " + prompt + " [y/n] ❯ ";
  ncplane_putstr_yx(inputpl_, 1, 0, msg.c_str());
  notcurses_render(nc_);
  std::string answer;
  for (;;) {
    ncinput ni{};
    notcurses_get_blocking(nc_, &ni);
    if (ni.id == NCKEY_ENTER || ni.id == '\r' || ni.id == '\n') break;
    if (ni.id == NCKEY_BACKSPACE && !answer.empty()) { answer.pop_back(); }
    else if (ni.id >= 32 && ni.id < 127) { answer += (char)ni.id; }
    ncplane_erase(inputpl_);
    ncplane_set_channels(inputpl_, inp_ch(255, 200, 80));
    ncplane_putstr_yx(inputpl_, 1, 0, (msg + answer).c_str());
    notcurses_render(nc_);
  }
  std::string lo = answer;
  ranges::transform(lo, lo.begin(), ::tolower);
  redraw_input();
  notcurses_render(nc_);
  return (lo == "y" || lo == "yes" || lo == "sure" || lo == "k");
}

//
// Integrates InputHistory:  Up/Down arrows navigate the history stack.
// On submit the entry is pushed to history, and nav is reset.
//
std::string Tui::readline_blocking() {
  input_buf.clear();
  cursor_pos = 0;
  history.reset_nav();
  redraw_input();
  notcurses_render(nc_);

  // Temporary saved draft so Down from history restores the user's current text.
  std::string draft;

  for (;;) {
    ncinput ni{};
    notcurses_get_blocking(nc_, &ni);

    if (ni.id == NCKEY_ENTER || ni.id == '\r' || ni.id == '\n') {
      std::string result = input_buf;
      if (!result.empty()) {
        history.push(result);
      }
      input_buf.clear();
      cursor_pos = 0;
      scroll_offset = 0;
      redraw_input();
      notcurses_render(nc_);
      return result;
    }

    if (ni.id == NCKEY_UP) {
      // Entering history from a fresh prompt: save current text as draft.
      std::string hist_entry;
      if (history.up(hist_entry)) {
        if (!input_buf.empty() && hist_entry != input_buf) {
          // Only save draft when we first leave the bottom of history.
          // (history.reset_nav was called on entry so the first Up call
          //  always comes from the "new input" position.)
          draft = input_buf;
        }
        input_buf  = hist_entry;
        cursor_pos = input_buf.size();
      }
      redraw_input();
      notcurses_render(nc_);
      continue;
    }

    if (ni.id == NCKEY_DOWN) {
      std::string hist_entry;
      if (history.down(hist_entry)) {
        input_buf  = hist_entry;
        cursor_pos = input_buf.size();
      } else {
        // Past the newest entry → restore draft.
        input_buf  = draft;
        cursor_pos = input_buf.size();
        draft.clear();
      }
      redraw_input();
      notcurses_render(nc_);
      continue;
    }

    // Scroll the chat pane — not the input history.
    if (ni.id == NCKEY_PGUP) {
      scroll_offset += std::max(1, term_rows - 4);
      redraw_chat();
      notcurses_render(nc_);
      continue;
    }
    if (ni.id == NCKEY_PGDOWN) {
      scroll_offset = std::max(0, scroll_offset - std::max(1, term_rows - 4));
      redraw_chat();
      notcurses_render(nc_);
      continue;
    }
    if (ni.id == NCKEY_SCROLL_UP && scroll_offset < term_rows + 10) {
      scroll_offset += 1;
      redraw_chat();
      notcurses_render(nc_);
      continue;
    }
    if (ni.id == NCKEY_SCROLL_DOWN && scroll_offset > 0) {
      scroll_offset -= 1;
      redraw_chat();
      notcurses_render(nc_);
      continue;
    }
    if (ni.id == NCKEY_F01) {
      show_help();
      continue;
    }
    if (ni.id == NCKEY_F02) {
      mouse_mode = !mouse_mode;
      if (mouse_mode) {
        notcurses_mice_enable(nc_, NCMICE_BUTTON_EVENT);
      } else {
        notcurses_mice_disable(nc_);
      }
      continue;
    }
    if (ni.id == NCKEY_BACKSPACE || ni.id == 127) {
      if (cursor_pos > 0) { input_buf.erase(cursor_pos - 1, 1); --cursor_pos; }
    } else if (ni.id == NCKEY_LEFT) {
      if (cursor_pos > 0) --cursor_pos;
    } else if (ni.id == NCKEY_RIGHT) {
      if (cursor_pos < input_buf.size()) ++cursor_pos;
    } else if (ni.id == NCKEY_HOME) {
      cursor_pos = 0;
    } else if (ni.id == NCKEY_END) {
      cursor_pos = input_buf.size();
    } else if (ni.id == NCKEY_DEL) {
      if (cursor_pos < input_buf.size()) input_buf.erase(cursor_pos, 1);
    } else if (ni.id >= 32 && ni.id < 0xD800) {
      // Any printable character — entering new text clears the nav draft
      // so that Down won't resurrect a stale saved buffer.
      draft.clear();
      history.reset_nav();
      input_buf.insert(cursor_pos, 1, (char)ni.id);
      ++cursor_pos;
    }

    redraw_input();
    notcurses_render(nc_);
  }
}

bool Tui::is_escape() {
  ncinput ni{};
  notcurses_get_nblock(nc_, &ni);
  if (ni.id == NCKEY_ESC) {
    set_thinking(false);
    append_line(ICON_ERR + "Generation cancelled by user (Escape)");
    redraw_all();
  }
  return ni.id == NCKEY_ESC;
}

void Tui::clear_chat() {
  std::lock_guard<std::mutex> lk(lines_mutex);
  chat_lines.clear();
}

void Tui::setup_model(std::string &model_name, const LlamaMemoryInfo &mem) {
  update_usage(0.0f, mem);
  current_model = model_name;
  append_line(ICON_SYS + "Model ready: " + current_model);
  append_line(ICON_SYS + "" + mem.advice);
  append_line(ICON_SYS + "Thinking mode: " + (thinking ? "enabled" : "disabled"));
  redraw_all();
}
