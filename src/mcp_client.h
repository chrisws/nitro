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

namespace mcp {

struct Tool {
  std::string name_;
  std::string description_;
  std::string spec_;
};

struct Settings {
  std::string host_;
  int port_;
};

struct Result {
  std::string content_;
  std::string isError_;
  bool success_;
};

class Client {
  public:
  Client();
  ~Client();

  bool connect() const;
  bool enabled() const { return enabled_; }
  void enable() { enabled_ = true; }
  
  std::vector<Tool> list_tools() const;
  Result call_tool(const std::string &name, const std::string &args) const;
  void disconnect();

  private:
  std::string session_id_;
  bool enabled_;
  Settings settings_;
  CURL *curl_;

  std::string send_request(const std::string &params) const;
};

}
