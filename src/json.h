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
#include <yyjson.h>

// JSON dependency inversion layer header
// Provides a C++ wrapper around yyjson for JSON parsing and manipulation

namespace json {

// Forward declarations for type aliases
using JsonString = std::string;
using JsonArray = std::vector<std::string>;
using JsonObject = std::map<std::string, std::string>;

// Parse flags for yyjson_write
enum class WriteFlag {
  Pretty = 1,
  Compact = 0
};

// Veneer class around yyjson_doc for abstraction
class JsonDoc {
public:
  // Constructors
  JsonDoc() = default;
  explicit JsonDoc(yyjson_doc* doc) : doc_(doc) {}

  // Constructor that parses from string
  static JsonDoc parse(const std::string& json_str);

  // Destructor - frees the yyjson_doc
  ~JsonDoc() = default;

  // Non-copyable but movable
  JsonDoc(const JsonDoc&) = delete;
  JsonDoc& operator=(const JsonDoc&) = delete;
  JsonDoc(JsonDoc&& other) noexcept : doc_(other.doc_) { other.doc_ = nullptr; }
  JsonDoc& operator=(JsonDoc&& other) noexcept {
    if (this != &other) {
      if (doc_) yyjson_doc_free(doc_);
      doc_ = other.doc_;
      other.doc_ = nullptr;
    }
    return *this;
  }

  // Check if doc is valid
  bool valid() const { return doc_ != nullptr; }

  // Get raw yyjson_doc pointer
  yyjson_doc* get() const { return doc_; }

  // Get root value
  yyjson_val* get_root() const { return doc_ ? yyjson_doc_get_root(doc_) : nullptr; }

private:
  yyjson_doc* doc_ = nullptr;
};

// Mutable veneer class around yyjson_mut_doc for building/modifying JSON
class JsonMutDoc {
public:
  // Constructors
  JsonMutDoc() = default;
  explicit JsonMutDoc(yyjson_mut_doc* doc) : doc_(doc) {}

  // Constructor that parses from string
  static JsonMutDoc parse(const std::string& json_str);

  // Destructor - frees the yyjson_mut_doc
  ~JsonMutDoc() = default;

  // Non-copyable but movable
  JsonMutDoc(const JsonMutDoc&) = delete;
  JsonMutDoc& operator=(const JsonMutDoc&) = delete;
  JsonMutDoc(JsonMutDoc&& other) noexcept : doc_(other.doc_) { other.doc_ = nullptr; }
  JsonMutDoc& operator=(JsonMutDoc&& other) noexcept {
    if (this != &other) {
      if (doc_) yyjson_mut_doc_free(doc_);
      doc_ = other.doc_;
      other.doc_ = nullptr;
    }
    return *this;
  }

  // Check if doc is valid
  bool valid() const { return doc_ != nullptr; }

  // Get raw yyjson_mut_doc pointer
  yyjson_mut_doc* get() const { return doc_; }

  // Get root value
  yyjson_mut_val* get_root() const { return doc_ ? yyjson_mut_doc_get_root(doc_) : nullptr; }

private:
  yyjson_mut_doc* doc_ = nullptr;
};

// String utility functions
inline std::string to_string(const JsonDoc& doc) {
  std::string result;
  if (doc.valid()) {
    char* buffer = yyjson_write(doc.get(), 0);
    if (buffer) {
      result = std::string(buffer);
      free(buffer);
    }
  }
  return result;
}

inline std::string to_string(const JsonMutDoc& doc) {
  std::string result;
  if (doc.valid()) {
    char* buffer = yyjson_mut_write(doc.get(), 0, nullptr);
    if (buffer) {
      result = std::string(buffer);
      free(buffer);
    }
  }
  return result;
}

inline std::string to_string(const JsonDoc& doc, WriteFlag flags) {
  std::string result;
  if (doc.valid()) {
    char* buffer = yyjson_write(doc.get(), static_cast<yyjson_write_flag>(flags));
    if (buffer) {
      result = std::string(buffer);
      free(buffer);
    }
  }
  return result;
}

inline std::string to_string(const JsonMutDoc& doc, WriteFlag flags) {
  std::string result;
  if (doc.valid()) {
    char* buffer = yyjson_mut_write(doc.get(), static_cast<yyjson_write_flag>(flags), nullptr);
    if (buffer) {
      result = std::string(buffer);
      free(buffer);
    }
  }
  return result;
}

// Parsing functions

/**
 * Create a JsonDoc from a string
 * @param json The JSON string
 * @return JsonDoc wrapper around the parsed document
 */
JsonDoc parse(const std::string& json);

/**
 * Create a JsonMutDoc from a string
 * @param json The JSON string
 * @return JsonMutDoc wrapper around the parsed mutable document
 */
JsonMutDoc parse_mutable(const std::string& json);

// Read/write convenience functions

/**
 * Parse JSON from a string and return a JsonDoc
 * @param json_str The JSON string
 * @return JsonDoc wrapper around the parsed document
 */
JsonDoc parse_string(const std::string& json_str);

/**
 * Write JsonDoc to string
 * @param doc The JsonDoc to convert
 * @param flags Write flags
 * @return JSON string
 */
std::string write(const JsonDoc& doc, WriteFlag flags = WriteFlag::Compact);

/**
 * Write JsonMutDoc to string
 * @param doc The JsonMutDoc to convert
 * @param flags Write flags
 * @return JSON string
 */
std::string write_mutable(const JsonMutDoc& doc, WriteFlag flags = WriteFlag::Compact);

/**
 * Write JsonDoc to file
 * @param path Path to the file
 * @param doc The JsonDoc to write
 * @param flags Write flags
 * @return true if successful, false otherwise
 */
bool write_file(const std::string& path, const JsonDoc& doc, WriteFlag flags = WriteFlag::Compact);

/**
 * Write JsonMutDoc to file
 * @param path Path to the file
 * @param doc The JsonMutDoc to write
 * @param flags Write flags
 * @return true if successful, false otherwise
 */
bool write_file_mutable(const std::string& path, const JsonMutDoc& doc, WriteFlag flags = WriteFlag::Compact);

/**
 * Read JSON from a file into a JsonDoc
 * @param path Path to the JSON file
 * @return JsonDoc wrapper around the parsed document
 */
JsonDoc read_file(const std::string& path);

/**
 * Check if a JSON file exists
 * @param path Path to the file
 * @return true if file exists, false otherwise
 */
bool file_exists(const std::string& path);

/**
 * Validate JSON syntax
 * @param doc The JsonDoc to validate
 * @return true if valid, false otherwise
 */
bool validate(const JsonDoc& doc);

/**
 * Deep copy a JsonDoc
 * @param doc The source JsonDoc
 * @return New JsonDoc with deep copy of contents
 */
JsonDoc deep_copy(const JsonDoc& doc);

/**
 * Filter an array in JsonDoc based on a predicate
 * @param doc The JsonDoc containing the array
 * @param key The key containing the array
 * @param predicate The filtering predicate
 * @return New JsonDoc with filtered array
 */
JsonDoc filter(const JsonDoc& doc,
               const std::string& key,
               std::function<bool(const std::string&)> predicate);

/**
 * Merge two JsonDocs
 * @param base_doc The base JsonDoc
 * @param overlay_doc The overlay JsonDoc
 * @return New JsonDoc with merged contents
 */
JsonDoc merge(const JsonDoc& base_doc, const JsonDoc& overlay_doc);

// Type checking functions

/**
 * Check if JSON is a valid object
 * @param doc The JsonDoc to check
 * @return true if object, false otherwise
 */
bool is_object(const JsonDoc& doc);

/**
 * Check if JSON is a valid array
 * @param doc The JsonDoc to check
 * @return true if array, false otherwise
 */
bool is_array(const JsonDoc& doc);

/**
 * Check if JSON is a valid string
 * @param doc The JsonDoc to check
 * @return true if string, false otherwise
 */
bool is_string(const JsonDoc& doc);

/**
 * Check if JSON is a valid number
 * @param doc The JsonDoc to check
 * @return true if number, false otherwise
 */
bool is_number(const JsonDoc& doc);

/**
 * Check if JSON is a valid boolean
 * @param doc The JsonDoc to check
 * @return true if boolean, false otherwise
 */
bool is_bool(const JsonDoc& doc);

/**
 * Check if JSON is null
 * @param doc The JsonDoc to check
 * @return true if null, false otherwise
 */
bool is_null(const JsonDoc& doc);

// Value access functions - work with JsonDoc

/**
 * Get a string value from a JsonDoc by key
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the string value
 * @return true if successful, false otherwise
 */
bool get_string(const JsonDoc& doc,
                const std::string& key,
                std::string& out);

/**
 * Get an integer value from a JsonDoc by key
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the integer value
 * @return true if successful, false otherwise
 */
bool get_int(const JsonDoc& doc,
             const std::string& key,
             int& out);

/**
 * Get a float value from a JsonDoc by key
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the float value
 * @return true if successful, false otherwise
 */
bool get_float(const JsonDoc& doc,
               const std::string& key,
               float& out);

/**
 * Get a boolean value from a JsonDoc by key
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the boolean value
 * @return true if successful, false otherwise
 */
bool get_bool(const JsonDoc& doc,
              const std::string& key,
              bool& out);

/**
 * Get an array of strings from a JsonDoc by key
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the array
 * @return true if successful, false otherwise
 */
bool get_array(const JsonDoc& doc,
               const std::string& key,
               std::vector<std::string>& out);

/**
 * Get an object as a map of strings from a JsonDoc by key
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the object
 * @return true if successful, false otherwise
 */
bool get_object(const JsonDoc& doc,
                const std::string& key,
                std::map<std::string, std::string>& out);

/**
 * Get all keys from a JsonDoc
 * @param doc The JsonDoc
 * @param out Output parameter for the keys
 * @return true if successful, false otherwise
 */
bool get_keys(const JsonDoc& doc,
              std::vector<std::string>& out);

/**
 * Get a value as raw yyjson_val pointer from JsonDoc
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the yyjson_val pointer
 * @return true if successful, false otherwise
 */
bool get_value(const JsonDoc& doc,
               const std::string& key,
               yyjson_val** out);

/**
 * Get the length of an array in JsonDoc
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the array length
 * @return true if successful, false otherwise
 */
bool get_length(const JsonDoc& doc,
                const std::string& key,
                size_t& out);

/**
 * Get the last element of an array in JsonDoc
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the last element
 * @return true if successful, false otherwise
 */
bool get_last(const JsonDoc& doc,
              const std::string& key,
              std::string& out);

/**
 * Get the first element of an array in JsonDoc
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param out Output parameter for the first element
 * @return true if successful, false otherwise
 */
bool get_first(const JsonDoc& doc,
               const std::string& key,
               std::string& out);

/**
 * Get an element by index from an array in JsonDoc
 * @param doc The JsonDoc
 * @param key The key to look up
 * @param index The index of the element
 * @param out Output parameter for the element
 * @return true if successful, false otherwise
 */
bool get_index(const JsonDoc& doc,
               const std::string& key,
               size_t index,
               std::string& out);

/**
 * Check if JsonDoc has a specific key
 * @param doc The JsonDoc
 * @param key The key to check
 * @return true if key exists, false otherwise
 */
bool has_key(const JsonDoc& doc,
             const std::string& key);

/**
 * Check if a key exists and is a valid string
 * @param doc The JsonDoc
 * @param key The key to check
 * @return true if key exists and is a string, false otherwise
 */
bool has_string_key(const JsonDoc& doc,
                    const std::string& key);

/**
 * Check if a key exists and is a valid array
 * @param doc The JsonDoc
 * @param key The key to check
 * @return true if key exists and is an array, false otherwise
 */
bool has_array_key(const JsonDoc& doc,
                   const std::string& key);

// Mutation functions - work with JsonMutDoc

/**
 * Set a string value in a JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key to set
 * @param value The string value
 * @return true if successful, false otherwise
 */
bool set_string(JsonMutDoc& doc,
                const std::string& key,
                const std::string& value);

/**
 * Set an integer value in a JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key to set
 * @param value The integer value
 * @return true if successful, false otherwise
 */
bool set_int(JsonMutDoc& doc,
             const std::string& key,
             int value);

/**
 * Set a float value in a JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key to set
 * @param value The float value
 * @return true if successful, false otherwise
 */
bool set_float(JsonMutDoc& doc,
               const std::string& key,
               float value);

/**
 * Set a boolean value in a JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key to set
 * @param value The boolean value
 * @return true if successful, false otherwise
 */
bool set_bool(JsonMutDoc& doc,
              const std::string& key,
              bool value);

/**
 * Set an array of strings in a JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key to set
 * @param values The array of strings
 * @return true if successful, false otherwise
 */
bool set_array(JsonMutDoc& doc,
               const std::string& key,
               const std::vector<std::string>& values);

/**
 * Append a value to an array in a JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key containing the array
 * @param value The value to append
 * @return true if successful, false otherwise
 */
bool append_array(JsonMutDoc& doc,
                  const std::string& key,
                  const std::string& value);

/**
 * Remove a key from a JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key to remove
 * @return true if successful, false otherwise
 */
bool remove(JsonMutDoc& doc,
            const std::string& key);

/**
 * Remove an element by index from an array in JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key containing the array
 * @param index The index of the element to remove
 * @return true if successful, false otherwise
 */
bool remove_index(JsonMutDoc& doc,
                  const std::string& key,
                  size_t index);

/**
 * Insert a value at a specific index in an array in JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key containing the array
 * @param index The index to insert at
 * @param value The value to insert
 * @return true if successful, false otherwise
 */
bool insert(JsonMutDoc& doc,
            const std::string& key,
            size_t index,
            const std::string& value);

/**
 * Insert an element at the beginning of an array in JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key containing the array
 * @param value The value to insert
 * @return true if successful, false otherwise
 */
bool insert_first(JsonMutDoc& doc,
                  const std::string& key,
                  const std::string& value);

/**
 * Insert an element at the end of an array in JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key containing the array
 * @param value The value to insert
 * @return true if successful, false otherwise
 */
bool insert_last(JsonMutDoc& doc,
                 const std::string& key,
                 const std::string& value);

/**
 * Clear an array in JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key containing the array
 * @return true if successful, false otherwise
 */
bool clear_array(JsonMutDoc& doc,
                 const std::string& key);

/**
 * Remove all elements from an array in JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key containing the array
 * @return true if successful, false otherwise
 */
bool clear_array(JsonMutDoc& doc,
                 const std::string& key) {
  return remove_array(doc, key);
}

/**
 * Remove an array from a JsonMutDoc
 * @param doc The JsonMutDoc
 * @param key The key containing the array
 * @return true if successful, false otherwise
 */
bool remove_array(JsonMutDoc& doc,
                  const std::string& key);

/**
 * Merge an overlay JsonMutDoc into a base JsonMutDoc
 * @param base_doc The base JsonMutDoc
 * @param overlay_doc The overlay JsonMutDoc
 * @return true if successful, false otherwise
 */
bool merge(JsonMutDoc& base_doc,
           const JsonMutDoc& overlay_doc);

} // namespace json
