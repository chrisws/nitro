// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "llama.h"
#include <notcurses/notcurses.h>
#include "config.h"
#include "tui_state.h"
#include "agent_state.h"
#include "logging.h"
#include "curl.h"

//
// handling for strip_code_fences
//
static const std::vector<std::string> CODE_EXTENSIONS = {
  ".py",".c",".cpp",".h",".bas",".java",".html",".js",".ts",
  ".json",".yaml",".toml",".sh",".go",".rs",".jsx",".tsx"
};

static std::string read_file(const std::string &path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    return "ERROR: cannot open [" + path + "]";
  }
  std::ostringstream oss; oss << f.rdbuf();
  return oss.str();
}

static bool make_dir(const std::string &path) {
  try {
    std::filesystem::path p(path);
    if (fs::exists(p)) {
      return true;
    }
    std::error_code ec;
    return fs::create_directories(p, ec);
  }
  catch (const std::filesystem::filesystem_error &e) {
    log_write(DEBUG_LEVEL, "mkdir failed [%s]", e.what());
    return false;
  }
}

static std::string join_path(const std::string &a, const std::string &b) {
  if (b.empty()) return a;
  if (b[0] == '/') return b;
  std::string pa = a;
  if (!pa.empty() && pa.back() == '/') pa.pop_back();
  std::string pb = (b.front() == '/') ? b.substr(1) : b;
  return pa + "/" + pb;
}

static std::string list_dir(const std::string &path) {
  std::ostringstream oss;
  std::error_code ec;
  for (const auto &e : fs::directory_iterator(path, ec)) {
    if (ec) break;
    std::string name = e.path().filename().string();
    if (name.empty() || name[0] == '.') continue;
    oss << (e.is_directory() ? "[" + name + "]" : name) << "\n";
  }
  return oss.str();
}

static bool path_in_sandbox(const std::string &sandbox, const std::string &path) {
  std::error_code ec;
  auto base   = fs::canonical(sandbox, ec);  if (ec) return false;
  auto target = fs::weakly_canonical(path, ec);
  std::string bstr = base.string() + "/";
  std::string tstr = target.string();
  return tstr == base.string() || tstr.compare(0, bstr.size(), bstr) == 0;
}

static bool write_file(const std::string &path, const std::string &data) {
  fs::path p(path);
  if (p.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
  }
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  f.write(data.data(), (std::streamsize)data.size());
  return f.good();
}

//
// Trims whitespace from both ends of a string
//
static std::string trim(std::string_view str) {
  constexpr std::string_view whitespace = " \t\n\r\f\v";

  // Find the first non-whitespace character
  const auto start = str.find_first_not_of(whitespace);
  if (start == std::string_view::npos) {
    return ""; // The string is entirely whitespace
  }

  // Find the last non-whitespace character
  const auto end = str.find_last_not_of(whitespace);

  // Return the substring between start and end
  return std::string(str.substr(start, end - start + 1));
}

//
// unwrap() - Remove a matching outer "wrapper" from a string.
//
// Trims leading/trailing whitespace first, then checks (in order):
//
//  1. Same-character pairs   "..."  '...'  |...|  `...`
//  2. Mirror pairs           (...)  [...]  {...}
//  3. HTML-like tags         <tag>...</tag>
//  4. Plain angle brackets   <...>          (fallback if tags don't match)
//
// If none of the above apply, returns the whitespace-trimmed input unchanged.
//
// Examples:
//   unwrap("\"hello\"")        -> "hello"
//   unwrap("  [foo]  ")        -> "foo"
//   unwrap("<b>bold</b>")      -> "bold"
//   unwrap("<file>x</file>")   -> "x"
//   unwrap("<hello>")          -> "hello"
//   unwrap("plain")            -> "plain"
//   unwrap("")                 -> ""
//
std::string unwrap(const std::string &input) {
  if (input.empty()) {
    return input;
  }

  size_t left = 0;
  size_t right = input.length() - 1;

  while (left <= right && std::isspace(static_cast<unsigned char>(input[left]))) {
    left++;
  }
  while (left <= right && std::isspace(static_cast<unsigned char>(input[right]))) {
    right--;
  }

  if (left > right) {
    return "";
  }

  // Same-character pairs: "", '', ||, ``
  // Note: [], {} are NOT same-char pairs — they belong in mirror pairs only
  if (input[left] == input[right]) {
    if (input[left] == '"'  || input[left] == '\'' ||
        input[left] == '|'  || input[left] == '`') {
      return input.substr(left + 1, right - left - 1);
    }
  }

  // Mirror pairs: (), [], {}, but NOT <> (handled below as possible HTML tags)
  if (input[left] != input[right]) {
    if ((input[left] == '(' && input[right] == ')') ||
        (input[left] == '[' && input[right] == ']') ||
        (input[left] == '{' && input[right] == '}')) {
      return input.substr(left + 1, right - left - 1);
    }
  }

  // HTML-like tags: <tag>content</tag>
  // Also handles plain <...> as a fallback at the end
  if (input[left] == '<' && input[right] == '>') {
    // Find end of opening tag
    size_t openTagEnd = left + 1;
    while (openTagEnd <= right && input[openTagEnd] != '>') openTagEnd++;

    if (openTagEnd < right) {
      std::string openTagName = input.substr(left + 1, openTagEnd - left - 1);

      // Find start of closing tag (search backwards for '<')
      size_t closeTagStart = right;
      while (closeTagStart > openTagEnd && input[closeTagStart] != '<') closeTagStart--;

      if (closeTagStart > openTagEnd && input[closeTagStart + 1] == '/') {
        std::string closeTagName = input.substr(closeTagStart + 2, right - closeTagStart - 2);

        if (!openTagName.empty() && openTagName == closeTagName) {
          // Return content between the tags
          return input.substr(openTagEnd + 1, closeTagStart - openTagEnd - 1);
        }
      }
    }

    // Fallback: plain <...> with no matching HTML tags — unwrap the angle brackets
    return input.substr(left + 1, right - left - 1);
  }

  return input.substr(left, right - left + 1);
}

static std::string strip_code_fences(const std::string &filename,
                                     const std::string &src) {
  auto ext = fs::path(filename).extension().string();
  bool is_code = ranges::any_of(CODE_EXTENSIONS, [&](const std::string &e){ return ext == e; });
  if (!is_code) {
    return unwrap(src);
  }
  auto pos = src.find("```");
  if (pos == std::string::npos) {
    return src;
  }
  auto nl = src.find('\n', pos + 3);
  if (nl == std::string::npos) {
    return src;
  }
  std::string inner = src.substr(nl + 1);
  auto end = inner.rfind("```");
  if (end != std::string::npos) {
    inner = inner.substr(0, end);
  }
  return inner;
}

void AgentState::apply_generation_params(const NitroConfig &cfg) const {
  llama->add_stop("<|turn|>");
  llama->add_stop("<|im_end|>");
  llama->set_max_tokens(512000);
  llama->set_temperature(cfg.temperature);
  llama->set_top_k(cfg.top_k);
  llama->set_top_p(cfg.top_p);
  llama->set_min_p(cfg.min_p);
  llama->set_penalty_repeat(cfg.penalty_repeat);
  llama->set_penalty_last_n(cfg.penalty_last_n);
  llama->set_log_level(cfg.log_level);
}

//
// Shows a modal loading popup while the model loads.
//
bool AgentState::setup_model(const NitroConfig &cfg, TuiState &tui) {
  if (cfg.model_path.empty()) {
    tui.append_line(ICON_SYS + "No model loaded.  Use /model <path> to load a GGUF.");
    tui.redraw_all();
    return false;
  }

  // Show a modal popup so the user knows loading is in progress.
  std::string model_name = fs::path(cfg.model_path).filename().string();
  tui.show_modal_popup("Loading " + model_name);
  // Destroy the iterator first — it holds references into the llama context.
  // Freeing llama while iter is still alive causes use-after-free / load failure.
  iter.reset();
  model_loaded = false;
  llama = std::make_unique<Llama>();

  apply_generation_params(cfg);
  if (!llama->load_model(cfg.model_path, cfg.n_ctx, cfg.n_batch,
                         cfg.n_gpu_layers, cfg.log_level)) {
    tui.dismiss_modal_popup();
    tui.append_line(ICON_ERR + llama->last_error());
    tui.redraw_all();
    return false;
  }

  tui.dismiss_modal_popup();
  model_loaded = true;
  tui.current_model = model_name;
  tui.append_line(ICON_SYS + "Model ready: " + tui.current_model);
  LlamaMemoryInfo mem = llama->memory_info();
  tui.append_line(ICON_SYS + "" + mem.advice);
  tui.kv_used  = mem.kv_used;
  tui.kv_total = mem.kv_total;
  tui.vram_used  = mem.vram_used;
  tui.vram_total = mem.vram_total;
  tui.append_line(ICON_SYS + "Thinking mode: " + (cfg.thinking ? "enabled" : "disabled"));
  tui.redraw_all();
  return true;
}

bool AgentState::setup_embed(const std::string &path, TuiState &tui) {
  tui.show_modal_popup("Loading embedding model: " + fs::path(path).filename().string());
  tui.redraw_all();
  embed_llama = std::make_unique<Llama>();
  if (!embed_llama->load_embedding_model(path)) {
    tui.dismiss_modal_popup();
    tui.append_line(ICON_ERR + embed_llama->last_error());
    tui.redraw_all();
    embed_llama.reset();
    return false;
  }
  tui.dismiss_modal_popup();
  rag_db      = std::make_unique<RagDB>();
  rag_session = std::make_unique<RagSession>();
  tui.append_line(ICON_SYS + "Embedding model ready.");
  tui.redraw_all();
  return true;
}

void AgentState::reset_conversation(const std::string &sysprompt, TuiState &tui) {
  system_prompt = sysprompt;
  llama->reset();
  apply_generation_params(NitroConfig{});
  iter = std::make_unique<LlamaIter>();
  if (!llama->add_message(*iter, "system", system_prompt)) {
    tui.append_line(ICON_ERR + "System prompt injection: " + llama->last_error());
    tui.redraw_all();
  }
}

float AgentState::tokens_per_sec() const {
  if (!iter) return 0.0f;
  auto now = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(now - iter->_t_start).count();
  if (elapsed <= 0.0 || iter->_tokens_generated <= 0) return 0.0f;
  return (float)(iter->_tokens_generated / elapsed);
}

std::string AgentState::memory_info_status() const {
  float kv_percent = llama->memory_kv_percent();
  auto message = kv_percent > 75 ? "(Warning: Approaching limit)" : "";
  return std::format("\n[KV-INFO] KV Cache: {}%{} [/KV-INFO]", kv_percent, message);
}

std::string AgentState::memory_info_text() const {
  if (!model_loaded) return "No model loaded.";
  LlamaMemoryInfo m = llama->memory_info();
  std::ostringstream oss;
  oss << "KV cache  : " << m.kv_used << " / " << m.kv_total
      << "  (" << m.kv_percent << "%)\n";
  if (m.vram_total > 0) {
    oss << "VRAM      : " << (m.vram_used >> 20) << " MB / "
        << (m.vram_total >> 20) << " MB  (" << m.vram_percent << "%)\n";
  }
  oss << "GPU layers: " << m.n_layers_gpu << " / " << m.n_layers_total << "\n";
  oss << "CPU layers: " << m.n_layers_cpu << "\n";
  oss << "Advice    : " << m.advice << "\n";
  return oss.str();
}

std::string AgentState::rag_tool(const NitroConfig &cfg, const std::string &agent_query) const {
  std::string result;
  if (embed_llama && rag_db && rag_session) {
    result = embed_llama->rag_retrieve(*rag_db, agent_query, cfg.rag_top_k, *rag_session);
    if (result.empty()) {
      result = "RAG: no context found";
    }
  } else {
    result = "RAG: not enabled";
  }
  return result;
}

bool AgentState::rag_load_index(const std::string &path, TuiState &tui) const {
  if (!embed_llama || !rag_db) {
    tui.append_line(ICON_ERR + "Load an embedding model first: /embed <path>");
    tui.redraw_all();
    return false;
  }

  if (!rag_db->load(path)) {
    tui.append_line(ICON_SYS + "failed to load");
    tui.redraw_all();
  }

  return true;
}

bool AgentState::rag_index(const std::string &path, const NitroConfig &cfg, TuiState &tui) const {
  if (!embed_llama || !rag_db) {
    tui.append_line(ICON_ERR + "Load an embedding model first: /embed <path>");
    tui.redraw_all();
    return false;
  }

  auto index_one = [&](const std::string &filepath) {
    tui.append_line(ICON_SYS + "  indexing: " + filepath);
    tui.redraw_all();
    if (!embed_llama->rag_index(*rag_db, filepath)) {
      tui.append_line(ICON_ERR + "rag_load: " + embed_llama->last_error());
      tui.redraw_all();
    }
  };

  // must be set before indexing
  rag_db->embed_dim = embed_llama->get_embed_dim();

  fs::path rp(path);
  std::error_code ec;
  if (fs::is_directory(rp, ec)) {
    for (const auto &entry : fs::recursive_directory_iterator(rp, ec)) {
      if (entry.is_regular_file()) {
        index_one(entry.path().string());
      }
    }
  } else {
    index_one(path);
  }

  std::string save_path = join_path(cfg.sandbox, "rag-index.bin");
  tui.append_line(ICON_SYS + "saving index: " + save_path);
  tui.redraw_all();
  rag_db->save(save_path);

  return true;
}

std::string AgentState::restart(const NitroConfig &cfg, TuiState &tui) {
  if (fs::exists("SESSION.md")) {
    std::vector<std::string> knowledge_files;
    reset_conversation(cfg.build_system_prompt(), tui);
    tui.append_line(ICON_ERR + "Session restarted");
    tui.redraw_all();
    return "continue the pending actions found SESSION.md";
  }
  return "save work context to SESSION.md then try again";
}

//
// Tool dispatch
//
std::string AgentState::process_tool(const std::string &cmd, const NitroConfig &cfg, TuiState &tui) {
  const std::string &sandbox = cfg.sandbox;
  const std::vector<std::string> &run_allowed = cfg.run_allowed;

  std::string op, arg1, arg2;
  auto WS = cmd.find_first_of(" \n");
  if (WS == std::string::npos) {
    op = trim(cmd);
  } else {
    op = trim(cmd.substr(0, WS));
    std::string rest = cmd.substr(WS + 1);
    // clear leading WS from rest
    rest.erase(0, rest.find_first_not_of(" \t"));

    // handle space of newline separator
    // TOOL:WRITE src/render_transport.cpp\n#include "render_transport.h"\n...
    // TOOL:WRITE src/render_transport.cpp #include "render_transport.h"\n ...
    auto NL = rest.find('\n');
    auto SP = rest.find(' ');
    int sep;
    if (NL != std::string::npos && (SP == std::string::npos || NL < SP)) {
      sep = NL;
    } else {
      sep = SP;
    }
    if (sep == std::string::npos) {
      arg1 = rest;
    } else {
      arg1 = rest.substr(0, sep);
      arg2 = rest.substr(sep + 1);
    }
  }

  auto resolve = [&](const std::string &p) -> std::string {
    if (p.empty() || p == ".") {
      return sandbox;
    }
    if (p.substr(0, 2) == "./") {
      return join_path(sandbox, p.substr(2));
    }
    if (p[0] == '/') {
      return p;
    }
    return join_path(sandbox, unwrap(p));
  };

  auto show_tool = [&](const std::string &tool) -> void {
    tui.append_line(ICON_TOOL + "→ " + tool);
    tui.redraw_all();
  };

  if (op == "TOOL:DATE") {
    show_tool(op);
    char buf[32]; time_t t = time(nullptr);
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
    return buf;
  }
  if (op == "TOOL:TIME") {
    show_tool(op);
    char buf[32]; time_t t = time(nullptr);
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
    return buf;
  }
  if (op == "TOOL:RND") {
    show_tool(op);
    return std::to_string((double)rand() / RAND_MAX);
  }
  if (op == "TOOL:RAG") {
    show_tool(op);
    return rag_tool(cfg, arg1);
  }
  if (op == "TOOL:LIST") {
    std::string dir = resolve(arg1);
    show_tool("listing: " + dir);
    return list_dir(dir);
  }
  if (op == "TOOL:EXISTS") {
    std::string p = resolve(arg1);
    show_tool("checking: " + p);
    return fs::exists(p) ? "YES" : "NO";
  }
  if (op == "TOOL:READ") {
    show_tool("reading: " + arg1);
    std::string p = resolve(arg1);
    return read_file(p);
  }
  if (op == "TOOL:WRITE") {
    show_tool("writing: " + arg1);
    std::string p = resolve(arg1);
    if (!path_in_sandbox(sandbox, p)) {
      return "ERROR: path outside sandbox";
    }
    if (cfg.permission_prompt && !tui.confirm_dialog(std::format("Allow model to write {}?", p))) {
      return "ERROR: action prevented by user";
    }
    std::string content = strip_code_fences(arg1, arg2);
    return write_file(p, content) ? "OK: written to " + arg1 : "ERROR: write failed for " + arg1;
  }
  if (op == "TOOL:MKDIR") {
    std::string p = resolve(arg1);
    show_tool("mkdir: " + arg1);
    if (!path_in_sandbox(sandbox, p)) {
      return "ERROR: path outside sandbox";
    }
    return make_dir(p) ? "OK: created " + arg1 : "ERROR: mkdir failed for " + arg1;
  }
  if (op == "TOOL:CURL") {
    show_tool("curl: " + arg1);
    return tool_curl(arg1);
  }
  if (op == "TOOL:INTROSPECT") {
    show_tool("introspecting: " + arg1);
    return cfg.introspect();
  }
  if (op == "TOOL:ASK") {
    tui.set_thinking(false);
    show_tool("asking: " + arg1 + " " + arg2);
    return tui.readline_blocking();
  }
  if (op == "TOOL:RESTART") {
    show_tool("restart ...");
    return restart(cfg, tui);
  }
  if (op == "TOOL:PERMISSION") {
    tui.set_thinking(false);
    show_tool("asking permission: " + arg1 + " " + arg2);
    return tui.confirm_dialog(arg1 + " " + arg2) ? "YES" : "NO";
  }
  if (op == "TOOL:RUN") {
    if (!run_allowed.empty()) {
      bool permitted = ranges::any_of(run_allowed, [&](const std::string &a) {return a == arg1;});
      if (!permitted) {
        return "ERROR: '" + arg1 + "' is not in the TOOL:RUN allowlist. "
          "Use /set run_allowed <name> to permit it.";
      }
    } else if (cfg.permission_prompt && !tui.confirm_dialog(std::format("Allow {} {} to run?", arg1, arg2))) {
      return "ERROR: prevented by user";
    }
    std::string command = arg1 + " " + arg2 + " 2>&1";
    show_tool("running: " + command);
    FILE *fp = popen(command.c_str(), "r");
    if (!fp) {
      return "ERROR: popen failed";
    }
    std::string out;
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) {
      out += buf;
    }
    pclose(fp);
    if (out.size() > 4096) {
      out = out.substr(0, 4096) + "\n…(truncated)";
    }
    return out;
  }
  return "ERROR: unknown tool: [" + op + "]";
}

//
// Agent turn
//
bool AgentState::run_turn(const std::string &user_message, const NitroConfig &cfg, TuiState &tui) {
  if (!model_loaded) {
    tui.append_line(ICON_ERR + "No model loaded. Use /model <path>");
    tui.redraw_all();
    return false;
  }
  std::string effective_message = user_message;
  if (embed_llama && rag_db && rag_session) {
    std::string context = embed_llama->rag_retrieve(*rag_db, user_message, cfg.rag_top_k, *rag_session);
    if (!context.empty()) {
      log_write(DEBUG_LEVEL, "RAG: %s", context.c_str());
      effective_message = "Context:\n" + context + "\n\nUser: " + user_message;
    } else {
      log_write(DEBUG_LEVEL, "RAG: no context found [%s]", embed_llama->last_error());
    }
  }
  if (!iter) {
    tui.append_line(ICON_ERR + "Conversation not initialised (call /clear to reset)");
    tui.redraw_all();
    return false;
  }
  if (!llama->add_message(*iter, "user", effective_message)) {
    tui.append_line(ICON_ERR + "add_message: " + llama->last_error());
    tui.redraw_all();
    return false;
  }
  tui.append_line("Nitro: ");

  // in_think starts false — models that don't use <think> blocks emit
  // visible text immediately.  The spinner activates only while thinking.
  enum {t_init, t_think, t_thunk} think_mode = (cfg.thinking ? t_init : t_thunk);

  tui.set_thinking(true);
  std::string buffer;

  auto invoke_tool = [&](const std::string &buffer, const std::string_view template_str) -> void {
    static constexpr std::string_view END_TOOL = "\nNITRO_END_TOOL";
    static const std::string TOOL_RESULT = "NITRO_TOOL_RESULT: ";

    std::string tool;
    const auto pos = buffer.rfind(END_TOOL);
    if (pos != std::string::npos) {
      tool = buffer.substr(0, pos);
      auto endTool = buffer.substr(pos);
      if (endTool.length() > END_TOOL.length()) {
        log_write(DEBUG_LEVEL, "ERROR: trailing delimiter: [%s]", endTool.c_str());
      }
    } else {
      tool = buffer;
    }
    log_write(DEBUG_LEVEL, "tool request: mode:[%d] [%s]", think_mode, tool.c_str());
    std::string result = process_tool(tool, cfg, tui);
    if (result.empty()) {
      return;
    }
    std::string content = TOOL_RESULT + std::vformat(template_str, std::make_format_args(result)) + memory_info_status();
    log_write(DEBUG_LEVEL, "tool: [%s] result: [%s]", tool.c_str(), result.c_str());
    tui.update_usage(tokens_per_sec(), llama->memory_info());
    if (!llama->add_message(*iter, "tool_result", content)) {
      tui.append_line(ICON_ERR + "tool result inject: " + llama->last_error());
    }
    if (!iter->_has_next) {
      tui.append_line(ICON_ERR + "failed to evoke tool response: " + llama->last_error());
    }
    if (llama->is_memory_flush()) {
      tui.append_line(ICON_ERR + "Warning! - memory has been flushed!");
    }
    tui.redraw_all();
  };

  auto start_think = [&](const std::string &tag) -> void {
    if (think_mode != t_think) {
      auto pos = buffer.find(tag);
      if (pos == 0) {
        think_mode = t_think;
        // display prededing text
        buffer = buffer.substr(0, pos);
      }
    }
  };

  auto end_think = [&](const std::string &tag) -> void {
    if (think_mode == t_think) {
      auto pos = buffer.find(tag);
      if (pos == 0) {
        think_mode = t_thunk;
        // display remaining text
        buffer = buffer.substr(pos + tag.length());
      }
    }
  };

  auto is_escape = [&]() -> bool {
    ncinput ni{};
    notcurses_get_nblock(tui.nc, &ni);
    if (ni.id == NCKEY_ESC) {
      tui.set_thinking(false);
      tui.append_line(ICON_ERR + "Generation cancelled by user (Escape)");
      tui.redraw_all();
    }
    return ni.id == NCKEY_ESC;
  };

  auto fetch_tool = [&]() -> void {
    while (iter->_has_next && !is_escape()) {
      std::string tok = llama->next(*iter);
      buffer += tok;
      tui.tick_spinner();
      auto pos = buffer.find("</think>");
      if (pos != std::string::npos) {
        break;
      }
    }
  };

  while (iter->_has_next && !is_escape()) {
    std::string tok = llama->next(*iter);
    if (tok == "<") {
      // fetch the complete tag
      std::string tag = tok;
      while (iter->_has_next && tag.find(">") == std::string::npos) {
        tag += llama->next(*iter);
      }
      if (tag == "<|think|>") {
        think_mode = t_think;
        continue;
      } else {
        buffer += tag;
      }
    } else {
      buffer += tok;
    }

    start_think("<think>");
    start_think("<|think|>");
    start_think("<think|>");
    start_think("<|channel>thought");

    end_think("</think>");
    end_think("</|think|>");
    end_think("</|think>");
    end_think("<think|>");
    end_think("<channel|>");

    if (think_mode == t_think) {
      tui.tick_spinner();
    }
    auto tool_start = buffer.find("TOOL:");
    if (tool_start == 0) {
      fetch_tool();
      invoke_tool(trim(buffer), "TOOL_RESULT: {}");
      buffer.clear();
      think_mode = t_init;
      continue;
    }
    tool_start = buffer.find("<|tool_call>call:");
    if (tool_start != std::string::npos) {
      // see https://ai.google.dev/gemma/docs/core/prompt-formatting-gemma4
      fetch_tool();
      auto pos = buffer.find_last_not_of("}<tool_call|>");
      if (pos != std::string::npos) {
        buffer = buffer.substr(0, pos);
      }
      pos = buffer.find_first_not_of('{');
      if (pos != std::string::npos) {
        buffer = buffer.substr(0, pos) + buffer.substr(pos + 1);
      }
      invoke_tool(trim(buffer), "<|tool_response>{}<tool_response|>");
      buffer.clear();
      think_mode = t_init;
      continue;
    }
    auto pos = buffer.find('\n');
    if (pos != std::string::npos) {
      if (think_mode == t_think) {
        auto thought = buffer.substr(0, pos + 1);
        if (thought.length() > 1) {
          tui.append_token(ICON_THINK + thought);
        }
      } else {
        tui.append_token(buffer.substr(0, pos + 1));
      }
      buffer = buffer.substr(pos + 1);
    }
  }

  if (!buffer.empty()) {
    tui.append_token(buffer + "\n");
  }

  tui.flush_token_acc();
  tui.set_thinking(false);
  tui.update_usage(tokens_per_sec(), llama->memory_info());

  char stat[128];
  auto patterm = ICON_SYS + "%.1f tok/s  (%d tokens)  KV %.1f%%";
  std::snprintf(stat, sizeof(stat), patterm.c_str(),
                (double)tui.tokens_per_sec,
                iter->_tokens_generated,
                (double)tui.kv_percent);
  tui.append_line(stat);
  tui.redraw_all();
  return true;
}

