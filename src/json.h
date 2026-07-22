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

// Parsing functions

/**
 * Extract a string value from a JSON object by key
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the string value
 * @return true if successful, false otherwise
 */
bool get_string(const std::string &json,
                const std::string &key,
                std::string       &out);

/**
 * Extract an integer value from a JSON object by key
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the integer value
 * @return true if successful, false otherwise
 */
bool get_int(const std::string &json,
             const std::string &key,
             int             &out);

/**
 * Extract a float value from a JSON object by key
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the float value
 * @return true if successful, false otherwise
 */
bool get_float(const std::string &json,
               const std::string &key,
               float            &out);

/**
 * Extract a boolean value from a JSON object by key
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the boolean value
 * @return true if successful, false otherwise
 */
bool get_bool(const std::string &json,
              const std::string &key,
              bool             &out);

/**
 * Extract an array of strings from a JSON object by key
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the array
 * @return true if successful, false otherwise
 */
bool get_array(const std::string &json,
               const std::string &key,
               std::vector<std::string> &out);

/**
 * Extract an object as a map of strings from a JSON object by key
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the object
 * @return true if successful, false otherwise
 */
bool get_object(const std::string &json,
                const std::string &key,
                std::map<std::string, std::string> &out);

/**
 * Get all keys from a JSON object
 * @param json JSON string
 * @param out Output parameter for the keys
 * @return true if successful, false otherwise
 */
bool get_keys(const std::string &json,
              std::vector<std::string> &out);

/**
 * Get a value as raw yyjson_val pointer
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the yyjson_val pointer
 * @return true if successful, false otherwise
 */
bool get_value(const std::string &json,
               const std::string &key,
               yyjson_val **out);

/**
 * Get the length of an array
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the array length
 * @return true if successful, false otherwise
 */
bool get_length(const std::string &json,
                const std::string &key,
                size_t            &out);

/**
 * Get the last element of an array
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the last element
 * @return true if successful, false otherwise
 */
bool get_last(const std::string &json,
              const std::string &key,
              std::string       &out);

/**
 * Get the first element of an array
 * @param json JSON string
 * @param key The key to look up
 * @param out Output parameter for the first element
 * @return true if successful, false otherwise
 */
bool get_first(const std::string &json,
               const std::string &key,
               std::string       &out);

/**
 * Get an element by index from an array
 * @param json JSON string
 * @param key The key to look up
 * @param index The index of the element
 * @param out Output parameter for the element
 * @return true if successful, false otherwise
 */
bool get_index(const std::string &json,
               const std::string &key,
               size_t            index,
               std::string       &out);

// Mutation functions

/**
 * Set a string value in a JSON object
 * @param json JSON string
 * @param key The key to set
 * @param value The string value
 * @return true if successful, false otherwise
 */
bool set_string(const std::string &json,
                const std::string &key,
                const std::string &value);

/**
 * Set an integer value in a JSON object
 * @param json JSON string
 * @param key The key to set
 * @param value The integer value
 * @return true if successful, false otherwise
 */
bool set_int(const std::string &json,
             const std::string &key,
             int             value);

/**
 * Set a float value in a JSON object
 * @param json JSON string
 * @param key The key to set
 * @param value The float value
 * @return true if successful, false otherwise
 */
bool set_float(const std::string &json,
               const std::string &key,
               float            value);

/**
 * Set a boolean value in a JSON object
 * @param json JSON string
 * @param key The key to set
 * @param value The boolean value
 * @return true if successful, false otherwise
 */
bool set_bool(const std::string &json,
              const std::string &key,
              bool             value);

/**
 * Set an array of strings in a JSON object
 * @param json JSON string
 * @param key The key to set
 * @param values The array of strings
 * @return true if successful, false otherwise
 */
bool set_array(const std::string &json,
               const std::string &key,
               const std::vector<std::string> &values);

/**
 * Append a value to an array in a JSON object
 * @param json JSON string
 * @param key The key containing the array
 * @param value The value to append
 * @return true if successful, false otherwise
 */
bool append_array(const std::string &json,
                  const std::string &key,
                  const std::string &value);

/**
 * Remove a key from a JSON object
 * @param json JSON string
 * @param key The key to remove
 * @return true if successful, false otherwise
 */
bool remove(const std::string &json,
            const std::string &key);

/**
 * Remove an element by index from an array
 * @param json JSON string
 * @param key The key containing the array
 * @param index The index of the element to remove
 * @return true if successful, false otherwise
 */
bool remove_index(const std::string &json,
                  const std::string &key,
                  size_t            index);

/**
 * Insert a value at a specific index in an array
 * @param json JSON string
 * @param key The key containing the array
 * @param index The index to insert at
 * @param value The value to insert
 * @return true if successful, false otherwise
 */
bool insert(const std::string &json,
            const std::string &key,
            size_t            index,
            const std::string &value);

/**
 * Merge an overlay JSON object into a base JSON object
 * @param base_json The base JSON object
 * @param overlay_json The overlay JSON object
 * @param out Output parameter for the merged result
 * @return true if successful, false otherwise
 */
bool merge(const std::string &base_json,
           const std::string &overlay_json,
           std::string &out);

/**
 * Create a deep copy of a JSON object
 * @param json JSON string
 * @param out Output parameter for the copied JSON
 * @return true if successful, false otherwise
 */
bool deep_copy(const std::string &json,
               std::string &out);

/**
 * Filter an array based on a predicate
 * @param json JSON string
 * @param key The key containing the array
 * @param predicate The filtering predicate
 * @param out Output parameter for the filtered array
 * @return true if successful, false otherwise
 */
bool filter(const std::string &json,
            const std::string &key,
            std::function<bool(const std::string &)> predicate,
            std::string &out);

// Type checking functions

/**
 * Validate JSON syntax
 * @param json JSON string
 * @return true if valid, false otherwise
 */
bool validate(const std::string &json);

/**
 * Check if JSON is a valid object
 * @param json JSON string
 * @return true if object, false otherwise
 */
bool is_object(const std::string &json);

/**
 * Check if JSON is a valid array
 * @param json JSON string
 * @return true if array, false otherwise
 */
bool is_array(const std::string &json);

/**
 * Check if JSON is a valid string
 * @param json JSON string
 * @return true if string, false otherwise
 */
bool is_string(const std::string &json);

/**
 * Check if JSON is a valid number
 * @param json JSON string
 * @return true if number, false otherwise
 */
bool is_number(const std::string &json);

/**
 * Check if JSON is a valid boolean
 * @param json JSON string
 * @return true if boolean, false otherwise
 */
bool is_bool(const std::string &json);

/**
 * Check if JSON is null
 * @param json JSON string
 * @return true if null, false otherwise
 */
bool is_null(const std::string &json);

/**
 * Check if JSON has a specific key
 * @param json JSON string
 * @param key The key to check
 * @return true if key exists, false otherwise
 */
bool has_key(const std::string &json,
             const std::string &key);

// File I/O functions

/**
 * Read a JSON file into a string
 * @param path Path to the JSON file
 * @param out Output parameter for the JSON content
 * @return true if successful, false otherwise
 */
bool read_file(const std::string &path,
               std::string &out);

/**
 * Write JSON to a file
 * @param path Path to the file
 * @param content The JSON content to write
 * @return true if successful, false otherwise
 */
bool write_file(const std::string &path,
                const std::string &content);

/**
 * Check if a JSON file exists
 * @param path Path to the file
 * @return true if file exists, false otherwise
 */
bool exists(const std::string &path);

// Utility functions

/**
 * Parse JSON from a string and return a yyjson_doc pointer
 * @param json_str The JSON string
 * @param out Output parameter for the yyjson_doc pointer
 * @return true if successful, false otherwise
 */
bool parse(const std::string &json_str,
           yyjson_doc **out);

/**
 * Convert yyjson_doc to JSON string
 * @param doc The yyjson_doc to convert
 * @param flags Write flags
 * @param out Output parameter for the JSON string
 * @return true if successful, false otherwise
 */
bool write(const yyjson_doc *doc,
           yyjson_write_flag flags,
           std::string &out);

/**
 * Convert mutable yyjson_mut_doc to JSON string
 * @param doc The mutable yyjson_mut_doc to convert
 * @param flags Write flags
 * @param out Output parameter for the JSON string
 * @return true if successful, false otherwise
 */
bool write_mutable(yyjson_mut_doc *doc,
                   yyjson_write_flag flags,
                   std::string &out);

} // namespace json