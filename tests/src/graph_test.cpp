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

static void test_horizontal_bar_chart() {
  std::cout << "\n=== Test: Horizontal Bar Chart ===\n";

  std::string json = R"({
    "type": "horizontal",
    "title": "Memory Usage",
    "data": [
      {"label": "Heap", "value": 75},
      {"label": "Stack", "value": 25},
      {"label": "Static", "value": 10}
    ],
    "width": 80,
    "height": 10,
    "color": "purple"
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
  assert(result.success_);
}

static void test_tree_with_nested_children() {
  std::cout << "\n=== Test: Tree with Nested Children ===\n";

  std::string json = R"({
    "title": "Project Structure",
    "type": "tree",
    "color": "#4CAF50",
    "unit": "%",
    "width": 80,
    "height": 12,
    "data": [{
      "label": "src",
      "children": [{
        "label": "graph.cpp",
        "value": 45.5,
        "children": [{
          "label": "render",
          "value": 25.0,
          "children": []
        }, {
          "label": "parse",
          "value": 20.5,
          "children": []
        }]
      }, {
        "label": "json.cpp",
        "value": 30.0,
        "children": [{
          "label": "parse",
          "value": 15.0,
          "children": []
          }
        ]}
      ]}, {
        "label": "include",
        "value": 80.0,
        "children": [{
          "label": "graph.h",
          "value": 35.0,
          "children": []
        },{
          "label": "json.h",
          "value": 45.0,
          "children": []
        }]
    }]
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
  assert(result.success_);
}

static void test_zero_values() {
  std::cout << "\n=== Test: Zero Values ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Zero Test",
    "data": [
      {"label": "A", "value": 0},
      {"label": "B", "value": 50},
      {"label": "C", "value": 0}
    ],
    "width": 40,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_single_value() {
  std::cout << "\n=== Test: Single Value ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Single",
    "data": [
      {"label": "Only", "value": 100}
    ],
    "width": 40,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_very_small_canvas() {
  std::cout << "\n=== Test: Very Small Canvas ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Small",
    "data": [
      {"label": "X", "value": 50}
    ],
    "width": 10,
    "height": 5
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  std::cout << "Data size: " << result.data_.size() << "\n";
  assert(result.success_);
}

static void test_very_large_values() {
  std::cout << "\n=== Test: Very Large Values ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Large Values",
    "data": [
      {"label": "A", "value": 9999},
      {"label": "B", "value": 8888},
      {"label": "C", "value": 7777}
    ],
    "width": 60,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_decimal_values() {
  std::cout << "\n=== Test: Decimal Values ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Decimals",
    "data": [
      {"label": "A", "value": 33.33},
      {"label": "B", "value": 66.66},
      {"label": "C", "value": 12.34}
    ],
    "width": 50,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_no_title() {
  std::cout << "\n=== Test: No Title ===\n";

  std::string json = R"({
    "type": "bar",
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
  assert(result.success_);
}

static void test_no_color() {
  std::cout << "\n=== Test: No Color Specified ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "No Color",
    "data": [
      {"label": "A", "value": 50}
    ],
    "width": 40,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_very_long_labels() {
  std::cout << "\n=== Test: Very Long Labels ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Long Labels",
    "data": [
      {"label": "ThisIsAVeryLongLabelThatShouldBeTruncated", "value": 50},
      {"label": "AnotherVeryLongLabelHere", "value": 60}
    ],
    "width": 40,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_special_characters_in_labels() {
  std::cout << "\n=== Test: Special Characters in Labels ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Special Chars",
    "data": [
      {"label": "CPU@100%", "value": 95},
      {"label": "GPU#88", "value": 88},
      {"label": "RAM$76", "value": 76}
    ],
    "width": 50,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_width_constraint() {
  std::cout << "\n=== Test: Width Constraint Applied ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Width Test",
    "data": [
      {"label": "A", "value": 50},
      {"label": "B", "value": 60},
      {"label": "C", "value": 70},
      {"label": "D", "value": 80},
      {"label": "E", "value": 90}
    ],
    "width": 30,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  std::cout << "Data size: " << result.data_.size() << "\n";
  assert(result.success_);
}

static void test_height_constraint() {
  std::cout << "\n=== Test: Height Constraint Applied ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Height Test",
    "data": [
      {"label": "A", "value": 50},
      {"label": "B", "value": 60},
      {"label": "C", "value": 70}
    ],
    "width": 60,
    "height": 6
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_all_same_values() {
  std::cout << "\n=== Test: All Same Values ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Equal Values",
    "data": [
      {"label": "A", "value": 50},
      {"label": "B", "value": 50},
      {"label": "C", "value": 50}
    ],
    "width": 40,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_monotonic_increasing() {
  std::cout << "\n=== Test: Monotonic Increasing ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Increasing",
    "data": [
      {"label": "A", "value": 10},
      {"label": "B", "value": 20},
      {"label": "C", "value": 30},
      {"label": "D", "value": 40}
    ],
    "width": 50,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_monotonic_decreasing() {
  std::cout << "\n=== Test: Monotonic Decreasing ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Decreasing",
    "data": [
      {"label": "A", "value": 100},
      {"label": "B", "value": 80},
      {"label": "C", "value": 60},
      {"label": "D", "value": 40}
    ],
    "width": 50,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_empty_title() {
  std::cout << "\n=== Test: Empty Title ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "",
    "data": [
      {"label": "A", "value": 50}
    ],
    "width": 40,
    "height": 10
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  assert(result.success_);
}

static void test_very_narrow_canvas() {
  std::cout << "\n=== Test: Very Narrow Canvas ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Narrow",
    "data": [
      {"label": "A", "value": 50}
    ],
    "width": 15,
    "height": 8
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  std::cout << "Data size: " << result.data_.size() << "\n";
  assert(result.success_);
}

static void test_very_tall_canvas() {
  std::cout << "\n=== Test: Very Tall Canvas ===\n";

  std::string json = R"({
    "type": "bar",
    "title": "Tall",
    "data": [
      {"label": "A", "value": 50}
    ],
    "width": 40,
    "height": 30
  })";

  GraphResult result = tool_graph(80, json);

  std::cout << "Success: " << (result.success_ ? "YES" : "NO") << "\n";
  std::cout << "Message: " << result.message_ << "\n";
  std::cout << "Data size: " << result.data_.size() << "\n";
  assert(result.success_);
}

void graph_test() {
  std::cout << "========================================\n";
  std::cout << "  Graph Integration Test Suite\n";
  std::cout << "========================================\n";

  // test_invalid_json();
  // test_empty_data();
  // test_unsupported_type();
  // test_zero_values();
  // test_single_value();
  // test_very_small_canvas();
  // test_very_large_values();
  // test_decimal_values();
  // test_no_title();
  // test_no_color();
  // test_very_long_labels();
  // test_special_characters_in_labels();
  // test_width_constraint();
  // test_height_constraint();
  // test_all_same_values();
  // test_monotonic_increasing();
  // test_monotonic_decreasing();
  // test_empty_title();
  // test_very_narrow_canvas();
  // test_very_tall_canvas();

  // visual tests
  test_tree_with_nested_children();
  // test_large_dataset();  
  // test_valid_tree_view();
  // test_valid_bar_chart();
  // test_horizontal_bar_chart();
  // test_minimal_bar_chart();
  
  std::cout << "\n========================================\n";
  std::cout << "  All tests completed!\n";
  std::cout << "========================================\n";

}

