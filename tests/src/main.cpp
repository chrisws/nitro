void file_test();
void graph_test();
void string_utils_test();

#include "logging.h"

int main() {
  log_open_console();
  file_test();
  graph_test();
  string_utils_test();
  return 0;
}
