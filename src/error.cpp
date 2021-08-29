#include "ytree.h"

#include <cstdarg>

static void MapErrorWindow(const char* header);
static void MapNoticeWindow(const char* header);
static void UnmapErrorWindow();
static void PrintErrorLine(int y, const char* str);
static void DisplayErrorMessage(const char* msg);
static int PrintErrorMessage(const char* msg);

static char message[MESSAGE_LENGTH + 1];

void Message(const char* msg)
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

void Notice(const char* msg)
{
  MapNoticeWindow("N O T I C E");
  DisplayErrorMessage(msg);
  RefreshWindow(error_window);
  refresh();
}

void Warning(const char* msg)
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

void Error(const char* msg, const char* module, int line)
{
  char buffer[MESSAGE_LENGTH + 1];

  MapErrorWindow("INTERNAL ERROR");
  std::snprintf(
    buffer,
    MESSAGE_LENGTH,
    "%s*In Module \"%s\"*Line %d",
    msg,
    module,
    line
  );
  PrintErrorMessage(buffer);
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
  Error(message, module, line);
}

static void MapErrorWindow(const char *header)
{
   werase( error_window );
   box( error_window, 0, 0 );

   PrintSpecialString( error_window,
		       ERROR_WINDOW_HEIGHT - 3,
		       0,
		       "6--------------------------------------7",
		       WINERR_COLOR
		     );
   wattrset( error_window, A_REVERSE | A_BLINK );
   MvWAddStr( error_window,
	      ERROR_WINDOW_HEIGHT - 2,
	      1,
	      "             PRESS ENTER              "
	    );
   wattrset( error_window, 0 );
   PrintErrorLine( 1, header );
}


static void MapNoticeWindow(const char *header)
{
   werase( error_window );
   box( error_window, 0, 0 );

   PrintSpecialString( error_window,
		       ERROR_WINDOW_HEIGHT - 3,
		       0,
		       "6--------------------------------------7",
		       WINERR_COLOR
		     );
   wattrset( error_window, A_REVERSE | A_BLINK );
   MvWAddStr( error_window,
	      ERROR_WINDOW_HEIGHT - 2,
	      1,
	      "             PLEASE WAIT              "
	    );
   wattrset( error_window, 0 );
   PrintErrorLine( 1, header );
}


static void UnmapErrorWindow(void)
{
   werase( error_window );
   touchwin( stdscr );
   doupdate();
}


void UnmapNoticeWindow(void)
{
   werase( error_window );
   touchwin( stdscr );
   doupdate();
}



static void PrintErrorLine(int y, const char *str)
{
  int l;

  l = strlen( str );

  MvWAddStr( error_window, y, (ERROR_WINDOW_WIDTH - l) >> 1, str );
}




static void DisplayErrorMessage(const char *msg)
{
  int  y, i, j, count;
  char buffer[ERROR_WINDOW_WIDTH - 2 + 1];

  for(i=0, count=0; msg[i]; i++)
    if( msg[i] == '*' ) count++;

  if( count > 3 )      y = 2;
  else if( count > 1 ) y = 3;
  else                 y = 4;


  for( i=0,j=0; msg[i]; i++ )
  {
    if( msg[i] == '*' )
    {
      buffer[j] = '\0';
      PrintErrorLine( y++, buffer );
      j=0;
    }
    else
    {
      if( j < (int)((sizeof( buffer) - 1)) ) buffer[j++] = msg[i];
    }
  }
  buffer[j] = '\0';
  PrintErrorLine( y, buffer );
}



static int PrintErrorMessage(const char *msg)
{
  int c;

  DisplayErrorMessage( msg );
  beep();
  RefreshWindow( error_window );
  doupdate();
  c = wgetch(error_window);
  UnmapErrorWindow();
  touchwin( dir_window );
  return( c );
}

