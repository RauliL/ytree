/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/system.c,v 1.12 2008/07/26 14:29:30 werner Exp $
 *
 * System Call
 *
 ***************************************************************************/


#include "ytree.h"




int SystemCall(char *command_line)
{
  int result;

  endwin();
  result = SilentSystemCall( command_line );

  (void) GetAvailBytes( &statistic.disk_space );
  refresh();
  return( result );
}


int QuerySystemCall(char *command_line)
{
  int result;

  endwin();
  result = SilentSystemCall( command_line );
  HitReturnToContinue();
  (void) GetAvailBytes( &statistic.disk_space );
  refresh();

  return( result );
}



extern struct itimerval value, ovalue;

int SilentSystemCall(char *command_line)
{
  return(SilentSystemCallEx(command_line, true));
}

int SilentSystemCallEx(char *command_line, bool enable_clock)
{
  int result;

  /* Hier ist die einzige Stelle, in der Kommandos aufgerufen werden! */

    SuspendClock();

  result = system( command_line );

  leaveok(stdscr, true);
  curs_set(0);
  if(enable_clock)
    InitClock();
  (void) GetAvailBytes( &statistic.disk_space );
  return( result );
}


