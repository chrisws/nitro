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
#include <memory>

#include <notcurses/notcurses.h>
#include "llama-sb.h"
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
// Color theme system ───────────────────────────────────────────────────
// Solarized-inspired palette with Dark and Light themes
//
namespace Color {
  struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };

  enum class ColorElement {
    // UI components
    BUTTON,
    BUTTON_BACKGROUND,
    BUTTON_FOCUS,
    BUTTON_INACTIVE,
    COMPONENT,
    COMPONENT_BACKGROUND_FOCUS,
    COMPONENT_FOCUS,
    LABEL,
    PLAYHEAD,
    ACTIVE,
    INACTIVE,
    SLIDER,
    SLIDER_BASE,
    SLIDER_KNOB,
    TEXT,
    TEXT_FOCUS,
    WAVEFORM,
    RECORD,

    // TUI-specific
    CHAT_BACKGROUND,
    INPUT_BACKGROUND,
    HEADER_BACKGROUND,
    HEADER_TEXT,
    POPUP_BACKGROUND,
    POPUP_BORDER,
    POPUP_TEXT,
  };
}

// Color theme base class
class ColorTheme {
public:
  virtual ~ColorTheme() = default;
  virtual Color::RGB get_color(Color::ColorElement element) const = 0;
  virtual Color::RGB get_background() const = 0;
  virtual Color::RGB get_popup_color() const = 0;
  virtual Color::RGB get_popup_background() const = 0;
  virtual Color::RGB get_popup_border() const = 0;
};

// Solarized Dark theme
namespace Color {
  namespace DarkTheme {
    constexpr RGB BUTTON                    = {147, 161, 161};
    constexpr RGB BUTTON_BACKGROUND         = {  6, 35, 45 };  // Fixed: slightly lighter for readability
    constexpr RGB BUTTON_FOCUS              = {253, 246, 227};
    constexpr RGB BUTTON_INACTIVE           = { 88, 110, 117};
    constexpr RGB COMPONENT                 = { 42, 161, 152 };
    constexpr RGB COMPONENT_BACKGROUND_FOCUS= { 88, 110, 117};
    constexpr RGB COMPONENT_FOCUS           = { 238, 232, 213 };
    constexpr RGB LABEL                     = {131, 148, 150};
    constexpr RGB PLAYHEAD                  = {203,  75,  22};
    constexpr RGB ACTIVE                    = {133, 153,   0};
    constexpr RGB INACTIVE                  = { 88, 110, 117};
    constexpr RGB SLIDER                    = {147, 161, 161};
    constexpr RGB SLIDER_BASE               = {  7, 35, 45 };  // Fixed: lighter base
    constexpr RGB SLIDER_KNOB               = { 42, 161, 152};
    constexpr RGB TEXT                      = {147, 161, 161};
    constexpr RGB TEXT_FOCUS                = {253, 246, 227};
    constexpr RGB WAVEFORM                  = {108, 113, 196};
    constexpr RGB RECORD                    = {255,   0,   0};
    constexpr RGB BACKGROUND                = {  0, 29, 38 };  // Fixed: lighter base for readability
    constexpr RGB POPUP_BACKGROUND          = { 40, 65, 74 };
    constexpr RGB POPUP_BORDER              = {181, 137,   0};
    constexpr RGB POPUP_TEXT                = {253, 246, 227};

    struct Impl : ColorTheme {
      Color::RGB get_color(Color::ColorElement element) const override {
        switch (element) {
          case Color::ColorElement::BUTTON:
            return BUTTON;
          case Color::ColorElement::BUTTON_BACKGROUND:
            return BUTTON_BACKGROUND;
          case Color::ColorElement::BUTTON_FOCUS:
            return BUTTON_FOCUS;
          case Color::ColorElement::BUTTON_INACTIVE:
            return BUTTON_INACTIVE;
          case Color::ColorElement::COMPONENT:
            return COMPONENT;
          case Color::ColorElement::COMPONENT_BACKGROUND_FOCUS:
            return COMPONENT_BACKGROUND_FOCUS;
          case Color::ColorElement::COMPONENT_FOCUS:
            return COMPONENT_FOCUS;
          case Color::ColorElement::LABEL:
            return LABEL;
          case Color::ColorElement::PLAYHEAD:
            return PLAYHEAD;
          case Color::ColorElement::ACTIVE:
            return ACTIVE;
          case Color::ColorElement::INACTIVE:
            return INACTIVE;
          case Color::ColorElement::SLIDER:
            return SLIDER;
          case Color::ColorElement::SLIDER_BASE:
            return SLIDER_BASE;
          case Color::ColorElement::SLIDER_KNOB:
            return SLIDER_KNOB;
          case Color::ColorElement::TEXT:
            return TEXT;
          case Color::ColorElement::TEXT_FOCUS:
            return TEXT_FOCUS;
          case Color::ColorElement::WAVEFORM:
            return WAVEFORM;
          case Color::ColorElement::RECORD:
            return RECORD;
          case Color::ColorElement::CHAT_BACKGROUND:
            return BACKGROUND;
          case Color::ColorElement::INPUT_BACKGROUND:
            return { 6, 35, 45 };  // Fixed: matches BUTTON_BACKGROUND
          case Color::ColorElement::HEADER_BACKGROUND:
            return { 0, 35, 45 };  // Fixed: lighter header bg
          case Color::ColorElement::HEADER_TEXT:
            return TEXT;
          case Color::ColorElement::POPUP_BACKGROUND:
            return POPUP_BACKGROUND;
          case Color::ColorElement::POPUP_BORDER:
            return POPUP_BORDER;
          case Color::ColorElement::POPUP_TEXT:
            return POPUP_TEXT;
          default:
            return TEXT;
        }
      }

      Color::RGB get_background() const override { return BACKGROUND; }
      Color::RGB get_popup_color() const override { return POPUP_TEXT; }
      Color::RGB get_popup_background() const override { return POPUP_BACKGROUND; }
      Color::RGB get_popup_border() const override { return POPUP_BORDER; }
    };
  }

  // Solarized Light theme
  namespace LightTheme {
    constexpr RGB BUTTON                    = { 88, 110, 117};
    constexpr RGB BUTTON_BACKGROUND         = {247, 240, 221};
    constexpr RGB BUTTON_FOCUS              = {  0,  43,  54};
    constexpr RGB BUTTON_INACTIVE           = {147, 161, 161};
    constexpr RGB COMPONENT                 = { 42, 161, 152};
    constexpr RGB COMPONENT_BACKGROUND_FOCUS= {147, 161, 161};
    constexpr RGB COMPONENT_FOCUS           = {  0,  43,  54};
    constexpr RGB LABEL                     = {101, 123, 131};
    constexpr RGB PLAYHEAD                  = {203,  75,  22};
    constexpr RGB ACTIVE                    = {133, 153,   0};
    constexpr RGB INACTIVE                  = {147, 161, 161};
    constexpr RGB SLIDER                    = { 88, 110, 117};
    constexpr RGB SLIDER_BASE               = {238, 232, 213};
    constexpr RGB SLIDER_KNOB               = { 42, 161, 152};
    constexpr RGB TEXT                      = {101, 123, 131};
    constexpr RGB TEXT_FOCUS                = {  0,  43,  54};
    constexpr RGB WAVEFORM                  = {108, 113, 196};
    constexpr RGB RECORD                    = {255,   0,   0};
    constexpr RGB BACKGROUND                = {253, 246, 227};
    constexpr RGB POPUP_BACKGROUND          = {190, 200, 200};
    constexpr RGB POPUP_BORDER              = {181, 137,   0};
    constexpr RGB POPUP_TEXT                = {  0,  43,  54};

    struct Impl : ColorTheme {
      Color::RGB get_color(Color::ColorElement element) const override {
        switch (element) {
          case Color::ColorElement::BUTTON:
            return BUTTON;
          case Color::ColorElement::BUTTON_BACKGROUND:
            return BUTTON_BACKGROUND;
          case Color::ColorElement::BUTTON_FOCUS:
            return BUTTON_FOCUS;
          case Color::ColorElement::BUTTON_INACTIVE:
            return BUTTON_INACTIVE;
          case Color::ColorElement::COMPONENT:
            return COMPONENT;
          case Color::ColorElement::COMPONENT_BACKGROUND_FOCUS:
            return COMPONENT_BACKGROUND_FOCUS;
          case Color::ColorElement::COMPONENT_FOCUS:
            return COMPONENT_FOCUS;
          case Color::ColorElement::LABEL:
            return LABEL;
          case Color::ColorElement::PLAYHEAD:
            return PLAYHEAD;
          case Color::ColorElement::ACTIVE:
            return ACTIVE;
          case Color::ColorElement::INACTIVE:
            return INACTIVE;
          case Color::ColorElement::SLIDER:
            return SLIDER;
          case Color::ColorElement::SLIDER_BASE:
            return SLIDER_BASE;
          case Color::ColorElement::SLIDER_KNOB:
            return SLIDER_KNOB;
          case Color::ColorElement::TEXT:
            return TEXT;
          case Color::ColorElement::TEXT_FOCUS:
            return TEXT_FOCUS;
          case Color::ColorElement::WAVEFORM:
            return WAVEFORM;
          case Color::ColorElement::RECORD:
            return RECORD;
          case Color::ColorElement::CHAT_BACKGROUND:
            return BACKGROUND;
          case Color::ColorElement::INPUT_BACKGROUND:
            return {238, 232, 213};  // base2, slightly darker than chat
          case Color::ColorElement::HEADER_BACKGROUND:
            return BACKGROUND;
          case Color::ColorElement::HEADER_TEXT:
            return TEXT;
          case Color::ColorElement::POPUP_BACKGROUND:
            return POPUP_BACKGROUND;
          case Color::ColorElement::POPUP_BORDER:
            return POPUP_BORDER;
          case Color::ColorElement::POPUP_TEXT:
            return POPUP_TEXT;
          default:
            return TEXT;
        }
      }

      Color::RGB get_background() const override { return BACKGROUND; }
      Color::RGB get_popup_color() const override { return POPUP_TEXT; }
      Color::RGB get_popup_background() const override { return POPUP_BACKGROUND; }
      Color::RGB get_popup_border() const override { return POPUP_BORDER; }
    };
  }

  // Navy theme - the original Nitro color scheme
  namespace NavyTheme {
    // Original hardcoded colors from tui.cpp
    constexpr RGB BUTTON                    = {210, 210, 210};  // light grey for text
    constexpr RGB BUTTON_BACKGROUND         = {22, 28, 38};    // input background
    constexpr RGB BUTTON_FOCUS              = {130, 170, 220}; // cyan highlight
    constexpr RGB BUTTON_INACTIVE           = {22, 28, 38};    // input background
    constexpr RGB COMPONENT                 = {80, 160, 200};  // cyan accent
    constexpr RGB COMPONENT_BACKGROUND_FOCUS= {22, 28, 38};
    constexpr RGB COMPONENT_FOCUS           = {130, 170, 220};
    constexpr RGB LABEL                     = {160, 180, 190};
    constexpr RGB PLAYHEAD                  = {180, 100, 50};
    constexpr RGB ACTIVE                    = {100, 200, 100};
    constexpr RGB INACTIVE                  = {22, 28, 38};
    constexpr RGB SLIDER                    = {210, 210, 210};
    constexpr RGB SLIDER_BASE               = {7, 54, 66};
    constexpr RGB SLIDER_KNOB               = {80, 160, 200};
    constexpr RGB TEXT                      = {210, 210, 210};
    constexpr RGB TEXT_FOCUS                = {130, 170, 220};
    constexpr RGB WAVEFORM                  = {100, 140, 200};
    constexpr RGB RECORD                    = {255, 50, 50};
    constexpr RGB BACKGROUND                = {18, 22, 30};    // chat background
    constexpr RGB POPUP_BACKGROUND          = {25, 30, 40};
    constexpr RGB POPUP_BORDER              = {80, 160, 200};  // cyan
    constexpr RGB POPUP_TEXT                = {210, 210, 210};

    struct Impl : ColorTheme {
      Color::RGB get_color(Color::ColorElement element) const override {
        switch (element) {
          case Color::ColorElement::BUTTON:
            return BUTTON;
          case Color::ColorElement::BUTTON_BACKGROUND:
            return BUTTON_BACKGROUND;
          case Color::ColorElement::BUTTON_FOCUS:
            return BUTTON_FOCUS;
          case Color::ColorElement::BUTTON_INACTIVE:
            return BUTTON_INACTIVE;
          case Color::ColorElement::COMPONENT:
            return COMPONENT;
          case Color::ColorElement::COMPONENT_BACKGROUND_FOCUS:
            return COMPONENT_BACKGROUND_FOCUS;
          case Color::ColorElement::COMPONENT_FOCUS:
            return COMPONENT_FOCUS;
          case Color::ColorElement::LABEL:
            return LABEL;
          case Color::ColorElement::PLAYHEAD:
            return PLAYHEAD;
          case Color::ColorElement::ACTIVE:
            return ACTIVE;
          case Color::ColorElement::INACTIVE:
            return INACTIVE;
          case Color::ColorElement::SLIDER:
            return SLIDER;
          case Color::ColorElement::SLIDER_BASE:
            return SLIDER_BASE;
          case Color::ColorElement::SLIDER_KNOB:
            return SLIDER_KNOB;
          case Color::ColorElement::TEXT:
            return TEXT;
          case Color::ColorElement::TEXT_FOCUS:
            return TEXT_FOCUS;
          case Color::ColorElement::WAVEFORM:
            return WAVEFORM;
          case Color::ColorElement::RECORD:
            return RECORD;
          case Color::ColorElement::CHAT_BACKGROUND:
            return BACKGROUND;
          case Color::ColorElement::INPUT_BACKGROUND:
            return {22, 28, 38};  // original BG_INP
          case Color::ColorElement::HEADER_BACKGROUND:
            return {30, 40, 55};  // original BG_HDR
          case Color::ColorElement::HEADER_TEXT:
            return {210, 210, 210};
          case Color::ColorElement::POPUP_BACKGROUND:
            return POPUP_BACKGROUND;
          case Color::ColorElement::POPUP_BORDER:
            return POPUP_BORDER;
          case Color::ColorElement::POPUP_TEXT:
            return POPUP_TEXT;
          default:
            return TEXT;
        }
      }

      Color::RGB get_background() const override { return BACKGROUND; }
      Color::RGB get_popup_color() const override { return POPUP_TEXT; }
      Color::RGB get_popup_background() const override { return POPUP_BACKGROUND; }
      Color::RGB get_popup_border() const override { return POPUP_BORDER; }
    };
  }
}

// Theme enum for switching
enum class ThemeMode {
  DARK = 0,
  LIGHT = 1,
  NAVY = 2  // Original Nitro color scheme
};

//
// Notcurses TUI
//
class Tui final: TuiContext {
public:
  Tui();
  virtual ~Tui();

  // ── lifecycle ─────────────────────────────────────────────────────
  void init();
  void resize();
  bool is_escape();
  void setup_model(std::string &model_name, const LlamaMemoryInfo &mem);
  void tick_spinner();
  void set_thinking(bool on);
  void update_usage(int tokens_sec, const LlamaMemoryInfo &mem);
  int get_term_rows() const { return term_rows_; }
  
  // ── draw ──────────────────────────────────────────────────────────
  void redraw_header() const;
  void redraw_chat() override;
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
  void show_modal_popup(const std::string &message);
  void show_help() override;
  void dismiss_modal_popup();

  // ── folder picker popup ───────────────────────────────────────
  std::string file_picker(const std::string &start_dir,
                          const std::string &title_hint = "File") const;
  std::string rag_folder_picker(const std::string &start_dir) const {
    return file_picker(start_dir, "RAG Folder");
  }

  double get_tokens_per_sec() const { return tokens_per_sec_; }
  double get_kv_percent() const { return kv_percent_; }

  // TuiContext
  InputEvent get_event() override { return InputEvent(nc_); }
  void render() override { notcurses_render(nc_); }
  void enable_mouse(bool enable) override;

  // ── history ────────────────────────────────────────────────────────
  void history_load(const std::string &path) { input_.load(path); }
  void history_save(const std::string &path) const { input_.save(path); }
  
  // ── theme management ───────────────────────────────────────────────
  void set_theme(ThemeMode mode);
  ThemeMode get_current_theme() const { return current_theme_; }
  void toggle_theme();

private:
  // ── notcurses handles ──────────────────────────────────────────────
  struct notcurses *nc_;
  struct ncplane *stdpl_;
  struct ncplane *header_;
  struct ncplane *chatpl_;
  struct ncplane *inputpl_;
  struct ncplane *modal_plane_;

  // ─── Theme-aware colour helpers ───────────────────────────────────
  void set_plane_background(struct ncplane *pl, Color::ColorElement elem) const;
  void set_plane_foreground(struct ncplane *pl, Color::ColorElement elem) const;
  
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

  // ── theme ─────────────────────────────────────────────────────────
  ThemeMode current_theme_;
  std::unique_ptr<ColorTheme> theme_;
};


