#include "ytree.h"
#include "tilde.h"

#include "./mouse.hpp"


/***************************************************************************
 * InputStr                                                                *
 * Liest eine Zeichenkette an Position (y,x) mit der max. Laenge length    *
 * Vorschlagswert fuer die Eingabe ist s selbst                            *
 * Bei Bestaetigung (Return) wird der bearbeitete Text zurueckgegeben.     *
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

int StrVisualLength(std::string_view str)
{
  const std::string tmp(str);
  const char* cstr = tmp.c_str();

#if defined(WITH_UTF8)
  std::mbstate_t state{};
  auto len = static_cast<int>(std::mbsrtowcs(nullptr, &cstr, tmp.size(), &state));
  if (len < 0)
  {
    /* Invalid multibyte sequence */
    len = static_cast<int>(tmp.size());
  }

  return len;
#else
  return static_cast<int>(tmp.size());
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

std::string FitVisualWidth(std::string_view str, std::size_t width, bool left_justify)
{
  const auto len = static_cast<std::size_t>(StrVisualLength(str));

  if (len > width)
  {
    return StrLeft(std::string(str).c_str(), width);
  }

  const auto pad = width - len;
  if (pad == 0)
  {
    return std::string(str);
  }

  if (left_justify)
  {
    return std::string(str) + std::string(pad, ' ');
  }

  return std::string(pad, ' ') + std::string(str);
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

static bool IsInputWordBreak(char c)
{
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

static std::size_t UnixWordRuboutStart(const std::string& buffer, std::size_t pos)
{
  if (pos == 0)
  {
    return 0;
  }

  while (pos > 0)
  {
    const auto* const p = StrVisualIndex(buffer.c_str(), pos - 1);
    if (!IsInputWordBreak(*p))
    {
      break;
    }
    --pos;
  }

  while (pos > 0)
  {
    const auto* const p = StrVisualIndex(buffer.c_str(), pos - 1);
    if (IsInputWordBreak(*p))
    {
      break;
    }
    --pos;
  }

  return pos;
}

static std::size_t InputForwardWordEnd(const std::string& buffer, std::size_t pos)
{
  const auto vis_len = static_cast<std::size_t>(StrVisualLength(buffer));

  while (pos < vis_len)
  {
    const auto* const p = StrVisualIndex(buffer.c_str(), pos);
    if (!IsInputWordBreak(*p))
    {
      break;
    }
    ++pos;
  }

  while (pos < vis_len)
  {
    const auto* const p = StrVisualIndex(buffer.c_str(), pos);
    if (IsInputWordBreak(*p))
    {
      break;
    }
    ++pos;
  }

  return pos;
}

static bool InputApplyWordMotion(
  int key,
  const std::string& buffer,
  std::size_t& pos
)
{
  const auto ch = static_cast<unsigned char>(key);
  const auto letter = static_cast<char>(ch & 0x7f);

  if ((ch & 0x80) == 0)
  {
    return false;
  }

  if (letter == 'f' || letter == 'F')
  {
    const auto new_pos = InputForwardWordEnd(buffer, pos);
    if (new_pos == pos)
    {
      beep();
    } else {
      pos = new_pos;
    }
    return true;
  }

  if (letter == 'b' || letter == 'B')
  {
    const auto new_pos = UnixWordRuboutStart(buffer, pos);
    if (new_pos == pos)
    {
      beep();
    } else {
      pos = new_pos;
    }
    return true;
  }

  return false;
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

struct InputEditorState
{
  std::string buffer;
  std::size_t pos = 0;
  std::string char_buffer;
  bool max_length_reached = false;
};

static std::size_t input_visual_length(const InputEditorState& state)
{
  return static_cast<std::size_t>(StrVisualLength(state.buffer));
}

static void input_commit_pending_chars(InputEditorState& state, const bool insert_flag)
{
  if (state.char_buffer.empty())
  {
    return;
  }

  const auto ptr = state.buffer.c_str();
  const auto vis_len = input_visual_length(state);

  if (insert_flag && state.pos >= vis_len)
  {
    state.buffer.append(state.char_buffer);
  } else {
    const auto ls = state.pos > 0 ? StrLeft(ptr, state.pos) : std::string{};
    const auto rs = StrRight(
      ptr,
      static_cast<int>(vis_len) - static_cast<int>(state.pos) - (insert_flag ? 0 : 1)
    );

    state.buffer = ls;
    state.buffer.append(state.char_buffer);
    state.buffer.append(rs);
  }

  state.char_buffer.clear();
  ++state.pos;
}

static void input_refresh(
  const InputEditorState& state,
  const int y,
  const int x,
  const std::size_t max_length
)
{
  RefreshInputString(state.buffer, y, x, state.pos, max_length);
}

static void input_on_idle(
  InputEditorState& state,
  const bool insert_flag,
  const int y,
  const int x,
  const std::size_t max_length
)
{
  input_commit_pending_chars(state, insert_flag);
  state.max_length_reached = input_visual_length(state) >= max_length;
  input_refresh(state, y, x, max_length);
}

static void input_set_buffer(
  InputEditorState& state,
  const char* text,
  const std::size_t max_length
)
{
  state.buffer = StrLeft(text, max_length);
  state.pos = static_cast<std::size_t>(StrVisualLength(state.buffer));
}

static void input_backspace(InputEditorState& state)
{
  if (state.pos == 0)
  {
    beep();
    return;
  }

  const auto ptr = state.buffer.c_str();
  const auto vis_len = static_cast<int>(input_visual_length(state));
  const auto ls = StrLeft(ptr, state.pos - 1);
  const auto rs = StrRight(ptr, vis_len - static_cast<int>(state.pos));

  state.buffer = ls;
  state.buffer.append(rs);
  --state.pos;
}

static void input_kill_before_cursor(InputEditorState& state)
{
  if (state.pos == 0)
  {
    beep();
    return;
  }

  const auto ptr = state.buffer.c_str();
  const auto rs = StrRight(
    ptr,
    static_cast<int>(input_visual_length(state)) - static_cast<int>(state.pos)
  );

  state.buffer = rs;
  state.pos = 0;
}

static void input_kill_word_before_cursor(InputEditorState& state)
{
  if (state.pos == 0)
  {
    beep();
    return;
  }

  const auto new_pos = UnixWordRuboutStart(state.buffer, state.pos);
  const auto ptr = state.buffer.c_str();
  const auto ls = StrLeft(ptr, new_pos);
  const auto rs = StrRight(
    ptr,
    static_cast<int>(input_visual_length(state)) - static_cast<int>(state.pos)
  );

  state.buffer = ls;
  state.buffer.append(rs);
  state.pos = new_pos;
}

static void input_delete_at_cursor(InputEditorState& state)
{
  const auto vis_len = input_visual_length(state);
  if (state.pos >= vis_len)
  {
    beep();
    return;
  }

  const auto ptr = state.buffer.c_str();
  const auto ls = StrLeft(ptr, state.pos);
  const auto rs = StrRight(
    ptr,
    static_cast<int>(vis_len) - static_cast<int>(state.pos) - 1
  );

  state.buffer = ls;
  state.buffer.append(rs);
}

static void input_kill_after_cursor(InputEditorState& state)
{
  state.buffer = StrLeft(state.buffer.c_str(), state.pos);
}

static void input_move_word_forward(InputEditorState& state)
{
  if (const auto new_pos = InputForwardWordEnd(state.buffer, state.pos); new_pos == state.pos)
  {
    beep();
  } else {
    state.pos = new_pos;
  }
}

static void input_move_word_backward(InputEditorState& state)
{
  if (const auto new_pos = UnixWordRuboutStart(state.buffer, state.pos); new_pos == state.pos)
  {
    beep();
  } else {
    state.pos = new_pos;
  }
}

static void input_handle_escape(InputEditorState& state, int& c)
{
  nodelay(stdscr, FALSE);
  const int next = wgetch(stdscr);
  nodelay(stdscr, TRUE);

  if (next == ERR)
  {
    c = ESC;
    return;
  }

  if (next == 'f' || next == 'F')
  {
    input_move_word_forward(state);
    c = 0;
    return;
  }

  if (next == 'b' || next == 'B')
  {
    input_move_word_backward(state);
    c = 0;
    return;
  }

  c = ESC;
}

static void input_clear_field(
  const int y,
  const int x,
  const std::string& buffer,
  const std::size_t max_length
)
{
  wmove(stdscr, y, x + buffer.length());
  for (std::size_t i = 0; i < max_length - buffer.length(); ++i)
  {
    mvwaddch(stdscr, y, x + i, ' ');
  }
  wmove(stdscr, y, x);
}

static std::string input_finalize_value(
  const InputEditorState& state,
  const std::size_t max_length
)
{
  InsHistory(state.buffer);
  const auto expanded = tilde_expand(state.buffer);
  return StrLeft(expanded.c_str(), max_length);
}

std::optional<std::string> InputString(
  const std::string& initial,
  const int y,
  const int x,
  const std::size_t initial_pos,
  const std::size_t max_length
)
{
  static bool insert_flag = true;
  InputEditorState state{ .buffer = initial, .pos = initial_pos };

  print_time = false;
  curs_set(1);
  leaveok(stdscr, FALSE);
  nodelay(stdscr, TRUE);

  input_refresh(state, y, x, max_length);

  int c = 0;
  while (c != ESC && c != CR)
  {
    if ((c = wgetch(stdscr)) == ERR)
    {
      input_on_idle(state, insert_flag, y, x, max_length);
      continue;
    }

    switch (c)
    {
      case 'C' & 0x1f:
        c = ESC;
        break;

      case KEY_LEFT:
        if (state.pos > 0)
        {
          --state.pos;
        } else {
          beep();
        }
        break;

#if defined(KEY_SLEFT)
      case KEY_SLEFT:
        input_move_word_backward(state);
        break;
#endif

      case KEY_RIGHT:
        if (state.pos < input_visual_length(state))
        {
          ++state.pos;
        }
        break;

#if defined(KEY_SRIGHT)
      case KEY_SRIGHT:
        input_move_word_forward(state);
        break;
#endif

      case ESC:
        input_handle_escape(state, c);
        break;

      case KEY_BACKSPACE:
      case 'H' & 0x1f:
      case 0x7f:
        input_backspace(state);
        break;

      case 'U' & 0x1f:
        input_kill_before_cursor(state);
        break;

      case 'W' & 0x1f:
        input_kill_word_before_cursor(state);
        break;

      case KEY_DC:
        input_delete_at_cursor(state);
        break;

      case KEY_DL:
        input_kill_after_cursor(state);
        break;

      case KEY_UP:
        nodelay(stdscr, FALSE);
        if (const auto selected = GetHistory(); selected && !selected->empty())
        {
          input_set_buffer(state, selected->c_str(), max_length);
          input_refresh(state, y, x, max_length);
          RefreshWindow(stdscr);
          doupdate();
        }
        nodelay(stdscr, TRUE);
        break;

      case KEY_HOME:
      case 'A' & 0x1f:
        state.pos = 0;
        break;

      case KEY_END:
      case 'E' & 0x1f:
        state.pos = input_visual_length(state);
        break;

      case KEY_EIC:
      case KEY_IC:
        insert_flag = !insert_flag;
        break;

      case '\t':
        if (const auto match = GetMatches(state.buffer); match && !match->empty())
        {
          input_set_buffer(state, match->c_str(), max_length);
          input_refresh(state, y, x, max_length);
          RefreshWindow(stdscr);
          doupdate();
        }
        break;

#if defined(KEY_F)
      case KEY_F(2):
#endif
      case 'F' & 0x1f:
      {
        char path[PATH_LENGTH + 1];

        if (KeyF2Get(statistic.tree.get(), statistic.disp_begin_pos, statistic.cursor_pos, path))
        {
          break;
        }
        if (*path)
        {
          input_set_buffer(state, path, max_length);
        }
        break;
      }

      case LF:
        c = CR;
        break;

#ifdef KEY_MOUSE
      case KEY_MOUSE:
        DecodeMouse(MouseFocus::Overlay);
        break;
#endif

      default:
        if (InputApplyWordMotion(c, state.buffer, state.pos))
        {
          c = 0;
        } else if (c >= ' ' && c < 0xff && c != 127)
        {
          if (state.max_length_reached)
          {
            beep();
          } else {
            state.char_buffer.append(1, static_cast<char>(c));
          }
        }
        break;
    }
  }

  input_clear_field(y, x, state.buffer, max_length);

  nodelay(stdscr, FALSE);
  leaveok(stdscr, TRUE);
  curs_set(0);
  print_time = true;

  const auto value = input_finalize_value(state, max_length);
  if (c == CR)
  {
    return value;
  }
  return std::nullopt;
}

int InputChoise(const std::string& msg, const char *term)
{
  int  c;

  ClearHelp();

  curs_set(1);
  leaveok(stdscr, false);
  mvprintw(LINES - 2, 1, "%s", msg.c_str());
  RefreshWindow(stdscr);
  doupdate();
  do
  {
    c = Getch();
#ifdef KEY_MOUSE
    if (c == KEY_MOUSE)
    {
      DecodeMouse(MouseFocus::Overlay);
      continue;
    }
#endif
    if(c >= 0)
      if( std::islower(c) ) c = std::toupper(c);
  } while( c != -1 && !std::strchr(term, c) );

  if(c >= 0)
    echochar(c);

  move(LINES - 2, 1); clrtoeol();
  leaveok(stdscr, true);
  curs_set(0);

  return( c );
}





std::optional<std::string> GetTapeDeviceName()
{
  ClearHelp();

  MvAddStr(LINES - 2, 1, "Tape-Device:");
  const auto tape = InputString(statistic.tape_name, LINES - 2, 14, 0, COLS - 15);
  move(LINES - 2, 1);
  clrtoeol();
  return tape;
}


void HitReturnToContinue()
{
  std::FILE* tty = std::fopen("/dev/tty", "r+");
  if (tty != nullptr)
  {
    std::fputs("\n[Hit return to continue]", tty);
    std::fflush(tty);
    std::fgetc(tty);
    std::fclose(tty);
    return;
  }

  std::fputs("\n[Hit return to continue]", stdout);
  std::fflush(stdout);
  std::fgetc(stdin);
}



bool KeyPressed()
{
  bool pressed = false;

#if !defined( __linux__ )
  nodelay(stdscr, true);
  if( wgetch(stdscr) != ERR ) pressed = true;
  nodelay(stdscr, false);
#endif /* __linux__ */

  return( pressed );
}


bool EscapeKeyPressed()
{
  bool pressed = false;
  int  c = 0;

#if !defined( __linux__ )
  nodelay(stdscr, true);
  if( ( c = wgetch(stdscr) ) != ERR ) pressed = true;
  nodelay(stdscr, false);
#endif /* __linux__ */

  return( ( pressed && c == ESC ) ? true : false );
}




#ifdef VI_KEYS

int ViKey(int ch)
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

