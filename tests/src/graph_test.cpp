// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <iostream>
#include <cassert>
#include <string>
#include "graph.h"

// Test cases for tool_graph
static void test_valid_bar_chart() {
  std::cout << "\n=== Test: Valid Bar Chart ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "KV Cache Usage",
    "data": [
      {"label": "A", "value": 85},
      {"label": "B", "value": 72},
      {"label": "C", "value": 91}
    ],
    "width": 40,
    "height": 10,
    "color": "green"
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  std::cout << "Data size: " << result.data_.size() << "\n";

  if (result.success_) {
    for (const auto& line : result.data_) {
      std::cout << line << "\n";
    }
  }
}

static void test_valid_tree_view() {
  std::cout << "\n=== Test: Valid Tree View ===\n";

  std::string json = R"({
    "type": "tree",
    "title": "Project Structure",
    "data": [
      {"label": "nitro", "value": 100},
      {"label": "src", "value": 85},
      {"label": "include", "value": 72},
      {"label": "tests", "value": 65}
    ],
    "width": 60,
    "height": 15,
    "color": "blue"
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  std::cout << "Data size: " << result.data_.size() << "\n";

  if (result.success_) {
    for (const auto& line : result.data_) {
      std::cout << line << "\n";
    }
  }
}

static void test_invalid_json() {
  std::cout << "\n=== Test: Invalid JSON ===\n";

  std::string json = "not valid json {{{";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(!result.success_);
  assert(result.message_.find("ERROR") != std::string::npos);
}

static void test_empty_data() {
  std::cout << "\n=== Test: Empty Data ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Empty Chart",
    "data": [],
    "width": 40,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(!result.success_);
  assert(result.message_.find("ERROR") != std::string::npos);
}

static void test_unsupported_type() {
  std::cout << "\n=== Test: Unsupported Graph Type ===\n";

  std::string json = R"({
    "type": "pie",
    "title": "Pie Chart",
    "data": [
      {"label": "A", "value": 50},
      {"label": "B", "value": 50}
    ],
    "width": 40,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(!result.success_);
  assert(result.message_.find("ERROR") != std::string::npos);
}

static void test_minimal_bar_chart() {
  std::cout << "\n=== Test: Minimal Bar Chart ===\n";

  std::string json = R"({
    "type": "bar",
    "data": [
      {"label": "X", "value": 50}
    ]
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  std::cout << "Data size: " << result.data_.size() << "\n";

  if (result.success_) {
    for (const auto& line : result.data_) {
      std::cout << line << "\n";
    }
  }
}

static void test_large_dataset() {
  std::cout << "\n=== Test: Large Dataset ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Performance Metrics",
    "data": [
      {"label": "CPU", "value": 95},
      {"label": "GPU", "value": 88},
      {"label": "RAM", "value": 76},
      {"label": "SSD", "value": 92},
      {"label": "NET", "value": 65},
      {"label": "DISK", "value": 81},
      {"label": "IO", "value": 73},
      {"label": "MEM", "value": 89}
    ],
    "width": 100,
    "height": 15,
    "color": "red"
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  std::cout << "Data size: " << result.data_.size() << "\n";

  if (result.success_) {
    for (const auto& line : result.data_) {
      std::cout << line << "\n";
    }
  }
}

void graph_test() {
  std::cout << "========================================\n";
  std::cout << "  Graph Integration Test Suite\n";
  std::cout << "========================================\n";

  test_large_dataset();
  test_valid_bar_chart();
  test_valid_tree_view();
  test_invalid_json();
  test_empty_data();
  test_unsupported_type();
  test_minimal_bar_chart();

  std::cout << "\n========================================\n";
  std::cout << "  All tests completed!\n";
  std::cout << "========================================\n";

}

