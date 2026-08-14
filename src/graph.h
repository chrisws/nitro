// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>
#include <vector>
#include <optional>

//
// Data Format
//
// {
//   "type": "bar",
//   "title": "KV Cache Usage",
//   "data": [
//     {"label": "A", "value": 85},
//     {"label": "B", "value": 72},
//     {"label": "C", "value": 91}
//   ],
//   "width": 40,
//   "height": 10,
//   "color": "green"
// }
//
// Visual Examples
//
// Bar Chart (Performance Metrics)
//
// ┌─────────────────────────────────────────────────────────────┐
// │ KV Cache Usage Over Time                                    │
// ├─────────────────────────────────────────────────────────────┤
// │  ████████████████████  85%  ─┐                              │
// │  ████████████████      72%  ─┤                              │
// │  █████████████████████  91% ─┤                              │
// │  ██████████            58%  ─┤                              │
// │  ████████████████████  83%  ─┘                              │
// └─────────────────────────────────────────────────────────────┘
//
// Tree View (Directory Structure)
//
// ┌─────────────────────────────────────────────────────────────┐
// │ Project Structure                                           │
// ├─────────────────────────────────────────────────────────────┤
// │ ┌── CMakeLists.txt                                          │
// │ ├── src/                                                    │
// │ │   ├── nitro.cpp                                           │
// │ │   ├── llama-sb.h                                          │
// │ │   └── llama-sb-rag.cpp                                    │
// │ ├── include/                                                │
// │ │   └── param.h                                             │
// │ └── llama.cpp/                                              │
// │     └── (submodule)                                         │
// └─────────────────────────────────────────────────────────────┘
//

struct GraphResult {
  GraphResult() : success_(true) {}
  ~GraphResult() = default;

  void set_error(const char *message) {
    message_ = std::string("ERROR: ") + message;
    success_ = false;
  }

  std::vector<std::string> data_;
  std::string message_;
  bool success_;
};
  
GraphResult tool_graph(int term_cols, const std::string &graph_json);
