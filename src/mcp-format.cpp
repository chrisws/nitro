// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include "mcp-format.h"

static std::string generateProperties(const json::JsonValue &propObj) {
  std::string result = "{";
  std::vector<std::string> keys;
  propObj.get_keys(keys);

  for (size_t i = 0; i < keys.size(); ++i) {
    const std::string &key = keys[i];
    auto child = propObj.get_child(key);

    if (child.is_object()) {
      result += "\"" + key + "\": " + generateProperties(child);
    } else if (child.is_str()) {
      result += "\"" + key + "\": \"" + child.get_str() + "\"";
    } else if (child.is_arr()) {
      result += "\"" + key + "\": [";
      std::vector<json::JsonValue> arrItems;
      child.get_array(arrItems);
      for (size_t j = 0; j < arrItems.size(); ++j) {
        result += arrItems[j].get_str();
        if (j < arrItems.size() - 1) result += ", ";
      }
      result += ']';
    }

    if (i < keys.size() - 1) result += ", ";
  }

  result += '}';
  return result;
}

std::string formatInputExample(const json::JsonValue &root) {
  std::string example = "{";
  std::vector<std::string> keys;
  root.get_keys(keys);

  for (size_t i = 0; i < keys.size(); ++i) {
    const std::string &key = keys[i];
    auto propObj = root.get_child(key);

    if (key == "properties") {
      // Recursively generate nested properties
      example += generateProperties(propObj);
    } else if (key == "type" && propObj.is_str()) {
      example += "\"" + key + "\": " + propObj.get_str() + ", ";
    } else if (key == "required" && propObj.is_arr()) {
      example += "\"" + key + "\": [";
      std::vector<std::string> reqKeys;      
      propObj.get_keys(reqKeys);
      for (size_t j = 0; j < reqKeys.size(); ++j) {
        example += "\"" + reqKeys[j] + "\"";
        if (j < reqKeys.size() - 1) example += ", ";
      }
      example += "], ";
    } else if (key == "items" && propObj.is_object()) {
      example += "\"" + key + R"(": {"type": "array", )";
      example += R"("items": {"type": )" + propObj.get_str() + "}}";
    }

    if (i < keys.size() - 1) example += ", ";
  }

  example += '}';
  return example;
}

std::string formatOutputExample(const json::JsonValue &root) {
  std::string example = "{";
  std::vector<std::string> keys;
  root.get_keys(keys);

  for (size_t i = 0; i < keys.size(); ++i) {
    const std::string &key = keys[i];
    auto propObj = root.get_child(key);

    if (key == "items" && propObj.is_object()) {
      example += "\"" + key + "\": [";
      example += '{';
      auto itemsProps = propObj.get_child("properties");
      std::vector<std::string> itemsKeys;
      itemsProps.get_keys(itemsKeys);

      for (size_t j = 0; j < itemsKeys.size(); ++j) {
        const std::string& itemKey = itemsKeys[j];
        auto itemProp = itemsProps.get_child(itemKey);

        if (itemProp.is_str()) {
          example += "\"" + itemKey + "\": \"" + itemProp.get_str() + "\"";
        } else if (itemProp.is_arr()) {
          example += "\"" + itemKey + "\": [";
          std::vector<json::JsonValue> arrItems;          
          itemProp.get_array(arrItems);
          for (size_t k = 0; k < arrItems.size(); ++k) {
            example += arrItems[k].get_str();
            if (k < arrItems.size() - 1) example += ", ";
          }
          example += ']';
        } else if (itemProp.is_object()) {
          example += "\"" + itemKey + "\": {";
          std::vector<std::string> nestedKeys;          
          itemProp.get_keys(nestedKeys);
          for (size_t k = 0; k < nestedKeys.size(); ++k) {
            const std::string &nk = nestedKeys[k];
            auto nval = itemProp.get_child(nk);
            if (nval.is_str()) {
              example += "\"" + nk + "\": \"" + nval.get_str() + "\"";
            } else if (nval.is_arr()) {
              example += "\"" + nk + "\": [";
              std::vector<json::JsonValue> arrItems;              
              nval.get_array(arrItems);
              for (size_t m = 0; m < arrItems.size(); ++m) {
                example += arrItems[m].get_str();
                if (m < arrItems.size() - 1) example += ", ";
              }
              example += ']';
            }
            if (k < nestedKeys.size() - 1) example += ", ";
          }
          example += '}';
        }

        if (j < itemsKeys.size() - 1) example += ", ";
      }
      example += "], ";
    } else if (key == "more") {
      example += "\"" + key + "\": false";
    } else if (key == "partialResultReason") {
      example += "\"" + key + "\": null";
    }

    if (i < keys.size() - 1) example += ", ";
  }

  example += '}';
  return example;
}

