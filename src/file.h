// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>

constexpr std::string PATCH_BEGIN    = "<<<<<<<";
constexpr std::string PATCH_BOUNDARY = "=======";
constexpr std::string PATCH_END      = ">>>>>>>";

std::string tool_patch(const std::string& filename, const std::string& patch_str);
std::string tool_write(const std::string &path, const std::string &data);
