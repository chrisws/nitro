// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include "mcp-client.h"
#include <sstream>
#include <regex>
#include <fstream>
#include <filesystem>
#include <yyjson.h>

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
    // Use yyjson_read_opts for reading JSON from string
    yyjson_doc *doc = yyjson_read_opts((char*)content.data(), content.size(), 0, NULL, NULL);
    if (doc == nullptr) {
      return "";
    }

    yyjson_val *root = yyjson_doc_get_root(doc);

    // Extract MCP server configuration
    if (root && yyjson_is_obj(root)) {
      yyjson_val *server = yyjson_obj_get(root, "server");
      if (server && yyjson_is_obj(server)) {
        yyjson_val *host = yyjson_obj_get(server, "host");
        yyjson_val *port = yyjson_obj_get(server, "port");
        if (host && port && yyjson_is_str(host) && yyjson_is_num(port)) {
          std::string host_str = yyjson_get_str(host);
          int port_num = yyjson_get_num(port);
          return host_str + ":" + std::to_string(port_num);
        }
      }
    }

    // Alternative: top-level host/port
    if (root && yyjson_is_obj(root)) {
      yyjson_val *host = yyjson_obj_get(root, "host");
      yyjson_val *port = yyjson_obj_get(root, "port");
      if (host && port && yyjson_is_str(host) && yyjson_is_num(port)) {
        std::string host_str = yyjson_get_str(host);
        int port_num = yyjson_get_num(port);
        return host_str + ":" + std::to_string(port_num);
      }
    }

    // Alternative: base_url as string
    if (root && yyjson_is_str(root)) {
      return yyjson_get_str(root);
    }

    yyjson_doc_free(doc);
    
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
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return false;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);

  yyjson_mut_val *protocolVersion = yyjson_mut_str(mut_doc, "2025-06-18");
  yyjson_mut_val *capabilities = yyjson_mut_obj(mut_doc);
  yyjson_mut_val *clientInfo = yyjson_mut_obj(mut_doc);

  // Clear the capabilities object (empty object)
  yyjson_mut_obj_clear(capabilities);

  // Use yyjson_mut_obj_add to set keys in mutable object
  yyjson_mut_obj_add(capabilities, yyjson_mut_str(mut_doc, "roots"), yyjson_mut_null(mut_doc));
  yyjson_mut_obj_add(capabilities, yyjson_mut_str(mut_doc, "prompts"), yyjson_mut_null(mut_doc));
  yyjson_mut_obj_add(capabilities, yyjson_mut_str(mut_doc, "tools"), yyjson_mut_null(mut_doc));
  yyjson_mut_obj_add(capabilities, yyjson_mut_str(mut_doc, "sampling"), yyjson_mut_null(mut_doc));

  yyjson_mut_obj_add(clientInfo, yyjson_mut_str(mut_doc, "name"), yyjson_mut_str(mut_doc, "nitro"));
  yyjson_mut_obj_add(clientInfo, yyjson_mut_str(mut_doc, "version"), yyjson_mut_str(mut_doc, "0.1"));

  yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, "protocolVersion"), protocolVersion);
  yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, "capabilities"), capabilities);
  yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, "clientInfo"), clientInfo);

  // Convert to string
  char *params_str = yyjson_mut_write(mut_doc, 0, NULL);
  if (params_str == nullptr) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  // Send request and get session ID
  _session_id = make_request("initialize", params_str);
  free(params_str);

  if (_session_id.empty()) {
    yyjson_mut_doc_free(mut_doc);
    return false;
  }

  yyjson_mut_doc_free(mut_doc);
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
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    return tools;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);
  yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, "method"), yyjson_mut_str(mut_doc, "tools/list"));

  yyjson_mut_val *params = yyjson_mut_obj(mut_doc);
  yyjson_mut_obj_add(params, yyjson_mut_str(mut_doc, "sessionId"), yyjson_mut_str(mut_doc, session_id.c_str()));
  yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, "params"), params);

  // Convert to string
  char *params_str = yyjson_mut_write(mut_doc, 0, NULL);
  if (params_str == nullptr) {
    return tools;
  }

  // Send request
  std::string request = make_request("tools/list", params_str);
  free(params_str);

  if (request.empty()) {
    return tools;
  }

  // Parse response using immutable API
  yyjson_doc *resp_doc = yyjson_read_opts((char*)request.data(), request.size(), 0, NULL, NULL);
  if (!resp_doc) {
    return tools;
  }

  yyjson_val *resp_root = yyjson_doc_get_root(resp_doc);

  if (resp_root && yyjson_is_obj(resp_root) && yyjson_obj_get(resp_root, "result")) {
    yyjson_val *result = yyjson_obj_get(resp_root, "result");
    if (result && yyjson_is_obj(result)) {
      yyjson_val *tools_arr = yyjson_obj_get(result, "tools");
      if (tools_arr && yyjson_is_arr(tools_arr)) {
        for (size_t i = 0; i < yyjson_arr_size(tools_arr); i++) {
          yyjson_val *tool = yyjson_arr_get(tools_arr, i);
          McpTool mcp_tool;

          if (tool && yyjson_is_str(tool)) {
            mcp_tool.name = yyjson_get_str(tool);
          } else if (tool && yyjson_is_obj(tool)) {
            mcp_tool.name = yyjson_get_str(tool);
            mcp_tool.description = yyjson_get_str(tool);

            // Extract param names by iterating over object using foreach
            // Note: val parameter is required but unused - just a placeholder lvalue
            yyjson_val *key;
            yyjson_val *unused_val = nullptr;
            yyjson_obj_foreach(tool, i, i, key, unused_val) {
              mcp_tool.params.push_back(yyjson_get_str(key));
            }
          }

          tools.push_back(mcp_tool);
        }
      }
    }
  }

  _tools_json = request;
  yyjson_doc_free(resp_doc);
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
  yyjson_doc *doc = yyjson_read_opts((char*)response.data(), response.size(), 0, NULL, NULL);
  if (doc && yyjson_doc_get_root(doc) && yyjson_is_obj(yyjson_doc_get_root(doc))) {
    if (yyjson_obj_get(yyjson_doc_get_root(doc), "result")) {
      yyjson_doc_free(doc);
      return response;
    }
    yyjson_doc_free(doc);
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
  yyjson_doc *doc = yyjson_read_opts((char*)response.data(), response.size(), 0, NULL, NULL);
  if (!doc) {
    return "";
  }

  yyjson_val *root = yyjson_doc_get_root(doc);

  if (root && yyjson_is_obj(root)) {
    yyjson_val *content = yyjson_obj_get(root, "content");
    if (content) {
      if (yyjson_is_arr(content)) {
        for (size_t i = 0; i < yyjson_arr_size(content); i++) {
          yyjson_val *item = yyjson_arr_get(content, i);
          if (item && yyjson_is_obj(item)) {
            // Use yyjson_ptr_get_str for mutable API or yyjson_get_str for immutable
            const char *type = yyjson_get_str(item);
            if (type && std::string(type) == "text") {
              const char *text = yyjson_get_str(item);
              if (text) {
                yyjson_doc_free(doc);
                return std::string(text);
              }
            }
          }
        }
      } else if (yyjson_is_str(content)) {
        std::string text = yyjson_get_str(content);
        yyjson_doc_free(doc);
        return text;
      }
    }
  }

  yyjson_doc_free(doc);
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
  yyjson_mut_doc *mut_doc = yyjson_mut_doc_new(NULL);
  if (!mut_doc) {
    result.success = false;
    result.isError = "Failed to create request";
    return result;
  }

  yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);
  yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, "method"), yyjson_mut_str(mut_doc, "tools/call"));

  yyjson_mut_val *params = yyjson_mut_obj(mut_doc);
  yyjson_mut_obj_add(params, yyjson_mut_str(mut_doc, "name"), yyjson_mut_str(mut_doc, name.c_str()));
  yyjson_mut_obj_add(params, yyjson_mut_str(mut_doc, "arguments"), yyjson_mut_str(mut_doc, args_str.c_str()));
  yyjson_mut_obj_add(root, yyjson_mut_str(mut_doc, "params"), params);

  // Convert to string
  char *request_str = yyjson_mut_write(mut_doc, 0, NULL);
  if (request_str == nullptr) {
    yyjson_mut_doc_free(mut_doc);
    result.success = false;
    result.isError = "Failed to serialize request";
    return result;
  }

  std::string response = make_request("tools/call", request_str);
  free(request_str);

  if (response.empty()) {
    yyjson_mut_doc_free(mut_doc);
    result.success = false;
    result.isError = "Failed to communicate with MCP server";
    return result;
  }

  // Parse response using immutable API
  yyjson_doc *resp_doc = yyjson_read_opts((char*)response.data(), response.size(), 0, NULL, NULL);
  if (!resp_doc) {
    yyjson_mut_doc_free(mut_doc);
    result.success = false;
    result.isError = "Failed to parse response";
    result.content = response;
    return result;
  }

  yyjson_val *resp_root = yyjson_doc_get_root(resp_doc);

  if (resp_root && yyjson_is_obj(resp_root)) {
    yyjson_val *result_val = yyjson_obj_get(resp_root, "result");
    if (result_val && yyjson_is_obj(result_val)) {
      result.content = extract_text_content(response);
    } else {
      result.isError = "MCP server returned error";
      if (yyjson_obj_get(resp_root, "error")) {
        result.content = response;
      }
      result.success = false;
    }
  } else {
    result.isError = "Unexpected response format";
    result.content = response;
    result.success = false;
  }

  yyjson_doc_free(resp_doc);
  yyjson_mut_doc_free(mut_doc);
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


