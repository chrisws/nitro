// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include "mcp-client.h"
#include "json.h"
#include <sstream>
#include <regex>
#include <fstream>
#include <filesystem>

// Try to load MCP settings from mcp.json or mcp-settings.json in the sandbox
// Returns empty string if file not found (MCP functionality not available)
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
    // Use json::parse_mutable to read JSON from string
    auto doc = json::parse_mutable(content);
    if (!doc.valid()) {
      return "";
    }

    auto root = doc.get_root();

    // Extract MCP server configuration
    if (root && json::is_object(*root)) {
      auto server = json::get_value(*root, "server", nullptr);
      if (server && json::is_object(*server)) {
        auto host = json::get_value(*server, "host", nullptr);
        auto port = json::get_value(*server, "port", nullptr);
        if (host && json::is_string(*host) && port && json::is_number(*port)) {
          std::string host_str;
          json::get_string(*host, host_str);
          int port_num = 0;
          json::get_int(*port, port_num);
          return host_str + ":" + std::to_string(port_num);
        }
      }
    }

    // Alternative: top-level host/port
    if (root && json::is_object(*root)) {
      auto host = json::get_value(*root, "host", nullptr);
      auto port = json::get_value(*root, "port", nullptr);
      if (host && json::is_string(*host) && port && json::is_number(*port)) {
        std::string host_str;
        json::get_string(*host, host_str);
        int port_num = 0;
        json::get_int(*port, port_num);
        return host_str + ":" + std::to_string(port_num);
      }
    }

    // Alternative: base_url as string
    if (root && json::is_string(*root)) {
      std::string url;
      json::get_string(*root, url);
      return url;
    }

  } catch (...) {
    // JSON parsing failed
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
      _base_url = "http://" + settings.substr(0, colon_pos) + ":" +
        settings.substr(colon_pos + 1) + "/stream";
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
  auto mut_doc = json::JsonMutDoc::parse("{}");
  if (!mut_doc.valid()) {
    return false;
  }

  // Set protocol version
  json::set_string(*mut_doc, "protocolVersion", "2025-06-18");

  // Set capabilities - create empty object
  auto capabilities_str = json::set_string(*mut_doc, "capabilities", "{}");
  if (!capabilities_str) {
    return false;
  }

  // Set client info
  json::set_string(*mut_doc, "clientInfo", "{\"name\":\"nitro\",\"version\":\"0.1\"}");

  // Convert to string
  std::string params_str = json::write_mutable(*mut_doc);
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
  auto mut_doc = json::JsonMutDoc::parse("{}");
  if (!mut_doc.valid()) {
    return tools;
  }

  // Set method
  json::set_string(*mut_doc, "method", "tools/list");

  // Set params
  json::set_string(*mut_doc, "params", "{\"sessionId\":\"" + session_id + "\"}");

  // Convert to string
  std::string params_str = json::write_mutable(*mut_doc);
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
  if (!resp_doc.valid()) {
    return tools;
  }

  auto resp_root = resp_doc.get_root();

  if (resp_root && json::is_object(*resp_root) && json::has_key(*resp_root, "result")) {
    auto result = json::get_value(*resp_root, "result", nullptr);
    if (result && json::is_object(*result)) {
      auto tools_arr = json::get_value(*result, "tools", nullptr);
      if (tools_arr && json::is_array(*tools_arr)) {
        for (size_t i = 0; i < json::get_length(*result, "tools", 0); i++) {
          auto tool = json::get_index(*result, "tools", i, "");
          McpTool mcp_tool;

          if (!tool.empty()) {
            if (json::is_string(*tool)) {
              mcp_tool.name = json::get_string(*tool, "");
            } else if (json::is_object(*tool)) {
              mcp_tool.name = json::get_string(*tool, "");
              mcp_tool.description = json::get_string(*tool, "");

              // Extract param names by iterating over object keys
              std::vector<std::string> keys;
              json::get_keys(*tool, keys);
              for (const auto& key : keys) {
                mcp_tool.params.push_back(key);
              }
            }
          }

          tools.push_back(mcp_tool);
        }
      }
    }
  }

  _tools_json = request;
  return tools;
}

std::string McpClient::make_request(const std::string& method, const std::string& params_str) {
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

std::string McpClient::parse_response(const std::string& response) {
  // For MCP, responses can be JSON or SSE
  // Try to parse as JSON first
  auto doc = json::parse(response);
  if (doc.valid() && json::is_object(*doc.get_root())) {
    if (json::has_key(*doc.get_root(), "result")) {
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

std::string McpClient::extract_text_content(const std::string& response) {
  auto doc = json::parse(response);
  if (!doc.valid()) {
    return "";
  }

  auto root = doc.get_root();

  if (root && json::is_object(*root)) {
    auto content = json::get_value(*root, "content", nullptr);
    if (content) {
      if (json::is_array(*content)) {
        for (size_t i = 0; i < json::get_length(*root, "content", 0); i++) {
          auto item = json::get_index(*root, "content", i, "");
          if (!item.empty() && json::is_object(*item)) {
            // Use yyjson_ptr_get_str for mutable API or yyjson_get_str for immutable
            auto type = json::get_string(*item, "");
            if (type == "text") {
              auto text = json::get_string(*item, "");
              return text;
            }
          }
        }
      } else if (json::is_string(*content)) {
        return json::get_string(*content, "");
      }
    }
  }

  return "";
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

McpResult McpClient::call_tool(const std::string& name, const std::string& args_str) {
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
  auto mut_doc = json::JsonMutDoc::parse("{}");
  if (!mut_doc.valid()) {
    result.success = false;
    result.isError = "Failed to create request";
    return result;
  }

  // Set method
  json::set_string(*mut_doc, "method", "tools/call");

  // Set params
  json::set_string(*mut_doc, "params", "{\"name\":\"" + name + "\",\"arguments\":\"" + args_str + "\"}");

  // Convert to string
  std::string request_str = json::write_mutable(*mut_doc);
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
  if (!resp_doc.valid()) {
    result.success = false;
    result.isError = "Failed to parse response";
    result.content = response;
    return result;
  }

  auto resp_root = resp_doc.get_root();

  if (resp_root && json::is_object(*resp_root)) {
    auto result_val = json::get_value(*resp_root, "result", nullptr);
    if (result_val && json::is_object(*result_val)) {
      result.content = extract_text_content(response);
    } else {
      result.isError = "MCP server returned error";
      if (json::has_key(*resp_root, "error")) {
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
std::string McpClient::parse_url_host(const std::string& url) {
  size_t colon_pos = url.find(':');
  if (colon_pos != std::string::npos) {
    return url.substr(0, colon_pos);
  }
  return url;
}



