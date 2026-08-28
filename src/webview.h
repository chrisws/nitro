// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org

#pragma once

namespace webview {
  bool start(const std::string &root, int port);
  void stop();
  void broadcast_reload();
  bool is_running();
}
