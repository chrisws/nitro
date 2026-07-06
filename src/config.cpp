// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <string>
#include <fstream>
#include <format>
#include <sstream>
#include "config.h"

//
// A minimal hand-rolled JSON reader/writer for the flat key-value settings
// we care about.  We deliberately avoid a full JSON library dependency.
//
static bool json_get_string(const std::string &json,
                            const std::string &key,
                            std::string       &out) {
  std::string search = "\"" + key + "\":";
  size_t pos = json.find(search);
  if (pos == std::string::npos) return false;
  pos += search.size();
  while (pos < json.size() && json[pos] == ' ') ++pos;
  if (pos >= json.size() || json[pos] != '"') return false;
  ++pos;
  out.clear();
  while (pos < json.size()) {
    char c = json[pos++];
    if (c == '\\' && pos < json.size()) {
      char e = json[pos++];
      switch (e) {
      case 'n':  out += '\n'; break;
      case 't':  out += '\t'; break;
      case '"':  out += '"';  break;
      case '\\': out += '\\'; break;
      default:   out += e;    break;
      }
    } else if (c == '"') {
      break;
    } else {
      out += c;
    }
  }
  return true;
}

// Tiny helper: extract a quoted string value from flat JSON for a known key.
static bool settings_get_str(const std::string &json,
                             const std::string &key,
                             std::string &out) {
  return json_get_string(json, key, out);
}

// Tiny helper: extract an integer value from flat JSON.
static bool settings_get_int(const std::string &json,
                             const std::string &key,
                             int &out) {
  std::string search = "\"" + key + "\":";
  size_t pos = json.find(search);
  if (pos == std::string::npos) return false;
  pos += search.size();
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
  if (pos >= json.size()) return false;
  // read digits (and optional leading minus)
  size_t start = pos;
  if (json[pos] == '-') ++pos;
  while (pos < json.size() && std::isdigit((unsigned char)json[pos])) ++pos;
  if (pos == start) return false;
  out = std::stoi(json.substr(start, pos - start));
  return true;
}

// Tiny helper: extract a float value from flat JSON.
static bool settings_get_float(const std::string &json,
                               const std::string &key,
                               float &out) {
  std::string search = "\"" + key + "\":";
  size_t pos = json.find(search);
  if (pos == std::string::npos) {
    return false;
  }
  pos += search.size();
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
    ++pos;
  }
  if (pos >= json.size()) {
    return false;
  }
  size_t start = pos;
  if (json[pos] == '-') {
    ++pos;
  }
  while (pos < json.size() && (std::isdigit((unsigned char)json[pos]) || json[pos] == '.')) {
    ++pos;
  }
  if (pos == start) {
    return false;
  }
  out = std::stof(json.substr(start, pos - start));
  return true;
}

//
// Settings persistence  (~/.config/nitro/nitro.settings.json)
// Returns the canonical settings path: ~/.config/nitro/settings.json
//
std::string NitroConfig::settings_path() const {
  // Attempt to read settings from the current working directory first
  if (fs::exists("nitro.config.json")) {
    return "nitro.config.json";
  }
  const char *home = getenv("HOME");
  std::string base = home ? std::string(home) : ".";
  return base + "/.config/nitro/settings.json";
}

// Load settings from disk into cfg.  Fields present in the file overwrite
// the defaults already in cfg; fields absent are left at their defaults.
// Silently succeeds if the file doesn't exist yet.
void NitroConfig::load_settings() {
  std::string path = settings_path();
  std::ifstream f(path);
  if (!f) return;                  // no file → use defaults
  std::ostringstream oss; oss << f.rdbuf();
  std::string json = oss.str();

  thinking = true;

  // String fields
  settings_get_str(json, "model_path",  model_path);
  settings_get_str(json, "embed_path",  embed_path);
  settings_get_str(json, "sandbox",     sandbox);

  // Integer fields
  settings_get_int(json, "n_ctx",          n_ctx);
  settings_get_int(json, "n_batch",        n_batch);
  settings_get_int(json, "n_gpu_layers",   n_gpu_layers);
  settings_get_int(json, "top_k",          top_k);
  settings_get_int(json, "penalty_last_n", penalty_last_n);
  settings_get_int(json, "rag_top_k",      rag_top_k);

  // Float fields
  settings_get_float(json, "temperature",    temperature);
  settings_get_float(json, "top_p",          top_p);
  settings_get_float(json, "min_p",          min_p);
  settings_get_float(json, "penalty_repeat", penalty_repeat);
}

// Persist the current cfg to ~/.config/nitro/settings.json.
bool NitroConfig::save_settings() const {
  std::string path = settings_path();
  fs::path dir = fs::path(path).parent_path();
  std::error_code ec;
  fs::create_directories(dir, ec);

  std::ofstream f(path, std::ios::trunc);
  if (!f) {
    return false;
  }

  f << introspect();

  return f.good();
}

//
// System prompt
//
std::string NitroConfig::build_system_prompt() const {
  std::string p;
  p +=
    "You are Nitro, an agentic AI assistant for software development. "
    "Proceed with caution, guided by logic and the pursuit of knowledge.\n\n"

    "Your sandbox (project directory) is: " + sandbox + "\n\n"

    "## Core Principle\n"
    "Always follow this loop: THINK → DECIDE → ACT → RESPOND\n\n"

    "## Reasoning Protocol\n"
    "Use <|think|> to reason BEFORE acting. Keep it concise and structured.\n"
    "Format:\n"
    "<|think|>\n"
    "- What is the user asking?\n"
    "- Do I need external data (files, tools)?\n"
    "- What is the safest and most correct action?\n"
    "</|think|>\n\n"
    "Rules:\n"
    "- Do NOT call tools inside <|think|>\n"
    "- Do NOT include the final answer inside <|think|>\n"
    "- Always follow <|think|> with either a tool call OR a final answer\n"
    "- Skip <|think|> only for trivial or conversational responses\n\n"

    "## Tool Protocol\n"
    "Emit ONE tool call at a time, immediately followed by NITRO_END_TOOL.\n"
    "Do NOT add any commentary, explanation, or text between the tool call and NITRO_END_TOOL.\n"
    "The host executes the tool and returns NITRO_TOOL_RESULT: <value>.\n"
    "Wait for the result before continuing.\n"
    "After receiving NITRO_TOOL_RESULT you may explain what you did.\n\n"
    "Examples:\n\n"
    "TOOL:LIST\n"
    "NITRO_END_TOOL\n\n"
    "TOOL:READ readme.txt\n"
    "NITRO_END_TOOL\n\n"
    "TOOL:WRITE index.html <!DOCTYPE html><html>...</html>\n"
    "NITRO_END_TOOL\n\n"
    "TOOL:RUN ./build.sh\n"
    "NITRO_END_TOOL\n\n"

    "## Available Tools\n"
    "  TOOL:LIST   [dir]          list files (default: sandbox root)\n"
    "  TOOL:READ   <file>         read file contents\n"
    "  TOOL:WRITE  <file> <text>  write text to file\n"
    "  TOOL:MKDIR  <dir>          create a subfolder inside the sandbox\n"
    "  TOOL:EXISTS <file>         YES or NO\n"
    "  TOOL:RUN    <prog> [args]  run program inside sandbox\n"
    "  TOOL:DATE                  current date\n"
    "  TOOL:TIME                  current time\n"
    "  TOOL:RND                   random float 0..1\n"
    "  TOOL:RAG    <query>        query the RAG index for additional context\n"
    "  TOOL:ASK    <query>        ask the user for clarification or additional context\n"
    "  TOOL:INTROSPECT            show current model settings\n"
    "  TOOL:CURL   <url>          HTTP GET, returns response body (max 32 KB)\n"
    "  TOOL:PERMISSION            ask user for explicit permission\n"
    "  TOOL:RESTART               restart after writing current task context to `SESSION.md`\n\n"

    "## Tool Decision Rules\n"
    "Use tools ONLY if:\n"
    "- The user explicitly references files or the project, OR\n"
    "- The answer depends on local or project data, OR\n"
    "- The user asks for date, time, or a random number\n"
    "Otherwise answer directly using internal knowledge.\n\n"

    "## Tool Rules\n"
    "- NITRO_END_TOOL must immediately follow the tool call — no exceptions\n"
    "- Never add commentary before NITRO_END_TOOL\n"
    "- Only use one tool at a time, step by step\n"
    "- Never access files outside the sandbox\n"
    "- Use TOOL:PERMISSION before destructive or irreversible operations\n"
    "- Do NOT hallucinate file contents\n"
    "- Do NOT fabricate tool outputs\n"
    "- Do NOT assume files exist — use TOOL:EXISTS to check first\n\n"

    "## File Writing Rules\n"
    "Use TOOL:WRITE only if explicitly requested.\n"
    "- Write complete and valid content\n"
    "- Do not overwrite without clear intent\n"
    "- Use TOOL:PERMISSION before overwriting an existing file\n"
    "- Format: TOOL:WRITE <filename> <complete file content>\n"
    "- Do not echo back the KV Cache information as part of the file content\n\n"

    "## Interaction Guidelines\n"
    "- Be precise and efficient\n"
    "- Ask clarifying questions if the request is ambiguous or missing parameters\n"
    "- Prefer direct answers when no tools are needed\n"
    "- After each tool result, explain in plain English what was done\n"
    "- If no user request is provided, respond with a brief readiness message\n\n"

    "## Auto-Restart Protocol\n"
    "**When:** - When KV >= 80% (as reported in the tool results footer).\n"
    "**Steps:**\n"
    "1. **Save State:** Write current task context to `SESSION.md` using `TOOL:WRITE`.\n"
    "   - Include: Timestamp, KV usage, current task description, pending actions, and last conversation summary.\n"
    "   - Don't check if SESSION.md already exists from another session. just use TOOL:WRITE.\n"
    "2. **Trigger Restart:** Call `TOOL:RESTART`.\n"
    "**Example `SESSION.md` Content:**\n"
    "```markdown\n"
    "# Session State Snapshot\n"
    "**Timestamp:** <date> <time>\n"
    "**KV Usage:** <percentage>%\n"
    "**Current Task:** <task description>\n"
    "**Pending Actions:**\n"
    "- <action 1>\n"
    "- <action 2>\n"
    "**Last Output:**\n"
    "<last few lines of conversation>\n"
    "```\n\n";

  for (const auto &kf : knowledge_files) {
    std::ifstream f(kf);
    if (f) {
      std::ostringstream oss; oss << f.rdbuf();
      p += "## Knowledge: " + kf + "\n" + oss.str() + "\n\n";
    }
  }
  return p;
}

std::string NitroConfig::introspect() const {
  static constexpr std::string_view tmpl =
    "{{\n"
    "  \"model_path\":     \"{}\",\n"
    "  \"embed_path\":     \"{}\",\n"
    "  \"sandbox\":        \"{}\",\n"
    "  \"n_ctx\":          {},\n"
    "  \"n_batch\":        {},\n"
    "  \"n_gpu_layers\":   {},\n"
    "  \"temperature\":    {},\n"
    "  \"top_p\":          {},\n"
    "  \"min_p\":          {},\n"
    "  \"top_k\":          {},\n"
    "  \"penalty_repeat\": {},\n"
    "  \"penalty_last_n\": {},\n"
    "  \"rag_top_k\":      {}\n"
    "}}\n";
  return std::format(tmpl,
                     model_path,
                     embed_path,
                     sandbox,
                     n_ctx,
                     n_batch,
                     n_gpu_layers,
                     temperature,
                     top_p,
                     min_p,
                     top_k,
                     penalty_repeat,
                     penalty_last_n,
                     rag_top_k);
}
