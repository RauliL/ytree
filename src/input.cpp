#include "ytree.h"
#include "tilde.h"


/***************************************************************************
 * InputStr                                                                *
 * Liest eine Zeichenkette an Position (y,x) mit der max. Laenge length    *
 * Vorschlagswert fuer die Eingabe ist s selbst                            *
 * Zurueckgegeben wird das Zeichen, mit dem die Eingabe beendet wurde      *
 ***************************************************************************/

std::string StrLeft(const char* str, std::size_t count)
{
#if defined(WITH_UTF8)
  std::mbstate_t state{};
#endif
  std::size_t len = 0;

  if (count == 0)
  {
    return {};
  }

  const auto visual_len = static_cast<std::size_t>(StrVisualLength(str));
  if (count >= visual_len)
  {
    return str;
  }

#if defined(WITH_UTF8)
  const char* p = str;
  for (std::size_t i = 0; i < count; ++i)
  {
    const auto n = std::mbrlen(p, 4, &state);
    if (n == static_cast<std::size_t>(-1) || n == static_cast<std::size_t>(-2) || n == 0)
    {
      break;
    }
    len += n;
    p += n;
  }
#else
  len = count;
#endif

  return { str, len };
}

static std::string StrRight(const char* str, std::size_t count)
{
#if defined(WITH_UTF8)
  std::mbstate_t state{};
#endif
  const auto byte_len = std::strlen(str);
  auto char_len = static_cast<std::size_t>(StrVisualLength(str));

  if (count == 0 || char_len == 0)
  {
    return {};
  }

  if (count > char_len)
  {
    count = char_len;
  }

  const char* p = str;
  std::size_t i = 0;
  std::string result;

  while (static_cast<std::size_t>(p - str) < byte_len)
  {
    if (i == char_len - count)
    {
      result = p;
      break;
    }
#if defined(WITH_UTF8)
    const auto n = std::mbrlen(p, 4, &state);
    if (n == static_cast<std::size_t>(-1) || n == static_cast<std::size_t>(-2) || n == 0)
    {
      break;
    }
    p += n;
#else
    ++p;
#endif
    ++i;
  }

  return result;
}

int StrVisualLength(const char* str)
{
#if defined(WITH_UTF8)
  std::mbstate_t state;
  int len = 0;

  std::memset(static_cast<void*>(&state), 0, sizeof(state));
  len = std::mbsrtowcs(nullptr, &str, std::strlen(str), &state);
  if (len < 0)
  {
    /* Invalid multibyte sequence */
    len = std::strlen(str);
  }

  return len;
#else
  return std::strlen(str);
#endif
}

const char* StrVisualIndex(const char* str, std::size_t index)
{
#if defined(WITH_UTF8)
  std::mbstate_t state{};
  const char* p = str;

  for (std::size_t i = 0; i < index && *p; ++i)
  {
    const auto n = std::mbrlen(p, 4, &state);
    if (n == static_cast<std::size_t>(-1) || n == static_cast<std::size_t>(-2) || n == 0)
    {
      break;
    }
    p += n;
  }

  return p;
#else
  const auto len = std::strlen(str);
  return str + std::min(index, len);
#endif
}

std::string FitVisualWidth(const char* str, std::size_t width, bool left_justify)
{
  const auto len = static_cast<std::size_t>(StrVisualLength(str));

  if (len > width)
  {
    return StrLeft(str, width);
  }

  const auto pad = width - len;
  if (pad == 0)
  {
    return str;
  }

  if (left_justify)
  {
    return std::string(str) + std::string(pad, ' ');
  }

  return std::string(pad, ' ') + str;
}

void TruncateVisual(char* str, std::size_t max_len)
{
  if (static_cast<std::size_t>(StrVisualLength(str)) <= max_len)
  {
    return;
  }

  const auto truncated = StrLeft(str, max_len);
  std::memcpy(str, truncated.c_str(), truncated.size() + 1);
}

static inline void RefreshInputString(
  const std::string& buffer,
  const int y,
  const int x,
  const std::size_t pos,
  const std::size_t max_length
)
{
  MvWAddStr(stdscr, y, x, buffer);
  for (auto i = buffer.length(); i < max_length; ++i)
  {
    mvwaddch(stdscr, y, x + i, '_');
  }
  wmove(stdscr, y, x + pos);
}

int InputString(
  char* s,
  const int y,
  const int x,
  const std::size_t initial_pos,
  const std::size_t max_length
)
{
  static bool insert_flag = true;
  bool max_length_reached = false;
  int c;
  std::string buffer = s;
  std::size_t pos = initial_pos;
  std::string char_buffer;

  /* Feld gefuellt ausgeben */
  /*------------------------*/
  print_time = false;
  curs_set(1);
  leaveok(stdscr, FALSE);
  nodelay(stdscr, TRUE);

  RefreshInputString(buffer, y, x, pos, max_length);

  do
  {
    if ((c = wgetch(stdscr)) == ERR)
    {
      if (!char_buffer.empty())
      {
        const auto ptr = buffer.c_str();

        if (insert_flag && pos >= static_cast<std::size_t>(StrVisualLength(ptr)))
        {
          // Append symbol.
          buffer.append(char_buffer);
        } else {
          // Insert / overwrite symbol at cursor position.
          const auto ls = pos > 0 ? StrLeft(ptr, pos) : std::string{};
          const auto rs = StrRight(
            ptr,
            StrVisualLength(ptr) - static_cast<int>(pos) - (insert_flag ? 0 : 1)
          );

          buffer = ls;
          buffer.append(char_buffer);
          buffer.append(rs);
        }
        char_buffer.clear();
        ++pos;
      }

      max_length_reached =
        static_cast<std::size_t>(StrVisualLength(buffer.c_str())) >= max_length;
      RefreshInputString(buffer, y, x, pos, max_length);
      continue;
    }

    switch (c)
    {
      case 'C' & 0x1f:
        c = 27;
        break;

      case KEY_LEFT:
        if (pos > 0)
        {
          --pos;
        } else {
          beep();
        }
        break;

      case KEY_RIGHT:
        if (pos < static_cast<std::size_t>(StrVisualLength(buffer.c_str())))
        {
          ++pos;
        } else {
          break;
        }
        break;

      case KEY_BACKSPACE:
      case 'H' & 0x1f:
      case 0x7f:
        if (pos > 0)
        {
          const auto ptr = buffer.c_str();
          const auto ls = StrLeft(ptr, pos - 1);
          const auto rs = StrRight(ptr, StrVisualLength(ptr) - pos);

          buffer = ls;
          buffer.append(rs);
          --pos;
        } else {
          beep();
        }
        break;

      case KEY_DC:
        if (pos < static_cast<std::size_t>(StrVisualLength(buffer.c_str())))
        {
          const auto ptr = buffer.c_str();
          const auto ls = StrLeft(ptr, pos);
          const auto rs = StrRight(
            ptr,
            StrVisualLength(ptr) - static_cast<int>(pos) - 1
          );

          buffer = ls;
          buffer.append(rs);
        } else {
          beep();
        }
        break;

      case KEY_DL:
        {
          const auto ls = StrLeft(buffer.c_str(), pos);

          buffer = ls;
          break;
        }

      case KEY_UP:
      {
        const char* pp;

        nodelay(stdscr, FALSE);
        pp = GetHistory();
        nodelay(stdscr, TRUE);
        if (pp && *pp)
        {
          const auto ls = StrLeft(pp, max_length);

          buffer = ls;
          pos = StrVisualLength(ls.c_str());
          MvAddStr(y, x, buffer);
          for (auto i = pos; i < max_length; ++i)
          {
            addch('_');
          }
          RefreshWindow(stdscr);
          doupdate();
        }
        break;
      }

      case KEY_HOME:
      case 'A' & 0x1f:
        pos = 0;
        break;

      case KEY_END:
      case 'E' & 0x1f:
        pos = StrVisualLength(buffer.c_str());
        break;

      case KEY_EIC:
      case KEY_IC:
        insert_flag = !insert_flag;
        break;

      case '\t':
      {
        auto pp = GetMatches(buffer);

        if (!pp)
        {
          break;
        }
        if (*pp)
        {
          const auto ls = StrLeft(pp, max_length);

          buffer = ls;
          pos = StrVisualLength(ls.c_str());
          MvWAddStr(stdscr, y, x, buffer);
          for (auto i = pos; i < max_length; ++i)
          {
            addch('_');
          }
          RefreshWindow(stdscr);
          doupdate();
        }
        std::free(static_cast<void*>(pp));
        break;
      }

#if defined(KEY_F)
      case KEY_F(2):
#endif
      case 'F' & 0x1f:
      {
        char path[PATH_LENGTH + 1];

        if (KeyF2Get(statistic.tree, statistic.disp_begin_pos, statistic.cursor_pos, path))
        {
          break;
        }
        if (*path)
        {
          const auto ls = StrLeft(path, max_length);

          buffer = ls;
          pos = StrVisualLength(ls.c_str());
        }
        break;
      }

      case LF:
        c = CR;
        break;

      default:
        if (c >= ' ' && c < 0xff && c != 127)
        {
          if (max_length_reached)
          {
            beep();
          } else {
            char_buffer.append(1, static_cast<char>(c));
          }
        }
        break;
    }
  }
  while (c != 27 && c != CR);

  wmove(stdscr, y, x + buffer.length());
  for (std::size_t i = 0; i < max_length - buffer.length(); ++i)
  {
    mvwaddch(stdscr, y, x + i, ' ');
  }
  wmove(stdscr, y, x);

  nodelay(stdscr, FALSE);
  leaveok(stdscr, TRUE);
  curs_set(0);
  print_time = true;

  InsHistory(buffer);

  const auto expanded = tilde_expand(buffer);

  std::strncpy(s, expanded.c_str(), max_length - 1);
  s[max_length] = 0;

  return c;
}

int InputChoise(const char *msg, const char *term)
{
  int  c;

  ClearHelp();

  curs_set(1);
  leaveok(stdscr, false);
  mvprintw( LINES - 2, 1, "%s", msg );
  RefreshWindow( stdscr );
  doupdate();
  do
  {
    c = Getch();
    if(c >= 0)
      if( islower( c ) ) c = toupper( c );
  } while( c != -1 && !strchr( term, c ) );

  if(c >= 0)
    echochar( c );

  move( LINES - 2, 1 ); clrtoeol();
  leaveok(stdscr, true);
  curs_set(0);

  return( c );
}





int GetTapeDeviceName( void )
{
  int  result;
  char path[PATH_LENGTH * 2 +1];

  result = -1;

  ClearHelp();

  *std::format_to(path, "{}", statistic.tape_name) = '\0';

  MvAddStr( LINES - 2, 1, "Tape-Device:" );
  if (InputString( path, LINES - 2, 14, 0, COLS - 15) == CR)
  {
    result = 0;
    *std::format_to(statistic.tape_name, "{}", path) = '\0';
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}


void HitReturnToContinue(void)
{
  curs_set(1);
  vidattr( A_REVERSE );
  putp( "[Hit return to continue]" );
  vidattr( 0 );
  (void) fflush( stdout );
  (void) Getch();
  curs_set(0);
  doupdate();
}



bool KeyPressed()
{
  bool pressed = false;

#if !defined( __linux__ )
  nodelay( stdscr, true );
  if( wgetch( stdscr ) != ERR ) pressed = true;
  nodelay( stdscr, false );
#endif /* __linux__ */

  return( pressed );
}


bool EscapeKeyPressed()
{
  bool pressed = false;
  int  c = 0;

#if !defined( __linux__ )
  nodelay( stdscr, true );
  if( ( c = wgetch( stdscr ) ) != ERR ) pressed = true;
  nodelay( stdscr, false );
#endif /* __linux__ */

  return( ( pressed && c == ESC ) ? true : false );
}




#ifdef VI_KEYS

int ViKey( int ch )
{
  switch( ch )
  {
    case VI_KEY_UP:    ch = KEY_UP;    break;
    case VI_KEY_DOWN:  ch = KEY_DOWN;  break;
    case VI_KEY_RIGHT: ch = KEY_RIGHT; break;
    case VI_KEY_LEFT:  ch = KEY_LEFT;  break;
    case VI_KEY_PPAGE: ch = KEY_PPAGE; break;
    case VI_KEY_NPAGE: ch = KEY_NPAGE; break;
  }
  return(ch);
}

#endif /* VI_KEYS */


int Getch()
{
  int c;

  c = getch();

#ifdef KEY_RESIZE
  if(c == KEY_RESIZE) {
    resize_request = true;
    c = -1;
  }
#endif

  return(c);
}

