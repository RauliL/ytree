#include "core/ui.hpp"
#include "ytree.hpp"

void Message(const std::string& msg)
{
  ui::dialogs().message(msg);
}

void Warning(const std::string& msg)
{
  ui::dialogs().warning(msg);
}

void ErrorEx(const std::string& msg, const std::string& module, int line)
{
  ui::dialogs().error(msg, module, line);
}

void UnmapNoticeWindow()
{
  werase(error_window);
  touchwin(stdscr);
  doupdate();
}
