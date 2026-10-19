#include "./ytree.hpp"

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

bool SystemCall(const std::string& command_line)
{
  bool result;

  PrepareTerminalForShell();
  result = SilentSystemCall(command_line);
  RestoreScreenAfterShell(true);
  GetAvailBytes(statistic.disk_space);

  return result;
}

bool QuerySystemCall(const std::string& command_line)
{
  bool result;

  PrepareTerminalForShell();
  result = SilentSystemCall(command_line);
  HitReturnToContinue();
  RestoreScreenAfterShell(true);
  GetAvailBytes(statistic.disk_space);

  return result;
}

extern struct itimerval value, ovalue;

bool SilentSystemCall(const std::string& command_line)
{
  int status;

  SuspendClock();
  PrepareTerminalForShell();
  status = std::system(command_line.c_str());
  GetAvailBytes(statistic.disk_space);

  return status == EXIT_SUCCESS;
}
