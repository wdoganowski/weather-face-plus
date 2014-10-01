#include "mainwindow.h"

int main(void) {
  main_window_init();
  app_event_loop();
  main_window_deinit();
}

