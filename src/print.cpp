#include "ytree.h"

void MvAddStr(int y, int x, const std::string& str)
{
#if defined(WITH_UTF8)
  mvaddstr(y, x, str.c_str());
#else
  for (const auto& c : str)
  {
    mvaddch(y, x++, PRINT(c));
  }
#endif
}

void MvWAddStr(WINDOW* win, int y, int x, const std::string& str)
{
#if defined(WITH_UTF8)
  mvwaddstr(win, y, x, str.c_str());
#else
  for (const auto& c : str)
  {
    mvwaddch(win, y, x++, PRINT(c));
  }
#endif
}

void WAddStr(WINDOW* win, const std::string& str)
{
#if defined(WITH_UTF8)
  waddstr(win, str.c_str());
#else
  for (const auto& c : str)
  {
    waddch(win, PRINT(c));
  }
#endif
}

void AddStr(const std::string& str)
{
#if defined(WITH_UTF8)
  addstr(str.c_str());
#else
  for (const auto& c : str)
  {
    addch(PRINT(c));
  }
#endif
}

void WAttrAddStr(WINDOW* win, int attr, const std::string& str)
{
  wattrset(win, attr);
  WAddStr(win, str);
  wattrset(win, 0);
}
