#include <iostream>
#include <cassert>
#include <string>
#include "file.cpp"

using namespace std;

// Test isBalanced function
void test_isBalanced() {
  // Valid balanced strings
  assert(isBalanced("{}") == true);
  assert(isBalanced("()") == true);
  assert(isBalanced("{{}}") == true);
  assert(isBalanced("()()") == true);
  assert(isBalanced("{()}") == true);
  assert(isBalanced("if (x) { return 1; }") == true);
  assert(isBalanced("int main() { return 0; }") == true);
  
  // Invalid unbalanced strings
  assert(isBalanced("{") == false);
  assert(isBalanced("}") == false);
  assert(isBalanced("(") == false);
  assert(isBalanced(")") == false);
  assert(isBalanced("{{") == false);
  assert(isBalanced("}}") == false);
  assert(isBalanced("{()}") == false);
  assert(isBalanced("if (x) { return 1") == false);
  
  // Strings with strings (should still be balanced)
  assert(isBalanced("\"hello\"") == false);
  assert(isBalanced("\"hello\";") == false);
  assert(isBalanced("string s = \"hello\";") == true);
  
  cout << "test_isBalanced passed" << endl;
}

// Test parsePatch function
void test_parsePatch() {
  string patch1 = "<<<<<<< OLD\nint foo() { return 1; }\n=======\nint foo() { return 2; }\n>>>>>>> NEW";
  
  auto [old1, new1] = parsePatch(patch1);
  assert(old1 == "int foo() { return 1; }\n");
  assert(new1 == "int foo() { return 2; }\n");
  
  string patch2 = "<<<<<<< FUNCTION\nvoid bar() {\n  int x = 5;\n  return x;\n}\n=======\nvoid bar() {\n  int y = 10;\n  return y;\n}\n>>>>>>> FUNCTION";
  
  auto [old2, new2] = parsePatch(patch2);
  assert(old2 == "void bar() {\n  int x = 5;\n  return x;\n}\n");
  assert(new2 == "void bar() {\n  int y = 10;\n  return y;\n}\n");
  
  cout << "test_parsePatch passed" << endl;
}

// Test countOccurrences function
void test_countOccurrences() {
  string text = "foo foo foo";
  assert(countOccurrences(text, "foo") == 3);
  
  string text2 = "bar baz bar";
  assert(countOccurrences(text2, "bar") == 2);
  
  string text3 = "unique";
  assert(countOccurrences(text3, "unique") == 1);
  
  string text4 = "no match";
  assert(countOccurrences(text4, "xyz") == 0);
  
  cout << "test_countOccurrences passed" << endl;
}

// Test replaceAll function
void test_replaceAll() {
  string text = "foo bar foo baz foo";
  string result = replaceAll(text, "foo", "qux");
  assert(result == "qux bar qux baz qux");
  
  string text2 = "hello world";
  string result2 = replaceAll(text2, "world", "universe");
  assert(result2 == "hello universe");
  
  cout << "test_replaceAll passed" << endl;
}

// Test tool_patch function with valid patch
void test_tool_patch_valid() {
  // Create a temporary file with test content
  string test_file = "/tmp/test_patch.cpp";
  string content = "int foo() { return 1; }\nint bar() { return 2; }";
  
  ofstream out(test_file);
  out << content;
  out.close();
  
  string patch = "<<<<<<< OLD\nint foo() { return 1; }\n=======\nint foo() { return 42; }\n>>>>>>> NEW";
  
  string result = tool_patch(test_file, patch);
  assert(result == "SUCCESS: Patch applied to " + test_file);
  
  // Verify the file was updated
  ifstream in(test_file);
  string updated_content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  in.close();
  
  assert(updated_content == "int foo() { return 42; }\nint bar() { return 2; }");
  
  // Clean up
  remove(test_file.c_str());
  
  cout << "test_tool_patch_valid passed" << endl;
}

// Test tool_patch function with missing OLD block
void test_tool_patch_missing_old() {
  string test_file = "/tmp/test_patch2.cpp";
  string content = "int foo() { return 1; }\nint bar() { return 2; }";
  
  ofstream out(test_file);
  out << content;
  out.close();
  
  string patch = "<<<<<<< OLD\nint baz() { return 99; }\n=======\nint baz() { return 100; }\n>>>>>>> NEW";
  
  string result = tool_patch(test_file, patch);
  assert(result.find("ERROR") != string::npos);
  assert(result.find("not found verbatim") != string::npos);
  
  // Clean up
  remove(test_file.c_str());
  
  cout << "test_tool_patch_missing_old passed" << endl;
}

// Test tool_patch function with multiple OLD blocks
void test_tool_patch_multiple_old() {
  string test_file = "/tmp/test_patch3.cpp";
  string content = "int foo() { return 1; }\nint foo() { return 2; }\nint bar() { return 3; }";
  
  ofstream out(test_file);
  out << content;
  out.close();
  
  string patch = "<<<<<<< OLD\nint foo() { return 1; }\n=======\nint foo() { return 42; }\n>>>>>>> NEW";
  
  string result = tool_patch(test_file, patch);
  assert(result.find("ERROR") != string::npos);
  assert(result.find("found") != string::npos && result.find("times") != string::npos);
  
  // Clean up
  remove(test_file.c_str());
  
  cout << "test_tool_patch_multiple_old passed" << endl;
}

// Test tool_patch function with unbalanced NEW block
void test_tool_patch_unbalanced() {
  string test_file = "/tmp/test_patch4.cpp";
  string content = "int foo() { return 1; }\n";
  
  ofstream out(test_file);
  out << content;
  out.close();
  
  string patch = "<<<<<<< OLD\nint foo() { return 1; }\n=======\nint foo() { return 42; // missing brace\n>>>>>>> NEW";
  
  string result = tool_patch(test_file, patch);
  assert(result.find("ERROR") != string::npos);
  assert(result.find("unbalanced") != string::npos);
  
  // Clean up
  remove(test_file.c_str());
  
  cout << "test_tool_patch_unbalanced passed" << endl;
}

// Test tool_patch function with conflict markers in file
void test_tool_patch_conflict_markers() {
  string test_file = "/tmp/test_patch5.cpp";
  string content = "<<<<<<< CONFLICT\nint foo() { return 1; }\n=======\nint foo() { return 2; }\n>>>>>>> CONFLICT\n";
  
  ofstream out(test_file);
  out << content;
  out.close();
  
  string patch = "<<<<<<< OLD\nint foo() { return 1; }\n=======\nint foo() { return 42; }\n>>>>>>> NEW";
  
  string result = tool_patch(test_file, patch);
  assert(result.find("ERROR") != string::npos);
  assert(result.find("conflict markers") != string::npos);
  
  // Clean up
  remove(test_file.c_str());
  
  cout << "test_tool_patch_conflict_markers passed" << endl;
}

// Test tool_patch function with empty OLD block
void test_tool_patch_empty_old() {
  string test_file = "/tmp/test_patch6.cpp";
  string content = "int foo() { return 1; }\n";
  
  ofstream out(test_file);
  out << content;
  out.close();
  
  string patch = "<<<<<<< \n=======\nint foo() { return 42; }\n>>>>>>> NEW";
  
  string result = tool_patch(test_file, patch);
  assert(result.find("ERROR") != string::npos);
  assert(result.find("empty") != string::npos);
  
  // Clean up
  remove(test_file.c_str());
  
  cout << "test_tool_patch_empty_old passed" << endl;
}

// Test tool_patch function with empty NEW block
void test_tool_patch_empty_new() {
  string test_file = "/tmp/test_patch7.cpp";
  string content = "int foo() { return 1; }\n";
  
  ofstream out(test_file);
  out << content;
  out.close();
  
  string patch = "<<<<<<< OLD\nint foo() { return 1; }\n=======\n>>>>>>> NEW";
  
  string result = tool_patch(test_file, patch);
  assert(result.find("ERROR") != string::npos);
  assert(result.find("empty") != string::npos);
  
  // Clean up
  remove(test_file.c_str());
  
  cout << "test_tool_patch_empty_new passed" << endl;
}

int main() {
  test_isBalanced();
  test_parsePatch();
  test_countOccurrences();
  test_replaceAll();
  test_tool_patch_valid();
  test_tool_patch_missing_old();
  test_tool_patch_multiple_old();
  test_tool_patch_unbalanced();
  test_tool_patch_conflict_markers();
  test_tool_patch_empty_old();
  test_tool_patch_empty_new();
  
  cout << "\nAll tests passed!" << endl;
  return 0;
}


