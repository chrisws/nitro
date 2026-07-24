// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <string>
#include <fstream>
#include <format>
#include <sstream>
#include <filesystem>
#include <yyjson.h>
#include "json.h"

namespace json {

//
// JsonValue implementation
//
bool JsonValue::get_str(const std::string &key, std::string &out) const {
  if (!is_valid()) return false;

  yyjson_val *val = yyjson_obj_get(value_, key.c_str());
  if (!val || !yyjson_is_str(val)) return false;

  out = yyjson_get_str(val);
  return true;
}

bool JsonValue::get_int(const std::string &key, int &out) const {
  if (!is_valid()) return false;

  yyjson_val *val = yyjson_obj_get(value_, key.c_str());
  if (!val || !yyjson_is_num(val)) return false;

  out = yyjson_get_num(val);
  return true;
}

bool JsonValue::get_float(const std::string &key, float &out) const {
  if (!is_valid()) return false;

  yyjson_val *val = yyjson_obj_get(value_, key.c_str());
  if (!val || !yyjson_is_num(val)) return false;

  out = yyjson_get_num(val);
  return true;
}

JsonValue JsonValue::get(const std::string &key) const {
  return JsonValue(yyjson_obj_get(value_, key.c_str()));
}

bool JsonValue::get_array(const std::string &key, std::vector<JsonValue> &out) const {
  if (!is_valid()) return false;

  yyjson_val *val = yyjson_obj_get(value_, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  out.reserve(yyjson_arr_size(val));
  for (size_t i = 0; i < yyjson_arr_size(val); i++) {
    yyjson_val *item = yyjson_arr_get(val, i);
    if (item) {
      out.push_back(JsonValue(item));
    }
  }
  return true;
}

bool JsonValue::get_keys(std::vector<std::string> &out) const {
  if (!is_valid()) return false;

  yyjson_val *key_ptr;
  yyjson_obj_iter iter;
  yyjson_obj_iter_init(value_, &iter);
  for (key_ptr = yyjson_obj_iter_next(&iter); key_ptr != nullptr; key_ptr = yyjson_obj_iter_next(&iter)) {
    out.push_back(yyjson_get_str(key_ptr));
  }
  return true;
}

bool JsonValue::has_string_key(const std::string &key) const {
  if (!is_valid()) return false;

  yyjson_val *val = yyjson_obj_get(value_, key.c_str());
  return (val != nullptr && yyjson_is_str(val));
}

//
// JsonDoc implementation
//
JsonDoc parse(const std::string &json_str) {
  return JsonDoc::parse(json_str);
}

JsonDoc JsonDoc::parse(const std::string &json_str) {
  yyjson_doc *doc = yyjson_read_opts((char*)json_str.data(), json_str.size(), 0, nullptr, nullptr);
  return JsonDoc(doc);
}

JsonValue JsonDoc::get_root() const {
  if (doc_ != nullptr) {
    yyjson_val *root = yyjson_doc_get_root(doc_);
    if (root && yyjson_is_obj(root)) {
      return JsonValue(root);
    }
  }
  return JsonValue();
}

std::string JsonDoc::write(WriteFlag flags) const {
  if (!is_valid()) return "";
  char *buffer = yyjson_write(doc_, static_cast<yyjson_write_flag>(flags), nullptr);
  if (!buffer) return "";
  std::string result(buffer);
  free(buffer);
  return result;
}

bool JsonDoc::write_file(const std::string &path, WriteFlag flags) const {
  std::ofstream file(path);
  if (!file.is_open()) return false;
  file << write(flags);
  return true;
}

//
// JsonMutValue implementation
//
bool JsonMutValue::set_str(const std::string &key, const std::string &value) {
  if (!is_valid()) return false;
  yyjson_mut_obj_add(value_, yyjson_mut_str(doc_, key.c_str()), yyjson_mut_str(doc_, value.c_str()));
  return true;
}

bool JsonMutValue::set_obj(const std::string &key, const JsonMutValue &value) {
  if (!is_valid() || !value.is_valid()) return false;
  yyjson_mut_obj_add(value_, yyjson_mut_str(doc_, key.c_str()), value.value_);
  return true;
}

//
// JsonMutDoc implementation
//
JsonMutDoc parse_mutable(const std::string &json_str) {
  return JsonMutDoc::parse(json_str);
}

JsonMutDoc JsonMutDoc::parse(const std::string &json_str) {
  yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
  if (!doc) return JsonMutDoc(nullptr);

  yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);
  yyjson_doc *parsed = yyjson_read_opts((char*)json_str.data(), json_str.size(), 0, nullptr, nullptr);
  if (parsed) {
    yyjson_mut_val *root_copy = yyjson_val_mut_copy(nullptr, yyjson_doc_get_root(parsed));
    yyjson_mut_doc_set_root(doc, root_copy);
    yyjson_doc_free(parsed);
  }

  return JsonMutDoc(doc);
}

JsonMutValue JsonMutDoc::get_root() const {
  if (doc_ != nullptr) {
    auto *root = yyjson_mut_doc_get_root(doc_);
    if (root && yyjson_mut_is_obj(root)) {
      return JsonMutValue(doc_, root);
    }
  }
  return JsonMutValue();
}

std::string JsonMutDoc::write(WriteFlag flags) const {
  if (!is_valid()) return "";
  char *buffer = yyjson_mut_write(doc_, static_cast<yyjson_write_flag>(flags), nullptr);
  if (!buffer) return "";
  std::string result(buffer);
  free(buffer);
  return result;
}

} // namespace json

