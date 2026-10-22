#pragma once

/**
 * Curses-shaped terminal surface implemented on libterm.
 * Covers the window, color, input, and mouse calls the ytree TUI uses.
 */

#include <cstdint>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef OK
#define OK 0
#endif
#ifndef ERR
#define ERR (-1)
#endif

using chtype = std::uint32_t;

#define A_NORMAL     0u
#define A_BOLD       (1u << 21)
#define A_REVERSE    (1u << 22)
#define A_BLINK      (1u << 23)
#define A_DIM        (1u << 24)
#define A_UNDERLINE  (1u << 25)
#define A_ALTCHARSET (1u << 26)
#define A_COLOR      (0x1Fu << 16)
#define A_CHARTEXT   0xFFFFu

#define COLOR_PAIR(n) (static_cast<chtype>((n) & 0x1F) << 16)

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

#define ACS_ULCORNER (A_ALTCHARSET | 0x250Cu)
#define ACS_URCORNER (A_ALTCHARSET | 0x2510u)
#define ACS_LLCORNER (A_ALTCHARSET | 0x2514u)
#define ACS_LRCORNER (A_ALTCHARSET | 0x2518u)
#define ACS_LTEE     (A_ALTCHARSET | 0x251Cu)
#define ACS_RTEE     (A_ALTCHARSET | 0x2524u)
#define ACS_BTEE     (A_ALTCHARSET | 0x2534u)
#define ACS_TTEE     (A_ALTCHARSET | 0x252Cu)
#define ACS_HLINE    (A_ALTCHARSET | 0x2500u)
#define ACS_VLINE    (A_ALTCHARSET | 0x2502u)
#define ACS_LARROW   (A_ALTCHARSET | 0x2190u)
#define ACS_BLOCK    (A_ALTCHARSET | 0x2588u)

/* Values match historical ncurses key codes. */
#define KEY_CODE_YES  0400
#define KEY_DOWN      0402
#define KEY_UP        0403
#define KEY_LEFT      0404
#define KEY_RIGHT     0405
#define KEY_HOME      0406
#define KEY_BACKSPACE 0407
#define KEY_F0        0410
#define KEY_F(n)      (KEY_F0 + (n))
#define KEY_DL        0510
#define KEY_DC        0512
#define KEY_IC        0513
#define KEY_EIC       0514
#define KEY_EOL       0517
#define KEY_NPAGE     0522
#define KEY_PPAGE     0523
#define KEY_BTAB      0541
#define KEY_END       0550
#define KEY_SLEFT     0611
#define KEY_SRIGHT    0622
#define KEY_MOUSE     0631
#define KEY_RESIZE    0632

#define NCURSES_MOUSE_VERSION 2

#define BUTTON1_CLICKED        (1ul << 0)
#define BUTTON1_DOUBLE_CLICKED (1ul << 1)
#define BUTTON3_CLICKED        (1ul << 2)
#define BUTTON4_PRESSED        (1ul << 3)
#define BUTTON5_PRESSED        (1ul << 4)

using mmask_t = unsigned long;

struct MEVENT
{
  short id;
  int x;
  int y;
  int z;
  mmask_t bstate;
};

struct Window;
using WINDOW = Window;

extern WINDOW* stdscr;
extern int LINES;
extern int COLS;
extern int COLORS;
extern int COLOR_PAIRS;

WINDOW* initscr();
int endwin();
bool isendwin();
int refresh();
int wrefresh(WINDOW* win);
int wnoutrefresh(WINDOW* win);
int doupdate();

WINDOW* newwin(int nlines, int ncols, int begin_y, int begin_x);
WINDOW* subwin(WINDOW* orig, int nlines, int ncols, int begin_y, int begin_x);
int delwin(WINDOW* win);

int cbreak();
int noecho();
int nonl();
int raw();
int keypad(WINDOW* win, int bf);
int nodelay(WINDOW* win, int bf);
int scrollok(WINDOW* win, int bf);
int clearok(WINDOW* win, int bf);
int leaveok(WINDOW* win, int bf);
int immedok(WINDOW* win, int bf);
int curs_set(int visibility);
int typeahead(int fd);
int baudrate();
int reset_shell_mode();

int start_color();
int init_pair(short pair, short fg, short bg);
int init_color(short color, short r, short g, short b);
int can_change_color();

int wbkgdset(WINDOW* win, chtype ch);
int wattrset(WINDOW* win, chtype attrs);

int werase(WINDOW* win);
int wclear(WINDOW* win);
int clear();
int wmove(WINDOW* win, int y, int x);
int wclrtoeol(WINDOW* win);
int waddch(WINDOW* win, chtype ch);
int waddstr(WINDOW* win, const char* text);
int mvwaddstr(WINDOW* win, int y, int x, const char* text);
int mvwprintw(WINDOW* win, int y, int x, const char* fmt, ...);
int mvprintw(int y, int x, const char* fmt, ...);
int echochar(chtype ch);
int box(WINDOW* win, chtype verch, chtype horch);
int wscrl(WINDOW* win, int n);
int scroll(WINDOW* win);
int winsertln(WINDOW* win);
int touchwin(WINDOW* win);
int beep();

int wgetch(WINDOW* win);
int getmouse(MEVENT* event);
mmask_t mousemask(mmask_t mask, mmask_t* old_mask);
int mouseinterval(int interval);
bool wenclose(const WINDOW* win, int y, int x);
bool wmouse_trafo(const WINDOW* win, int* y, int* x, bool to_screen);

inline int getch()
{
  return wgetch(stdscr);
}

inline int addch(chtype ch)
{
  return waddch(stdscr, ch);
}

inline int move(int y, int x)
{
  return wmove(stdscr, y, x);
}

inline int clrtoeol()
{
  return wclrtoeol(stdscr);
}

inline int mvwaddch(WINDOW* win, int y, int x, chtype ch)
{
  if (wmove(win, y, x) == ERR)
  {
    return ERR;
  }
  return waddch(win, ch);
}

inline int mvaddch(int y, int x, chtype ch)
{
  return mvwaddch(stdscr, y, x, ch);
}

inline int mvaddstr(int y, int x, const char* text)
{
  return mvwaddstr(stdscr, y, x, text);
}
