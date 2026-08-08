// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>

/*
 * patch.cpp - TOOL:PATCH implementation
 *
 * Required System Prompt Changes:
 *
 * 1. Scope patches at function/block granularity, not line-diffs.
 *    Have it emit "here's the old function body, here's the new one"
 *
 * 2. Re-quote the target block before patching,
 *
 * 3. full-rewrite is a fallback for small files (say under ~100-150 lines)
 *
 * 4. Verify after applying, not just before - a quick syntax sanity check (does it still parse/compile)
 *    before accepting the edit gives you a second backstop.
 *
 * 5. always provide the complete function body, from signature to closing brace — never a partial range, never
 *
 * 6. Must include the full function signature line as the first line of both OLD and NEW blocks, even if the signature itself is unchanged
 *
 * Rules:
 * - Report an error if the file itself ever legitimately contains <<<<<<</=======/>>>>>>>
 * - Search the file for the OLD block as an exact, single match. Zero
 *   matches or multiple matches → reject, feed back an error like "OLD
 *   block not found verbatim — re-quote it exactly"
 * - Before writing, do a cheap brace/paren balance check on the NEW
 *   block alone — catches a truncated or malformed function before it hits disk.
 * - Verify after applying, not just before - a quick syntax sanity check (does it still parse/compile)
 *   before accepting the edit gives you a second backstop.
 * - always provide the complete function body, from signature to closing brace — never a partial range, never
 * - Must include the full function signature line as the first line of both OLD and NEW blocks, even if the signature itself is unchanged
 */
std::string tool_patch(const std::string& filename, const std::string& patch_str);
std::string tool_write(const std::string &path, const std::string &data);
