// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "yyjson.h"

//
// JSON dependency inversion layer header
// Provides a C++ wrapper around yyjson for JSON parsing and manipulation
//

namespace json {

// Parse flags for yyjson_write
enum class WriteFlag {
  Pretty = 1,
  Compact = 0
};

struct JsonValue {
  JsonValue() = default;
  ~JsonValue() = default;
  explicit JsonValue(yyjson_val *value) : value_(value) {}

  bool is_arr() const { return value_ && yyjson_is_arr(value_); }
  bool is_object() const { return value_ && yyjson_is_obj(value_); }
  bool is_valid() const { return value_ != nullptr; }
  bool get_str(const std::string &key, std::string &out) const;
  bool get_int(const std::string &key, int &out) const;
  bool get_float(const std::string &key, float &out) const;
  JsonValue get(const std::string &key) const;

  private:
  yyjson_val *value_;
};

struct JsonMutValue {
  JsonMutValue() = default;
  ~JsonMutValue() = default;

  explicit JsonMutValue(yyjson_mut_doc *doc, yyjson_mut_val *value)
    : doc_(doc)
    , value_(value) {
  }

  bool is_valid() const { return value_ != nullptr; }
  bool set_str(const std::string &key, const std::string &value);

  private:
  yyjson_mut_doc *doc_;
  yyjson_mut_val *value_;
};

class JsonDoc {
public:
  // Constructors
  JsonDoc() = default;
  explicit JsonDoc(yyjson_doc *doc) : doc_(doc) {}

  // Constructor that parses from string
  static JsonDoc parse(const std::string &json_str);

  // Destructor - frees the yyjson_doc
  ~JsonDoc() = default;

  // Non-copyable but movable
  JsonDoc(const JsonDoc&) = delete;
  JsonDoc &operator=(const JsonDoc&) = delete;
  JsonDoc(JsonDoc &&other) noexcept : doc_(other.doc_) { other.doc_ = nullptr; }
  JsonDoc &operator=(JsonDoc &&other) noexcept {
    if (this != &other) {
      if (doc_) yyjson_doc_free(doc_);
      doc_ = other.doc_;
      other.doc_ = nullptr;
    }
    return *this;
  }

  JsonValue get_root() const;
  std::string write(WriteFlag flags) const;
  bool write_file(const std::string &path, WriteFlag flags) const;
  bool valid() const { return doc_ != nullptr; }

private:
  yyjson_doc *doc_ = nullptr;
};

// Mutable veneer class around yyjson_mut_doc for building/modifying JSON
class JsonMutDoc {
public:
  // Constructors
  JsonMutDoc() = default;
  explicit JsonMutDoc(yyjson_mut_doc *doc) : doc_(doc) {}

  // Constructor that parses from string
  static JsonMutDoc parse(const std::string &json_str);

  // Destructor - frees the yyjson_mut_doc
  ~JsonMutDoc() = default;

  // Non-copyable but movable
  JsonMutDoc(const JsonMutDoc&) = delete;
  JsonMutDoc &operator=(const JsonMutDoc &) = delete;
  JsonMutDoc(JsonMutDoc &&other) noexcept : doc_(other.doc_) { other.doc_ = nullptr; }
  JsonMutDoc &operator=(JsonMutDoc &&other) noexcept {
    if (this != &other) {
      if (doc_) yyjson_mut_doc_free(doc_);
      doc_ = other.doc_;
      other.doc_ = nullptr;
    }
    return *this;
  }

  JsonMutValue get_root() const;
  std::string write(WriteFlag flags = WriteFlag::Compact) const;
  bool valid() const { return doc_ != nullptr; }

private:
  yyjson_mut_doc *doc_ = nullptr;
};

/**
 * Create a JsonDoc from a string
 * @param json_str The JSON string
 * @return JsonDoc wrapper around the parsed document
 */
JsonDoc parse(const std::string &json_str);

/**
 * Create a JsonMutDoc from a string
 * @param json_str The JSON string
 * @return JsonMutDoc wrapper around the parsed mutable document
 */
JsonMutDoc parse_mutable(const std::string &json_str);

// String utility functions (inline implementations in header)

// inline std::string to_string(const JsonMutDoc &doc) {
//   std::string result;
//   if (doc.valid()) {
//     char *buffer = yyjson_mut_write(doc.get(), 0, nullptr);
//     if (buffer) {
//       result = std::string(buffer);
//       free(buffer);
//     }
//   }
//   return result;
// }

// inline std::string to_string(const JsonDoc &doc, WriteFlag flags) {
//   std::string result;
//   if (doc.valid()) {
//     char *buffer = yyjson_write(doc.get(), static_cast<yyjson_write_flag>(flags), nullptr);
//     if (buffer) {
//       result = std::string(buffer);
//       free(buffer);
//     }
//   }
//   return result;
// }

// inline std::string to_string(const JsonMutDoc &doc, WriteFlag flags) {
//   std::string result;
//   if (doc.valid()) {
//     char *buffer = yyjson_mut_write(doc.get(), static_cast<yyjson_write_flag>(flags), nullptr);
//     if (buffer) {
//       result = std::string(buffer);
//       free(buffer);
//     }
//   }
//   return result;
// }

// Parsing functions

// // Read/write convenience functions

// /**
//  * Parse JSON from a string and return a JsonDoc
//  * @param json_str The JSON string
//  * @return JsonDoc wrapper around the parsed document
//  */
// JsonDoc parse_string(const std::string &json_str);

// /**
//  * Write JsonDoc to string
//  * @param doc The JsonDoc to convert
//  * @param flags Write flags
//  * @return JSON string
//  */
// std::string write(const JsonDoc &doc, WriteFlag flags = WriteFlag::Compact);

// /**
//  * Write JsonMutDoc to string
//  * @param doc The JsonMutDoc to convert
//  * @param flags Write flags
//  * @return JSON string
//  */
// std::string write_mutable(const JsonMutDoc &doc, WriteFlag flags = WriteFlag::Compact);

// /**
//  * Write JsonDoc to file
//  * @param path Path to the file
//  * @param doc The JsonDoc to write
//  * @param flags Write flags
//  * @return true if successful, false otherwise
//  */
// bool write_file(const std::string &path, const JsonDoc &doc, WriteFlag flags = WriteFlag::Compact);

// /**
//  * Write JsonMutDoc to file
//  * @param path Path to the file
//  * @param doc The JsonMutDoc to write
//  * @param flags Write flags
//  * @return true if successful, false otherwise
//  */
// bool write_file_mutable(const std::string &path, const JsonMutDoc &doc, WriteFlag flags = WriteFlag::Compact);

// /**
//  * Read JSON from a file into a JsonDoc
//  * @param path Path to the JSON file
//  * @return JsonDoc wrapper around the parsed document
//  */
// JsonDoc read_file(const std::string &path);

// /**
//  * Check if a JSON file exists
//  * @param path Path to the file
//  * @return true if file exists, false otherwise
//  */
// bool file_exists(const std::string &path);

// /**
//  * Validate JSON syntax
//  * @param doc The JsonDoc to validate
//  * @return true if valid, false otherwise
//  */
// bool validate(const JsonDoc &doc);

// // Type checking functions

// /**
//  * Check if JSON is a valid object
//  * @param doc The JsonDoc to check
//  * @return true if object, false otherwise
//  */
// bool is_object(const JsonDoc &doc);

// /**
//  * Check if JSON is a valid array
//  * @param doc The JsonDoc to check
//  * @return true if array, false otherwise
//  */
// bool is_array(const JsonDoc &doc);

// /**
//  * Check if JSON is a valid string
//  * @param doc The JsonDoc to check
//  * @return true if string, false otherwise
//  */
// bool is_string(const JsonDoc &doc);

// /**
//  * Check if JSON is a valid number
//  * @param doc The JsonDoc to check
//  * @return true if number, false otherwise
//  */
// bool is_number(const JsonDoc &doc);

// /**
//  * Check if JSON is a valid boolean
//  * @param doc The JsonDoc to check
//  * @return true if boolean, false otherwise
//  */
// bool is_bool(const JsonDoc &doc);

// /**
//  * Check if JSON is null
//  * @param doc The JsonDoc to check
//  * @return true if null, false otherwise
//  */
// bool is_null(const JsonDoc &doc);

// // Value access functions - work with JsonDoc


// /**
//  * Get a boolean value from a JsonDoc by key
//  * @param doc The JsonDoc
//  * @param key The key to look up
//  * @param out Output parameter for the boolean value
//  * @return true if successful, false otherwise
//  */
// bool get_bool(const JsonDoc &doc,
//               const std::string &key,
//               bool &out);

// /**
//  * Get an array of strings from a JsonDoc by key
//  * @param doc The JsonDoc
//  * @param key The key to look up
//  * @param out Output parameter for the array
//  * @return true if successful, false otherwise
//  */
// bool get_array(const JsonDoc &doc,
//                const std::string &key,
//                std::vector<std::string> &out);

// /**
//  * Get all keys from a JsonDoc
//  * @param doc The JsonDoc
//  * @param out Output parameter for the keys
//  * @return true if successful, false otherwise
//  */
// bool get_keys(const JsonDoc &doc,
//               std::vector<std::string> &out);

// /**
//  * Get a value as raw yyjson_val pointer from JsonDoc
//  * @param doc The JsonDoc
//  * @param key The key to look up
//  * @return yyjson_val pointer (nullptr if not found)
//  */
// JsonValue get_value(const JsonDoc &doc,
//                     const std::string &key);

// /**
//  * Get the length of an array in JsonDoc
//  * @param doc The JsonDoc
//  * @param key The key to look up
//  * @param out Output parameter for the array length
//  * @return true if successful, false otherwise
//  */
// bool get_length(const JsonDoc &doc,
//                 const std::string &key,
//                 size_t &out);

// /**
//  * Get the last element of an array in JsonDoc
//  * @param doc The JsonDoc
//  * @param key The key to look up
//  * @param out Output parameter for the last element
//  * @return true if successful, false otherwise
//  */
// bool get_last(const JsonDoc &doc,
//               const std::string &key,
//               std::string &out);

// /**
//  * Get the first element of an array in JsonDoc
//  * @param doc The JsonDoc
//  * @param key The key to look up
//  * @param out Output parameter for the first element
//  * @return true if successful, false otherwise
//  */
// bool get_first(const JsonDoc &doc,
//                const std::string &key,
//                std::string &out);

// /**
//  * Get an element by index from an array in JsonDoc
//  * @param doc The JsonDoc
//  * @param key The key to look up
//  * @param index The index of the element
//  * @param out Output parameter for the element
//  * @return true if successful, false otherwise
//  */
// bool get_index(const JsonDoc &doc,
//                const std::string &key,
//                size_t index,
//                std::string &out);

// /**
//  * Check if JsonDoc has a specific key
//  * @param doc The JsonDoc
//  * @param key The key to check
//  * @return true if key exists, false otherwise
//  */
// bool has_key(const JsonDoc &doc,
//              const std::string &key);

// /**
//  * Check if a key exists and is a valid string
//  * @param doc The JsonDoc
//  * @param key The key to check
//  * @return true if key exists and is a string, false otherwise
//  */
// bool has_string_key(const JsonDoc &doc,
//                     const std::string &key);

// /**
//  * Check if a key exists and is a valid array
//  * @param doc The JsonDoc
//  * @param key The key to check
//  * @return true if key exists and is an array, false otherwise
//  */
// bool has_array_key(const JsonDoc &doc,
//                    const std::string &key);

// // Mutable value access functions - work with JsonMutDoc
// // These combine type checking with extraction to reduce boilerplate

// /**
//  * Get a string from a mutable JSON object by key
//  * Combines yyjson_mut_obj_get + yyjson_mut_is_str into one call
//  * @param doc The JsonMutDoc
//  * @param key The key to look up
//  * @param out Output parameter for the string value
//  * @return true if successful, false otherwise
//  */
// bool get_str_mut(JsonMutDoc &doc,
//                  const std::string &key,
//                  std::string &out);

// /**
//  * Get an integer from a mutable JSON object by key
//  * @param doc The JsonMutDoc
//  * @param key The key to look up
//  * @param out Output parameter for the integer value
//  * @return true if successful, false otherwise
//  */
// bool get_int_mut(JsonMutDoc &doc,
//                  const std::string &key,
//                  int &out);

// /**
//  * Get a float from a mutable JSON object by key
//  * @param doc The JsonMutDoc
//  * @param key The key to look up
//  * @param out Output parameter for the float value
//  * @return true if successful, false otherwise
//  */
// bool get_float_mut(JsonMutDoc &doc,
//                     const std::string &key,
//                     float &out);

// /**
//  * Get a boolean from a mutable JSON object by key
//  * @param doc The JsonMutDoc
//  * @param key The key to look up
//  * @param out Output parameter for the boolean value
//  * @return true if successful, false otherwise
//  */
// bool get_bool_mut(JsonMutDoc &doc,
//                   const std::string &key,
//                   bool &out);

// /**
//  * Get an array from a mutable JSON object by key
//  * @param doc The JsonMutDoc
//  * @param key The key to look up
//  * @param out Output parameter for the array
//  * @return true if successful, false otherwise
//  */
// bool get_array_mut(JsonMutDoc &doc,
//                    const std::string &key,
//                    std::vector<std::string> &out);

// /**
//  * Get an object from a mutable JSON object by key
//  * @param doc The JsonMutDoc
//  * @param key The key to look up
//  * @param out Output parameter for the yyjson_mut_val pointer
//  * @return true if successful, false otherwise
//  */
// bool get_obj_mut(JsonMutDoc &doc,
//                  const std::string &key,
//                  yyjson_mut_val** out);

// /**
//  * Check if a key exists and is a valid string in mutable JSON
//  * @param doc The JsonMutDoc
//  * @param key The key to check
//  * @return true if key exists and is a string, false otherwise
//  */
// bool has_str_mut(JsonMutDoc &doc,
//                  const std::string &key);

// /**
//  * Check if a key exists and is a valid object in mutable JSON
//  * @param doc The JsonMutDoc
//  * @param key The key to check
//  * @return true if key exists and is an object, false otherwise
//  */
// bool has_obj_mut(JsonMutDoc &doc,
//                  const std::string &key);

// // Mutation functions - work with JsonMutDoc

// /**
//  * Set a string value in a JsonMutDoc
//  * @param doc The JsonMutDoc
//  * @param key The key to set
//  * @param value The string value
//  * @return true if successful, false otherwise
//  */
// bool set_string(JsonMutDoc &doc,
//                 const std::string &key,
//                 const std::string &value);

// // Type checking for mutable JSON

// /**
//  * Check if mutable JSON is a valid object
//  * @param doc The JsonMutDoc to check
//  * @return true if object, false otherwise
//  */
// bool is_object_mut(const JsonMutDoc &doc);

// /**
//  * Check if mutable JSON is a valid array
//  * @param doc The JsonMutDoc to check
//  * @return true if array, false otherwise
//  */
// bool is_array_mut(const JsonMutDoc &doc);

// /**
//  * Check if mutable JSON is a valid string
//  * @param doc The JsonMutDoc to check
//  * @return true if string, false otherwise
//  */
// bool is_string_mut(const JsonMutDoc &doc);

// /**
//  * Check if mutable JSON is a valid number
//  * @param doc The JsonMutDoc to check
//  * @return true if number, false otherwise
//  */
// bool is_number_mut(const JsonMutDoc &doc);

// /**
//  * Check if mutable JSON is a valid boolean
//  * @param doc The JsonMutDoc to check
//  * @return true if boolean, false otherwise
//  */
// bool is_bool_mut(const JsonMutDoc &doc);

// /**
//  * Check if mutable JSON is null
//  * @param doc The JsonMutDoc to check
//  * @return true if null, false otherwise
//  */
// bool is_null_mut(const JsonMutDoc &doc);

// yyjson_val *get_dotted(yyjson_val *obj, const char *path) {
//     char buf[256];
//     strncpy(buf, path, sizeof(buf));
//     char *tok = strtok(buf, ".");
//     yyjson_val *cur = obj;
//     while (tok && cur) {
//         cur = yyjson_obj_get(cur, tok);
//         tok = strtok(NULL, ".");
//     }
//     return cur;
// }



} // namespace json

