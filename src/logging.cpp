// This file is part of AudioCore
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include "logging.h"

#include <cstdio>
#include <ctime>
#include <cstdarg>
#include <string>
#include <unordered_map>

static FILE *g_logfile = nullptr;
static LogLevel g_level = DEBUG_LEVEL;
static bool g_log_console = false;

static LogLevel get_level(const std::string& level) {
  static std::unordered_map<std::string, LogLevel> loggingMap = {
    {"0", DEBUG_LEVEL},
    {"1", DEBUG_LEVEL},
    {"2", INFO_LEVEL},
    {"3", WARNING_LEVEL},
    {"4", ERROR_LEVEL},
    {"5", ERROR_LEVEL},
  };
  auto result = INFO_LEVEL;
  if (!level.empty()) {
    if (const auto it = loggingMap.find(level); it != loggingMap.end()) {
      result = it->second;
    }
  }
  return result;
}

void log_open(const std::string& level) {
  g_level = get_level(level);
  if (g_logfile == nullptr) {
    const char *home = getenv("HOME");
    const auto path = std::string(home ? home : ".") + "/.config/nitro/nitro.log";
    g_logfile = fopen(path.c_str(), "a");
  }
}

void log_open_console() {
  g_logfile = nullptr;
  g_level = ERROR_LEVEL;
  g_log_console = true;
}

void log_close() {
  if (g_logfile != nullptr) {
    fclose(g_logfile);
    g_logfile = nullptr;
  }
}

void log_write(LogLevel level, const char* format, ...) {
  if (g_log_console) {
    fprintf(stderr, "LOG: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    return;
  }

  if (!g_logfile || level < g_level) {
    return;
  }

  const time_t now = time(nullptr);
  char timestamp[20];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

  const char* level_str;
  switch (level) {
    case DEBUG_LEVEL: level_str = "DEBUG"; break;
    case INFO_LEVEL: level_str = "INFO"; break;
    case WARNING_LEVEL: level_str = "WARNING"; break;
    case ERROR_LEVEL: level_str = "ERROR"; break;
    default: level_str = "UNKNOWN"; break;
  }

  fprintf(g_logfile, "[%s] [%s] ", timestamp, level_str);
  if (format) {
    va_list args;
    va_start(args, format);
    vfprintf(g_logfile, format, args);
    va_end(args);
  }
  fprintf(g_logfile, "\n");
  fflush(g_logfile);
}
