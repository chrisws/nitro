// This file is part of AudioCore
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#ifndef LOGGING_H
#define LOGGING_H

#include <string>

enum LogLevel {
  DEBUG_LEVEL = 0,
  INFO_LEVEL = 1,
  WARNING_LEVEL = 2,
  ERROR_LEVEL = 3
};

void log_write(LogLevel level, const char* format, ...);
void log_open(std::string level);
void log_close();

#endif // LOGGING_H
