#include "ytree.h"

#include <cstdarg>

static void MapErrorWindow(const std::string& header);
static void MapNoticeWindow(const std::string& header);
static void UnmapErrorWindow();
static inline void PrintErrorLine(int y, const std::string& str);
static void DisplayErrorMessage(const std::string& msg);
static void PrintErrorMessage(const std::string& msg);

static char message[MESSAGE_LENGTH + 1];

void Message(const std::string& msg)
{
  MapErrorWindow("E R R O R");
  PrintErrorMessage(msg);
}

void MessagePrintf(const char* format, ...)
{
  std::va_list args;

  va_start(args, format);
  std::vsnprintf(
    message,
    MESSAGE_LENGTH,
    format,
    args
  );
  va_end(args);
  Message(message);
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

void WarningPrintf(const char* format, ...)
{
  std::va_list args;

  va_start(args, format);
  std::vsnprintf(
    message,
    MESSAGE_LENGTH,
    format,
    args
  );
  va_end(args);
  Warning(message);
}

void ErrorEx(const std::string& msg, const std::string& module, int line)
{
  MapErrorWindow("INTERNAL ERROR");
  PrintErrorMessage(
    msg +
    "*In Module \"" +
    module +
    "\"*Line " +
    std::to_string(line)
  );
}

void ErrorPrintfEx(const char* format, const char* module, int line, ...)
{
  std::va_list args;

  va_start(args, line);
  std::vsnprintf(
    message,
    MESSAGE_LENGTH,
    format,
    args
  );
  va_end(args);
  ErrorEx(message, module, line);
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

static inline void PrintErrorLine(int y, const std::string& str)
{
  MvWAddStr(
    error_window,
    y,
    (ERROR_WINDOW_WIDTH - str.length()) >> 1,
    str
  );
}

static void DisplayErrorMessage(const std::string& msg)
{
  int count = 0;
  int y;
  int j = 0;
  char buffer[ERROR_WINDOW_WIDTH - 2 + 1];

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
    else if (j < static_cast<int>(sizeof(buffer) - 1))
    {
      buffer[j++] = c;
    }
  }
  buffer[j] = 0;
  PrintErrorLine(y, buffer);
}

static void PrintErrorMessage(const std::string& msg)
{
  DisplayErrorMessage(msg);
  beep();
  RefreshWindow(error_window);
  doupdate();
  wgetch(error_window);
  UnmapErrorWindow();
  touchwin(dir_window);
}
