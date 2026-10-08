#include "ytree.h"

static void PrepareTerminalForShell()
{
  if (!isendwin())
  {
    endwin();
  }
  reset_shell_mode();
  std::fflush(stdout);
  std::fflush(stderr);
  std::fputc('\n', stdout);
  std::fflush(stdout);
}

static void RestoreScreenAfterShell(const bool restart_clock)
{
  if (isendwin())
  {
    refresh();
  }
  leaveok(stdscr, true);
  curs_set(0);
  if (restart_clock)
  {
    InitClock();
  }
}

int SystemCall(const std::string& command_line)
{
  int result;

  PrepareTerminalForShell();
  result = SilentSystemCall(command_line);
  RestoreScreenAfterShell(true);
  GetAvailBytes(&statistic.disk_space);

  return result;
}

int QuerySystemCall(const std::string& command_line)
{
  int result;

  PrepareTerminalForShell();
  result = SilentSystemCall(command_line);
  HitReturnToContinue();
  RestoreScreenAfterShell(true);
  GetAvailBytes(&statistic.disk_space);

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

  (void)enable_clock;

  SuspendClock();

  PrepareTerminalForShell();

  result = std::system(command_line.c_str());

  GetAvailBytes(&statistic.disk_space);

  return result;
}
