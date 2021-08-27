#include "ytree.h"

int SystemCall(const std::string& command_line)
{
  int result;

  endwin();
  result = SilentSystemCall(command_line);
  GetAvailBytes(&statistic.disk_space);
  refresh();

  return result;
}

int QuerySystemCall(const std::string& command_line)
{
  int result;

  endwin();
  result = SilentSystemCall(command_line);
  HitReturnToContinue();
  GetAvailBytes(&statistic.disk_space);
  refresh();

  return result;
}

extern struct itimerval value, ovalue;

int SilentSystemCall(const std::string& command_line)
{
  return SilentSystemCallEx(command_line, true);
}

int SilentSystemCallEx(const std::string& command_line, bool enable_clock)
{
  int result;

  // Hier ist die einzige Stelle, in der Kommandos aufgerufen werden!
  SuspendClock();

  result = std::system(command_line.c_str());

  leaveok(stdscr, true);
  curs_set(0);
  if (enable_clock)
  {
    InitClock();
  }
  GetAvailBytes(&statistic.disk_space);

  return result;
}
