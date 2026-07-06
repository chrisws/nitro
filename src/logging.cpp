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

#include "ggml.h"

static FILE *g_logfile = nullptr;
LogLevel g_level = DEBUG_LEVEL;

void log_open(LogLevel level) {
  g_level = level;
  if (g_logfile == nullptr) {
    const char *home = getenv("HOME");
    std::string path = std::string(home ? home : ".") + "/.config/nitro/nitro.log";
    g_logfile = fopen(path.c_str(), "a");
  }
}

void log_close() {
  if (g_logfile != nullptr) {
    fclose(g_logfile);
    g_logfile = nullptr;
  }
}

void log_write(LogLevel level, const char* format, ...) {
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
