#include "ytree.h"
#include "tilde.h"
#include "xmalloc.h"


/***************************************************************************
 * InputStr                                                                *
 * Liest eine Zeichenkette an Position (y,x) mit der max. Laenge length    *
 * Vorschlagswert fuer die Eingabe ist s selbst                            *
 * Zurueckgegeben wird das Zeichen, mit dem die Eingabe beendet wurde      *
 ***************************************************************************/

char *StrLeft(const char* str, std::size_t count)
{
#if defined(WITH_UTF8)
  std::mbstate_t state;
  char* p;
#endif
  char* rez;
  char* tmp;
  std::size_t len;

#if defined(WITH_UTF8)
  std::memset(static_cast<void*>(&state), 0, sizeof(state));
#endif
  if (count == 0)
  {
    return Strdup("");
  }
  len = StrVisualLength(str);
  if (count >= len)
  {
    return Strdup(str);
  }

  len = 0;
  tmp = Strdup(str);
#ifdef WITH_UTF8
  p = tmp;
#endif

  for (std::size_t i = 0; i < count; ++i)
  {
#if defined(WITH_UTF8)
    len += std::mbrlen(p, 4, &state);
    p = tmp + len;
#else
    ++len;
#endif
  }

  std::free(static_cast<void*>(tmp));

  rez = Strndup(str, len);
  rez[len] = 0;

  return rez;
}

static char* StrRight(const char* str, std::size_t count)
{
#if defined(WITH_UTF8)
  std::mbstate_t state;
#endif
  char* rez;
  char* p;
  char* tmp;
  std::size_t byte_len;
  std::size_t char_len;
  std::size_t i;

#if defined(WITH_UTF8)
  std::memset(static_cast<void*>(&state), 0, sizeof(state));
#endif

  if (count == 0)
  {
    return Strdup("");
  }

  byte_len = std::strlen(str);
  char_len = StrVisualLength(str);

  if (count > char_len)
  {
    count = char_len;
  }

  tmp = Strdup(str);
  p = tmp;
  i = 0;
  rez = nullptr;

  while ((p - tmp) < byte_len)
  {
    if (i == (char_len - count))
    {
      rez = Strdup(p);
    }
#if defined(WITH_UTF8)
    p += std::mbrlen(p, 4, &state);
#else
    ++p;
#endif
    ++i;
  }

  std::free(static_cast<void*>(tmp));

  return rez;
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
  char* pp;

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

        if (insert_flag && pos >= StrVisualLength(ptr))
        {
          // Append symbol.
          buffer.append(char_buffer);
        } else {
          // Insert / overwrite symbol at cursor position.
          auto ls = pos > 0 ? StrLeft(ptr, pos) : Strdup("");
          auto rs = StrRight(
            ptr,
            StrVisualLength(ptr) - pos - (insert_flag ? 0 : 1)
          );

          buffer.assign(ls);
          std::free(static_cast<void*>(ls));
          buffer.append(char_buffer);
          if (rs)
          {
            buffer.append(rs);
            std::free(static_cast<void*>(rs));
          }
        }
        char_buffer.clear();
        ++pos;
      }

      max_length_reached = StrVisualLength(buffer.c_str()) >= max_length;
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
        if (pos < StrVisualLength(buffer.c_str()))
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

          buffer.assign(ls);
          std::free(static_cast<void*>(ls));
          if (rs)
          {
            buffer.append(rs);
            std::free(static_cast<void*>(rs));
          }
          --pos;
        } else {
          beep();
        }
        break;

      case KEY_DC:
        if (pos < StrVisualLength(buffer.c_str()))
        {
          const auto ptr = buffer.c_str();
          const auto ls = StrLeft(ptr, pos);
          const auto rs = StrRight(ptr, StrVisualLength(ptr) - pos - 1);

          buffer.assign(ls);
          std::free(static_cast<void*>(ls));
          if (rs)
          {
            buffer.append(rs);
            std::free(static_cast<void*>(rs));
          }
        } else {
          beep();
        }
        break;

      case KEY_DL:
        {
          const auto ls = StrLeft(buffer.c_str(), pos);

          buffer.assign(ls);
          std::free(static_cast<void*>(ls));
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
          pos = StrVisualLength(ls);
          std::free(static_cast<void*>(ls));
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
          pos = StrVisualLength(ls);
          std::free(static_cast<void*>(ls));
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
          pos = StrVisualLength(ls);
          std::free(static_cast<void*>(ls));
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

#if defined(READLINE_SUPPORT)
  pp = tilde_expand(buffer.c_str());
#else
  pp = Strdup(buffer.c_str());
#endif

  std::strncpy(s, pp, max_length - 1);
  s[max_length] = 0;
  xfree(static_cast<void*>(pp));

  return c;
}

int InputChoise(const char *msg, const char *term)
{
  int  c;

  ClearHelp();

  curs_set(1);
  leaveok(stdscr, false);
  mvprintw( LINES - 2, 1, msg );
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

  (void) strcpy( path, statistic.tape_name );

  MvAddStr( LINES - 2, 1, "Tape-Device:" );
  if (InputString( path, LINES - 2, 14, 0, COLS - 15) == CR)
  {
    result = 0;
    (void) strcpy( statistic.tape_name, path );
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

#if !defined( linux )
  nodelay( stdscr, true );
  if( wgetch( stdscr ) != ERR ) pressed = true;
  nodelay( stdscr, false );
#endif /* linux */

  return( pressed );
}


bool EscapeKeyPressed()
{
  bool pressed = false;
  int  c = 0;

#if !defined( linux )
  nodelay( stdscr, true );
  if( ( c = wgetch( stdscr ) ) != ERR ) pressed = true;
  nodelay( stdscr, false );
#endif /* linux */

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

