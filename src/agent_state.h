// This file is part of Nitro
//
// Copyright(C) 2026 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0
// Download the GNU Public License (GPL) from www.gnu.org
//

#pragma once

//
// AgentState
//
struct AgentState {
  std::unique_ptr<Llama> llama;
  std::unique_ptr<LlamaIter> iter;
  std::unique_ptr<Llama> embed_llama;
  std::unique_ptr<RagDB> rag_db;
  std::unique_ptr<RagSession> rag_session;
  bool model_loaded = false;
  std::string system_prompt;

  bool rag_index(const std::string &path, const NitroConfig &cfg, TuiState &tui) const;
  bool rag_load_index(const std::string &path, TuiState &tui) const;
  bool run_turn(const std::string &user_message, const NitroConfig &cfg, TuiState &tui);
  bool setup_embed(const std::string &path, TuiState &tui);
  bool setup_model(const NitroConfig &cfg, TuiState &tui);
  void apply_generation_params(const NitroConfig &cfg) const;
  void reset_conversation(const std::string &sysprompt, TuiState &tui);
  std::string memory_info_status() const;
  std::string memory_info_text() const;
  std::string process_tool(const std::string &cmd, const NitroConfig &cfg, TuiState &tui);
  std::string rag_tool(const NitroConfig &cfg, const std::string &agent_query) const;
  std::string restart(const NitroConfig &cfg, TuiState &tui);
  float tokens_per_sec() const;
};
