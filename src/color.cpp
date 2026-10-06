/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/color.c,v 1.2 1997/08/13 12:24:58 werner Rel $
 *
 * Dynamic Colors-Support
 *
 ***************************************************************************/


#include "ytree.h"


#ifdef COLOR_SUPPORT

static bool color_enabled = false;

#ifdef COLOR_THEME_GRUVBOX

/* Convert 0-255 channel to ncurses 0-1000 scale. */
#define RGB8(r, g, b) ((r) * 1000 / 255), ((g) * 1000 / 255), ((b) * 1000 / 255)

static void InitGruvboxPalette()
{
  if (!can_change_color()) {
    return;
  }

  /* Gruvbox dark (morhetz) — base ANSI colors */
  init_color(COLOR_BLACK,   RGB8(0x28, 0x28, 0x28)); /* bg0 */
  init_color(COLOR_RED,     RGB8(0xcc, 0x24, 0x1d)); /* neutral_red */
  init_color(COLOR_GREEN,   RGB8(0x98, 0x97, 0x1a)); /* neutral_green */
  init_color(COLOR_YELLOW,  RGB8(0xd7, 0x99, 0x21)); /* neutral_yellow */
  init_color(COLOR_BLUE,    RGB8(0x45, 0x85, 0x88)); /* neutral_blue */
  init_color(COLOR_MAGENTA, RGB8(0xb1, 0x62, 0x86)); /* neutral_purple */
  init_color(COLOR_CYAN,    RGB8(0x68, 0x9d, 0x6a)); /* neutral_aqua */
  init_color(COLOR_WHITE,   RGB8(0xeb, 0xdb, 0xb2)); /* fg / light1 */

  /* Bright / bold variants when the terminal exposes 16 colors */
  if (COLORS >= 16) {
    init_color(8,  RGB8(0x92, 0x83, 0x74)); /* gray */
    init_color(9,  RGB8(0xfb, 0x49, 0x34)); /* bright_red */
    init_color(10, RGB8(0xb8, 0xbb, 0x26)); /* bright_green */
    init_color(11, RGB8(0xfa, 0xbd, 0x2f)); /* bright_yellow */
    init_color(12, RGB8(0x83, 0xa5, 0x98)); /* bright_blue */
    init_color(13, RGB8(0xd3, 0x86, 0x9b)); /* bright_purple */
    init_color(14, RGB8(0x8e, 0xc0, 0x7c)); /* bright_aqua */
    init_color(15, RGB8(0xeb, 0xdb, 0xb2)); /* fg */
  }
}

#endif /* COLOR_THEME_GRUVBOX */


void StartColors()
{
  start_color();
  if ((COLORS < 8) || (COLOR_PAIRS < 19)) {
    ESCAPE; /* no color support */
  }

#ifdef COLOR_THEME_GRUVBOX
  InitGruvboxPalette();

  /* Gruvbox dark UI roles */
  init_pair(DIR_COLOR,     COLOR_WHITE,   COLOR_BLACK);
  init_pair(HIDIR_COLOR,   COLOR_BLACK,   COLOR_YELLOW);
  init_pair(WINDIR_COLOR,  COLOR_WHITE,   COLOR_BLACK);
  init_pair(FILE_COLOR,    COLOR_WHITE,   COLOR_BLACK);
  init_pair(HIFILE_COLOR,  COLOR_BLACK,   COLOR_YELLOW);
  init_pair(WINFILE_COLOR, COLOR_WHITE,   COLOR_BLACK);
  init_pair(STATS_COLOR,   COLOR_BLACK,   COLOR_CYAN);
  init_pair(WINSTATS_COLOR,COLOR_BLACK,   COLOR_CYAN);
  init_pair(BORDERS_COLOR, COLOR_YELLOW,  COLOR_BLACK);
  init_pair(HIMENUS_COLOR, COLOR_YELLOW,  COLOR_BLACK);
  init_pair(MENU_COLOR,    COLOR_WHITE,   COLOR_BLACK);
  init_pair(WINERR_COLOR,  COLOR_BLACK,   COLOR_RED);
  init_pair(HST_COLOR,     COLOR_YELLOW,  COLOR_BLACK);
  init_pair(HIHST_COLOR,   COLOR_BLACK,   COLOR_YELLOW);
  init_pair(WINHST_COLOR,  COLOR_CYAN,    COLOR_BLACK);
  init_pair(HIGLOBAL_COLOR,COLOR_BLACK,   COLOR_YELLOW);
  init_pair(GLOBAL_COLOR,  COLOR_YELLOW,  COLOR_BLACK);
#else
  init_pair(DIR_COLOR,     COLOR_WHITE,   COLOR_BLUE);
  init_pair(HIDIR_COLOR,   COLOR_BLACK,   COLOR_WHITE);
  init_pair(WINDIR_COLOR,  COLOR_CYAN,    COLOR_BLUE);
  init_pair(FILE_COLOR,    COLOR_WHITE,   COLOR_BLUE);
  init_pair(HIFILE_COLOR,  COLOR_BLACK,   COLOR_WHITE);
  init_pair(WINFILE_COLOR, COLOR_CYAN,    COLOR_BLUE);
  init_pair(STATS_COLOR,   COLOR_BLUE,    COLOR_CYAN);
  init_pair(WINSTATS_COLOR,COLOR_BLUE,    COLOR_CYAN);
  init_pair(BORDERS_COLOR, COLOR_BLUE,    COLOR_CYAN);
  init_pair(HIMENUS_COLOR, COLOR_WHITE,   COLOR_BLUE);
  init_pair(MENU_COLOR,    COLOR_CYAN,    COLOR_BLUE);
  init_pair(WINERR_COLOR,  COLOR_BLUE,    COLOR_WHITE);
  init_pair(HST_COLOR,     COLOR_YELLOW,  COLOR_CYAN);
  init_pair(HIHST_COLOR,   COLOR_WHITE,   COLOR_WHITE);
  init_pair(WINHST_COLOR,  COLOR_YELLOW,  COLOR_CYAN);
  init_pair(HIGLOBAL_COLOR,COLOR_BLUE,    COLOR_WHITE);
  init_pair(GLOBAL_COLOR,  COLOR_YELLOW,  COLOR_CYAN);
#endif /* COLOR_THEME_GRUVBOX */

  color_enabled = true;
FNC_XIT: ;

}



void WbkgdSet(WINDOW *w, chtype c)
{
  if(color_enabled) {
    wbkgdset(w, c);
  } else {
    c &= ~A_BOLD;
    if(c == COLOR_PAIR(HIDIR_COLOR)   ||
       c == COLOR_PAIR(HIFILE_COLOR)  ||
       c == COLOR_PAIR(HISTATS_COLOR) ||
       c == COLOR_PAIR(HIMENUS_COLOR) ||
       c == COLOR_PAIR(HIHST_COLOR)) {

      wattrset(w, A_REVERSE);
    } else {
      wattrset(w, 0);
    }
  }
}


#endif /* COLOR_SUPPORT */
