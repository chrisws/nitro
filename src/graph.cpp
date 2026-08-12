// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <regex>
#include <cmath>

#include "graph.h"
#include "json.h"

constexpr std::string g_horz_bar = "─";
constexpr std::string g_vert_bar = "│";
constexpr std::string g_block = "█";

//
// Graph data structure for visualization
//
struct TreeNode {
  std::string label;
  float value;
  std::vector<TreeNode> children;
};

struct GraphData {
  std::string title_;
  std::string type_;
  std::vector<std::pair<std::string, float>> data_;
  int width_ = 80;
  int height_ = 24;
  std::string color_;
  std::string unit_;

  // Nested tree support
  std::vector<TreeNode> tree_nodes_;

  // Validation
  bool isValid() const;
};

//
// Canvas class for drawing operations
//
class Canvas {
  public:
  Canvas(size_t width, size_t height)
    : width_(width)
    , height_(height) {
    buffer_.resize(height, std::vector<std::string>(width, ""));
  }

  void draw_vline(const std::string &color, int x, int y1, int y2);
  void draw_hline(const std::string &color, int x1, int y1, int x2);
  void draw_text(const std::string &color, int x, int y, const std::string &str);
  void draw_char(const std::string &color, int x, int y, const std::string str);

  // Set a unicode character (as string) at position (y, x)
  void insert_at(size_t y, size_t x, std::string str);

  // Clear the entire canvas
  void clear();

  // Get canvas dimensions
  int width() const { return width_; }
  int height() const { return height_; }

  // Render the canvas by concatenating all columns into rows
  std::vector<std::string> render() const;

  private:
  std::vector<std::vector<std::string>> buffer_;
  int width_ = 0;
  int height_ = 0;
};

void Canvas::clear() {
  for (auto& row : buffer_) {
    for (auto& cell : row) {
      cell.clear();
    }
  }
}

std::vector<std::string> Canvas::render() const {
  std::vector<std::string> result;
  result.reserve(height_);
  for (size_t y = 0; y < height_; ++y) {
    std::string row;
    // Reserve space for potential multi-byte chars
    row.reserve(width_ * 4);
    for (size_t x = 0; x < width_; ++x) {
      row += buffer_[y][x];
    }
    result.push_back(row);
  }
  return result;
}

void Canvas::insert_at(size_t y, size_t x, std::string wch) {
  if (x < 0 || x >= width_ || y < 0 || y >= height_) {
    return;
  }
  buffer_[y][x] = wch;
}

void Canvas::draw_vline(const std::string &color, int x, int y1, int y2) {
  if (x < 0 || x >= width_) return;
  int startY = std::max(0, y1);
  int endY = std::min(height_ - 1, y2);
  for (int y = startY; y <= endY; ++y) {
    insert_at(y, x, g_vert_bar);
  }
}

void Canvas::draw_hline(const std::string &color, int x1, int y1, int x2) {
  if (y1 < 0 || y1 >= height_) {
    return;
  }
  int startX = std::max(0, x1);
  int endX = std::min(width_ - 1, x2);
  for (int x = startX; x <= endX; ++x) {
    insert_at(y1, x, g_horz_bar);
  }
}

void Canvas::draw_text(const std::string &color, int x, int y, const std::string &text) {
  if (y < 0 || y >= height_) {
    return;
  }
  for (size_t i = 0; i < text.length(); ++i) {
    int pos = x + static_cast<int>(i);
    if (pos >= 0 && pos < width_) {
      buffer_[y][pos] = text[i];
    }
  }
}

void Canvas::draw_char(const std::string &color, int x, int y, const std::string str) {
  insert_at(y, x, str);
}

//
// GraphData validation
//
bool GraphData::isValid() const {
  if (type_.empty()) return false;
  if (data_.empty()) return false;
  if (width_ <= 0 || height_ <= 0) return false;
  return true;
}

//
// JSON parsing
//
static TreeNode parse_tree_node(const json::JsonValue &item) {
  TreeNode node;
  if (item.is_object()) {
    item.get_str("label", node.label);
    item.get_float("value", node.value);

    // Parse children if present
    std::vector<json::JsonValue> children_arr;
    if (item.get_array("children", children_arr)) {
      for (const auto &child : children_arr) {
        node.children.push_back(parse_tree_node(child));
      }
    }
  }
  return node;
}

std::optional<GraphData> parse(const std::string &json) {
  json::JsonDoc doc = json::parse(json);
  if (!doc.is_valid()) {
    return std::nullopt;
  }

  json::JsonValue root = doc.get_root();
  if (!root.is_object()) {
    return std::nullopt;
  }

  GraphData result;

  // Extract string fields
  root.get_str("title", result.title_);
  root.get_str("type", result.type_);
  root.get_str("color", result.color_);
  root.get_str("unit", result.unit_);

  // Extract integer fields with defaults
  int width = 80;
  int height = 24;
  root.get_int("width", width);
  root.get_int("height", height);
  result.width_ = width;
  result.height_ = height;

  // Parse data array
  std::vector<json::JsonValue> data_arr;
  if (root.get_array("data", data_arr)) {
    for (const auto &item : data_arr) {
      if (!item.is_object()) continue;

      std::string label;
      float value = 0.0f;

      item.get_str("label", label);
      item.get_float("value", value);

      result.data_.emplace_back(label, value);
    }
  }

  // Parse tree nodes for tree type
  if (result.type_ == "tree") {
    std::vector<json::JsonValue> tree_arr;
    if (root.get_array("data", tree_arr)) {
      for (const auto &item : tree_arr) {
        result.tree_nodes_.push_back(parse_tree_node(item));
      }
    }
  }

  if (result.isValid() && !result.data_.empty()) {
    return result;
  }
  return std::nullopt;
}

//
// Rendering functions
//
static void render_bar_chart(Canvas &canvas, const GraphData &data, bool horizontal = false) {
  int width = data.width_;
  int height = data.height_;

  // Find max value for scaling
  float max_val = 0;
  for (const auto &point : data.data_) {
    max_val = std::max(max_val, point.second);
  }
  if (max_val == 0) max_val = 1;

  // Calculate bar dimensions
  int bar_area_width = width - 4; // Leave space for labels and values
  int bar_area_height = height - 4; // Leave space for title and axis labels
  int bar_width = std::max(1, bar_area_width / static_cast<int>(data.data_.size()));

  // Draw title
  if (!data.title_.empty()) {
    canvas.draw_text(data.color_, 2, 1, data.title_);
  }

  // Draw axes
  canvas.draw_hline(data.color_, 2, height - 2, width - 2);
  canvas.draw_vline(data.color_, 2, 2, height - 3);

  // Draw bars
  for (size_t i = 0; i < data.data_.size(); ++i) {
    int x = 4 + static_cast<int>(i) * bar_width;
    int bar_height = static_cast<int>((data.data_[i].second / max_val) * bar_area_height);

    // Draw bar
    for (int y = 0; y < bar_height; ++y) {
      canvas.draw_char(data.color_, x, height - 3 - y, g_block);
    }

    // Draw label
    if (bar_width >= 4 && !data.data_[i].first.empty()) {
      std::string label = data.data_[i].first.substr(0, bar_width - 2);
      canvas.draw_text(data.color_, x + 1, height - 1, label);
    }

    // Draw value
    std::ostringstream value_str;
    value_str << std::fixed << std::setprecision(1) << data.data_[i].second;
    canvas.draw_text(data.color_, x, height - 4, value_str.str());
  }
}

static void render_horizontal_bar_chart(Canvas &canvas, const GraphData &data) {
  int width = data.width_;
  int height = data.height_;

  // Find max value for scaling
  float max_val = 0;
  for (const auto &point : data.data_) {
    max_val = std::max(max_val, point.second);
  }
  if (max_val == 0) max_val = 1;

  // Calculate bar dimensions
  int bar_area_width = width - 4; // Leave space for labels and values
  int bar_area_height = height - 4; // Leave space for title and axis labels
  int bar_height = std::max(1, bar_area_height / static_cast<int>(data.data_.size()));

  // Draw title
  if (!data.title_.empty()) {
    canvas.draw_text(data.color_, 2, 1, data.title_);
  }

  // Draw axes
  canvas.draw_hline(data.color_, 2, height - 2, width - 2);
  canvas.draw_vline(data.color_, width - 2, 2, height - 3);

  // Draw bars (horizontal)
  for (size_t i = 0; i < data.data_.size(); ++i) {
    int y = 4 + static_cast<int>(i) * bar_height;
    int bar_width = static_cast<int>((data.data_[i].second / max_val) * bar_area_width);

    // Draw bar
    for (int x = 0; x < bar_width; ++x) {
      canvas.draw_char(data.color_, width - 3 - x, y, g_block);
    }

    // Draw label
    if (bar_height >= 4 && !data.data_[i].first.empty()) {
      std::string label = data.data_[i].first.substr(0, bar_height - 2);
      canvas.draw_text(data.color_, 2, y + 1, label);
    }

    // Draw value
    std::ostringstream value_str;
    value_str << std::fixed << std::setprecision(1) << data.data_[i].second;
    canvas.draw_text(data.color_, width - 4, y, value_str.str());
  }
}

static void render_tree_node(Canvas &canvas, const TreeNode &node, int indent_level, int &y, int width, int height, const std::string &color) {
  if (y >= height - 1) return;

  // Calculate indentation
  int indent = indent_level * 2;
  std::string prefix;
  for (int i = 0; i < indent; i++) {
    prefix += " ";
  }

  // Add connector based on position
  if (indent_level > 0) {
    prefix += "├── ";
  } else {
    prefix += "└── ";
  }

  // Build line with label and value
  std::ostringstream line;
  line << prefix << node.label << " (" << std::fixed << std::setprecision(1) << node.value << ")";

  // Truncate if too long
  std::string line_str = line.str();
  if (line_str.length() > static_cast<size_t>(width - 2)) {
    line_str = line_str.substr(0, width - 2);
  }

  canvas.draw_text(color, 2, y, line_str);
  y++;

  // Render children
  for (const auto &child : node.children) {
    render_tree_node(canvas, child, indent_level + 1, y, width, height, color);
  }
}

static void render_tree_view(Canvas &canvas, const GraphData &data) {
  int width = data.width_;
  int height = data.height_;

  if (!data.title_.empty()) {
    canvas.draw_text(data.color_, 2, 1, data.title_);
  }

  int y = 3;

  // Render root nodes
  for (const auto &node : data.tree_nodes_) {
    render_tree_node(canvas, node, 0, y, width, height, data.color_);
  }
}

GraphResult tool_graph(int term_cols, const std::string &graph_json) {
  GraphResult result;
  auto data = parse(graph_json);
  if (!data) {
    result.set_error("Failed to parse graph data");
  } else if (data->data_.empty()) {
    result.set_error("No data to render");
  } else if (!data->isValid()) {
    result.set_error("Invalid graph data");
  } else if (data->type_ != "bar" && data->type_ != "tree" && data->type_ != "horizontal") {
    result.set_error("Unsupported graph type");
  } else {
    data->width_ = std::min(data->width_, term_cols);
    data->height_ = std::min(data->height_, 24);
    Canvas canvas(data->width_, data->height_);
    if (data->type_ == "bar") {
      render_bar_chart(canvas, *data);
    } else if (data->type_ == "horizontal") {
      render_horizontal_bar_chart(canvas, *data);
    } else {
      render_tree_view(canvas, *data);
    }
    result.data_ = canvas.render();
  }
  return result;
}

