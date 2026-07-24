// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <sstream>
#include <regex>
#include <fstream>
#include <filesystem>

#include "mcp-client.h"
#include "json.h"
#include "logging.h"

//
// Try to load MCP settings from mcp.json or mcp-settings.json in the sandbox
// Returns empty string if file not found (MCP functionality not available)
//
static std::string load_mcp_settings() {
  std::string settings_path = "";

  // Try mcp.json first
  if (std::filesystem::exists("mcp.json")) {
    settings_path = "mcp.json";
  }

  // Try mcp-settings.json
  else if (std::filesystem::exists("mcp-settings.json")) {
    settings_path = "mcp-settings.json";
  }

  if (settings_path.empty()) {
    return "";
  }

  std::ifstream file(settings_path);
  if (!file.is_open()) {
    return "";
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  std::string content = oss.str();

  try {
    auto doc = json::parse(content);
    if (!doc.is_valid()) {
      return "";
    }

    auto root = doc.get_root();
    if (!root.is_valid()) {
      return "";
    }

    auto server = root.get("server");
    if (server.is_object()) {
      std::string host;
      int port;
      if (server.get_str("host", host) &&
          server.get_int("port", port)) {
        return host + ":" + std::to_string(port);
      }
    }

    // alternative: top-level host/port
    std::string host;
    int port;
    if (root.get_str("host", host) &&
        root.get_int("port", port)) {
      return host + ":" + std::to_string(port);
    }

    // alternative: base_url as string
    std::string base_url;
    if (root.get_str("base_url", base_url)) {
      return base_url;
    }
  } catch (...) {
    log_write(LogLevel::ERROR_LEVEL, "JSON parsing failed");
  }

  return "";
}

McpClient::McpClient()
  : _base_url("http://127.0.0.1:64342/stream")
  , _connected(false)
  , _curl(nullptr) {

  // Try to load settings from sandbox
  std::string settings = load_mcp_settings();
  if (!settings.empty()) {
    size_t colon_pos = settings.find(':');
    if (colon_pos != std::string::npos) {
      _base_url = "http://" + settings.substr(0, colon_pos) + ":" +  settings.substr(colon_pos + 1) + "/stream";
    }
  }
}

McpClient::~McpClient() {
  disconnect();
}

bool McpClient::connect(const std::string &host, int port) {
  // If explicit host/port provided, use them
  if (!host.empty() && port > 0) {
    _base_url = "http://" + host + ":" + std::to_string(port) + "/stream";
  }
  // Otherwise, use the loaded settings or default

  // Initialize curl
  _curl = curl_easy_init();
  if (!_curl) {
    return false;
  }

  // Initialize handshake using mutable API
  auto doc = json::parse_mutable("{}");
  if (!doc.is_valid()) {
    return false;
  }

  auto root = doc.get_root();
  if (!root.is_valid()) {
    return "";
  }

  // Set protocol version
  root.set_str("protocolVersion", "2025-06-18");

  // Set capabilities - create empty object
  root.set_str("capabilities", "{}");

  // Set client info
  auto params = doc.get_child();
  params.set_str("name", "nitro");
  params.set_str("version", "0.1");
  root.set_obj("clientInfo", params);

  // Convert to string
  std::string params_str = doc.write();
  if (params_str.empty()) {
    return false;
  }

  // Send request and get session ID
  _session_id = make_request("initialize", params_str);

  if (_session_id.empty()) {
    return false;
  }

  _connected = true;
  return true;
}

std::vector<McpTool> McpClient::list_tools() {
  std::vector<McpTool> tools;

  if (!_connected) {
    return tools;
  }

  // Get session ID from headers if available
  std::string session_id = _session_id;
  if (session_id.empty()) {
    session_id = "default-session";
  }

  // Request tools/list using mutable API
  auto doc = json::parse_mutable("{}");
  if (!doc.is_valid()) {
    return tools;
  }

  auto root = doc.get_root();
  if (!root.is_valid()) {
    return tools;
  }

  // Set method
  root.set_str("method", "tools/list");

  // Set params
  auto params = doc.get_child();
  params.set_str("session", session_id);
  root.set_obj("params", params);

  std::string params_str = doc.write();
  if (params_str.empty()) {
    return tools;
  }

  // Send request
  std::string request = make_request("tools/list", params_str);

  if (request.empty()) {
    return tools;
  }

  // Parse response using immutable API
  auto resp_doc = json::parse(request);
  if (!resp_doc.is_valid()) {
    return tools;
  }

  auto resp_root = resp_doc.get_root();

  if (resp_root.is_object() && resp_root.has_string_key("result")) {
    std::vector<json::JsonValue> vec;
    if (resp_root.get_array("result", vec)) {
      for (const auto &tool: vec) {
        if (tool.is_object()) {
          McpTool mcp_tool;

          // Extract name
          std::string name;
          if (tool.get_str("name", name)) {
            mcp_tool.name = name;
          }

          // Extract description
          std::string description;
          if (tool.get_str("description", description)) {
            mcp_tool.description = description;
          }

          // Extract param names by iterating over object keys
          tool.get_keys(mcp_tool.params);

          tools.push_back(mcp_tool);
        }
      }
    }
  }

  _tools_json = request;
  return tools;
}

std::string McpClient::make_request(const std::string &method, const std::string &params_str) {
  std::string request_body = params_str;
  std::string url = _base_url;
  std::string session_header = "";

  // Add session ID to header
  if (!_session_id.empty()) {
    session_header = "Mcp-Session-Id: " + _session_id + "\r\n";
  }

  // Build request
  std::ostringstream oss;
  oss << "POST " << url << " HTTP/1.1\r\n";
  oss << "Host: " << parse_url_host(url) << "\r\n";
  oss << "Content-Type: application/json\r\n";
  oss << "Accept: application/json, text/event-stream\r\n";
  oss << session_header;
  oss << "Content-Length: " << request_body.length() << "\r\n";
  oss << "\r\n";
  oss << request_body;

  return oss.str();
}

std::string McpClient::parse_response(const std::string &response) {
  // For MCP, responses can be JSON or SSE
  // Try to parse as JSON first
  auto doc = json::parse(response);
  if (doc.is_valid()) {
    auto root = doc.get_root();
    if (root.is_object() && root.has_string_key("result")) {
      return response;
    }
  }

  // Try SSE format
  size_t pos = response.find("data:");
  if (pos != std::string::npos) {
    std::string data = response.substr(pos + 5);
    // Remove trailing whitespace and parse
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r' || data.back() == ' ')) {
      data.pop_back();
    }
    if (!data.empty()) {
      return data;
    }
  }

  return response;
}

std::string McpClient::get_session_id() const {
  return _session_id;
}

void McpClient::disconnect() {
  if (_curl) {
    curl_easy_cleanup(_curl);
    _curl = nullptr;
  }
  _connected = false;
  _session_id.clear();
  _tools_json.clear();
}

static std::string extract_text_content(const json::JsonValue &root) {
  // Check if root has "content" key
  std::string text;
  if (root.get_str("content", text)) {
    return text;
  }

  // Check if content is an array
  std::vector<json::JsonValue> vec;
  if (root.get_array("content", vec)) {
    for (const auto &item: vec) {
      if (item.is_object()) {
        // Check type
        std::string type;
        if (item.get_str("type", type) && type == "text") {
          // Extract text
          std::string text_val;
          if (item.get_str("text", text_val)) {
            return text_val;
          }
        }
      }
    }
  }
  return "";
}

McpResult McpClient::call_tool(const std::string &name, const std::string &args_str) {
  McpResult result;
  result.success = true;
  result.isError = "";

  if (!_connected) {
    result.success = false;
    result.isError = "Not connected to MCP server";
    return result;
  }

  // Get session ID from headers
  std::string session_id = _session_id;
  if (session_id.empty()) {
    session_id = "default-session";
  }

  // Build request using mutable API
  auto doc = json::parse_mutable("{}");
  if (!doc.is_valid()) {
    result.success = false;
    result.isError = "Failed to create request";
    return result;
  }

  auto root = doc.get_root();
  if (!root.is_valid()) {
    result.success = false;
    result.isError = "Failed to create request";
  }

  // Set method
  root.set_str("method", "tools/call");

  // Create params object with nested values
  auto params = doc.get_child();
  params.set_str("name", name);
  params.set_str("arguments", args_str);
  root.set_obj("params", params);

  // Convert to string
  std::string request_str = doc.write();
  if (request_str.empty()) {
    result.success = false;
    result.isError = "Failed to serialize request";
    return result;
  }

  std::string response = make_request("tools/call", request_str);

  if (response.empty()) {
    result.success = false;
    result.isError = "Failed to communicate with MCP server";
    return result;
  }

  // Parse response using immutable API
  auto resp_doc = json::parse(response);
  if (!resp_doc.is_valid()) {
    result.success = false;
    result.isError = "Failed to parse response";
    result.content = response;
    return result;
  }

  auto resp_root = resp_doc.get_root();

  if (resp_root.is_object()) {
    auto result_val = resp_root.get("result");
    if (result_val.is_object()) {
      result.content = extract_text_content(result_val);
    } else {
      result.isError = "MCP server returned error";
      if (resp_root.has_string_key("error")) {
        result.content = response;
      }
      result.success = false;
    }
  } else {
    result.isError = "Unexpected response format";
    result.content = response;
    result.success = false;
  }

  return result;
}

// Helper function to parse URL host
std::string McpClient::parse_url_host(const std::string &url) {
  size_t colon_pos = url.find(':');
  if (colon_pos != std::string::npos) {
    return url.substr(0, colon_pos);
  }
  return url;
}
