#include "ui/term/term_curses.hpp"

#include <libterm/libterm.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <thread>
#include <utility>
#include <vector>

struct TermCell
{
  std::uint32_t cp = ' ';
  std::uint16_t pair = 0;
  std::uint16_t flags = 0;
  bool cont = false;
};

struct Window
{
  int begy = 0;
  int begx = 0;
  int rows = 0;
  int cols = 0;
  int cury = 0;
  int curx = 0;
  chtype attr = 0;
  chtype bkgd = 0;
  bool scroll_ok = false;
  bool leave_ok = false;
  bool clear_ok = false;
  bool immed_ok = false;
  bool nodelay = false;
  bool dirty = false;
  Window* parent = nullptr;
  std::vector<TermCell> cells;
  std::vector<char> row_dirty;
};

namespace {

constexpr int kNodelayPauseMs = 15;
constexpr uint16_t kBold = 1u << 0;
constexpr uint16_t kReverse = 1u << 1;
constexpr uint16_t kBlink = 1u << 2;
constexpr uint16_t kDim = 1u << 3;
constexpr uint16_t kUnderline = 1u << 4;

struct Rgb
{
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
};

struct Pair
{
  short fg = COLOR_WHITE;
  short bg = COLOR_BLACK;
};

struct Click
{
  int x = -1;
  int y = -1;
  std::chrono::steady_clock::time_point when{};
  bool valid = false;
};

bool g_started = false;
bool g_ended = false;
bool g_need_invalidate = false;
int g_cursor_visibility = 1;
bool g_cursor_valid = false;
int g_cursor_y = 0;
int g_cursor_x = 0;
int g_mouse_interval_ms = 200;
mmask_t g_mouse_mask = 0;
MEVENT g_mouse{};
bool g_have_mouse = false;
Click g_last_click{};
std::deque<int> g_pending;
std::vector<TermCell> g_screen;
std::array<Pair, 256> g_pairs{};
std::array<Rgb, 16> g_palette{};

uint16_t FlagsOf(chtype ch)
{
  uint16_t flags = 0;
  if ((ch & A_BOLD) != 0)
  {
    flags |= kBold;
  }
  if ((ch & A_REVERSE) != 0)
  {
    flags |= kReverse;
  }
  if ((ch & A_BLINK) != 0)
  {
    flags |= kBlink;
  }
  if ((ch & A_DIM) != 0)
  {
    flags |= kDim;
  }
  if ((ch & A_UNDERLINE) != 0)
  {
    flags |= kUnderline;
  }
  return flags;
}

std::uint32_t GlyphOf(chtype ch)
{
  if ((ch & A_ALTCHARSET) != 0)
  {
    return ch & 0xFFFFu;
  }
  const std::uint32_t glyph = ch & 0xFFu;
  return glyph == 0 ? static_cast<std::uint32_t>(' ') : glyph;
}

void ApplyStyle(const Window* win, chtype ch, TermCell& cell)
{
  cell.pair = static_cast<std::uint16_t>((win->bkgd & A_COLOR) >> 16);
  cell.flags = FlagsOf(win->bkgd);
  if ((win->attr & A_COLOR) != 0)
  {
    cell.pair = static_cast<std::uint16_t>((win->attr & A_COLOR) >> 16);
  }
  cell.flags |= FlagsOf(win->attr);
  if ((ch & A_COLOR) != 0)
  {
    cell.pair = static_cast<std::uint16_t>((ch & A_COLOR) >> 16);
  }
  cell.flags |= FlagsOf(ch);
}

TermCell Background(const Window* win)
{
  TermCell cell;
  const chtype ch = win->bkgd;
  cell.cp = GlyphOf(ch);
  cell.pair = static_cast<std::uint16_t>((ch & A_COLOR) >> 16);
  cell.flags = FlagsOf(ch);
  return cell;
}

Window* RootOf(Window* win)
{
  while (win != nullptr && win->parent != nullptr)
  {
    win = win->parent;
  }
  return win;
}

TermCell* TryCell(Window* win, int y, int x)
{
  if (win == nullptr || y < 0 || x < 0 || y >= win->rows || x >= win->cols)
  {
    return nullptr;
  }
  if (win->parent != nullptr)
  {
    Window* root = RootOf(win);
    const int sy = win->begy + y;
    const int sx = win->begx + x;
    if (root == nullptr || sy < 0 || sx < 0 || sy >= root->rows || sx >= root->cols)
    {
      return nullptr;
    }
    return &root->cells[static_cast<std::size_t>(sy * root->cols + sx)];
  }
  return &win->cells[static_cast<std::size_t>(y * win->cols + x)];
}

void EnsureRows(Window* win)
{
  if (win != nullptr && static_cast<int>(win->row_dirty.size()) != win->rows)
  {
    win->row_dirty.assign(static_cast<std::size_t>(std::max(win->rows, 0)), 0);
  }
}

void MarkRow(Window* win, int y)
{
  if (win == nullptr || y < 0 || y >= win->rows)
  {
    return;
  }
  EnsureRows(win);
  win->row_dirty[static_cast<std::size_t>(y)] = 1;
  win->dirty = true;
}

void MarkAll(Window* win)
{
  if (win == nullptr)
  {
    return;
  }
  EnsureRows(win);
  std::fill(win->row_dirty.begin(), win->row_dirty.end(), 1);
  win->dirty = true;
}

void ClearDamage(Window* win)
{
  if (win == nullptr)
  {
    return;
  }
  std::fill(win->row_dirty.begin(), win->row_dirty.end(), 0);
  win->dirty = false;
}

void NoteCursor(const Window* win)
{
  if (win->leave_ok)
  {
    return;
  }
  g_cursor_valid = true;
  g_cursor_y = win->begy + win->cury;
  g_cursor_x = win->begx + win->curx;
}

void ConfigureTerm()
{
  lt_set_output_mode(LT_OUTPUT_TRUECOLOR);
  lt_set_input_mode(LT_INPUT_ESC | LT_INPUT_MOUSE | LT_INPUT_COMPAT);
}

void InitPalette()
{
  static constexpr std::uint32_t kAnsi[16] = {
    0x000000, 0xCC0000, 0x00CC00, 0xCCCC00,
    0x0000CC, 0xCC00CC, 0x00CCCC, 0xEEEEEE,
    0x7F7F7F, 0xFF0000, 0x00FF00, 0xFFFF00,
    0x5C5CFF, 0xFF00FF, 0x00FFFF, 0xFFFFFF,
  };
  for (int i = 0; i < 16; ++i)
  {
    g_palette[static_cast<std::size_t>(i)] = Rgb{
      static_cast<std::uint8_t>((kAnsi[i] >> 16) & 0xFF),
      static_cast<std::uint8_t>((kAnsi[i] >> 8) & 0xFF),
      static_cast<std::uint8_t>(kAnsi[i] & 0xFF),
    };
  }
  for (auto& pair : g_pairs)
  {
    pair = Pair{COLOR_WHITE, COLOR_BLACK};
  }
}

lt_attr PackColor(short index)
{
  if (index < 0 || index > 15)
  {
    index = COLOR_WHITE;
  }
  const Rgb color = g_palette[static_cast<std::size_t>(index)];
  lt_attr packed = LT_RGB(color.r, color.g, color.b);
  if ((packed & 0xFFFFFFu) == 0)
  {
    packed |= LT_HI_BLACK;
  }
  return packed;
}

int EnqueueUnicode(std::uint32_t cp)
{
  if (cp < 256)
  {
    return static_cast<int>(cp);
  }
  char utf8[8] = {};
  const int n = lt_utf8_unicode_to_char(utf8, cp);
  if (n <= 0)
  {
    return '?';
  }
  for (int i = 1; i < n; ++i)
  {
    g_pending.push_back(static_cast<unsigned char>(utf8[i]));
  }
  return static_cast<unsigned char>(utf8[0]);
}

int MapKey(const lt_event& ev)
{
  if (ev.action == LT_KEY_RELEASE)
  {
    return -2;
  }

  const auto key = ev.key;
  const auto ch = ev.ch;
  const bool shift = (ev.mod & LT_MOD_SHIFT) != 0;

  if (key >= LT_KEY_RIGHT_SUPER && key <= LT_KEY_F1)
  {
    if (shift && key == LT_KEY_ARROW_LEFT)
    {
      return KEY_SLEFT;
    }
    if (shift && key == LT_KEY_ARROW_RIGHT)
    {
      return KEY_SRIGHT;
    }
    if (key <= LT_KEY_F1 && key >= LT_KEY_F12)
    {
      return KEY_F(LT_KEY_F1 - key + 1);
    }
    switch (key)
    {
      case LT_KEY_INSERT: return KEY_IC;
      case LT_KEY_DELETE: return KEY_DC;
      case LT_KEY_HOME: return KEY_HOME;
      case LT_KEY_END: return KEY_END;
      case LT_KEY_PGUP: return KEY_PPAGE;
      case LT_KEY_PGDN: return KEY_NPAGE;
      case LT_KEY_ARROW_UP: return KEY_UP;
      case LT_KEY_ARROW_DOWN: return KEY_DOWN;
      case LT_KEY_ARROW_LEFT: return KEY_LEFT;
      case LT_KEY_ARROW_RIGHT: return KEY_RIGHT;
      case LT_KEY_BACK_TAB: return KEY_BTAB;
      default: return -2;
    }
  }

  if (shift && (key == LT_KEY_TAB || ch == '\t'))
  {
    return KEY_BTAB;
  }
  if ((ev.mod & LT_MOD_ALT) != 0 && ch >= 32 && ch < 127)
  {
    g_pending.push_back(static_cast<int>(ch));
    return 0x1B;
  }
  if (ch >= 32 && ch != 127)
  {
    return EnqueueUnicode(ch);
  }
  if (key == LT_KEY_BACKSPACE || key == 0x08)
  {
    return KEY_BACKSPACE;
  }
  if (key == LT_KEY_BACKSPACE2 || key == 0x7F || ch == 0x7F)
  {
    return 0x7F;
  }
  if (key == LT_KEY_ENTER || key == '\r' || ch == '\r')
  {
    return '\r';
  }
  if (key == LT_KEY_ESC || key == 0x1B || ch == 0x1B)
  {
    return 0x1B;
  }
  if (key == LT_KEY_TAB || key == '\t' || ch == '\t')
  {
    return '\t';
  }
  if (key == LT_KEY_SPACE || ch == ' ')
  {
    return ' ';
  }
  if (key > 0 && key < 256)
  {
    return static_cast<int>(key);
  }
  if (ch > 0 && ch < 256)
  {
    return static_cast<int>(ch);
  }
  return -2;
}

bool TakeMouse(const lt_event& ev)
{
  if (ev.action == LT_KEY_RELEASE)
  {
    return false;
  }

  MEVENT event{};
  event.x = ev.x;
  event.y = ev.y;

  if (ev.key == LT_KEY_MOUSE_WHEEL_UP)
  {
    event.bstate = BUTTON4_PRESSED;
  }
  else if (ev.key == LT_KEY_MOUSE_WHEEL_DOWN)
  {
    event.bstate = BUTTON5_PRESSED;
  }
  else if (ev.key == LT_KEY_MOUSE_RIGHT)
  {
    event.bstate = BUTTON3_CLICKED;
  }
  else if (ev.key == LT_KEY_MOUSE_LEFT)
  {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - g_last_click.when
    );
    if (g_last_click.valid && g_last_click.x == ev.x && g_last_click.y == ev.y &&
        elapsed.count() <= g_mouse_interval_ms)
    {
      event.bstate = BUTTON1_DOUBLE_CLICKED;
      g_last_click.valid = false;
    }
    else
    {
      event.bstate = BUTTON1_CLICKED;
      g_last_click = Click{ev.x, ev.y, now, true};
    }
  }
  else
  {
    return false;
  }

  g_mouse = event;
  g_have_mouse = true;
  return true;
}

void ResizeScreen(int rows, int cols)
{
  if (rows < 1)
  {
    rows = 1;
  }
  if (cols < 1)
  {
    cols = 1;
  }
  LINES = rows;
  COLS = cols;
  g_screen.assign(static_cast<std::size_t>(rows * cols), TermCell{});
  g_need_invalidate = true;
  if (stdscr == nullptr)
  {
    return;
  }

  std::vector<TermCell> next(static_cast<std::size_t>(rows * cols), TermCell{});
  const int copy_rows = std::min(rows, stdscr->rows);
  const int copy_cols = std::min(cols, stdscr->cols);
  for (int y = 0; y < copy_rows; ++y)
  {
    for (int x = 0; x < copy_cols; ++x)
    {
      next[static_cast<std::size_t>(y * cols + x)] =
        stdscr->cells[static_cast<std::size_t>(y * stdscr->cols + x)];
    }
  }
  stdscr->cells = std::move(next);
  stdscr->rows = rows;
  stdscr->cols = cols;
  if (stdscr->cury >= rows)
  {
    stdscr->cury = rows - 1;
  }
  if (stdscr->curx >= cols)
  {
    stdscr->curx = cols - 1;
  }
  MarkAll(stdscr);
}

int RestartTerm()
{
  if (lt_init() != LT_OK)
  {
    return ERR;
  }
  g_started = true;
  g_ended = false;
  ConfigureTerm();
  int rows = lt_height();
  int cols = lt_width();
  if (rows < 1)
  {
    rows = 24;
  }
  if (cols < 1)
  {
    cols = 80;
  }
  if (rows != LINES || cols != COLS)
  {
    ResizeScreen(rows, cols);
  }
  g_need_invalidate = true;
  MarkAll(stdscr);
  return OK;
}

WINDOW* MakeWindow(int rows, int cols, int begy, int begx, Window* parent)
{
  auto* win = new Window;
  win->rows = rows;
  win->cols = cols;
  win->begy = begy;
  win->begx = begx;
  win->parent = parent;
  win->row_dirty.assign(static_cast<std::size_t>(rows), 0);
  if (parent == nullptr)
  {
    win->cells.assign(static_cast<std::size_t>(rows * cols), TermCell{});
  }
  return win;
}

} // namespace

WINDOW* stdscr = nullptr;
int LINES = 0;
int COLS = 0;
int COLORS = 16;
int COLOR_PAIRS = 256;

namespace {

int Place(Window* win, TermCell cell, int width)
{
  if (win == nullptr || win->rows <= 0 || win->cols <= 0)
  {
    return ERR;
  }
  if (width < 1)
  {
    width = 1;
  }
  if (win->cury < 0 || win->cury >= win->rows)
  {
    win->cury = std::clamp(win->cury, 0, win->rows - 1);
  }
  if (win->curx < 0)
  {
    win->curx = 0;
  }
  if (win->curx + width > win->cols)
  {
    win->curx = 0;
    if (win->cury + 1 < win->rows)
    {
      ++win->cury;
    }
    else if (win->scroll_ok)
    {
      wscrl(win, 1);
      win->cury = win->rows - 1;
    }
    else
    {
      win->curx = std::max(0, win->cols - width);
      win->cury = win->rows - 1;
    }
  }

  const int painted_row = win->cury;
  if (TermCell* slot = TryCell(win, win->cury, win->curx))
  {
    *slot = cell;
  }
  if (width > 1)
  {
    TermCell cont = cell;
    cont.cp = 0;
    cont.cont = true;
    if (TermCell* slot = TryCell(win, win->cury, win->curx + 1))
    {
      *slot = cont;
    }
  }

  win->curx += width;
  if (win->curx >= win->cols)
  {
    if (win->cury + 1 < win->rows)
    {
      win->curx = 0;
      ++win->cury;
    }
    else if (win->scroll_ok)
    {
      wscrl(win, 1);
      win->curx = 0;
      win->cury = win->rows - 1;
    }
    else
    {
      win->curx = win->cols - 1;
    }
  }

  MarkRow(win, painted_row);
  if (win->immed_ok)
  {
    wrefresh(win);
  }
  return OK;
}

void Present()
{
  if (!g_started || g_ended || LINES <= 0 || COLS <= 0)
  {
    return;
  }
  if (static_cast<int>(g_screen.size()) != LINES * COLS)
  {
    g_screen.assign(static_cast<std::size_t>(LINES * COLS), TermCell{});
  }
  if (g_need_invalidate)
  {
    lt_invalidate();
    g_need_invalidate = false;
  }

  for (int y = 0; y < LINES; ++y)
  {
    for (int x = 0; x < COLS; ++x)
    {
      const TermCell& cell = g_screen[static_cast<std::size_t>(y * COLS + x)];
      const int pair = cell.pair > 0 && cell.pair < g_pairs.size()
        ? cell.pair
        : 0;
      short fg_index = g_pairs[static_cast<std::size_t>(pair)].fg;
      short bg_index = g_pairs[static_cast<std::size_t>(pair)].bg;
      lt_attr fg = PackColor(fg_index);
      lt_attr bg = PackColor(bg_index);
      if ((cell.flags & kReverse) != 0)
      {
        std::swap(fg, bg);
      }
      if ((cell.flags & kBold) != 0)
      {
        fg |= LT_BOLD;
      }
      if ((cell.flags & kBlink) != 0)
      {
        fg |= LT_BLINK;
      }
      if ((cell.flags & kDim) != 0)
      {
        fg |= LT_DIM;
      }
      if ((cell.flags & kUnderline) != 0)
      {
        fg |= LT_UNDERLINE;
      }
      const std::uint32_t cp = cell.cont ? 0 : cell.cp;
      lt_set_cell(x, y, cp, fg, bg);
    }
  }

  if (g_cursor_visibility > 0 && g_cursor_valid &&
      g_cursor_x >= 0 && g_cursor_y >= 0 &&
      g_cursor_x < COLS && g_cursor_y < LINES)
  {
    lt_set_cursor(g_cursor_x, g_cursor_y);
  }
  else
  {
    lt_hide_cursor();
  }
  lt_present();
}

} // namespace

WINDOW* initscr()
{
  if (g_started && !g_ended && stdscr != nullptr)
  {
    return stdscr;
  }
  if (lt_init() != LT_OK)
  {
    std::fputs("ytree: cannot initialize the terminal\n", stderr);
    std::exit(EXIT_FAILURE);
  }
  g_started = true;
  g_ended = false;
  ConfigureTerm();
  InitPalette();

  int rows = lt_height();
  int cols = lt_width();
  if (rows < 1)
  {
    rows = 24;
  }
  if (cols < 1)
  {
    cols = 80;
  }
  LINES = rows;
  COLS = cols;
  COLORS = 16;
  COLOR_PAIRS = static_cast<int>(g_pairs.size());
  stdscr = MakeWindow(rows, cols, 0, 0, nullptr);
  g_screen.assign(static_cast<std::size_t>(rows * cols), TermCell{});
  g_need_invalidate = true;
  return stdscr;
}

int endwin()
{
  if (g_started && !g_ended)
  {
    lt_shutdown();
    g_ended = true;
  }
  return OK;
}

bool isendwin()
{
  return g_ended;
}

int wnoutrefresh(WINDOW* win)
{
  if (win == nullptr || LINES <= 0 || COLS <= 0)
  {
    return ERR;
  }
  if (static_cast<int>(g_screen.size()) != LINES * COLS)
  {
    g_screen.assign(static_cast<std::size_t>(LINES * COLS), TermCell{});
  }
  const bool copy_all = win->clear_ok;
  if (win->clear_ok)
  {
    g_need_invalidate = true;
    win->clear_ok = false;
  }
  EnsureRows(win);

  const TermCell blank = Background(win);
  for (int y = 0; y < win->rows; ++y)
  {
    if (!copy_all && win->row_dirty[static_cast<std::size_t>(y)] == 0)
    {
      continue;
    }
    const int sy = win->begy + y;
    if (sy < 0 || sy >= LINES)
    {
      continue;
    }
    for (int x = 0; x < win->cols; ++x)
    {
      const int sx = win->begx + x;
      if (sx < 0 || sx >= COLS)
      {
        continue;
      }
      const TermCell* src = TryCell(win, y, x);
      g_screen[static_cast<std::size_t>(sy * COLS + sx)] = src != nullptr ? *src : blank;
    }
  }
  NoteCursor(win);
  ClearDamage(win);
  return OK;
}

int doupdate()
{
  if (!g_started || g_ended)
  {
    return ERR;
  }
  Present();
  return OK;
}

int wrefresh(WINDOW* win)
{
  if (wnoutrefresh(win) == ERR)
  {
    return ERR;
  }
  return doupdate();
}

int refresh()
{
  if (g_ended)
  {
    if (RestartTerm() == ERR)
    {
      return ERR;
    }
  }
  return wrefresh(stdscr);
}

WINDOW* newwin(int nlines, int ncols, int begin_y, int begin_x)
{
  if (nlines <= 0 || ncols <= 0)
  {
    return nullptr;
  }
  return MakeWindow(nlines, ncols, begin_y, begin_x, nullptr);
}

WINDOW* subwin(WINDOW* orig, int nlines, int ncols, int begin_y, int begin_x)
{
  if (orig == nullptr || nlines <= 0 || ncols <= 0)
  {
    return nullptr;
  }
  return MakeWindow(nlines, ncols, begin_y, begin_x, orig);
}

int delwin(WINDOW* win)
{
  if (win == nullptr || win == stdscr)
  {
    return ERR;
  }
  delete win;
  return OK;
}

int cbreak()
{
  return OK;
}

int noecho()
{
  return OK;
}

int nonl()
{
  return OK;
}

int raw()
{
  return OK;
}

int keypad(WINDOW*, int)
{
  return OK;
}

int nodelay(WINDOW* win, int bf)
{
  if (win == nullptr)
  {
    return ERR;
  }
  win->nodelay = bf != 0;
  return OK;
}

int scrollok(WINDOW* win, int bf)
{
  if (win == nullptr)
  {
    return ERR;
  }
  win->scroll_ok = bf != 0;
  return OK;
}

int clearok(WINDOW* win, int bf)
{
  if (win == nullptr)
  {
    return ERR;
  }
  win->clear_ok = bf != 0;
  return OK;
}

int leaveok(WINDOW* win, int bf)
{
  if (win == nullptr)
  {
    return ERR;
  }
  win->leave_ok = bf != 0;
  return OK;
}

int immedok(WINDOW* win, int bf)
{
  if (win == nullptr)
  {
    return ERR;
  }
  win->immed_ok = bf != 0;
  return OK;
}

int curs_set(int visibility)
{
  const int previous = g_cursor_visibility;
  g_cursor_visibility = visibility;
  return previous;
}

int typeahead(int)
{
  return OK;
}

int baudrate()
{
  return 38400;
}

int reset_shell_mode()
{
  return endwin();
}

int start_color()
{
  return OK;
}

int init_pair(short pair, short fg, short bg)
{
  if (pair <= 0 || pair >= static_cast<short>(g_pairs.size()))
  {
    return ERR;
  }
  g_pairs[static_cast<std::size_t>(pair)] = Pair{fg, bg};
  return OK;
}

int init_color(short color, short r, short g, short b)
{
  if (color < 0 || color >= static_cast<short>(g_palette.size()))
  {
    return ERR;
  }
  const auto channel = [](short value) -> std::uint8_t {
    if (value < 0)
    {
      value = 0;
    }
    if (value > 1000)
    {
      value = 1000;
    }
    return static_cast<std::uint8_t>((value * 255 + 500) / 1000);
  };
  g_palette[static_cast<std::size_t>(color)] = Rgb{channel(r), channel(g), channel(b)};
  return OK;
}

int can_change_color()
{
  return TRUE;
}

int wbkgdset(WINDOW* win, chtype ch)
{
  if (win == nullptr)
  {
    return ERR;
  }
  win->bkgd = ch;
  return OK;
}

int wattrset(WINDOW* win, chtype attrs)
{
  if (win == nullptr)
  {
    return ERR;
  }
  win->attr = attrs;
  return OK;
}

int werase(WINDOW* win)
{
  if (win == nullptr)
  {
    return ERR;
  }
  const TermCell blank = Background(win);
  for (int y = 0; y < win->rows; ++y)
  {
    for (int x = 0; x < win->cols; ++x)
    {
      if (TermCell* slot = TryCell(win, y, x))
      {
        *slot = blank;
      }
    }
  }
  win->cury = 0;
  win->curx = 0;
  MarkAll(win);
  return OK;
}

int wclear(WINDOW* win)
{
  if (werase(win) == ERR)
  {
    return ERR;
  }
  win->clear_ok = true;
  return OK;
}

int clear()
{
  return wclear(stdscr);
}

int wmove(WINDOW* win, int y, int x)
{
  if (win == nullptr || y < 0 || x < 0 || y >= win->rows || x >= win->cols)
  {
    return ERR;
  }
  win->cury = y;
  win->curx = x;
  return OK;
}

int wclrtoeol(WINDOW* win)
{
  if (win == nullptr || win->cury < 0 || win->cury >= win->rows)
  {
    return ERR;
  }
  const TermCell blank = Background(win);
  for (int x = std::max(win->curx, 0); x < win->cols; ++x)
  {
    if (TermCell* slot = TryCell(win, win->cury, x))
    {
      *slot = blank;
    }
  }
  MarkRow(win, win->cury);
  return OK;
}

int waddch(WINDOW* win, chtype ch)
{
  if (win == nullptr)
  {
    return ERR;
  }
  TermCell cell;
  cell.cp = GlyphOf(ch);
  ApplyStyle(win, ch, cell);
  int width = 1;
  if ((ch & A_ALTCHARSET) != 0 || cell.cp > 127)
  {
    const int measured = lt_wcwidth(cell.cp);
    if (measured > 1)
    {
      width = measured;
    }
  }
  return Place(win, cell, width);
}

int waddstr(WINDOW* win, const char* text)
{
  if (win == nullptr || text == nullptr)
  {
    return ERR;
  }
  const bool immediate = win->immed_ok;
  win->immed_ok = false;
  while (*text != '\0')
  {
    std::uint32_t cp = 0;
    const int n = lt_utf8_char_to_unicode(&cp, text);
    if (n <= 0)
    {
      waddch(win, static_cast<unsigned char>(*text));
      ++text;
      continue;
    }
    int width = lt_wcwidth(cp);
    if (width < 0)
    {
      cp = '?';
      width = 1;
    }
    if (width == 0)
    {
      text += n;
      continue;
    }
    TermCell cell;
    cell.cp = cp;
    ApplyStyle(win, 0, cell);
    Place(win, cell, width);
    text += n;
  }
  win->immed_ok = immediate;
  if (immediate)
  {
    wrefresh(win);
  }
  return OK;
}

int mvwaddstr(WINDOW* win, int y, int x, const char* text)
{
  if (wmove(win, y, x) == ERR)
  {
    return ERR;
  }
  return waddstr(win, text);
}

int mvwprintw(WINDOW* win, int y, int x, const char* fmt, ...)
{
  char buffer[1024];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buffer, sizeof buffer, fmt, args);
  va_end(args);
  return mvwaddstr(win, y, x, buffer);
}

int mvprintw(int y, int x, const char* fmt, ...)
{
  char buffer[1024];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buffer, sizeof buffer, fmt, args);
  va_end(args);
  return mvwaddstr(stdscr, y, x, buffer);
}

int echochar(chtype ch)
{
  if (waddch(stdscr, ch) == ERR)
  {
    return ERR;
  }
  return wrefresh(stdscr);
}

int box(WINDOW* win, chtype verch, chtype horch)
{
  if (win == nullptr || win->rows < 1 || win->cols < 1)
  {
    return ERR;
  }
  if (verch == 0)
  {
    verch = ACS_VLINE;
  }
  if (horch == 0)
  {
    horch = ACS_HLINE;
  }
  const int saved_y = win->cury;
  const int saved_x = win->curx;
  const int last_y = win->rows - 1;
  const int last_x = win->cols - 1;
  for (int x = 0; x < win->cols; ++x)
  {
    const chtype top = x == 0 ? ACS_ULCORNER : (x == last_x ? ACS_URCORNER : horch);
    const chtype bottom = x == 0 ? ACS_LLCORNER : (x == last_x ? ACS_LRCORNER : horch);
    mvwaddch(win, 0, x, top);
    if (last_y != 0)
    {
      mvwaddch(win, last_y, x, bottom);
    }
  }
  for (int y = 1; y < last_y; ++y)
  {
    mvwaddch(win, y, 0, verch);
    if (last_x != 0)
    {
      mvwaddch(win, y, last_x, verch);
    }
  }
  wmove(win, saved_y, saved_x);
  return OK;
}

int wscrl(WINDOW* win, int n)
{
  if (win == nullptr)
  {
    return ERR;
  }
  if (n == 0 || win->rows <= 0)
  {
    return OK;
  }
  if (n > win->rows)
  {
    n = win->rows;
  }
  if (n < -win->rows)
  {
    n = -win->rows;
  }
  const TermCell blank = Background(win);
  if (n > 0)
  {
    for (int y = 0; y < win->rows - n; ++y)
    {
      for (int x = 0; x < win->cols; ++x)
      {
        TermCell* dst = TryCell(win, y, x);
        TermCell* src = TryCell(win, y + n, x);
        if (dst != nullptr)
        {
          *dst = src != nullptr ? *src : blank;
        }
      }
    }
    for (int y = win->rows - n; y < win->rows; ++y)
    {
      for (int x = 0; x < win->cols; ++x)
      {
        if (TermCell* dst = TryCell(win, y, x))
        {
          *dst = blank;
        }
      }
    }
  }
  else
  {
    const int k = -n;
    for (int y = win->rows - 1; y >= k; --y)
    {
      for (int x = 0; x < win->cols; ++x)
      {
        TermCell* dst = TryCell(win, y, x);
        TermCell* src = TryCell(win, y - k, x);
        if (dst != nullptr)
        {
          *dst = src != nullptr ? *src : blank;
        }
      }
    }
    for (int y = 0; y < k; ++y)
    {
      for (int x = 0; x < win->cols; ++x)
      {
        if (TermCell* dst = TryCell(win, y, x))
        {
          *dst = blank;
        }
      }
    }
  }
  MarkAll(win);
  return OK;
}

int scroll(WINDOW* win)
{
  if (win == nullptr || !win->scroll_ok)
  {
    return ERR;
  }
  return wscrl(win, 1);
}

int winsertln(WINDOW* win)
{
  if (win == nullptr || win->cury < 0 || win->cury >= win->rows)
  {
    return ERR;
  }
  const TermCell blank = Background(win);
  for (int y = win->rows - 1; y > win->cury; --y)
  {
    for (int x = 0; x < win->cols; ++x)
    {
      TermCell* dst = TryCell(win, y, x);
      TermCell* src = TryCell(win, y - 1, x);
      if (dst != nullptr)
      {
        *dst = src != nullptr ? *src : blank;
      }
    }
  }
  for (int x = 0; x < win->cols; ++x)
  {
    if (TermCell* dst = TryCell(win, win->cury, x))
    {
      *dst = blank;
    }
  }
  MarkAll(win);
  return OK;
}

int touchwin(WINDOW* win)
{
  if (win == nullptr)
  {
    return ERR;
  }
  MarkAll(win);
  return OK;
}

int beep()
{
  if (g_started && !g_ended)
  {
    lt_send("\a", 1);
    return OK;
  }
  std::fputc('\a', stderr);
  return OK;
}

int wgetch(WINDOW* win)
{
  if (win == nullptr || g_ended)
  {
    return ERR;
  }
  if (!g_pending.empty())
  {
    const int key = g_pending.front();
    g_pending.pop_front();
    return key;
  }

  const bool was_dirty = win->dirty;
  if (was_dirty)
  {
    wrefresh(win);
  }

  for (;;)
  {
    lt_event ev{};
    const int rc = lt_peek_event(&ev, win->nodelay ? 0 : -1);
    if (rc == LT_ERR_NO_EVENT)
    {
      if (!win->nodelay)
      {
        continue;
      }
      if (was_dirty)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(kNodelayPauseMs));
      }
      return ERR;
    }
    if (rc != LT_OK)
    {
      return ERR;
    }
    if (ev.type == LT_EVENT_RESIZE)
    {
      const int rows = ev.h > 0 ? ev.h : lt_height();
      const int cols = ev.w > 0 ? ev.w : lt_width();
      ResizeScreen(rows, cols);
      return KEY_RESIZE;
    }
    if (ev.type == LT_EVENT_MOUSE)
    {
      if (!TakeMouse(ev))
      {
        continue;
      }
      return KEY_MOUSE;
    }
    if (ev.type == LT_EVENT_KEY)
    {
      const int key = MapKey(ev);
      if (key == -2)
      {
        continue;
      }
      return key;
    }
  }
}

int getmouse(MEVENT* event)
{
  if (event == nullptr || !g_have_mouse)
  {
    return ERR;
  }
  *event = g_mouse;
  return OK;
}

mmask_t mousemask(mmask_t mask, mmask_t* old_mask)
{
  if (old_mask != nullptr)
  {
    *old_mask = g_mouse_mask;
  }
  g_mouse_mask = mask;
  return mask;
}

int mouseinterval(int interval)
{
  const int previous = g_mouse_interval_ms;
  if (interval >= 0)
  {
    g_mouse_interval_ms = interval;
  }
  return previous;
}

bool wenclose(const WINDOW* win, int y, int x)
{
  if (win == nullptr)
  {
    return false;
  }
  return y >= win->begy && x >= win->begx &&
    y < win->begy + win->rows && x < win->begx + win->cols;
}

bool wmouse_trafo(const WINDOW* win, int* y, int* x, bool to_screen)
{
  if (win == nullptr || y == nullptr || x == nullptr)
  {
    return false;
  }
  if (to_screen)
  {
    *y += win->begy;
    *x += win->begx;
    return true;
  }
  *y -= win->begy;
  *x -= win->begx;
  return *y >= 0 && *x >= 0 && *y < win->rows && *x < win->cols;
}
