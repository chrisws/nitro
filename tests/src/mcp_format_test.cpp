#include <iostream>
#include <cassert>
#include <string>

#include "mcp_format.h"

using namespace std;

static const string TOOL_SINGLE_JSON = R"(
{
  "name": "search_regex",
  "inputSchema": {
    "properties": {
      "q": {
        "type": "string",
        "description": "Regex pattern to search for"
      },
      "paths": {
        "type": "array",
        "items": {
          "type": "string"
        },
        "description": "Optional list of project-relative glob patterns to filter results. Supports '!' excludes. Trailing '/' expands to '**'. Patterns without '/' are treated as '**/pattern'. Empty strings are ignored."
      },
      "limit": {
        "type": "integer",
        "description": "Maximum number of results to return"
      },
      "projectPath": {
        "type": "string",
        "description": " The project path. Pass this value ALWAYS if you are aware of it. It reduces numbers of ambiguous calls. \n In the case you know only the current working directory you can use it as the project path.\n If you're not aware about the project path you can ask user about it."
      }
    },
    "required": [
      "q"
    ],
    "type": "object"
  },
  "description": "Searches for regex matches within project files.\nUse this tool when you need regex search with match coordinates.\nResults include match coordinates when available (1-based line/column, end exclusive).\n\nPaths are glob patterns relative to the project root.\nExamples: [\"src/**\", \"!**/test/**\"], [\"**/*.kt\"], [\"foo/\"].",
  "outputSchema": {
    "properties": {
      "items": {
        "type": "array",
        "items": {
          "type": "object",
          "required": [
            "filePath"
          ],
          "properties": {
            "filePath": {
              "type": "string"
            },
            "startLine": {
              "type": [
                "integer",
                "null"
              ]
            },
            "startColumn": {
              "type": [
                "integer",
                "null"
              ]
            },
            "endLine": {
              "type": [
                "integer",
                "null"
              ]
            },
            "endColumn": {
              "type": [
                "integer",
                "null"
              ]
            }
          }
        }
      },
      "more": {
        "type": "boolean"
      },
      "partialResultReason": {
        "type": [
          "string",
          "null"
        ]
      }
    },
    "required": [],
    "type": "object"
  },
  "annotations": {
    "readOnlyHint": true,
    "openWorldHint": false
  }
}
)";

// Expected output (guess for now)
static const string EXPECTED_OUTPUT = R"(
# Name: search_regex

## Description
Searches for regex matches within project files.
Use this tool when you need regex search with match coordinates.
Results include match coordinates when available (1-based line/column, end exclusive).

Paths are glob patterns relative to the project root.
Examples: ["src/**", "!**/test/**"], ["**/*.kt"], ["foo/"].
Field q is required

## Example request
```
{"q":"Regex pattern to search for","paths":[item1, item2],"limit":1,"projectPath":"The project path."}
```

## Response schema
```
{"properties":{"items":{"type":"array","items":{"type":"object","required":["filePath"],"properties":{"filePath":{"type":"string"},"startLine":{"type":["integer","null"]},"startColumn":{"type":["integer","null"]},"endLine":{"type":["integer","null"]},"endColumn":{"type":["integer","null"]}}}},"more":{"type":"boolean"},"partialResultReason":{"type":["string","null"]}},"required":[],"type":"object"}
```
)";

// Test formatSpec with valid input
static void test_formatSpec_valid() {
  auto doc = json::parse(TOOL_SINGLE_JSON);
  assert(doc.is_valid());
  auto root = doc.get_root();
  
  std::string result = formatSpec(root);

  assert(result.find("# Name: search_regex") != std::string::npos);
  assert(result.find("## Description") != std::string::npos);
  assert(result.find("## Example request") != std::string::npos);
  assert(result.find("## Response schema") != std::string::npos);
  assert(result.find("{\"q\":\"Regex pattern to search for\",\"paths\":[item1, item2],\"limit\":1,\"projectPath\":\"The project path.\"}") != std::string::npos);
  assert(result.find("Field q is required") != std::string::npos);
 
  cout << "test_formatSpec_valid passed" << endl;
}

void mcp_format_test() {
  test_formatSpec_valid();
  cout << "\nAll mcp_format tests passed!" << endl;
}
