// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>
#include <vector>
#include <curl/curl.h>
#include "yyjson.h"

struct McpTool {
  std::string name;
  std::string description;
  std::vector<std::string> params;
};

struct McpResult {
  std::string content;
  std::string isError;
  bool success;
};

class McpClient {
  public:
  McpClient();
  ~McpClient();

  bool connect(const std::string& host, int port);
  std::vector<McpTool> list_tools();
  McpResult call_tool(const std::string& name, const std::string& args);
  std::string get_session_id() const;
  void disconnect();

  private:
  std::string _session_id;
  std::string _base_url;
  std::string _tools_json;
  bool _connected;
  CURL *_curl;

  std::string make_request(const std::string& method, const std::string& params);
  std::string parse_response(const std::string& response);
  std::string parse_url_host(const std::string& url);
};
