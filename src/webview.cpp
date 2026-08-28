// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//
// Serves static files from the sandbox directory and provides a
// WebSocket-based live-reload channel.  When TOOL:WRITE or TOOL:PATCH
// modifies a file, the host calls webview_broadcast_reload() to push
// a "reload" frame to every connected browser tab.
//
// Key design decisions
//   • No file watching — the trigger is TOOL:WRITE / TOOL:PATCH.
//   • HTTP server uses POSIX sockets (libcurl is a client library and
//     cannot listen).  The protocol handling follows the same RFC 7230 /
//     RFC 6455 patterns that libcurl uses internally.
//   • A minimal SHA-1 implementation is included for the WebSocket
//     handshake (Sec-WebSocket-Accept = Base64(SHA1(key + GUID))).
//
//

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <openssl/sha.h>

#include "webview.h"
#include "logging.h"

namespace fs = std::filesystem;

// ────────────────────────────────────────────────────────────────────────────
// Base64 encoding (for WebSocket handshake)
// ────────────────────────────────────────────────────────────────────────────

static std::string base64_encode(const uint8_t *data, size_t len) {
  static const char TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((len + 2) / 3 * 4);
  for (size_t i = 0; i < len; i += 3) {
    uint32_t octet_a = data[i];
    uint32_t octet_b = (i + 1 < len) ? data[i + 1] : 0;
    uint32_t octet_c = (i + 2 < len) ? data[i + 2] : 0;
    uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;
    out += TABLE[(triple >> 18) & 0x3F];
    out += TABLE[(triple >> 12) & 0x3F];
    out += (i + 1 < len) ? TABLE[(triple >> 6) & 0x3F] : '=';
    out += (i + 2 < len) ? TABLE[triple & 0x3F]       : '=';
  }
  return out;
}

// ────────────────────────────────────────────────────────────────────────────
// WebSocket constants
// ────────────────────────────────────────────────────────────────────────────

static constexpr std::string_view WS_PATH = "/__nitro_ws__";
static constexpr char  WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// ────────────────────────────────────────────────────────────────────────────
// MIME types
// ────────────────────────────────────────────────────────────────────────────

static std::string mime_type(const std::string &path) {
  auto ext = [](const std::string &p) -> std::string {
    auto dot = p.find_last_of('.');
    if (dot == std::string::npos) return "";
    return p.substr(dot);
  };
  auto e = ext(path);
  if (e == ".html" || e == ".htm") return "text/html; charset=utf-8";
  if (e == ".js"   || e == ".mjs")  return "application/javascript";
  if (e == ".css")                   return "text/css";
  if (e == ".json")                  return "application/json";
  if (e == ".png")                   return "image/png";
  if (e == ".jpg"  || e == ".jpeg")  return "image/jpeg";
  if (e == ".gif")                   return "image/gif";
  if (e == ".svg")                   return "image/svg+xml";
  if (e == ".ico")                   return "image/x-icon";
  if (e == ".woff")                  return "font/woff";
  if (e == ".woff2")                 return "font/woff2";
  if (e == ".txt")                   return "text/plain";
  if (e == ".md")                    return "text/plain";
  if (e == ".xml")                   return "application/xml";
  if (e == ".wasm")                  return "application/wasm";
  return "application/octet-stream";
}

// ────────────────────────────────────────────────────────────────────────────
// Reload client snippet — injected into every .html response.
// ────────────────────────────────────────────────────────────────────────────

static const std::string RELOAD_SNIPPET =
  "\n<script>\n"
  "(function() {\n"
  "  function connect() {\n"
  "    var proto = location.protocol === 'https:' ? 'wss://' : 'ws://';\n"
  "    var ws = new WebSocket(proto + location.host + '"
  + std::string(WS_PATH) +
  "');\n"
  "    ws.onmessage = function()  { location.reload(); };\n"
  "    ws.onopen    = function(e) { console.log('open'); ws.send('watch:' + navigator.userAgent); };\n"
  "    ws.onclose   = function(e) { console.log('closed %o', e); };\n"
  "    ws.onerror   = function(e) { console.error('[%o]', e); };\n"
  "  }\n"
  "  connect();\n"
  "})();\n"
  "</script>\n";

// ────────────────────────────────────────────────────────────────────────────
// WebDevServer
// ────────────────────────────────────────────────────────────────────────────
//
// A lightweight HTTP + WebSocket server for local web development.
//
//   • Serves static files from a sandbox root directory.
//   • Injects a WebSocket live-reload client into every .html response.
//   • Accepts WebSocket upgrades on the WS_PATH endpoint.
//   • Exposes broadcast_reload() so TOOL:WRITE / TOOL:PATCH can trigger
//     a page refresh in all connected browser tabs.
//   • No file-system watching — the caller is responsible for invoking
//     broadcast_reload() at the right moment.
//
struct WebDevServer {
  int         port_        = 9080;
  std::string root_;
  int         listen_fd_   = -1;
  bool        running_     = false;
  bool        live_reload_ = true;

  std::vector<int> ws_clients_;
  std::mutex       ws_mutex_;
  std::atomic<bool> stop_{false};
  std::thread      accept_thread_;

  // ── Lifecycle ─────────────────────────────────────────────────────

  bool start(const std::string &root, int port) {
    root_  = root;
    port_  = port;

    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) return false;

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    if (::bind(listen_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
      ::close(listen_fd_);
      listen_fd_ = -1;
      return false;
    }
    if (::listen(listen_fd_, 16) < 0) {
      ::close(listen_fd_);
      listen_fd_ = -1;
      return false;
    }

    running_ = true;
    stop_    = false;
    accept_thread_ = std::thread([this] { accept_loop(); });
    return true;
  }

  void stop() {
    stop_    = true;
    running_ = false;
    if (listen_fd_ >= 0) {
      ::shutdown(listen_fd_, SHUT_RDWR);
      ::close(listen_fd_);
      listen_fd_ = -1;
    }
    {
      std::lock_guard<std::mutex> lock(ws_mutex_);
      for (int fd : ws_clients_) {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
      }
      ws_clients_.clear();
    }
    if (accept_thread_.joinable()) {
      accept_thread_.join();
    }
  }

  ~WebDevServer() { stop(); }

  // ── Public: call after TOOL:WRITE / TOOL:PATCH ────────────────────

  void broadcast_reload() {
    if (!live_reload_) return;
    std::string frame = ws_encode_frame("reload");
    std::lock_guard<std::mutex> lock(ws_mutex_);
    for (auto it = ws_clients_.begin(); it != ws_clients_.end(); ) {
      ssize_t n = ::send(*it, frame.data(), frame.size(), MSG_NOSIGNAL);
      if (n < 0) {
        ::close(*it);
        it = ws_clients_.erase(it);
      } else {
        ++it;
      }
    }
  }

  // ── Internals ─────────────────────────────────────────────────────

  static std::string ws_encode_frame(const std::string &payload) {
    // RFC 6455: server → client text frame, unmasked, payload < 126 bytes.
    std::string frame;
    frame += char(0x81); // FIN=1, opcode=1 (text)
    frame += char(payload.size());
    frame += payload;
    return frame;
  }

  std::string ws_accept_key(const std::string &client_key) {
    const std::string input = client_key + WS_GUID;
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.data()),  input.size(), digest);
    return base64_encode(digest, SHA_DIGEST_LENGTH);
  }

  void accept_loop() {
    while (!stop_) {
      sockaddr_in client_addr{};
      socklen_t   len = sizeof(client_addr);
      int client_fd = ::accept(listen_fd_, reinterpret_cast<sockaddr *>(&client_addr), &len);
      if (client_fd < 0) {
        if (stop_) {
          break;
        }
        continue;
      }
      // Each connection is handled in a detached thread so the accept
      // loop never blocks.  For a local dev server the number of
      // concurrent connections is tiny.
      std::thread([this, client_fd] { handle_client(client_fd); }).detach();
    }
  }

  // Read the full HTTP request head (up to \r\n\r\n).
  static bool read_request_head(int fd, std::string &head) {
    head.clear();
    char buf[4096];
    while (true) {
      auto pos = head.find("\r\n\r\n");
      if (pos != std::string::npos) return true;
      ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
      if (n <= 0) return false;
      head.append(buf, static_cast<size_t>(n));
      if (head.size() > 65536) return false; // safety cap
    }
  }

  // Parsed HTTP request with case-insensitive header lookup
  // that preserves the case of header *values* (important for
  // the base64 Sec-WebSocket-Key).
  struct Request {
    std::string method, path;
    std::string head;

    std::string header(const std::string &name) const {
      std::string lower_name = name;
      std::ranges::transform(lower_name, lower_name.begin(), ::tolower);

      size_t line_start = 0;
      auto nl = head.find("\r\n");
      if (nl != std::string::npos) line_start = nl + 2;

      while (line_start < head.size()) {
        auto eol = head.find("\r\n", line_start);
        if (eol == std::string::npos) eol = head.size();
        auto colon = head.find(':', line_start);
        if (colon == std::string::npos || colon > eol) break;

        // Lowercase the header name only for comparison.
        std::string hname_lower = head.substr(line_start, colon - line_start);
        std::ranges::transform(hname_lower, hname_lower.begin(), ::tolower);
        hname_lower.erase(0, hname_lower.find_first_not_of(" \t"));
        hname_lower.erase(hname_lower.find_last_not_of(" \t") + 1);

        if (hname_lower == lower_name) {
          // Return the value as-is (preserving case).
          size_t vstart = colon + 1;
          std::string val = head.substr(vstart, eol - vstart);
          val.erase(0, val.find_first_not_of(" \t"));
          val.erase(val.find_last_not_of(" \t\r\n") + 1);
          return val;
        }
        line_start = eol + 2;
      }
      return "";
    }
  };

  static Request parse_request(const std::string &head) {
    Request req;
    req.head = head;
    auto sp1 = head.find(' ');
    if (sp1 == std::string::npos) return req;
    req.method = head.substr(0, sp1);
    auto sp2 = head.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return req;
    req.path = head.substr(sp1 + 1, sp2 - sp1 - 1);
    auto qm = req.path.find('?');
    if (qm != std::string::npos) req.path = req.path.substr(0, qm);
    return req;
  }

  static void send_http_response(int fd, int status, const std::string &status_text,
                                 const std::string &content_type,
                                 const std::string &body) {
    std::ostringstream resp;
    resp << "HTTP/1.1 " << status << " " << status_text << "\r\n";
    resp << "Content-Type: " << content_type << "\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Cache-Control: no-store\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << body;
    std::string out = resp.str();
    size_t sent = 0;
    while (sent < out.size()) {
      ssize_t n = ::send(fd, out.data() + sent, out.size() - sent, MSG_NOSIGNAL);
      if (n <= 0) break;
      sent += static_cast<size_t>(n);
    }
  }

  // Serve a static file, injecting the reload snippet into .html files.
  void serve_file(int fd, const std::string &rel_path) {
    // Prevent path traversal.
    if (rel_path.find("..") != std::string::npos) {
      send_http_response(fd, 403, "Forbidden", "text/plain", "Forbidden");
      return;
    }

    fs::path file_path = fs::path(root_) / rel_path;

    // If the resolved path is a directory, look for index.html.
    if (fs::is_directory(file_path)) {
      file_path /= "index.html";
    }

    if (!fs::is_regular_file(file_path)) {
      send_http_response(fd, 404, "Not Found", "text/plain", "Not Found");
      return;
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
      send_http_response(fd, 500, "Internal Server Error", "text/plain",
                         "Cannot read file");
      return;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // Inject the live-reload WebSocket client into HTML pages.
    if (live_reload_) {
      std::string ext = file_path.extension().string();
      if (ext == ".html" || ext == ".htm") {
        auto pos = content.find("</head>");
        if (pos != std::string::npos) {
          content.insert(pos, RELOAD_SNIPPET);
        } else {
          content += RELOAD_SNIPPET;
        }
      }
    }

    send_http_response(fd, 200, "OK", mime_type(rel_path), content);
  }

  // Per-connection handler: WebSocket upgrade or static file.
  void handle_client(int fd) {
    log_write(INFO_LEVEL, "handle request entered");

    std::string head;
    if (!read_request_head(fd, head)) {
      ::close(fd);
      return;
    }

    Request req = parse_request(head);

    // ── WebSocket upgrade ───────────────────────────────────────────
    if (req.path == std::string(WS_PATH)) {
      std::string key = req.header("sec-websocket-key");
      if (key.empty()) {
        log_write(INFO_LEVEL, "ws-upgrade bad request");
        send_http_response(fd, 400, "Bad Request", "text/plain", "Expected WebSocket upgrade request");
        ::close(fd);
        return;
      }

      std::string accept = ws_accept_key(key);
      std::ostringstream resp;
      log_write(INFO_LEVEL, "ws-upgrade [%s] key[%s] accept[%s]", req.path.c_str(), key.c_str(), accept.c_str());
      resp << "HTTP/1.1 101 Switching Protocols\r\n";
      resp << "Upgrade: websocket\r\n";
      resp << "Connection: Upgrade\r\n";
      resp << "Sec-WebSocket-Accept: " << accept << "\r\n";
      resp << "\r\n";
      std::string out = resp.str();
      size_t sent = 0;
      while (sent < out.size()) {
        ssize_t n = ::send(fd, out.data() + sent, out.size() - sent, MSG_WAITALL);
        if (n <= 0) break;
        sent += static_cast<size_t>(n);
      }

      log_write(INFO_LEVEL, "sent [%d] of [%d] bytes", sent, out.length());
      {
        std::lock_guard<std::mutex> lock(ws_mutex_);
        ws_clients_.push_back(fd);
      }

      // Block until the client disconnects.  The channel is
      // server → client only; the browser has nothing to say.
      char buf[4096];
      while (::recv(fd, buf, sizeof(buf), MSG_WAITALL) > 0) {
        // discard — we never need data from the client
      }

      log_write(INFO_LEVEL, "ws connection released");
      {
        std::lock_guard<std::mutex> lock(ws_mutex_);
        auto it = std::find(ws_clients_.begin(), ws_clients_.end(), fd);
        if (it != ws_clients_.end()) ws_clients_.erase(it);
      }
      ::close(fd);
      return;
    }

    // ── Static file ─────────────────────────────────────────────────
    std::string rel_path = req.path;
    if (rel_path == "/" || rel_path.empty()) {
      rel_path = "index.html";
    } else if (!rel_path.empty() && rel_path[0] == '/') {
      rel_path = rel_path.substr(1);
    }

    log_write(INFO_LEVEL, "serve %s", rel_path.c_str());
    serve_file(fd, rel_path);
    ::close(fd);
  }
}; // struct WebDevServer

// ────────────────────────────────────────────────────────────────────────────
// Global instance + public API for Nitro agent integration
// ────────────────────────────────────────────────────────────────────────────
static WebDevServer g_webview_server;

namespace webview {
  bool start(const std::string &root, int port) {
    return g_webview_server.start(root, port);
  }
  void stop() {
    g_webview_server.stop();
  }
  void broadcast_reload() {
    g_webview_server.broadcast_reload();
  }
  bool is_running() {
    return g_webview_server.running_;
  }
}
