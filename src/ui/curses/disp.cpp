#include "core/ui.hpp"
#include "core/version.hpp"
#include "ui/curses/curses_ui.hpp"
#include "ui/help.hpp"

static void PrintMenuLine(WINDOW* win, int y, int x, const std::string& line);
static void PrintLine(WINDOW *win, int y, int x, const std::string& line, int len);
static void DisplayVersion();

static const std::vector<std::string> mask =
{
  "5-----------------------2",
  "|FILE:                  |",
  "6-----------------------7",
  "|DISK:                  |",
  "| Avail                 |",
  "6-----------------------7",
  "|DISK Statistics        |",
  "|[Total]                  |",
  "| Files:                |",
  "| Bytes:                |",
  "|[Matching]               |",
  "| Files:                |",
  "| Bytes:                |",
  "|[Tagged]                 |",
  "| Files:                |",
  "| Bytes:                |",
  "|[Current Directory]      |",
  "|                       |",
  "| Bytes:                |"
};


static const char *logo[] =
{
  "                  #                       ",
  "                ##                        ",
  "     ##  ##   #####  ## ###   ####    ####",
  "    ##  ##    ##     ##  ## ##  ##  ##  ##",
  "   ##  ##    ##     ##  ## ######  ###### ",
  "   #####    ## #   ##     ##      ##      ",
  "     ##     ###  ####     ####    ####    ",
  "#####                                     "
};

static const std::string extended_line = "| |                       |";
static const std::string last_line = "3-8-----------------------4";
static const std::string first_line = "1-5";

static void DisplayVersion()
{
  const auto version = std::format(
    "ytree Version {}PL{} {} (Werner Bregulla)",
    VERSION,
    PATCHLEVEL,
    VERSIONDATE
  );

  ClearHelp();
  MvAddStr(
    LINES - 2,
    static_cast<unsigned>(COLS - version.length()) >> 1,
    version
  );
}

static void DisplayHelpLines(const HelpLines& help)
{
  for (std::size_t i = 0; i < help.size(); ++i)
  {
    PrintOptions(stdscr, LINES - 2 + static_cast<int>(i), 0, help[i]);
    clrtoeol();
  }
}

void DisplayDirHelp()
{
  DisplayHelpLines(DirHelpLines());
}

void DisplayFileHelp()
{
  DisplayHelpLines(FileHelpLines());
}

void ClearHelp()
{
  ui::dialogs().clearHelp();
}



void DisplayMenu()
{
  int y = 1;
  int    l, c;


  PrintSpecialString(stdscr, 0, 0, "Path: ", MENU_COLOR);
#ifdef COLOR_SUPPORT
  clrtoeol();
#endif

  werase(dir_window);
  werase(big_file_window);
  werase(small_file_window);

  for (const auto& line : mask)
  {
    PrintOptions(stdscr, y, 0, "|");
    PrintOptions(stdscr, y, COLS - 25 , line);
    ++y;
  }
  for (; y < LINES - 4; ++y)
  {
    PrintMenuLine(stdscr, y, 0, extended_line);
  }
  PrintLine(stdscr, DIR_WINDOW_HEIGHT + 2, 0, "6-7", COLS - 25);
  PrintLine(stdscr, 1, 0, first_line, COLS - 25);
  PrintMenuLine(stdscr, y, 0, last_line);

  l = sizeof(logo) / sizeof(logo[0]);
  c = std::strlen(logo[0]);

  for( y=0; y < l; y++ )
  {
    MvWAddStr(dir_window,
         y + ((DIR_WINDOW_HEIGHT - l) >> 1),
         (DIR_WINDOW_WIDTH - c) >> 1,
         logo[y]
    );
  }
  DisplayVersion();

  touchwin(dir_window);
  /* refresh(); */
}

void SwitchToSmallFileWindow()
{
  werase(file_window);
  PrintLine(stdscr, DIR_WINDOW_HEIGHT + 2, 0, "6-7", COLS - 25);
  file_window = small_file_window;
  RefreshWindow(stdscr);
}

void SwitchToBigFileWindow()
{
  werase(file_window);
  RefreshWindow(file_window);
#ifdef COLOR_SUPPORT
  mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X - 1,
        ACS_VLINE | COLOR_PAIR(MENU_COLOR)| A_BOLD);
  mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X + DIR_WINDOW_WIDTH,
           ACS_VLINE | COLOR_PAIR(MENU_COLOR)| A_BOLD);

#else
  mvwaddch(stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
     DIR_WINDOW_X - 1,
     ACS_VLINE
);
  mvwaddch(stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
     DIR_WINDOW_X + DIR_WINDOW_WIDTH,
     ACS_VLINE
);
#endif /* COLOR_SUPPORT */
  file_window = big_file_window;
  RefreshWindow(stdscr);
}


void MapF2Window()
{
  werase(f2_window);
  PrintSpecialString(
    f2_window,
    F2_WINDOW_HEIGHT - 1,
    0,
    std::string(F2_WINDOW_WIDTH, '='),
    HST_COLOR
  );
  RefreshWindow(f2_window);
}


void UnmapF2Window()
{
  werase(f2_window);
  if (file_window == big_file_window)
  {
#ifdef COLOR_SUPPORT
    mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X - 1,
            ACS_VLINE | COLOR_PAIR(MENU_COLOR)| A_BOLD);

    mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X + DIR_WINDOW_WIDTH,
            ACS_VLINE | COLOR_PAIR(MENU_COLOR)| A_BOLD);
#else
    mvwaddch(stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
      DIR_WINDOW_X - 1,
      ACS_VLINE
    );

    mvwaddch(stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
      DIR_WINDOW_X + DIR_WINDOW_WIDTH,
      ACS_VLINE
    );
#endif /* COLOR_SUPPORT */
  }
  touchwin(stdscr);
}

static void PrintMenuLine(WINDOW* win, int y, int x, const std::string& line)
{
  const auto has_paren = line.find('(') != std::string::npos;
  std::size_t p = has_paren ? 2 : 0;
  const std::size_t l = COLS + 2 + (has_paren ? 2 : 0);
  std::size_t i = 1;
  std::string buffer(l, '\0');

  buffer[0] = line[0];
  if (p == 0)
  {
     p = COLS - 25;
  } else {
     p = COLS - 27;
  }
  for (; i < p; i++)
  {
    buffer[i] = line[1];
  }
  std::strncpy(&buffer[i], &line[2], l - i);
  buffer[l - 1] = '\0';
  PrintOptions(stdscr, y, x , buffer);
}

static void PrintLine(
  WINDOW* win,
  int y,
  int x,
  const std::string& line,
  int len
)
{
  std::string buffer;

  if (len <= 0)
  {
    return;
  }
  buffer += line.empty() ? " " : std::string(1, line[0]);
  buffer.append(
    static_cast<std::size_t>(len - 1),
    line.size() > 1 ? line[1] : ' '
  );
  if (line.size() > 2)
  {
    buffer += line.substr(2);
  }
  PrintOptions(stdscr, y, x, buffer);
}

void RefreshWindow(WINDOW* win)
{
  wnoutrefresh(win);
}
