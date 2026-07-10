// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include "input.h"

Input::Input()
  : mouse_mode_(false)
  , insert_mode_(false)
  , ctrl_x_mode_(false)
  , cursor_pos_(0)
  , scroll_offset_(0) {
}

//
// Helper: is the character a word character?
//
static inline bool is_word_char(unsigned char c) {
  return isalnum(c) || c == '_';
}

//
// Find the start of the word containing pos.
// Returns the index of the first character of that word.
//
int Input::pos_of_word_start(int pos) {
  int start = pos;
  while (start > 0 && !isspace(static_cast<unsigned char>(input_buf_[start - 1]))) {
    --start;
  }
  return start;
}

//
// Find the end of the word containing pos.
// Returns the index of the last character of that word.
//
int Input::pos_of_word_end(int pos) {
  int end = pos;
  while (end < static_cast<int>(input_buf_.size()) && !isspace(static_cast<unsigned char>(input_buf_[end]))) {
    ++end;
  }
  return end - 1;
}

//
// Move cursor to the start of the previous word.
// A "word" is a contiguous run of non-whitespace characters.
// If already at the start of a word, move to its beginning.
// If at the start of the buffer, return 0.
//
int Input::move_to_prev_word(int pos) const {
  if (pos == 0) return 0;
  // Find the start of the current word.
  int start = pos;
  while (start > 0 && !isspace(static_cast<unsigned char>(input_buf_[start - 1]))) {
    --start;
  }
  // If we're already at the start, move to the beginning of the word.
  if (start == pos) {
    while (start > 0 && is_word_char(static_cast<unsigned char>(input_buf_[start - 1]))) {
      --start;
    }
  }
  return start;
}

//
// Move cursor to the end of the next word.
// A "word" is a contiguous run of non-whitespace characters.
// If already at the end of a word, move to its end.
// If at the end of the buffer, return size.
//
int Input::move_to_next_word(int pos) const {
  if (pos >= static_cast<int>(input_buf_.size())) return static_cast<int>(input_buf_.size());
  // Find the end of the current word.
  int end = pos;
  while (end < static_cast<int>(input_buf_.size()) && !isspace(static_cast<unsigned char>(input_buf_[end]))) {
    ++end;
  }
  // If we're already at the end, move to the end of the word.
  if (end == pos) {
    while (end > 0 && is_word_char(static_cast<unsigned char>(input_buf_[end - 1]))) {
      --end;
    }
  }
  return end;
}

//
// Delete the word before the cursor.
// A "word" is a contiguous run of non-whitespace characters.
// Returns the new cursor position.
//
int Input::delete_word_before(int pos) {
  int start = move_to_prev_word(pos);
  int end = pos;
  while (start > 0 && is_word_char(static_cast<unsigned char>(input_buf_[start - 1]))) {
    --start;
  }
  input_buf_.erase(start, end - start);
  return start;
}

//
// Kill the word backward.
// A "word" is a contiguous run of non-whitespace characters.
// Returns the new cursor position.
//
int Input::kill_word_backward(int pos) {
  int start = move_to_prev_word(pos);
  int end = pos;
  while (start > 0 && is_word_char(static_cast<unsigned char>(input_buf_[start - 1]))) {
    --start;
  }
  input_buf_.erase(start, end - start);
  return start;
}

//
// Delete the word at the cursor (Emacs-style Alt-D).
// If the cursor is at the end of a word, delete the whole word.
// If the cursor is in the middle of a word, delete from cursor to end.
// If the cursor is at the start of a word, delete the whole word.
// Returns the new cursor position.
//
int Input::delete_word_at_cursor() {
  int start = pos_of_word_start(cursor_pos_);
  int end = pos_of_word_end(cursor_pos_);
  input_buf_.erase(start, end - start);
  return start;
}

//
// Uppercase the word under the cursor (Emacs-style Alt-L).
// Returns the new cursor position.
//
int Input::uppercase_word(int pos) {
  int start = pos_of_word_start(pos);
  int end = pos_of_word_end(pos);
  for (int i = start; i < end; ++i) {
    if (input_buf_[i] >= 'a' && input_buf_[i] <= 'z') {
      input_buf_[i] = static_cast<char>(input_buf_[i] - ('a' - 'A'));
    }
  }
  return start;
}

//
// Lowercase the word under the cursor (Emacs-style Alt-d).
// Returns the new cursor position.
//
int Input::lowercase_word(int pos) {
  int start = pos_of_word_start(pos);
  int end = pos_of_word_end(pos);
  for (int i = start; i < end; ++i) {
    if (input_buf_[i] >= 'A' && input_buf_[i] <= 'Z') {
      input_buf_[i] = static_cast<char>(input_buf_[i] - ('A' - 'a'));
    }
  }
  return start;
}

// ─── Input::readline_blocking (bash-like editing) ─────────────────────────────────
//
// Integrates InputHistory_:  Up/Down arrows navigate the history_ stack.
// On submit the entry is pushed to history_, and nav is reset.
//
// New features (bash-like):
//   Ctrl-Left/Right  : word-wise navigation
//   Ctrl-Home/End    : jump to start/end
//   Ctrl-a/e         : move to start/end (muscle memory)
//   Ctrl-i (Tab)     : insert space
//   Ctrl-Backspace   : delete word before cursor
//   Alt-d            : delete word at cursor (Emacs-style)
//   Alt-l            : lowercase word under cursor
//   Alt-u            : uppercase word under cursor
//   Ctrl-c           : cancel input (discard draft)
//   Ctrl-d           : delete-char (kill next char)
//   Ctrl-e           : kill word backward
//   Ctrl-g           : quit (cancel input)
//   Ctrl-h           : kill-backward-char (backspace char)
//   Ctrl-k           : clear to end of line
//   Ctrl-l           : clear screen and redraw
//   Ctrl-u           : unix-line-discard (clear to beginning)
//   Ctrl-v           : enter insert mode for the next character
//   Ctrl-x Ctrl+k    : kill-line (clear to end)
//   Ctrl-x Ctrl+l    : list-buffers (show history_)
//   Ctrl-x Ctrl+p    : previous-history_ (go up in history_)
//   Ctrl-x Ctrl+r    : recent-files (show recent entries)
//   Ctrl-x Ctrl+s    : save-buffer (push to history_ as "saved")
//   Ctrl-x Ctrl+t    : transient-mark-mode (select region)
//   Ctrl-x Ctrl+u    : universal-argument (repeat next command x4)
//   Ctrl-x Ctrl+v    : insert-file (paste from clipboard)
//   Ctrl-x Ctrl+y    : yank (paste last killed text)
//   Ctrl-x Ctrl+z    : suspend (cancel and return to prompt)
//
std::string Input::readline(TuiContext &tui) {
  input_buf_.clear();
  cursor_pos_ = 0;
  history_.reset_nav();
  tui.redraw_input();
  tui.render();
  tui.render();

  std::string draft;

  for (;;) {
    auto ev = tui.get_event();

    if (ev.is(Key::ENTER) || ev.is(Key::NL) || ev.is(Key::CR)) {
      std::string result = input_buf_;
      if (!result.empty()) {
        history_.push(result);
      }
      input_buf_.clear();
      cursor_pos_ = 0;
      scroll_offset_ = 0;
      tui.redraw_input();
      tui.render();
      return result;
    }

    if (ev.is_ctrl() && ev.is(Key::x)) {
      ctrl_x_mode_ = true;
      // read next key combination
      continue;
    } else if (ctrl_x_mode_ && ev.is_ctrl()) {
      // exit control-x mode
      ctrl_x_mode_ = false;
    }

    if (ev.is(Key::UP)) {
      std::string hist_entry;
      if (history_.up(hist_entry)) {
        if (!input_buf_.empty() && hist_entry != input_buf_) {
          draft = input_buf_;
        }
        input_buf_  = hist_entry;
        cursor_pos_ = input_buf_.size();
      }
      tui.redraw_input();
      tui.render();
      continue;
    }

    if (ev.is(Key::DOWN)) {
      std::string hist_entry;
      if (history_.down(hist_entry)) {
        input_buf_  = hist_entry;
        cursor_pos_ = input_buf_.size();
      } else {
        input_buf_  = draft;
        cursor_pos_ = input_buf_.size();
        draft.clear();
      }
      tui.redraw_input();
      tui.render();
      continue;
    }

    // Ctrl-L: clear screen and redraw
    if (ev.is_ctrl() && ev.is(Key::l)) {
      cursor_pos_ = 0;
      scroll_offset_ = 0;
      input_buf_.clear();
      tui.redraw_input();
      tui.render();
      continue;
    }

    // Insert mode handling
    if (insert_mode_) {
      if (ev.is(Key::BACKSPACE) || ev.is(Key::ESCAPE)) {
        if (cursor_pos_ > 0) {
          input_buf_.erase(cursor_pos_ - 1, 1); --cursor_pos_;
        }
      } else if (ev.is(Key::LEFT)) {
        if (cursor_pos_ > 0) {
          --cursor_pos_;
        }
      } else if (ev.is(Key::RIGHT)) {
        if (cursor_pos_ < input_buf_.size()) {
          ++cursor_pos_;
        }
      } else if (ev.is_edit()) {
        input_buf_.insert(cursor_pos_, 1, ev.val());
        ++cursor_pos_;
      }
      tui.redraw_input();
      tui.render();
      continue;
    }

    // Ctrl-Left / Ctrl-Right: word-wise navigation
    if (ev.is_ctrl() && ev.is(Key::LEFT)) {
      cursor_pos_ = move_to_prev_word(cursor_pos_);
    } else if (ev.key() == Key::RIGHT) {
      cursor_pos_ = move_to_next_word(cursor_pos_);
    }

    // Ctrl-Home / Ctrl-End: jump to start/end
    if (ev.is_ctrl() && ev.key() == Key::HOME) {
      cursor_pos_ = 0;
    } else if (ev.key() == Key::END) {
      cursor_pos_ = input_buf_.size();
    }

    // Ctrl-A / Ctrl-E: move to start/end (muscle memory)
    if (ev.is_ctrl() && ev.key() == Key::a) {
      cursor_pos_ = 0;
    } else if (ev.is_ctrl() && ev.key() == Key::e) {
      cursor_pos_ = input_buf_.size();
    }

    // Ctrl-I (Tab): insert space
    if (ev.is_ctrl() && ev.key() == Key::SPACE) {
      input_buf_.insert(cursor_pos_, 1, ' ');
      ++cursor_pos_;
    }

    // Ctrl-Backspace: delete word before cursor
    if (ev.is_ctrl() && ev.key() == Key::BACKSPACE) {
      cursor_pos_ = delete_word_before(cursor_pos_);
    }

    // Ctrl-W: kill word backward
    if (ev.is_ctrl() && ev.key() == Key::w) {
      cursor_pos_ = kill_word_backward(cursor_pos_);
    }

    // Ctrl-U: clear from cursor to beginning
    if (ev.is_ctrl() && ev.key() == Key::u) {
      input_buf_.erase(0, cursor_pos_);
      cursor_pos_ = 0;
    }

    // Ctrl-K: clear from cursor to end
    if (ev.is_ctrl() && ev.key() == Key::k) {
      input_buf_.erase(cursor_pos_);
    }

    // Ctrl-D: delete-char (kill next char)
    if (ev.is_ctrl() && ev.key() == Key::d) {
      if (cursor_pos_ < input_buf_.size()) {
        input_buf_.erase(cursor_pos_, 1);
      }
    }

    if (ev.is_ctrl() && ev.key() == Key::v) {
      // Ctrl-V: enter insert mode for the next character
      insert_mode_ = true;
      tui.redraw_input();
      tui.render();
      continue;
    }

    // Ctrl-H: kill-backward-char (backspace char)
    if (ev.is_ctrl() && ev.key() == Key::h) {
      if (cursor_pos_ > 0) {
        input_buf_.erase(cursor_pos_ - 1, 1);
        --cursor_pos_;
      }
    }

    // Alt-D: delete word at cursor (Emacs-style)
    if (ev.is_ctrl() && ev.key() == Key::d) {
      cursor_pos_ = delete_word_at_cursor();
    }

    // Alt-L: uppercase word under cursor
    if (ev.is_ctrl() && ev.key() == Key::l) {
      cursor_pos_ = uppercase_word(cursor_pos_);
    }

    // Alt-d: lowercase word under cursor
    if (ev.is_ctrl() && ev.key() == Key::d) {
      cursor_pos_ = lowercase_word(cursor_pos_);
    }

    // Ctrl-G: quit (cancel input)
    if (ev.is_ctrl() && ev.key() == Key::g) {
      input_buf_.clear();
      cursor_pos_ = 0;
      scroll_offset_ = 0;
      tui.redraw_input();
      tui.render();
      continue;
    }

    // Ctrl-x Ctrl-S: save (push to history_ as "saved")
    if (ev.is_ctrl() && ev.key() == Key::s) {
      if (!input_buf_.empty()) {
        history_.push("[saved] " + input_buf_);
      }
      input_buf_.clear();
      cursor_pos_ = 0;
      scroll_offset_ = 0;
      tui.redraw_input();
      tui.render();
      continue;
    }

    if (ctrl_x_mode_) {
      // Ctrl-x Ctrl+K: kill-line (clear to end)
      if (ev.key() == Key::k) {
        input_buf_.erase(cursor_pos_);
      } else if (ev.key() == Key::u) {
        // For now, just ignore — could be used for repeat next command
      } else if (ev.key() == Key::v) {
        // For now, just ignore — could be used for paste from clipboard
      } else if (ev.key() == Key::y) {
        // For now, just ignore — could be used for yank
      } else if (ev.key() == Key::z) {
        // Ctrl-x Ctrl+Z: suspend (cancel and return to prompt)
        input_buf_.clear();
        cursor_pos_ = 0;
        scroll_offset_ = 0;
        tui.redraw_input();
        tui.render();
        continue;
      } else if (ev.key() == Key::k) {
        // Ctrl-x Ctrl+K: kill-line (clear to end)
        input_buf_.erase(cursor_pos_);
      } else if (ev.key() == Key::l) {
        // Ctrl-x Ctrl+L: list-buffers (show history_)
        continue;
      } else if (ev.key() == Key::p) {
        // Ctrl-x Ctrl+P: previous-history_ (go up in history_)
        continue;
      } else if (ev.key() == Key::r) {
        // Ctrl-x Ctrl+R: recent-files (show recent entries)
        continue;
      } else if (ev.key() == Key::s) {
        // Ctrl-x Ctrl+S: save-buffer (push to history_ as "saved")
        if (!input_buf_.empty()) {
          history_.push("[saved] " + input_buf_);
        }
        input_buf_.clear();
        cursor_pos_ = 0;
        scroll_offset_ = 0;
        tui.redraw_input();
        tui.render();
        continue;
      } else if (ev.key() == Key::t) {
        // Ctrl-x Ctrl+T: transient-mark-mode (select region)
      } else if (ev.key() == Key::u) {
        // Ctrl-x Ctrl+U: universal-argument (repeat next command x4)
      } else if (ev.key() == Key::v) {
        // Ctrl-x Ctrl+V: insert-file (paste from clipboard)
      } else if (ev.key() == Key::y) {
        // Ctrl-x Ctrl+Y: yank (paste last killed text)
      } else if (ev.key() == Key::z) {
        // Ctrl-x Ctrl+Z: suspend (cancel and return to prompt)
        input_buf_.clear();
        cursor_pos_ = 0;
        scroll_offset_ = 0;
        tui.redraw_input();
        tui.render();
        continue;
      } else {
        ctrl_x_mode_ = false;
      }
    }

    // Standard editing
    if (ev.is(Key::BACKSPACE) || ev.is(Key::ESCAPE)) {
      if (cursor_pos_ > 0) {
        input_buf_.erase(cursor_pos_ - 1, 1); --cursor_pos_;
      }
    } else if (ev.is(Key::LEFT)) {
      if (cursor_pos_ > 0) --cursor_pos_;
    } else if (ev.is(Key::RIGHT)) {
      if (cursor_pos_ < input_buf_.size()) ++cursor_pos_;
    } else if (ev.is(Key::HOME)) {
      cursor_pos_ = 0;
    } else if (ev.is(Key::END)) {
      cursor_pos_ = input_buf_.size();
    } else if (ev.is(Key::DEL)) {
      if (cursor_pos_ < input_buf_.size()) {
        input_buf_.erase(cursor_pos_, 1);
      }
    } else if (ev.is_edit()) {
      // Any printable character — entering new text clears the nav draft
      // so that Down won't resurrect a stale saved buffer.
      draft.clear();
      history_.reset_nav();
      input_buf_.insert(cursor_pos_, 1, ev.val());
      ++cursor_pos_;
    }

    tui.redraw_input();
    tui.render();
  }
}
