#include "notcurses.h"
#include <string>
#include <vector>
#include <memory>

namespace Nitro {

// ─── InputHistory ──────────────────────────────────────────────────────
class InputHistory {
public:
  void push(const std::string &entry);
  bool pop(std::string &entry);
  void reset();

private:
  std::vector<std::string> stack;
  size_t pos = 0;
};

// ─── TuiBase ───────────────────────────────────────────────────────────
class TuiBase {
public:
  TuiBase();
  ~TuiBase();

  void init();
  void destroy();
  void resize();

  bool is_ready() const;

  // Output
  void draw_str_yx(const char *str, int y, int x);
  void draw_char_yx(wchar_t chr, int y, int x);
  void hline(int y, int x, int length);
  void vline(int y, int x, int length);
  void box(int y, int x, int height, int width);
  void fill_rect(int y, int x, int height, int width);

  // Widgets
  void button(const char *label, int y, int x, int width);
  void panel(int y, int x, int height, int width);
  void panel_with_label(const std::string &label, int y, int x, int height, int width);

  // Modals
  void show_modal_popup(const std::string &title, const std::string &message);
  void dismiss_modal_popup();
  bool is_popup_active() const;

  // Input
  std::string readline(const std::string &prompt);

  // State
  void set_thinking(bool on);
  void tick_spinner();

  // Utilities
  std::string file_picker(const std::string &start_dir, const std::string &title_hint) const;
  void flush() const;
  void update_dimensions(unsigned *rows, unsigned *cols);

private:
  notcurses *nc_;
  ncplane *stdpl_;
  ncplane *header_;
  ncplane *chatpl_;
  ncplane *inputpl_;
  ncplane *modal_pl_;

  std::vector<std::string> chat_lines_;
  int scroll_offset_;
  bool thinking_;
  unsigned term_rows_;
  unsigned term_cols_;
  unsigned spinner_frame_;

  InputHistory history_;
  std::string input_buf_;
  size_t cursor_pos_;

  std::unique_ptr<std::mutex> lines_mutex_;
};

} // namespace Nitro

