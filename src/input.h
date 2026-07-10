// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>
#include <cstdint>
#include <notcurses/notcurses.h>

#include "input_history.h"
#include "tui_context.h"

class Input {
  public:
  std::string readline(TuiContext &tui);

  private:
  int pos_of_word_start(int pos);
  int pos_of_word_end(int pos);
  int move_to_prev_word(int pos) const;
  int move_to_next_word(int pos) const;
  int delete_word_before(int pos);
  int kill_word_backward(int pos);
  int delete_word_at_cursor();
  int uppercase_word(int pos);
  int lowercase_word(int pos);

  std::string input_buf_;
  size_t cursor_pos_;
  bool mouse_mode_;
  bool insert_mode_;
  bool ctrl_x_mode_;
  int scroll_offset_;
  InputHistory history_;
};

