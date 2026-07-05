// This file is part of AudioCore
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#ifndef LOGGING_H
#define LOGGING_H

#include <cstdio>

enum LogLevel {
  DEBUG_LEVEL,
  INFO_LEVEL,
  WARNING_LEVEL,
  ERROR_LEVEL
};

void log_write(LogLevel level, const char* format, ...);
void log_open();
void log_close();

#endif // LOGGING_H
