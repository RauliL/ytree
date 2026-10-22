#include "ui/curses/curses_ui.hpp"

void MvAddStr(int y, int x, const std::string& str)
{
#if defined(WITH_UTF8)
  mvaddstr(y, x, str.c_str());
#else
  for (const auto& c : str)
  {
    mvaddch(y, x++, MakePrintable(c));
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
    mvwaddch(win, y, x++, MakePrintable(c));
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
    waddch(win, MakePrintable(c));
  }
#endif
}

void WAttrAddStr(WINDOW* win, int attr, const std::string& str)
{
  wattrset(win, attr);
  WAddStr(win, str);
  wattrset(win, 0);
}

unsigned char MakePrintable(int ch)
{
  if (std::iscntrl(ch) && static_cast<unsigned char>(ch) < ' ')
  {
    return '?';
  }

  return static_cast<unsigned char>(ch);
}

void PrintSpecialString(
  WINDOW* win,
  int y,
  int x,
  const std::string& str,
  int color
)
{
  if (x < 0 || y < 0)
  {
    return; // Screen too small.
  }

  wmove(win, y, x);

  for (const auto& c : str)
  {
    int result;

    if (!std::iscntrl(c) || !std::isspace(c) || c == ' ')
    {
      switch (c)
      {
        case '1': result = ACS_ULCORNER; break;
        case '2': result = ACS_URCORNER; break;
        case '3': result = ACS_LLCORNER; break;
        case '4': result = ACS_LRCORNER; break;
        case '5': result = ACS_TTEE;     break;
        case '6': result = ACS_LTEE;     break;
        case '7': result = ACS_RTEE;     break;
        case '8': result = ACS_BTEE;     break;
        case '9': result = ACS_LARROW;   break;
        case '|': result = ACS_VLINE;    break;
        case '-': result = ACS_HLINE;    break;
        default:  result = MakePrintable(c);
      }
    } else {
      result = ACS_BLOCK;
    }
#if defined(COLOR_SUPPORT)
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#endif
    waddch(win, result);
#if defined(COLOR_SUPPORT)
    wattrset(win, 0);
#endif
  }
}

void Print(WINDOW* win, int y, int x, const std::string& str, int color)
{
  if (x < 0 || y < 0)
  {
    return; // Screen too small.
  }

  wmove(win, y, x);

  for (const auto& c : str)
  {
#if defined(COLOR_SUPPORT)
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#endif
    waddch(win, MakePrintable(c));
#if defined(COLOR_SUPPORT)
    wattrset(win, 0);
#endif
  }
}

void PrintOptions(WINDOW *win, int y, int x, const std::string& str)
{
  int color, hi_color, lo_color;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }

#ifdef COLOR_SUPPORT
     lo_color = MENU_COLOR;
     hi_color = HIMENUS_COLOR;
#else
     lo_color = A_NORMAL;
     hi_color = A_BOLD;
#endif

  color = lo_color;

  for (const auto& c : str)
  {
    auto ch = static_cast<int>(c);

    switch (c)
    {
        case '(': color = hi_color;  continue;
  case ')': color = lo_color;  continue;

#ifdef COLOR_SUPPORT
  case ']': color = lo_color;  continue;
  case '[': color = hi_color;  continue;
#else
  case ']':
  case '[': /* ignore */ continue;
#endif

        case '1': ch = ACS_ULCORNER; break;
        case '2': ch = ACS_URCORNER; break;
        case '3': ch = ACS_LLCORNER; break;
        case '4': ch = ACS_LRCORNER; break;
        case '5': ch = ACS_TTEE;     break;
        case '6': ch = ACS_LTEE;     break;
        case '7': ch = ACS_RTEE;     break;
        case '8': ch = ACS_BTEE;     break;
        case '9': ch = ACS_LARROW;   break;
        case '|': ch = ACS_VLINE;    break;
        case '-': ch = ACS_HLINE;    break;
        default:  ch = MakePrintable(c);
     }

#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#else
    wattrset(win, color);
#endif
     mvwaddch(win, y, x++, ch);
     wattrset(win, 0);
   }
}


void PrintMenuOptions(
  WINDOW* win,
  int y,
  int x,
  const std::string& str,
  int ncolor,
  int hcolor
)
{
  int color;
  int hi_color;
  int lo_color;
  std::string sbuf;

  if (x < 0 || y < 0)
  {
     /* screen too small */
    return;
  }

#ifdef COLOR_SUPPORT
  lo_color = MENU_COLOR;
  hi_color = HIMENUS_COLOR;
#else
  lo_color = A_NORMAL;
  hi_color = A_REVERSE;
#endif

  color = lo_color;
  wmove(win, y, x);

  for (const auto& c : str)
  {
    auto ch = static_cast<int>(c);

    switch (ch)
    {
      case '(':
        color = hi_color;
        WAttrAddStr(
          win,
#ifdef COLOR_SUPPORT
          COLOR_PAIR(color) | A_BOLD,
#else
          color,
#endif
          sbuf
        );
        sbuf.clear();
        continue;

      case ')':
        color = lo_color;
        WAttrAddStr(
          win,
#ifdef COLOR_SUPPORT
          COLOR_PAIR(color) | A_BOLD,
#else
          color,
#endif
          sbuf
        );
        sbuf.clear();
        continue;

#ifdef COLOR_SUPPORT
      case ']':
        color = lo_color;
        WAttrAddStr(win, COLOR_PAIR(color) | A_BOLD, sbuf);
        sbuf.clear();
        continue;

      case '[':
        color = hi_color;
        WAttrAddStr(win, COLOR_PAIR(color) | A_BOLD, sbuf);
        sbuf.clear();
        continue;
#else
      case ']':
      case '[':
        /* ignore */
        continue;
#endif

      default:
        sbuf += MakePrintable(c);
    }
  }

  WAttrAddStr(
    win,
#ifdef COLOR_SUPPORT
    COLOR_PAIR(color) | A_BOLD,
#else
    color,
#endif
    sbuf
  );
}

void GetMaxYX(WINDOW *win, int *height, int *width)
{
  if( win == dir_window )
  {
    *height = std::max(DIR_WINDOW_HEIGHT, 1);
    *width  = std::max(DIR_WINDOW_WIDTH, 1);
  }
  else if( win == small_file_window )
  {
    *height = std::max(FILE_WINDOW_1_HEIGHT, 1);
    *width  = std::max(FILE_WINDOW_1_WIDTH, 1);
  }
  else if( win == big_file_window )
  {
    *height = std::max(FILE_WINDOW_2_HEIGHT, 1);
    *width  = std::max(FILE_WINDOW_2_WIDTH, 1);
  }
  else if( win == f2_window )
  {
    *height = std::max(F2_WINDOW_HEIGHT - 1, 1); /* fake for separator line */
    *width  = std::max(F2_WINDOW_WIDTH, 1);
  }
  else if( win == history_window )
  {
    *height = std::max(HISTORY_WINDOW_HEIGHT, 1);
    *width  = std::max(HISTORY_WINDOW_WIDTH, 1);
  }
  else
  {
    YTreeError("Unknown Window-ID*ABORT");
    std::exit(EXIT_FAILURE);
  }
}

