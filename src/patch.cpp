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
#include <iostream>
#include <regex>
#include <algorithm>
#include <stdexcept>

#include "patch.h"

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
  std::string current_block;

  for (size_t i = 0; i < patch_str.length(); ++i) {
    if (patch_str.substr(i, 6) == "<<<<<<<") {
      in_old = true;
      in_new = false;
      i += 6;
      while (i < patch_str.length() && patch_str[i] != '\n') i++;
      if (i < patch_str.length()) i++; // skip newline
      continue;
    }

    if (patch_str.substr(i, 4) == "=====") {
      in_old = false;
      in_new = true;
      i += 4;
      while (i < patch_str.length() && patch_str[i] != '\n') i++;
      if (i < patch_str.length()) i++; // skip newline
      continue;
    }

    if (patch_str.substr(i, 9) == ">>>>>>>") {
      in_new = false;
      i += 9;
      while (i < patch_str.length() && patch_str[i] != '\n') i++;
      if (i < patch_str.length()) i++; // skip newline
      continue;
    }

    if (in_old) {
      current_block += patch_str[i];
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
// Main patch function
//
const std::string tool_patch(const std::string& filename, const std::string& patch_str) {
  // Read the target file
  std::ifstream file(filename);
  if (!file) {
    return "ERROR: Cannot open file " + filename + " for reading.";
  }
  std::string file_content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
  file.close();

  // Check for conflict markers in the file itself  
  if (file_content.find("<<<<<<<") != std::string::npos ||
      file_content.find("=======") != std::string::npos ||
      file_content.find(">>>>>>>") != std::string::npos) {
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

