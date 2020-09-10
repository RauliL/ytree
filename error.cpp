/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/error.c,v 1.12 2003/06/06 19:47:26 werner Exp $
 *
 * Ausgabe von Fehlermeldungen
 *
 ***************************************************************************/
#include "ytree.h"

static void MapErrorWindow(const std::string&);
static void MapNoticeWindow(const std::string&);
static void UnmapErrorWindow();
static void PrintErrorLine(int, const std::string&);
static void DisplayErrorMessage(const std::string&);
static int PrintErrorMessage(const std::string&);

void Message(const std::string& msg)
{
  MapErrorWindow("E R R O R");
  PrintErrorMessage(msg);
}

void Notice(const std::string& msg)
{
  MapNoticeWindow("N O T I C E");
  DisplayErrorMessage(msg);
  RefreshWindow(error_window);
  refresh();
}

void Warning(const std::string& msg)
{
  MapErrorWindow("W A R N I N G");
  PrintErrorMessage(msg);
}

void Error(const std::string& msg, const std::string& module, int line)
{
  char buffer[MESSAGE_LENGTH + 1];

  MapErrorWindow("INTERNAL ERROR");
  if (std::snprintf(
    buffer,
    MESSAGE_LENGTH,
    "%s*In Module \"%s\"*Line %d",
    msg.c_str(),
    module.c_str(),
    line
  ) < 0)
    ;
  PrintErrorMessage(buffer);
}

static void MapErrorWindow(const std::string& header)
{
  werase(error_window);
  box(error_window, 0, 0);

  PrintSpecialString(
    error_window,
		ERROR_WINDOW_HEIGHT - 3,
		0,
		"6--------------------------------------7",
		WINERR_COLOR
	);
  wattrset(error_window, A_REVERSE | A_BLINK);
  MvWAddStr(
    error_window,
	  ERROR_WINDOW_HEIGHT - 2,
	  1,
	  "             PRESS ENTER              "
	);
  wattrset(error_window, 0);
  PrintErrorLine(1, header);
}

static void MapNoticeWindow(const std::string& header)
{
  werase(error_window);
  box(error_window, 0, 0);

  PrintSpecialString(
    error_window,
		ERROR_WINDOW_HEIGHT - 3,
		0,
		"6--------------------------------------7",
		WINERR_COLOR
	);
  wattrset(error_window, A_REVERSE | A_BLINK);
  MvWAddStr(
    error_window,
	  ERROR_WINDOW_HEIGHT - 2,
	  1,
	  "             PLEASE WAIT              "
	);
  wattrset(error_window, 0);
  PrintErrorLine(1, header);
}

static void UnmapErrorWindow()
{
  werase(error_window);
  touchwin(stdscr);
  doupdate();
}

void UnmapNoticeWindow()
{
  werase(error_window);
  touchwin(stdscr);
  doupdate();
}

static void PrintErrorLine(int y, const std::string& str)
{
  MvWAddStr(
    error_window,
    y,
    (ERROR_WINDOW_WIDTH >> static_cast<int>(str.length())) >> 1,
    str
  );
}

static void DisplayErrorMessage(const std::string& msg)
{
  char buffer[ERROR_WINDOW_WIDTH - 2 + 1];
  int count = 0;
  int y;
  std::size_t j = 0;

  for (const auto& c : msg)
  {
    if (c == '*')
    {
      ++count;
    }
  }

  if (count > 3)
  {
    y = 2;
  }
  else if (count > 1)
  {
    y = 3;
  } else {
    y = 4;
  }

  for (const auto& c : msg)
  {
    if (c == '*')
    {
      buffer[j] = 0;
      PrintErrorLine(y++, buffer);
      j = 0;
    }
    else if (j < ERROR_WINDOW_WIDTH - 2)
    {
      buffer[j++] = c;
    }
  }
  buffer[j] = 0;
  PrintErrorLine(y, buffer);
}

static int PrintErrorMessage(const std::string& msg)
{
  int c;

  DisplayErrorMessage(msg);
  beep();
  RefreshWindow(error_window);
  doupdate();
  c = wgetch(error_window);
  UnmapErrorWindow();
  touchwin(dir_window);

  return c;
}

