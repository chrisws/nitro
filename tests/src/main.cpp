void file_test();
void graph_test();

#include "logging.h"

int main() {
  log_open_console();
  file_test();
  graph_test();
  return 0;
}
