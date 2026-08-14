# Graph Tree View JSON Format

# Graph Tree View JSON Format

This document describes the JSON format for creating tree-view graphs with nested children in Nitro.

## Overview

The tree view supports unlimited nesting depth. Each node can have child nodes, which can themselves have children.

## Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `title` | string | Title of the graph (optional) |
| `type` | string | Must be `"tree"` |
| `color` | string | Color for rendering (optional) |
| `unit` | string | Unit label (optional) |
| `width` | integer | Canvas width (optional, default: 80) |
| `height` | integer | Canvas height (optional, default: 24) |
| `data` | array | Root-level nodes array |

## Node Structure

Each node in the `data` array must have:

| Field | Type | Description |
|-------|------|-------------|
| `label` | string | Node label/name |
| `value` | float | Numeric value for the node |
| `children` | array | Optional array of child nodes |

## Example

```json
{
  "title": "Project Structure",
  "type": "tree",
  "color": "#4CAF50",
  "unit": "%",
  "width": 80,
  "height": 24,
  "data": [
    {
      "label": "src",
      "value": 100.0,
      "children": [
        {
          "label": "graph.cpp",
          "value": 45.5,
          "children": [
            {
              "label": "render()",
              "value": 25.0,
              "children": []
            },
            {
              "label": "parse()",
              "value": 20.5,
              "children": []
            }
          ]
        },
        {
          "label": "json.cpp",
          "value": 30.0,
          "children": [
            {
              "label": "parse()",
              "value": 15.0,
              "children": []
            }
          ]
        }
      ]
    },
    {
      "label": "include",
      "value": 80.0,
      "children": [
        {
          "label": "graph.h",
          "value": 35.0,
          "children": []
        },
        {
          "label": "json.h",
          "value": 45.0,
          "children": []
        }
      ]
    }
  ]
}
```

## Usage

To render a tree graph, call the `tool_graph` function with:

```cpp
GraphResult result = tool_graph(term_cols, json_string);
```

The `json_string` should be a valid JSON string matching the format above.

## Notes

- Empty `children` arrays (`[]`) indicate leaf nodes
- The `data` field at the root level contains top-level nodes
- Nested nodes use the `children` field instead of `data`
- Values are displayed with 1 decimal precision
- The tree is rendered with box-drawing characters (├──, └──, │)