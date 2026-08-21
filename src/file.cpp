// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>

#include "file.h"

namespace fs = std::filesystem;

static const std::vector<std::string> curlyBraceExtensions = {
  ".js", ".jsx", ".ts", ".tsx", ".java", ".c", ".cpp", ".h", ".hpp"
};

static bool isCurlyBraceLanguage(const fs::path& path) {
  const std::string ext = path.extension().string();
  for (const auto& e : curlyBraceExtensions) {
    if (ext == e) return true;
  }
  return false;
}

//
// Check if a string has balanced braces and parentheses
//
static bool isBalanced(const std::string& code) {
  int braceCount = 0;
  int parenCount = 0;
  bool inString = false;
  char stringChar = 0;

  for (size_t i = 0; i < code.length(); ++i) {
    char c = code[i];

    // Handle string literals
    if (!inString && (c == '"' || c == '\'')) {
      inString = true;
      stringChar = c;
      continue;
    }

    if (inString) {
      if (c == stringChar && (i == 0 || code[i-1] != '\\')) {
        inString = false;
        stringChar = 0;
      }
      continue;
    }

    if (c == '{') braceCount++;
    else if (c == '}') braceCount--;
    else if (c == '(') parenCount++;
    else if (c == ')') parenCount--;

    if (braceCount < 0 || parenCount < 0) return false;
  }

  return braceCount == 0 && parenCount == 0;
}

//
// Parse patch_str to extract OLD and NEW blocks
//
static std::pair<std::string, std::string> parsePatch(const std::string& patch_str) {
  std::string old_block;
  std::string new_block;
  bool in_old = false;
  bool in_new = false;

  for (size_t i = 0; i < patch_str.length(); ++i) {
    if (patch_str.substr(i, PATCH_BEGIN.length()) == PATCH_BEGIN) {
      in_old = true;
      in_new = false;
      i += PATCH_BEGIN.length();
      while (i < patch_str.length() && patch_str[i] != '\n') i++;
      continue;
    }

    if (patch_str.substr(i, PATCH_BOUNDARY.length()) == PATCH_BOUNDARY) {
      in_old = false;
      in_new = true;
      i += PATCH_BOUNDARY.length();
      while (i < patch_str.length() && patch_str[i] != '\n') i++;
      continue;
    }

    if (patch_str.substr(i, PATCH_END.length()) == PATCH_END) {
      in_new = false;
      i += PATCH_END.length();
      while (i < patch_str.length() && patch_str[i] != '\n') i++;
      continue;
    }

    if (in_old) {
      old_block += patch_str[i];
    } else if (in_new) {
      new_block += patch_str[i];
    }
  }

  return {old_block, new_block};
}

//
// Count occurrences of a string in a text
//
static size_t countOccurrences(const std::string& text, const std::string& pattern) {
  size_t count = 0;
  size_t pos = 0;
  while ((pos = text.find(pattern, pos)) != std::string::npos) {
    count++;
    pos += pattern.length();
  }
  return count;
}

//
// Replace all occurrences of old_block with new_block in text
//
static std::string replaceAll(std::string text, const std::string& old_block, const std::string& new_block) {
  size_t pos = 0;
  while ((pos = text.find(old_block, pos)) != std::string::npos) {
    text.replace(pos, old_block.length(), new_block);
    pos += new_block.length();
  }
  return text;
}

//
// Append data to a file
// Similar to tool_write but uses append mode
//
std::string tool_append(const std::string &path, const std::string &data) {
  fs::path p(path);

  // Validation checks for curly brace languages
  // Check if it's a curly brace language and content is unbalanced
  if (isCurlyBraceLanguage(p) && !isBalanced(data)) {
    return "ERROR: File appears to be a curly brace language with unbalanced braces";
  }

  // Create parent directories if needed
  if (p.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
  }

  // Open file in append mode
  std::ofstream f(path, std::ios::binary | std::ios::app);

  if (f) {
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
  }
  return f && f.good() ? "OK: appended to " + path : "ERROR: append failed for " + path;
}

//
// Main patch function
//
std::string tool_patch(const std::string& filename, const std::string& patch_str) {
  // Read the target file
  std::ifstream file(filename);
  if (!file) {
    return "ERROR: Cannot open file " + filename + " for reading.";
  }
  std::string file_content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
  file.close();

  // Check for conflict markers in the file itself
  if (file_content.find(PATCH_BEGIN) != std::string::npos ||
      file_content.find(PATCH_BOUNDARY) != std::string::npos ||
      file_content.find(PATCH_END) != std::string::npos) {
    return "ERROR: File contains conflict markers (<<<<<<</=======/>>>>>>>). Cannot patch.";
  }

  // Parse the patch
  auto [old_block, new_block] = parsePatch(patch_str);

  // Validate OLD block is not empty
  if (old_block.empty()) {
    return "ERROR: OLD block is empty. Cannot patch.";
  }

  // Validate NEW block is not empty
  if (new_block.empty()) {
    return "ERROR: NEW block is empty. Cannot patch.";
  }

  // Validate NEW block has balanced braces/parentheses
  if (!isBalanced(new_block)) {
    return "ERROR: NEW block has unbalanced braces or parentheses. Check syntax before patching.";
  }

  // Search for OLD block as exact, single match
  size_t count = countOccurrences(file_content, old_block);

  if (count == 0) {
    return "ERROR: OLD block not found verbatim in " + filename + " — re-quote it exactly";
  }

  if (count > 1) {
    return "ERROR: OLD block found " + std::to_string(count) + " times in " + filename + ". Cannot determine which to replace.";
  }

  // Apply the patch
  std::string patched_content = replaceAll(file_content, old_block, new_block);

  // Write the patched content back to the file
  std::ofstream out_file(filename);
  if (!out_file) {
    return "ERROR: Cannot write to file " + filename;
  }
  out_file << patched_content;
  out_file.close();

  return "SUCCESS: Patch applied to " + filename;
}

//
// tool_write_validate
//
std::string tool_write_validate(const std::string &path, const std::string &data) {
  std::string result;
  fs::path p(path);

  if (fs::exists(p)) {
    auto old_size = fs::file_size(p);
    if (old_size > 0 && data.size() < old_size * 0.5) {
      result = "Warning: content shrinks by more than 50%";
    } else if (old_size > 1000) {
      result = "Warning: old content is > 1000 bytes";
    }
  }

  if (isCurlyBraceLanguage(p) && !isBalanced(data)) {
    result  = "Warning: file has unbalanced braces";
  }

  return result;
}

//
// tool_write
//
std::string tool_write(const std::string &path, const std::string &data) {
  fs::path p(path);

  // Create parent directories if needed
  if (p.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
  }

  // Write the file
  std::ofstream f(path, std::ios::binary | std::ios::trunc);

  if (f) {
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
  }
  return f && f.good() ? "OK: written to " + path : "ERROR: write failed for " + path;
}

