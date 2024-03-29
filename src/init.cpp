/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/init.c,v 1.20 2003/08/31 11:11:00 werner Exp $
 *
 * Initialisierungen
 *
 ***************************************************************************/


#include "ytree.h"


static WINDOW *Subwin(WINDOW *orig, int nlines, int ncols,
                      int begin_y, int begin_x);
static WINDOW *Newwin(int nlines, int ncols,
                      int begin_y, int begin_x);

int Init(
  const std::optional<std::string>& configuration_file,
  const std::optional<std::string>& history_file
)
{

  user_umask = umask(0);
  initscr();
  StartColors(); /* even on b/w terminals... */

  cbreak();
  noecho();
  nonl();
  raw();
  keypad( stdscr, true );
  clearok(stdscr, true);
  leaveok(stdscr,false);
  curs_set(0);


  WbkgdSet( stdscr, COLOR_PAIR(WINDIR_COLOR)|A_BOLD);

  ReCreateWindows();

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );

  file_window = small_file_window;

  ReadProfile(configuration_file);
  ReadHistory(history_file);

  SetFileMode( strtod(FILEMODE, NULL) );
  SetKindOfSort( SORT_BY_NAME );
  number_seperator = *(NUMBERSEP);
  bypass_small_window = (bool)strtod(NOSMALLWINDOW, NULL );
  initial_directory = INITIALDIR;

  InitClock();

  return( 0 );
}



void ReCreateWindows()
{
  bool is_small;

  is_small = (file_window == small_file_window) ? true : false;


  if(dir_window)
    delwin(dir_window);

  dir_window = Subwin( stdscr,
		       DIR_WINDOW_HEIGHT,
		       DIR_WINDOW_WIDTH,
		       DIR_WINDOW_Y,
		       DIR_WINDOW_X
		      );

  keypad( dir_window, true );
  scrollok( dir_window, true );
  clearok( dir_window, true);
  leaveok(dir_window, true);
  WbkgdSet( dir_window, COLOR_PAIR(WINDIR_COLOR) );

  if(small_file_window)
    delwin(small_file_window);

  small_file_window = Subwin( stdscr,
			      FILE_WINDOW_1_HEIGHT,
			      FILE_WINDOW_1_WIDTH,
			      FILE_WINDOW_1_Y,
		              FILE_WINDOW_1_X
		           );

  if(!small_file_window)
    beep();

  keypad( small_file_window, true );
  clearok(small_file_window, true);
  leaveok(small_file_window, true);

  WbkgdSet(small_file_window, COLOR_PAIR(WINFILE_COLOR));

  if(big_file_window)
    delwin(big_file_window);

  big_file_window = Subwin( stdscr,
			    FILE_WINDOW_2_HEIGHT,
			    FILE_WINDOW_2_WIDTH,
			    FILE_WINDOW_2_Y,
		            FILE_WINDOW_2_X
		          );

  keypad( big_file_window, true );
  clearok(big_file_window, true);
  leaveok(big_file_window, true);
  WbkgdSet(big_file_window, COLOR_PAIR(WINFILE_COLOR));

  if(error_window)
    delwin(error_window);

  error_window = Newwin(
		       ERROR_WINDOW_HEIGHT,
		       ERROR_WINDOW_WIDTH,
		       ERROR_WINDOW_Y,
		       ERROR_WINDOW_X
		      );
  WbkgdSet(error_window, COLOR_PAIR(WINERR_COLOR));
  clearok(error_window, true);
  leaveok(error_window, true);


#ifdef CLOCK_SUPPORT

  if(time_window)
    delwin(time_window);

  time_window = Subwin( stdscr,
                      TIME_WINDOW_HEIGHT,
                      TIME_WINDOW_WIDTH,
                      TIME_WINDOW_Y,
                      TIME_WINDOW_X
                    );
  clearok( time_window, true );
  scrollok( time_window, false );
  leaveok( time_window, true );
  WbkgdSet( time_window, COLOR_PAIR(WINDIR_COLOR|A_BOLD) );
  immedok(time_window, true);
#endif


  if(history_window)
    delwin(history_window);

  history_window = Newwin(
                       HISTORY_WINDOW_HEIGHT,
                       HISTORY_WINDOW_WIDTH,
                       HISTORY_WINDOW_Y,
                       HISTORY_WINDOW_X
                      );
  scrollok(history_window, true);
  clearok(history_window, true );
  leaveok(history_window, true);
  WbkgdSet(history_window, COLOR_PAIR(WINHST_COLOR));

  matches_window = history_window;

  if(f2_window)
    delwin(f2_window);

  f2_window = Newwin( F2_WINDOW_HEIGHT,
                      F2_WINDOW_WIDTH,
                      F2_WINDOW_Y,
                      F2_WINDOW_X
                    );

  keypad( f2_window, true );
  scrollok( f2_window, false );
  clearok( f2_window, true);
  leaveok( f2_window, true );
  WbkgdSet( f2_window, COLOR_PAIR(WINHST_COLOR) );

  file_window = (is_small) ? small_file_window : big_file_window;

  clear();
}


static WINDOW *Subwin(WINDOW *orig, int nlines, int ncols,
                      int begin_y, int begin_x)
{
  int x, y, h, w;
  WINDOW *win;

  if(nlines > LINES)   nlines = LINES;
  if(ncols > COLS)     ncols = COLS;

  h = MAXIMUM(nlines, 1);
  w = MAXIMUM(ncols, 1);
  x = MAXIMUM(begin_x, 0);
  y = MAXIMUM(begin_y, 0);

  if(x+w > COLS)  x = COLS - w;
  if(y+h > LINES) y = LINES - h;

  win = subwin(orig, h, w, y, x);

  return(win);
}



static WINDOW *Newwin(int nlines, int ncols,
                      int begin_y, int begin_x)
{
  int x, y, h, w;
  WINDOW *win;

  if(nlines > LINES)   nlines = LINES;
  if(ncols > COLS)     ncols = COLS;

  h = MAXIMUM(nlines, 1);
  w = MAXIMUM(ncols, 1);
  x = MAXIMUM(begin_x, 0);
  y = MAXIMUM(begin_y, 0);

  if(x+w > COLS)  x = COLS - w;
  if(y+h > LINES) y = LINES - h;

  win = newwin(h, w, y, x);

  return(win);
}



