#pragma once

/**
 * ncurses binding and UI layout constants.
 * UI translation units include this (usually via ytree.hpp).
 * Core should prefer core/ui.hpp ports over these types.
 */

#include "config.hpp"

#if defined(WITH_UTF8) && __has_include(<ncursesw/ncurses.h>)
# include <ncursesw/ncurses.h>
#elif defined(WITH_UTF8) && __has_include(<ncursesw/curses.h>)
# include <ncursesw/curses.h>
#elif defined(CURSES_HAVE_NCURSES_NCURSES_H)
# include <ncurses/ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_H)
# include <ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_CURSES_H)
# include <ncurses/curses.h>
#else
# include <curses.h>
#endif

#ifndef KEY_BTAB
#define KEY_BTAB  0x1d
#endif

#ifndef KEY_END
#define KEY_END   KEY_EOL
#endif

/* Line-drawing fallbacks when ACS_* is unavailable */
#ifndef ACS_ULCORNER
#define ACS_ULCORNER '+'
#endif
#ifndef ACS_URCORNER
#define ACS_URCORNER '+'
#endif
#ifndef ACS_LLCORNER
#define ACS_LLCORNER '+'
#endif
#ifndef ACS_LRCORNER
#define ACS_LRCORNER '+'
#endif
#ifndef ACS_VLINE
#define ACS_VLINE    '|'
#endif
#ifndef ACS_HLINE
#define ACS_HLINE    '-'
#endif
#ifndef ACS_RTEE
#define ACS_RTEE    '+'
#endif
#ifndef ACS_LTEE
#define ACS_LTEE    '+'
#endif
#ifndef ACS_BTEE
#define ACS_BTEE    '+'
#endif
#ifndef ACS_TTEE
#define ACS_TTEE    '+'
#endif
#ifndef ACS_BLOCK
#define ACS_BLOCK   '?'
#endif
#ifndef ACS_LARROW
#define ACS_LARROW  '<'
#endif

#define DIR_COLOR        1
#define FILE_COLOR       2
#define STATS_COLOR      3
#define BORDERS_COLOR    4
#define MENU_COLOR       5
#define WINDIR_COLOR     6
#define WINFILE_COLOR    7
#define WINSTATS_COLOR   8
#define WINERR_COLOR     9
#define HIDIR_COLOR     10
#define HIFILE_COLOR    11
#define HISTATS_COLOR   12
#define HIMENUS_COLOR   13
#define WINHST_COLOR    14
#define HST_COLOR       15
#define HIHST_COLOR     16
#define WINMTCH_COLOR   14
#define MTCH_COLOR      15
#define HIMTCH_COLOR    16
#define GLOBAL_COLOR    17
#define HIGLOBAL_COLOR  18

#define DIR_WINDOW_X         1
#define DIR_WINDOW_Y         2
#define DIR_WINDOW_WIDTH     (COLS - 26)
#define DIR_WINDOW_HEIGHT    ((LINES * 8 / 14)-1)

#define F2_WINDOW_X          DIR_WINDOW_X
#define F2_WINDOW_Y          DIR_WINDOW_Y
#define F2_WINDOW_WIDTH      DIR_WINDOW_WIDTH
#define F2_WINDOW_HEIGHT     (DIR_WINDOW_HEIGHT + 1)

#define FILE_WINDOW_1_X      1
#define FILE_WINDOW_1_Y      DIR_WINDOW_HEIGHT + 3
#define FILE_WINDOW_1_WIDTH  (COLS - 26)
#define FILE_WINDOW_1_HEIGHT (LINES - DIR_WINDOW_HEIGHT - 7 )

#define FILE_WINDOW_2_X      1
#define FILE_WINDOW_2_Y      2
#define FILE_WINDOW_2_WIDTH  (COLS - 26)
#define FILE_WINDOW_2_HEIGHT (LINES - 6)

#define ERROR_WINDOW_WIDTH   40
#define ERROR_WINDOW_HEIGHT  10
#define ERROR_WINDOW_X       ((COLS - ERROR_WINDOW_WIDTH) >> 1)
#define ERROR_WINDOW_Y       ((LINES - ERROR_WINDOW_HEIGHT) >> 1)

#define HISTORY_WINDOW_X       1
#define HISTORY_WINDOW_Y       2
#define HISTORY_WINDOW_WIDTH   (COLS - 26)
#define HISTORY_WINDOW_HEIGHT  (LINES - 6)

#define TIME_WINDOW_X        ((COLS > 20) ? (COLS - 20) : 1)
#define TIME_WINDOW_Y        1
#define TIME_WINDOW_WIDTH    ((COLS > 15) ? 15 : COLS)
#define TIME_WINDOW_HEIGHT   1

#ifdef COLOR_SUPPORT
extern void StartColors();
extern void WbkgdSet(WINDOW *w, chtype c);
#else
#define StartColors() ;
#define WbkgdSet(a, b)  ;
#endif /* COLOR_SUPPORT */

extern WINDOW *dir_window;
extern WINDOW *small_file_window;
extern WINDOW *big_file_window;
extern WINDOW *file_window;
extern WINDOW *error_window;
extern WINDOW *history_window;
extern WINDOW *matches_window;
extern WINDOW *f2_window;
extern WINDOW *time_window;
