// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <cctype>
#include <cstdint>

#include "utf8.h"
#include "string_utils.h"

namespace utils {

std::string trim(const std::string_view str) {
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

bool is_word_boundary(char32_t cp) {
  if (std::isspace(static_cast<unsigned char>(cp))) {
    return true;
  }
  if (cp >= 0x0020 && cp <= 0x002F) {
    return true;
  }
  if (cp >= 0x003A && cp <= 0x0040) {
    return true;
  }
  if (cp >= 0x005B && cp <= 0x0060) {
    return true;
  }
  if (cp >= 0x007B && cp <= 0x007E) {
    return true;
  }
  return false;
}

std::vector<std::string> split_utf8_string(const std::string &input, size_t max_chars_per_segment) {
  std::vector<std::string> result;

  //
  // cats and \ndogsrabbits  | line break
  // catssandssdogsrabbits   | no space to break
  // cats and dogsrabbits    | break on space
  // cats and dogs rabbits   | break on space
  // [----^---^----^--]
  //               ^  ^-- pending
  //               |----- current
  std::string current;
  std::string pending;

  unsigned count = 0;
  auto it = input.begin();
  const auto end = input.end();

  auto push_text = [&]() -> void {
    current.append(pending);
    if (!is_blank(current)) {
      result.push_back(current);
    }
    current.clear();
    pending.clear();
    count = 0;
  };

  while (it != end) {
    const char32_t code_point = utf8::next(it, end);
    if (code_point == '\n' || code_point == '\r') {
      push_text();
      continue;
    }
    utf8::append(code_point, pending);
    ++count;
    if (count >= max_chars_per_segment) {
      if (is_word_boundary(code_point)) {
        push_text();
      } else if (current.empty()) {
        result.push_back(pending);
        pending.clear();
        count = 0;
      } else if (!is_blank(current)) {
        result.push_back(current);
        current.clear();
      }
      if (is_word_boundary(utf8::peek_next(it, end))) {
        // discard leading whitespace on next line
        utf8::next(it, end);
      }
    } else if (is_word_boundary(code_point)) {
      current.append(pending);
      pending.clear();
    }
  }
  push_text();
  return result;
}

}
