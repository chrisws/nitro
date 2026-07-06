// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

#include <string>
#include <vector>
#include <filesystem>  
#include "llama.h"
#include "config.h"

namespace fs = std::filesystem;

//
// NitroConfig
//
struct NitroConfig {
  std::string build_system_prompt() const;
  std::string introspect() const;
  bool save_settings() const;
  void load_settings();
  std::string settings_path() const;
  
  std::string model_path;
  std::string embed_path;
  std::string sandbox;

  int   n_ctx          = 65536;
  int   n_batch        = 512;
  int   n_gpu_layers   = 32;
  int   log_level      = GGML_LOG_LEVEL_CONT;
  float temperature    = 0.6f;
  float top_p          = 0.95f;
  float min_p          = 0.0f;
  int   top_k          = 20;
  float penalty_repeat = 1.0f;
  int   penalty_last_n = 256;
  int   rag_top_k      = 5;
  bool  thinking       = true;
  bool  permission_prompt = false;

  // TOOL:RUN allowlist - if non-empty, only these program basenames may run.
  // Empty means "allow anything inside the sandbox" (original behaviour).
  std::vector<std::string> run_allowed;
  std::vector<std::string> knowledge_files;    
};
