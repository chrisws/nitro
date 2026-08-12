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

// Graph data structure for visualization
struct GraphData {
  std::string title;
  // "bar", "line", "tree", "network"
  std::string type;
  std::vector<std::pair<std::string, float>> data;
  int width = 80;
  int height = 24;
  std::string color;
  std::string unit;

  // Validation
  bool isValid() const;
};

//
// Canvas class for drawing operations
//
class Canvas {
  public:
  Canvas(std::vector<std::string> buffer, int width, int height);

  void draw_vline(const std::string &color, int x, int y1, int y2);
  void draw_hline(const std::string &color, int x1, int y1, int x2);
  void draw_text(const std::string &color, int x, int y, const std::string &text);
  void draw_char(const std::string &color, int x, int y, std::string c);

  int getWidth() const { return width_; }
  int getHeight() const { return height_; }

  private:
  std::vector<std::string> &buffer_;
  int width_;
  int height_;
};

Canvas::Canvas(std::vector<std::string> buffer, int width, int height)
  : buffer_(buffer)
  , width_(width), height_(height) {
  buffer_.resize(height, std::string(width, ' '));
}

void Canvas::draw_vline(const std::string &color, int x, int y1, int y2) {
  if (x < 0 || x >= width_) return;
  int startY = std::max(0, y1);
  int endY = std::min(height_ - 1, y2);
  for (int y = startY; y <= endY; ++y) {
    buffer_[y][x] = g_vert_bar.front();
  }
}

void Canvas::draw_hline(const std::string &color, int x1, int y1, int x2) {
  if (y1 < 0 || y1 >= height_) return;
  int startX = std::max(0, x1);
  int endX = std::min(width_ - 1, x2);
  for (int x = startX; x <= endX; ++x) {
    buffer_[y1][x] = g_horz_bar.front();
  }
}

void Canvas::draw_text(const std::string &color, int x, int y, const std::string &text) {
  if (y < 0 || y >= height_) return;
  for (size_t i = 0; i < text.length(); ++i) {
    int pos = x + static_cast<int>(i);
    if (pos >= 0 && pos < width_) {
      buffer_[y][pos] = text[i];
    }
  }
}

void Canvas::draw_char(const std::string &color, int x, int y, std::string c) {
  if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
  buffer_[y][x] = c.front();
}

//
// GraphData validation
//
bool GraphData::isValid() const {
  if (type.empty()) return false;
  if (data.empty()) return false;
  if (width <= 0 || height <= 0) return false;
  return true;
}

//
// JSON parsing
//
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
  root.get_str("title", result.title);
  root.get_str("type", result.type);
  root.get_str("color", result.color);
  root.get_str("unit", result.unit);

  // Extract integer fields with defaults
  int width = 80;
  int height = 24;
  root.get_int("width", width);
  root.get_int("height", height);
  result.width = width;
  result.height = height;

  // Parse data array
  std::vector<json::JsonValue> data_arr;
  if (root.get_array("data", data_arr)) {
    for (const auto &item : data_arr) {
      if (!item.is_object()) continue;

      std::string label;
      float value = 0.0f;

      item.get_str("label", label);
      item.get_float("value", value);

      result.data.emplace_back(label, value);
    }
  }

  if (result.isValid() && !result.data.empty()) {
    return result;
  }
  return std::nullopt;
}

//
// Rendering functions
//
static void render_bar_chart(Canvas &canvas, const GraphData &data) {
  int width = std::min(data.width, 80);
  int height = std::min(data.height, 24);

  // Find max value for scaling
  float max_val = 0;
  for (const auto &point : data.data) {
    max_val = std::max(max_val, point.second);
  }
  if (max_val == 0) max_val = 1;

  // Calculate bar dimensions
  int bar_area_width = width - 4; // Leave space for labels and values
  int bar_area_height = height - 4; // Leave space for title and axis labels
  int bar_width = std::max(1, bar_area_width / static_cast<int>(data.data.size()));

  // Draw title
  if (!data.title.empty()) {
    canvas.draw_text(data.color, 2, 1, data.title);
  }

  // Draw axes
  canvas.draw_hline(data.color, 2, height - 2, width - 2);
  canvas.draw_vline(data.color, 2, 2, height - 3);

  // Draw bars
  for (size_t i = 0; i < data.data.size(); ++i) {
    int x = 4 + static_cast<int>(i) * bar_width;
    int bar_height = static_cast<int>((data.data[i].second / max_val) * bar_area_height);

    // Draw bar
    for (int y = 0; y < bar_height; ++y) {
      canvas.draw_char(data.color, x, height - 3 - y, g_block);
    }

    // Draw label
    if (bar_width >= 4 && !data.data[i].first.empty()) {
      std::string label = data.data[i].first.substr(0, bar_width - 2);
      canvas.draw_text(data.color, x + 1, height - 1, label);
    }

    // Draw value
    std::ostringstream value_str;
    value_str << std::fixed << std::setprecision(1) << data.data[i].second;
    canvas.draw_text(data.color, x, height - 4, value_str.str());
  }
}

static void render_tree_view(Canvas &canvas, const GraphData &data) {
  int width = std::min(data.width, 80);
  int height = std::min(data.height, 24);

  // Draw title
  if (!data.title.empty()) {
    canvas.draw_text(data.color, 2, 1, data.title);
  }

  // Draw tree structure
  int y = 3;
  for (const auto &point : data.data) {
    if (y >= height - 1) break;

    std::ostringstream line;
    line << "├── " << point.first << " (" << std::fixed << std::setprecision(1) << point.second << ")";

    std::string line_str = line.str();
    if (line_str.length() > static_cast<size_t>(width - 2)) {
      line_str = line_str.substr(0, width - 2);
    }

    canvas.draw_text(data.color, 2, y, line_str);
    y++;
  }
}

GraphResult tool_graph(std::vector<std::string> &out, int term_cols, const std::string &graph_json) {
  GraphResult result;
  auto data = parse(graph_json);
  if (!data) {
    result.set_error("Failed to parse graph data");
  } else if (data->data.empty()) {
    result.set_error("No data to render");
  } else if (!data->isValid()) {
    result.set_error("Invalid graph data");
  } else if (data->type != "bar" && data->type != "tree") {
    result.set_error("Unsupported graph type");
  } else {
    data->width = std::min(data->width, term_cols);
    data->height = std::min(data->height, 24);
    Canvas canvas(result.data_, data->width, data->height);
    if (data->type == "bar") {
      render_bar_chart(canvas, *data);
    } else {
      render_tree_view(canvas, *data);
    }
  }
  return result;
}

