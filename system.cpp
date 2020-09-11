/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/system.c,v 1.12 2008/07/26 14:29:30 werner Exp $
 *
 * System Call
 *
 ***************************************************************************/
#include "ytree.h"

int SystemCall(const std::string& command_line)
{
  int result;

#if !defined(XCURSES)
  endwin();
#endif
  result = SilentSystemCall(command_line);
  GetAvailBytes(&statistic.disk_space);
  refresh();

  return result;
}

int QuerySystemCall(const std::string& command_line)
{
  int result;

#if !defined(XCURSES)
  endwin();
#endif
  result = SilentSystemCall(command_line);
  HitReturnToContinue();
  GetAvailBytes(&statistic.disk_space);
  refresh();

  return result;
}

int SilentSystemCall(const std::string& command_line)
{
  return SilentSystemCallEx(command_line, true);
}

int SilentSystemCallEx(const std::string& command_line, bool enable_clock)
{
  int result;
#if defined(XCURSES)
  std::string xterm = "xterm -e " + command_line + " &";
#endif

  // Hier ist die einzige Stelle, in der Kommandos aufgerufen werden!
#if defined( __NeXT__ )
  nl();
#endif /* linux */

  SuspendClock();

#if defined(XCURSES)
  result = std::system(xterm.c_str());
#else
  result = std::system(command_line.c_str());
#endif

#if !defined(XCURSES)
  leaveok(stdscr, true);
  curs_set(0);
#if defined( __NeXT__ )
  cbreak();
  nonl();
  noecho();
  clearok(stdscr, true);
#endif /* linux */
#endif /* XCURSES */
  if (enable_clock)
  {
    InitClock();
  }
  GetAvailBytes(&statistic.disk_space);

  return result;
}
