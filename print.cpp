/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/print.c,v 1.3 2005/01/22 16:32:29 werner Exp $
 *
 * Enhanced Curses Functions
 *
 ***************************************************************************/



#include "ytree.h"



int MvAddStr(int y, int x, const char *str)
{
#ifdef WITH_UTF8
  mvaddstr(y, x, str);
#else
  for (const char* p = str; *p; ++p)
  {
    mvaddch(y, x++, PRINT(*p));
  }
#endif
  return 0;
}

int MvWAddStr(WINDOW *win, int y, int x, const char *str)
{
#ifdef WITH_UTF8
  mvwaddstr(win, y, x, str);
#else
  for(;*str != '\0';str++)
      mvwaddch(win, y, x++, PRINT(*str));
#endif
  return 0;
}


int WAddStr(WINDOW *win, const char *str)
{
#ifdef WITH_UTF8
  waddstr(win, str);
#else
  for(;*str != '\0';str++)
      waddch(win, PRINT(*str));
#endif
  return 0;
}

int AddStr(const char *str)
{
#ifdef WITH_UTF8
  addstr(str);
#else
  for(;*str != '\0';str++)
      addch( PRINT(*str));
#endif
  return 0;
}

int WAttrAddStr(WINDOW *win, int attr, char *str)
{
  int rc;

  wattrset( win, attr );
  rc = WAddStr(win, str);
  wattrset( win, 0 );

  return(rc);
}

