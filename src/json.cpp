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
#include <yyjson.h>
#include "json.h"

namespace json {

// JsonDoc implementation

JsonDoc JsonDoc::parse(const std::string& json_str) {
  yyjson_doc* doc = yyjson_read_opts((char*)json_str.data(), json_str.size(), 0, nullptr, nullptr);
  return JsonDoc(doc);
}

JsonDoc parse(const std::string& json_str) {
  return JsonDoc::parse(json_str);
}

JsonDoc parse_string(const std::string& json_str) {
  return JsonDoc::parse(json_str);
}

std::string write(const JsonDoc& doc, WriteFlag flags) {
  if (!doc.valid()) return "";
  char* buffer = yyjson_write(doc.get(), static_cast<yyjson_write_flag>(flags));
  if (!buffer) return "";
  std::string result(buffer);
  free(buffer);
  return result;
}

std::string write_mutable(const JsonMutDoc& doc, WriteFlag flags) {
  if (!doc.valid()) return "";
  char* buffer = yyjson_mut_write(doc.get(), static_cast<yyjson_write_flag>(flags), nullptr);
  if (!buffer) return "";
  std::string result(buffer);
  free(buffer);
  return result;
}

std::string to_string(const JsonDoc& doc) {
  return write(doc, WriteFlag::Compact);
}

std::string to_string(const JsonMutDoc& doc) {
  return write_mutable(doc, WriteFlag::Compact);
}

std::string to_string(const JsonDoc& doc, WriteFlag flags) {
  return write(doc, flags);
}

std::string to_string(const JsonMutDoc& doc, WriteFlag flags) {
  return write_mutable(doc, flags);
}

bool write_file(const std::string& path, const JsonDoc& doc, WriteFlag flags) {
  std::string content = write(doc, flags);
  return write_file(path, content);
}

bool write_file_mutable(const std::string& path, const JsonMutDoc& doc, WriteFlag flags) {
  std::string content = write_mutable(doc, flags);
  return write_file(path, content);
}

JsonDoc read_file(const std::string& path) {
  std::string content;
  if (!read_file(path, content)) {
    return JsonDoc(nullptr);
  }
  return JsonDoc::parse(content);
}

bool file_exists(const std::string& path) {
  return std::filesystem::exists(path);
}

bool validate(const JsonDoc& doc) {
  return doc.valid();
}

JsonDoc deep_copy(const JsonDoc& doc) {
  if (!doc.valid()) return JsonDoc(nullptr);
  yyjson_mut_doc* mut_doc = yyjson_mut_doc_new(nullptr);
  if (!mut_doc) return JsonDoc(nullptr);

  yyjson_mut_val* root = yyjson_mut_copy(nullptr, yyjson_doc_get_root(doc.get()));
  yyjson_mut_doc_set_root(mut_doc, root);

  return JsonDoc(mut_doc);
}

JsonDoc filter(const JsonDoc& doc, const std::string& key, std::function<bool(const std::string&)> predicate) {
  if (!doc.valid()) return JsonDoc(nullptr);

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return JsonDoc(nullptr);

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return JsonDoc(nullptr);

  yyjson_mut_doc* mut_doc = yyjson_mut_doc_new(nullptr);
  if (!mut_doc) return JsonDoc(nullptr);

  yyjson_mut_val* result_arr = yyjson_mut_arr(mut_doc);

  for (size_t i = 0; i < yyjson_arr_size(val); i++) {
    yyjson_val* item = yyjson_arr_get(val, i);
    if (item && yyjson_is_str(item)) {
      std::string item_str = yyjson_get_str(item);
      if (predicate(item_str)) {
        yyjson_mut_arr_add(result_arr, yyjson_mut_str(mut_doc, item_str.c_str()));
      }
    }
  }

  yyjson_mut_val* new_root = yyjson_mut_copy(nullptr, root);
  yyjson_mut_obj_set(new_root, yyjson_mut_str(mut_doc, key.c_str()), result_arr);
  yyjson_mut_doc_set_root(mut_doc, new_root);

  return JsonDoc(mut_doc);
}

JsonDoc merge(const JsonDoc& base_doc, const JsonDoc& overlay_doc) {
  if (!base_doc.valid()) return JsonDoc(nullptr);
  if (!overlay_doc.valid()) return base_doc;

  yyjson_mut_doc* mut_doc = yyjson_mut_doc_new(nullptr);
  if (!mut_doc) return JsonDoc(nullptr);

  yyjson_mut_val* root = yyjson_mut_copy(nullptr, yyjson_doc_get_root(base_doc.get()));
  yyjson_mut_doc_set_root(mut_doc, root);

  yyjson_val* overlay_root = yyjson_doc_get_root(overlay_doc.get());
  if (overlay_root && yyjson_is_obj(overlay_root)) {
    yyjson_val* key_ptr, *unused_val;
    for (yyjson_val* k = yyjson_obj_first(overlay_root); k != nullptr; k = yyjson_obj_next(overlay_root, k)) {
      const char* key_str = yyjson_get_str(k);
      yyjson_val* v = yyjson_obj_get(overlay_root, key_str);
      if (v) {
        yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key_str), yyjson_mut_copy(nullptr, v));
      }
    }
  }

  return JsonDoc(mut_doc);
}

bool is_object(const JsonDoc& doc) {
  return doc.valid() && yyjson_is_obj(doc.get_root());
}

bool is_array(const JsonDoc& doc) {
  return doc.valid() && yyjson_is_arr(doc.get_root());
}

bool is_string(const JsonDoc& doc) {
  return doc.valid() && yyjson_is_str(doc.get_root());
}

bool is_number(const JsonDoc& doc) {
  return doc.valid() && yyjson_is_num(doc.get_root());
}

bool is_bool(const JsonDoc& doc) {
  return doc.valid() && yyjson_is_bool(doc.get_root());
}

bool is_null(const JsonDoc& doc) {
  return doc.valid() && yyjson_is_null(doc.get_root());
}

bool get_string(const JsonDoc& doc, const std::string& key, std::string& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_str(val)) return false;

  out = yyjson_get_str(val);
  return true;
}

bool get_int(const JsonDoc& doc, const std::string& key, int& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_num(val)) return false;

  out = yyjson_get_num(val);
  return true;
}

bool get_float(const JsonDoc& doc, const std::string& key, float& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_num(val)) return false;

  out = yyjson_get_num(val);
  return true;
}

bool get_bool(const JsonDoc& doc, const std::string& key, bool& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_bool(val)) return false;

  out = yyjson_get_bool(val);
  return true;
}

bool get_array(const JsonDoc& doc, const std::string& key, std::vector<std::string>& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  out.reserve(yyjson_arr_size(val));
  for (size_t i = 0; i < yyjson_arr_size(val); i++) {
    yyjson_val* item = yyjson_arr_get(val, i);
    if (item && yyjson_is_str(item)) {
      out.push_back(yyjson_get_str(item));
    }
  }

  return true;
}

bool get_object(const JsonDoc& doc, const std::string& key, std::map<std::string, std::string>& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_obj(val)) return false;

  yyjson_val* key_ptr, *unused_val;
  for (yyjson_val* k = yyjson_obj_first(val); k != nullptr; k = yyjson_obj_next(val, k)) {
    const char* key_str = yyjson_get_str(k);
    yyjson_val* v = yyjson_obj_get(val, key_str);
    if (v && yyjson_is_str(v)) {
      out[std::string(key_str)] = yyjson_get_str(v);
    }
  }

  return true;
}

bool get_keys(const JsonDoc& doc, std::vector<std::string>& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* key_ptr, *unused_val;
  for (yyjson_val* k = yyjson_obj_first(root); k != nullptr; k = yyjson_obj_next(root, k)) {
    out.push_back(yyjson_get_str(k));
  }

  return true;
}

bool get_value(const JsonDoc& doc, const std::string& key, yyjson_val** out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val) return false;

  *out = yyjson_copy(nullptr, val);
  return true;
}

bool get_length(const JsonDoc& doc, const std::string& key, size_t& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  out = yyjson_arr_size(val);
  return true;
}

bool get_last(const JsonDoc& doc, const std::string& key, std::string& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  size_t size = yyjson_arr_size(val);
  if (size == 0) return false;

  yyjson_val* item = yyjson_arr_get(val, size - 1);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    return false;
  }

  return true;
}

bool get_first(const JsonDoc& doc, const std::string& key, std::string& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  if (yyjson_arr_size(val) == 0) return false;

  yyjson_val* item = yyjson_arr_get(val, 0);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    return false;
  }

  return true;
}

bool get_index(const JsonDoc& doc, const std::string& key, size_t index, std::string& out) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) return false;

  if (index >= yyjson_arr_size(val)) return false;

  yyjson_val* item = yyjson_arr_get(val, index);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    return false;
  }

  return true;
}

bool has_key(const JsonDoc& doc, const std::string& key) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  return (yyjson_obj_get(root, key.c_str()) != nullptr);
}

bool has_string_key(const JsonDoc& doc, const std::string& key) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  return (val != nullptr && yyjson_is_str(val));
}

bool has_array_key(const JsonDoc& doc, const std::string& key) {
  if (!doc.valid()) return false;

  yyjson_val* root = yyjson_doc_get_root(doc.get());
  if (!root || !yyjson_is_obj(root)) return false;

  yyjson_val* val = yyjson_obj_get(root, key.c_str());
  return (val != nullptr && yyjson_is_arr(val));
}

// JsonMutDoc implementation

JsonMutDoc JsonMutDoc::parse(const std::string& json_str) {
  yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
  if (!doc) return JsonMutDoc(nullptr);

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc);
  yyjson_doc* parsed = yyjson_read_opts((char*)json_str.data(), json_str.size(), 0, nullptr, nullptr);
  if (parsed) {
    yyjson_mut_val* root_copy = yyjson_mut_copy(nullptr, yyjson_doc_get_root(parsed));
    yyjson_mut_doc_set_root(doc, root_copy);
    yyjson_doc_free(parsed);
  }

  return JsonMutDoc(doc);
}

JsonMutDoc parse_mutable(const std::string& json_str) {
  return JsonMutDoc::parse(json_str);
}

bool set_string(JsonMutDoc& doc, const std::string& key, const std::string& value) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());
  yyjson_mut_obj_add(root, yyjson_mut_str(doc.get(), key.c_str()), yyjson_mut_str(doc.get(), value.c_str()));
  return true;
}

bool set_int(JsonMutDoc& doc, const std::string& key, int value) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());
  yyjson_mut_obj_add(root, yyjson_mut_str(doc.get(), key.c_str()), yyjson_mut_num(doc.get(), value));
  return true;
}

bool set_float(JsonMutDoc& doc, const std::string& key, float value) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());
  yyjson_mut_obj_add(root, yyjson_mut_str(doc.get(), key.c_str()), yyjson_mut_num_mut(doc.get(), value));
  return true;
}

bool set_bool(JsonMutDoc& doc, const std::string& key, bool value) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());
  yyjson_mut_obj_add(root, yyjson_mut_str(doc.get(), key.c_str()), yyjson_mut_bool(doc.get(), value));
  return true;
}

bool set_array(JsonMutDoc& doc, const std::string& key, const std::vector<std::string>& values) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());
  yyjson_mut_val* arr = yyjson_mut_arr(doc.get());
  for (const auto& val : values) {
    yyjson_mut_arr_add(arr, yyjson_mut_str(doc.get(), val.c_str()));
  }
  yyjson_mut_obj_add(root, yyjson_mut_str(doc.get(), key.c_str()), arr);
  return true;
}

bool append_array(JsonMutDoc& doc, const std::string& key, const std::string& value) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());

  yyjson_mut_val* arr = yyjson_mut_get(root, yyjson_mut_str(doc.get(), key.c_str()));
  if (!arr || !yyjson_is_arr(arr)) return false;

  yyjson_mut_arr_add(arr, yyjson_mut_str(doc.get(), value.c_str()));
  return true;
}

bool remove(JsonMutDoc& doc, const std::string& key) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());
  yyjson_mut_obj_del(root, yyjson_mut_str(doc.get(), key.c_str()));
  return true;
}

bool remove_index(JsonMutDoc& doc, const std::string& key, size_t index) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());

  yyjson_mut_val* arr = yyjson_mut_get(root, yyjson_mut_str(doc.get(), key.c_str()));
  if (!arr || !yyjson_is_arr(arr)) return false;

  if (index >= yyjson_arr_size(arr)) return false;

  yyjson_mut_val* item = yyjson_arr_get(arr, index);
  yyjson_mut_arr_remove_at(yyjson_mut_copy(nullptr, arr), index);
  yyjson_mut_obj_set(root, yyjson_mut_str(doc.get(), key.c_str()), item);
  return true;
}

bool insert(JsonMutDoc& doc, const std::string& key, size_t index, const std::string& value) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());

  yyjson_mut_val* arr = yyjson_mut_get(root, yyjson_mut_str(doc.get(), key.c_str()));
  if (!arr || !yyjson_is_arr(arr)) return false;

  yyjson_mut_arr_insert(arr, index, yyjson_mut_str(doc.get(), value.c_str()));
  return true;
}

bool insert_first(JsonMutDoc& doc, const std::string& key, const std::string& value) {
  return insert(doc, key, 0, value);
}

bool insert_last(JsonMutDoc& doc, const std::string& key, const std::string& value) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());

  yyjson_mut_val* arr = yyjson_mut_get(root, yyjson_mut_str(doc.get(), key.c_str()));
  if (!arr || !yyjson_is_arr(arr)) return false;

  yyjson_mut_arr_add(arr, yyjson_mut_str(doc.get(), value.c_str()));
  return true;
}

bool clear_array(JsonMutDoc& doc, const std::string& key) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());

  yyjson_mut_val* arr = yyjson_mut_get(root, yyjson_mut_str(doc.get(), key.c_str()));
  if (!arr || !yyjson_is_arr(arr)) return false;

  yyjson_mut_arr_clear(arr);
  return true;
}

bool remove_array(JsonMutDoc& doc, const std::string& key) {
  if (!doc.valid()) return false;

  yyjson_mut_val* root = yyjson_mut_doc_get_root(doc.get());

  yyjson_mut_val* arr = yyjson_mut_get(root, yyjson_mut_str(doc.get(), key.c_str()));
  if (!arr || !yyjson_is_arr(arr)) return false;

  yyjson_mut_val* new_arr = yyjson_mut_arr(doc.get());
  yyjson_mut_obj_set(root, yyjson_mut_str(doc.get(), key.c_str()), new_arr);
  return true;
}

bool merge(JsonMutDoc& base_doc, const JsonMutDoc& overlay_doc) {
  if (!base_doc.valid() || !overlay_doc.valid()) return false;

  yyjson_mut_val* base_root = yyjson_mut_doc_get_root(base_doc.get());
  yyjson_val* overlay_root = yyjson_doc_get_root(overlay_doc.get());

  if (!overlay_root || !yyjson_is_obj(overlay_root)) return false;

  yyjson_val* key_ptr, *unused_val;
  for (yyjson_val* k = yyjson_obj_first(overlay_root); k != nullptr; k = yyjson_obj_next(overlay_root, k)) {
    const char* key_str = yyjson_get_str(k);
    yyjson_val* v = yyjson_obj_get(overlay_root, key_str);
    if (v) {
      yyjson_mut_obj_add(base_root, yyjson_mut_str(base_doc.get(), key_str), yyjson_mut_copy(nullptr, v));
    }
  }

  return true;
}

} // namespace json
