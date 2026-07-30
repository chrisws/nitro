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
#include <format>

#include "mcp_client.h"
#include "mcp_format.h"
#include "curl.h"
#include "json.h"
#include "logging.h"

using namespace mcp;

//
// Reads Mcp-Session-Id from the headers
//
static size_t header_callback(const char *buffer, size_t size, const size_t nItems, void *userdata) {
  const size_t total = size * nItems;
  auto *session_id_out = static_cast<std::string*>(userdata);

  const std::string line(buffer, total);
  const std::string prefix = "Mcp-Session-Id:";

  // case-sensitive check is risky — HTTP headers are case-insensitive.
  // Do a case-insensitive compare instead:
  if (line.size() >= prefix.size() &&
      strncasecmp(line.c_str(), prefix.c_str(), prefix.size()) == 0) {
    const std::string value = line.substr(prefix.size());
    // trim leading/trailing whitespace and \r\n
    const size_t start = value.find_first_not_of(" \t");
    const size_t end = value.find_last_not_of(" \t\r\n");
    if (start != std::string::npos) {
      *session_id_out = value.substr(start, end - start + 1);
    }
  }

  // must return the number of bytes "handled" - libcurl aborts if you return anything else
  return total;
}

//
// Try to load MCP settings from mcp.json or mcp-settings.json in the sandbox
// Returns empty string if file not found (MCP functionality not available)
//
static Settings load_settings() {
  Settings result;
  std::string settings_path;

  // Try mcp.json first
  if (std::filesystem::exists("mcp.json")) {
    settings_path = "mcp.json";
  }

  // Try mcp-settings.json
  else if (std::filesystem::exists("mcp-settings.json")) {
    settings_path = "mcp-settings.json";
  }

  if (settings_path.empty()) {
    return result;
  }

  std::ifstream file(settings_path);
  if (!file.is_open()) {
    return result;
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  std::string content = oss.str();

  try {
    auto doc = json::parse(content);
    if (!doc.is_valid()) {
      return result;
    }

    auto root = doc.get_root();
    if (!root.is_valid()) {
      return result;
    }

    if (auto server = root.get_child("server"); server.is_object()) {
      if (server.get_str("host", result.host_) &&
          server.get_int("port", result.port_)) {
        return result;
      }
    }

    // alternative: top-level host/port
    if (root.get_str("host", result.host_) &&
        root.get_int("port", result.port_)) {
      return result;
    }
  } catch (...) {
    log_write(ERROR_LEVEL, "JSON parsing failed");
  }

  return result;
}

Client::Client()
  : enabled_(false)
  , settings_(load_settings())
  , curl_(curl_easy_init()) {
}

Client::~Client() {
  disconnect();
}

bool Client::connect() const {
  if (settings_.host_.empty()) {
    return false;
  }

  // Initialize curl
  if (!curl_) {
    log_write(ERROR_LEVEL, "failed to init curl");
    return false;
  }

  curl_set_opts(curl_);

  // Initialize handshake using mutable API
  const auto doc = json::parse_mutable("");
  if (!doc.is_valid()) {
    log_write(ERROR_LEVEL, "failed to build json doc");
    return false;
  }

  auto root = doc.get_root();
  if (!root.is_valid()) {
    log_write(ERROR_LEVEL, "failed to build json root");
    return false;
  }

  // '{"jsonrpc":"2.0","id":1,"method":"initialize",
  //      "params":{"protocolVersion":"2025-06-18","capabilities":{},
  //                "clientInfo":{"name":"nitro","version":"0.1"}}}'

  // Add jsonrpc and id fields
  root.set_str("jsonrpc", "2.0");
  root.set_int("id", 1);
  root.set_str("method", "initialize");

  auto params = doc.get_child("params");
  params.set_str("protocolVersion", "2025-06-18");
  params.set_empty_obj("capabilities");

  auto clientInfo = params.get_child("clientInfo");
  clientInfo.set_str("name", "nitro");
  clientInfo.set_str("version", "0.1");

  // Convert to string
  const std::string params_str = doc.to_string();
  if (params_str.empty()) {
    log_write(ERROR_LEVEL, "failed to build json string");
    return false;
  }

  log_write(INFO_LEVEL, "sending [%s]", params_str.c_str());

  // Send request and get session ID
  const auto response = send_request(params_str);

  if (response.empty()) {
    log_write(LogLevel::ERROR_LEVEL, "mcp request failed");
    return false;
  }

  log_write(INFO_LEVEL, "received [%s]", response.c_str());
  log_write(INFO_LEVEL, "sessionId [%s]", session_id_.c_str());

  if (auto resp_doc = json::parse(response); resp_doc.is_valid()) {
    const auto resp_root = resp_doc.get_root();
    if (int id; !resp_root.get_int("id", id)) {
      log_write(INFO_LEVEL, "failed to read id field");
    } else {
      log_write(INFO_LEVEL, "id=[%d]", id);
    }
  } else {
    log_write(ERROR_LEVEL, "failed to parse response");
  }

  return true;
}

std::vector<Tool> Client::list_tools() const {
  std::vector<Tool> tools;

  if (!curl_) {
    log_write(ERROR_LEVEL, "list_tools failed - curl not initialised");
    return tools;
  }

  // Get session ID from stored value
  std::string session_id = session_id_;
  if (session_id.empty()) {
    session_id = "default-session";
  }

  // Request tools/list using mutable API
  const auto doc = json::parse_mutable("");
  if (!doc.is_valid()) {
    log_write(ERROR_LEVEL, "failed to build json doc");
    return tools;
  }

  auto root = doc.get_root();
  if (!root.is_valid()) {
    log_write(ERROR_LEVEL, "failed to build json root");
    return tools;
  }

  // Add jsonrpc and id fields
  root.set_str("jsonrpc", "2.0");
  root.set_int("id", 2);

  // Set method
  root.set_str("method", "tools/list");

  // Set params with sessionId and arguments
  auto params = doc.get_child("params");
  params.set_str("sessionId", session_id);
  params.set_empty_obj("arguments");

  const std::string params_str = doc.to_string();
  if (params_str.empty()) {
    log_write(ERROR_LEVEL, "failed to build json string");
    return tools;
  }

  // Send request
  const std::string response = send_request(params_str);

  if (response.empty()) {
    log_write(ERROR_LEVEL, "list tools failed");
    return tools;
  }

  // Parse response using immutable API
  const auto resp_doc = json::parse(response);
  if (!resp_doc.is_valid()) {
    log_write(ERROR_LEVEL, "failed to parse [%s]", response.c_str());
    return tools;
  }

  const auto resp_root = resp_doc.get_root();

  if (!resp_root.is_object() || resp_root.has_string_key("result")) {
    log_write(ERROR_LEVEL, "result is not an object");
    return tools;
  }

  std::vector<json::JsonValue> vec;
  const auto result_node = resp_root.get_child("result");
  if (!result_node.is_object()) {
    log_write(ERROR_LEVEL, "result is not an object");
    return tools;
  }

  if (!result_node.get_array("tools", vec)) {
    log_write(ERROR_LEVEL, "tools is not an object os result");
    return tools;
  }

  for (const auto &tool: vec) {
    if (!tool.is_object()) {
      log_write(ERROR_LEVEL, "tools is not an object os result");
      return tools;
    }
    Tool mcp_tool;
    tool.get_str("name", mcp_tool.name_);
    tool.get_str("description", mcp_tool.description_);
    mcp_tool.spec_ = formatSpec(tool);
    tools.push_back(mcp_tool);
  }

  log_write(INFO_LEVEL, "list tools success - found [%d] tools", tools.size());
  return tools;
}

std::string Client::send_request(const std::string &request_body) const {
  std::string body;
  body.reserve(4096);

  curl_slist *curl_headers = nullptr;
  curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
  curl_headers = curl_slist_append(curl_headers, "Accept: application/json, text/event-stream");
  if (!session_id_.empty()) {
    const std::string session_header = "Mcp-Session-Id: " + session_id_;
    curl_headers = curl_slist_append(curl_headers, session_header.c_str());
  }

  const std::string base_url = std::format("http://{}:{}/stream", settings_.host_, settings_.port_);
  curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, curl_headers);
  curl_easy_setopt(curl_, CURLOPT_URL, base_url.c_str());
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request_body.c_str());
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, request_body.length());
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, header_callback);
  curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &session_id_);

  const CURLcode res = curl_easy_perform(curl_);
  long http_code = 0;
  curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);

  if (curl_headers) {
    curl_slist_free_all(curl_headers);
  }

  if (res != CURLE_OK) {
    log_write(ERROR_LEVEL, "ERROR: curl: %s [%s]", curl_easy_strerror(res), base_url.c_str());
    log_write(ERROR_LEVEL, "sent [%s]", request_body.c_str());
    return "";
  }
  if (http_code >= 400) {
    log_write(ERROR_LEVEL, "ERROR: HTTP %s", std::to_string(http_code).c_str());
    log_write(ERROR_LEVEL, "sent [%s]", request_body.c_str());
    return "";
  }

  return body;
}

void Client::disconnect() {
  if (curl_) {
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
  }
  session_id_.clear();
}

static std::string extract_text_content(const json::JsonValue &root) {
  // Check if root has "content" key
  if (std::string text; root.get_str("content", text)) {
    return text;
  }

  // Check if content is an array
  if (std::vector<json::JsonValue> vec; root.get_array("content", vec)) {
    for (const auto &item: vec) {
      if (item.is_object()) {
        // Check type
        if (std::string type; item.get_str("type", type) && type == "text") {
          // Extract text
          if (std::string text_val; item.get_str("text", text_val)) {
            return text_val;
          }
        }
      }
    }
  }
  return "";
}

Result Client::call_tool(const std::string &name, const std::string &args_str) const {
  Result result;
  result.success_ = true;
  result.isError_ = "";

  if (!curl_) {
    result.success_ = false;
    result.isError_ = "Not connected to MCP server";
    return result;
  }

  // Get session ID from stored value
  std::string session_id = session_id_;
  if (session_id.empty()) {
    session_id = "default-session";
  }

  // Build request using mutable API
  auto doc = json::parse_mutable("{}");
  if (!doc.is_valid()) {
    result.success_ = false;
    result.isError_ = "Failed to create request";
    return result;
  }

  auto root = doc.get_root();
  if (!root.is_valid()) {
    result.success_ = false;
    result.isError_ = "Failed to create request";
    return result;
  }

  // Add jsonrpc and id fields
  root.set_str("jsonrpc", "2.0");
  root.set_int("id", 3);

  // Set method
  root.set_str("method", "tools/call");

  // Create params object with sessionId and arguments nested properly
  auto params = doc.get_child("params");
  params.set_str("sessionId", session_id);

  // Create arguments object
  auto args = doc.get_child("arguments");
  args.set_str("name", name);
  args.set_str("arguments", args_str);

  // Convert to string
  std::string request_str = doc.to_string();
  if (request_str.empty()) {
    result.success_ = false;
    result.isError_ = "Failed to serialize request";
    return result;
  }

  std::string response = send_request(request_str);

  if (response.empty()) {
    result.success_ = false;
    result.isError_ = "Failed to communicate with MCP server";
    return result;
  }

  // Parse response using immutable API
  auto resp_doc = json::parse(response);
  if (!resp_doc.is_valid()) {
    result.success_ = false;
    result.isError_ = "Failed to parse response";
    result.content_ = response;
    return result;
  }

  if (auto resp_root = resp_doc.get_root(); resp_root.is_object()) {
    if (auto result_val = resp_root.get_child("result"); result_val.is_object()) {
      result.content_ = extract_text_content(result_val);
    } else {
      result.isError_ = "MCP server returned error";
      if (resp_root.has_string_key("error")) {
        result.content_ = response;
      }
      result.success_ = false;
    }
  } else {
    result.isError_ = "Unexpected response format";
    result.content_ = response;
    result.success_ = false;
  }

  return result;
}

