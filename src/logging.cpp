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

static FILE *log_file = nullptr;

void log_open() {
  if (log_file == nullptr) {
    log_file = fopen("audiocore.log", "w");
  }
}

void log_close() {
  if (log_file != nullptr) {
    fclose(log_file);
    log_file = nullptr;
  }
}

void log_write(LogLevel level, const char* format, ...) {
  if (!log_file) {
    return;
  }

  time_t now = time(nullptr);
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

  if (log_file) {
    fprintf(log_file, "[%s] [%s] ", timestamp, level_str);
    if (format) {
      va_list args;
      va_start(args, format);
      vfprintf(log_file, format, args);
      va_end(args);
    }
    fprintf(log_file, "\n");
    fflush(log_file);
  }
}
