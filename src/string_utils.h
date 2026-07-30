// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>
#include <string_view>

namespace utils {

// Check if string starts with a prefix
inline bool starts_with(const std::string &s, const std::string &prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

// Check if string contains only whitespace
inline bool is_blank(const std::string &s) {
  for (char c : s) if (!isspace(static_cast<unsigned char>(c))) return false;
  return true;
}

//
// Helper: is the character a word character?
//
static bool is_word_char(const unsigned char c) {
  return isalnum(c) || c == '_';
}

//
// Trims whitespace from both ends of a string
//
static std::string trim(std::string_view str) {
  constexpr std::string_view whitespace = " \t\n\r\f\v";

  // Find the first non-whitespace character
  const auto start = str.find_first_not_of(whitespace);
  if (start == std::string_view::npos) {
    return ""; // The string is entirely whitespace
  }

  // Find the last non-whitespace character
  const auto end = str.find_last_not_of(whitespace);

  // Return the substring between start and end
  return std::string(str.substr(start, end - start + 1));
}

} // namespace utils

