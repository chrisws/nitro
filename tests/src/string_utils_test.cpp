#include <iostream>
#include <cassert>
#include <string>
#include <string_view>
#include "string_utils.h"

using namespace std;
using namespace utils;

// Test starts_with function
static void test_starts_with() {
  // Basic prefix matches
  assert(starts_with("hello world", "hello") == true);
  assert(starts_with("hello", "hello") == true);
  assert(starts_with("123abc", "123") == true);

  // No prefix match
  assert(starts_with("hello world", "world") == false);
  assert(starts_with("hello", "hello world") == false);
  assert(starts_with("abc", "abcd") == false);

  // Empty prefix
  assert(starts_with("hello", "") == true);
  assert(starts_with("", "") == true);

  // Empty string
  assert(starts_with("", "hello") == false);

  // Case sensitive
  assert(starts_with("Hello", "hello") == false);
  assert(starts_with("HELLO", "hello") == false);

  cout << "test_starts_with passed" << endl;
}

// Test is_blank function
static void test_is_blank() {
  // Whitespace only
  assert(is_blank("   ") == true);
  assert(is_blank("\t\t") == true);
  assert(is_blank("\n\n") == true);
  assert(is_blank(" \t\n\r\f\v") == true);

  // Empty string
  assert(is_blank("") == true);

  // Non-whitespace
  assert(is_blank("hello") == false);
  assert(is_blank("a") == false);
  assert(is_blank("123") == false);

  // Mixed (should be false)
  assert(is_blank("  hello  ") == false);
  assert(is_blank("hello world") == false);

  cout << "test_is_blank passed" << endl;
}

// Test is_word_char function
static void test_is_word_char() {
  // Alphanumeric characters
  assert(is_word_char('a') == true);
  assert(is_word_char('z') == true);
  assert(is_word_char('A') == true);
  assert(is_word_char('Z') == true);
  assert(is_word_char('0') == true);
  assert(is_word_char('9') == true);

  // Underscore
  assert(is_word_char('_') == true);

  // Non-word characters
  assert(is_word_char(' ') == false);
  assert(is_word_char('-') == false);
  assert(is_word_char('.') == false);
  assert(is_word_char('@') == false);
  assert(is_word_char('$') == false);

  cout << "test_is_word_char passed" << endl;
}

// Test trim function
static void test_trim() {
  // Leading and trailing whitespace
  assert(trim("  hello  ") == "hello");
  assert(trim("\thello\t") == "hello");
  assert(trim("\nhello\n") == "hello");

  // Only leading whitespace
  assert(trim("   hello") == "hello");

  // Only trailing whitespace
  assert(trim("hello   ") == "hello");

  // No whitespace
  assert(trim("hello") == "hello");

  // Empty string
  assert(trim("") == "");

  // All whitespace
  assert(trim("   \t\n  ") == "");

  // Mixed whitespace
  assert(trim(" \t  hello  \n\r\t ") == "hello");

  cout << "test_trim passed" << endl;
}

// Test trim with string_view input
static void test_trim_string_view() {
  std::string_view sv = "  test  ";
  assert(trim(sv) == "test");

  std::string_view sv2 = "\t\t";
  assert(trim(sv2) == "");

  std::string_view sv3 = "no spaces";
  assert(trim(sv3) == "no spaces");

  cout << "test_trim_string_view passed" << endl;
}

void string_utils_test() {
  test_starts_with();
  test_is_blank();
  test_is_word_char();
  test_trim();
  test_trim_string_view();

  cout << "\nAll string_utils tests passed!\n" << endl;
}


