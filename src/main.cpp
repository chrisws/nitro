// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//
//
// A standalone agentic LLM shell with notcurses TUI.
// Uses llama-sb.h as the sole llama.cpp integration layer.
//
// Usage:
//   ./nitro [options] [project_dir]
//
// Options:
//   -m, --model  <path>       GGUF model to load on startup
//   -e, --embed  <path>       embedding model for RAG
//   -g, --gpu-layers <n>      layers to offload to GPU (default: 32)
//

#include <random>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "llama.h"
#include "config.h"
#include "tui.h"
#include "agent_state.h"
#include "logging.h"
#include "curl.h"

// Returns the history file path: ~/.config/nitro/history.txt
static std::string history_path() {
  const char *home = getenv("HOME");
  std::string base = home ? std::string(home) : ".";
  return base + "/.config/nitro/history.txt";
}

//
// Slash command handler
//
static void handle_slash(const std::string &input,
                         NitroConfig       &cfg,
                         AgentState        &agent,
                         Tui          &tui) {
  auto sp = input.find(' ');
  std::string verb = (sp == std::string::npos) ? input : input.substr(0, sp);
  std::string rest;
  if (sp != std::string::npos) {
    rest = input.substr(sp + 1);
    rest.erase(0, rest.find_first_not_of(" \t"));
  }

  if (verb == "/help") {
    tui.show_help();
    return;
  }
  // ── /model ──────────────────────────────────────────────────────────────
  // If no path is given, open the file picker so the user can browse to a
  // GGUF.  The picker starts in the current sandbox directory.
  if (verb == "/model") {
    if (rest.empty()) {
      tui.append_line(ICON_SYS + "Opening model picker…");
      tui.redraw_all();
      rest = tui.file_picker(cfg.sandbox, "Model File");
      if (rest.empty()) {
        tui.append_line(ICON_SYS + "/model cancelled.");
        tui.redraw_all();
        return;
      }
    }
    cfg.model_path = rest;
    if (agent.setup_model(cfg, tui)) {
      std::string sysp = cfg.build_system_prompt();
      agent.reset_conversation(sysp, tui);
      cfg.save_settings();
    }
    tui.redraw_all();
    return;
  }

  // ── /embed ──────────────────────────────────────────────────────────────
  // If no path is given, open the file picker so the user can browse to an
  // embedding GGUF.
  if (verb == "/embed") {
    if (rest.empty()) {
      tui.append_line(ICON_SYS + "Opening embedding model picker…");
      tui.redraw_all();
      rest = tui.file_picker(cfg.sandbox, "Embedding Model");
      if (rest.empty()) {
        tui.append_line(ICON_SYS + "/embed cancelled.");
        tui.redraw_all();
        return;
      }
    }
    cfg.embed_path = rest;
    if (agent.setup_embed(rest, tui)) {
      cfg.save_settings();
    }
    return;
  }

  // ── /rag ────────────────────────────────────────────────────────────────
  if (verb == "/rag") {
    std::string path = rest;
    if (path.empty()) {
      // Launch the interactive folder picker starting from the sandbox.
      path = tui.rag_folder_picker(cfg.sandbox);
      if (path.empty()) {
        tui.append_line(ICON_SYS + "RAG indexing cancelled.");
        tui.redraw_all();
        return;
      }
    }
    if (path.find_last_not_of(".bin") != std::string::npos) {
      tui.append_line(ICON_SYS + "Loading index: " + path);
      tui.redraw_all();
      agent.rag_load_index(path, tui);
    } else {
      tui.append_line(ICON_SYS + "Indexing: " + path);
      tui.redraw_all();
      agent.rag_index(path, cfg, tui);
    }
    tui.append_line(ICON_SYS + "done");
    tui.redraw_all();
    return;
  }

  if (verb == "/memory") {
    std::istringstream iss(agent.memory_info_text());
    std::string line;
    while (std::getline(iss, line)) {
      tui.append_line(ICON_SYS + "" + line);
    }
    tui.redraw_all();
    return;
  }

  if (verb == "/clear") {
    std::string sysp = cfg.build_system_prompt();
    agent.reset_conversation(sysp, tui);
    tui.append_line(ICON_SYS + "Conversation cleared.");
    tui.redraw_all();
    return;
  }

  if (verb == "/theme") {
    tui.toggle_theme();
    return;
  }

  if (verb == "/settings") {
    tui.append_line(ICON_SYS + "Current settings:");
    tui.append_line(ICON_SYS + "  model_path    : " + cfg.model_path);
    tui.append_line(ICON_SYS + "  embed_path    : " + cfg.embed_path);
    tui.append_line(ICON_SYS + "  sandbox       : " + cfg.sandbox);
    tui.append_line(ICON_SYS + "  n_ctx         : " + std::to_string(cfg.n_ctx));
    tui.append_line(ICON_SYS + "  n_gpu_layers  : " + std::to_string(cfg.n_gpu_layers));
    tui.append_line(ICON_SYS + "  temperature   : " + std::to_string(cfg.temperature));
    tui.append_line(ICON_SYS + "  top_p         : " + std::to_string(cfg.top_p));
    tui.append_line(ICON_SYS + "  top_k         : " + std::to_string(cfg.top_k));
    tui.append_line(ICON_SYS + "  penalty_repeat: " + std::to_string(cfg.penalty_repeat));
    tui.append_line(ICON_SYS + "  rag_top_k     : " + std::to_string(cfg.rag_top_k));
    tui.append_line(ICON_SYS + "  saved to      : " + cfg.settings_path());
    tui.redraw_all();
    return;
  }

  if (verb == "/set") {
    // Usage: /set <key> <value>
    // Parses the key and value, updates cfg in place, re-applies generation
    // params if needed, and saves settings to disk.
    auto sp2 = rest.find(' ');
    std::string key = (sp2 == std::string::npos) ? rest : rest.substr(0, sp2);
    std::string val = (sp2 == std::string::npos) ? "" : rest.substr(sp2 + 1);
    val.erase(0, val.find_first_not_of(" \t"));

    if (key.empty() || val.empty()) {
      tui.append_line(ICON_ERR + "Usage: /set <key> <value>");
      tui.redraw_all(); return;
    }

    bool ok = true;
    bool needs_reparam = false;
    try {
      if (key == "temperature")    { cfg.temperature    = std::stof(val); needs_reparam = true; }
      else if (key == "top_p")     { cfg.top_p          = std::stof(val); needs_reparam = true; }
      else if (key == "min_p")     { cfg.min_p          = std::stof(val); needs_reparam = true; }
      else if (key == "top_k")     { cfg.top_k          = std::stoi(val); needs_reparam = true; }
      else if (key == "penalty_repeat") { cfg.penalty_repeat = std::stof(val); needs_reparam = true; }
      else if (key == "penalty_last_n") { cfg.penalty_last_n = std::stoi(val); needs_reparam = true; }
      else if (key == "rag_top_k")      { cfg.rag_top_k      = std::stoi(val); }
      else if (key == "n_gpu_layers")   {
        cfg.n_gpu_layers = std::stoi(val);
        tui.append_line(ICON_SYS + "n_gpu_layers will take effect on next /model load.");
      } else if (key == "run_allowed") {
        // Accept a comma-separated list of basenames, or "none" to clear.
        cfg.run_allowed.clear();
        if (val != "none") {
          std::istringstream iss(val);
          std::string tok;
          while (std::getline(iss, tok, ',')) {
            tok.erase(0, tok.find_first_not_of(" \t"));
            tok.erase(tok.find_last_not_of(" \t") + 1);
            if (!tok.empty()) cfg.run_allowed.push_back(tok);
          }
        }
        if (cfg.run_allowed.empty()) {
          tui.append_line(ICON_SYS + "run_allowed cleared — all sandbox programs permitted.");
        } else {
          std::string list;
          for (const auto &e : cfg.run_allowed) list += e + " ";
          tui.append_line(ICON_SYS + "run_allowed: " + list);
        }
      } else {
        tui.append_line(ICON_ERR + "Unknown key '" + key + "'.  Try /help for list.");
        ok = false;
      }
    } catch (const std::exception &ex) {
      tui.append_line(ICON_ERR + "/set: " + ex.what());
      ok = false;
    }

    if (ok) {
      if (needs_reparam && agent.model_loaded) {
        agent.apply_generation_params(cfg);
      }
      cfg.save_settings();
      tui.append_line(ICON_SYS + "" + key + " = " + val);
    }
    tui.redraw_all();
    return;
  }

  tui.append_line(ICON_ERR + "Unknown command: " + verb + "  (try /help)");
  tui.redraw_all();
}

//
// Welcome banner  — colourful multi-line ASCII logo
//
static void welcome(Tui &tui, const std::string &sandbox) {
  tui.append_line("");
  tui.append_line("[logo_0]  ███╗   ██╗██╗████████╗██████╗  ██████╗ ");
  tui.append_line("[logo_1]  ████╗  ██║██║╚══██╔══╝██╔══██╗██╔═══██╗");
  tui.append_line("[logo_2]  ██╔██╗ ██║██║   ██║   ██████╔╝██║   ██║");
  tui.append_line("[logo_3]  ██║╚██╗██║██║   ██║   ██╔══██╗██║   ██║");
  tui.append_line("[logo_4]  ██║ ╚████║██║   ██║   ██║  ██║╚██████╔╝");
  tui.append_line("[logo_5]  ╚═╝  ╚═══╝╚═╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ");
  tui.append_line("[logo_6]  ─────────── agentic LLM shell v1.0 ──────────────");
  tui.append_line("");
  tui.append_line(ICON_SYS + " Sandbox : " + sandbox);
  tui.append_line(ICON_SYS + " /help for commands  ·  exit to quit");
  tui.append_line("");
  tui.redraw_all();
}

//
// main()
//
int main(int argc, char **argv) {
  // ── Load persisted settings first (provides defaults) ────────────
  NitroConfig cfg;
  // ── Parse arguments (command-line overrides saved settings) ──────
  cfg.load_settings();
  auto resolve_path = [](const std::string &arg) -> std::string {
    if (arg.substr(0, 2) == "~/") {
      const char *home = getenv("HOME");
      return std::string(home ? home : ".") + "/" + arg.substr(2);
    }
    if (arg.substr(0, 2) == "./") {
      std::error_code ec;
      return (fs::current_path(ec) / arg.substr(2)).string();
    }
    return arg;
  };

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto take_next = [&](const char *flag) -> std::string {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "nitro: %s requires an argument\n", flag);
        std::exit(1);
      }
      return argv[++i];
    };
    if (a == "-m" || a == "--model") {
      cfg.model_path = resolve_path(take_next(a.c_str()));
    } else if (a == "-e" || a == "--embed") {
      cfg.embed_path = resolve_path(take_next(a.c_str()));
    } else if (a == "-g" || a == "--gpu-layers") {
      cfg.n_gpu_layers = std::stoi(take_next(a.c_str()));
    } else if (a == "-l" || a == "--log") {
      log_open(take_next(a.c_str()));
    } else if (a == "-t" || a == "--think") {
      cfg.thinking = false;
    } else if (a == "-p" || a == "--prompt-permission") {
      cfg.permission_prompt = true;
    } else if (a == "-h" || a == "--help") {
      std::puts("Usage: nitro [options] [project_dir]\n"
                "\n"
                "Options:\n"
                "  -m, --model  <path>      GGUF model to load on startup\n"
                "  -e, --embed  <path>      embedding model for RAG\n"
                "  -g, --gpu-layers <n>     GPU layers to offload (default: 32)\n"
                "  -l, --log <n>            enabled logging at verbosity level [1-4]\n"
                "  -h, --help               show this help\n"
                "\n"
                "project_dir defaults to the current working directory.\n"
                "Settings are persisted to ~/.config/nitro/settings.json.\n"
                "\n"
                "Slash commands inside nitro:\n"
                "  /model  [path]           load / hot-reload a GGUF (picker if no path)\n"
                "  /embed  [path]           load an embedding model  (picker if no path)\n"
                "  /rag    [path]           index file or directory  (picker if no path)\n"
                "  /memory                  KV / VRAM / layer stats\n"
                "  /settings                show current settings\n"
                "  /clear                   reset conversation\n"
                "  /help                    list commands\n"
                );
      return 0;
    } else if (!a.empty() && a[0] == '-') {
      std::fprintf(stderr, "nitro: unknown option '%s'  (try --help)\n", a.c_str());
      std::exit(1);
    } else {
      cfg.sandbox = resolve_path(a);
    }
  }

  // ── Resolve sandbox ───────────────────────────────────────────────
  if (cfg.sandbox.empty()) {
    std::error_code ec;
    cfg.sandbox = fs::current_path(ec).string();
  }
  {
    std::error_code ec;
    fs::create_directories(cfg.sandbox, ec);
  }

  // ── Auto-discover knowledge files ─────────────────────────────────
  for (const char *kf : {"nitro.md", "AGENTS.md", "README.md"}) {
    if (fs::exists(kf)) {
      cfg.knowledge_files.emplace_back(kf);
    }
  }

  // ── Init curl globally ────────────────────────────────────────────
  curl_init();

  // ── Init TUI ──────────────────────────────────────────────────────
  Tui tui;
  tui.init();
  // Load persisted input history so up-arrow works across sessions.
  tui.history_load(history_path());
  welcome(tui, cfg.sandbox);

  log_write(INFO_LEVEL, "nitro starting");

  // ── Init agent ────────────────────────────────────────────────────
  AgentState agent;
  if (!cfg.model_path.empty()) {
    if (agent.setup_model(cfg, tui)) {
      tui.append_line(ICON_SYS + "Loading context...");
      tui.redraw_all();
      std::string sysp = cfg.build_system_prompt();
      agent.reset_conversation(sysp, tui);
      tui.append_line(ICON_SYS + "Ready");
      tui.redraw_all();
    }
    if (!cfg.embed_path.empty()) {
      agent.setup_embed(cfg.embed_path, tui);
    }
  } else {
    tui.append_line(ICON_SYS + "No model specified.  Use /model to open the file picker,");
    tui.append_line(ICON_SYS + "or /model <path> to load directly.");
    tui.append_line(ICON_SYS + "Example: /model ~/models/qwen2.5-7b-q4_k_m.gguf");
    tui.redraw_all();
  }

  // ── Main loop ─────────────────────────────────────────────────────
  for (;;) {
    tui.resize();
    std::string input = tui.readline();
    input.erase(0, input.find_first_not_of(" \t"));
    if (!input.empty()) {
      input.erase(input.find_last_not_of(" \t\r\n") + 1);
    }
    if (input.empty()) {
      continue;
    }
    if (input == "exit" || input == "quit") {
      break;
    }
    tui.append_line("You: " + input);
    tui.redraw_all();
    if (input[0] == '/') {
      handle_slash(input, cfg, agent, tui);
    } else {
      agent.run_turn(input, cfg, tui);
    }
  }

  log_write(INFO_LEVEL, "nitro exiting");
  log_close();

  // Persist input history for the next session.
  tui.history_save(history_path());
  curl_close();
  return 0;
}
