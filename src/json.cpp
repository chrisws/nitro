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

// JsonDoc implementation

JsonDoc JsonDoc::parse(const std::string &json_str) {
  yyjson_doc *doc = yyjson_read_opts((char*)json_str.data(), json_str.size(), 0, nullptr, nullptr);
  return JsonDoc(doc);
}

JsonDoc parse(const std::string &json_str) {
  return JsonDoc::parse(json_str);
}

JsonDoc parse_string(const std::string &json_str) {
  return JsonDoc::parse(json_str);
}

std::string write(const JsonDoc &doc, WriteFlag flags) {
  if (!doc.valid()) return "";
  char *buffer = yyjson_write(doc.get(), static_cast<yyjson_write_flag>(flags), nullptr);
  if (!buffer) return "";
  std::string result(buffer);
  free(buffer);
  return result;
}

std::string write_mutable(const JsonMutDoc &doc, WriteFlag flags) {
  if (!doc.valid()) return "";
  char *buffer = yyjson_mut_write(doc.get(), static_cast<yyjson_write_flag>(flags), nullptr);
  if (!buffer) return "";
  std::string result(buffer);
  free(buffer);
  return result;
}

bool write_file(const std::string &path, const JsonDoc &doc, WriteFlag flags) {
  std::ofstream file(path);
  if (!file.is_open()) return false;
  file << write(doc, flags);
  return true;
}

bool write_file_mutable(const std::string &path, const JsonMutDoc &doc, WriteFlag flags) {
  std::ofstream file(path);
  if (!file.is_open()) return false;
  file << write_mutable(doc, flags);
  return true;
}

JsonDoc read_file(const std::string &path) {
  std::string content;
  std::ifstream file(path);
  if (!file.is_open()) {
    return JsonDoc(nullptr);
  }
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);
  content.resize(size);
  file.read(&content[0], size);
  return JsonDoc::parse(content);
}

bool file_exists(const std::string &path) {
  return std::filesystem::exists(path);
}

bool validate(const JsonDoc &doc) {
  return doc.valid();
}

bool is_object(const JsonDoc &doc) {
  return doc.valid() && yyjson_is_obj(doc.get_root());
}

bool is_array(const JsonDoc &doc) {
  return doc.valid() && yyjson_is_arr(doc.get_root());
}

bool is_string(const JsonDoc &doc) {
  return doc.valid() && yyjson_is_str(doc.get_root());
}

bool is_number(const JsonDoc &doc) {
  return doc.valid() && yyjson_is_num(doc.get_root());
}

bool is_bool(const JsonDoc &doc) {
  return doc.valid() && yyjson_is_bool(doc.get_root());
}

bool is_null(const JsonDoc &doc) {
  return doc.valid() && yyjson_is_null(doc.get_root());
}

bool get_string(const JsonDoc &doc, const std::string &key, std::string &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_str(val)) return false;

  out = yyjson_get_str(val);
  return true;
}

bool get_int(const JsonDoc &doc, const std::string &key, int &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_num(val)) return false;

  out = yyjson_get_num(val);
  return true;
}

bool get_float(const JsonDoc &doc, const std::string &key, float &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_num(val)) return false;

  out = yyjson_get_num(val);
  return true;
}

bool get_bool(const JsonDoc &doc, const std::string &key, bool &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_bool(val)) return false;

  out = yyjson_get_bool(val);
  return true;
}

bool get_array(const JsonDoc &doc, const std::string &key, std::vector<std::string> &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  out.reserve(yyjson_arr_size(val));
  for (size_t i = 0; i < yyjson_arr_size(val); i++) {
    yyjson_val *item = yyjson_arr_get(val, i);
    if (item && yyjson_is_str(item)) {
      out.push_back(yyjson_get_str(item));
    }
  }

  return true;
}

bool get_keys(const JsonDoc &doc, std::vector<std::string> &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *key_ptr;
  yyjson_obj_iter iter;
  yyjson_obj_iter_init(root, &iter);
  for (key_ptr = yyjson_obj_iter_next(&iter); key_ptr != nullptr; key_ptr = yyjson_obj_iter_next(&iter)) {
    out.push_back(yyjson_get_str(key_ptr));
  }

  return true;
}

// Fixed: get_value now returns yyjson_val* directly instead of using output parameter
yyjson_val* get_value(const JsonDoc &doc, const std::string &key) {
  if (!doc.valid()) return nullptr;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return nullptr;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val) return nullptr;

  return val;
}

bool get_length(const JsonDoc &doc, const std::string &key, size_t &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  out = yyjson_arr_size(val);
  return true;
}

bool get_last(const JsonDoc &doc, const std::string &key, std::string &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  size_t size = yyjson_arr_size(val);
  if (size == 0) return false;

  yyjson_val *item = yyjson_arr_get(val, size - 1);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    return false;
  }

  return true;
}

bool get_first(const JsonDoc &doc, const std::string &key, std::string &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  if (yyjson_arr_size(val) == 0) return false;

  yyjson_val *item = yyjson_arr_get(val, 0);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    return false;
  }

  return true;
}

bool get_index(const JsonDoc &doc, const std::string &key, size_t index, std::string &out) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  if (index >= yyjson_arr_size(val)) return false;

  yyjson_val *item = yyjson_arr_get(val, index);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    return false;
  }

  return true;
}

bool has_key(const JsonDoc &doc, const std::string &key) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  return (yyjson_obj_get(root, key.c_str()) != nullptr);
}

bool has_string_key(const JsonDoc &doc, const std::string &key) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  return (val != nullptr && yyjson_is_str(val));
}

bool has_array_key(const JsonDoc &doc, const std::string &key) {
  if (!doc.valid()) return false;

  yyjson_val *root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  return (val != nullptr && yyjson_is_arr(val));
}

// JsonMutDoc implementation

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

JsonMutDoc parse_mutable(const std::string &json_str) {
  return JsonMutDoc::parse(json_str);
}

bool set_string(JsonMutDoc &doc, const std::string &key, const std::string &value) {
  if (!doc.valid()) return false;

  yyjson_mut_val *root = yyjson_mut_doc_get_root(doc.get());
  yyjson_mut_obj_add(root, yyjson_mut_str(doc.get(), key.c_str()), yyjson_mut_str(doc.get(), value.c_str()));
  return true;
}

} // namespace json
