#include "ui/curses/curses_ui.hpp"

void UnmapNoticeWindow()
{
  werase(error_window);
  touchwin(stdscr);
  doupdate();
}
