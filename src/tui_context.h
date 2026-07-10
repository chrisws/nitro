// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include "input_event.h"

class TuiContext {
public:
  virtual InputEvent get_event() = 0; // ncinput ni{};  notcurses_get_blocking(nc, &ni);
  virtual void render() = 0;  // notcurses_render(nc);
  virtual void redraw_input();
  virtual void clear(); // notcurses_clear(nc);
};
