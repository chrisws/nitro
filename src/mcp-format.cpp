// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include "mcp-format.h"

static std::string quote(const std::string value) {
  return "\"" + value + "\"";
}

static std::string shorten(const std::string &desc) {
  // Find first '.' or '\n'
  size_t stopPos = std::string::npos;
  int startPos = 0;
  size_t i = 0;

  while (i < desc.size() && desc[i] == ' ') {
    startPos++;
    i++;
  }

  while (i < desc.size()) {
    if (desc[i] == '.' || desc[i] == '\n') {
      stopPos = i;
      break;
    }
    i++;
  }

  if (stopPos != std::string::npos) {
    return desc.substr(startPos, stopPos);
  }
  return desc;
}

static std::string createExample(const json::JsonValue &propObj) {
  std::string result = "{";
  std::vector<std::string> keys;
  propObj.get_keys(keys);

  std::string delim = "";
  for (size_t i = 0; i < keys.size(); ++i) {
    const std::string &key = keys[i];
    auto field = propObj.get_child(key);
    std::string type;
    std::string description;
    std::string element;

    if (field.get_str("type", type) &&
        field.get_str("description", description)) {
      if (type == "string") {
        element = quote(shorten(description));
      } else if (type == "array") {
        element = "[item1, item2]";
      } else if (type == "boolean") {
        element = "true";
      } else if (type == "integer") {
        element = "1";
      }
      if (!element.empty()) {
        result += delim;
        result += quote(key) + ":" + element;
        delim = ",";
      }
    }
  }
  result += '}';
  return result;
}

static std::string required(const json::JsonValue &inputSchema) {
  std::string result;
  auto value = inputSchema.get_child("required");
  std::vector<json::JsonValue> fields;
  value.get_array(fields);
  if (fields.size() > 0) {
    result = "Field " + fields[0].get_str() + " is required\n\n";
  } else {
    result = "\n\n";
  }
  return result;
}

std::string formatSpec(const json::JsonValue &root) {
  std::string result = "";

  json::JsonValue inputSchema = root.get_child("inputSchema");
  json::JsonValue outputSchema = root.get_child("outputSchema");
  std::string name;
  std::string description;

  if (root.get_str("name", name) &&
      root.get_str("description", description) &&
      inputSchema.is_valid() &&
      outputSchema.is_valid()) {
    result = "# Tool name: " + name + "\n\n";
    result += "## Description\n";
    result += description + "\n";
    result += required(inputSchema);
    result += "## Input example\n";
    result += "```\n";
    result += createExample(inputSchema.get_child("properties"));;
    result += "\n```\n\n";
    result += "## Output schema\n";
    result += "```\n";
    result += outputSchema.to_string();
    result += "\n```\n";
  }
  return result;
}
