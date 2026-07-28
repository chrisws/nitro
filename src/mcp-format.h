// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>
#include "json.h"

std::string formatInputExample(const json::JsonValue &inputSchema);
std::string formatOutputExample(const json::JsonValue &outputSchema);
