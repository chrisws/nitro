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

// JSON dependency inversion layer

static bool json_get_string(const std::string &json,
                            const std::string &key,
                            std::string       &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_str(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  out = yyjson_get_str(val);
  yyjson_doc_free(doc);
  return true;
}

static bool json_get_int(const std::string &json,
                             const std::string &key,
                             int &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_num(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  out = yyjson_get_num(val);
  yyjson_doc_free(doc);
  return true;
}

// Tiny helper: extract a float value from flat JSON.
static bool json_get_float(const std::string &json,
                           const std::string &key,
                           float &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_num(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  out = yyjson_get_num(val);
  yyjson_doc_free(doc);
  return true;
}

static bool json_set_string(const std::string &json,
                            const std::string &key,
                            const std::string &value) {
  // For setting, we need to parse and modify
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  // Parse existing JSON
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    // Add/modify the key
    yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), yyjson_mut_str(mut_doc, value.c_str()));
    yyjson_doc_free(doc);
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  // Write back
  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_get_bool(const std::string &json,
                          const std::string &key,
                          bool &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_bool(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  out = yyjson_get_bool(val);
  yyjson_doc_free(doc);
  return true;
}

static bool json_get_array(const std::string &json,
                           const std::string &key,
                           std::vector<std::string> &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  out.reserve(yyjson_arr_size(val));
  for (size_t i = 0; i < yyjson_arr_size(val); i++) {
    yyjson_val *item = yyjson_arr_get(val, i);
    if (item && yyjson_is_str(item)) {
      out.push_back(yyjson_get_str(item));
    }
  }

  yyjson_doc_free(doc);
  return true;
}

static bool json_get_object(const std::string &json,
                            const std::string &key,
                            std::map<std::string, std::string> &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_obj(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *key_ptr, *unused_val;
  for (yyjson_val *k = yyjson_obj_first(val); k != NULL; k = yyjson_obj_next(val, k)) {
    const char *key_str = yyjson_get_str(k);
    yyjson_val *v = yyjson_obj_get(val, key_str);
    if (v && yyjson_is_str(v)) {
      out[std::string(key_str)] = yyjson_get_str(v);
    }
  }

  yyjson_doc_free(doc);
  return true;
}

static bool json_to_string(const std::string &json,
                           std::string &out) {
  out = json;
  return true;
}

static bool json_parse(const std::string &json_str,
                       yyjson_doc **out) {
  *out = yyjson_read_opts((char*)json_str.data(), json_str.size(), 0, NULL, NULL);
  return *out != nullptr;
}

static bool json_write(const yyjson_doc *doc,
                       yyjson_write_flag flags,
                       std::string &out) {
  char *result = yyjson_write(doc, flags);
  if (!result) {
    return false;
  }

  out = std::string(result);
  free(result);
  return true;
}

static bool json_write_mutable(yyjson_mut_doc *doc,
                               yyjson_write_flag flags,
                               std::string &out) {
  char *result = yyjson_mut_write(doc, flags, NULL);
  if (!result) {
    yyjson_mut_doc_free(doc);
    return false;
  }

  out = std::string(result);
  free(result);
  return true;
}

static bool json_append_array(const std::string &json,
                              const std::string &key,
                              const std::string &value) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    yyjson_val *val = yyjson_obj_get(existing, key.c_str());
    if (!val || !yyjson_is_arr(val)) {
      yyjson_doc_free(doc);
      yyjson_mut_doc_free(mut_doc);
      return false;
    }

    // Add to existing array
    yyjson_mut_val *arr_mut = yyjson_mut_copy(NULL, val);
    yyjson_mut_arr_add(arr_mut, yyjson_mut_str(mut_doc, value.c_str()));

    yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), arr_mut);
    yyjson_doc_free(doc);
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_remove(const std::string &json,
                        const std::string &key) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    yyjson_mut_obj_del(root, yyjson_mut_str(mut_doc, key.c_str()));
    yyjson_doc_free(doc);
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_merge(const std::string &base_json,
                       const std::string &overlay_json,
                       std::string &out) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *base_doc = yyjson_read_opts((char*)base_json.data(), base_json.size(), 0, NULL, NULL);
  if (!base_doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *base = yyjson_doc_get_root(base_doc);
  if (base && yyjson_is_obj(base)) {
    yyjson_mut_val *overlay = yyjson_read_opts((char*)overlay_json.data(), overlay_json.size(), 0, NULL, NULL);
    if (overlay && yyjson_is_obj(overlay)) {
      // Merge overlay into base
      yyjson_val *key_ptr, *unused_val;
      for (yyjson_val *k = yyjson_obj_first(overlay); k != NULL; k = yyjson_obj_next(overlay, k)) {
        const char *key_str = yyjson_get_str(k);
        yyjson_val *v = yyjson_obj_get(overlay, key_str);
        if (v) {
          yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key_str), yyjson_mut_copy(NULL, v));
        }
      }
      yyjson_doc_free(base_doc);
    } else {
      yyjson_doc_free(base_doc);
      yyjson_mut_doc_free(mut_doc);
      return false;
    }
  } else {
    yyjson_doc_free(base_doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  out = std::string(result);
  free(result);
  return true;
}

static bool json_deep_copy(const std::string &json,
                           std::string &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_mut_val *copy = yyjson_mut_copy(NULL, root);
  yyjson_mut_doc_set_root(mut_doc, copy);

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    yyjson_doc_free(doc);
    return false;
  }

  out = std::string(result);
  free(result);
  yyjson_doc_free(doc);
  return true;
}

static bool json_validate(const std::string &json) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_doc_free(doc);
  return true;
}

static bool json_get_keys(const std::string &json,
                          std::vector<std::string> &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *key_ptr, *unused_val;
  for (yyjson_val *k = yyjson_obj_first(root); k != NULL; k = yyjson_obj_next(root, k)) {
    out.push_back(yyjson_get_str(k));
  }

  yyjson_doc_free(doc);
  return true;
}

static bool json_get_value(const std::string &json,
                           const std::string &key,
                           yyjson_val **out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val) {
    yyjson_doc_free(doc);
    return false;
  }

  *out = yyjson_copy(NULL, val);
  yyjson_doc_free(doc);
  return true;
}

static bool json_set_value(const std::string &json,
                           const std::string &key,
                           const std::string &value) {
  return json_set_string(json, key, value);
}

static bool json_set_int(const std::string &json,
                         const std::string &key,
                         int value) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), yyjson_mut_num(mut_doc, value));
    yyjson_doc_free(doc);
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_set_float(const std::string &json,
                           const std::string &key,
                           float value) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), yyjson_mut_num_mut(mut_doc, value));
    yyjson_doc_free(doc);
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_set_bool(const std::string &json,
                          const std::string &key,
                          bool value) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), yyjson_mut_bool(mut_doc, value));
    yyjson_doc_free(doc);
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_set_array(const std::string &json,
                           const std::string &key,
                           const std::vector<std::string> &values) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    yyjson_mut_val *arr = yyjson_mut_arr(mut_doc);
    for (const auto &val : values) {
      yyjson_mut_arr_add(arr, yyjson_mut_str(mut_doc, val.c_str()));
    }
    yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), arr);
    yyjson_doc_free(doc);
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_read_file(const std::string &path,
                           std::string &out) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return false;
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  out = oss.str();
  return true;
}

static bool json_write_file(const std::string &path,
                            const std::string &content) {
  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }

  file << content;
  return true;
}

static bool json_exists(const std::string &path) {
  return std::filesystem::exists(path);
}

static bool json_is_valid(const std::string &json) {
  return json_validate(json);
}

static bool json_is_object(const std::string &json) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  bool result = (root != nullptr && yyjson_is_obj(root));
  yyjson_doc_free(doc);
  return result;
}

static bool json_is_array(const std::string &json) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  bool result = (root != nullptr && yyjson_is_arr(root));
  yyjson_doc_free(doc);
  return result;
}

static bool json_is_string(const std::string &json) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  bool result = (root != nullptr && yyjson_is_str(root));
  yyjson_doc_free(doc);
  return result;
}

static bool json_is_number(const std::string &json) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  bool result = (root != nullptr && yyjson_is_num(root));
  yyjson_doc_free(doc);
  return result;
}

static bool json_is_bool(const std::string &json) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  bool result = (root != nullptr && yyjson_is_bool(root));
  yyjson_doc_free(doc);
  return result;
}

static bool json_is_null(const std::string &json) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  bool result = (root != nullptr && yyjson_is_null(root));
  yyjson_doc_free(doc);
  return result;
}

static bool json_has_key(const std::string &json,
                         const std::string &key) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  bool result = (yyjson_obj_get(root, key.c_str()) != nullptr);
  yyjson_doc_free(doc);
  return result;
}

static bool json_get_length(const std::string &json,
                            const std::string &key,
                            size_t &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  out = yyjson_arr_size(val);
  yyjson_doc_free(doc);
  return true;
}

static bool json_get_last(const std::string &json,
                          const std::string &key,
                          std::string &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  size_t size = yyjson_arr_size(val);
  if (size == 0) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *item = yyjson_arr_get(val, size - 1);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_doc_free(doc);
  return true;
}

static bool json_get_first(const std::string &json,
                           const std::string &key,
                           std::string &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  if (yyjson_arr_size(val) == 0) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *item = yyjson_arr_get(val, 0);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_doc_free(doc);
  return true;
}

static bool json_get_index(const std::string &json,
                           const std::string &key,
                           size_t index,
                           std::string &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  if (index >= yyjson_arr_size(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *item = yyjson_arr_get(val, index);
  if (item && yyjson_is_str(item)) {
    out = yyjson_get_str(item);
  } else {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_doc_free(doc);
  return true;
}

static bool json_insert(const std::string &json,
                        const std::string &key,
                        size_t index,
                        const std::string &value) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    yyjson_mut_val *arr = yyjson_mut_copy(NULL, val);
    yyjson_mut_arr_insert(arr, index, yyjson_mut_str(mut_doc, value.c_str()));
    yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), arr);
    yyjson_doc_free(doc);
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_remove_index(const std::string &json,
                              const std::string &key,
                              size_t index) {
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_val *existing = yyjson_doc_get_root(doc);
  if (existing && yyjson_is_obj(existing)) {
    yyjson_val *val = yyjson_obj_get(existing, key.c_str());
    if (val && yyjson_is_arr(val)) {
      if (index >= yyjson_arr_size(val)) {
        yyjson_doc_free(doc);
        yyjson_mut_doc_free(mut_doc);
        return false;
      }

      yyjson_val *item = yyjson_arr_get(val, index);
      yyjson_mut_arr_remove_at(yyjson_mut_copy(NULL, val), index);
      yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), yyjson_mut_copy(NULL, val));
      yyjson_doc_free(doc);
    } else {
      yyjson_doc_free(doc);
      yyjson_mut_doc_free(mut_doc);
      return false;
    }
  } else {
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    return false;
  }

  std::string result_str(result);
  free(result);
  return true;
}

static bool json_filter(const std::string &json,
                        const std::string &key,
                        std::function<bool(const std::string &)> predicate,
                        std::string &out) {
  yyjson_doc *doc = yyjson_read_opts((char*)json.data(), json.size(), 0, NULL, NULL);
  if (!doc) {
    return false;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  if (!root || !yyjson_is_obj(root)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_val *val = yyjson_obj_get(root, key.c_str());
  if (!val || !yyjson_is_arr(val)) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    yyjson_doc_free(doc);
    return false;
  }

  yyjson_mut_val *result_arr = yyjson_mut_arr(mut_doc);

  for (size_t i = 0; i < yyjson_arr_size(val); i++) {
    yyjson_val *item = yyjson_arr_get(val, i);
    if (item && yyjson_is_str(item)) {
      std::string item_str = yyjson_get_str(item);
      if (predicate(item_str)) {
        yyjson_mut_arr_add(result_arr, yyjson_mut_str(mut_doc, item_str.c_str()));
      }
    }
  }

  yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, key.c_str()), result_arr);

  char *result = yyjson_mut_write(mut_doc, 0, NULL);
  yyjson_mut_doc_free(mut_doc);

  if (!result) {
    yyjson_doc_free(doc);
    return false;
  }

  out = std::string(result);
  free(result);
  yyjson_doc_free(doc);
  return true;
}