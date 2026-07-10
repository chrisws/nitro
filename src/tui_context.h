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
  TuiContext() = default;
  virtual ~TuiContext() = default;
  TuiContext(const TuiContext &) = delete;
  TuiContext &operator=(const TuiContext &) = delete;

  virtual InputEvent get_event() = 0;
  virtual void render() = 0;
  virtual void redraw_input() const = 0;
};
