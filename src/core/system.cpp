#include "core/ui.hpp"
#include "ytree.hpp"

bool SystemCall(const std::string& command_line)
{
  bool result;

  ui::session().suspend();
  result = SilentSystemCall(command_line);
  ui::session().resume(true);
  GetAvailBytes(statistic.disk_space);

  return result;
}

bool QuerySystemCall(const std::string& command_line)
{
  bool result;

  ui::session().suspend();
  result = SilentSystemCall(command_line);
  ui::HitReturnToContinue();
  ui::session().resume(true);
  GetAvailBytes(statistic.disk_space);

  return result;
}

extern struct itimerval value, ovalue;

bool SilentSystemCall(const std::string& command_line)
{
  int status;

  SuspendClock();
  ui::session().suspend();
  status = std::system(command_line.c_str());
  GetAvailBytes(statistic.disk_space);

  return status == EXIT_SUCCESS;
}
